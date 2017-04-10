//
// Copyright 2017 Ettus Research (National Instruments)
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

#pragma once

namespace mpm {
    namespace ad937x {
        namespace gpio {
            enum class gain_pin_t {
                NONE,
                PIN0,
                PIN1,
                PIN2,
                PIN3,
                PIN4,
                PIN5,
                PIN6,
                PIN7,
                PIN8,
                PIN9,
                PIN10,
                PIN11,
                PIN12,
                PIN13,
                PIN14,
                PIN15,
                PIN16,
                PIN17,
                PIN18,
            };

            struct gain_pins_t
            {
                gain_pin_t rx1_inc_gain_pin;
                gain_pin_t rx1_dec_gain_pin;
                gain_pin_t rx2_inc_gain_pin;
                gain_pin_t rx2_dec_gain_pin;
                gain_pin_t tx1_inc_gain_pin;
                gain_pin_t tx1_dec_gain_pin;
                gain_pin_t tx2_inc_gain_pin;
                gain_pin_t tx2_dec_gain_pin;
            };
        }
    }
}