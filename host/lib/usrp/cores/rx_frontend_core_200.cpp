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

/*
    void set_mux(const std::string &mode){
        static const uhd::dict<std::string, boost::uint32_t> mode_to_mux = boost::assign::map_list_of
            ("iq", (0x1 << 4) | (0x0 << 0)) //DAC0Q=DUC0Q, DAC0I=DUC0I
            ("qi", (0x0 << 4) | (0x1 << 0)) //DAC0Q=DUC0I, DAC0I=DUC0Q
            ("i",  (0xf << 4) | (0x0 << 0)) //DAC0Q=ZERO,  DAC0I=DUC0I
            ("q",  (0x0 << 4) | (0xf << 0)) //DAC0Q=DUC0I, DAC0I=ZERO
        ;
        _iface->poke32(, mode_to_mux[mode]);
    }
*/
