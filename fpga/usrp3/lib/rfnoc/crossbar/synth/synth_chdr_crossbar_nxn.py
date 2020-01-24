#! /usr/bin/python3
#!/usr/bin/python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

import argparse
import synth_run

modname = 'chdr_crossbar_nxn'

# Parse command line options
def get_options():
    parser = argparse.ArgumentParser(description='Generate synthesis results for ' + modname)
    parser.add_argument('--opt', type=str, default='AREA', help='Optimization strategies (CSV)')
    parser.add_argument('--ports', type=str, default='8', help='Number of ports (CSV)')
    parser.add_argument('--dataw', type=str, default='64', help='Router datapath width (CSV)')
    parser.add_argument('--mtu', type=str, default='10', help='MTU or Ingress buffer size (CSV)')
    parser.add_argument('--rlutsize', type=str, default='6', help='Router lookup table size (CSV)')
    return parser.parse_args()

def main():
    args = get_options()
    keys = ['opt', 'ports', 'dataw', 'mtu', 'rlutsize']
    for opt in args.opt.strip().split(','):
        for ports in args.ports.strip().split(','):
            for dataw in args.dataw.strip().split(','):
                for mtu in args.mtu.strip().split(','):
                    for rlutsize in args.rlutsize.strip().split(','):
                        # Collect parameters
                        transform = {'opt':opt, 'ports':ports, 'dataw':dataw, 'mtu':mtu, 'rlutsize':rlutsize}
                        synth_run.synth_run(modname, keys, transform)

if __name__ == '__main__':
    main()
