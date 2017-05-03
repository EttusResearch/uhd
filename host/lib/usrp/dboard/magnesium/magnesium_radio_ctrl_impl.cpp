//
// Copyright 2017 Ettus Research
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

#include "magnesium_radio_ctrl_impl.hpp"

#include <uhd/utils/log.hpp>
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/utils/math.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

static const size_t IO_MASTER_RADIO = 0;

UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(magnesium_radio_ctrl)
{
	std::cout << "magnesium_radio_ctrl_impl::ctor() " << std::endl;
}

magnesium_radio_ctrl_impl::~magnesium_radio_ctrl_impl()
{
}


double magnesium_radio_ctrl_impl::set_rate(double rate)
{
}

void magnesium_radio_ctrl_impl::set_tx_antenna(const std::string &ant, const size_t chan)
{
}

void magnesium_radio_ctrl_impl::set_rx_antenna(const std::string &ant, const size_t chan)
{
}

double magnesium_radio_ctrl_impl::set_tx_frequency(const double freq, const size_t chan)
{
}

double magnesium_radio_ctrl_impl::set_rx_frequency(const double freq, const size_t chan)
{
}

double magnesium_radio_ctrl_impl::set_rx_bandwidth(const double bandwidth, const size_t chan)
{
}

double magnesium_radio_ctrl_impl::get_tx_frequency(const size_t chan)
{
}

double magnesium_radio_ctrl_impl::get_rx_frequency(const size_t chan)
{
}

double magnesium_radio_ctrl_impl::get_rx_bandwidth(const size_t chan)
{
}

double magnesium_radio_ctrl_impl::set_tx_gain(const double gain, const size_t chan)
{
}

double magnesium_radio_ctrl_impl::set_rx_gain(const double gain, const size_t chan)
{
}

size_t magnesium_radio_ctrl_impl::get_chan_from_dboard_fe(const std::string &fe, const direction_t dir)
{
}

std::string magnesium_radio_ctrl_impl::get_dboard_fe_from_chan(const size_t chan, const direction_t dir) {
}

double magnesium_radio_ctrl_impl::get_output_samp_rate(size_t port)
{
}

UHD_RFNOC_BLOCK_REGISTER(magnesium_radio_ctrl, "MagnesiumRadio");
