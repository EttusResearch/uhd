#!/usr/bin/env python3

import argparse
import sys

# Parse command line options
def get_options():
    parser = argparse.ArgumentParser(description='Utility script to generate a properly formed partid for Xilinx projects')
    parser.add_argument('target', type=str, default=None, help='Input value for target. Must be of the form <arch>/<device>/<package>/<speedgrade>[/<temperaturegrade>[/<silicon revision>]]')
    args = parser.parse_args()
    if not args.target:
        print('ERROR: Please specify a target device tuple\n')
        parser.print_help()
        sys.exit(1)
    return args

def main():
    args = get_options();

    target_tok = args.target.split('/')
    if len(target_tok) < 4:
        print('ERROR: Invalid target format. Must be <arch>/<device>/<package>/<speedgrade>[/<temperaturegrade>[/<silicon_revision>]]')
        print('ERROR: Parsed only ' + str(len(target_tok)) + ' tokens')
        sys.exit(1)
    if target_tok[0] in ['artix7', 'kintex7', 'zynq', 'spartan7', 'virtex7']:
        print('' + target_tok[1] + target_tok[2] + target_tok[3])
    elif target_tok[0] in ['zynquplus', 'zynquplusRFSOC']:
        if len(target_tok) > 5:
            print('' + target_tok[1] + '-' + target_tok[2] + target_tok[3] + '-' + target_tok[4] + '-' + target_tok[5])
        else:
            print('' + target_tok[1] + '-' + target_tok[2] + target_tok[3] + '-' + target_tok[4])
    else:
        print('unknown-part-error')

if __name__ == '__main__':
    main()
