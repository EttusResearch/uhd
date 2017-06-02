#!/usr/bin/env python

import argparse
from usrptest import parsers
from usrptest.flowgraphs import selftest_fg


def main():
    parser = argparse.ArgumentParser()
    parser = parsers.add_core_args(parser)
    parser = parsers.add_selftest_args(parser)
    args = parser.parse_args()
    my_flowgraph = selftest_fg.selftest_fg(args.frequency, args.samp_rate, args.dphase ,args.devices)
    results = my_flowgraph.run()
    print(results)
if __name__ == '__main__':
    main()
