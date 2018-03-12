//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
