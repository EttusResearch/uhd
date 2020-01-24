#! /usr/bin/python3
#!/usr/bin/python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

import argparse
import synth_run

modname = 'axis_ctrl_crossbar_nxn'

# Parse command line options
def get_options():
    parser = argparse.ArgumentParser(description='Generate synthesis results for ' + modname)
    parser.add_argument('--top', type=str, default='TORUS', help='Topologies (CSV)')
    parser.add_argument('--ports', type=str, default='8', help='Number of ports (CSV)')
    parser.add_argument('--dataw', type=str, default='32', help='Router datapath width (CSV)')
    parser.add_argument('--mtu', type=str, default='5', help='MTU (CSV)')
    parser.add_argument('--ralloc', type=str, default='WORMHOLE', help='Router allocation method (CSV)')
    return parser.parse_args()

def main():
    args = get_options()
    keys = ['top', 'ports', 'dataw', 'mtu', 'ralloc']
    for top in args.top.strip().split(','):
        for ports in args.ports.strip().split(','):
            for dataw in args.dataw.strip().split(','):
                for mtu in args.mtu.strip().split(','):
                    for ralloc in args.ralloc.strip().split(','):
                        # Collect parameters
                        transform = {'ports':ports, 'dataw':dataw, 'mtu':mtu, 'top':top, 'ralloc':ralloc}
                        synth_run.synth_run(modname, keys, transform)

if __name__ == '__main__':
    main()
