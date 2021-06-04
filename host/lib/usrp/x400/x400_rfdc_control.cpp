//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/x400_rfdc_control.hpp>
#include <unordered_map>

using namespace uhd::rfnoc::x400;

rfdc_control::rfdc_control(uhd::memmap32_iface_timed&& iface, const std::string& log_id)
    : _iface(std::move(iface)), _log_id(log_id)
{
    // nop
}

void rfdc_control::reset_ncos(
    const std::vector<rfdc_type>& ncos, const uhd::time_spec_t& time)
{
    if (ncos.empty()) {
        UHD_LOG_WARNING(_log_id,
            "reset_ncos() called with empty NCO list! "
            "Not resetting NCOs.");
        return;
    }
    UHD_LOG_TRACE(_log_id, "Resetting " << ncos.size() << " NCOs...");

    const uint32_t reset_word = 1;
    // TODO: When the FPGA supports it, map the list of ncos into bits onto
    // reset_word to set the bit for the specific NCOs that will be reset.

    _iface.poke32(regmap::NCO_RESET, reset_word, time);
}

void rfdc_control::reset_gearboxes(
    const std::vector<rfdc_type>& gearboxes, const uhd::time_spec_t& time)
{
    if (gearboxes.empty()) {
        UHD_LOG_WARNING(_log_id,
            "reset_gearboxes() called with empty gearbox list! "
            "Not resetting gearboxes.");
        return;
    }
    // This is intentionally at INFO: It should typically happen once per session.
    UHD_LOG_INFO(_log_id, "Resetting " << gearboxes.size() << " gearbox(es)...");
    // TODO: Either the FPGA supports resetting gearboxes individually, or we
    // remove this TODO
    static const std::unordered_map<rfdc_type, const uint32_t> gb_map{
        {rfdc_type::TX0, regmap::DAC_RESET_MSB},
        {rfdc_type::TX1, regmap::DAC_RESET_MSB},
        {rfdc_type::RX0, regmap::ADC_RESET_MSB},
        {rfdc_type::RX1, regmap::ADC_RESET_MSB}};

    uint32_t reset_word = 0;
    for (const auto gb : gearboxes) {
        reset_word = reset_word | (1 << gb_map.at(gb));
    }

    _iface.poke32(regmap::GEARBOX_RESET, reset_word, time);
}

bool rfdc_control::get_nco_reset_done()
{
    return bool(_iface.peek32(regmap::NCO_RESET) & (1 << regmap::NCO_RESET_DONE_MSB));
}

double rfdc_control::set_nco_freq(const rfdc_type, const double freq)
{
    UHD_LOG_WARNING(_log_id, "set_nco_freq() called but not yet implemented!");
    return freq;
}
