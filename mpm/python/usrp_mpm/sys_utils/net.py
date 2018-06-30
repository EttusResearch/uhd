#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Network utilities for MPM
"""
import itertools
import socket
from six import iteritems
from pyroute2 import IPRoute, IPDB
from usrp_mpm.mpmlog import get_logger

def get_hostname():
    """Return the current device's hostname"""
    return socket.gethostname()

def get_valid_interfaces(iface_list):
    """
    Given a list of interfaces (['eth1', 'eth2'] for example), return the
    subset that contains actually valid entries.
    Interfaces are checked for if they actually exist, and if so, if they're up.
    """
    valid_ifaces = []
    with IPRoute() as ipr:
        for iface in iface_list:
            valid_iface_idx = ipr.link_lookup(ifname=iface)
            if len(valid_iface_idx) == 0:
                continue
            valid_iface_idx = valid_iface_idx[0]
            link_info = ipr.get_links(valid_iface_idx)[0]
            if link_info.get_attr('IFLA_OPERSTATE') == 'UP' \
                    and len(get_iface_addrs(link_info.get_attr('IFLA_ADDRESS'))):
                assert link_info.get_attr('IFLA_IFNAME') == iface
                valid_ifaces.append(iface)
    return valid_ifaces


def get_iface_info(ifname):
    """
    Given an interface name (e.g. 'eth1'), return a dictionary with the
    following keys:
    - ip_addr: Main IPv4 address, if set, or an empty string otherwise.
    - ip_addrs: List of valid IPv4 addresses. Can be an empty list.
    - mac_addr: MAC address

    All values are stored as strings.
    """
    try:
        with IPRoute() as ipr:
            links = ipr.link_lookup(ifname=ifname)
            if len(links) == 0:
                raise LookupError("No interfaces known with name `{}'!"
                                  .format(ifname))
            link_info = ipr.get_links(links)[0]
    except IndexError:
        raise LookupError("Could not get links for interface `{}'"
                          .format(ifname))
    mac_addr = link_info.get_attr('IFLA_ADDRESS')
    ip_addrs = get_iface_addrs(mac_addr)
    return {
        'mac_addr': mac_addr,
        'ip_addr': ip_addrs[0] if ip_addrs else '',
        'ip_addrs': ip_addrs,
    }

def ip_addr_to_iface(ip_addr, iface_list):
    """
    Return an Ethernet interface (e.g. 'eth1') given an IP address.

    Arguments:
    ip_addr -- The IP address as a string
    iface_list -- A map "interface name" -> iface_info, where iface_info
                  is another map as returned by net.get_iface_info().
    """
    # Flip around the iface_info map and then use it to look up by IP addr
    return {
        iface_info['ip_addr']: iface_name
        for iface_name, iface_info in iteritems(iface_list)
    }[ip_addr]


def get_iface_addrs(mac_addr):
    """
    Return a list of IPv4 addresses for a given MAC address.
    If there are no IP addresses assigned to the MAC address, it will return
    an empty list.

    Arguments:
    mac_addr -- A MAC address as a string, input format: "aa:bb:cc:dd:ee:ff"
    """
    with IPRoute() as ip2:
        [link_index] = ip2.link_lookup(address=mac_addr)
        # Only get v4 addresses
        addresses = [addr.get_attrs('IFA_ADDRESS')
                     for addr in ip2.get_addr(family=socket.AF_INET)
                     if addr.get('index', None) == link_index]
        # flatten possibly nested list
        return list(itertools.chain.from_iterable(addresses))


def byte_to_mac(byte_str):
    """
    converts a bytestring into nice hex representation
    """
    return ':'.join(["%02x" % ord(x) for x in byte_str])


def get_mac_addr(remote_addr):
    """
    return MAC address of a remote host already discovered
    or None if no host entry was found
    """
    with IPRoute() as ip2:
        addrs = ip2.get_neighbours(dst=remote_addr)
        if len(addrs) > 1:
            get_logger('get_mac_addr').warning("More than one device with the "
                                               "same IP address found. "
                                               "Picking entry at random")
        if not addrs:
            return None
        return addrs[0].get_attr('NDA_LLADDR')

def get_local_ip_addrs(ipv4_only=False):
    """
    Return a set of IP addresses which are bound to local interfaces.
    """
    with IPDB() as ipdb:
        return {
            ip_subnet[0]
            for ip_subnet_list
                in [x['ipaddr'] for x in ipdb.interfaces.values()]
                    for ip_subnet in ip_subnet_list
            if not ipv4_only or ip_subnet[0].find(':') == -1
        }

