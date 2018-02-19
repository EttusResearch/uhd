#!/usr/bin/env python
#
# Copyright 2013 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# This script exists for convenience. Run it once to get your hardware info.
# Save the created files in the directory where you execute your tests.

import os
import netifaces
from netifaces import AF_INET
from optparse import OptionParser

try:
    from gnuradio import uhd
except:
    print "Can't gather USRP info! gr-uhd not found."

# If other paths for this file are known, add them to this list.
pci_hwdata_paths = ["/usr/share/hwdata/pci.ids", "/usr/share/misc/pci.ids"]


def main():
    # Just get the file name, where results are supposed to be stored.
    usage = "%prog: [savefile]"
    parser = OptionParser(usage=usage)
    parser.add_option("", "--file", type="string", help="file to save results. [default=%default]", default="usrps_info.txt")
    (options, args) = parser.parse_args()

    eths_ids = get_eths_with_ids()
    for eth in eths_ids:
        print eth
    save_eth_ids_info_to_file(eths_ids, options.file)

    usrps = []
    try:
        usrps = get_usrps_with_device_info()
        for usrp in usrps:
            print usrp
    except Exception as e:
        print "Can't gather USRP info!"
        print e.message,
    try:
        save_usrp_info(usrps, options.file)
    except Exception as e:
        print "Can't save USRP info!"
        print e.message


def get_eths_with_ids():
    eths = get_eth_interface_with_address()
    eths_ids = []
    for eth in eths:
        vd_id = get_vendor_device_id(eth['interface'])
        vd_string = get_pci_string_from_id(vd_id)
        vendor = {'id': vd_id['vendor'], 'name': vd_string['vendor']}
        device = {'id': vd_id['device'], 'name': vd_string['device']}
        phys = {'vendor': vendor, 'device': device}
        eth['physical'] = phys
        eths_ids.append(eth)
    return eths_ids


def get_eth_interface_with_address():
    eths = []
    for iface in netifaces.interfaces():
        if iface.find("eth") == 0:
            ips = netifaces.ifaddresses(iface).get(netifaces.AF_INET)
            macs = netifaces.ifaddresses(iface).get(netifaces.AF_PACKET)
            if ips and macs:
                for ip, mac in zip(ips, macs):
                    eths.append({'interface': iface, 'addr': ip['addr'], 'mac': mac['addr']})
    if not eths:
        print "Can't gather Ethernet info. Check if a network based USRP is connected to host and responding to \'uhd_find_devices\'"
    return eths


def get_usrps_with_device_info():
    devs = uhd.find_devices()
    devs_infos = []
    eths_ids = get_eths_with_ids()
    for dev in devs:
        if dev['addr']:
            ridx = dev['addr'].rfind('.')
            net = dev['addr'][0:ridx + 1]
            for eth in eths_ids:
                if eth['addr'].startswith(net):
                    dev_info = {'type': dev['type'], 'addr': dev['addr'], 'name': dev['name'], 'serial': dev['serial'],
                                'host': eth}
                    devs_infos.append(dev_info)

    return devs_infos


def save_usrp_info(usrps, filename):
    if not usrps:
        print "No USRP data available. Not saving any data."
        return
    with open(filename, 'w') as f:
        if f.closed:
            print "Warning: Couldn't open", filename, "to save results."
        f.write("#\n")
        f.write("#\n")
        f.write("# This file contains gathered information about USRPs connected to the host\n")
        f.write("#\n")
        count = 0
        for usrp in usrps:
            f.write("\n## USRP Device " + str(count) + "\n")
            f.write("type:    " + usrp['type'] + "\n")
            f.write("address: " + usrp['addr'] + "\n")
            f.write("name:    " + usrp['name'] + "\n")
            f.write("serial:  " + usrp['serial'] + "\n")
            f.write("host\n")
            f.write("\t" + usrp['host']['interface'] + "\n")
            f.write("\t" + usrp['host']['addr'] + "\n")
            f.write("\t" + usrp['host']['mac'] + "\n")
            f.write("\t\tphysical port info\n")
            f.write("\t\t\t" + usrp['host']['physical']['vendor']['id'] + " " + usrp['host']['physical']['vendor'][
                'name'] + "\n")
            f.write("\t\t\t" + usrp['host']['physical']['device']['id'] + " " + usrp['host']['physical']['device'][
                'name'] + "\n")
            f.write("## End USRP Device " + str(count) + "\n\n")
            count += 1


def save_eth_ids_info_to_file(eths, filename):
    with open(filename, 'w') as f:
        if f.closed:
            print "Warning: Couldn't open", filename, "to save results."
        f.write("#\n")
        f.write("#\n")
        f.write("# This file contains infos about the available eth interfaces\n")
        f.write("#\n")
        #print eths
        count = 0
        for eth in eths:
            f.write("\n## ETH Interface " + str(count) + "\n")
            f.write(eth['interface'] + "\n")
            f.write("\tip " + eth['addr'] + "\n")
            f.write("\tmac " + eth['mac'] + "\n")
            f.write("phys_port_info\n")
            f.write("\t\tvendor " + eth['physical']['vendor']['id'] + " " + eth['physical']['vendor']['name'] + "\n")
            f.write("\t\tdevice " + eth['physical']['device']['id'] + " " + eth['physical']['device']['name'] + "\n")
            f.write("## End ETH Interface " + str(count) + "\n\n")
            count += 1


def get_vendor_device_id(eth):
    path = "/sys/class/net/" + eth + "/device/"
    vendor_id = get_id(path + "vendor")
    device_id = get_id(path + "device")
    return {'vendor': vendor_id, 'device': device_id}


def get_id(path):
    gid = 0
    with open(path, 'r') as f:
        if f.closed:
            print "Warning: Couldn't open", path, "to gather device information."
        data = f.read()
        gid = data[0:-1]
    return gid


def get_pci_string_from_id(vid):
    vendors = get_vendors()
    vendor_id = vid['vendor'][2:]
    device_id = vid['device'][2:]
    vendor = vendors[vendor_id]['vendor']
    device = vendors[vendor_id]['devices'][device_id]

    return {'vendor': vendor, 'device': device}


_g_vendors = {}


def get_vendors():
    global _g_vendors
    if len(_g_vendors) > 0:
        return _g_vendors

    path = ""
    vendors = {}
    # Check for possible locations of pci.ids on the system.
    for pci_path in pci_hwdata_paths:
        if os.path.isfile(pci_path):
            path = pci_path
            break
    if path == "":
        print "Couldn't find pci.ids file. Vendor data not available!"
        return vendors

    vendor_id = ''
    with open(path, 'r') as f:
        if f.closed:
            print "Warning: Couldn't open", path, ". Vendor data not available."
        for line in f.readlines():
            if line.startswith("#"):
                if line.startswith("# List of known device classes"):
                    break
                else:
                    continue
            l = line.split()
            if len(l) > 1 and not line.startswith("\t"):
                vendor_id = l[0]
                vendor = " ".join(l[1:])
                vendors[vendor_id] = {'vendor': vendor, 'devices': {}}
            if len(l) > 1 and line.startswith("\t") and not line.startswith("\t\t"):
                device_id = l[0]
                device = " ".join(l[1:])
                vendors[vendor_id]['devices'][device_id] = device
    _g_vendors = vendors
    return vendors


if __name__ == '__main__':
    main()
