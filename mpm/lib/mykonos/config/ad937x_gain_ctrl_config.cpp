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

#include "ad937x_gain_ctrl_config.hpp"

using namespace mpm::ad937x::gpio;
using namespace mpm::ad937x::device;
using namespace uhd;

const uint8_t ad937x_gain_ctrl_channel_t::DEFAULT_GAIN_STEP = 1;
const bool ad937x_gain_ctrl_channel_t::DEFAULT_ENABLE = 0;

// rx uses gain, tx uses attenuation
enum class pin_direction_t
{
    INCREASE,
    DECREASE,
};

mykonosGpioSelect_t _convert_gain_pin(gain_pin_t pin)
{
    switch (pin)
    {
    case gain_pin_t::PIN0:  return MYKGPIO0;
    case gain_pin_t::PIN1:  return MYKGPIO1;
    case gain_pin_t::PIN2:  return MYKGPIO2;
    case gain_pin_t::PIN3:  return MYKGPIO3;
    case gain_pin_t::PIN4:  return MYKGPIO4;
    case gain_pin_t::PIN5:  return MYKGPIO5;
    case gain_pin_t::PIN6:  return MYKGPIO6;
    case gain_pin_t::PIN7:  return MYKGPIO7;
    case gain_pin_t::PIN8:  return MYKGPIO8;
    case gain_pin_t::PIN9:  return MYKGPIO9;
    case gain_pin_t::PIN10: return MYKGPIO10;
    case gain_pin_t::PIN11: return MYKGPIO11;
    case gain_pin_t::PIN12: return MYKGPIO12;
    case gain_pin_t::PIN13: return MYKGPIO13;
    case gain_pin_t::PIN14: return MYKGPIO14;
    case gain_pin_t::PIN15: return MYKGPIO15;
    case gain_pin_t::PIN16: return MYKGPIO16;
    case gain_pin_t::PIN17: return MYKGPIO17;
    case gain_pin_t::PIN18: return MYKGPIO18;
    default: return MYKGPIONAN;
    }
}

ad937x_gain_ctrl_channel_t::ad937x_gain_ctrl_channel_t(mykonosGpioSelect_t inc_pin, mykonosGpioSelect_t dec_pin) :
    enable(DEFAULT_ENABLE),
    inc_step(DEFAULT_GAIN_STEP),
    dec_step(DEFAULT_GAIN_STEP),
    inc_pin(inc_pin),
    dec_pin(dec_pin)
{

}

mykonosGpioSelect_t _get_gain_pin(
    direction_t direction,
    chain_t chain,
    pin_direction_t pin_direction,
    const gain_pins_t & gain_pins)
{
    switch (direction)
    {
    case RX_DIRECTION:
        switch (chain)
        {
        case chain_t::ONE:
            switch (pin_direction)
            {
            case pin_direction_t::INCREASE: return _convert_gain_pin(gain_pins.rx1_inc_gain_pin);
            case pin_direction_t::DECREASE: return _convert_gain_pin(gain_pins.rx1_dec_gain_pin);
            }
        case chain_t::TWO:
            switch (pin_direction)
            {
            case pin_direction_t::INCREASE: return _convert_gain_pin(gain_pins.rx2_inc_gain_pin);
            case pin_direction_t::DECREASE: return _convert_gain_pin(gain_pins.rx2_dec_gain_pin);
            }
        }

        // !!! TX is attenuation direction, so the pins are flipped !!!
    case TX_DIRECTION:
        switch (chain)
        {
        case chain_t::ONE:
            switch (pin_direction)
            {
            case pin_direction_t::INCREASE: return _convert_gain_pin(gain_pins.tx1_dec_gain_pin);
            case pin_direction_t::DECREASE: return _convert_gain_pin(gain_pins.tx1_inc_gain_pin);
            }
        case chain_t::TWO:
            switch (pin_direction)
            {
            case pin_direction_t::INCREASE: return _convert_gain_pin(gain_pins.tx2_dec_gain_pin);
            case pin_direction_t::DECREASE: return _convert_gain_pin(gain_pins.tx2_inc_gain_pin);
            }
        }

    default:
        return MYKGPIONAN;
    }
}

ad937x_gain_ctrl_config_t::ad937x_gain_ctrl_config_t(gain_pins_t gain_pins)
{
    config.emplace(std::piecewise_construct, std::forward_as_tuple(RX_DIRECTION), std::forward_as_tuple());
    config.emplace(std::piecewise_construct, std::forward_as_tuple(TX_DIRECTION), std::forward_as_tuple());

    config.at(RX_DIRECTION).emplace(std::piecewise_construct, std::forward_as_tuple(chain_t::ONE),
        std::forward_as_tuple(
        _get_gain_pin(RX_DIRECTION, chain_t::ONE, pin_direction_t::INCREASE, gain_pins),
        _get_gain_pin(RX_DIRECTION, chain_t::ONE, pin_direction_t::DECREASE, gain_pins)));
    config.at(RX_DIRECTION).emplace(std::piecewise_construct, std::forward_as_tuple(chain_t::TWO),
        std::forward_as_tuple(
        _get_gain_pin(RX_DIRECTION, chain_t::TWO, pin_direction_t::INCREASE, gain_pins),
        _get_gain_pin(RX_DIRECTION, chain_t::TWO, pin_direction_t::DECREASE, gain_pins)));

    config.at(TX_DIRECTION).emplace(std::piecewise_construct, std::forward_as_tuple(chain_t::ONE),
        std::forward_as_tuple(
        _get_gain_pin(TX_DIRECTION, chain_t::ONE, pin_direction_t::INCREASE, gain_pins),
        _get_gain_pin(TX_DIRECTION, chain_t::ONE, pin_direction_t::DECREASE, gain_pins)));
    config.at(TX_DIRECTION).emplace(std::piecewise_construct, std::forward_as_tuple(chain_t::TWO),
        std::forward_as_tuple(
        _get_gain_pin(TX_DIRECTION, chain_t::TWO, pin_direction_t::INCREASE, gain_pins),
        _get_gain_pin(TX_DIRECTION, chain_t::TWO, pin_direction_t::DECREASE, gain_pins)));
}

