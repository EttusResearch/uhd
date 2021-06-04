#!/usr/bin/env python3
#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import uhd
import json
import pathlib

def convert(name):
    print("Convert {} ... ".format(name), end='')
    with open(name) as jsonfile:
        data = json.load(jsonfile)
        num_steps = len(data['freq_gain_dsa_map'][0]['gains'][0]['steps'])
        metadata = data['metadata']
        if num_steps == 2:
            cal = uhd.usrp.cal.ZbxTxDsaCal(metadata['name'], metadata.get('serial', ''), metadata['timestamp'])
        elif num_steps == 3:
            cal = uhd.usrp.cal.ZbxRxDsaCal(metadata['name'], metadata.get('serial', ''), metadata['timestamp'])
        else:
            raise ValueError("Invalid number of steps {}".format(num_steps))
        for band in data["freq_gain_dsa_map"]:
            print(band['name'], end=',')
            gain_indizes = [i['steps'] for i in band['gains']]
            print(len(gain_indizes), end=' ')
            cal.add_frequency_band(band['max_freq'], band['name'], gain_indizes)

        rc_file = pathlib.Path(name).stem + ".cal"
        with open(rc_file, "wb") as f:
            f.write(cal.serialize())
    print('Done')


if __name__ == "__main__":
    for filename in pathlib.Path.cwd().glob('*.json'):
        convert(filename)
