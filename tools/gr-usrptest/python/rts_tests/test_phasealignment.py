#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2016 Ettus Research LLC.
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#
import unittest
from tinydb import TinyDB, Query
from usrptest.flowgraphs import phasealignment_fg
from usrptest.functions import setup_phase_alignment_parser, run_test
from gnuradio.uhd.uhd_app import UHDApp
import numpy as np
import argparse
import time

class gr_usrp_test(unittest.TestCase):
    def __init__(self, methodName='runTest', args=None):
        super(gr_usrp_test,self).__init__(methodName)
        self.args = args

class qa_phasealignment(gr_usrp_test):
    def setUp(self):
        self.uhd_app = UHDApp(args=self.args)
        self.tb = phasealignment_fg.phasealignment_fg(self.uhd_app)
        self.db = TinyDB('phase_db.json')

    def tearDown(self):
        self.uhd_app = None
        self.tb = None

    def test_001(self):
        self.tb.start()
        time.sleep(2)
        results = run_test(self.tb,self.args.runs) # dict key:dev, value: dict{dphase:[],stddev:[]}
        time.sleep(1)
        self.first_device = self.tb.measurement_channels_names[:-1]
        self.second_device = self.tb.measurement_channels_names[1:]
        #self.tb.stop()
        #self.tb.wait()
        self.time_stamp = time.strftime('%Y%m%d%H%M')
        self.passed = True
        for fdev, sdev in zip(self.first_device,self.second_device):
            print('Comparing values for phase difference between {} and {}'.format(fdev, sdev))
            dphase_list = results[fdev]['avg']
            dev_list = results[fdev]['stddev']
            dphase = np.average(dphase_list)
            dev = np.average(dev_list)
            ref_meas = get_reference_meas(self.db, fdev, sdev, self.args.freq)
            for dphase_i in dphase_list:
                passed = True
                if abs(dphase_i - dphase) > self.args.dphi and passed:
                    print('\t dPhase of a measurement_run differs from average dhpase. dphase_run: {}, dphase_avg: {}'.format(dphase_i, dphase))
                    passed = False
            if dev > self.args.phasedev:
                print('\t dPhase deviates during measurement. stddev: {}'.format(dev))
                passed = False
            if ref_meas:
                if abs(ref_meas['dphase'] - dphase) > self.args.dphi:
                    print('\t dPhase differs from reference measurement. Now: {}, reference: {}'.format(dphase, ref_meas['dphase']))
            if not passed:
                self.passed = False
            else:
                self.db.insert({'dev1':fdev, 'dev2':sdev, 'timestamp':self.time_stamp, 'dphase':dphase, 'dphase_dev': dev, 'freq': self.args.freq})
        self.tb.stop()
        self.assertTrue(self.passed)

def get_previous_meas(db, dev1, dev2, freq):
    meas = Query()
    results = db.search((meas.dev1 == dev1) & (meas.dev2 == dev2) & (meas.freq == freq))
    prev_result = dict()
    if results:
        prev_result = results[0]
        for result in results:
            if result['timestamp'] > prev_result['timestamp']:
                prev_result = result
    return prev_result

def get_reference_meas(db, dev1, dev2, freq):
    meas = Query()
    results = db.search((meas.dev1 == dev1) & (meas.dev2 == dev2) & (meas.freq == freq))
    ref_result = dict()
    if results:
        ref_result = results[0]
        for result in results:
            if result['timestamp'] < ref_result['timestamp']:
                ref_result = result
    return ref_result



if __name__ == '__main__':
    parser = argparse.ArgumentParser(conflict_handler='resolve')
    parser = setup_phase_alignment_parser(parser)
    UHDApp.setup_argparser(parser=parser)
    args = parser.parse_args()
    def make_suite(testcase_class):
        testloader = unittest.TestLoader()
        testnames = testloader.getTestCaseNames(testcase_class)
        suite = unittest.TestSuite()
        for name in testnames:
            suite.addTest(testcase_class(name, args=args))
        return suite

    # Add tests.
    alltests = unittest.TestSuite()
    alltests.addTest(make_suite(qa_phasealignment))
    result = unittest.TextTestRunner(verbosity=2).run(alltests) # Run tests.
