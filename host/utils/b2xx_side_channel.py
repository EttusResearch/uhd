#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2013-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Tool to read log buffers from the FX3. Use to debug USB connection issues.
Requires PyUSB 1.0.
"""

import sys
import time
import struct
from optparse import OptionParser
import serial

try:
    import usb.core
    import usb.util
except Exception as e:
    print("Failed to import module 'usb'.")
    print("Please make sure you have PyUSB installed and in your PYTHONPATH.")
    print("PyUSB PyPI website: https://pypi.python.org/pypi/pyusb")
    print("To install, download from the website or use 'pip install pyusb'")
    exit(1)

VRT_OUT = usb.util.CTRL_TYPE_VENDOR | usb.util.CTRL_OUT
VRT_IN = usb.util.CTRL_TYPE_VENDOR | usb.util.CTRL_IN

VRQS = {}
B200_VREQ_GET_LOG                 = 0x23
VRQS[B200_VREQ_GET_LOG]           = 'B200_VREQ_GET_LOG'
B200_VREQ_GET_COUNTERS            = 0x24
VRQS[B200_VREQ_GET_COUNTERS]      = 'B200_VREQ_GET_COUNTERS'
B200_VREQ_CLEAR_COUNTERS          = 0x25
VRQS[B200_VREQ_CLEAR_COUNTERS]    = 'B200_VREQ_CLEAR_COUNTERS'
B200_VREQ_GET_USB_EVENT_LOG       = 0x26
VRQS[B200_VREQ_GET_USB_EVENT_LOG] = 'B200_VREQ_GET_USB_EVENT_LOG'
B200_VREQ_SET_CONFIG              = 0x27
VRQS[B200_VREQ_SET_CONFIG]        = 'B200_VREQ_SET_CONFIG'
B200_VREQ_GET_CONFIG              = 0x28
VRQS[B200_VREQ_GET_CONFIG]        = 'B200_VREQ_GET_CONFIG'
B200_VREQ_GET_USB_SPEED           = 0x80
VRQS[B200_VREQ_GET_USB_SPEED]     ='B200_VREQ_GET_USB_SPEED'
B200_VREQ_WRITE_SB                = 0x29
VRQS[B200_VREQ_WRITE_SB]          = 'B200_VREQ_WRITE_SB'
B200_VREQ_SET_SB_BAUD_DIV         = 0x30
VRQS[B200_VREQ_SET_SB_BAUD_DIV]   = 'B200_VREQ_SET_SB_BAUD_DIV'
B200_VREQ_FLUSH_DATA_EPS          = 0x31
VRQS[B200_VREQ_FLUSH_DATA_EPS]    = 'B200_VREQ_FLUSH_DATA_EPS'
B200_VREQ_AD9361_LOOPBACK         = 0x92
VRQS[B200_VREQ_AD9361_LOOPBACK]   = 'B200_VREQ_AD9361_LOOPBACK'

COUNTER_MAGIC                     = 0x10024001
"""
typedef struct Counters {
    int magic;

    DMA_COUNTERS dma_to_host;
    DMA_COUNTERS dma_from_host;

    int log_overrun_count;

    int usb_error_update_count;
    USB_ERROR_COUNTERS usb_error_counters;

    int usb_ep_underrun_count;

    int heap_size;
} COUNTERS, *PCOUNTERS;

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

    int resume_count;
} DMA_COUNTERS, *PDMA_COUNTERS;
"""
DMA_COUNTERS = [
    'XFER_CPLT',
    'SEND_CPLT',
    'RECV_CPLT',
    'PROD_EVENT',
    'CONS_EVENT',
    'ABORTED',
    'ERROR',
    'PROD_SUSP',
    'CONS_SUSP',

    'BUFFER_MARKER',
    'BUFFER_EOP',
    'BUFFER_ERROR',
    'BUFFER_OCCUPIED',

    'last_count',
    'last_size',

    'last_sid',
    'bad_sid_count'
]

USB_ERROR_COUNTERS = [
    'phy_error_count',
    'link_error_count'
]

USB_PHY_ERROR_REGISTERS = [
    'PHY_LOCK_EV',
    'TRAINING_ERROR_EV',
    'RX_ERROR_CRC32_EV',
    'RX_ERROR_CRC16_EV',
    'RX_ERROR_CRC5_EV',
    'PHY_ERROR_DISPARITY_EV',
    'PHY_ERROR_EB_UND_EV',
    'PHY_ERROR_EB_OVR_EV',
    'PHY_ERROR_DECODE_EV'
]

USB_ERROR_COUNTERS += USB_PHY_ERROR_REGISTERS

PIB_COUNTERS = [
    'socket_inactive'
]

COUNTERS = [
    'magic',

    ('dma_to_host', DMA_COUNTERS),
    ('dma_from_host', DMA_COUNTERS),

    'log_overrun_count',

    'usb_error_update_count',
    ('usb_error_counters', USB_ERROR_COUNTERS),

    'usb_ep_underrun_count',

    'heap_size',

    'resume_count',
    'state_transition_count',
    'invalid_gpif_state',
    ('thread_0', PIB_COUNTERS),
    ('thread_1', PIB_COUNTERS),
    ('thread_2', PIB_COUNTERS),
    ('thread_3', PIB_COUNTERS),
]

USB_EVENTS = {}
USB_EVENTS[0x01] = ('CYU3P_USB_LOG_VBUS_OFF'          , 'Indicates VBus turned off.')
USB_EVENTS[0x02] = ('CYU3P_USB_LOG_VBUS_ON'           , 'Indicates VBus turned on.')
USB_EVENTS[0x03] = ('CYU3P_USB_LOG_USB2_PHY_OFF'      , 'Indicates that the 2.0 PHY has been turned off.')
USB_EVENTS[0x04] = ('CYU3P_USB_LOG_USB3_PHY_OFF'      , 'Indicates that the 3.0 PHY has been turned off.')
USB_EVENTS[0x05] = ('CYU3P_USB_LOG_USB2_PHY_ON'       , 'Indicates that the 2.0 PHY has been turned on.')
USB_EVENTS[0x06] = ('CYU3P_USB_LOG_USB3_PHY_ON'       , 'Indicates that the 3.0 PHY has been turned on.')
USB_EVENTS[0x10] = ('CYU3P_USB_LOG_USBSS_DISCONNECT'  , 'Indicates that the USB 3.0 link has been disabled.')
USB_EVENTS[0x11] = ('CYU3P_USB_LOG_USBSS_RESET'       , 'Indicates that a USB 3.0 reset (warm/hot) has happened.')
USB_EVENTS[0x12] = ('CYU3P_USB_LOG_USBSS_CONNECT'     , 'Indicates that USB 3.0 Rx Termination has been detected.')
USB_EVENTS[0x14] = ('CYU3P_USB_LOG_USBSS_CTRL'        , 'Indicates that a USB 3.0 control request has been received.')
USB_EVENTS[0x15] = ('CYU3P_USB_LOG_USBSS_STATUS'      , 'Indicates completion of status stage for a 3.0 control request.')
USB_EVENTS[0x16] = ('CYU3P_USB_LOG_USBSS_ACKSETUP'    , 'Indicates that the CyU3PUsbAckSetup API has been called.')
USB_EVENTS[0x21] = ('CYU3P_USB_LOG_LGO_U1'            , 'Indicates that a LGO_U1 command has been received.')
USB_EVENTS[0x22] = ('CYU3P_USB_LOG_LGO_U2'            , 'Indicates that a LGO_U2 command has been received.')
USB_EVENTS[0x23] = ('CYU3P_USB_LOG_LGO_U3'            , 'Indicates that a LGO_U3 command has been received.')
USB_EVENTS[0x40] = ('CYU3P_USB_LOG_USB2_SUSP'         , 'Indicates that a USB 2.0 suspend condition has been detected.')
USB_EVENTS[0x41] = ('CYU3P_USB_LOG_USB2_RESET'        , 'Indicates that a USB 2.0 bus reset has been detected.')
USB_EVENTS[0x42] = ('CYU3P_USB_LOG_USB2_HSGRANT'      , 'Indicates that the USB High-Speed handshake has been completed.')
USB_EVENTS[0x44] = ('CYU3P_USB_LOG_USB2_CTRL'         , 'Indicates that a FS/HS control request has been received.')
USB_EVENTS[0x45] = ('CYU3P_USB_LOG_USB2_STATUS'       , 'Indicates completion of status stage for a FS/HS control transfer.')
USB_EVENTS[0x50] = ('CYU3P_USB_LOG_USB_FALLBACK'      , 'Indicates that the USB connection is dropping from 3.0 to 2.0')
USB_EVENTS[0x51] = ('CYU3P_USB_LOG_USBSS_ENABLE'      , 'Indicates that a USB 3.0 connection is being attempted again.')
USB_EVENTS[0x52] = ('CYU3P_USB_LOG_USBSS_LNKERR'      , 'The number of link errors has crossed the threshold.')
USB_EVENTS[0x80] = ('CYU3P_USB_LOG_LTSSM_CHG'         , 'Base of values that indicate a USB 3.0 LTSSM state change.')

LTSSM_STATES = {}
LTSSM_STATES[0x00] = ['00000',    "SS.Disabled"]
LTSSM_STATES[0x01] = ['00001',    "Rx.Detect.Reset"]
LTSSM_STATES[0x02] = ['00010',    "Rx.Detect.Active"]
LTSSM_STATES[0x03] = ['00011',    "Rx.Detect.Quiet"]
LTSSM_STATES[0x04] = ['00100',    "SS.Inactive.Quiet"]
LTSSM_STATES[0x05] = ['00101',    "SS.Inactive.Disconnect.Detect"]
LTSSM_STATES[0x06] = ['00110',    "Hot Reset.Active"]
LTSSM_STATES[0x07] = ['00111',    "Hot Reset.Exit"]
LTSSM_STATES[0x08] = ['01000',    "Polling.LFPS"]
LTSSM_STATES[0x09] = ['01001',    "Polling.RxEQ"]
LTSSM_STATES[0x0a] = ['01010',    "Polling.Active"]
LTSSM_STATES[0x0b] = ['01011',    "Polling.Configuration"]
LTSSM_STATES[0x0c] = ['01100',    "Polling.Idle"]
LTSSM_STATES[0x0d] = ['01101',    "(none)"]
#LTSSM_STATES[0x0X] = ['0111X',    "(none)"]
LTSSM_STATES[0x0e] = ['0111X',    "(none)"]
LTSSM_STATES[0x0f] = ['0111X',    "(none)"]
LTSSM_STATES[0x10] = ['10000',    "U0"]
LTSSM_STATES[0x11] = ['10001',    "U1"]
LTSSM_STATES[0x12] = ['10010',    "U2"]
LTSSM_STATES[0x13] = ['10011',    "U3"]
LTSSM_STATES[0x14] = ['10100',    "Loopback.Active"]
LTSSM_STATES[0x15] = ['10101',    "Loopback.Exit"]
LTSSM_STATES[0x16] = ['10110',    "(none)"]
LTSSM_STATES[0x17] = ['10111',    "Compliance"]
LTSSM_STATES[0x18] = ['11000',    "Recovery.Active"]
LTSSM_STATES[0x19] = ['11001',    "Recovery.Configuration"]
LTSSM_STATES[0x1a] = ['11010',    "Recovery.Idle"]
LTSSM_STATES[0x1b] = ['11011',    "(none)"]
#LTSSM_STATES[0x1X] = ['111XX',    "(none)"]
LTSSM_STATES[0x1c] = ['111XX',    "(none)"]
LTSSM_STATES[0x1d] = ['111XX',    "(none)"]
LTSSM_STATES[0x1c] = ['111XX',    "(none)"]
LTSSM_STATES[0x1f] = ['111XX',    "(none)"]
LTSSM_STATES[0x2c] = ['101100',    "Cypress/Intel workaround"]

CF_NONE                 = 0
CF_TX_SWING             = 1 << 0
CF_TX_DEEMPHASIS        = 1 << 1
CF_DISABLE_USB2         = 1 << 2
CF_ENABLE_AS_SUPERSPEED = 1 << 3
CF_PPORT_DRIVE_STRENGTH = 1 << 4
CF_DMA_BUFFER_SIZE      = 1 << 5
CF_DMA_BUFFER_COUNT     = 1 << 6
CF_MANUAL_DMA           = 1 << 7
CF_SB_BAUD_DIV          = 1 << 8

CF_RE_ENUM              = 1 << 31

"""
typedef struct Config {
    int tx_swing;               // [90] [65] 45
    int tx_deemphasis;          // 0x11
    int disable_usb2;           // 0
    int enable_as_superspeed;   // 1
    int pport_drive_strength;   // CY_U3P_DS_THREE_QUARTER_STRENGTH
    int dma_buffer_size;        // [USB3] (max)
    int dma_buffer_count;       // [USB3] 1
    int manual_dma;             // 0
    int sb_baud_div;            // 434*2
} CONFIG, *PCONFIG;

typedef struct ConfigMod {
    int flags;
    CONFIG config;
} CONFIG_MOD, *PCONFIG_MOD;
"""

class Config():
    def __init__(self,
                tx_swing=None, tx_deemphasis=None, disable_usb2=None, enable_as_superspeed=None,
                pport_drive_strength=None,
                dma_buffer_size=None, dma_buffer_count=None, manual_dma=None,
                sb_baud_div=None,
                raw=None):
        self.tx_swing = tx_swing
        self.tx_deemphasis = tx_deemphasis
        self.disable_usb2 = disable_usb2
        self.enable_as_superspeed = enable_as_superspeed
        self.pport_drive_strength = pport_drive_strength
        self.dma_buffer_size = dma_buffer_size
        self.dma_buffer_count = dma_buffer_count
        self.manual_dma = manual_dma
        self.sb_baud_div = sb_baud_div
        self._count = 9

        if raw:
            (self.tx_swing,
            self.tx_deemphasis,
            self.disable_usb2,
            self.enable_as_superspeed,
            self.pport_drive_strength,
            self.dma_buffer_size,
            self.dma_buffer_count,
            self.manual_dma,
            self.sb_baud_div) = struct.unpack("i"*self._count, raw)
    def pack(self):
        return struct.pack("i"*self._count,
            self.tx_swing,
            self.tx_deemphasis,
            self.disable_usb2,
            self.enable_as_superspeed,
            self.pport_drive_strength,
            self.dma_buffer_size,
            self.dma_buffer_count,
            self.manual_dma,
            self.sb_baud_div)
    def __str__(self):
        return self.to_string()
    def to_string(self, flags=-1):
        s = ""
        if flags & CF_TX_SWING:
            s += "tx_swing             = %s\n" % (self.tx_swing)
        if flags & CF_TX_DEEMPHASIS:
            s += "tx_deemphasis        = %s\n" % (self.tx_deemphasis)
        if flags & CF_DISABLE_USB2:
            s += "disable_usb2         = %s\n" % (self.disable_usb2)
        if flags & CF_ENABLE_AS_SUPERSPEED:
            s += "enable_as_superspeed = %s\n" % (self.enable_as_superspeed)
        if flags & CF_PPORT_DRIVE_STRENGTH:
            s += "pport_drive_strength = %s\n" % (self.pport_drive_strength)
        if flags & CF_DMA_BUFFER_SIZE:
            s += "dma_buffer_size      = %s\n" % (self.dma_buffer_size)
        if flags & CF_DMA_BUFFER_COUNT:
            s += "dma_buffer_count     = %s\n" % (self.dma_buffer_count)
        if flags & CF_MANUAL_DMA:
            s += "manual_dma           = %s\n" % (self.manual_dma)
        if flags & CF_SB_BAUD_DIV:
            s += "sb_baud_div          = %s\n" % (self.sb_baud_div)
        return s

def _parse_usb_event_log(data):
    l = []
    for d in data:
        if d == 0x14 or d == 0x15 or d == 0x16:    # CTRL, STATUS, ACKSETUP
            continue
        elif (d & 0x80):
            #l += [(USB_EVENTS[0x80][0] + "+%i" % (d & ~0x80), USB_EVENTS[0x80][1])]
            ltssm_key = (d & ~0x80)
            ltssm_val = "(unknown)"
            if ltssm_key in LTSSM_STATES:
                ltssm_val = LTSSM_STATES[ltssm_key][1]
            ltssm_val = "LTSSM: " + ltssm_val
            l += [(USB_EVENTS[0x80][0] + "+%i" % (d & ~0x80), ltssm_val)]
        elif d in USB_EVENTS:
            l += [USB_EVENTS[d]]
        #else:
        #    l += [("?", "?")]
    return l

class counter_set():
    def __init__(self, counters, name='(top)'):
        self._counters = counters
        self._counter_names = []
        self._name = name
        for c in counters:
            o = 0
            default_value = False
            if isinstance(c, str):
                name = c
                default_value = True
            elif isinstance(c, tuple):
                name = c[0]
                o = counter_set(c[1])
            elif isinstance(c, dict):
                raise Exception('Not implemented yet')
            else:
                raise Exception('Unknown counter format')
            setattr(self, name, o)
            self._counter_names += [(name, default_value)]
        self._fmt_str = self._get_struct_format()

    def _get_struct_format(self):
        fmt_str = ""
        for name, default_value in self._counter_names:
            if default_value:
                fmt_str += "i"
            else:
                o = getattr(self, name)
                fmt_str += o._get_struct_format()
        return fmt_str

    def _update(self, data, parents=[]):
        if len(data) == 0:
            raise Exception('Ran out of data entering %s' % (self._name))
            #return []
        for name, default_value in self._counter_names:
            if default_value:
                if len(data) == 0:
                    raise Exception('Ran out of data setting %s in %s' % (name, self._name))
                setattr(self, name, data[0])
                data = data[1:]
            else:
                o = getattr(self, name)
                data = o._update(data, parents+[self])
        return data

    def update(self, data):
        try:
            vals = struct.unpack(self._fmt_str, data)
            self._update(vals)
        except Exception as e:
            print(("While updating counter set '%s':" % self._name), e)

    def __str__(self):
        return self.to_string()

    def to_string(self, parents=[]):
        s = ""
        cnt = 0
        for name, default_value in self._counter_names:
            o = getattr(self, name)
            if default_value:
                if cnt > 0:
                    s += "\t"
                s += "%s: %05i" % (name, o)
                cnt += 1
            else:
                if cnt > 0:
                    s += "\n"
                s += "\t"*(len(parents) + 1)
                s += o.to_string(parents+[self])
                cnt = 0
        s += "\n"
        return s

class usb_device():
    def __init__(self):
        #self.max_buffer_size = 64        # Default to USB2
        self.max_buffer_size = 1024*4    # Apparently it'll frag bigger packets
        self.counters = counter_set(COUNTERS)
        self.timeout = 2000

    def open(self, idVendor, idProduct):
        print("Finding %04x:%04x..." % (idVendor, idProduct))
        self.dev = usb.core.find(idVendor=idVendor, idProduct=idProduct)
        if self.dev is None:
            raise ValueError('Device not found: %04x:%04x' % (idVendor, idProduct))

        self.log_index = 0
        self.log_read_count = 0
        self.usb_event_log_read_count = 0
        self.counters_read_count = 0

        #if self.dev.is_kernel_driver_active(0):
        #    print "Detaching kernel driver..."
        #    self.dev.detach_kernel_driver(0)

        #self.dev.set_configuration()    # This will throw as device is already claimed

        print("Opened %04x:%04x" % (idVendor, idProduct))

        #self.dev.ctrl_transfer(0x21,          0x09,     0,        0,        [0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00] )
        #self.dev.ctrl_transfer(bmRequestType, bRequest, wValue=0, wIndex=0, data_or_wLength = None,                   timeout = None

        #res = self.dev.ctrl_transfer(VRT_IN, 0x83, 0, 0, 1024)
        # Can give 1024 byte size for IN, result will be actual payload size
        # Invalid VREQ results in usb.core.USBError 32 Pipe error
        #print res

        #res = self.dev.ctrl_transfer(VRT_IN, B200_VREQ_GET_USB_SPEED, 0, 0, 1)
        #self.usb_speed = res[0]
        while True:
            #l = self.vrt_get(B200_VREQ_GET_USB_SPEED)
            l = []
            try:
                l = self.dev.ctrl_transfer(VRT_IN, B200_VREQ_GET_USB_SPEED, 0, 0, 1)
            except usb.core.USBError as e:
                if e.errno == 32:
                    print(e)
                    print("Is the firmware loaded?")
                    sys.exit(0)
            if len(l) > 0:
                self.usb_speed = l[0]
                print("Operating at USB", self.usb_speed)
                break
            else:
                print("Retrying...")
        #if self.usb_speed == 3:
        #    self.max_buffer_size = 512
        print("Max buffer size:", self.max_buffer_size)
        print()

    def _handle_error(self, e, vrt):
        if e.errno == 19:    # No such device
            raise e
        vrt_str = "0x%02x" % (vrt)
        if vrt in VRQS:
            vrt_str += " (%s)" % (VRQS[vrt])
        print("%s: %s" % (vrt_str, str(e)))

    def vrt_get(self, vrt):
        try:
            return self.dev.ctrl_transfer(VRT_IN, vrt, 0, 0, self.max_buffer_size, self.timeout)
        except usb.core.USBError as e:
            self._handle_error(e, vrt)
        return []

    def vrt_set(self, vrt, data=""):
        try:
            return self.dev.ctrl_transfer(VRT_OUT, vrt, 0, 0, data, self.timeout)
        except usb.core.USBError as e:
            self._handle_error(e, vrt)
        return None

    def get_log(self, with_log_index=True):
        lines = []
        raw = self.vrt_get(B200_VREQ_GET_LOG)
        if len(raw) == 0:
            return lines
        if raw[0] == 0:
            return lines
        self.log_read_count += 1
        raw = list(raw)
        last = 0
        while raw[last] != 0:
            try:
                try:
                    idx = raw.index(0, last)
                except ValueError as e:
                    print("No null termination in log buffer (length: %d, last null: %d)" % (len(raw), last))
                    break
                self.log_index += 1
                line = "".join(map(chr, raw[last:idx]))
                #print "[%05i %05i] %s" % (self.log_index, self.log_read_count, line)
                if with_log_index:
                    lines += [(self.log_index, line)]
                else:
                    lines += [line]
                last = idx + 1
                if last >= len(raw):
                    break
            except Exception as e:
                print("Exception while parsing log buffer:", e)
                break
        return lines

    def print_log(self):
        lines = self.get_log()
        if len(lines) == 0:
            return
        for l in lines:
            #print l
            print("[%05i %05i] %s" % (l[0], self.log_read_count, l[1]))
        print()

    def get_counters(self):
        data = self.vrt_get(B200_VREQ_GET_COUNTERS)
        if len(data) == 0:
            return
        self.counters_read_count += 1
        self.counters.update(data)

    def print_counters(self):
        self.get_counters()
        print("[%05i]" % (self.counters_read_count))
        print(self.counters)

    def get_usb_event_log(self):
        data = self.vrt_get(B200_VREQ_GET_USB_EVENT_LOG)
        if len(data) == 0:
            return []
        if len(data) == self.max_buffer_size:    # ZLP when no new events have been recorded
            return []
        if len(data) > 64:
            raise Exception("USB event log data len = %i" % (len(data)))
        self.usb_event_log_read_count += 1
        return _parse_usb_event_log(data)

    def print_usb_event_log(self):
        l = self.get_usb_event_log()
        if len(l) == 0:
            return
        print("\n".join([("[%05i] " % (self.usb_event_log_read_count)) + x[0] + ":\t" + x[1] for x in l]))
        print()

def run_log(dev, options):
    items = [
        (options.log,            dev.print_log),
        (options.counters,        dev.print_counters),
        (options.usb_events,    dev.print_usb_event_log)
    ]
    items = [x for x in items if x[0] > 0]
    smallest_interval = min([x[0] for x in items])
    time_now = time.time()
    last = [time_now]*len(items)

    try:
        for i in items:
            if i[0] < 0:
                i[1]()
        while True:
            time_now = time.time()
            cleared = False
            for i in range(len(items)):
                time_last = last[i]
                if time_now < (time_last + items[i][0]):
                    continue
                if options.clear_screen and not cleared:
                    print(chr(27) + "[2J")
                    cleared = True
                #print items[i][1]
                items[i][1]()
                last[i] = time.time()
            time.sleep(smallest_interval)
    except KeyboardInterrupt:
        return

def hex_to_int(s):
    radix = 10
    s = s.lower()
    if (len(s) > 1 and s[0] == 'x') or (len(s) > 2 and s[0:2] == "0x"):
        radix = 16
    return int(s, radix)

def recv_serial_data(ser):
    data = ""
    usb_event_log_read_count = 0
    time_start = time.time()
    while True:
        c = ser.read()
        data += c
        #if c == '\n':
        if len(data) >= 2 and data[-2:] == "\r\n":
            time_now_str = "[%06d]" % (int(time.time() - time_start))
            data = data[0:-2]
            if data == "":
                #print "[Received an empty line]"
                print()
            elif data[0] == ' ':
                print(time_now_str, data[1:])
            elif data[0] == 'U':
                data = data[1:]
                cur_type = 0
                i = 0
                usb_events = []
                while len(data) > 0:
                    c = data[0]

                    if cur_type == 0:
                        if c == 'a':
                            cur_type = 1
                        elif (c >= 'A') and (c <= 'P'):
                            i = ord(c) - ord('A')
                            cur_type = 2
                        else:
                            print(time_now_str, "[Unknown type: '%s' (0x%02x) in '%s']" % (c, ord(c), data))

                    elif cur_type == 1:
                        i = ord(c) - ord('a')
                        if (i < 0) or (i >= len(USB_PHY_ERROR_REGISTERS)):
                            print(time_now_str, "[Unknown PHY error register index: '%s' (0x%02x) (%d) in '%s']" % (c, ord(c), i, data))
                        else:
                            print(time_now_str, USB_PHY_ERROR_REGISTERS[i])
                        cur_type = 0

                    elif cur_type == 2:
                        i2 = ord(c) - ord('A')
                        if (c < 'A') or (c > 'P'):
                            print(time_now_str, "[Unknown second USB event part: '%s' (0x%02x) (%d) in '%s']" % (c, ord(c), i2, data))
                        else:
                            i = (i << 4) | i2
                            usb_events += [i]

                        cur_type = 0

                    data = data[1:]

                if len(usb_events) > 0:
                    usb_event_log_read_count += 1
                    l = _parse_usb_event_log(usb_events)
                    print("\n".join([time_now_str + ("[%05i] " % (usb_event_log_read_count)) + x[0] + ":\t" + x[1] for x in l]))
                    #print

            data = ""

def main():
    parser = OptionParser(usage="%prog: [options]")    #option_class=eng_option,

    parser.add_option("-v", "--vid", type="string", default="0x2500", help="VID [default=%default]")
    parser.add_option("-p", "--pid", type="string", default="0x0020", help="PID [default=%default]")
    parser.add_option("-t", "--tty", type="string", default=None, help="TTY [default=%default]")
    parser.add_option("-c", "--cmd", type="string", default="", help="Command (empty opens prompt)")
    parser.add_option("-n", "--counters", type="float", default="5.0", help="Counter print interval [default=%default]")
    parser.add_option("-l", "--log", type="float", default="0.25", help="Log print interval [default=%default]")
    parser.add_option("-e", "--usb-events", type="float", default="0.25", help="USB event log print interval [default=%default]")
    parser.add_option("-s", "--sb", type="string", default=None, help="Settings Bus write message [default=%default]")
    parser.add_option("-d", "--sb-baud-div", type="int", default=None, help="Settings Bus baud rate divisor [default=%default]")
    parser.add_option("-b", "--sb-baud", type="int", default=None, help="Settings Bus baud rate [default=%default]")
    parser.add_option("-r", "--clear-screen", action="store_true", default=False, help="Clear screen [default=%default]")
    parser.add_option("-R", "--reset-counters", action="store_true", default=False, help="Reset counters [default=%default]")
    parser.add_option("-f", "--flush-data-eps", action="store_true", default=False, help="Flush data endpoints [default=%default]")
    parser.add_option("-L", "--fe-loopback", type="int", default=None, help="Change AD9361 digital loopback [default=%default]")

    (options, args) = parser.parse_args()

    if options.tty is not None and options.tty != "":
        while True:
            try:
                ser = serial.Serial(port=options.tty, baudrate=115200, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS, timeout=None)    # timeout: None (blocking), 0 (non-blocking)
                print("Opened", options.tty)

                try:
                    recv_serial_data(ser)
                except KeyboardInterrupt:
                    break
            except Exception as e:
                print("Unable to open serial port:", e)
            break
    else:
        dev = usb_device()

        while True:
            try:
                dev.open(idVendor=hex_to_int(options.vid), idProduct=hex_to_int(options.pid))

                raw_config = dev.vrt_get(B200_VREQ_GET_CONFIG)
                current_config = Config(raw=raw_config)
                print("Current config:")
                print(current_config)

                if options.flush_data_eps:
                    dev.vrt_set(B200_VREQ_FLUSH_DATA_EPS)
                if options.sb_baud_div is not None:
                    dev.vrt_set(B200_VREQ_SET_SB_BAUD_DIV, struct.pack('H', options.sb_baud_div))
                if options.sb is not None and len(options.sb) > 0:
                    dev.vrt_set(B200_VREQ_WRITE_SB, " " + options.sb)
                if options.reset_counters:
                    dev.vrt_set(B200_VREQ_CLEAR_COUNTERS)
                if options.fe_loopback is not None:
                    dev.vrt_set(B200_VREQ_AD9361_LOOPBACK, struct.pack('B', int(options.fe_loopback)))
                options.cmd = options.cmd.strip()
                if len(options.cmd) == 0:
                    run_log(dev, options)
                else:
                    cmds = options.cmd.split(',')
                    flags = 0
                    for cmd in cmds:
                        cmd = cmd.strip()
                        if len(cmd) == 0:
                            continue
                        parts = cmd.split(' ')
                        action = parts[0].lower()
                        try:
                            if action == "txswing":
                                current_config.tx_swing = int(parts[1])
                                flags |= CF_TX_SWING
                            elif action == "txdeemph":
                                current_config.tx_deemphasis = int(parts[1])
                                flags |= CF_TX_DEEMPHASIS
                            elif action == "ss":
                                current_config.enable_as_superspeed = int(parts[1])
                                flags |= CF_ENABLE_AS_SUPERSPEED
                            elif action == "disableusb2":
                                current_config.disable_usb2 = int(parts[1])
                                flags |= CF_DISABLE_USB2
                            elif action == "pportdrive":
                                current_config.pport_drive_strength = int(parts[1])
                                flags |= CF_PPORT_DRIVE_STRENGTH
                            elif action == "dmasize":
                                current_config.dma_buffer_size = int(parts[1])
                                flags |= CF_DMA_BUFFER_SIZE
                            elif action == "dmacount":
                                current_config.dma_buffer_count = int(parts[1])
                                flags |= CF_DMA_BUFFER_COUNT
                            elif action == "manualdma":
                                current_config.manual_dma = int(parts[1])
                                flags |= CF_MANUAL_DMA
                            elif action == "sbbauddiv":
                                current_config.sb_baud_div = int(parts[1])
                                flags |= CF_SB_BAUD_DIV
                            elif action == "reenum":
                                flags |= CF_RE_ENUM
                            else:
                                print("'%s' not implemented" % (action))
                        except Exception as e:
                            print("Exception while handling action '%s'" % (action), e)
                    if flags != 0:
                        print("New config to be set:")
                        print(current_config.to_string(flags))
                        #print current_config
                        #print "Update flags: 0x%x" % (flags)
                        new_config = struct.pack("I", flags) + current_config.pack()
                        dev.vrt_set(B200_VREQ_SET_CONFIG, new_config)
                    else:
                        print("Not updating config")
                break
            except usb.core.USBError as e:
                if e.errno == 19:    # No such device
                    pass
                print(e)
                break

    return 0

if __name__ == '__main__':
    main()
