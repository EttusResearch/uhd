#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2013-2014 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
"""
Plots the output of the streamers that is produced when DEBUG_TXRX
is enabled.
"""

import argparse
import matplotlib.pyplot as plt
import numpy as np

# This is a top level function to load a debug file. It will return a list of lists with all the data.
def get_data(filename):
    res = []
    with open(filename) as f:
        for line in f.readlines():
            line = line.rstrip('\n')
            s = line.split(',')
            res.append(s)
    res = fix_known_data_corruption_patterns(res)
    return res

# Sometimes 'O' etc. get printed to stderr. This disturbs further processing. Thus, known pattern will be removed here.
def fix_known_data_corruption_patterns(data):
    # Some lines might be corrupted in the way that e.g. UHD prints sth else in the same line.
    # O, D, Press Enter... are known patterns. These should be fixed.
    counts = {'O': 0, 'D': 0, 'exit_msg': 0}
    for i in range(len(data)):
        if data[i][0].startswith('O'):
            counts['O'] += 1
            data[i][0] = data[i][0].replace('O', '')
        if data[i][0].startswith('D'):
            counts['D'] += 1
            data[i][0] = data[i][0].replace('D', '')
        if data[i][0].startswith('Press Enter to quit: '):
            counts['exit_msg'] += 1
            data[i][0] = data[i][0].replace('Press Enter to quit: ', '')
    print counts
    return data

# Extract lines with a certain prefix.
def extract_lines(prefix, data):
    res = []
    for line in data:
        if line[0] == prefix:
            res.append(line[1:])
    return res

# boolean values are stored as true/false. Convert them to real booleans again.
def convert_to_bool(data):
    res = []
    for d in data:
        res.append(d == "true")
    return res

# The different outputs have different structures. With this function you can conert them back to their actual type.
def convert_data_set(data, reqtype):
    zdata = zip(*data)
    res = []
    if len(zdata) == 0:
        return res
    for i in range(len(reqtype)):
        if reqtype[i] == np.bool:
            res.append(np.asarray(convert_to_bool(zdata[i]), dtype=np.bool))
        else:
            res.append(np.asarray(zdata[i], dtype=reqtype[i]))
    return res

# Wrapper for super_send_packet_handler data to be used with convert_data_set
def convert_super_send_packet_handler_data(data):
    # wallclock, timeout, avail_samps, sent_samps, sob, eob, has_time_spec, time_spec (ticks)
    reqtype = [np.uint64, np.float, np.int, np.int, np.bool, np.bool, np.bool, np.uint64]
    cdata = convert_data_set(data, reqtype)
    return cdata

# same o' same o'
def convert_super_recv_packet_handler_data(data):
    # wallclock, timeout, requested_samps, received_samps, one_packet, error_code, sob, eob, more_fragments, fragment_offset, has_timespec, time_spec
    reqtype = [np.uint64, np.float, np.int, np.int, np.bool, np.int, np.bool, np.bool, np.bool, np.int, np.bool, np.uint64]
    cdata = convert_data_set(data, reqtype)
    return cdata

def extract_super_recv_packet_handler_data(data):
    pref1 = "super_recv_packet_handler"
    pref2 = "recv"
    super_recv = extract_lines(pref1, data)
    recv = extract_lines(pref2, super_recv)
    recv = convert_super_recv_packet_handler_data(recv)
    return recv

# Sometimes TX or RX is interrupted by system jiffies. Those are found by this function.
def find_jiffy(data, thr):
    res = []
    last = data[0]
    for i in range(len(data)):
        if data[i] - last > thr:
            res.append([last, data[i]])
        last = data[i]
    return res


# Get difference between tx and rx wallclock
def get_diff(tx, rx):
    print "get diff, len(rx) = ", len(rx)
    diff = [0] * len(rx[0])
    for i in range(len(diff)):
        r = rx[3][i]
        idx = rx[0][i] - 1 # call count starts at 1. idx is 0 based.
        t = tx[3][idx]
        diff[i] = t - r
    return diff


def bps(samps, time):
    bp = []
    last = time[0]-1000
    for i in range(len(samps)):
        td = time[i] - last
        last = time[i]
        td = td /1e6
        bp.append(samps[i] * 4 / td)
    return bp


# same as the other wrappers this time for libusb1
def extract_libusb(trx, data):
    pr1 = "libusb1_zero_copy"
    pr2 = "libusb_async_cb"
    ldata = extract_lines(pr1, data)
    edata = extract_lines(pr2, ldata)
    d = extract_lines(trx, edata)
    # buff_num, actual_length, status, end_time, start_time
    reqtype = [np.int, np.int, np.int, np.uint64, np.uint64]
    return convert_data_set(d, reqtype)


# Extract data for stream buffers. Typically there are 16 TX and 16 RX buffers. And there numbers are static. Though the number of buffers might be changed and the constant parameters must be adjusted in this case.
def extract_txrx(data):
    tx = [[], [], [], [], []]
    rx = [[], [], [], [], []]
    for i in range(len(data[0])):
        print data[0][i]
        if data[0][i] > 31 and data[0][i] < 48:
            rx[0].append(data[0][i])
            rx[1].append(data[1][i])
            rx[2].append(data[2][i])
            rx[3].append(data[3][i])
            rx[4].append(data[4][i])
            #print "tx\t", data[0][i], "\t", data[3][i], "\t", data[4][i]
        if data[0][i] > 47 and data[0][i] < 64:
            tx[0].append(data[0][i])
            tx[1].append(data[1][i])
            tx[2].append(data[2][i])
            tx[3].append(data[3][i])
            tx[4].append(data[4][i])
            #print "rx\t", data[0][i], "\t", data[3][i], "\t", data[4][i]
    return [tx, rx]

# Calculate momentary throughput
def throughput(data):
    start = data[2][0]
    total = data[1][-1] - data[2][0]
    print total
    thr = np.zeros(total)

    for i in range(len(data[0])):
        s = data[2][i] - start
        f = data[1][i] - start
        ticks = data[1][i] - data[2][i]
        pertick = 1. * data[0][i] / ticks
        vals = [pertick] * ticks
        thr[s:f] = np.add(thr[s:f], vals)
        #print pertick
    print np.shape(thr)
    return thr


# Calculate a moving average
def ma(data, wl):
    ap = np.zeros(wl)
    data = np.concatenate((ap, data, ap))
    print np.shape(data)
    res = np.zeros(len(data)-wl)
    for i in range(len(data)-wl):
        av = np.sum(data[i:i+wl]) / wl
        res[i] = av
        print i, "\t", av
    return res


def get_x_axis(stamps):
    scale = 10e-6
    return np.multiply(stamps, scale)


# plot status codes.
def plot_status_codes_over_time(data, fignum):
    print "printing status numbers over time"

    # extract and convert the data
    recv = extract_super_recv_packet_handler_data(data)
    libusb_rx = extract_libusb("rx", data)
    libusb_tx = extract_libusb("tx", data)

    # Plot all data
    plt.figure(fignum) # Make sure these plots are printed to a new figure.
    #plt.plot(get_x_axis(libusb_rx[3]), libusb_rx[2], marker='x', label='RX')
    #plt.plot(get_x_axis(libusb_tx[3]), libusb_tx[2], marker='x', label='TX')

    pos = 5
    recv_error_codes = recv[pos]
    plt.plot(get_x_axis(recv[0]), recv_error_codes, marker='x', label='recv')
    plt.title("Status codes over timestamps")
    plt.ylabel("status codes")
    plt.xlabel("timestamps")
    plt.grid()
    plt.legend()

    for i in range(len(recv[0])):
        if recv[pos][i] == 8:
            xaxis = get_x_axis([recv[0][i]])
            plt.axvline(xaxis, color='b')


    # Get some statistics and print them too
    codes = []
    code_dict = {}
    for i in range(len(recv_error_codes)):
        if recv_error_codes[i] != 0:
            code = rx_metadata_error_codes[recv_error_codes[i]]
            pair = [i, code]
            codes.append(pair)
            if not code_dict.has_key(code):
                code_dict[code] = 0
            code_dict[code] += 1
    print codes
    print code_dict


# plot rtt times as peaks. That's the fast and easy way.
def plot_rtt_times(data, fignum):
    print "plot RTT times"
    rx = extract_libusb("rx", data)
    tx = extract_libusb("tx", data)

    scale = 10e-6
    rx_diff = np.multiply(np.subtract(rx[3], rx[4]), scale)
    tx_diff = np.multiply(np.subtract(tx[3], tx[4]), scale)

    plt.figure(fignum)
    plt.plot(get_x_axis(rx[3]), rx_diff, marker='x', ls='', label="rx RTT")
    plt.plot(get_x_axis(tx[3]), tx_diff, marker='x', ls='', label="tx RTT")
    plt.title("Round trip times")
    plt.ylabel("RTT (us)")
    plt.grid()
    plt.legend()


# plot RTT as actual lines as long as buffers are on the fly.
# This can take a long time if the function has to print a lot of small lines. Careful with this!
def plot_rtt_lines(data, fignum):
    print "plot RTT lines"
    rx = extract_libusb("rx", data)
    #tx = extract_libusb("tx", data)

    if len(data) == 0 or len(rx) == 0:
        return

    plt.figure(fignum)
    for i in range(len(rx[0])):
        if rx[0][i] > -1:
            start = rx[4][i]
            stop = rx[3][i]
            status = rx[2][i]

            val = rx[0][i]#(stop - start) * scale
            if not status == 0:
                if status == 2:
                    print "status = ", status
                val +=0.5
            xaxis = get_x_axis([start, stop])
            plt.plot(xaxis, [val, val], marker='x')
    plt.ylabel('buffer number')

    # Careful with these lines here.
    # Basically they should always have these values to separate CTRL, TX, RX buffer number blocks.
    # But these values can be adjusted. Thus, it requires you to adjust these lines
    plt.axhline(15.5)
    plt.axhline(31.5)
    plt.axhline(47.5)
    plt.grid()


# only plot on-the-fly buffers.
def plot_buff_otf(trx, nrange, data):
    d = extract_libusb(trx, data)
    res = [[], []]
    num = 0
    for i in range(len(d[0])):
        if d[0][i] in nrange:
            res


# If there are still unknown lines after cleanup, they can be caught and printed here.
# This way you can check what got caught but shouldn't have been caught.
def get_unknown_lines(data):
    # These are the 3 known starting lines. More might be added in the future.
    known = ['super_recv_packet_handler', 'super_send_packet_handler', 'libusb1_zero_copy']
    res = []
    for i in range(len(data)):
        if not data[i][0] in known:
            print data[i]
            res.append(data[i])
    return res

# LUT for all the return codes
rx_metadata_error_codes = {0x0: "NONE", 0x1: "TIMEOUT", 0x2: "LATE_COMMAND", 0x4: "BROKEN_CHAIN", 0x8: "OVERFLOW",
                           0xc: "ALIGNMENT", 0xf: "BAD_PACKET"}


def parse_args():
    """ Parse args, yo """
    parser = argparse.ArgumentParser(description='Plot tool for debug prints.')
    parser.add_argument('-f', '--filename', default=None, help='Debug output file')
    return parser.parse_args()

# Be really careful with the input files. They get pretty huge in a small time.
# Doing some of the plotting can eat up a lot of time then.
# Although this file contains a lot of functions by now, it is still left to the user to use everythin correctly.
def main():
    args = parse_args()
    filename = args.filename
    print "get data from: ", filename
    #pref1 = "super_recv_packet_handler"
    #pref2 = "recv"

    # Here you get the data from a file
    data = get_data(filename)
    #print "data len: ", len(data)

    # extract lines with unknown content.
    unknown = get_unknown_lines(data)

    # plot status codes and RTT lines.
    plot_status_codes_over_time(data, 0)
    plot_rtt_lines(data, 0)
    #plot_rtt_times(data, 0)

    #[tx, rx] = extract_txrx(data)
    #print "txlen\t", len(tx[0])
    #print "rxlen\t", len(rx[0])


    #print "plot data"
    #plt.title(filename)
    #plt.plot(tx_data[0], tx_data[7], 'r', marker='o', label='TX')
    #plt.plot(rx_data[0], rx_data[11], 'g', marker='x', label='RX')
    #for j in jiffies:
    #    plt.axvline(x=j[0], color='r')
    #    plt.axvline(x=j[1], color='g')
    #
    #plt.xlabel('wallclock (us)')
    #plt.ylabel('timestamp (ticks)')
    #plt.legend()

    #lencal = 200000

    # calculate and plot rx throughput
    #rxthr = throughput([rx[1], rx[3], rx[4]])
    #plt.plot(rxthr[0:lencal])
    #rxav = ma(rxthr[0:lencal], 2000)
    #plt.plot(rxav)

    # calculate and plot tx throughput
    #txthr = throughput([tx[1], tx[3], tx[4]])
    #plt.plot(txthr[0:lencal])
    #txav = ma(txthr[0:lencal], 20)
    #plt.plot(txav)


    #bp = bps(data[1], data[2])
    #print np.sum(bp)
    #ave = np.sum(bp)/len(bp)
    #print ave
    #plt.plot(tx_data[0], tx_data[2], 'r')
    #plt.plot(rx_data[0], rx_data[2], 'b')
    #plt.plot(bp, 'g', marker='+')
    #plt.plot(np.multiply(data[1], 1e6), 'r', marker='o')
    #plt.plot(np.multiply(data[3],1e2), 'b', marker='x')
    #plt.plot(ave)

    # in the end, put a grid on the graph and show it.
    plt.grid()
    plt.show()

if __name__ == '__main__':
    print "[WARNING] This tool is in alpha status. Only use if you know what you're doing!"
    main()

