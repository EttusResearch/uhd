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
#include <iostream>

using namespace uhd;


lmk04828_iface::lmk04828_iface(write_fn_t write_fn, read_fn_t read_fn) : _write_fn(write_fn), _read_fn(read_fn)
{

}

bool lmk04828_iface::verify_chip_id()
{
    // Check ID Device Type, ID Prod, and ID Maskrev registers
    uint8_t id_device_type = get_chip_id();

    std::cout << "LMK device ID: " << int(id_device_type) << std::endl;

    // assert(id_device_type == 6);
    if (id_device_type != 6){
        std::cout << "id_device_type is not 6!" << std::endl;
        return false;
    }

    return true;
}

uint8_t lmk04828_iface::get_chip_id(){
    uint8_t id_device_type = _read_fn(3);
    return id_device_type;
}

void lmk04828_iface::init()
{
    // Configure the LMK to start producing clocks
// TODO: Convert to use ic_reg_map fields once values are finalized/working
    std::vector<uint16_t> write_addrs = {
        0x000000,0x000000,0x000002,0x000149,0x00014A,0x000100,0x000101,0x000103,0x000104,0x000105,0x000106,0x000107,0x000120,0x000121,0x000123,0x000124,0x000125,0x000126,0x000127,0x000130,0x000131,0x000133,0x000134,0x000135,0x000136,0x000137,0x000128,0x000129,0x00012B,0x00012C,0x00012D,0x00012E,0x00012F,0x000108,0x000109,0x00010B,0x00010C,0x00010D,0x00010E,0x00010F,0x000118,0x000119,0x00011B,0x00011C,0x00011D,0x00011E,0x00011F,0x000138,0x00013F,0x000140,0x000144,0x000146,0x000147,0x00014B,0x00014C,0x000153,0x000154,0x000155,0x000156,0x000157,0x000158,0x000159,0x00015A,0x00015B,0x00015E,0x000160,0x000161,0x000162,0x000163,0x000164,0x000165,0x000166,0x000167,0x000168,0x00016E,0x000173,0x000169,0x00016C,0x00016D};

    std::vector<uint8_t> write_data = {
        0x000090,0x000010,0x000000,0x000040,0x000033,0x000078,0x000055,0x000000,0x000020,0x000000,0x0000F1,0x000055,0x000078,0x000055,0x000000,0x000020,0x000000,0x0000F1,0x000055,0x000078,0x000055,0x000000,0x000020,0x000000,0x0000F1,0x000005,0x000078,0x000055,0x000000,0x000000,0x000000,0x0000F0,0x000050,0x00007E,0x000055,0x000000,0x000000,0x000000,0x0000F0,0x000055,0x000078,0x000055,0x000000,0x000020,0x000000,0x0000F1,0x000000,0x000030,0x000009,0x000000,0x000000,0x000010,0x00001A,0x00000D,0x0000F6,0x000000,0x000001,0x000000,0x00000A,0x000000,0x000001,0x000000,0x00007D,0x0000DB,0x000000,0x000000,0x000004,0x0000A0,0x000000,0x000000,0x000019,0x000000,0x000000,0x000019,0x00006B,0x000000,0x000051,0x000000,0x000000};

    std::vector<uint32_t> writes {};
    for (size_t index = 0; index < write_addrs.size(); index++) {
        writes.push_back((write_addrs[index] << 8) | write_data[index]); 
    }

    std::cout << "LMK Initialization writes" << std::endl;
    for (uint32_t reg : writes) {
        std::cout << std::hex << reg << " "; 
    }
    std::cout << std::endl;

    _write_fn(writes);

    if (!verify_chip_id()) {
        throw uhd::runtime_error("LMK ID not correct!");
    }
}

void lmk04828_iface::enable_sysref_pulse()
{
    // Configure the LMK to issue a single SysRef pulse each time SYNC is asserted

    // TODO: Convert to use ic reg map fields once functional
    // Addr 0x139 Value 0x2
    // Addr 0x144 Value 0xFF
    // Addr 0x143 Value 0x52

    std::vector<uint32_t> writes = {
        (0x139 << 11) | 0x2,
        (0x144 << 11) | 0xFF,
        (0x143 << 11) | 0x52
    };

    _write_fn(writes);
}

