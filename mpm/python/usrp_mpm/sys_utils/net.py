#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Network utilities for MPM
"""
import itertools
import os
import socket
import threading

import pyudev
from pyroute2 import IPRoute
from six import iteritems
from usrp_mpm.mpmlog import get_logger

# pyudev.Context() forks (find_library -> ldconfig) on creation; doing that on
# every GetDeviceInfo interface enumeration trips gRPC's fork handlers and can
# stall unrelated RPCs. Keep one context per process instead. IPRoute does not
# fork, so it stays per-call (its own netlink socket, no shared state to lock).
_udev_ctx = None
_udev_pid = None


def _get_udev_context():
    """Return the shared pyudev.Context, recreating it if the PID changed.

    The PID guard ensures a child process (e.g. spawned via multiprocessing)
    never reuses a context inherited from its parent.
    """
    global _udev_ctx, _udev_pid
    pid = os.getpid()
    if _udev_ctx is None or _udev_pid != pid:
        _udev_ctx = pyudev.Context()
        _udev_pid = pid
    return _udev_ctx


# Interface-change epoch: increments on rtnetlink address or link events so
# callers can detect stale cached interface information. If the listener cannot
# start, callers fall back to TTL-based caching.
NETLINK_ROUTE = 0
RTMGRP_LINK = 0x1
RTMGRP_IPV4_IFADDR = 0x10

_iface_epoch = 0  # single-writer (the listener greenlet); readers just read it
_iface_monitor_started = False
_iface_monitor_pid = None
_iface_monitor_lock = threading.Lock()


def _netlink_listen_loop(sock):
    """Bump the iface-change epoch on every rtnetlink link/address message."""
    global _iface_epoch
    log = get_logger("net")
    try:
        while True:
            data = sock.recv(65536)
            if not data:
                break
            _iface_epoch += 1
            log.debug(
                "OS net change (rtnetlink); iface-change epoch -> %d", _iface_epoch
            )
    except OSError as ex:
        log.warning("rtnetlink interface-change listener stopped: %s", ex)
    finally:
        try:
            sock.close()
        except OSError:
            pass


def _ensure_iface_monitor():
    """Start the rtnetlink listener once per process (best-effort).

    Failure is non-fatal: we log and leave the epoch fixed, so callers degrade to
    TTL-only cache behavior.
    """
    global _iface_monitor_started, _iface_monitor_pid
    pid = os.getpid()
    if _iface_monitor_started and _iface_monitor_pid == pid:
        return
    with _iface_monitor_lock:
        if _iface_monitor_started and _iface_monitor_pid == pid:
            return
        try:
            import gevent
            from gevent import socket as gevent_socket

            sock = gevent_socket.socket(
                socket.AF_NETLINK, socket.SOCK_RAW, NETLINK_ROUTE
            )
            sock.bind((0, RTMGRP_LINK | RTMGRP_IPV4_IFADDR))
            gevent.spawn(_netlink_listen_loop, sock)
            _iface_monitor_started = True
            _iface_monitor_pid = pid
            get_logger("net").info(
                "Started rtnetlink interface-change monitor (RTMGRP_IPV4_IFADDR|"
                "RTMGRP_LINK; OS-driven xport cache invalidation active)."
            )
        except Exception as ex:  # pylint: disable=broad-except
            get_logger("net").warning(
                "Could not start rtnetlink interface-change monitor: %s; "
                "falling back to TTL-only interface caching.",
                ex,
            )


def get_iface_change_epoch():
    """Return a counter that increments whenever the OS reports a net change.

    Lazily starts the rtnetlink listener on first use. A caller that caches
    interface info can store this value alongside its cache and refresh when the
    epoch changes (in addition to any TTL).
    """
    _ensure_iface_monitor()
    return _iface_epoch


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
    from usrp_mpm import __simulated__

    with IPRoute() as ipr:
        # Dump all links and addresses once and filter in Python, so the netlink
        # round-trips stay constant instead of scaling with len(iface_list).
        links_by_name = {link.get_attr("IFLA_IFNAME"): link for link in ipr.get_links()}
        addrs_by_index = {}
        for addr in ipr.get_addr(family=socket.AF_INET):
            for ip_addr in addr.get_attrs("IFA_ADDRESS"):
                addrs_by_index.setdefault(addr.get("index"), []).append(ip_addr)
        for iface in iface_list:
            link_info = links_by_name.get(iface)
            if link_info is None:
                continue
            # IFLA_OPERSTATE attribute isn't implemented on WSL
            # Workaround is ignore it in the simulator
            if (link_info.get_attr("IFLA_OPERSTATE") == "UP" or __simulated__) and addrs_by_index.get(
                link_info.get("index")
            ):
                assert link_info.get_attr("IFLA_IFNAME") == iface
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

    def is_bridge(link_info_info):
        "Returns True if link_info_ is a bridge"
        return (link_info_info is not None) and (
            link_info_info.get_attr("IFLA_INFO_KIND") == "bridge"
        )

    try:
        with IPRoute() as ipr:
            if getattr(ipr, "filter_messages", None):
                # Pyroute version >= 0.7.0
                link_infos = ipr.filter_messages(
                    lambda x: x.get_attr("IFLA_IFNAME") == ifname, ipr.link("dump")
                )
                if len(link_infos) == 0:
                    raise LookupError("No interfaces known with name `{}'!".format(ifname))
                link_info = link_infos[0]
                link_speed = get_link_speed(ifname)
            else:
                links = ipr.link_lookup(ifname=ifname)
                if not links:
                    raise LookupError(f"No interfaces known with name `{ifname}'!")
                link_list = ipr.get_links(links[0])
                if not link_list:
                    raise LookupError(f"Could not get link info for interface `{ifname}'")
                link_info = link_list[0]
                link_speed = get_link_speed(ifname)
    except IndexError:
        raise LookupError(f"Could not get links for interface `{ifname}'")
    mac_addr = link_info.get_attr("IFLA_ADDRESS")
    ip_addrs = get_iface_addrs(mac_addr)
    return {
        "mac_addr": mac_addr,
        "ip_addr": ip_addrs[0] if ip_addrs else "",
        "ip_addrs": ip_addrs,
        "link_speed": link_speed,
        "bridge": is_bridge(link_info.get_attr("IFLA_LINKINFO")),
        "mtu": link_info.get_attr("IFLA_MTU"),
    }


def get_link_speed(ifname):
    """
    Given an interface name (e.g 'eth0'), return link speed
    of that interface as unsigned integer.
    If interface is not found, IndexError will be thrown.
    The speed is Megabits/sec
    (from kernel at https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net)
    """
    # This wasn't implemented in WSL or in the linux pc I tested it on
    # We will return a sensible default
    from usrp_mpm import __simulated__

    if __simulated__:
        return 1000
    net_sysfs = [
        device
        for device in _get_udev_context().list_devices(subsystem="net")
        if device.sys_name == ifname
    ][0]

    speed = net_sysfs.attributes.asint("speed")
    # FIXME: This sysfs call occasionally returns -1 as the speed if the connection is at all
    #        flaky. Returning 10 Gbs rather than 1 Gbs in this case mitigates negative side
    #        effects in the driver when this occurs on 10GbE ports without breaking mpm
    #        compatability.
    if speed < 0:
        return 10000

    # TODO: 1Gige driver returns a bad value (less than 1000). Remove the conditional once the
    #       driver is fixed
    return speed if speed >= 10000 else 1000


def ip_addr_to_iface(ip_addr, iface_list):
    """
    Return an Ethernet interface (e.g. 'eth1') given an IP address.

    Arguments:
    ip_addr -- The IP address as a string
    iface_list -- A map "interface name" -> iface_info, where iface_info
                  is another map as returned by net.get_iface_info().
    """
    # Flip around the iface_info map and then use it to look up by IP addr
    return {iface_info["ip_addr"]: iface_name for iface_name, iface_info in iteritems(iface_list)}[
        ip_addr
    ]


def get_iface_addrs(mac_addr):
    """
    Return a list of IPv4 addresses for a given MAC address.
    If there are no IP addresses assigned to the MAC address, it will return
    an empty list.

    Arguments:
    mac_addr -- A MAC address as a string, input format: "aa:bb:cc:dd:ee:ff"
    """
    with IPRoute() as ip2:
        links = ip2.link_lookup(address=mac_addr)
        if len(links) > 1:
            raise ValueError(f"found multiple links for MAC {mac_addr}")
        link_index = links[0] if links else None
        # Only get v4 addresses
        addresses = [
            addr.get_attrs("IFA_ADDRESS")
            for addr in ip2.get_addr(family=socket.AF_INET)
            if addr.get("index", None) == link_index
        ]
        # flatten possibly nested list
        return list(itertools.chain.from_iterable(addresses))


def byte_to_mac(byte_str):
    """
    converts a bytestring into nice hex representation
    """
    return ":".join(["%02x" % ord(x) for x in byte_str])


def get_mac_addr(ip_addr):
    """
    return MAC address of a remote host already discovered
    or None if no host entry was found
    """
    with IPRoute() as ip2:

        def _get_local_mac_addr(ip_addr):
            "Lookup MAC addr of local device"
            if_addr = ip2.get_addr(match=lambda x: x.get_attr("IFA_ADDRESS") == ip_addr)
            if not if_addr:
                return None
            if len(if_addr) > 1:
                get_logger("get_mac_addr").warning(
                    f"More than one device with the same IP address `{ip_addr}' found. "
                    "Picking entry at random."
                )
            iface_idx = if_addr[0]["index"]
            link_list = ip2.get_links(iface_idx)
            if not link_list:
                return None
            if_info = link_list[0]
            return if_info.get_attr("IFLA_ADDRESS")

        def _get_remote_mac_addr(remote_addr):
            "Basically an ARP lookup"
            addrs = ip2.get_neighbours(dst=remote_addr)
            if len(addrs) > 1:
                get_logger("get_mac_addr").warning(
                    f"More than one device with the same IP address `{ip_addr}' found. "
                    "Picking entry at random."
                )
            if not addrs:
                return None
            return addrs[0].get_attr("NDA_LLADDR")

        mac_addr = _get_local_mac_addr(ip_addr) or _get_remote_mac_addr(ip_addr)
        ip2.close()
        return mac_addr


def get_local_ip_addrs(ipv4_only=False):
    """
    Return a set of IP addresses which are bound to local interfaces.
    """
    # Use a single IPRoute netlink dump instead of IPDB, which spawns helper
    # threads and builds a full interface/route database on every call
    # (~100s of ms, non-yielding) - that starves other RPCs when GetDeviceInfo
    # is flooded during discovery.
    with IPRoute() as ipr:
        addrs = ipr.get_addr()
    # Prefer IFA_LOCAL, fall back to IFA_ADDRESS, matching how IPDB keyed
    # addresses.
    return {
        ip_addr
        for addr in addrs
        for ip_addr in (addr.get_attrs("IFA_LOCAL") or addr.get_attrs("IFA_ADDRESS"))
        if not ipv4_only or ip_addr.find(":") == -1
    }
