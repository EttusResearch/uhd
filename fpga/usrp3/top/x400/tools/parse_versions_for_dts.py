#!/usr/bin/env python3
#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

"""
This script parses versioning information from a Verilog file and generates a
devicetree include file (.dtsi) that follows the syntax that is required by MPM
for version checking.

usage: parse-versions-for-dts.py [-h] --input INPUT --output OUTPUT
                                 --components COMPONENTS

Arguments:

  -h, --help            show this help message and exit
  --input INPUT         The input file(s) to parse. A comma separated list can
                        be used to specify multiple files
  --output OUTPUT       The output file
  --components COMPONENTS
                        The components for which the version information is
                        extracted. A comma separated list can be used to
                        specify multiple components.

Example call:

./parse-versions-for-dts.py \
        --input regmap/global_regs_regmap_utils.vh \
        --output build/component_versions.dtsi \
        --components fpga

Example for variable definitions in a Verilog file
(regmap/global_regs_regmap_utils.vh):

    localparam FPGA_CURRENT_VERSION_MAJOR = 'h4;
    localparam FPGA_CURRENT_VERSION_MINOR = 'h2;
    localparam FPGA_CURRENT_VERSION_BUILD = 'h18;
    localparam FPGA_OLDEST_COMPATIBLE_VERSION_MAJOR = 'h4;
    localparam FPGA_OLDEST_COMPATIBLE_VERSION_MINOR = 'h0;
    localparam FPGA_OLDEST_COMPATIBLE_VERSION_BUILD = 'h0;
    localparam FPGA_VERSION_LAST_MODIFIED_TIME = 'h20111013;

Example for the generated dtsi include file (build/component_versions.dtsi)

    // mpm_version fpga_current_version 4.2.24
    // mpm_version fpga_oldest_compatible_version 4.0.0
    // mpm_version fpga_version_last_modified_time 0x20111013
"""
import argparse
import re
import sys

def parse_args():
    """Parser for the command line arguments"""
    parser = argparse.ArgumentParser(description='Parse variables from Verilog files')
    parser.add_argument('--input', required=True,
                        help='The input file(s) to parse. A comma separated ' \
                             'list can be used to specify multiple files')
    parser.add_argument('--output', required=True, help='The output file')
    parser.add_argument('--components', required=True,
                        help='The components for which the version information ' \
                             'is extracted. A comma separated list can be used ' \
                             'to specify multiple components.')
    args = parser.parse_args()
    args.input = args.input.split(',')
    args.components = args.components.split(',')
    return args

def parse_variables_from_verilog(filename):
    """Parses variables from a Verilog file and stores them in a dict.
    The following syntax is supported:
      localparam <VARIABLE> = <VALUE>;
    :param str filename: input filename
    """
    pattern_dec = re.compile(r'^(\d*)$')
    pattern_hex = re.compile(r'^\d*\'h(\w*)$')

    def _convert_verilog_datatype(str_value):
        """Converts the Verilog value string to the corresponding Python datatype
        The following notations are supported:
        - decimal (e.g. 1)
        - hexadecimal (e.g. 'h4)
        - hexadecimal with specification of the variable width (e.g. 32'h4)
        :param str str_value: the string value to be converted
        """
        match_dec = pattern_dec.match(str_value)
        if match_dec:
            return int(match_dec.group(1))
        match_hex = pattern_hex.match(str_value)
        if match_hex:
            return int(match_hex.group(1), 16)
        return '?'

    with open(filename) as f:
        lines = f.read().splitlines()
    pattern = re.compile(r'\s*localparam (\S*)\s*=\s*(\S*);')
    variables = {}
    for line in lines:
        match = pattern.match(line)
        if match:
            key = match.group(1)
            value = _convert_verilog_datatype(match.group(2))
            variables[key] = value
    return variables

def map_variables(variables, component):
    """map the variables as they shall appear in the generated dtsi file
    :param dict variables: the variables that were parsed from the Verilog file(s)
    :param str component: the name of the component
    """
    COMPONENT = component.upper()
    mapped_vars = {
        '{}_current_version'.format(component): '{}.{}.{}'.format(
            variables['{}_CURRENT_VERSION_MAJOR'.format(COMPONENT)],
            variables['{}_CURRENT_VERSION_MINOR'.format(COMPONENT)],
            variables['{}_CURRENT_VERSION_BUILD'.format(COMPONENT)]
        ),
        '{}_oldest_compatible_version'.format(component): '{}.{}.{}'.format(
            variables['{}_CURRENT_VERSION_MAJOR'.format(COMPONENT)],
            variables['{}_CURRENT_VERSION_MINOR'.format(COMPONENT)],
            variables['{}_CURRENT_VERSION_BUILD'.format(COMPONENT)]
        ),
        '{}_version_last_modified_time'.format(component): '0x{:X}'.format(
            variables['{}_VERSION_LAST_MODIFIED_TIME'.format(COMPONENT)]
        ),
    }
    return mapped_vars

def generate_dtsi_file(filename, mapped_vars):
    """Generate the dtsi file
    :param str filename: the output filename
    :param dict mapped_vars: the variables that shall be written to the file
    """
    with open(filename, 'w') as f:
        for k, v in mapped_vars.items():
            f.write('// mpm_version {} {}\n'.format(k, v))

def main():
    """ main function """
    args = parse_args()
    variables = {}
    mapped_vars = {}
    for file in args.input:
        variables.update(parse_variables_from_verilog(file))
    for component in args.components:
        mapped_vars.update(map_variables(variables, component))
    generate_dtsi_file(args.output, mapped_vars)
    return True

if __name__ == '__main__':
    sys.exit(not main())
