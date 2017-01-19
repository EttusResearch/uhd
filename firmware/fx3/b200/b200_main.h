//
// Copyright 2013-2014 Ettus Research LLC
//

#ifndef _B200_MAIN_H
#define _B200_MAIN_H

#include "cyu3externcstart.h"

#include "cyu3types.h"
#include "cyu3usbconst.h"

#define FX3_COMPAT_MAJOR            (uint8_t)(8)
#define FX3_COMPAT_MINOR            (uint8_t)(0)

/* GPIO Pins */
#define GPIO_FPGA_RESET             (uint32_t)(26)  // CTL[9]
#define GPIO_DONE                   (uint32_t)(27)
#define GPIO_PROGRAM_B              (uint32_t)(45)
#define GPIO_INIT_B                 (uint32_t)(50)
#define GPIO_AUX_PWR_ON             (uint32_t)(51)
#define GPIO_SHDN_SW                (uint32_t)(52)
#define GPIO_FX3_SCLK               (uint32_t)(53)
#define GPIO_FX3_CE                 (uint32_t)(54)
#define GPIO_FX3_MISO               (uint32_t)(55)
#define GPIO_FX3_MOSI               (uint32_t)(56)
#define GPIO_FPGA_SB_SCL            (uint32_t)(25)  // CTL[8]
#define GPIO_FPGA_SB_SDA            (uint32_t)(23)  // CTL[6]

/* Create the bit-shifts that define the above GPIOs for bitmaps. The bitshifts
 * are relative to 32-bit masks, so shifts > 32 are adjusted accordingly. Note
 * that GPIOs < 32 are configured without the use of masks. */
#define MASK_GPIO_PROGRAM_B         (uint32_t)(1 << (GPIO_PROGRAM_B - 32))
#define MASK_GPIO_INIT_B            (uint32_t)(1 << (GPIO_INIT_B - 32))
#define MASK_GPIO_AUX_PWR_ON        (uint32_t)(1 << (GPIO_FX3_SCLK - 32))
#define MASK_GPIO_SHDN_SW           (uint32_t)(1 << (GPIO_FX3_SCLK - 32))
#define MASK_GPIO_FX3_SCLK          (uint32_t)(1 << (GPIO_FX3_SCLK - 32))
#define MASK_GPIO_FX3_CE            (uint32_t)(1 << (GPIO_FX3_CE - 32))
#define MASK_GPIO_FX3_MISO          (uint32_t)(1 << (GPIO_FX3_MISO - 32))
#define MASK_GPIO_FX3_MOSI          (uint32_t)(1 << (GPIO_FX3_MOSI - 32))
#define MASK_GPIO_FPGA_SB_SCL       (uint32_t)(1 << (GPIO_FPGA_SB_SCL - 0))
#define MASK_GPIO_FPGA_SB_SDA       (uint32_t)(1 << (GPIO_FPGA_SB_SDA - 0))

#define USB3_PACKETS_PER_BURST          (8) // Optimized value from Cypress AN86947
#define USB2_PACKETS_PER_BURST          (1)
#define DMA_SIZE_INFINITE               (0)

#define APP_THREAD_STACK_SIZE           (0x0800)
#define THREAD_PRIORITY                 (8)

#define B200_VREQ_BITSTREAM_START       (uint8_t)(0x02)
#define B200_VREQ_BITSTREAM_DATA        (uint8_t)(0x12)
#define B200_VREQ_BITSTREAM_DATA_FILL   (uint8_t)(0x13)
#define B200_VREQ_BITSTREAM_DATA_COMMIT (uint8_t)(0x14)
#define B200_VREQ_GET_COMPAT            (uint8_t)(0x15)
#define B200_VREQ_SET_FPGA_HASH         (uint8_t)(0x1C)
#define B200_VREQ_GET_FPGA_HASH         (uint8_t)(0x1D)
#define B200_VREQ_SET_FW_HASH           (uint8_t)(0x1E)
#define B200_VREQ_GET_FW_HASH           (uint8_t)(0x1F)
#define B200_VREQ_LOOP_CODE             (uint8_t)(0x22)
#define B200_VREQ_GET_LOG               (uint8_t)(0x23)
#define B200_VREQ_GET_COUNTERS          (uint8_t)(0x24)
#define B200_VREQ_CLEAR_COUNTERS        (uint8_t)(0x25)
#define B200_VREQ_GET_USB_EVENT_LOG     (uint8_t)(0x26)
#define B200_VREQ_SET_CONFIG            (uint8_t)(0x27)
#define B200_VREQ_GET_CONFIG            (uint8_t)(0x28)
#define B200_VREQ_WRITE_SB              (uint8_t)(0x29)
#define B200_VREQ_SET_SB_BAUD_DIV       (uint8_t)(0x30)
#define B200_VREQ_FLUSH_DATA_EPS        (uint8_t)(0x31)
#define B200_VREQ_FPGA_CONFIG           (uint8_t)(0x55)
#define B200_VREQ_TOGGLE_FPGA_RESET     (uint8_t)(0x62)
#define B200_VREQ_TOGGLE_GPIF_RESET     (uint8_t)(0x72)
#define B200_VREQ_GET_USB_SPEED         (uint8_t)(0x80)
#define B200_VREQ_GET_STATUS            (uint8_t)(0x83)
#define B200_VREQ_RESET_DEVICE          (uint8_t)(0x99)
#define B200_VREQ_EEPROM_WRITE          (uint8_t)(0xBA)
#define B200_VREQ_EEPROM_READ           (uint8_t)(0xBB)

#define EVENT_BITSTREAM_START           (1 << 1)
#define EVENT_GPIO_DONE_HIGH            (1 << 2)
#define EVENT_GPIO_INITB_RISE           (1 << 3)
#define EVENT_FPGA_CONFIG               (1 << 4)
#define EVENT_RE_ENUM                   (1 << 5)


/* FX3 States */
#define STATE_UNDEFINED                 (0)
#define STATE_FPGA_READY                (1)
#define STATE_CONFIGURING_FPGA          (2)
#define STATE_BUSY                      (3)
#define STATE_RUNNING                   (4)
#define STATE_UNCONFIGURED              (5)
#define STATE_ERROR                     (6)


/* Define the USB endpoints, sockets, and directions.  The LSB is the socket
 * number, and the MSB is the direction. For USB 2.0, sockets are mapped
 * one-to-one since they must be uni-directional. */
#define VREQ_ENDPOINT_PRODUCER          0x00    // OUT (host -> FX3)
#define VREQ_ENDPOINT_CONSUMER          0x80    // IN  (FX3  -> host)

#define DATA_ENDPOINT_PRODUCER          0x02    // OUT (host -> FX3),  produces for  FPGA
#define DATA_ENDPOINT_CONSUMER          0x86    // IN  (FX3  -> host), consumes from FPGA

#define CTRL_ENDPOINT_PRODUCER          0x04    // OUT (host -> FX3),  produces for  FPGA
#define CTRL_ENDPOINT_CONSUMER          0x88    // IN  (FX3  -> host), consumes from FPGA

#define PRODUCER_DATA_SOCKET            CY_U3P_UIB_SOCKET_PROD_2
#define CONSUMER_DATA_SOCKET            CY_U3P_UIB_SOCKET_CONS_6

#define PRODUCER_CTRL_SOCKET            CY_U3P_UIB_SOCKET_PROD_4
#define CONSUMER_CTRL_SOCKET            CY_U3P_UIB_SOCKET_CONS_8

#define DATA_TX_PPORT_SOCKET            CY_U3P_PIB_SOCKET_0
#define DATA_RX_PPORT_SOCKET            CY_U3P_PIB_SOCKET_1
#define CTRL_COMM_PPORT_SOCKET          CY_U3P_PIB_SOCKET_2
#define CTRL_RESP_PPORT_SOCKET          CY_U3P_PIB_SOCKET_3


/* Descriptor definitions for USB enumerations. */
extern uint8_t b200_usb2_dev_desc[];
extern uint8_t b200_usb3_dev_desc[];
extern const uint8_t b200_dev_qual_desc[];
extern const uint8_t b200_usb_fs_config_desc[];
extern const uint8_t b200_usb_hs_config_desc[];
extern const uint8_t b200_usb_bos_desc[];
extern const uint8_t b200_usb_ss_config_desc[];
extern const uint8_t b200_string_lang_id_desc[];
extern const uint8_t b200_usb_manufacture_desc[];
extern const uint8_t b200_usb_product_desc[];
extern const uint8_t niusrp_usb_manufacture_desc[];
extern const uint8_t niusrp_2900_usb_product_desc[];
extern const uint8_t niusrp_2901_usb_product_desc[];
extern const uint8_t unknown_desc[];
extern uint8_t dev_serial[];


#include "cyu3externcend.h"

#endif /* _B200_MAIN_H */
