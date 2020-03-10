#!/usr/bin/env python3
"""
Copyright 2020 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

convert_cal_data: A utility to convert CSV-based calibration data into binary
cal data.
"""

import os
import glob
import argparse
import uhd

def csv_to_iq_cal(filename):
    """
    Opens the file "filename", parses the CSV data, and returns a
    uhd.usrp.cal.IQCal class.
    """
    def parse_metadata(handle):
        """
        Return name, serial, and timestamp, and verify version.
        """
        num_lines_md = 4
        metadata = {}
        for _ in range(num_lines_md):
            key, val = [x.strip() for x in handle.readline().split(",", 1)]
            if key == "version":
                if val != "0, 1":
                    raise RuntimeError("Unexpected version: " + val)
            elif key == "timestamp":
                metadata[key] = int(val)
            else:
                metadata[key] = val
        if handle.readline().strip() != "DATA STARTS HERE":
            raise RuntimeError("Unexpected header!")
        handle.readline() # Flush header
        return metadata
    with open(filename, 'r') as handle:
        metadata = parse_metadata(handle)
        print("Found metadata: ")
        for key, val in metadata.items():
            print("* {k}: {v}".format(k=key, v=val))
        iq_cal = uhd.usrp.cal.IQCal(
            metadata['name'], metadata['serial'], metadata['timestamp'])
        for line in handle.readlines():
            lo_freq, corr_real, corr_imag, supp_abs, supp_delta = \
                line.split(",")
            iq_cal.set_cal_coeff(
                float(lo_freq),
                complex(float(corr_real.strip()), float(corr_imag.strip())),
                float(supp_abs.strip()),
                float(supp_delta.strip()),
            )
    return iq_cal

def get_csv_location():
    """
    Returns the path to the location of the CSV files
    """
    return os.path.expandvars("$HOME/.uhd/cal")

def get_all_files(csv_location=None):
    """
    Returns a list of all CSV files in the standard location
    """
    csv_path = csv_location or get_csv_location()
    return glob.glob(os.path.join(csv_path, "*.csv"))

def parse_args():
    """ Parse args and return args object """
    parser = argparse.ArgumentParser(
        description="Convert CSV-Based cal data to binary format",
    )
    parser.add_argument(
        'files',
        help="List files to convert. If empty, will attempt to auto-detect "
             "files from the standard calibration location ($HOME/.uhd/cal)",
        nargs="*",
    )
    return parser.parse_args()


def main():
    """Go, go, go!"""
    args = parse_args()
    file_list = args.files or get_all_files()
    for filename in file_list:
        print("Converting {}...".format(filename))
        print("Identifying cal data type...")
        key = os.path.basename(filename).split('_cal')[0]
        if key not in ('rx_iq', 'tx_iq', 'tx_dc'):
            print("Unidentified cal data type: {}".format(key))
            continue
        iq_cal = csv_to_iq_cal(filename)
        uhd.usrp.cal.database.write_cal_data(
            key,
            iq_cal.get_serial(),
            iq_cal.serialize())
    return 0

if __name__ == "__main__":
    exit(main())
