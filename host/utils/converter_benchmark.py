#!/usr/bin/env python
#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Wrap the converter_benchmark tool and produce prettier results.
"""

from __future__ import print_function
import argparse
import csv
import subprocess

INTRO_SETUP = {
    'n_samples': {
        'title': 'Samples per iteration',
    },
    'iterations': {
        'title': 'Number of iterations'
    },
}

TABLE_SETUP = {
    'prio': {
        'title': 'Priority',
    },
    'duration_ms': {
        'title': 'Total Duration (ms)',
    },
    'avg_duration_ms': {
        'title': 'Avg. Duration (ms)',
    },
}

def run_benchmark(args):
    """ Run the tool with the given arguments, return the section in the {{{ }}} brackets """
    call_args = ['./converter_benchmark',]
    for k, v in args.__dict__.iteritems():
        k = k.replace('_', '-')
        if v is None:
            continue
        if k in ('debug-converter', 'hex'):
            if v:
                call_args.append('--{0}'.format(k))
            continue
        call_args.append('--{0}'.format(k))
        call_args.append(str(v))
    print(call_args)
    try:
        output = subprocess.check_output(call_args)
    except subprocess.CalledProcessError as ex:
        print(ex.output)
        exit(ex.returncode)
    header_out, csv_output = output.split('{{{', 1)
    csv_output = csv_output.split('}}}', 1)
    assert len(csv_output) == 2 and csv_output[1].strip() == ''
    return header_out, csv_output[0]

def print_stats_table(args, csv_output):
    """
    Print stats.
    """
    reader = csv.reader(csv_output.strip().split('\n'), delimiter=',')
    title_row = reader.next()
    row_widths = [0,] * len(TABLE_SETUP)
    for idx, row in enumerate(reader):
        if idx == 0:
            # Print intro:
            for k, v in INTRO_SETUP.iteritems():
                print("{title}: {value}".format(
                    title=v['title'],
                    value=row[title_row.index(k)],
                ))
            print("")
            # Print table header
            for idx, item in enumerate(TABLE_SETUP):
                print(" {title} ".format(title=TABLE_SETUP[item]['title']), end='')
                row_widths[idx] = len(TABLE_SETUP[item]['title'])
                if idx < len(TABLE_SETUP) - 1:
                    print("|", end='')
            print("")
            for idx, item in enumerate(TABLE_SETUP):
                print("-" * (row_widths[idx] + 2), end='')
                if idx < len(TABLE_SETUP) - 1:
                    print("+", end='')
            print("")
        # Print actual row data
        for idx, item in enumerate(TABLE_SETUP):
            format_str = " {{item:>{n}}} ".format(n=row_widths[idx])
            print(format_str.format(item=row[title_row.index(item)]), end='')
            if idx < len(TABLE_SETUP) - 1:
                print("|", end='')
        print("")

def print_debug_table(args, csv_output):
    """
    Print debug output.
    """
    reader = csv.reader(csv_output.strip().split('\n'), delimiter=';')
    print_widths_hex = {
        'u8': 2,
        'sc16': 8,
        'fc32': 16,
        's16': 4,
    }
    if args.hex:
        format_str = "{{0[0]:0>{n_in}}} => {{0[1]:0>{n_out}}}".format(
            n_in=print_widths_hex[getattr(args, 'in').split('_', 1)[0]],
            n_out=print_widths_hex[args.out.split('_', 1)[0]]
        )
    else:
        format_str = "{0[0]}\t=>\t{0[1]}"
    for row in reader:
        print(format_str.format(row))

def setup_argparse():
    """ Configure arg parser. """
    parser = argparse.ArgumentParser(
        description="UHD Converter Benchmark + Debugging Utility.",
    )
    parser.add_argument(
        "-i", "--in", required=True,
        help="Input format  (e.g. 'sc16')"
    )
    parser.add_argument(
        "-o", "--out", required=True,
        help="Output format  (e.g. 'sc16')"
    )
    parser.add_argument(
        "-s", "--samples", type=int,
        help="Number of samples per iteration"
    )
    parser.add_argument(
        "-N", "--iterations", type=int,
        help="Number of iterations per benchmark",
    )
    parser.add_argument(
        "-p", "--priorities",
        help="Converter priorities. Can be 'default', 'all', or a comma-separated list of priorities.",
    )
    parser.add_argument(
        "--max-prio", type=int,
        help="Largest available priority (advanced feature)",
    )
    parser.add_argument(
        "--n-inputs", type=int,
        help="Number of input vectors",
    )
    parser.add_argument(
        "--n-outputs", type=int,
        help="Number of output vectors",
    )
    parser.add_argument(
        "--seed-mode", choices=('random', 'incremental'),
        help="How to initialize the data: random, incremental",
    )
    parser.add_argument(
        "--debug-converter", action='store_true',
        help="Skip benchmark and print conversion results. Implies iterations==1 and will only run on a single converter.",
    )
    parser.add_argument(
        "--hex", action='store_true',
        help="In debug mode, display data as hex values.",
    )
    return parser

def main():
    """ Go, go, go! """
    args = setup_argparse().parse_args()
    print("Running converter benchmark...")
    header_out, csv_output = run_benchmark(args)
    print(header_out)
    if args.debug_converter:
        print_debug_table(args, csv_output)
    else:
        print_stats_table(args, csv_output)

if __name__ == "__main__":
    main()

