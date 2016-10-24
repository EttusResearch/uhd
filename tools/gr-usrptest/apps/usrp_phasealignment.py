#!/usr/bin/env python

from gnuradio.uhd.uhd_app import UHDApp
from usrptest.flowgraphs import phasealignment_fg
from usrptest.functions import run_test, setup_phase_alignment_parser, setup_tx_phase_alignment_parser, setup_manual_phase_alignment_parser
import time
import argparse


def plot_results(results):
    import matplotlib.pyplot as plt
    ax = plt.axes()
    ax.set_ylim(-180, 180)
    for result in results:
        plt.errorbar(
            range(len(result['avg'])),
            result["avg"],
            result["stddev"],
            label="{} - {}".format(result["first"], result["second"]),
            axes=ax)
    ax.legend(loc='upper left', bbox_to_anchor=(0.0, 0.0))
    plt.show()


def print_results(results):
    for result in results:
        print('Results for: {first} - {second}'.format(
            first=result['first'], second=result['second']))
        for i, (avg,
                stddev) in enumerate(zip(result['avg'], result['stddev'])):
            print('\t {}. run avg: {}, stddev: {}'.format(i + 1, avg, stddev))


def main():
    parser = argparse.ArgumentParser(conflict_handler='resolve')
    UHDApp.setup_argparser(parser=parser)
    parser = setup_phase_alignment_parser(parser)
    parser = setup_tx_phase_alignment_parser(parser)
    parser = setup_manual_phase_alignment_parser(parser)
    args = parser.parse_args()
    test_app = UHDApp(args=args)
    if args.auto and args.start_freq and args.stop_freq:
        from random import uniform
        bw = (args.stop_freq - args.start_freq) / args.freq_bands
        for nband in range(args.freq_bands):
            freq1 = args.start_freq + nband * bw
            new_freq = uniform(freq1, freq1 + bw)
            test_app.args.freq = new_freq
            raw_input(
                "New test frequency: {:f} MHz. Adjust your signal generator and press ENTER to start measurement.".
                format(new_freq / 1e6))
            fg = phasealignment_fg.phasealignment_fg(test_app)
            fg.start()
            results = run_test(fg, args.runs)
            fg.stop()
            fg.wait()
            if args.plot:
                plot_results(results)
            print_results(results)
    else:
        fg = phasealignment_fg.phasealignment_fg(test_app)
        fg.start()
        results = run_test(fg, args.runs)
        fg.stop()
        fg.wait()
        if args.plot:
            plot_results(results)
        print_results(results)


if __name__ == '__main__':
    main()
