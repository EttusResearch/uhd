#
# Copyright 2010-2013 Ettus Research LLC
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

########################################################################

add_library(usrp3fw STATIC
    udp_uart.c
    wb_uart.c
    wb_i2c.c
    wb_spi.c
    printf.c
    wb_pkt_iface64.c
    u3_net_stack.c
    ethernet.c
    chinch.c
    print_addrs.c
    link_state_route_proto.c
    cron.c
    flash/spi_flash.c
    flash/spif_spsn_s25flxx.c
)
