#!/usr/bin/env python3
#
# Copyright 2023 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utility to generate license keys for FPGA IP cores (e.g., RFNoC blocks).
"""

import argparse
import base64
import hashlib

DEVICE_TO_KEYLEN = {
    # RFSoCs have 96 bits serial, leaves 312 bits private key
    ('x410', 'x440', 'x4xx'): (12, 39),
    # 7-series have 57 bits DNA, but we round that to 64. That leaves 344 bits
    # for a private key, although if we want to share private keys, we just use
    # 312 bits and pad with zeros.
    ('x310', 'x300', 'x3xx'): (8, 43),
}

def parse_args():
    """ Parse args and return args object """
    parser = argparse.ArgumentParser(
        description="Generate license key",
    )
    parser.add_argument('--serial', help="Device serial")
    parser.add_argument('--private-key', help="Private key")
    parser.add_argument('--fid', help="Feature ID.")
    parser.add_argument('--device-type', help="Device Type",
                        choices=[c for cs in DEVICE_TO_KEYLEN for c in cs])
    return parser.parse_args()


def get_field_widths(device_type):
    """
    Return number of serial and private key bytes.
    """
    for types, lens in DEVICE_TO_KEYLEN.items():
        if device_type in types:
            return lens
    raise ValueError(f"Unknown device type: {device_type}")


def fix_length(string, length):
    """
    Ensure that string is of correct length. Since this is used for base16
    encoded numbers, too-short strings are padded with zeros (on the left).
    If the string is too long, an exception is thrown.
    """
    if len(string) < length:
        return bytes([0,] * (length - len(string))) + string
    if len(string) > length:
        raise ValueError("Input string too long!")
    return string

def read_b16_arg(arg_str):
    """
    Read Base16 string from command line. Will fix erratic cases (b16decode()
    can only handle uppercase) and length (b16decode() will only handle strings
    with an even length).
    """
    arg_str = arg_str.upper()
    if len(arg_str) % 2:
        arg_str = "0" + arg_str
    return base64.b16decode(arg_str)


def main():
    """
    gogogo
    """
    args = parse_args()
    serial = read_b16_arg(args.serial)
    pkey = read_b16_arg(args.private_key)
    fid = read_b16_arg(args.fid)
    serial_w, pkey_w = get_field_widths(args.device_type) # These are bytes, not bits
    serial = fix_length(serial, serial_w)
    pkey = fix_length(pkey, pkey_w)
    fid = fix_length(fid, 4)
    assert len(serial + pkey + fid) <= 55
    key_hash = hashlib.sha256()
    key_hash.update(serial + pkey + fid)
    key = bytes([0x01,]) + fid + base64.b16decode(key_hash.hexdigest().upper())
    key = base64.b32encode(key)[:-4] # Remove ==== padding characters
    key = b'-'.join([key[x:x+5] for x in range(0, len(key), 5)])
    print('Hash:', key_hash.hexdigest().upper())
    print('Key:', key.decode('utf-8'))

if __name__ == "__main__":
    main()
