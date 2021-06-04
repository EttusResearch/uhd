#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utilities to write BIST executables for USRPs
"""

import os
import re
import sys
import time
import json
import select
import socket
from datetime import datetime
import argparse
import subprocess
from six import iteritems
from usrp_mpm.sys_utils import ectool

##############################################################################
# Aurora/SFP BIST code
##############################################################################
def get_sfp_bist_defaults():
    " Default dictionary for SFP/Aurora BIST dry-runs "
    return {
        'elapsed_time': 1.0,
        'max_roundtrip_latency': 0.8e-6,
        'throughput': 1000e6,
        'max_ber': 8.5e-11,
        'errors': 0,
        'bits': 12012486656,
    }

def assert_aurora_image(master, slave, device_args, product_id, aurora_image_type='AA'):
    """
    Make sure we have an FPGA image with which we can run the requested tests.

    Will load an AA image if not, which always satisfies all conditions for
    running Aurora tests.
    """
    from usrp_mpm.sys_utils import uio
    if not uio.find_uio_device(master)[0] or \
            (slave is not None and not uio.find_uio_device(slave)[0]):
        load_fpga_image(
            fpga_type=aurora_image_type,
            device_args=device_args,
            product_id=product_id,
        )

def aurora_results_to_status(bist_results):
    """
    Convert a dictionary coming from AuroraControl BIST to one that we can use
    for this BIST
    """
    return bist_results['mst_errors'] == 0, {
        'elapsed_time': bist_results['time_elapsed'],
        'max_roundtrip_latency': bist_results['mst_latency_us'],
        'throughput': bist_results['approx_throughput'],
        'max_ber': bist_results['max_ber'],
        'errors': bist_results['mst_errors'],
        'bits': bist_results['mst_samps'],
    }

def run_aurora_bist(
        device_args,
        product_id,
        master,
        slave=None,
        requested_rate=1300*8e6,
        aurora_image_type='AA'):
    """
    Spawn a BER test
    """
    from usrp_mpm import aurora_control
    from usrp_mpm.sys_utils.uio import open_uio

    class DummyContext(object):
        """Dummy class for context managers"""
        def __enter__(self):
            return

        def __exit__(self, exc_type, exc_value, traceback):
            return exc_type is None

    # Go, go, go!
    try:
        assert_aurora_image(master, slave, device_args, product_id, aurora_image_type)
        with open_uio(label=master, read_only=False) as master_au_uio:
            master_au_ctrl = aurora_control.AuroraControl(master_au_uio)
            with open_uio(label=slave, read_only=False)\
                    if slave is not None else DummyContext() as slave_au_uio:
                slave_au_ctrl = aurora_control.AuroraControl(slave_au_uio)\
                    if slave is not None else None
                return master_au_ctrl.run_ber_loopback_bist(
                    duration=10,
                    requested_rate=requested_rate,
                    slave=slave_au_ctrl,
                )
    except Exception as ex:
        print("Unexpected exception: {}".format(str(ex)))
        exit(1)


##############################################################################
# Helpers
##############################################################################
def post_results(results):
    """
    Given a dictionary, post the results.

    This will print the results as JSON to stdout.
    """
    print(json.dumps(
        results,
        sort_keys=True,
        indent=4,
        separators=(',', ': ')
    ))

def sock_read_line(my_sock, timeout=60, interval=0.1):
    """
    Read from a socket until newline. If there was no newline until the timeout
    occurs, raise an error. Otherwise, return the line.
    """
    line = b''
    end_time = time.time() + timeout
    while time.time() < end_time:
        socket_ready = select.select([my_sock], [], [], 0)[0]
        if socket_ready:
            next_char = my_sock.recv(1)
            if next_char == b'\n':
                return line.decode('ascii')
            line += next_char
        else:
            time.sleep(interval)
    raise RuntimeError("sock_read_line() exceeded read timeout!")

def poll_with_timeout(state_check, timeout_ms, interval_ms):
    """
    Calls state_check() every interval_ms until it returns a positive value, or
    until a timeout is exceeded.

    Returns True if state_check() returned True within the timeout.
    """
    max_time = time.time() + (float(timeout_ms) / 1000)
    interval_s = float(interval_ms) / 1000
    while time.time() < max_time:
        if state_check():
            return True
        time.sleep(interval_s)
    return False

def expand_options(option_list):
    """
    Turn a list ['foo=bar', 'spam=eggs'] into a dictionary {'foo': 'bar',
    'spam': 'eggs'}.
    """
    return dict(x.split('=') for x in option_list)

def load_fpga_image(
        fpga_type,
        device_args,
        product_id,
        images_folder='/usr/share/uhd/images/'):
    """Load an FPGA image (1G, XG, AA, ...)"""
    # cmd = ['uhd_image_loader', '--args', 'type=e3xx,addr=127.0.0.1', '--fpga-path']
    fpga_file_name = \
        'usrp_' + product_id + '_fpga_' + fpga_type.upper() + '.bit'
    fpga_image = images_folder + fpga_file_name
    cmd = [
        'uhd_image_loader',
        '--args', device_args,
        '--fpga-path', fpga_image
    ]
    cmd_str = ' '.join(cmd)
    subprocess.check_output(
        cmd_str,
        stderr=subprocess.STDOUT,
        shell=True
    )

def filter_results_for_lv(results, lv_compat_format):
    """
    The LabView JSON parser does not support a variety of things, such as
    nested dicts, and some downstream LV applications freak out if certain keys
    are not what they expect.
    This is a long hard-coded list of how results should look like for those
    cases. Note: This list needs manual supervision and attention for the case
    where either subsystems get renamed, or other architectural changes should
    occur.
    """
    def fixup_dict(result_dict, ref_dict):
        """
        Touches up result_dict according to ref_dict by the following rules:
        - If a key is in result_dict that is not in ref_dict, delete that
        - If a key is in ref_dict that is not in result_dict, use the value
          from ref_dict
        """
        ref_dict['error_msg'] = ""
        ref_dict['status'] = False
        result_dict = {
            k: v for k, v in iteritems(result_dict)
            if k in ref_dict or k in ('error_msg', 'status')
        }
        result_dict = {
            k: result_dict.get(k, ref_dict[k]) for k in ref_dict
        }
        return result_dict
    # GoGoGo
    results = {
        testname: fixup_dict(testresults, lv_compat_format[testname]) \
                    if testname in lv_compat_format else testresults
        for testname, testresults in iteritems(results)
    }
    return results

def get_product_id_from_eeprom(valid_ids, cmd='eeprom-id'):
    """Return the mboard product ID

    Returns something like n300, n310, e320...
    the eeprom parameter is needed if there are several eeprom within the system
    """
    output = subprocess.check_output(
        [cmd],
        stderr=subprocess.STDOUT,
        shell=True,
    ).decode('utf-8')
    for valid_id in valid_ids:
        if valid_id in output:
            return valid_id
    raise AssertionError("Cannot determine product ID.: `{}'".format(output))

def get_tpm_caps_info():
    """Read 'caps' info from TPM subsystem"""
    result = {}
    props_to_read = ('caps',)
    base_path = '/sys/class/tpm'
    for tpm_device in os.listdir(base_path):
        if tpm_device.startswith('tpm'):
            for key in props_to_read:
                result['{}_{}'.format(tpm_device, key)] = open(
                    os.path.join(base_path, tpm_device, key), 'r'
                ).read().strip()
    return result

def gpio_set_all(gpio_bank, value, gpio_size, ddr_mask):
    """
    Helper function to set GPIOs

    What this function do is take decimal value and convert to a binary string
    then try to set those individual bits to the gpio_bank.
    Arguments:
        gpio_bank  -- gpio bank type.
        value -- value to set onto gpio bank.
        gpio_size -- size of the gpio bank
        ddr_mask  -- data direction register bit mask. 0 is input; 1 is output.
    """
    ddr_size = bin(ddr_mask).count("1")
    value_bitstring = \
        ('{0:0' + str(ddr_size) + 'b}').format(value)[-(gpio_size):]
    ddr_bitstring = \
        ('{0:0' + str(gpio_size) + 'b}').format(ddr_mask)[-(gpio_size):]
    for i in range(gpio_size):
        if ddr_bitstring[gpio_size - 1 - i] == "1":
            gpio_bank.set(i, value_bitstring[i % ddr_size])

##############################################################################
# Common tests
##############################################################################
def test_ddr3_with_usrp_probe(extra_args=None):
    """
    Run uhd_usrp_probe and scrape the output to see if the DRAM FIFO block is
    reporting a good throughput. This is a bit of a roundabout way of testing
    the DDR3, but it uses existing software and also tests the RFNoC pathways.
    """
    dflt_args = {'addr':'127.0.0.1', 'rfnoc_num_blocks':1}
    extra_args = extra_args or {}
    # merge args dicts, extra_args overrides dflt_args if keys exists in both dicts
    args = {**dflt_args, **extra_args}
    args_str = ",".join(
        ['{k}={v}'.format(k=k, v=v) for k, v in iteritems(args)])
    cmd = [
        'uhd_usrp_probe',
        '--args',
        '{e}'.format(e=args_str)
    ]
    ddr3_bist_executor = ' '.join(cmd)
    try:
        output = subprocess.check_output(
            ddr3_bist_executor,
            stderr=subprocess.STDOUT,
            shell=True,
        )
    except subprocess.CalledProcessError as ex:
        # Don't throw errors from uhd_usrp_probe
        output = ex.output
    output = output.decode("utf-8")
    if re.search(r"DmaFIFO", output) is None:
        return {
            'error_msg': "DmaFIFO block not enabled. Cannot execute DDR3 BIST!",
            'throughput': 0,
        }
    mobj = re.search(r"Throughput: (?P<thrup>[0-9.]+)\s?MB", output)
    if mobj is not None:
        return {'throughput': float(mobj.group('thrup')) * 1000}
    else:
        return {
            'throughput': 0,
            'error_msg': "Failed match throughput regex!",
        }


def get_gpsd_tpv_result():
    """
    Query gpsd via a socket and return the corresponding JSON result as a
    dictionary.
    """
    my_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    my_sock.connect(('localhost', 2947))
    sys.stderr.write("Connected to GPSDO socket.\n")
    query_cmd = b'?WATCH={"enable":true,"json":true}'
    my_sock.sendall(query_cmd)
    sys.stderr.write("Sent query: {}\n".format(query_cmd))
    sock_read_line(my_sock, timeout=10)
    sys.stderr.write("Received initial newline.\n")
    result = {}
    while result.get('class', None) != 'TPV':
        json_result = sock_read_line(my_sock, timeout=60)
        sys.stderr.write(
            "Received JSON response: {}\n\n".format(json_result)
        )
        result = json.loads(json_result)
    my_sock.sendall(b'?WATCH={"enable":false}')
    my_sock.close()
    return result

def get_ref_clock_prop(clock_source, time_source, extra_args=None):
    """
    Helper function to determine reference clock lock
    Description: Checks to see if we can lock to a clock source.
    The actual value is yanked from the property tree.

    External Equipment: None
    Return dictionary:
     - <sensor-name>:
       - locked: Boolean lock status
    """
    dflt_args = {'addr':'127.0.0.1'}
    extra_args = extra_args or {}
    # merge args dicts, extra_args overrides dflt_args if keys exists in both dicts
    args = {**dflt_args, **extra_args}
    result = {}

    args_str = ",".join(
        ['{k}={v}'.format(k=k, v=v) for k, v in iteritems(args)])
    cmd = [
        'uhd_usrp_probe',
        '--args',
        'clock_source={c},time_source={t},{e}'.format(
            c=clock_source, t=time_source, e=args_str),
        '--sensor'
    ]
    sensor_path = '/mboards/0/sensors/ref_locked'
    cmd.append(sensor_path)
    ref_lock_executor = ' '.join(cmd)
    try:
        output = subprocess.check_output(
            ref_lock_executor,
            stderr=subprocess.PIPE,
            shell=True,
        )
    except subprocess.CalledProcessError as ex:
        # Don't throw errors from uhd_usrp_probe
        output = ex.output
    output = output.decode("utf-8").strip()
    mobj = re.search(r"true$", output)
    if mobj is not None:
        result['ref_locked'] = True
    else:
        result['ref_locked'] = False
        result['error_msg'] = "Reference Clock not locked: " + output
    return result

def get_temp_sensor_value(temp_sensor_map):
    """
    Read a temp sensor value from the system and return a dictionary of the
    form {temp_sensor_lookup(device): $temp}
    """
    import pyudev
    context = pyudev.Context()
    return {
        temp_sensor_map(device): \
                int(device.attributes.get('temp').decode('ascii'))
        for device in context.list_devices(subsystem='thermal')
        if 'temp' in device.attributes.available_attributes \
                and device.attributes.get('temp') is not None
    }


def get_iio_temp_sensor_values():
    """
    Read all devices in the IIO subsystem that can report a temperature and
    returns dictionary containing {name: temperature}, where name is the
    temperature device name and the temperature is a value in mC.
    """
    import pyudev
    context = pyudev.Context()
    iio_devs = context.list_devices(subsystem='iio')

    def is_temp_dev(dev):
        return 'in_temp_raw' in dev.attributes.available_attributes

    def get_temp(dev):
        raw = float(dev.attributes.get('in_temp_raw').decode('ascii'))
        offset = float(dev.attributes.get('in_temp_offset').decode('ascii'))
        scale = float(dev.attributes.get('in_temp_scale').decode('ascii'))
        return int(scale * (raw + offset))

    def get_name(dev):
        return dev.attributes.get('name').decode('ascii')

    temp_devs = [dev for dev in iio_devs if is_temp_dev(dev)]
    return {get_name(dev): get_temp(dev) for dev in temp_devs}


def get_fan_values():
    """
    Return a dict of fan name -> fan speed key/values.
    """
    import pyudev
    context = pyudev.Context()
    return {
        device.sys_name: int(device.attributes.get('cur_state'))
        for device in context.list_devices(subsystem='thermal')
        if 'cur_state' in device.attributes.available_attributes \
                and device.attributes.get('cur_state') is not None
    }

def get_ectool_fan_values():
    try:
        return ectool.get_fan_rpm()
    except RuntimeError as ex:
        return {
            'error_msg': "{}".format(str(ex))
        }

def get_link_up(if_name):
    """
    Return a dictionary {if_name: IFLA_OPERSTATE}
    """
    from pyroute2 import IPRoute
    result = {}
    with IPRoute() as ipr:
        links = ipr.link_lookup(ifname=if_name)
        if not links:
            return {'error_msg': "No interface found"}
        link_info = next(iter(ipr.get_links(links)), None)
        if link_info is None:
            return {'error_msg': "Error on get_links for sfp0"}
        result['sfp0'] = link_info.get_attr('IFLA_OPERSTATE')
        if result['sfp0'] != 'UP':
            result['error_msg'] = "Link not up for interface"
    return result



##############################################################################
# BIST class
##############################################################################
class UsrpBIST(object):
    """
    BIST parent class
    """
    usrp_type = None
    default_rev = 3 # Because why not
    # This defines special tests that are really collections of other tests.
    collections = {
        'standard': ["rtc",],
        'extended': "*",
    }
    # Default FPGA image type
    DEFAULT_FPGA_TYPE = None
    lv_compat_format = None
    device_args = 'addr=127.0.0.1'

    def make_arg_parser(self):
        """
        Return arg parser
        """
        parser = argparse.ArgumentParser(
            description="{} BIST Tool".format(self.usrp_type),
        )
        parser.add_argument(
            '-n', '--dry-run', action='store_true',
            help="Fake out the tests. All tests will return a valid" \
                 " response, but will not actually interact with hardware.",
        )
        parser.add_argument(
            '-v', '--verbose', action='store_true',
            help="Crank up verbosity level",
        )
        parser.add_argument(
            '--debug', action='store_true',
            help="For debugging this tool.",
        )
        parser.add_argument(
            '--option', '-o', action='append', default=[],
            help="Option for individual test.",
        )
        parser.add_argument(
            '--lv-compat', action='store_true',
            help="Provides compatibility with the LV JSON parser. Don't run "
                 "this mode unless you know what you're doing. The JSON "
                 "output does not necessarily reflect the actual system "
                 "status when using this mode.",
        )
        parser.add_argument(
            '--skip-fpga-reload', action='store_true',
            help="Skip reloading the default FPGA image post-test. Note: by"
                 "specifying this argument, the FPGA image loaded could be "
                 "anything post-test.",
        )
        parser.add_argument(
            'tests',
            help="List the tests that should be run",
            nargs='+', # There has to be at least one
        )
        return parser

    def get_mb_periph_mgr(self):
        """Needs to be implemented by child class"""
        raise NotImplementedError

    def get_product_id(self):
        """Needs to be implemented by child class"""
        raise NotImplementedError

    def __init__(self):
        assert self.DEFAULT_FPGA_TYPE is not None
        assert self.device_args is not None
        assert self.usrp_type is not None
        assert self.lv_compat_format is not None
        self.args = self.make_arg_parser().parse_args()
        self.args.option = expand_options(self.args.option)
        # If this is true, trigger a reload of the default FPGA image
        self.reload_fpga_image = False
        try:
            default_rev = self.get_mb_periph_mgr().mboard_max_rev
        except ImportError:
            # This means we're in dry run mode or something like that, so just
            # pick something
            default_rev = self.default_rev
        self.mb_rev = int(self.args.option.get('mb_rev', default_rev))
        self.tests_to_run = set()
        for test in self.args.tests:
            if test in self.collections:
                for this_test in self.expand_collection(test):
                    self.tests_to_run.add(this_test)
            else:
                self.tests_to_run.add(test)
        try:
            # Keep this import here so we can do dry-runs without any MPM code
            from usrp_mpm import get_main_logger
            if not self.args.verbose:
                from usrp_mpm.mpmlog import WARNING
                get_main_logger().setLevel(WARNING)
            self.log = get_main_logger().getChild('main')
        except ImportError:
            print("No logging capability available.")

    def expand_collection(self, coll):
        """
        Return names of tests in a collection
        """
        tests = self.collections[coll]
        if tests == "*":
            tests = {x.replace('bist_', '')
                     for x in dir(self)
                     if x.find('bist_') == 0
                    }
        else:
            tests = set(tests)
        return tests

    def run(self):
        """
        Execute tests.

        Returns True on Success.
        """
        def execute_test(testname):
            """
            Actually run a test.
            """
            testmethod_name = "bist_{0}".format(testname)
            sys.stderr.write(
                "Executing test method: {0}\n\n".format(testmethod_name)
            )
            if not hasattr(self, testmethod_name):
                sys.stderr.write("Test not defined: `{}`\n".format(testname))
                return False, {}
            try:
                status, data = getattr(self, testmethod_name)()
                data['status'] = status
                data['error_msg'] = data.get('error_msg', '')
                return status, data
            except Exception as ex:
                sys.stderr.write(
                    "Test {} failed to execute: {}\n".format(testname, str(ex))
                )
                if self.args.debug:
                    raise
                return False, {'error_msg': str(ex)}
        tests_successful = True
        result = {}
        for test in self.tests_to_run:
            status, result_data = execute_test(test)
            tests_successful = tests_successful and status
            result[test] = result_data
        if self.args.lv_compat:
            result = filter_results_for_lv(result, self.lv_compat_format)
        post_results(result)
        if self.reload_fpga_image and not self.args.skip_fpga_reload:
            load_fpga_image(
                self.DEFAULT_FPGA_TYPE,
                self.device_args,
                self.get_product_id(),
            )
        return tests_successful

#############################################################################
# BISTS
# All bist_* methods must return True/False success values!
#############################################################################
    def bist_rtc(self):
        """
        BIST for RTC (real time clock)

        Return dictionary:
        - date: Returns the current UTC time, with seconds-accuracy, in ISO 8601
                format, as a string. As if running 'date -Iseconds -u'.
        - time: Same time, but in seconds since epoch.

        Return status:
        Unless datetime throws an exception, returns True.
        """
        assert 'rtc' in self.tests_to_run
        utc_now = datetime.utcnow()
        return True, {
            'time': time.mktime(utc_now.timetuple()),
            'date': utc_now.replace(microsecond=0).isoformat() + "+00:00",
        }
