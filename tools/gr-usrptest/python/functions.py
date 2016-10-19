#!/usr/bin/env python2
import numpy as np
import time
import copy
import logging


def setup_phase_alignment_parser(parser):
    test_group = parser.add_argument_group(
        'Phase alignment specific arguments')
    test_group.add_argument(
        '--runs',
        default=10,
        type=int,
        help='Number of times to retune and measure d_phi')
    test_group.add_argument(
        '--duration',
        default=5.0,
        type=float,
        help='Duration of a measurement run')
    test_group.add_argument(
        '--measurement-setup',
        type=str,
        help='Comma-seperated list of channel ids. Phase difference will be calculated between consecutive channels. default=(0,1,2,..,M-1) M: num_chan'
    )
    test_group.add_argument(
        '--log-level',
        type=str,
        choices=["critical", "error", "warning", "info", "debug"],
        default="info")
    test_group.add_argument(
        '--freq-bands',
        type=int,
        help="Number of frequency bands in daughterboard range to randomly retune to",
        default=1)
    return parser


def setup_tx_phase_alignment_parser(parser):
    tx_group = parser.add_argument_group(
        'TX Phase alignment specific arguments.')
    tx_group.add_argument(
        '--tx-channels', type=str, help='which channels to use')
    tx_group.add_argument(
        '--tx-antenna',
        type=str,
        help='comma-separated list of channel antennas for tx')
    tx_group.add_argument(
        '--tx-offset',
        type=float,
        help='frequency offset in Hz which should be added to center frequency for transmission'
    )
    return parser


def setup_rts_phase_alignment_parser(parser):
    rts_group = parser.add_argument_group(
        'RTS Phase alignment specific arguments')
    rts_group.add_argument(
        '-pd',
        '--phasedev',
        type=float,
        default=1.0,
        help='maximum phase standard deviation of dphi in a run which is considered settled (in deg)'
    )
    rts_group.add_argument(
        '-dp',
        '--dphi',
        type=float,
        default=2.0,
        help='maximum allowed d_phase deviation between runs (in deg)')
    rts_group.add_argument(
        '--freqlist',
        type=str,
        help='comma-separated list of frequencies to test')
    rts_group.add_argument(
        '--lv-host',
        type=str,
        help='specify this argument if running tests with vst/switch')
    rts_group.add_argument('--lv-vst-name', type=str, help='vst device name')
    rts_group.add_argument(
        '--lv-switch-name', type=str, help='executive switch name')
    rts_group.add_argument(
        '--lv-basepath',
        type=str,
        help='basepath for LabVIEW VIs on Windows')
    rts_group.add_argument(
        '--tx-offset',
        type=float,
        help='transmitter frequency offset in VST')
    rts_group.add_argument(
        '--lv-switch-ports', type=str, help='comma-separated switch-port pair')
    return parser

def setup_manual_phase_alignment_parser(parser):
    manual_group = parser.add_argument_group(
        'Manual Phase alignment specific arguments')
    manual_group.add_argument(
        '--plot',
        dest='plot',
        action='store_true',
        help='Set this argument to enable plotting results with matplotlib'
    )
    manual_group.add_argument(
        '--auto',
        action='store_true',
        help='Set this argument to enable automatic selection of test frequencies'
    )
    manual_group.add_argument(
        '--start-freq',
        type=float,
        default=0.0,
        help='Start frequency for automatic selection'
    ),
    manual_group.add_argument(
        '--stop-freq',
        type=float,
        default=0.0,
        help='Stop frequency for automatic selection')

    parser.set_defaults(plot=False,auto=False)
    return parser


def process_measurement_sinks(top_block):
    data = list()
    curr_data = dict()
    for num, chan in enumerate(top_block.measurement_channels[:-1]):
        curr_data['avg'] = list(top_block.measurement_sink[num].get_avg())
        curr_data['stddev'] = list(top_block.measurement_sink[num].get_stddev(
        ))
        curr_data['first'] = top_block.measurement_channels_names[num]
        curr_data['second'] = top_block.measurement_channels_names[num + 1]
        data.append(copy.copy(curr_data))
    return data


def run_test(top_block, ntimes):
    results = dict()
    num_sinks = len(top_block.measurement_sink)
    for i in xrange(ntimes):
        #tune frequency to random position and back to specified frequency
        top_block.retune_frequency(bands=top_block.uhd_app.args.freq_bands,band_num=i+1)
        time.sleep(2)
        #trigger start in all measurement_sinks
        for sink in top_block.measurement_sink:
            sink.start_run()
        #wait until every measurement_sink is ready with the current run
        while (sum([ms.get_run() for ms in top_block.measurement_sink]) < (
            (i + 1) * num_sinks)):
            time.sleep(1)
    results = process_measurement_sinks(top_block)
    return results


def log_level(string):
    return getattr(logging, string.upper())
