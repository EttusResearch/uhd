//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "spi_lock.h"
#include "ad937x_ctrl.hpp"
#include <iostream>
#include <sstream>

std::string tune_req(ad937x_ctrl *device, ad937x_ctrl::direction_t direction, double freq)
{
    std::ostringstream ss;
    ss << "Requesting freq " << freq << std::endl;
    double c_freq = device->tune(direction, freq);
    ss << "Got freq        " << c_freq << std::endl;
    return ss.str();
}

std::string gain_req(ad937x_ctrl *device, ad937x_ctrl::direction_t direction, double gain)
{
    std::ostringstream ss;
    ss << "Requesting gain " << gain << std::endl;
    double c_gain = device->set_gain(direction, ad937x_ctrl::CHAIN_1, gain);
    ss << "Got gain        " << c_gain << std::endl;
    return ss.str();
}

void main()
{
    std::cout << "N310 Hardware Daemon Test Main" << std::endl;
    auto spiMon = spi_lock::make(0);

    spi_hwd_settings_t hwd_settings1 = { 0, 0 };
    spi_hwd_settings_t hwd_settings2 = { 0, 1 };
    ad937x_ctrl myk1(spiMon, hwd_settings1), myk2(spiMon, hwd_settings2);

    myk1.set_clock_rate(122800);
    myk2.set_clock_rate(555);
    myk1.set_active_chains(ad937x_ctrl::TX, true, true);
    myk1.set_active_chains(ad937x_ctrl::RX, true, false);
    myk2.set_active_chains(ad937x_ctrl::TX, false, true);
    myk2.set_active_chains(ad937x_ctrl::RX, false, false);

    std::cout << gain_req(&myk1, ad937x_ctrl::TX, 0);
    std::cout << gain_req(&myk1, ad937x_ctrl::TX, -5);
    std::cout << gain_req(&myk1, ad937x_ctrl::TX, 77);
    std::cout << gain_req(&myk1, ad937x_ctrl::RX, 0);
    std::cout << gain_req(&myk1, ad937x_ctrl::RX, -5);
    std::cout << gain_req(&myk1, ad937x_ctrl::RX, 77);

    std::cout << tune_req(&myk1, ad937x_ctrl::TX, 5.6e9);
    std::cout << tune_req(&myk1, ad937x_ctrl::TX, 500e6);
    std::cout << tune_req(&myk1, ad937x_ctrl::TX, 7e9);
    std::cout << tune_req(&myk1, ad937x_ctrl::TX, 150e6);
}

