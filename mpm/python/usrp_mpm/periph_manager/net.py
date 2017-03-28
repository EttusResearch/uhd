
# Copyright 2017 Ettus Research (National Instruments)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
"""
N310 implementation module
"""
import itertools
import socket
from pyroute2 import IPRoute
from logging import getLogger

LOG = getLogger(__name__)


def get_iface_addrs(mac_addr):
    """
    return ipv4 addresses for a given macaddress
    input format: "aa:bb:cc:dd:ee:ff"
    """
    ip2 = IPRoute()
    # returns index
    [link] = ip2.link_lookup(address=mac_addr)
    # Only get v4 addresses
    addresses = [addr.get_attrs('IFA_ADDRESS')
                 for addr in ip2.get_addr(family=socket.AF_INET)
                 if addr.get('index', None) == link]
    # flatten possibly nested list
    addresses = list(itertools.chain.from_iterable(addresses))
    return addresses


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
    ip2 = IPRoute()
    addrs = ip2.get_neighbours(dst=remote_addr)
    if len(addrs) > 1:
        LOG.warning("More than one device with the same IP address found. Picking entry at random")
    if not addrs:
        return None
    return addrs[0].get_attr('NDA_LLADDR')
