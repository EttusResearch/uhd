//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <sys/ioctl.h> //ioctl
#include <fcntl.h> //open, close

#include <linux/usrp_e.h>
#include "e100_regs.hpp"

static int fp;

static int peek16(int reg){
    int ret;
    struct usrp_e_ctl16 d;

    d.offset = reg;
    d.count = 1;
    ret = ioctl(fp, USRP_E_READ_CTL16, &d);
    return d.buf[0];
}

static void poke16(int reg, int val){
    int ret;
    struct usrp_e_ctl16 d;

    d.offset = reg;
    d.count = 1;
    d.buf[0] = val;
    ret = ioctl(fp, USRP_E_WRITE_CTL16, &d);
}

static int peek32(int reg){
    int ret;
    struct usrp_e_ctl32 d;

    d.offset = reg;
    d.count = 1;
    ret = ioctl(fp, USRP_E_READ_CTL32, &d);
    return d.buf[0];
}

static void poke32(int reg, int val){
    int ret;
    struct usrp_e_ctl32 d;

    d.offset = reg;
    d.count = 1;
    d.buf[0] = val;
    ret = ioctl(fp, USRP_E_WRITE_CTL32, &d);
}
