#! /usr/bin/python

import sys, os
import collections
import argparse
import datetime

# Parse command line options
def get_options():
    parser = argparse.ArgumentParser(description='Create a Makefile for Xilinx IP.')
    parser.add_argument('--ip_name', type=str, default=None, help='Name for the IP core')
    parser.add_argument('--dest', type=str, default=None, help='Destination directory')
    parser.add_argument('--copright_auth', type=str, default='Ettus Research', help='Copyright author')
    args = parser.parse_args()
    if not args.ip_name:
        print('ERROR: Please specify a name for the IP core\n')
        parser.print_help()
        sys.exit(1)
    if not args.dest:
        print('ERROR: Please specify the location for the IP core\n')
        parser.print_help()
        sys.exit(1)
    return args

g_makefile_template = """#
# {copyright}
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

{ip_srcs_var} = $(IP_BUILD_DIR)/{ip_name}/{ip_name}.xci

{ip_outs_var} = $(addprefix $(IP_BUILD_DIR)/{ip_name}/, \\
{ip_name}.xci.out \\
) 

$({ip_srcs_var}) $({ip_outs_var}) : $(IP_DIR)/{ip_name}/{ip_name}.xci
\t$(call BUILD_VIVADO_IP,{ip_name},$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
"""

def main():
    args = get_options();

    transform = {}
    transform['ip_name'] = args.ip_name
    transform['ip_srcs_var'] = 'IP_' + args.ip_name.upper() + '_SRCS'
    transform['ip_outs_var'] = 'IP_' + args.ip_name.upper() + '_OUTS'
    transform['copyright'] = 'Copyright ' + str(datetime.datetime.now().year) + ' ' + args.copright_auth

    with open(os.path.join(args.dest, 'Makefile.inc'), 'w') as mak_file:
        mak_file.write(g_makefile_template.format(**transform))

if __name__ == '__main__':
    main()
