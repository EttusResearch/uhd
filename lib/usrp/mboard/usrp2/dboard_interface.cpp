//
// Copyright 2010 Ettus Research LLC
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

#include <uhd/utils.hpp>
#include "dboard_interface.hpp"
#include "fw_common.h"

/***********************************************************************
 * Structors
 **********************************************************************/
dboard_interface::dboard_interface(impl_base *impl){
    _impl = impl;
}

dboard_interface::~dboard_interface(void){
    /* NOP */
}

/***********************************************************************
 * Clock Rates
 **********************************************************************/
double dboard_interface::get_rx_clock_rate(void){
    return _impl->get_master_clock_freq();
}

double dboard_interface::get_tx_clock_rate(void){
    return _impl->get_master_clock_freq();
}

/***********************************************************************
 * GPIO
 **********************************************************************/
/*!
 * Static function to convert a gpio bank enum
 * to an over-the-wire value for the usrp2 control.
 * \param bank the dboard interface gpio bank enum
 * \return an over the wire representation
 */
static uint8_t gpio_bank_to_otw(uhd::usrp::dboard::interface::gpio_bank_t bank){
    switch(bank){
    case uhd::usrp::dboard::interface::GPIO_TX_BANK: return USRP2_GPIO_BANK_TX;
    case uhd::usrp::dboard::interface::GPIO_RX_BANK: return USRP2_GPIO_BANK_RX;
    }
    throw std::runtime_error("unknown gpio bank");
}

void dboard_interface::set_gpio_ddr(gpio_bank_t bank, uint16_t value, uint16_t mask){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_USE_THESE_GPIO_DDR_SETTINGS_BRO);
    out_data.data.gpio_config.bank = gpio_bank_to_otw(bank);
    out_data.data.gpio_config.value = htons(value);
    out_data.data.gpio_config.mask = htons(mask);

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_GOT_THE_GPIO_DDR_SETTINGS_DUDE);
}

void dboard_interface::write_gpio(gpio_bank_t bank, uint16_t value, uint16_t mask){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_SET_YOUR_GPIO_PIN_OUTS_BRO);
    out_data.data.gpio_config.bank = gpio_bank_to_otw(bank);
    out_data.data.gpio_config.value = htons(value);
    out_data.data.gpio_config.mask = htons(mask);

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_I_SET_THE_GPIO_PIN_OUTS_DUDE);
}

uint16_t dboard_interface::read_gpio(gpio_bank_t bank){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_GIVE_ME_YOUR_GPIO_PIN_VALS_BRO);
    out_data.data.gpio_config.bank = gpio_bank_to_otw(bank);

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_HERE_IS_YOUR_GPIO_PIN_VALS_DUDE);
    return ntohs(in_data.data.gpio_config.value);
}

void dboard_interface::set_atr_reg(gpio_bank_t bank, uint16_t tx_value, uint16_t rx_value, uint16_t mask){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_USE_THESE_ATR_SETTINGS_BRO);
    out_data.data.atr_config.bank = gpio_bank_to_otw(bank);
    out_data.data.atr_config.tx_value = htons(tx_value);
    out_data.data.atr_config.rx_value = htons(rx_value);
    out_data.data.atr_config.mask = htons(mask);

    //send and recv
    usrp2_ctrl_data_t in_data = _impl->ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_GOT_THE_ATR_SETTINGS_DUDE);
}
