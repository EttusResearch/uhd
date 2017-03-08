//
// Copyright 2017 Ettus Research LLC
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

#include "lmk04828.hpp"
#include "lmk04828_regs.hpp"
#include "uhd/exception.hpp"

using namespace uhd;

static const uint32_t LMK04828_ID_DEVICE_TYPE = 6;
static const uint32_t LMK04828_ID_PROD_LSB    = 91;
static const uint32_t LMK04828_ID_PROD_MSB    = 208;
static const uint32_t LMK04828_ID_MASKREV     = 32;

class lmk04828_impl : public lmk04828_iface
{
public:
    lmk04828_impl(write_fn_t write_fn, read_fn_t read_fn) : _write_fn(write_fn), _read_fn(read_fn)
    {

    }

    ~lmk04828_impl()
    {

    }

    void verify_chip_id()
    {
        // Check ID Device Type, ID Prod, and ID Maskrev registers
        uint8_t id_device_type = _read_fn(3);

        // assert(id_device_type == 6);
        if (id_device_type != 6){
            printf("id_device_type is not 6!");
        }
    }

    uint8_t get_chip_id(){
        uint8_t id_device_type = _read_fn(3);
        return id_device_type;
    }

    void init()
    {
        // Configure the LMK to start producing clocks
        throw new uhd::not_implemented_error("Not needed for MPM bringup");
    }
    
    void send_sysref_pulse()
    {
        // Produce a single sysref pulse
        throw new uhd::not_implemented_error("Not needed for MPM bringup");
    }

private:
    
    lmk04828_regs_t _regs;
    
    write_fn_t _write_fn;
    read_fn_t _read_fn;
};

lmk04828_iface::sptr lmk04828_iface::make(write_fn_t write_fn, read_fn_t read_fn)
{
    return sptr(new lmk04828_impl(write_fn, read_fn));
}
