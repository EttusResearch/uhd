//
// Copyright 2013-2015 Ettus Research LLC
//

/* This file defines the application that runs on the Cypress FX3 device, and
 * enables the user to program the FPGA with an FPGA image.  Since the FPGA
 * doesn't yet have a clock, the image must be bit-banged into the FPGA.
 */

#include <stdarg.h>
#include <stdio.h>

#include "b200_main.h"
#include "b200_gpifconfig.h"
#include "b200_i2c.h"

#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3gpif.h"
#include "cyu3gpio.h"
#include "cyu3spi.h"
#include "cyu3os.h"
#include "cyu3pib.h"
#include "cyu3system.h"
#include "cyu3usb.h"
#include "cyu3utils.h"
#include "cyfxversion.h"
#include "pib_regs.h"

#define STATIC_SAVER static // Save stack space for variables in a non-re-entrant function (e.g. USB setup callback)

/*
 * WARNING: Before you enable any of the features below, please read the comments on the same line for that feature!
 *          Indented features must have the parent feature enabled as well.
 */

#define HAS_HEAP                    // This requires memory to be set aside for the heap (e.g. required for printing floating-point numbers). You can apply the accompanying patch ('fx3_mem_map.patch') to fx3.ld & cyfxtx.c to create one.
#define ENABLE_MSG                  // This will cause the compiled code to exceed the default text memory area (SYS_MEM). You can apply the accompanying patch ('fx3_mem_map.patch') to fx3.ld & cyfxtx.c to resize the memory map so it will fit.
#define ENABLE_MANUAL_DMA_XFER      // Allows us to set it from the host (using the side channel util), doesn't auto-enable it
//#define   ENABLE_P2U_SUSP_EOP
#define   ENABLE_MANUAL_DMA_XFER_FROM_HOST
#define   ENABLE_MANUAL_DMA_XFER_TO_HOST
//#define   ENABLE_DMA_BUFFER_PACKET_DEBUG
#define ENABLE_FPGA_SB              // Be careful: this will add an ever-so-slight delay to some operations (e.g. AD3961 tune)
#define ENABLE_RE_ENUM_THREAD
#define ENABLE_USB_EVENT_LOGGING
//#define PREVENT_LOW_POWER_MODE
/*#define ENABLE_INIT_B_WORKAROUND    // This should only be enabled if you have a board where the FPGA INIT_B line is broken, but the FPGA is known to work*/
/*#define ENABLE_DONE_WORKAROUND      // This should only be enabled if you have a board where the FPGA DONE line is broken, but the FPGA is known to work*/

#define WATCHDOG_TIMEOUT                1500
#define CHECK_POWER_STATE_SLEEP_TIME    500  // Should be less than WATCHDOG_TIMEOUT

#define FPGA_PROGRAMMING_POLL_SLEEP                 10  // ticks
#define FPGA_PROGRAMMING_BITSTREAM_START_POLL_COUNT 250 // ~2.5 secs
#define FPGA_PROGRAMMING_INITB_POLL_COUNT           100 // ~1 sec
#define FPGA_PROGRAMMING_DONE_POLL_COUNT            250 // ~2.5 secs  // This is the interval *after* no FPGA programming activity has been detected

#define FPGA_RESET_SETTLING_TIME    (1*10)  // ~10ms (for SB to initialise)

#define RE_ENUM_THREAD_SLEEP_TIME   100
#define KEEP_ALIVE_LOOP_COUNT       200

#pragma message "----------------------"

#ifdef ENABLE_MSG
#pragma message "msg enabled"
#else
#pragma message "msg disabled"
#endif // ENABLE_MSG

#ifdef ENABLE_MANUAL_DMA_XFER
#pragma message "Manual DMA transfers"

#ifdef ENABLE_MANUAL_DMA_XFER_FROM_HOST
#pragma message "   -> From host"
#endif // ENABLE_MANUAL_DMA_XFER_FROM_HOST

#ifdef ENABLE_MANUAL_DMA_XFER_TO_HOST
#pragma message "   <- To host"
#endif // ENABLE_MANUAL_DMA_XFER_TO_HOST

#ifdef ENABLE_DMA_BUFFER_PACKET_DEBUG
#pragma message "   Packet debugging enabled"
#endif // ENABLE_DMA_BUFFER_PACKET_DEBUG

#else
#pragma message "Auto DMA transfers"
#endif // ENABLE_MANUAL_DMA_XFER

#ifdef ENABLE_FPGA_SB
#pragma message "FPGA Settings Bus enabled"
#else
#pragma message "FPGA Settings Bus disabled"
#endif // ENABLE_FPGA_SB

#ifdef ENABLE_RE_ENUM_THREAD
#pragma message "Re-enumeration & statistics thread enabled"
#else
#pragma message "Re-enumeration & statistics thread disabled"
#endif // ENABLE_RE_ENUM_THREAD

#ifdef ENABLE_USB_EVENT_LOGGING
#pragma message "USB event logging enabled"
#else
#pragma message "USB event logging disabled"
#endif // ENABLE_USB_EVENT_LOGGING

#ifdef PREVENT_LOW_POWER_MODE
#pragma message "Preventing Low Power Mode"
#else
#pragma message "Allowing Low Power Mode"
#endif // PREVENT_LOW_POWER_MODE

#ifdef HAS_HEAP
#pragma message "Heap enabled"
#else
#pragma message "Heap disabled"
#endif // HAS_HEAP

#ifdef ENABLE_INIT_B_WORKAROUND
#pragma message "INIT_B workaround enabled"
#else
#pragma message "INIT_B workaround disabled"
#endif // ENABLE_INIT_B_WORKAROUND

#ifdef ENABLE_DONE_WORKAROUND
#pragma message "DONE workaround enabled"
#else
#pragma message "DONE workaround disabled"
#endif // ENABLE_DONE_WORKAROUND

#pragma message "----------------------"

/* Declare global & static fields for our bit-bang application. */
static CyU3PDmaChannel data_cons_to_prod_chan_handle;
static CyU3PDmaChannel data_prod_to_cons_chan_handle;

static CyU3PDmaChannel ctrl_cons_to_prod_chan_handle;
static CyU3PDmaChannel ctrl_prod_to_cons_chan_handle;

static CyU3PEvent g_event_usb_config;
static CyU3PThread thread_main_app;
static CyU3PThread thread_fpga_config;
#ifdef ENABLE_RE_ENUM_THREAD
static CyU3PThread thread_re_enum;
#endif // ENABLE_RE_ENUM_THREAD

static CyBool_t g_app_running = CyFalse;
static uint8_t g_fx3_state = STATE_UNDEFINED;

#define USB2_VREQ_BUF_SIZE          64
#define USB3_VREQ_BUF_SIZE          512
#define MIN_VREQ_BUF_SIZE           USB2_VREQ_BUF_SIZE
#define MAX_VREQ_BUF_SIZE           USB3_VREQ_BUF_SIZE

static uint16_t g_vendor_req_buff_size = MIN_VREQ_BUF_SIZE;
static uint8_t g_vendor_req_buffer[MAX_VREQ_BUF_SIZE] __attribute__ ((aligned (32)));
static uint16_t g_vendor_req_read_count = 0;

static uint8_t fpga_hash[4] __attribute__ ((aligned (32)));
static uint8_t fw_hash[4] __attribute__ ((aligned (32)));
static uint8_t compat_num[2];
static uint32_t g_fpga_programming_write_count = 0;

#define COUNTER_MAGIC       0x10024001
#define LOG_BUFFER_SIZE     /*MAX_VREQ_BUF_SIZE*/1024	// [Max vreq @ USB3 (64 @ USB2)] Can be larger
static char log_buffer[LOG_BUFFER_SIZE];
static char log_contiguous_buffer[LOG_BUFFER_SIZE];
static int log_buffer_idx = 0, log_buffer_len = 0;
#ifdef ENABLE_MSG
static int log_count = 0;
#endif // ENABLE_MSG

#define USB_EVENT_LOG_SIZE  64
static uint8_t g_usb_event_log[USB_EVENT_LOG_SIZE];
static uint16_t g_last_usb_event_log_index = 0;
static uint8_t g_usb_event_log_contiguous_buf[USB_EVENT_LOG_SIZE];

#ifdef ENABLE_FPGA_SB
static CyBool_t g_fpga_sb_enabled = CyFalse;
//static uint16_t g_fpga_sb_uart_div = 434*2;
static uint16_t g_fpga_sb_last_usb_event_log_index = 0;
static CyU3PThread thread_fpga_sb_poll;
static CyU3PMutex g_suart_lock;
#endif // ENABLE_FPGA_SB

static CyU3PMutex g_log_lock, g_counters_lock, g_counters_dma_from_host_lock, g_counters_dma_to_host_lock;

#define FPGA_SB_UART_ADDR_BASE  0x00

enum UARTRegs
{
    SUART_CLKDIV,
    SUART_TXLEVEL,
    SUART_RXLEVEL,
    SUART_TXCHAR,
    SUART_RXCHAR
};

enum UARTPacketType
{
    UPT_NONE        = '\0',
    UPT_MSG         = ' ',
    UPT_COUNTERS    = 'C',
    UPT_USB_EVENTS  = 'U',
};

enum ConfigFlags {
    CF_NONE                 = 0,
    CF_TX_SWING             = 1 << 0,
    CF_TX_DEEMPHASIS        = 1 << 1,
    CF_DISABLE_USB2         = 1 << 2,
    CF_ENABLE_AS_SUPERSPEED = 1 << 3,
    CF_PPORT_DRIVE_STRENGTH = 1 << 4,
    CF_DMA_BUFFER_SIZE      = 1 << 5,
    CF_DMA_BUFFER_COUNT     = 1 << 6,
    CF_MANUAL_DMA           = 1 << 7,
    CF_SB_BAUD_DIV          = 1 << 8,

    CF_RE_ENUM              = 1 << 31
};

typedef struct Config {
    int tx_swing;               // [90] [65] 45
    int tx_deemphasis;          // 0x11
    int disable_usb2;           // 0
    int enable_as_superspeed;   // 1
    int pport_drive_strength;   // CY_U3P_DS_THREE_QUARTER_STRENGTH
    int dma_buffer_size;        // [USB3] (max)
    int dma_buffer_count;       // [USB3] 2
    int manual_dma;             // 0
    int sb_baud_div;            // 434*2
} CONFIG, *PCONFIG;

typedef struct ConfigMod {
    int flags;
    CONFIG config;
} CONFIG_MOD, *PCONFIG_MOD;

static CONFIG g_config = {
    65,     // tx_swing
    0x11,   // tx_deemphasis
    0,      // disable_usb2
    1,      // enable_as_superspeed
    CY_U3P_DS_THREE_QUARTER_STRENGTH,   // pport_drive_strength
    16*1024,  // dma_buffer_size - optimized value from Cypress AN86947
    2,      // dma_buffer_count - optimized value from Cypress AN86947
    0,      // manual_dma
    434*2   // sb_baud_div
};
static CONFIG_MOD g_config_mod;

#define REG_LNK_PHY_ERROR_STATUS 0xE0033044

enum PhyErrors {
    PHYERR_PHY_LOCK_EV             = 1 << 8,
    PHYERR_TRAINING_ERROR_EV       = 1 << 7,
    PHYERR_RX_ERROR_CRC32_EV       = 1 << 6,
    PHYERR_RX_ERROR_CRC16_EV       = 1 << 5,
    PHYERR_RX_ERROR_CRC5_EV        = 1 << 4,
    PHYERR_PHY_ERROR_DISPARITY_EV  = 1 << 3,
    PHYERR_PHY_ERROR_EB_UND_EV     = 1 << 2,
    PHYERR_PHY_ERROR_EB_OVR_EV     = 1 << 1,
    PHYERR_PHY_ERROR_DECODE_EV     = 1 << 0,

    PHYERR_MAX                     = PHYERR_PHY_LOCK_EV,
    PHYERR_MASK                    = (PHYERR_MAX << 1) - 1
};

typedef struct USBErrorCounters {
    int phy_error_count;
    int link_error_count;

    int PHY_LOCK_EV;
    int TRAINING_ERROR_EV;
    int RX_ERROR_CRC32_EV;
    int RX_ERROR_CRC16_EV;
    int RX_ERROR_CRC5_EV;
    int PHY_ERROR_DISPARITY_EV;
    int PHY_ERROR_EB_UND_EV;
    int PHY_ERROR_EB_OVR_EV;
    int PHY_ERROR_DECODE_EV;
} USB_ERROR_COUNTERS, *PUSB_ERROR_COUNTERS;

typedef struct DMACounters {
    int XFER_CPLT;
    int SEND_CPLT;
    int RECV_CPLT;
    int PROD_EVENT;
    int CONS_EVENT;
    int ABORTED;
    int ERROR;
    int PROD_SUSP;
    int CONS_SUSP;

    int BUFFER_MARKER;
    int BUFFER_EOP;
    int BUFFER_ERROR;
    int BUFFER_OCCUPIED;

    int last_count;
    int last_size;

    int last_sid;
    int bad_sid_count;
} DMA_COUNTERS, *PDMA_COUNTERS;

typedef struct PIBCounters
{
    int socket_inactive;   // CYU3P_PIB_ERR_THR1_SCK_INACTIVE
} PIB_COUNTERS, *PPIB_COUNTERS;

typedef struct Counters {
    int magic;

    DMA_COUNTERS dma_to_host;
    DMA_COUNTERS dma_from_host;

    int log_overrun_count;

    int usb_error_update_count;
    USB_ERROR_COUNTERS usb_error_counters;

    int usb_ep_underrun_count;

    int heap_size;

    int resume_count;

    int state_transition_count;
    int invalid_gpif_state;

    PIB_COUNTERS pib_counters[4];
} COUNTERS, *PCOUNTERS;

volatile static COUNTERS g_counters;

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif // min

#define LOCKP(p)    CyU3PMutexGet(p, CYU3P_WAIT_FOREVER)
#define UNLOCKP(p)  CyU3PMutexPut(p)
#define LOCK(p)     LOCKP(&p)
#define UNLOCK(p)   UNLOCKP(&p)

////////////////////////////////////////////////////////////////////////////////

char *heap_end = 0;
caddr_t _sbrk(int incr)
{
#ifdef HAS_HEAP
    extern char __heap_start;
    extern char __heap_end;
    char *prev_heap_end;

    if (heap_end == 0)
    {
        heap_end = (char *)&__heap_start;
    }
    prev_heap_end = heap_end;

    if (heap_end + incr > &__heap_end)
    {
        return (caddr_t) 0;
    }
    heap_end += incr;
    g_counters.heap_size += incr;   // Not sync'd

    return (caddr_t) prev_heap_end;
#else
    return (caddr_t) -1;
#endif // HAS_HEAP
}

////////////////////////////////////////////////////////////////////////////////

void b200_start_fpga_sb_gpio(void);
void sb_write(uint8_t reg, uint32_t val);
void _sb_write_string(const char* msg);

void msg(const char* str, ...) {
#define msg_CHECK_USE_LOCK
//void _msgv(int use_lock, const char* str, va_list args) {
//#define msg_CHECK_USE_LOCK  if (use_lock)
#ifdef ENABLE_MSG
    va_list args;
    static char buf[LOG_BUFFER_SIZE];
    int idx = 0;

    msg_CHECK_USE_LOCK
        LOCK(g_log_lock);

    ++log_count;
    log_count %= 10000;

    va_start(args, str);

    if (1) { // FIXME: Optional
        uint32_t time_now = CyU3PGetTime();
        idx += sprintf(buf, "%08X %04i ", (uint)time_now, log_count);
    }
    else
        idx += sprintf(buf, "%04i ", log_count);
    idx += vsnprintf(buf + idx, LOG_BUFFER_SIZE - idx, str, args);

    va_end(args);

    if ((LOG_BUFFER_SIZE - log_buffer_len) < (idx + 1 + 1)) {
        msg_CHECK_USE_LOCK
            LOCK(g_counters_lock);
        ++g_counters.log_overrun_count;
        msg_CHECK_USE_LOCK
            UNLOCK(g_counters_lock);

        goto msg_exit;
    }

    // Circular buffer if we need it later, but currently won't wrap due to above condition
    memcpy(log_buffer + log_buffer_len, buf, min(idx + 1, LOG_BUFFER_SIZE - log_buffer_len));
    if ((idx + 1) > (LOG_BUFFER_SIZE - log_buffer_len))
    {
        memcpy(log_buffer, buf + (LOG_BUFFER_SIZE - log_buffer_len), (idx + 1) - (LOG_BUFFER_SIZE - log_buffer_len));
        log_buffer[(idx + 1) - (LOG_BUFFER_SIZE - log_buffer_len)] = '\0';
    }
    else
        log_buffer[log_buffer_len + idx + 1] = '\0';

    log_buffer_len += (idx + 1);
msg_exit:
    msg_CHECK_USE_LOCK
        UNLOCK(g_log_lock);
#ifdef ENABLE_FPGA_SB
    LOCK(g_suart_lock);
    sb_write(SUART_TXCHAR, UPT_MSG);
    _sb_write_string(buf);
    _sb_write_string("\r\n");
    UNLOCK(g_suart_lock);
#endif // ENABLE_FPGA_SB
#endif // ENABLE_MSG
}
/*
void msg(const char* str, ...)
{
    va_list args;
    va_start(args, str);
    _msgv(1, str, args);
    va_end(args);
}

void msg_nl(const char* str, ...)
{
    va_list args;
    va_start(args, str);
    _msgv(0, str, args);
    va_end(args);
}
*/
void log_reset(void) {
    //LOCK(g_log_lock);

    log_buffer_idx = 0;
    log_buffer_len = 0;
    log_buffer[0] = '\0';

    //UNLOCK(g_log_lock);
}

void counters_auto_reset(void) {
    //LOCK(g_counters_lock);

    g_counters.log_overrun_count = 0;

    //UNLOCK(g_counters_lock);
}

void counters_dma_reset(void) {
    LOCK(g_counters_lock);

    LOCK(g_counters_dma_to_host_lock);
    memset((void*)&g_counters.dma_to_host, 0x00, sizeof(DMA_COUNTERS));
    UNLOCK(g_counters_dma_to_host_lock);

    LOCK(g_counters_dma_from_host_lock);
    memset((void*)&g_counters.dma_from_host, 0x00, sizeof(DMA_COUNTERS));
    UNLOCK(g_counters_dma_from_host_lock);

    UNLOCK(g_counters_lock);
}

void counters_reset_usb_errors(void) {
    LOCK(g_counters_lock);

    g_counters.usb_error_update_count = 0;
    memset((void*)&g_counters.usb_error_counters, 0x00, sizeof(g_counters.usb_error_counters));

    UNLOCK(g_counters_lock);
}

#ifdef ENABLE_MANUAL_DMA_XFER
/* Callback funtion for the DMA event notification. */
void dma_callback (
        CyU3PDmaChannel   *chHandle, /* Handle to the DMA channel. */
        CyU3PDmaCbType_t  type,      /* Callback type.             */
        CyU3PDmaCBInput_t *input,    /* Callback status.           */
        int from_host)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    PDMA_COUNTERS cnt = (PDMA_COUNTERS)(from_host ? &g_counters.dma_from_host : &g_counters.dma_to_host);
    CyU3PMutex* lock = (from_host ? &g_counters_dma_from_host_lock : &g_counters_dma_to_host_lock);

    uint16_t buffer_status = (input->buffer_p.status & CY_U3P_DMA_BUFFER_STATUS_MASK);
    if (buffer_status & CY_U3P_DMA_BUFFER_MARKER)
    {
        cnt->BUFFER_MARKER++;
    }
    if (buffer_status & CY_U3P_DMA_BUFFER_EOP)
    {
        cnt->BUFFER_EOP++;
    }
    if (buffer_status & CY_U3P_DMA_BUFFER_ERROR)
    {
        cnt->BUFFER_ERROR++;
    }
    if (buffer_status & CY_U3P_DMA_BUFFER_OCCUPIED)
    {
        cnt->BUFFER_OCCUPIED++;
    }

    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
#ifdef ENABLE_DMA_BUFFER_PACKET_DEBUG
        LOCKP(lock);
        int prod_cnt = cnt->PROD_EVENT++;
        UNLOCKP(lock);

        //if (cnt->last_count != input->buffer_p.count)
        //    msg("[DMA%d %05d] buffer.count (%05d) != last_count (%05d)", from_host, prod_cnt, input->buffer_p.count, cnt->last_count);
        cnt->last_count = input->buffer_p.count;

        if (cnt->last_size != input->buffer_p.size)
            msg("[DMA%d %05d] buffer.size (%05d) != last_size (%05d)", from_host, prod_cnt, input->buffer_p.size, cnt->last_size);
        cnt->last_size = input->buffer_p.size;

        uint32_t* p32 = input->buffer_p.buffer;
        uint32_t sid = p32[1];
        cnt->last_sid = (int)sid;
        if (((from_host == 0) && ((sid != 0xa0) && (sid != 0xb0))) ||
            ((from_host == 1) && ((sid != 0x50) && (sid != 0x60))))
        {
            cnt->bad_sid_count++;
            msg("[DMA%d %05d] Bad SID: 0x%08x", from_host, prod_cnt, sid);
        }

        uint16_t* p16 = input->buffer_p.buffer;

        if (p32[0] & (((uint32_t)1) << 31))
        {
            msg("[DMA%d %05d] Error code: 0x%x (packet len: %05d, buffer count: %05d, seq: %04d)", from_host, prod_cnt, p32[4], p16[0], input->buffer_p.count, (p16[1] & 0x0fff)); // Status

            //msg("[DMA%d] 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x", from_host, p32[0], p32[1], p32[2], p32[3], p32[4], p32[5]);
        }
        else
        {
            if (p16[1] & (((uint16_t)1) << 12))
            {
                msg("[DMA%d %05d] EOB (packet len: %05d, buffer count: %05d)", from_host, prod_cnt, p16[0], input->buffer_p.count);   // Comes with one sample
            }

            if ((p16[0] != input->buffer_p.count) &&
               ((p16[0] + 4) != input->buffer_p.count)) // Case of overrun packet length being padded (being rounded up)
            {
                if (from_host == 0)
                    msg("[DMA%d %05d] Packet len (%05d) != buffer count (%05d), seq: %04d [0x%04x 0x%04x]", from_host, prod_cnt, p16[0], input->buffer_p.count, (p16[1] & 0x0fff), p16[0], p16[1]);
            }

            //msg("[DMA%d] 0x%04x 0x%04x 0x%04x 0x%04x", from_host, p16[0], p16[1], p16[2], p16[3]);

            if (p16[1] & (((uint16_t)1) << 12))
                msg("[DMA%d %05d] 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x", from_host, prod_cnt, p32[0], p32[1], p32[2], p32[3], p32[4], p32[5]);
        }
#endif // ENABLE_DMA_BUFFER_PACKET_DEBUG
        status = CyU3PDmaChannelCommitBuffer (chHandle, input->buffer_p.count, 0);
#ifndef ENABLE_DMA_BUFFER_PACKET_DEBUG
        LOCKP(lock);
        cnt->PROD_EVENT++;
        UNLOCKP(lock);
#endif // !ENABLE_DMA_BUFFER_PACKET_DEBUG
    }
    else if (type == CY_U3P_DMA_CB_CONS_EVENT)
    {
        LOCKP(lock);
        cnt->CONS_EVENT++;
        UNLOCKP(lock);
    }
    else if (type == CY_U3P_DMA_CB_XFER_CPLT)
    {
        LOCKP(lock);
        cnt->XFER_CPLT++;
        UNLOCKP(lock);
    }
    else if (type == CY_U3P_DMA_CB_SEND_CPLT)
    {
        LOCKP(lock);
        cnt->SEND_CPLT++;
        UNLOCKP(lock);
    }
    else if (type == CY_U3P_DMA_CB_RECV_CPLT)
    {
        LOCKP(lock);
        cnt->RECV_CPLT++;
        UNLOCKP(lock);
    }
    else if (type == CY_U3P_DMA_CB_ABORTED)
    {
        LOCKP(lock);
        cnt->ABORTED++;
        UNLOCKP(lock);

        msg("! Aborted %i", from_host);
    }
    else if (type == CY_U3P_DMA_CB_ERROR)
    {
        LOCKP(lock);
        cnt->ERROR++;
        UNLOCKP(lock);

        msg("! Error %i", from_host);
    }
    else if (type == CY_U3P_DMA_CB_PROD_SUSP)
    {
        LOCKP(lock);
        cnt->PROD_SUSP++;
        UNLOCKP(lock);

        msg("! Prod suspend %i", from_host);
    }
    else if (type == CY_U3P_DMA_CB_CONS_SUSP)
    {
        LOCKP(lock);
        cnt->CONS_SUSP++;
        UNLOCKP(lock);

        //msg("! Cons suspend %i", from_host);

        CyU3PDmaChannelResume (chHandle, CyFalse, CyTrue);
    }
}

void from_host_dma_callback (
        CyU3PDmaChannel   *chHandle, /* Handle to the DMA channel. */
        CyU3PDmaCbType_t  type,      /* Callback type.             */
        CyU3PDmaCBInput_t *input)    /* Callback status.           */
{
    return dma_callback(chHandle, type, input, 1);
}

void to_host_dma_callback (
        CyU3PDmaChannel   *chHandle, /* Handle to the DMA channel. */
        CyU3PDmaCbType_t  type,      /* Callback type.             */
        CyU3PDmaCBInput_t *input)    /* Callback status.           */
{
    return dma_callback(chHandle, type, input, 0);
}
#endif // ENABLE_MANUAL_DMA_XFER

/*! Interrupt callback for GPIOs.
 *
 * This function is invoked by the GPIO interrupt handler when pins configured
 * as inputs with interrupts are triggered. */
void gpio_interrupt_callback(uint8_t gpio_id) {
    CyBool_t gpio_value;

    if ((gpio_id == GPIO_DONE) && (g_fx3_state == STATE_CONFIGURING_FPGA)) {    // Only proceed if in the correct FX3 state
        CyU3PGpioGetValue(gpio_id, &gpio_value);

        if(gpio_value == CyTrue) {
            //msg("DONE HIGH");
            CyU3PEventSet(&g_event_usb_config, EVENT_GPIO_DONE_HIGH, CYU3P_EVENT_OR);
        }
    } else if ((gpio_id == GPIO_INIT_B) && (g_fx3_state == STATE_FPGA_READY)) { // Only proceed if in the correct FX3 state
        CyU3PGpioGetValue(gpio_id, &gpio_value);

        if(gpio_value == CyTrue) {
            //msg("INITB_RISE");
            CyU3PEventSet(&g_event_usb_config, EVENT_GPIO_INITB_RISE, CYU3P_EVENT_OR);
        }
    }
}


/*! Stops the application, and destroys transport data structures.
 *
 * This function is essentially a destructor for all transport configurations.
 * It ensures that if the USB configuration is reset without a power reboot,
 * everything will come back up properly. */
void b200_fw_stop(void) {
    msg("b200_fw_stop");

    CyU3PEpConfig_t usb_endpoint_config;

    /* Update the flag. */
    g_app_running = CyFalse;

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(DATA_ENDPOINT_PRODUCER);
    CyU3PUsbFlushEp(DATA_ENDPOINT_CONSUMER);
    CyU3PUsbFlushEp(CTRL_ENDPOINT_PRODUCER);
    CyU3PUsbFlushEp(CTRL_ENDPOINT_CONSUMER);

    /* Reset the DMA channels */
    // SDK 1.3 known issue #1 - probably not necessary since Destroy is next, but just in case
    CyU3PDmaChannelReset(&data_cons_to_prod_chan_handle);
    CyU3PDmaChannelReset(&data_prod_to_cons_chan_handle);
    CyU3PDmaChannelReset(&ctrl_cons_to_prod_chan_handle);
    CyU3PDmaChannelReset(&ctrl_prod_to_cons_chan_handle);

    /* Destroy the DMA channels */
    CyU3PDmaChannelDestroy(&data_cons_to_prod_chan_handle);
    CyU3PDmaChannelDestroy(&data_prod_to_cons_chan_handle);
    CyU3PDmaChannelDestroy(&ctrl_cons_to_prod_chan_handle);
    CyU3PDmaChannelDestroy(&ctrl_prod_to_cons_chan_handle);

    /* Disable endpoints. */
    CyU3PMemSet((uint8_t *) &usb_endpoint_config, 0, \
            sizeof(usb_endpoint_config));
    usb_endpoint_config.enable = CyFalse;

    CyU3PSetEpConfig(DATA_ENDPOINT_PRODUCER, &usb_endpoint_config);
    CyU3PSetEpConfig(DATA_ENDPOINT_CONSUMER, &usb_endpoint_config);
    CyU3PSetEpConfig(CTRL_ENDPOINT_PRODUCER, &usb_endpoint_config);
    CyU3PSetEpConfig(CTRL_ENDPOINT_CONSUMER, &usb_endpoint_config);
}


void reset_gpif(void) {
    g_fx3_state = STATE_BUSY;

    // Put the FPGA into RESET
    CyU3PGpioSetValue(GPIO_FPGA_RESET, CyTrue);

    // Bring down GPIF
    CyU3PGpifDisable(/*CyTrue*/CyFalse);

    /* Reset the DMA channels */
    CyU3PDmaChannelReset(&data_cons_to_prod_chan_handle);
    CyU3PDmaChannelReset(&data_prod_to_cons_chan_handle);
    CyU3PDmaChannelReset(&ctrl_cons_to_prod_chan_handle);
    CyU3PDmaChannelReset(&ctrl_prod_to_cons_chan_handle);

    /* Reset the DMA transfers */
    CyU3PDmaChannelSetXfer(&data_cons_to_prod_chan_handle, DMA_SIZE_INFINITE);
    CyU3PDmaChannelSetXfer(&data_prod_to_cons_chan_handle, DMA_SIZE_INFINITE);
    CyU3PDmaChannelSetXfer(&ctrl_cons_to_prod_chan_handle, DMA_SIZE_INFINITE);
    CyU3PDmaChannelSetXfer(&ctrl_prod_to_cons_chan_handle, DMA_SIZE_INFINITE);

    /* Flush the USB endpoints */
    CyU3PUsbFlushEp(DATA_ENDPOINT_PRODUCER);
    CyU3PUsbFlushEp(DATA_ENDPOINT_CONSUMER);
    CyU3PUsbFlushEp(CTRL_ENDPOINT_PRODUCER);
    CyU3PUsbFlushEp(CTRL_ENDPOINT_CONSUMER);

    /* Load the GPIF configuration for Slave FIFO sync mode. */
    //CyU3PGpifLoad(&CyFxGpifConfig);

    /* Start the state machine. */
    //if (CyU3PGpifSMStart(RESET, ALPHA_RESET) != CY_U3P_SUCCESS)
    //    msg("! CyU3PGpifSMStart");

    /* Configure the watermarks for the slfifo-write buffers. */
    //CyU3PGpifSocketConfigure(0, DATA_TX_PPORT_SOCKET, 5, CyFalse, 1);
    //CyU3PGpifSocketConfigure(1, DATA_RX_PPORT_SOCKET, 6, CyFalse, 1);
    //CyU3PGpifSocketConfigure(2, CTRL_COMM_PPORT_SOCKET, 5, CyFalse, 1);
    //CyU3PGpifSocketConfigure(3, CTRL_RESP_PPORT_SOCKET, 6, CyFalse, 1);

    CyU3PGpioSetValue(GPIO_FPGA_RESET, CyFalse);

    CyU3PThreadSleep(FPGA_RESET_SETTLING_TIME);

    if (CyU3PGpifSMStart(RESET, ALPHA_RESET) != CY_U3P_SUCCESS)
        msg("! CyU3PGpifSMStart");

    msg("GPIF reset");

    b200_start_fpga_sb_gpio();

    g_fx3_state = STATE_RUNNING;
}


CyU3PReturnStatus_t b200_set_io_matrix(CyBool_t fpga_config_mode) {
    CyU3PIoMatrixConfig_t io_config_matrix;
    CyU3PReturnStatus_t res;

    /* Configure the IO peripherals on the FX3. The gpioSimpleEn arrays are
     * bitmaps, where each bit represents the GPIO of the matching index - the
     * second array is index + 32. */
    CyU3PMemSet((uint8_t *) &io_config_matrix, 0, sizeof(io_config_matrix));
    io_config_matrix.isDQ32Bit = (fpga_config_mode == CyFalse);
    io_config_matrix.lppMode = CY_U3P_IO_MATRIX_LPP_DEFAULT;
    io_config_matrix.gpioSimpleEn[0] = 0 | MASK_GPIO_FPGA_SB_SCL | MASK_GPIO_FPGA_SB_SDA;
    io_config_matrix.gpioSimpleEn[1] = MASK_GPIO_PROGRAM_B \
                                       | MASK_GPIO_INIT_B \
                                       | (fpga_config_mode ? 0 : \
                                            // Used once FPGA config is done to bit-bang SPI, etc.
                                              MASK_GPIO_SHDN_SW \
                                            | MASK_GPIO_AUX_PWR_ON \
                                            | MASK_GPIO_FX3_SCLK \
                                            | MASK_GPIO_FX3_CE \
                                            | MASK_GPIO_FX3_MISO \
                                            | MASK_GPIO_FX3_MOSI);
    io_config_matrix.gpioComplexEn[0] = 0;
    io_config_matrix.gpioComplexEn[1] = 0;
    io_config_matrix.useUart = CyFalse;
    io_config_matrix.useI2C = CyTrue;
    io_config_matrix.useI2S = CyFalse;
    io_config_matrix.useSpi = fpga_config_mode;

    res = CyU3PDeviceConfigureIOMatrix(&io_config_matrix);
    if (res != CY_U3P_SUCCESS)
        msg("! ConfigureIOMatrix");

    return res;
}


CyU3PReturnStatus_t b200_gpio_init(CyBool_t set_callback) {
    CyU3PGpioClock_t gpio_clock_config;
    CyU3PReturnStatus_t res;

    /* Since we are only using FX3's 'simple GPIO' functionality, these values
     * must *NOT* change. Cypress says changing them will break stuff. */
    CyU3PMemSet((uint8_t *) &gpio_clock_config, 0, \
            sizeof(gpio_clock_config));
    gpio_clock_config.fastClkDiv = 2;
    gpio_clock_config.slowClkDiv = 0;
    gpio_clock_config.simpleDiv = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
    gpio_clock_config.clkSrc = CY_U3P_SYS_CLK;
    gpio_clock_config.halfDiv = 0;

    res = CyU3PGpioInit(&gpio_clock_config, (set_callback ? gpio_interrupt_callback : NULL));
    if (res != CY_U3P_SUCCESS)
        msg("! CyU3PGpioInit");

    return res;
}


void sb_write(uint8_t reg, uint32_t val) {
#ifdef ENABLE_FPGA_SB
    const int len = 32;
    int i;

    if (g_fpga_sb_enabled == CyFalse)
        return;

    reg += FPGA_SB_UART_ADDR_BASE;

    //CyU3PBusyWait(1); // Can be used after each SetValue to slow down bit changes

    // START
    CyU3PGpioSetValue(GPIO_FPGA_SB_SCL, 1);   // Should already be 1
    CyU3PGpioSetValue(GPIO_FPGA_SB_SDA, 0);

    // ADDR[8]
    for (i = 7; i >= 0; i--) {
        uint8_t bit = ((reg & (0x1 << i)) ? 0x01 : 0x00);
        CyU3PGpioSetValue(GPIO_FPGA_SB_SCL, 0);
        CyU3PGpioSetValue(GPIO_FPGA_SB_SDA, bit);

        CyU3PGpioSetValue(GPIO_FPGA_SB_SCL, 1); // FPGA reads bit
    }

    // DATA[32]
    for (i = (len-1); i >= 0; i--) {
        uint8_t bit = ((val & (0x1 << i)) ? 0x01 : 0x00);
        CyU3PGpioSetValue(GPIO_FPGA_SB_SCL, 0);
        CyU3PGpioSetValue(GPIO_FPGA_SB_SDA, bit);

        CyU3PGpioSetValue(GPIO_FPGA_SB_SCL, 1); // FPGA reads bit
    }

    // STOP
    CyU3PGpioSetValue(GPIO_FPGA_SB_SDA, 0);
    CyU3PGpioSetValue(GPIO_FPGA_SB_SCL, 0);
    CyU3PGpioSetValue(GPIO_FPGA_SB_SCL, 1); // Actual stop
    CyU3PGpioSetValue(GPIO_FPGA_SB_SDA, 1); // Xact occurs
#endif // ENABLE_FPGA_SB
}


void _sb_write_string(const char* msg) {
#ifdef ENABLE_FPGA_SB
    while (*msg) {
        sb_write(SUART_TXCHAR, (uint8_t)(*(msg++)));
    }
#endif // ENABLE_FPGA_SB
}


void sb_write_string(const char* msg) {
#ifdef ENABLE_FPGA_SB
    LOCK(g_suart_lock);
    _sb_write_string(msg);
    UNLOCK(g_suart_lock);
#endif // ENABLE_FPGA_SB
}


void b200_enable_fpga_sb_gpio(CyBool_t enable) {
#ifdef ENABLE_FPGA_SB
    CyU3PGpioSimpleConfig_t gpio_config;
    CyU3PReturnStatus_t res;

    if (enable == CyFalse) {
        g_fpga_sb_enabled = CyFalse;

        return;
    }

    gpio_config.outValue    = CyFalse;
    gpio_config.driveLowEn  = CyTrue;
    gpio_config.driveHighEn = CyTrue;
    gpio_config.inputEn     = CyFalse;
    gpio_config.intrMode    = CY_U3P_GPIO_NO_INTR;

    res = CyU3PGpioSetSimpleConfig(GPIO_FPGA_SB_SCL, &gpio_config);
    if (res != CY_U3P_SUCCESS) {
        msg("! GpioSetSimpleConfig GPIO_FPGA_SB_SCL");
    }
    res = CyU3PGpioSetSimpleConfig(GPIO_FPGA_SB_SDA, &gpio_config);
    if (res != CY_U3P_SUCCESS) {
        msg("! GpioSetSimpleConfig GPIO_FPGA_SB_SDA");
    }

    CyU3PGpioSetValue(GPIO_FPGA_SB_SCL, 1);
    CyU3PGpioSetValue(GPIO_FPGA_SB_SDA, 1);

    g_fpga_sb_enabled = CyTrue;

    msg("Debug SB OK");
#endif // ENABLE_FPGA_SB
}


void b200_start_fpga_sb_gpio(void) {
#ifdef ENABLE_FPGA_SB
    LOCK(g_suart_lock);
    sb_write(SUART_CLKDIV, /*g_fpga_sb_uart_div*/g_config.sb_baud_div);   // 16-bit reg, master clock = 100 MHz (434*2x = 230400/2)
    _sb_write_string("\r\n B2x0 FPGA reset\r\n");
    UNLOCK(g_suart_lock);

    msg("Compat:  %d.%d", FX3_COMPAT_MAJOR, FX3_COMPAT_MINOR);
    msg("FX3 SDK: %d.%d.%d (build %d)", CYFX_VERSION_MAJOR, CYFX_VERSION_MINOR, CYFX_VERSION_PATCH, CYFX_VERSION_BUILD);
#endif // ENABLE_FPGA_SB
}


/*! Initialize and configure the GPIO module for FPGA programming.
 *
 * This function initializes the FX3 GPIO module, creating a configuration that
 * allows us to program the FPGA. After the FPGA has been programmed, the
 * application thread will re-configure some of the pins. */
void b200_gpios_pre_fpga_config(void) {
    CyU3PGpioSimpleConfig_t gpio_config;

    //b200_enable_fpga_sb_gpio(CyFalse);

    //CyU3PGpioDeInit();

    b200_set_io_matrix(CyTrue);

    //b200_gpio_init(CyTrue);   // This now done once during startup

    ////////////////////////////////////

    /* GPIO[0:32] must be set with the DeviceOverride function, instead of the
     * SimpleEn array configuration. */
    CyU3PDeviceGpioOverride(GPIO_FPGA_RESET, CyTrue);
    CyU3PDeviceGpioOverride(GPIO_DONE, CyTrue);

    /* Configure GPIOs:
     *      Outputs:
     *          driveLowEn = True
     *          driveHighEn = True
     *          inputEn = False
     *      Inputs:
     *          driveLowEn = False
     *          driveHighEn = False
     *          outValue = Ignored
     */
    gpio_config.outValue = CyFalse;
    gpio_config.driveLowEn = CyTrue;
    gpio_config.driveHighEn = CyTrue;
    gpio_config.inputEn = CyFalse;
    gpio_config.intrMode = CY_U3P_GPIO_NO_INTR;

    CyU3PGpioSetSimpleConfig(GPIO_FPGA_RESET, &gpio_config);
    CyU3PGpioSetSimpleConfig(GPIO_PROGRAM_B, &gpio_config);

    /* Reconfigure the GPIO configure struct for inputs that DO require
     * interrupts attached to them. */
    gpio_config.outValue = CyTrue;
    gpio_config.inputEn = CyTrue;
    gpio_config.driveLowEn = CyFalse;
    gpio_config.driveHighEn = CyFalse;
    gpio_config.intrMode = CY_U3P_GPIO_INTR_POS_EDGE;

    CyU3PGpioSetSimpleConfig(GPIO_DONE, &gpio_config);
    CyU3PGpioSetSimpleConfig(GPIO_INIT_B, &gpio_config);

    /* Initialize GPIO output values. */
    CyU3PGpioSetValue(GPIO_FPGA_RESET, 0);
    CyU3PGpioSetValue(GPIO_PROGRAM_B, 1);

    b200_enable_fpga_sb_gpio(CyTrue);   // So SCL/SDA are already high when SB state machine activates
}


void b200_slfifo_mode_gpio_config(void) {
    CyU3PGpioSimpleConfig_t gpio_config;

    //b200_enable_fpga_sb_gpio(CyFalse);

    //CyU3PGpioDeInit();

    b200_set_io_matrix(CyFalse);

    //b200_gpio_init(CyFalse);  // This now done once during startup

    ////////////////////////////////////

    /* GPIO[0:32] must be set with the DeviceOverride function, instead of the
     * SimpleEn array configuration. */
    CyU3PDeviceGpioOverride(GPIO_FPGA_RESET, CyTrue);
    CyU3PDeviceGpioOverride(GPIO_DONE, CyTrue);
    CyU3PDeviceGpioOverride(GPIO_FX3_SCLK, CyTrue);
    CyU3PDeviceGpioOverride(GPIO_FX3_CE, CyTrue);
    CyU3PDeviceGpioOverride(GPIO_FX3_MISO, CyTrue);
    CyU3PDeviceGpioOverride(GPIO_FX3_MOSI, CyTrue);

    /* Configure GPIOs:
     *      Outputs:
     *          driveLowEn = True
     *          driveHighEn = True
     *          inputEn = False
     *      Inputs:
     *          driveLowEn = False
     *          driveHighEn = False
     *          outValue = Ignored
     */
    gpio_config.outValue = CyFalse;
    gpio_config.driveLowEn = CyTrue;
    gpio_config.driveHighEn = CyTrue;
    gpio_config.inputEn = CyFalse;
    gpio_config.intrMode = CY_U3P_GPIO_NO_INTR;

    CyU3PGpioSetSimpleConfig(GPIO_FPGA_RESET, &gpio_config);
    CyU3PGpioSetSimpleConfig(GPIO_SHDN_SW, &gpio_config);
    CyU3PGpioSetSimpleConfig(GPIO_FX3_SCLK, &gpio_config);
    CyU3PGpioSetSimpleConfig(GPIO_FX3_CE, &gpio_config);
    CyU3PGpioSetSimpleConfig(GPIO_FX3_MOSI, &gpio_config);

    /* Reconfigure the GPIO configure struct for inputs that do NOT require
     * interrupts attached to them. */
    gpio_config.outValue = CyFalse;
    gpio_config.inputEn = CyTrue;
    gpio_config.driveLowEn = CyFalse;
    gpio_config.driveHighEn = CyFalse;
    gpio_config.intrMode = CY_U3P_GPIO_NO_INTR;

    CyU3PGpioSetSimpleConfig(GPIO_FX3_MISO, &gpio_config);
    CyU3PGpioSetSimpleConfig(GPIO_AUX_PWR_ON, &gpio_config);
    CyU3PGpioSetSimpleConfig(GPIO_PROGRAM_B, &gpio_config);
    CyU3PGpioSetSimpleConfig(GPIO_INIT_B, &gpio_config);
    CyU3PGpioSetSimpleConfig(GPIO_DONE, &gpio_config);

    /* Initialize GPIO output values. */
    CyU3PGpioSetValue(GPIO_FPGA_RESET, 0);
    CyU3PGpioSetValue(GPIO_SHDN_SW, 1);
    CyU3PGpioSetValue(GPIO_FX3_SCLK, 0);
    CyU3PGpioSetValue(GPIO_FX3_CE, 1);
    CyU3PGpioSetValue(GPIO_FX3_MOSI, 0);

    // Disabled here as only useful once FPGA has been programmed
    //b200_enable_fpga_sb_gpio(CyTrue);
    //b200_start_fpga_sb_gpio();  // Set up SB USART
}


/*! Initializes and configures USB, and DMA.
 *
 * This function creates and connects the USB endpoints, and sets up the DMA
 * channels. After this is done, everything is 'running' on the FX3 chip, and
 * ready to receive data from the host. */
void b200_fw_start(void) {
    msg("b200_fw_start");

    CyU3PDmaChannelConfig_t dma_channel_config;
    CyU3PEpConfig_t usb_endpoint_config;
    CyU3PUSBSpeed_t usb_speed;
    uint16_t max_packet_size = 0;
    uint16_t data_buffer_count = 0;
    uint16_t data_buffer_size = 0;
    uint16_t data_buffer_size_to_host = 0;
    uint16_t data_buffer_size_from_host = 0;
    uint8_t num_packets_per_burst = 0;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Based on the USB bus speed, configure the endpoint packet size
     * and the DMA buffer size */
    usb_speed = CyU3PUsbGetSpeed();
    switch(usb_speed) {
        case CY_U3P_FULL_SPEED:
        case CY_U3P_HIGH_SPEED:
            max_packet_size = 512;
            data_buffer_count = 16;
            data_buffer_size = 512;
            g_vendor_req_buff_size = USB2_VREQ_BUF_SIZE;    // Max 64
            num_packets_per_burst = USB2_PACKETS_PER_BURST; // 1

            data_buffer_size_to_host = data_buffer_size_from_host = data_buffer_size;

            break;

        case CY_U3P_SUPER_SPEED:
//#ifdef PREVENT_LOW_POWER_MODE
            apiRetStatus = CyU3PUsbLPMDisable();    // This still allows my laptop to sleep

            if (apiRetStatus != CY_U3P_SUCCESS)
                msg("! LPMDisable failed (%d)", apiRetStatus);
            else
                msg("LPMDisable OK");
//#endif // PREVENT_LOW_POWER_MODE
            max_packet_size = 1024; // Per USB3 spec

            // SDK ver: total available buffer memory
            // 1.2.3: 204KB
            // 1.3.1: 188KB

            // These options should be ignored - data_buffer_count *MUST* be 1
            // They follow is kept for future testing

            // 1K
            //data_buffer_count = 64;
            //data_buffer_size = 1024;

            // 4K
            //data_buffer_count = 8;
            //data_buffer_size = 4096;

            // 8K
            //data_buffer_count = 4;
            //data_buffer_size = 4096*2;

            // 16K
            //data_buffer_count = 2*2;
            //data_buffer_size = 16384;    // Default 16K

            // 32K
            //data_buffer_count = 2;
            //data_buffer_size = 16384*2;

            //data_buffer_count = 1;
            //data_buffer_size = ((1 << 16) - 1);
            //data_buffer_size = 16384;
            //data_buffer_size -= (data_buffer_size % 1024);  // Align to 1K boundary

            data_buffer_count = g_config.dma_buffer_count;
            data_buffer_size = g_config.dma_buffer_size;

            data_buffer_size_to_host = data_buffer_size;
            data_buffer_size_from_host = data_buffer_size;

            g_vendor_req_buff_size = USB3_VREQ_BUF_SIZE;    // Max 512
            num_packets_per_burst = USB3_PACKETS_PER_BURST; // 8
            break;

        case CY_U3P_NOT_CONNECTED:
            msg("! CY_U3P_NOT_CONNECTED");
            return;

        default:
            return;
    }

    msg("[DMA] to host: %d, from host: %d, depth: %d, burst size: %d", data_buffer_size_to_host, data_buffer_size_from_host, data_buffer_count, num_packets_per_burst);

#ifdef ENABLE_MANUAL_DMA_XFER
    msg("[DMA] Callback enabled");

#ifdef ENABLE_P2U_SUSP_EOP
    msg("[DMA] P2U_SUSP_EOP enabled");
#endif // ENABLE_P2U_SUSP_EOP
#ifdef ENABLE_MANUAL_DMA_XFER_FROM_HOST
    msg("[DMA] Manual transfers from host");
#endif // ENABLE_MANUAL_DMA_XFER_FROM_HOST
#ifdef ENABLE_MANUAL_DMA_XFER_TO_HOST
    msg("[DMA] Manual transfers to host");
#endif // ENABLE_MANUAL_DMA_XFER_TO_HOST
#ifdef ENABLE_DMA_BUFFER_PACKET_DEBUG
    msg("[DMA] Packet debugging enabled");
#endif // ENABLE_DMA_BUFFER_PACKET_DEBUG

#else
    msg("[DMA] AUTO transfer mode");
#endif // ENABLE_MANUAL_DMA_XFER

    msg("[DMA] Transfer override: %s", (g_config.manual_dma ? "manual" : "auto"));

    /*************************************************************************
     * Slave FIFO Data DMA Channel Configuration
     *************************************************************************/

    /* Wipe out any old config. */
    CyU3PMemSet((uint8_t *) &usb_endpoint_config, 0, \
            sizeof(usb_endpoint_config));

    /* This is the configuration for the USB Producer and Consumer endpoints.
     *
     * The Producer endpoint is actually the endpoint on the FX3 that is
     * sending data BACK to the host. This endpoint enumerates as the
     * 'BULK IN' endpoint.

     * The Consumer endpoint is the endpoint on the FX3 that is
     * receiving data from the host. This endpoint enumerates as the
     * 'BULK OUT' endpoint.
     *
     * Note that this is opposite of what you might expect!. */
    usb_endpoint_config.enable = CyTrue;
    usb_endpoint_config.epType = CY_U3P_USB_EP_BULK;
    usb_endpoint_config.burstLen = num_packets_per_burst;
    usb_endpoint_config.streams = 0;
    usb_endpoint_config.pcktSize = max_packet_size;

    /* Configure the endpoints that we are using for slave FIFO transfers. */
    CyU3PSetEpConfig(DATA_ENDPOINT_PRODUCER, &usb_endpoint_config);
    CyU3PSetEpConfig(DATA_ENDPOINT_CONSUMER, &usb_endpoint_config);

    /* Create a DMA AUTO channel for U2P transfer.
     * DMA size is set based on the USB speed. */
    //dma_channel_config.size = data_buffer_size;
    dma_channel_config.size = data_buffer_size_from_host;
    dma_channel_config.count = data_buffer_count;
    dma_channel_config.prodSckId = PRODUCER_DATA_SOCKET;
    dma_channel_config.consSckId = DATA_TX_PPORT_SOCKET;
    dma_channel_config.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dma_channel_config.notification = 0 |
#if defined(ENABLE_MANUAL_DMA_XFER) && defined(ENABLE_MANUAL_DMA_XFER_FROM_HOST)
CY_U3P_DMA_CB_XFER_CPLT |
CY_U3P_DMA_CB_SEND_CPLT |
CY_U3P_DMA_CB_RECV_CPLT |
CY_U3P_DMA_CB_PROD_EVENT |
CY_U3P_DMA_CB_CONS_EVENT |
CY_U3P_DMA_CB_ABORTED |
CY_U3P_DMA_CB_ERROR |
CY_U3P_DMA_CB_PROD_SUSP |
CY_U3P_DMA_CB_CONS_SUSP |
#endif // ENABLE_MANUAL_DMA_XFER
    0;
    //if (g_config.manual_dma == 0)
    //    dma_channel_config.notification = 0;    // Force all off is manual is disabled
    dma_channel_config.cb =
#if defined(ENABLE_MANUAL_DMA_XFER) && defined(ENABLE_MANUAL_DMA_XFER_FROM_HOST)
    from_host_dma_callback;
#else
    NULL;
#endif // ENABLE_MANUAL_DMA_XFER
    dma_channel_config.prodHeader = 0;
    dma_channel_config.prodFooter = 0;
    dma_channel_config.consHeader = 0;
    dma_channel_config.prodAvailCount = 0;

    CyU3PDmaChannelCreate (&data_cons_to_prod_chan_handle,
#if defined(ENABLE_MANUAL_DMA_XFER) && defined(ENABLE_MANUAL_DMA_XFER_FROM_HOST)
            (g_config.manual_dma ? /*CY_U3P_DMA_TYPE_AUTO_SIGNAL*/CY_U3P_DMA_TYPE_MANUAL : CY_U3P_DMA_TYPE_AUTO),
#else
            CY_U3P_DMA_TYPE_AUTO,
#endif // ENABLE_MANUAL_DMA_XFER
            &dma_channel_config);

    // By default these will adopt 'usb_endpoint_config.pcktSize'
    //CyU3PSetEpPacketSize(DATA_ENDPOINT_PRODUCER, 16384);
    //CyU3PSetEpPacketSize(DATA_ENDPOINT_CONSUMER, 16384);

    /* Create a DMA AUTO channel for P2U transfer. */
    dma_channel_config.size = data_buffer_size_to_host;
    dma_channel_config.prodSckId = DATA_RX_PPORT_SOCKET;
    dma_channel_config.consSckId = CONSUMER_DATA_SOCKET;
    dma_channel_config.notification = 0 |
#if defined(ENABLE_MANUAL_DMA_XFER) && defined(ENABLE_MANUAL_DMA_XFER_TO_HOST)
CY_U3P_DMA_CB_XFER_CPLT |
CY_U3P_DMA_CB_SEND_CPLT |
CY_U3P_DMA_CB_RECV_CPLT |
CY_U3P_DMA_CB_PROD_EVENT |
CY_U3P_DMA_CB_CONS_EVENT |
CY_U3P_DMA_CB_ABORTED |
CY_U3P_DMA_CB_ERROR |
CY_U3P_DMA_CB_PROD_SUSP |
CY_U3P_DMA_CB_CONS_SUSP |
#endif // ENABLE_MANUAL_DMA_XFER
#if defined(ENABLE_MANUAL_DMA_XFER) && defined(ENABLE_P2U_SUSP_EOP)
CY_U3P_DMA_CB_CONS_SUSP |   // For 'CyU3PDmaChannelSetSuspend' below
#endif // ENABLE_P2U_SUSP_EOP
    0;
    //if (g_config.manual_dma == 0)
    //    dma_channel_config.notification = 0;    // Force all off is manual is disabled
    dma_channel_config.cb =
#if defined(ENABLE_MANUAL_DMA_XFER) && (defined(ENABLE_MANUAL_DMA_XFER_TO_HOST) || defined(ENABLE_P2U_SUSP_EOP))
    to_host_dma_callback;
#else
    NULL;
#endif // ENABLE_MANUAL_DMA_XFER
    CyU3PDmaChannelCreate (&data_prod_to_cons_chan_handle,
#if defined(ENABLE_MANUAL_DMA_XFER) && defined(ENABLE_MANUAL_DMA_XFER_TO_HOST)
            (g_config.manual_dma ? /*CY_U3P_DMA_TYPE_AUTO_SIGNAL*/CY_U3P_DMA_TYPE_MANUAL : CY_U3P_DMA_TYPE_AUTO),
#else
            CY_U3P_DMA_TYPE_AUTO,
#endif // ENABLE_MANUAL_DMA_XFER
            &dma_channel_config);
#ifdef ENABLE_P2U_SUSP_EOP
    CyU3PDmaChannelSetSuspend(&data_prod_to_cons_chan_handle, CY_U3P_DMA_SCK_SUSP_NONE, CY_U3P_DMA_SCK_SUSP_EOP);
#endif // ENABLE_P2U_SUSP_EOP
    /* Flush the Endpoint memory */
    CyU3PUsbFlushEp(DATA_ENDPOINT_PRODUCER);
    CyU3PUsbFlushEp(DATA_ENDPOINT_CONSUMER);

    /* Set DMA channel transfer size. */
    CyU3PDmaChannelSetXfer(&data_cons_to_prod_chan_handle, DMA_SIZE_INFINITE);
    CyU3PDmaChannelSetXfer(&data_prod_to_cons_chan_handle, DMA_SIZE_INFINITE);


    /*************************************************************************
     * Slave FIFO Control DMA Channel Configuration
     *************************************************************************/

    /* Wipe out any old config. */
    CyU3PMemSet((uint8_t *) &usb_endpoint_config, 0, \
            sizeof(usb_endpoint_config));

    /* This is the configuration for the USB Producer and Consumer endpoints.
     *
     * The Producer endpoint is actually the endpoint on the FX3 that is
     * sending data BACK to the host. This endpoint enumerates as the
     * 'BULK IN' endpoint.

     * The Consumer endpoint is the endpoint on the FX3 that is
     * receiving data from the host. This endpoint enumerates as the
     * 'BULK OUT' endpoint.
     *
     * Note that this is opposite of what you might expect!. */
    usb_endpoint_config.enable = CyTrue;
    usb_endpoint_config.epType = CY_U3P_USB_EP_BULK;
    usb_endpoint_config.burstLen = num_packets_per_burst;
    usb_endpoint_config.streams = 0;
    usb_endpoint_config.pcktSize = max_packet_size;

    /* Configure the endpoints that we are using for slave FIFO transfers. */
    CyU3PSetEpConfig(CTRL_ENDPOINT_PRODUCER, &usb_endpoint_config);
    CyU3PSetEpConfig(CTRL_ENDPOINT_CONSUMER, &usb_endpoint_config);

    /* Create a DMA AUTO channel for U2P transfer.
     * DMA size is set based on the USB speed. */
    dma_channel_config.size = max_packet_size;
    dma_channel_config.count = 2;
    dma_channel_config.prodSckId = PRODUCER_CTRL_SOCKET;
    dma_channel_config.consSckId = CTRL_COMM_PPORT_SOCKET;
    dma_channel_config.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dma_channel_config.notification = 0;
    dma_channel_config.cb = NULL;
    dma_channel_config.prodHeader = 0;
    dma_channel_config.prodFooter = 0;
    dma_channel_config.consHeader = 0;
    dma_channel_config.prodAvailCount = 0;

    CyU3PDmaChannelCreate (&ctrl_cons_to_prod_chan_handle,
            CY_U3P_DMA_TYPE_AUTO, &dma_channel_config);

    /* Create a DMA AUTO channel for P2U transfer. */
    dma_channel_config.prodSckId = CTRL_RESP_PPORT_SOCKET;
    dma_channel_config.consSckId = CONSUMER_CTRL_SOCKET;
    dma_channel_config.cb = NULL;
    CyU3PDmaChannelCreate (&ctrl_prod_to_cons_chan_handle,
            CY_U3P_DMA_TYPE_AUTO, &dma_channel_config);

    /* Flush the Endpoint memory */
    CyU3PUsbFlushEp(CTRL_ENDPOINT_PRODUCER);
    CyU3PUsbFlushEp(CTRL_ENDPOINT_CONSUMER);

    /* Set DMA channel transfer size. */
    CyU3PDmaChannelSetXfer(&ctrl_cons_to_prod_chan_handle, DMA_SIZE_INFINITE);
    CyU3PDmaChannelSetXfer(&ctrl_prod_to_cons_chan_handle, DMA_SIZE_INFINITE);

    //CyU3PUsbEnableEPPrefetch();	// To address USB_EVENT_EP_UNDERRUN on EP 0x86 (didn't fix it though)

    /* Update the application status flag. */
    g_app_running = CyTrue;
}


/*! This callback is invoked when the FX3 detects a USB event.
 *
 * We currently handle SETCONF, RESET, and DISCONNECT.
 *
 * We are _not_ handling SUSPEND or CONNECT.
 */
void event_usb_callback (CyU3PUsbEventType_t event_type, uint16_t event_data) {

    switch(event_type) {
        case CY_U3P_USB_EVENT_SETCONF:
            msg("USB_EVENT_SETCONF (#%d)", event_data); //evData provides the configuration number that is selected by the host.
            if(g_app_running) {
                b200_fw_stop();
            }

            b200_fw_start();
            break;

        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
            if (event_type == CY_U3P_USB_EVENT_RESET)
                msg("USB_EVENT_RESET");
            else
                msg("USB_EVENT_DISCONNECT");
            if(g_app_running) {
                b200_fw_stop();
            }
            break;

        case CY_U3P_USB_EVENT_CONNECT:
            msg("USB_EVENT_CONNECT");
            break;

        case CY_U3P_USB_EVENT_SUSPEND:
            msg("USB_EVENT_SUSPEND");
            break;

        case CY_U3P_USB_EVENT_RESUME:   // Known issue: this is called repeatedly after a resume
            //msg("USB_EVENT_RESUME");
            g_counters.resume_count++;  // Not locked
            break;

        case CY_U3P_USB_EVENT_SPEED:
            msg("USB_EVENT_SPEED");
            break;

        case CY_U3P_USB_EVENT_SETINTF:
            msg("USB_EVENT_SETINTF");
            break;

        case CY_U3P_USB_EVENT_SET_SEL:
            msg("USB_EVENT_SET_SEL");
            break;

        case CY_U3P_USB_EVENT_SOF_ITP:  // CyU3PUsbEnableITPEvent
            //msg("USB_EVENT_SOF_ITP");
            break;

        case CY_U3P_USB_EVENT_EP0_STAT_CPLT:
            //msg("USB_EVENT_EP0_STAT_CPLT");   // Occurs each time there's a control transfer
            break;

        case CY_U3P_USB_EVENT_VBUS_VALID:
            msg("USB_EVENT_VBUS_VALID");
            break;

        case CY_U3P_USB_EVENT_VBUS_REMOVED:
            msg("USB_EVENT_VBUS_REMOVED");
            break;

        case CY_U3P_USB_EVENT_HOST_CONNECT:
            msg("USB_EVENT_HOST_CONNECT");
            break;

        case CY_U3P_USB_EVENT_HOST_DISCONNECT:
            msg("USB_EVENT_HOST_DISCONNECT");
            break;

        case CY_U3P_USB_EVENT_OTG_CHANGE:
            msg("USB_EVENT_OTG_CHANGE");
            break;

        case CY_U3P_USB_EVENT_OTG_VBUS_CHG:
            msg("USB_EVENT_OTG_VBUS_CHG");
            break;

        case CY_U3P_USB_EVENT_OTG_SRP:
            msg("USB_EVENT_OTG_SRP");
            break;

        case CY_U3P_USB_EVENT_EP_UNDERRUN:  // See SDK 1.3 known issues 17 if this happens (can probably ignore first logged occurence)
            LOCK(g_counters_lock);
            ++g_counters.usb_ep_underrun_count;
            UNLOCK(g_counters_lock);

            msg("! USB_EVENT_EP_UNDERRUN on EP 0x%02x", event_data);
            break;

        case CY_U3P_USB_EVENT_LNK_RECOVERY:
            msg("USB_EVENT_LNK_RECOVERY");
            break;
#if (CYFX_VERSION_MAJOR >= 1) && (CYFX_VERSION_MINOR >= 3)
        case CY_U3P_USB_EVENT_USB3_LNKFAIL:
            msg("USB_EVENT_USB3_LNKFAIL");
            break;

        case CY_U3P_USB_EVENT_SS_COMP_ENTRY:
            msg("USB_EVENT_SS_COMP_ENTRY");
            break;

        case CY_U3P_USB_EVENT_SS_COMP_EXIT:
            msg("USB_EVENT_SS_COMP_EXIT");
            break;
#endif // (CYFX_VERSION_MAJOR >= 1) && (CYFX_VERSION_MINOR >= 3)

        default:
            msg("! Unhandled USB event");
            break;
    }
}


/*! Callback function that is invoked when a USB setup event occurs.
 *
 * We aren't actually handling the USB setup ourselves, but rather letting the
 * USB driver take care of it since the default options work fine.  The purpose
 * of this function is to register that the event happened at all, so that the
 * application thread knows it can proceed.
 *
 * This function is also responsible for receiving vendor requests, and triggering
 * the appropriate RTOS event to wake up the vendor request handler thread.
 */
CyBool_t usb_setup_callback(uint32_t data0, uint32_t data1) {
    STATIC_SAVER uint8_t bRequestType, bRequest, bType, bTarget, i2cAddr;
    STATIC_SAVER uint16_t wValue, wIndex, wLength;

    CyBool_t handled = CyFalse;

    /* Decode the fields from the setup request. */
    bRequestType = (uint8_t)(data0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (uint8_t)(bRequestType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (uint8_t)(bRequestType & CY_U3P_USB_TARGET_MASK);
    bRequest = (uint8_t)((data0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = (uint16_t)((data0 & CY_U3P_USB_VALUE_MASK) >> CY_U3P_USB_VALUE_POS);
    wIndex   = (uint16_t)((data1 & CY_U3P_USB_INDEX_MASK) >> CY_U3P_USB_INDEX_POS);
    wLength  = (uint16_t)((data1 & CY_U3P_USB_LENGTH_MASK) >> CY_U3P_USB_LENGTH_POS);

    if(bType == CY_U3P_USB_STANDARD_RQT) {
        /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if((bTarget == CY_U3P_USB_TARGET_INTF) \
                && ((bRequest == CY_U3P_USB_SC_SET_FEATURE) \
                || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0)) {

            if(g_app_running) {
                CyU3PUsbAckSetup();
                msg("ACK set/clear");
            } else {
                CyU3PUsbStall(0, CyTrue, CyFalse);
                msg("! STALL set/clear");
            }

            handled = CyTrue;
        }

        /* Handle Microsoft OS String Descriptor request. */
        if((bTarget == CY_U3P_USB_TARGET_DEVICE) \
                && (bRequest == CY_U3P_USB_SC_GET_DESCRIPTOR) \
                && (wValue == ((CY_U3P_USB_STRING_DESCR << 8) | 0xEE))) {
            /* Make sure we do not send more data than requested. */
            if(wLength > b200_usb_product_desc[0]) {
                wLength = b200_usb_product_desc[0];
            }

            //msg("MS string desc");

            CyU3PUsbSendEP0Data(wLength, ((uint8_t *) b200_usb_product_desc));
            handled = CyTrue;
        }

        /* CLEAR_FEATURE request for endpoint is always passed to the setup callback
         * regardless of the enumeration model used. When a clear feature is received,
         * the previous transfer has to be flushed and cleaned up. This is done at the
         * protocol level. Since this is just a loopback operation, there is no higher
         * level protocol. So flush the EP memory and reset the DMA channel associated
         * with it. If there are more than one EP associated with the channel reset both
         * the EPs. The endpoint stall and toggle / sequence number is also expected to be
         * reset. Return CyFalse to make the library clear the stall and reset the endpoint
         * toggle. Or invoke the CyU3PUsbStall (ep, CyFalse, CyTrue) and return CyTrue.
         * Here we are clearing the stall. */
        if((bTarget == CY_U3P_USB_TARGET_ENDPT) \
                && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
                && (wValue == CY_U3P_USBX_FS_EP_HALT)) {
            if(g_app_running) {
                if(wIndex == DATA_ENDPOINT_PRODUCER)  {
                    CyU3PDmaChannelReset(&data_cons_to_prod_chan_handle);
                    CyU3PUsbFlushEp(DATA_ENDPOINT_PRODUCER);
                    CyU3PUsbResetEp(DATA_ENDPOINT_PRODUCER);
                    CyU3PDmaChannelSetXfer(&data_cons_to_prod_chan_handle, \
                            DMA_SIZE_INFINITE);
                    CyU3PUsbStall(wIndex, CyFalse, CyTrue);
                    handled = CyTrue;
                    CyU3PUsbAckSetup();

                    msg("Clear DATA_ENDPOINT_PRODUCER");
                }

                if(wIndex == DATA_ENDPOINT_CONSUMER) {
                    CyU3PDmaChannelReset(&data_prod_to_cons_chan_handle);
                    CyU3PUsbFlushEp(DATA_ENDPOINT_CONSUMER);
                    CyU3PUsbResetEp(DATA_ENDPOINT_CONSUMER);
                    CyU3PDmaChannelSetXfer(&data_prod_to_cons_chan_handle, \
                            DMA_SIZE_INFINITE);
                    CyU3PUsbStall(wIndex, CyFalse, CyTrue);
                    handled = CyTrue;
                    CyU3PUsbAckSetup();

                    msg("Clear DATA_ENDPOINT_CONSUMER");
                }

                if(wIndex == CTRL_ENDPOINT_PRODUCER)  {
                    CyU3PDmaChannelReset(&ctrl_cons_to_prod_chan_handle);
                    CyU3PUsbFlushEp(CTRL_ENDPOINT_PRODUCER);
                    CyU3PUsbResetEp(CTRL_ENDPOINT_PRODUCER);
                    CyU3PDmaChannelSetXfer(&ctrl_cons_to_prod_chan_handle, \
                            DMA_SIZE_INFINITE);
                    CyU3PUsbStall(wIndex, CyFalse, CyTrue);
                    handled = CyTrue;
                    CyU3PUsbAckSetup();

                    msg("Clear CTRL_ENDPOINT_PRODUCER");
                }

                if(wIndex == CTRL_ENDPOINT_CONSUMER) {
                    CyU3PDmaChannelReset(&ctrl_prod_to_cons_chan_handle);
                    CyU3PUsbFlushEp(CTRL_ENDPOINT_CONSUMER);
                    CyU3PUsbResetEp(CTRL_ENDPOINT_CONSUMER);
                    CyU3PDmaChannelSetXfer(&ctrl_prod_to_cons_chan_handle, \
                            DMA_SIZE_INFINITE);
                    CyU3PUsbStall(wIndex, CyFalse, CyTrue);
                    handled = CyTrue;
                    CyU3PUsbAckSetup();

                    msg("Clear CTRL_ENDPOINT_CONSUMER");
                }
            }
        }
    }
    /* This must be & and not == so that we catch VREQs that are both 'IN' and
     * 'OUT' in direction. */
    else if(bRequestType & CY_U3P_USB_VENDOR_RQT) {

        handled = CyTrue;
        uint16_t read_count = 0;

        switch(bRequest) {
            case B200_VREQ_BITSTREAM_START: {
                CyU3PUsbGetEP0Data(1, g_vendor_req_buffer, &read_count);

                g_fpga_programming_write_count = 0;

                CyU3PEventSet(&g_event_usb_config, EVENT_BITSTREAM_START, \
                        CYU3P_EVENT_OR);
                break;
            }

            case B200_VREQ_BITSTREAM_DATA: {
                CyU3PUsbGetEP0Data(g_vendor_req_buff_size, g_vendor_req_buffer, \
                        &read_count);

                if (g_fx3_state == STATE_CONFIGURING_FPGA) {
                    ++g_fpga_programming_write_count;
                    CyU3PSpiTransmitWords(g_vendor_req_buffer, read_count);
                    CyU3PThreadSleep(1);  // Newer controllers don't have an issue when this short sleep here
                }
                break;
            }

            case B200_VREQ_BITSTREAM_DATA_FILL: {
                CyU3PUsbGetEP0Data(g_vendor_req_buff_size, g_vendor_req_buffer, &g_vendor_req_read_count);
                break;
            }

            case B200_VREQ_BITSTREAM_DATA_COMMIT: {
                /*CyU3PReturnStatus_t*/int spi_result = -1;
                if (g_fx3_state == STATE_CONFIGURING_FPGA) {
                    ++g_fpga_programming_write_count;
                    spi_result = CyU3PSpiTransmitWords(g_vendor_req_buffer, g_vendor_req_read_count);
                    CyU3PThreadSleep(1);   // 20 MHz, 512 bytes
                }
                CyU3PUsbSendEP0Data(sizeof(spi_result), (uint8_t*)&spi_result);
                break;
            }

            case B200_VREQ_FPGA_CONFIG: {
                CyU3PUsbGetEP0Data(1, g_vendor_req_buffer, &read_count);

                CyU3PEventSet(&g_event_usb_config, EVENT_FPGA_CONFIG, CYU3P_EVENT_OR);
                break;
            }

            case B200_VREQ_GET_COMPAT: {
                CyU3PUsbSendEP0Data(/*2*/sizeof(compat_num), compat_num);
                break;
            }

            case B200_VREQ_SET_FPGA_HASH: {
                CyU3PUsbGetEP0Data(4, fpga_hash, &read_count);
                break;
            }

            case B200_VREQ_GET_FPGA_HASH: {
                CyU3PUsbSendEP0Data(/*4*/sizeof(fpga_hash), fpga_hash);
                break;
            }

            case B200_VREQ_SET_FW_HASH: {
                CyU3PUsbGetEP0Data(4, fw_hash, &read_count);
                break;
            }

            case B200_VREQ_GET_FW_HASH: {
                CyU3PUsbSendEP0Data(/*4*/sizeof(fw_hash), fw_hash);
                break;
            }

            case B200_VREQ_LOOP_CODE: {
                CyU3PUsbSendEP0Data(g_vendor_req_buff_size, g_vendor_req_buffer);
                break;
            }

            case B200_VREQ_GET_LOG: {
                LOCK(g_log_lock);

                if (log_buffer_len == 0) {
                    log_buffer[0] = '\0';
                    CyU3PUsbSendEP0Data(1, (uint8_t*)log_buffer);
                    //CyU3PUsbSendEP0Data(0, NULL);
                    //CyU3PUsbAckSetup();
                }
                else if (log_buffer_idx == 0)
                    CyU3PUsbSendEP0Data(log_buffer_len, (uint8_t*)log_buffer);
                else {
                    int len1 = min(LOG_BUFFER_SIZE - log_buffer_idx, log_buffer_len);
                    memcpy(log_contiguous_buffer, log_buffer + log_buffer_idx, len1);
                    //if ((log_buffer_idx + log_buffer_len) > LOG_BUFFER_SIZE)
                    if (len1 < log_buffer_len)
                        memcpy(log_contiguous_buffer + len1, log_buffer, log_buffer_len - len1);
                    CyU3PUsbSendEP0Data(log_buffer_len, (uint8_t*)log_contiguous_buffer);
                }

                // FIXME: Necessary? Not used in the other ones
                //CyU3PUsbSendEP0Data(0, NULL);   // Send ZLP since previous send has resulted in an integral # of packets

                log_reset();

                UNLOCK(g_log_lock);

                //log_reset();

                break;
            }

            case B200_VREQ_GET_COUNTERS: {
                LOCK(g_counters_lock);

                CyU3PUsbSendEP0Data(sizeof(COUNTERS), (uint8_t*)&g_counters);

                counters_auto_reset();

                UNLOCK(g_counters_lock);

                //counters_auto_reset();

                break;
            }

            case B200_VREQ_CLEAR_COUNTERS: {
                CyU3PUsbAckSetup();
                //CyU3PUsbGetEP0Data(g_vendor_req_buff_size, g_vendor_req_buffer, &read_count);   // Dummy

                counters_dma_reset();

                break;
            }

            case B200_VREQ_GET_USB_EVENT_LOG: {
                uint16_t idx = CyU3PUsbGetEventLogIndex();  // Current *write* pointer
                if (idx > (USB_EVENT_LOG_SIZE-1)) {
                    msg("! USB event log idx = %i", (int)idx);
                    break;
                }
                // Assuming logging won't wrap around between get calls (i.e. buffer should be long enough)
                uint16_t len = 0;
                if (idx < g_last_usb_event_log_index) {
                    uint16_t len1 = (USB_EVENT_LOG_SIZE - g_last_usb_event_log_index);
                    if (len1 > (USB_EVENT_LOG_SIZE-1)) {
                        msg("! USB event log len 2.1 = %i", (int)len1);
                        break;
                    }
                    len = len1 + idx;
                    if (len > (USB_EVENT_LOG_SIZE-1)) {
                        msg("! USB event log len 2.2 = %i", (int)len);
                        break;
                    }
                    memcpy(g_usb_event_log_contiguous_buf, g_usb_event_log + g_last_usb_event_log_index, len1);
                    memcpy(g_usb_event_log_contiguous_buf + len1, g_usb_event_log, idx);
                    //msg("USB event log [2] %i %i", (int)len1, (int)len);
                } else {
                    len = idx - g_last_usb_event_log_index;
                    if (len > (USB_EVENT_LOG_SIZE-1)) {
                        msg("! USB event log len 1 = %i", (int)len);
                        break;
                    }
                    if (len > 0) {    // ZLP should be OK
                        memcpy(g_usb_event_log_contiguous_buf, g_usb_event_log + g_last_usb_event_log_index, len);
                        //msg("USB event log [1] %i", (int)len);
                    }
                }

                //if (len > 0)  // Send a ZLP, otherwise it'll timeout
                    CyU3PUsbSendEP0Data(len, g_usb_event_log_contiguous_buf);

                g_last_usb_event_log_index = idx;
                break;
            }

            case B200_VREQ_SET_CONFIG: {
                CyU3PUsbGetEP0Data(sizeof(CONFIG_MOD), (uint8_t*)g_vendor_req_buffer, &read_count);
                if (read_count == sizeof(CONFIG_MOD)) {
                    memcpy(&g_config_mod, g_vendor_req_buffer, sizeof(CONFIG_MOD));
                    CyU3PEventSet(&g_event_usb_config, EVENT_RE_ENUM, CYU3P_EVENT_OR);
                }
                break;
            }

            case B200_VREQ_GET_CONFIG: {
                CyU3PUsbSendEP0Data(sizeof(g_config), (uint8_t*)&g_config);
                break;
            }

            case B200_VREQ_WRITE_SB: {
                CyU3PUsbGetEP0Data(g_vendor_req_buff_size, (uint8_t*)g_vendor_req_buffer, &read_count);
#ifdef ENABLE_FPGA_SB
                uint16_t i;
                LOCK(g_suart_lock);
                for (i = 0; i < read_count; ++i)
                    sb_write(SUART_TXCHAR, g_vendor_req_buffer[i]);
                UNLOCK(g_suart_lock);

                msg("Wrote %d SB chars", read_count);
#else
                msg("SB is disabled");
#endif // ENABLE_FPGA_SB
                break;
            }

            case B200_VREQ_SET_SB_BAUD_DIV: {
                uint16_t div;
                CyU3PUsbGetEP0Data(sizeof(div), (uint8_t*)&div, &read_count);

                if (read_count == sizeof(div)) {
#ifdef ENABLE_FPGA_SB
                    LOCK(g_suart_lock);
                    sb_write(SUART_CLKDIV, div);
                    UNLOCK(g_suart_lock);
                    msg("SUART_CLKDIV = %d", div);
                    /*g_fpga_sb_uart_div*/g_config.sb_baud_div = div;   // Store for GPIF (FPGA) reset
#else
                    msg("SB is disabled");
#endif // ENABLE_FPGA_SB
                }
                else
                    msg("! SUART_CLKDIV received %d bytes", read_count);

                break;
            }

            case B200_VREQ_FLUSH_DATA_EPS: {
                //msg("Flushing data EPs...");

                //CyU3PUsbAckSetup();

                //CyU3PUsbResetEndpointMemories();

                // From host
                //CyU3PDmaChannelReset(&data_cons_to_prod_chan_handle);
                //CyU3PUsbFlushEp(DATA_ENDPOINT_PRODUCER);
                //CyU3PUsbResetEp(DATA_ENDPOINT_PRODUCER);
                //CyU3PDmaChannelSetXfer(&data_cons_to_prod_chan_handle, DMA_SIZE_INFINITE);

                //CyU3PDmaSocketDisable(DATA_RX_PPORT_SOCKET);

                //CyU3PDmaChannelReset(&data_cons_to_prod_chan_handle);
                if (CyU3PDmaChannelReset(&data_prod_to_cons_chan_handle) != CY_U3P_SUCCESS)
                {
                    msg("! CyU3PDmaChannelReset");
                }
                //CyU3PUsbFlushEp(DATA_ENDPOINT_PRODUCER);
                CyU3PUsbFlushEp(DATA_ENDPOINT_CONSUMER);
                //CyU3PUsbResetEp(DATA_ENDPOINT_PRODUCER);
//                CyU3PUsbResetEp(DATA_ENDPOINT_CONSUMER);
                //CyU3PUsbStall(DATA_ENDPOINT_CONSUMER, CyFalse, CyTrue);
                //CyU3PDmaChannelSetXfer(&data_cons_to_prod_chan_handle, DMA_SIZE_INFINITE);
                //CyU3PDmaSocketEnable(DATA_RX_PPORT_SOCKET);
                CyU3PDmaChannelSetXfer(&data_prod_to_cons_chan_handle, DMA_SIZE_INFINITE);

                //CyU3PUsbResetEndpointMemories();

                // To host
                //CyU3PDmaChannelReset(&data_prod_to_cons_chan_handle);
                //CyU3PUsbFlushEp(DATA_ENDPOINT_CONSUMER);
                //CyU3PUsbResetEp(DATA_ENDPOINT_CONSUMER);
                //CyU3PDmaChannelSetXfer(&data_prod_to_cons_chan_handle, DMA_SIZE_INFINITE);

                CyU3PUsbAckSetup();

                break;
            }

            case B200_VREQ_EEPROM_WRITE: {
                i2cAddr = 0xA0 | ((wValue & 0x0007) << 1);
                CyU3PUsbGetEP0Data(((wLength + 15) & 0xFFF0), g_vendor_req_buffer, NULL);

                CyFxUsbI2cTransfer (wIndex, i2cAddr, wLength,
                        g_vendor_req_buffer, CyFalse);
                break;
            }

            case B200_VREQ_EEPROM_READ: {
                i2cAddr = 0xA0 | ((wValue & 0x0007) << 1);
                CyU3PMemSet (g_vendor_req_buffer, 0, sizeof (g_vendor_req_buffer));
                CyFxUsbI2cTransfer (wIndex, i2cAddr, wLength,
                        g_vendor_req_buffer, CyTrue);

                CyU3PUsbSendEP0Data(wLength, g_vendor_req_buffer);
                break;
            }

            case B200_VREQ_TOGGLE_FPGA_RESET: {
                CyU3PUsbGetEP0Data(g_vendor_req_buff_size, g_vendor_req_buffer, \
                        &read_count);

                /* CyBool_t value = (g_vendor_req_buffer[0] & 0x01) ? CyTrue : CyFalse;
                CyU3PGpioSetValue(GPIO_FPGA_RESET, value); */
                break;
            }

            case B200_VREQ_TOGGLE_GPIF_RESET: {
                CyU3PUsbGetEP0Data(g_vendor_req_buff_size, g_vendor_req_buffer, \
                        &read_count);

                reset_gpif();
                break;
            }

            case B200_VREQ_RESET_DEVICE: {
                CyU3PUsbGetEP0Data(4, g_vendor_req_buffer, &read_count);

                CyU3PDeviceReset(CyFalse);  // FIXME: If CyTrue, this will *not* call static initialisers for global variables - must do this manually
                break;
            }

            case B200_VREQ_GET_USB_SPEED: {
                CyU3PUSBSpeed_t usb_speed = CyU3PUsbGetSpeed();
                switch(usb_speed) {
                    case CY_U3P_SUPER_SPEED:
                        g_vendor_req_buffer[0] = 3;
                        break;

                    case CY_U3P_FULL_SPEED:
                    case CY_U3P_HIGH_SPEED:
                        g_vendor_req_buffer[0] = 2;
                        break;

                    default:
                        g_vendor_req_buffer[0] = 1;
                        break;
                }

                CyU3PUsbSendEP0Data(1, g_vendor_req_buffer);
                break;
            }

            case B200_VREQ_GET_STATUS: {
                g_vendor_req_buffer[0] = g_fx3_state;
                CyU3PUsbSendEP0Data(1, g_vendor_req_buffer);
                break;
            }

            default:
                msg("! Unknown VREQ %02X", (uint32_t)bRequest);
                handled = CyFalse;
        }

        /* After processing the vendor request, flush the endpoints. */
        CyU3PUsbFlushEp(VREQ_ENDPOINT_PRODUCER);
        CyU3PUsbFlushEp(VREQ_ENDPOINT_CONSUMER);
    }

    return handled;
}


/* Callback function to handle LPM requests from the USB 3.0 host. This function
 * is invoked by the API whenever a state change from U0 -> U1 or U0 -> U2
 * happens.
 *
 * If we return CyTrue from this function, the FX3 device is retained
 * in the low power state.  If we return CyFalse, the FX3 device immediately
 * tries to trigger an exit back to U0.
 */
CyBool_t lpm_request_callback(CyU3PUsbLinkPowerMode link_mode) {
    msg("! lpm_request_callback = %i", link_mode);
    return
//#ifdef PREVENT_LOW_POWER_MODE
    CyFalse;    // This still allows my laptop to sleep
//#else
//    CyTrue;
//#endif // PREVENT_LOW_POWER_MODE
}


/* Callback function to check for PIB ERROR*/
void gpif_error_cb(CyU3PPibIntrType cbType, uint16_t cbArg)
{
    if (cbType==CYU3P_PIB_INTR_ERROR)
    {
        switch (CYU3P_GET_PIB_ERROR_TYPE(cbArg))
        {
            case CYU3P_PIB_ERR_NONE:
                break;
            case CYU3P_PIB_ERR_THR0_WR_OVERRUN:
                msg("CYU3P_PIB_ERR_THR0_WR_OVERRUN");
                break;
            case CYU3P_PIB_ERR_THR1_WR_OVERRUN:
                msg("CYU3P_PIB_ERR_THR1_WR_OVERRUN");
                break;
            case CYU3P_PIB_ERR_THR2_WR_OVERRUN:
                msg("CYU3P_PIB_ERR_THR2_WR_OVERRUN");
                break;
            case CYU3P_PIB_ERR_THR3_WR_OVERRUN:
                msg("CYU3P_PIB_ERR_THR3_WR_OVERRUN");
                break;
            case CYU3P_PIB_ERR_THR0_RD_UNDERRUN:
                msg("CYU3P_PIB_ERR_THR0_RD_UNDERRUN");
                break;
            case CYU3P_PIB_ERR_THR1_RD_UNDERRUN:
                msg("CYU3P_PIB_ERR_THR1_RD_UNDERRUN");
                break;
            case CYU3P_PIB_ERR_THR2_RD_UNDERRUN:
                msg("CYU3P_PIB_ERR_THR2_RD_UNDERRUN");
                break;
            case CYU3P_PIB_ERR_THR3_RD_UNDERRUN:
                msg("CYU3P_PIB_ERR_THR3_RD_UNDERRUN");
                break;
            case CYU3P_PIB_ERR_THR0_ADAP_UNDERRUN:
                msg("CYU3P_PIB_ERR_THR0_ADAP_UNDERRUN");
                break;
            case CYU3P_PIB_ERR_THR1_ADAP_OVERRUN:
                msg("CYU3P_PIB_ERR_THR1_ADAP_OVERRUN");
                break;
            // FIXME: Other threads
            case CYU3P_PIB_ERR_THR1_SCK_INACTIVE:
                //msg("CYU3P_PIB_ERR_THR1_SCK_INACTIVE");
                ++g_counters.pib_counters[1].socket_inactive;   // UNSYNC'd
                break;
            default:
                msg("Unknown CYU3P_PIB_ERR %i", CYU3P_GET_PIB_ERROR_TYPE(cbArg));
                break;
        }

        switch (CYU3P_GET_GPIF_ERROR_TYPE(cbArg))
        {
            case CYU3P_GPIF_ERR_NONE:             /**< No GPIF state machine errors. */
                //msg("CYU3P_GPIF_ERR_NONE");
                break;
            case CYU3P_GPIF_ERR_INADDR_OVERWRITE: /**< Content of INGRESS_ADDR register is overwritten before read. */
                msg("CYU3P_GPIF_ERR_INADDR_OVERWRITE");
                break;
            case CYU3P_GPIF_ERR_EGADDR_INVALID:   /**< Attempt to read EGRESS_ADDR register before it is written to. */
                msg("CYU3P_GPIF_ERR_EGADDR_INVALID");
                break;
            case CYU3P_GPIF_ERR_DATA_READ_ERR:    /**< Read from DMA data thread which is not ready. */
                msg("CYU3P_GPIF_ERR_DATA_READ_ERR");
                break;
            case CYU3P_GPIF_ERR_DATA_WRITE_ERR:   /**< Write to DMA data thread which is not ready. */
                msg("CYU3P_GPIF_ERR_DATA_WRITE_ERR");
                break;
            case CYU3P_GPIF_ERR_ADDR_READ_ERR:    /**< Read from DMA address thread which is not ready. */
                msg("CYU3P_GPIF_ERR_ADDR_READ_ERR");
                break;
            case CYU3P_GPIF_ERR_ADDR_WRITE_ERR:   /**< Write to DMA address thread which is not ready. */
                msg("CYU3P_GPIF_ERR_ADDR_WRITE_ERR");
                break;
            case CYU3P_GPIF_ERR_INVALID_STATE:    /**< GPIF state machine has reached an invalid state. */
                //msg("CYU3P_GPIF_ERR_INVALID_STATE");
                ++g_counters.invalid_gpif_state;    // UNSYNC'd
                break;
            default:
                msg("Unknown CYU3P_GPIF_ERR %i", CYU3P_GET_GPIF_ERROR_TYPE(cbArg));
                break;
        }
    }
}


void GpifStateChangeCb(uint8_t stateId)
{
    //msg("%d", stateId);
    ++g_counters.state_transition_count;
}


/*! Initialize and start the GPIF state machine.
 *
 * This function starts the GPIF Slave FIFO state machine on the FX3. Because on
 * of the GPIF pins is used for FPGA configuration, this cannot be done until
 * after FPGA configuration is complete. */
void b200_gpif_init(void) {
    msg("b200_gpif_init");

    CyU3PPibClock_t pib_clock_config;

    /* Initialize the p-port block; disable DLL for sync GPIF. */
    pib_clock_config.clkDiv = 2;
    pib_clock_config.clkSrc = CY_U3P_SYS_CLK;
    pib_clock_config.isHalfDiv = CyFalse;
    pib_clock_config.isDllEnable = CyFalse;
    if (CyU3PPibInit(CyTrue, &pib_clock_config) != CY_U3P_SUCCESS)
        msg("! CyU3PPibInit");

    /* Load the GPIF configuration for Slave FIFO sync mode. */
    if (CyU3PGpifLoad(&CyFxGpifConfig) != CY_U3P_SUCCESS)
        msg("! CyU3PGpifLoad");

    msg("GPIF loaded");

    //CyU3PGpifRegisterSMIntrCallback(GpifStateChangeCb);

    /* Start the state machine. */
    //CyU3PGpifSMStart(RESET, ALPHA_RESET);

    /* Configure the watermarks for the slfifo-write buffers. */
    if (CyU3PGpifSocketConfigure(0, DATA_TX_PPORT_SOCKET, 5, CyFalse, 1) != CY_U3P_SUCCESS)
        msg("! CyU3PGpifSocketConfigure 0");
    if (CyU3PGpifSocketConfigure(1, DATA_RX_PPORT_SOCKET, 6, CyFalse, 1) != CY_U3P_SUCCESS)
        msg("! CyU3PGpifSocketConfigure 1");
    if (CyU3PGpifSocketConfigure(2, CTRL_COMM_PPORT_SOCKET, 5, CyFalse, 1) != CY_U3P_SUCCESS)
        msg("! CyU3PGpifSocketConfigure 2");
    if (CyU3PGpifSocketConfigure(3, CTRL_RESP_PPORT_SOCKET, 6, CyFalse, 1) != CY_U3P_SUCCESS)
        msg("! CyU3PGpifSocketConfigure 3");

    //CyU3PGpifSMStart(RESET, ALPHA_RESET);

    /* Register a callback for notification of PIB interrupts*/
    CyU3PPibRegisterCallback(gpif_error_cb, CYU3P_PIB_INTR_ERROR);
}


/*! Start and configure the FX3's SPI module.
 *
 * This module is used for programming the FPGA. After the FPGA is configured,
 * the SPI module is disabled, as it cannot be used while we are using GPIF
 * 32-bit mode. */
CyU3PReturnStatus_t b200_spi_init(void) {
    msg("b200_spi_init");

    CyU3PSpiConfig_t spiConfig;

    /* Start the SPI module and configure the master. */
    CyU3PSpiInit();

    /* Start the SPI master block. Run the SPI clock at 8MHz
     * and configure the word length to 8 bits. Also configure
     * the slave select using FW. */
    CyU3PMemSet ((uint8_t *)&spiConfig, 0, sizeof(spiConfig));
    spiConfig.isLsbFirst = CyFalse;
    spiConfig.cpol       = CyFalse;
    spiConfig.cpha       = CyFalse;
    spiConfig.ssnPol     = CyTrue;
    spiConfig.leadTime   = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK;
    spiConfig.lagTime    = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK;
    spiConfig.ssnCtrl    = CY_U3P_SPI_SSN_CTRL_FW;
    spiConfig.clock      = 20000000;
    spiConfig.wordLen    = 8;

    CyU3PReturnStatus_t res = CyU3PSpiSetConfig(&spiConfig, NULL);

    if (res != CY_U3P_SUCCESS)
        msg("! CyU3PSpiSetConfig");

    return res;
}


/*! Initialize the USB module of the FX3 chip.
 *
 * This function handles USB initialization, re-enumeration (and thus coming up
 * as a USRP B200 device), configures USB endpoints and the DMA module.
 */
void b200_usb_init(void) {
    //msg("b200_usb_init");

    /* Initialize the I2C interface for the EEPROM of page size 64 bytes. */
    CyFxI2cInit(CY_FX_USBI2C_I2C_PAGE_SIZE);

    /* Start the USB system! */
    CyU3PUsbStart();

    /* Register our USB Setup callback. The boolean parameter indicates whether
     * or not we are using FX3's 'Fast Enumeration' mode, which relies on the
     * USB driver auto-detecting the connection speed and setting the correct
     * descriptors. */
    CyU3PUsbRegisterSetupCallback(usb_setup_callback, CyTrue);

    CyU3PUsbRegisterEventCallback(event_usb_callback);

    CyU3PUsbRegisterLPMRequestCallback(lpm_request_callback);

    /* Check to see if a VID/PID is in the EEPROM that we should use. */
    uint8_t valid[4];
    uint8_t vidpid[4];
    CyU3PMemSet(valid, 0, 4);
    CyFxUsbI2cTransfer(0x0, 0xA0, 4, valid, CyTrue);
    if(*((uint32_t *) &(valid[0])) == 0xB2145943) {

        /* Pull the programmed device serial out of the i2c EEPROM, and copy the
         * characters into the device serial string, which is then advertised as
         * part of the USB descriptors. */
        CyU3PMemSet(vidpid, 0, 4);
        CyFxUsbI2cTransfer(0x4, 0xA0, 4, vidpid, CyTrue);
        b200_usb2_dev_desc[8] = vidpid[2];
        b200_usb2_dev_desc[9] = vidpid[3];
        b200_usb2_dev_desc[10] = vidpid[0];
        b200_usb2_dev_desc[11] = vidpid[1];

        b200_usb3_dev_desc[8] = vidpid[2];
        b200_usb3_dev_desc[9] = vidpid[3];
        b200_usb3_dev_desc[10] = vidpid[0];
        b200_usb3_dev_desc[11] = vidpid[1];
    }

    /* We support two VIDs:
     *  Ettus Research:         0x2500
     *  National Instruments:   0x3923
     *
     * We support three PIDs:
     *  Ettus B200/B210:        0x0020
     *  NI USRP-2900:           0x7813
     *  NI USRP-2901:           0x7814
     */
    uint8_t *mfr_string = NULL;
    uint8_t *product_string = NULL;
    if((vidpid[3] == 0x25) && (vidpid[2] == 0x00)) {
        mfr_string = b200_usb_manufacture_desc;
        product_string = b200_usb_product_desc;
    } else if((vidpid[3] == 0x39) && (vidpid[2] == 0x23)) {
        mfr_string = niusrp_usb_manufacture_desc;

        if((vidpid[1] == 0x78) && (vidpid[0] == 0x13)) {
            product_string = niusrp_2900_usb_product_desc;
        } else if((vidpid[1] == 0x78) && (vidpid[0] == 0x14)) {
            product_string = niusrp_2901_usb_product_desc;
        } else {
            product_string = unknown_desc;
        }
    } else {
        mfr_string = unknown_desc;
        product_string = unknown_desc;
    }

    uint8_t ascii_serial[9];
    CyU3PMemSet(ascii_serial, 0, 9);
    CyFxUsbI2cTransfer(0x4f7, 0xA0, 9, ascii_serial, CyTrue);
    uint8_t count;
    dev_serial[0] = 2;
    for(count = 0; count < 9; count++) {
        uint8_t byte = ascii_serial[count];
        if (byte < 32 || byte > 127) break;
        dev_serial[2 + (count * 2)] = byte;
        // FIXME: Set count*2 + 1 = 0x00 ?
        dev_serial[0] += 2;
    }

    /* Set our USB enumeration descriptors! Note that there are different
    * function calls for each USB speed: FS, HS, SS. */

    /* Device descriptors */
    CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, 0,
            (uint8_t *) b200_usb2_dev_desc);

    CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, 0,
            (uint8_t *) b200_usb3_dev_desc);

    /* Device qualifier descriptors */
    CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, 0,
            (uint8_t *) b200_dev_qual_desc);

    /* Configuration descriptors */
    CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, 0,
            (uint8_t *) b200_usb_hs_config_desc);

    CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, 0,
            (uint8_t *) b200_usb_fs_config_desc);

    CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, 0,
            (uint8_t *) b200_usb_ss_config_desc);

    /* BOS Descriptor */
    CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, 0,
            (uint8_t *) b200_usb_bos_desc);

    /* String descriptors */
    CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0,
            (uint8_t *) b200_string_lang_id_desc);

    CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1,
            (uint8_t *) mfr_string);

    CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2,
            (uint8_t *) product_string);

    CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 3,
            (uint8_t *) dev_serial);

    ////////////////////////////////////////////////////////

    // FIXME: CyU3PUsbSetTxDeemphasis(0x11); <0x1F  // Shouldn't need to change this

    const uint32_t tx_swing = g_config.tx_swing/*65*//*45*/;   // 65 & 45 are OK, 120 causes much link recovery. <128. 1.2V is USB3 limit.
    if (CyU3PUsbSetTxSwing(tx_swing) == CY_U3P_SUCCESS)
        msg("CyU3PUsbSetTxSwing: %d", tx_swing);
    else
        msg("! CyU3PUsbSetTxSwing: %d", tx_swing);

    ////////////////////////////////////////////////////////

    /* Connect the USB pins, and enable SuperSpeed (USB 3.0). */
    if (CyU3PConnectState(CyTrue, (g_config.enable_as_superspeed != 0 ? CyTrue : CyFalse)) == CY_U3P_SUCCESS) {  // connect, ssEnable
        CyU3PUSBSpeed_t usb_speed = CyU3PUsbGetSpeed();
        msg("Link up (speed: USB %d)", (int)usb_speed); // MAGIC: Values happen to map
    }
    else
        msg("! Failed to establish link");
}


void b200_restore_gpio_for_fpga_config(void) {
    CyU3PDeviceGpioRestore(GPIO_FPGA_RESET);
    CyU3PDeviceGpioRestore(GPIO_DONE);

    CyU3PDeviceGpioRestore(GPIO_FX3_SCLK);
    CyU3PDeviceGpioRestore(GPIO_FX3_CE);
    CyU3PDeviceGpioRestore(GPIO_FX3_MISO);
    CyU3PDeviceGpioRestore(GPIO_FX3_MOSI);

    //CyU3PGpioDeInit();    // Moved to just before init
}

void thread_fpga_config_entry(uint32_t input) {
    uint32_t event_flag;

    //msg("thread_fpga_config_entry");

    for(;;) {

        // Event is set through VREQ
        if(CyU3PEventGet(&g_event_usb_config, \
            (EVENT_FPGA_CONFIG), CYU3P_EVENT_AND_CLEAR, \
            &event_flag, CYU3P_WAIT_FOREVER) == CY_U3P_SUCCESS) {

            //uint8_t old_state = g_fx3_state;
            uint32_t old_fpga_programming_write_count = 0;

            if(g_fx3_state == STATE_ERROR) {
                CyU3PThreadRelinquish();
                continue;
            }

            if(g_fx3_state == STATE_RUNNING) {
                /* The FX3 is currently configured for SLFIFO mode. We need to tear down
                * this configuration and re-configure to program the FPGA. */
                b200_restore_gpio_for_fpga_config();
                CyU3PGpifDisable(CyTrue);
            }

            CyU3PSysWatchDogClear();

            g_fx3_state = STATE_BUSY;

            /* Configure the device GPIOs for FPGA programming. */
            b200_gpios_pre_fpga_config();

            CyU3PSysWatchDogClear();

            /* Initialize the SPI module that will be used for FPGA programming. */
            b200_spi_init();    // This must be done *after* 'b200_gpios_pre_fpga_config'

            CyU3PSysWatchDogClear();

            /* Wait for the signal from the host that the bitstream is starting. */
            uint32_t wait_count = 0;

            /* We can now begin configuring the FPGA. */
            g_fx3_state = STATE_FPGA_READY;

            msg("Begin FPGA");

            // Event is set through VREQ
            while(CyU3PEventGet(&g_event_usb_config, \
                (EVENT_BITSTREAM_START), CYU3P_EVENT_AND_CLEAR, \
                &event_flag, CYU3P_NO_WAIT) != CY_U3P_SUCCESS) {

                if(wait_count >= FPGA_PROGRAMMING_BITSTREAM_START_POLL_COUNT) {
                    msg("! Bitstream didn't start");
                    g_fx3_state = STATE_UNCONFIGURED;   // Since IO configuration has changed, leave it in the unconfigured state (rather than the previous one, which might have been running)
                    CyU3PThreadRelinquish();
                    break;
                }

                wait_count++;
                CyU3PThreadSleep(FPGA_PROGRAMMING_POLL_SLEEP);
                CyU3PSysWatchDogClear();
            }

            if (wait_count >= FPGA_PROGRAMMING_BITSTREAM_START_POLL_COUNT)
                continue;

            /* Pull PROGRAM_B low and then release it. */
            CyU3PGpioSetValue(GPIO_PROGRAM_B, 0);
            CyU3PThreadSleep(20);
            CyU3PGpioSetValue(GPIO_PROGRAM_B, 1);

            /* Wait for INIT_B to fall and rise. */
            wait_count = 0;

            msg("Wait FPGA");

            while(CyU3PEventGet(&g_event_usb_config, \
                (EVENT_GPIO_INITB_RISE), CYU3P_EVENT_AND_CLEAR, \
                &event_flag, CYU3P_NO_WAIT) != CY_U3P_SUCCESS) {

                if(wait_count >= FPGA_PROGRAMMING_INITB_POLL_COUNT) {
                    msg("! INITB didn't rise");
                    g_fx3_state = STATE_UNCONFIGURED;   // Safer to call it unconfigured than the previous state
                    CyU3PThreadRelinquish();
                    break;
                }

                wait_count++;
                CyU3PThreadSleep(FPGA_PROGRAMMING_POLL_SLEEP);
                CyU3PSysWatchDogClear();
            }
#ifdef ENABLE_INIT_B_WORKAROUND
            if (wait_count >= FPGA_PROGRAMMING_INITB_POLL_COUNT)
            {
                CyBool_t gpio_init_b;
                CyU3PGpioGetValue(GPIO_INIT_B, &gpio_init_b);
                if (gpio_init_b == CyTrue)
                {
                    wait_count = 0;
                }
                else
                {
                    msg("! INIT_B still not high");
                }
            }
#endif // ENABLE_INIT_B_WORKAROUND
            if (wait_count >= FPGA_PROGRAMMING_INITB_POLL_COUNT)
                continue;

            /* We are ready to accept the FPGA bitstream! */
            wait_count = 0;
            g_fx3_state = STATE_CONFIGURING_FPGA;

            msg("Configuring FPGA");

            // g_fpga_programming_write_count is zero'd by VREQ triggering EVENT_BITSTREAM_START

            while(CyU3PEventGet(&g_event_usb_config, \
                (EVENT_GPIO_DONE_HIGH), CYU3P_EVENT_AND_CLEAR, \
                &event_flag, CYU3P_NO_WAIT) != CY_U3P_SUCCESS) {

                /* Wait for the configuration to complete, which will be indicated
                 * by the DONE pin going high and triggering the associated
                 * interrupt. */

                if(wait_count >= FPGA_PROGRAMMING_DONE_POLL_COUNT) {
                    msg("! DONE didn't go high");
                    g_fx3_state = STATE_UNCONFIGURED;
                    CyU3PThreadRelinquish();
                    break;
                }

                if (old_fpga_programming_write_count == g_fpga_programming_write_count) // Only increment wait count if we haven't written anything
                    wait_count++;
                else {
                    wait_count = 0;
                    old_fpga_programming_write_count = g_fpga_programming_write_count;
                }

                CyU3PThreadSleep(FPGA_PROGRAMMING_POLL_SLEEP);
                CyU3PSysWatchDogClear();
            }
#ifdef ENABLE_DONE_WORKAROUND
            if (wait_count >= FPGA_PROGRAMMING_DONE_POLL_COUNT)
            {
                CyBool_t gpio_done;
                CyU3PGpioGetValue(GPIO_DONE, &gpio_done);
                if (gpio_done == CyTrue)
                {
                    wait_count = 0;
                }
                else
                {
                    msg("! DONE still not high");
                }
            }
#endif // ENABLE_DONE_WORKAROUND
            if (wait_count >= FPGA_PROGRAMMING_DONE_POLL_COUNT)
                continue;

            msg("FPGA done");

            /* Tell the host that we are ignoring it for a while. */
            g_fx3_state = STATE_BUSY;

            CyU3PSysWatchDogClear();

            /* Now that the FPGA is configured, we need to tear down the current SPI and
            * GPIO configs, and re-config for GPIF & bit-banged SPI operation. */
            CyU3PSpiDeInit();
            b200_restore_gpio_for_fpga_config();

            CyU3PSysWatchDogClear();

            /* Load the GPIO configuration for normal SLFIFO use. */
            b200_slfifo_mode_gpio_config();

            /* Tone down the drive strength on the P-port. */
            //CyU3PSetPportDriveStrength(CY_U3P_DS_HALF_STRENGTH);

            CyU3PSysWatchDogClear();

            /* FPGA configuration is complete! Time to get the GPIF state machine
            * running for Slave FIFO. */
            b200_gpif_init();

            CyU3PThreadSleep(1);
            b200_start_fpga_sb_gpio();  // Moved here to give SB time to init

            /* RUN, BABY, RUN! */
            g_fx3_state = STATE_RUNNING;

            msg("Running");
        }

        CyU3PThreadRelinquish();
    }
}


/*! The primary program thread.
 *
 * This is the primary application thread running on the FX3 device.  It is
 * responsible for initializing much of the chip, and then bit-banging the FPGA
 * image, as it is sent from the host, into the FPGA.  It then re-configures the
 * FX3 for slave-fifo, and enters an infinite loop where it simply updates the
 * watchdog timer and does some minor power management state checking.
 */
void thread_main_app_entry(uint32_t input) {
    //msg("thread_main_app_entry");

    /* In your spectrum, stealing your Hz. */
    for(;;) {
        CyU3PSysWatchDogClear();
        CyU3PThreadSleep(CHECK_POWER_STATE_SLEEP_TIME);
#ifdef PREVENT_LOW_POWER_MODE
        /* Once data transfer has started, we keep trying to get the USB
         * link to stay in U0. If this is done
         * before data transfers have started, there is a likelihood of
         * failing the TD 9.24 U1/U2 test. */
        {
            CyU3PUsbLinkPowerMode current_state;

            if((CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)) {

                /* If the link is in U1/U2 states, try to get back to U0. */
                CyU3PUsbGetLinkPowerState(&current_state);

                if (current_state > CyU3PUsbLPM_U3)
                    msg("Power state %i", current_state);

                while((current_state >= CyU3PUsbLPM_U1) \
                        && (current_state <= CyU3PUsbLPM_U3)) {

                    msg("! LPS = %i", current_state);

                    CyU3PUsbSetLinkPowerState(CyU3PUsbLPM_U0);  // This will wake up the host if it's trying to sleep
                    CyU3PThreadSleep(1);

                    if (CyU3PUsbGetSpeed () != CY_U3P_SUPER_SPEED)
                        break;

                    CyU3PUsbGetLinkPowerState (&current_state);
                }
            }
        }
#endif // PREVENT_LOW_POWER_MODE
    }
}

static uint16_t g_poll_last_phy_error_count = 0, g_poll_last_link_error_count = 0;
static uint32_t g_poll_last_phy_error_status = 0;

void update_error_counters(void) {
    if (CyU3PUsbGetSpeed () != CY_U3P_SUPER_SPEED)
        return;

    uvint32_t reg = REG_LNK_PHY_ERROR_STATUS;
    uint32_t val = 0;
    if (CyU3PReadDeviceRegisters((uvint32_t*)reg, 1, &val) == CY_U3P_SUCCESS) {
        g_poll_last_phy_error_status |= (val & PHYERR_MASK);

        // Reset after read
        uint32_t zero = PHYERR_MASK;
        if (CyU3PWriteDeviceRegisters((uvint32_t*)reg, 1, &zero) != CY_U3P_SUCCESS)
            msg("! CyU3PWriteDeviceRegisters");
    }
    else {
        // FIXME: Log once
        msg("! Reg read fail");
    }

    // Equivalent code:
    //uint32_t* p = (uint32_t*)REG_LNK_PHY_ERROR_STATUS;
    //val = (*p);
    //(*p) = PHYERR_MASK;

    uint16_t phy_error_count = 0, link_error_count = 0;
    if (CyU3PUsbGetErrorCounts(&phy_error_count, &link_error_count) == CY_U3P_SUCCESS) {    // Resets internal counters after call
        g_poll_last_phy_error_count += phy_error_count;
        g_poll_last_link_error_count += link_error_count;
    }
    else {
        // FIXME: Log once
        msg("! CyU3PUsbGetErrorCounts");
    }

    LOCK(g_counters_lock);
    g_counters.usb_error_update_count++;
    g_counters.usb_error_counters.phy_error_count += phy_error_count;
    g_counters.usb_error_counters.link_error_count += link_error_count;
    if (val & PHYERR_MASK) {
        if (val & PHYERR_PHY_LOCK_EV)           g_counters.usb_error_counters.PHY_LOCK_EV++;
        if (val & PHYERR_TRAINING_ERROR_EV)     g_counters.usb_error_counters.TRAINING_ERROR_EV++;
        if (val & PHYERR_RX_ERROR_CRC32_EV)     g_counters.usb_error_counters.RX_ERROR_CRC32_EV++;
        if (val & PHYERR_RX_ERROR_CRC16_EV)     g_counters.usb_error_counters.RX_ERROR_CRC16_EV++;
        if (val & PHYERR_RX_ERROR_CRC5_EV)      g_counters.usb_error_counters.RX_ERROR_CRC5_EV++;
        if (val & PHYERR_PHY_ERROR_DISPARITY_EV)g_counters.usb_error_counters.PHY_ERROR_DISPARITY_EV++;
        if (val & PHYERR_PHY_ERROR_EB_UND_EV)   g_counters.usb_error_counters.PHY_ERROR_EB_UND_EV++;
        if (val & PHYERR_PHY_ERROR_EB_OVR_EV)   g_counters.usb_error_counters.PHY_ERROR_EB_OVR_EV++;
        if (val & PHYERR_PHY_ERROR_DECODE_EV)   g_counters.usb_error_counters.PHY_ERROR_DECODE_EV++;
    }
    UNLOCK(g_counters_lock);    // FIXME: Read/write regs
}


void thread_re_enum_entry(uint32_t input) {
    uint32_t event_flag;

    //msg("thread_re_enum_entry");

    int keep_alive = 0;

    while (1) {
        if (CyU3PEventGet(&g_event_usb_config, \
            (EVENT_RE_ENUM), CYU3P_EVENT_AND_CLEAR, \
            &event_flag, RE_ENUM_THREAD_SLEEP_TIME) == CY_U3P_SUCCESS) {
                msg("Re-config");

                // FIXME: This section is not finished

                // Not locking this since we only expect one write in VREQ and read afterward here

                int re_enum = g_config_mod.flags & (CF_RE_ENUM | CF_TX_SWING | CF_TX_DEEMPHASIS | CF_DISABLE_USB2 | CF_ENABLE_AS_SUPERSPEED);

                CyU3PThreadSleep(100);  // Wait for EP0 xaction to complete

                //b200_fw_stop();

/*
    int tx_swing;               // [90] [65] 45
    int tx_deemphasis;          // 0x11
    int disable_usb2;           // 0
    int enable_as_superspeed;   // 1
    int pport_drive_strength;   // CY_U3P_DS_THREE_QUARTER_STRENGTH
    int dma_buffer_size;        // [USB3] (max)
    int dma_buffer_count;       // [USB3] 1
    int manual_dma;             // 0
    int sb_baud_div;            // 434*2
*/

                if (re_enum) {
                    if (CyU3PConnectState(CyFalse, (g_config.enable_as_superspeed != 0 ? CyTrue : CyFalse)) == CY_U3P_SUCCESS)
                        msg("Link down");
                    else
                        msg("! Failed to bring link down");
                }

                if (g_config_mod.flags & CF_TX_DEEMPHASIS) {
#if (CYFX_VERSION_MAJOR >= 1) && (CYFX_VERSION_MINOR >= 3)
                    if ((g_config_mod.config.tx_deemphasis < 0x1F) && (CyU3PUsbSetTxDeemphasis(g_config_mod.config.tx_deemphasis) == CY_U3P_SUCCESS)) {
                        msg("TX deemphasis now: %d (was: %d)", g_config_mod.config.tx_deemphasis, g_config.tx_deemphasis);
                        g_config.tx_deemphasis = g_config_mod.config.tx_deemphasis;
                    }
                    else
#endif // #if (CYFX_VERSION_MAJOR >= 1) && (CYFX_VERSION_MINOR >= 3)
                        msg("! Failed to set TX deemphasis: %d (still: %d)", g_config_mod.config.tx_deemphasis, g_config.tx_deemphasis);
                }

                if (g_config_mod.flags & CF_TX_SWING) {
                    if ((g_config_mod.config.tx_swing < 128) && (CyU3PUsbSetTxSwing(g_config_mod.config.tx_swing) == CY_U3P_SUCCESS)) {
                        msg("TX swing now: %d (was: %d)", g_config_mod.config.tx_swing, g_config.tx_swing);
                        g_config.tx_swing = g_config_mod.config.tx_swing;
                    }
                    else
                        msg("! Failed to set TX swing: %d (still: %d)", g_config_mod.config.tx_swing, g_config.tx_swing);
                }

                if (g_config_mod.flags & CF_DISABLE_USB2) {
                    if (CyU3PUsbControlUsb2Support((g_config_mod.config.disable_usb2 != 0 ? CyTrue : CyFalse)) == CY_U3P_SUCCESS) {
                        msg("USB 2 support now: %s (was: %d)", (g_config_mod.config.disable_usb2 ? "disabled" : "enabled"), (g_config.disable_usb2 ? "disabled" : "enabled"));
                        g_config.disable_usb2 = g_config_mod.config.disable_usb2;
                    }
                    else
                        msg("! Failed to change USB 2 support to: %s (still: %s)", (g_config_mod.config.disable_usb2 ? "enabled" : "disabled"), (g_config.disable_usb2 ? "enabled" : "disabled"));
                }

                if (g_config_mod.flags & CF_PPORT_DRIVE_STRENGTH) {
                    // CY_U3P_DS_QUARTER_STRENGTH,CY_U3P_DS_HALF_STRENGTH,CY_U3P_DS_THREE_QUARTER_STRENGTH,CY_U3P_DS_FULL_STRENGTH
                    if ((g_config_mod.config.pport_drive_strength >= CY_U3P_DS_QUARTER_STRENGTH) &&
                        (g_config_mod.config.pport_drive_strength <= CY_U3P_DS_FULL_STRENGTH) &&
                        (CyU3PSetPportDriveStrength(g_config_mod.config.pport_drive_strength) == CY_U3P_SUCCESS)) {
                        msg("PPort drive strength now: %d (was: %d)", g_config_mod.config.pport_drive_strength, g_config.pport_drive_strength);
                        g_config.pport_drive_strength = g_config_mod.config.pport_drive_strength;
                    }
                    else
                        msg("! Failed to set PPort drive strength: %d (still: %d)", g_config_mod.config.pport_drive_strength, g_config.pport_drive_strength);
                }

                int reinit_dma = g_config_mod.flags & (CF_MANUAL_DMA | CF_DMA_BUFFER_COUNT | CF_DMA_BUFFER_SIZE);
                if (re_enum)
                    reinit_dma = 0; // Don't need to if re-enumerating

                if (g_config_mod.flags & CF_MANUAL_DMA) {
#ifdef ENABLE_MANUAL_DMA_XFER
                    msg("DMA transfers will be: %s (was: %s)", (g_config_mod.config.manual_dma ? "manual" : "auto"), (g_config.manual_dma ? "manual" : "auto"));
                    g_config.manual_dma = g_config_mod.config.manual_dma;
#else
                    msg("! Manual DMA transfers not compiled into FW");
#endif // ENABLE_MANUAL_DMA_XFER
                }

                if (g_config_mod.flags & CF_DMA_BUFFER_COUNT) {
                    msg("DMA buffer count will be: %d (was: %d)", g_config_mod.config.dma_buffer_count, g_config.dma_buffer_count);
                    g_config.dma_buffer_count = g_config_mod.config.dma_buffer_count;
                }

                if (g_config_mod.flags & CF_DMA_BUFFER_SIZE) {
                    msg("DMA buffer size will be: %d (was: %d)", g_config_mod.config.dma_buffer_size, g_config.dma_buffer_size);
                    g_config.dma_buffer_size = g_config_mod.config.dma_buffer_size;
                }

                if (g_config_mod.flags & CF_SB_BAUD_DIV) {
#ifdef ENABLE_FPGA_SB
                    LOCK(g_suart_lock);
                    sb_write(SUART_CLKDIV, g_config_mod.config.sb_baud_div);
                    UNLOCK(g_suart_lock);
                    msg("SUART_CLKDIV now: %d (was: %d)", g_config_mod.config.sb_baud_div, g_config.sb_baud_div);
                    g_config.sb_baud_div = g_config_mod.config.sb_baud_div;
#else
                    msg("! Failed to set SUART_CLKDIV: SB is disabled (still: %d)", g_config.sb_baud_div);
#endif // ENABLE_FPGA_SB
                }

                //b200_fw_start()

                if (g_config_mod.flags & CF_ENABLE_AS_SUPERSPEED) {
                    msg("Enable SuperSpeed: %s (was: %s)", (g_config_mod.config.enable_as_superspeed ? "yes" : "no"), (g_config.enable_as_superspeed ? "yes" : "no"));
                    g_config.enable_as_superspeed = g_config_mod.config.enable_as_superspeed;
                }

                if (reinit_dma) {
                    if (g_app_running) {
                        msg("Stopping FW...");

                        b200_fw_stop();
                    }

                    msg("Starting FW...");

                    b200_fw_start();
                }
                else // Shouldn't be re-init'ing AND re-enum'ing
                /* Connect the USB pins, and enable SuperSpeed (USB 3.0). */
                if (re_enum) {
                    msg("Connecting... (as SuperSpeed: %d)", g_config.enable_as_superspeed);

                    if (CyU3PConnectState(CyTrue, (g_config.enable_as_superspeed != 0 ? CyTrue : CyFalse)) == CY_U3P_SUCCESS) {  // CHECK: Assuming all other important state will persist
                        CyU3PUSBSpeed_t usb_speed = CyU3PUsbGetSpeed();
                        msg("Link up (speed: USB %d)", (int)usb_speed); // MAGIC: Values happen to map
                    }
                    else
                        msg("! Failed to bring link up");
                }

                counters_reset_usb_errors();
        }
        else {
            if (++keep_alive == KEEP_ALIVE_LOOP_COUNT) {
                msg("Keep-alive");
                keep_alive = 0;
            }
#ifndef ENABLE_FPGA_SB
            update_error_counters();
#endif // !ENABLE_FPGA_SB
        }

        CyU3PThreadRelinquish();
    }
}


void base16_encode(uint8_t v, char out[2], char first) {
    out[0] = first + (v >> 4);
    out[1] = first + (v & 0x0F);
}


#ifdef ENABLE_FPGA_SB
void thread_fpga_sb_poll_entry(uint32_t input) {
    //msg("thread_fpga_sb_poll_entry");

    while (1) {
        uint16_t i;
        uint8_t has_change = 0;

        update_error_counters();

        /*if (g_poll_last_phy_error_count > 0)
            has_change = 1;
        if (g_poll_last_link_error_count > 0)
            has_change = 1;*/
        if (g_poll_last_phy_error_status != 0)
            has_change = 1;

        uint16_t idx = CyU3PUsbGetEventLogIndex();  // Current *write* pointer
        if (idx > (USB_EVENT_LOG_SIZE-1)) {
            msg("! USB event log idx = %i", (int)idx);
            break;
        }

        uint8_t has_usb_events = 0;
        // Assuming logging won't wrap around between get calls (i.e. buffer should be long enough)
        if (g_fpga_sb_last_usb_event_log_index != idx) {
            if (idx < g_fpga_sb_last_usb_event_log_index) {
                for (i = g_fpga_sb_last_usb_event_log_index; i < USB_EVENT_LOG_SIZE; i++) {
                    if (g_usb_event_log[i] != 0x14 && g_usb_event_log[i] != 0x15 && g_usb_event_log[i] != 0x16) { // CTRL, STATUS, ACKSETUP
                        has_usb_events = 1;
                        break;
                    }
                }

                if (has_usb_events == 0) {
                    for (i = 0; i < idx; i++) {
                        if (g_usb_event_log[i] != 0x14 && g_usb_event_log[i] != 0x15 && g_usb_event_log[i] != 0x16) { // CTRL, STATUS, ACKSETUP
                            has_usb_events = 1;
                            break;
                        }
                    }
                }
            }
            else {
                for (i = g_fpga_sb_last_usb_event_log_index; i < idx; i++) {
                    if (g_usb_event_log[i] != 0x14 && g_usb_event_log[i] != 0x15 && g_usb_event_log[i] != 0x16) { // CTRL, STATUS, ACKSETUP
                        has_usb_events = 1;
                        break;
                    }
                }
            }
        }

        if (has_change || has_usb_events) {
            LOCK(g_suart_lock);

            sb_write(SUART_TXCHAR, UPT_USB_EVENTS);

            char out[3];
            out[2] = '\0';

            if (has_usb_events) {
                if (idx < g_fpga_sb_last_usb_event_log_index) {
                    for (i = g_fpga_sb_last_usb_event_log_index; i < USB_EVENT_LOG_SIZE; i++) {
                        if (g_usb_event_log[i] == 0x14 || g_usb_event_log[i] == 0x15 || g_usb_event_log[i] == 0x16) // CTRL, STATUS, ACKSETUP
                            continue;
                        base16_encode(g_usb_event_log[i], out, 'A');
                        _sb_write_string(out);
                    }

                    for (i = 0; i < idx; i++) {
                        if (g_usb_event_log[i] == 0x14 || g_usb_event_log[i] == 0x15 || g_usb_event_log[i] == 0x16) // CTRL, STATUS, ACKSETUP
                            continue;
                        base16_encode(g_usb_event_log[i], out, 'A');
                        _sb_write_string(out);
                    }
                }
                else {
                    for (i = g_fpga_sb_last_usb_event_log_index; i < idx; i++) {
                        if (g_usb_event_log[i] == 0x14 || g_usb_event_log[i] == 0x15 || g_usb_event_log[i] == 0x16) // CTRL, STATUS, ACKSETUP
                            continue;
                        base16_encode(g_usb_event_log[i], out, 'A');
                        _sb_write_string(out);
                    }
                }
            }

            // USB events: A-P,A-P
            // PHY error status: a,a-i

            if (g_poll_last_phy_error_status != 0) {
                uint32_t mask;
                size_t offset;
                for (mask = PHYERR_MAX, offset = 0; mask != 0; mask >>= 1, ++offset) {
                    if ((g_poll_last_phy_error_status & mask) != 0) {
                        sb_write(SUART_TXCHAR, 'a');
                        sb_write(SUART_TXCHAR, 'a' + offset);
                    }
                }
            }

            /*char buf[6];

            if (g_poll_last_phy_error_count > 0) {
                sb_write(SUART_TXCHAR, 'b');
                snprintf(buf, sizeof(buf)-1, "%d", g_poll_last_phy_error_count);
                _sb_write_string(buf);
            }

            if (g_poll_last_link_error_count > 0) {
                sb_write(SUART_TXCHAR, 'c');
                snprintf(buf, sizeof(buf)-1, "%d", g_poll_last_link_error_count);
                _sb_write_string(buf);
            }*/

            _sb_write_string("\r\n");

            UNLOCK(g_suart_lock);
        }

        g_poll_last_phy_error_count = 0;
        g_poll_last_link_error_count = 0;
        g_poll_last_phy_error_status = 0;

        g_fpga_sb_last_usb_event_log_index = idx;

        CyU3PThreadRelinquish();
    }
}
#endif // ENABLE_FPGA_SB

/*! Application define function which creates the threads.
 *
 * The name of this application cannot be changed, as it is called from the
 * tx_application _define function, referenced in the rest of the FX3 build
 * system.
 *
 * If thread creation fails, lock the system and force a power reset.
 */
void CyFxApplicationDefine(void) {
    void *app_thread_ptr, *fpga_thread_ptr;
#ifdef ENABLE_RE_ENUM_THREAD
    void *re_enum_thread_ptr;
#endif // ENABLE_RE_ENUM_THREAD
#ifdef ENABLE_FPGA_SB
    void *fpga_sb_poll_thread_ptr;
#endif // ENABLE_FPGA_SB

    g_counters.magic = COUNTER_MAGIC;
    //memset(&g_config, 0xFF, sizeof(g_config));  // Initialise to -1

    CyU3PMutexCreate(&g_log_lock, CYU3P_NO_INHERIT);
    CyU3PMutexCreate(&g_counters_lock, CYU3P_NO_INHERIT);
    CyU3PMutexCreate(&g_counters_dma_from_host_lock, CYU3P_NO_INHERIT);
    CyU3PMutexCreate(&g_counters_dma_to_host_lock, CYU3P_NO_INHERIT);
#ifdef ENABLE_FPGA_SB
    CyU3PMutexCreate(&g_suart_lock, CYU3P_NO_INHERIT);
#endif // ENABLE_FPGA_SB
#ifdef ENABLE_USB_EVENT_LOGGING
    CyU3PUsbInitEventLog(g_usb_event_log, USB_EVENT_LOG_SIZE);
#endif // ENABLE_USB_EVENT_LOGGING

    ////////////////////////////////////////////////////////

    /* Tell the host that we are ignoring it for a while. */
    g_fx3_state = STATE_BUSY;

    /* Set the FX3 compatibility number. */
    compat_num[0] = FX3_COMPAT_MAJOR;
    compat_num[1] = FX3_COMPAT_MINOR;

    /* Initialize the USB system. */
    b200_usb_init();

    /* Turn on the Watchdog Timer. */
    CyU3PSysWatchDogConfigure(CyTrue, WATCHDOG_TIMEOUT);

    /* Go do something. Probably not useful, because you aren't configured. */
    g_fx3_state = STATE_UNCONFIGURED;

    ////////////////////////////////////////////////////////

    b200_gpio_init(CyTrue);

    b200_enable_fpga_sb_gpio(CyTrue);

    msg("Compat:  %d.%d", FX3_COMPAT_MAJOR, FX3_COMPAT_MINOR);
    msg("FX3 SDK: %d.%d.%d (build %d)", CYFX_VERSION_MAJOR, CYFX_VERSION_MINOR, CYFX_VERSION_PATCH, CYFX_VERSION_BUILD);
    msg("FW built: %s %s", __TIME__, __DATE__);

    ////////////////////////////////////////////////////////

    /* Create the USB event group that we will use to track USB events from the
     * application thread. */
    CyU3PEventCreate(&g_event_usb_config);

    /* Allocate memory for the application thread. */
    app_thread_ptr = CyU3PMemAlloc(APP_THREAD_STACK_SIZE);

    /* Allocate memory for the FPGA configuration thread. */
    fpga_thread_ptr = CyU3PMemAlloc(APP_THREAD_STACK_SIZE);
#ifdef ENABLE_RE_ENUM_THREAD
    re_enum_thread_ptr = CyU3PMemAlloc(APP_THREAD_STACK_SIZE);
#endif // ENABLE_RE_ENUM_THREAD
#ifdef ENABLE_FPGA_SB
    fpga_sb_poll_thread_ptr = CyU3PMemAlloc(APP_THREAD_STACK_SIZE);
#endif // ENABLE_FPGA_SB
    ////////////////////////////////////////////////////////

    /* Create the thread for the application */
    if (app_thread_ptr != NULL)
        CyU3PThreadCreate(&thread_main_app,
                              "200:B200 Main",
                              thread_main_app_entry,
                              0,
                              app_thread_ptr,
                              APP_THREAD_STACK_SIZE,
                              THREAD_PRIORITY,
                              THREAD_PRIORITY,
                              CYU3P_NO_TIME_SLICE,
                              CYU3P_AUTO_START);

    /* Create the thread for FPGA configuration. */
    if (fpga_thread_ptr != NULL)
        CyU3PThreadCreate(&thread_fpga_config,
                              "300:B200 FPGA",
                              thread_fpga_config_entry,
                              0,
                              fpga_thread_ptr,
                              APP_THREAD_STACK_SIZE,
                              THREAD_PRIORITY,
                              THREAD_PRIORITY,
                              CYU3P_NO_TIME_SLICE,
                              CYU3P_AUTO_START);
#ifdef ENABLE_RE_ENUM_THREAD
    /* Create the thread for stats collection and re-enumeration/configuration */
    if (re_enum_thread_ptr != NULL)
        CyU3PThreadCreate(&thread_re_enum,
                              "400:B200 Re-enum",
                              thread_re_enum_entry,
                              0,
                              re_enum_thread_ptr,
                              APP_THREAD_STACK_SIZE,
                              THREAD_PRIORITY,
                              THREAD_PRIORITY,
                              CYU3P_NO_TIME_SLICE,
                              CYU3P_AUTO_START);
#endif // ENABLE_RE_ENUM_THREAD
#ifdef ENABLE_FPGA_SB
    /* Create thread to handling Settings Bus logging/transactions */
    if (fpga_sb_poll_thread_ptr != NULL)
        CyU3PThreadCreate(&thread_fpga_sb_poll,
                              "600:B200 FPGA SB poll",
                              thread_fpga_sb_poll_entry,
                              0,
                              fpga_sb_poll_thread_ptr,
                              APP_THREAD_STACK_SIZE,
                              THREAD_PRIORITY,
                              THREAD_PRIORITY,
                              CYU3P_NO_TIME_SLICE,
                              CYU3P_AUTO_START);
#endif // ENABLE_FPGA_SB
}


int main(void) {
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    CyU3PSysClockConfig_t clock_config;

    /* Configure the FX3 Clocking scheme:
     *      CPU Divider: 2 (~200 MHz)
     *      DMA Divider: 2 (~100 MHz)
     *      MMIO Divider: 2 (~100 MHz)
     *      32 kHz Standby Clock: Disabled
     *      System Clock Divider: 1 */
    clock_config.cpuClkDiv = 2;
    clock_config.dmaClkDiv = 2;
    clock_config.mmioClkDiv = 2;
    clock_config.useStandbyClk = CyFalse;
    clock_config.clkSrc = CY_U3P_SYS_CLK;
    clock_config.setSysClk400 = CyTrue;

    status = CyU3PDeviceInit(&clock_config);
    if(status != CY_U3P_SUCCESS)
        goto handle_fatal_error;

    /* Initialize the caches. Enable instruction cache and keep data cache disabled.
     * The data cache is useful only when there is a large amount of CPU based memory
     * accesses. When used in simple cases, it can decrease performance due to large
     * number of cache flushes and cleans and also it adds to the complexity of the
     * code. */
    status = CyU3PDeviceCacheControl(CyTrue, CyFalse, CyFalse); // Icache, Dcache, DMAcache
    if (status != CY_U3P_SUCCESS)
        goto handle_fatal_error;

    /* Configure the IO peripherals on the FX3. The gpioSimpleEn arrays are
     * bitmaps, where each bit represents the GPIO of the matching index - the
     * second array is index + 32. */
    status = b200_set_io_matrix(CyTrue);
    if(status != CY_U3P_SUCCESS)
        goto handle_fatal_error;

    /* This function calls starts the RTOS kernel.
     *
     * ABANDON ALL HOPE, YE WHO ENTER HERE */
    CyU3PKernelEntry();

    /* Although we will never make it here, this has to be here to make the
     * compiler happy. */
    return 0;

    /* If an error occurs before the launch of the kernel, it is unrecoverable.
     * Once you go down this hole, you aren't coming back out without a power
     * reset. */
    handle_fatal_error:
        while(1);
}
