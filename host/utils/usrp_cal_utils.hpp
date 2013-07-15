//
// Copyright 2011-2012 Ettus Research LLC
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

#include <uhd/utils/paths.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/utils/paths.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <cstdlib>
#include <fstream>

namespace fs = boost::filesystem;

struct result_t{double freq, real_corr, imag_corr, best, delta;};

typedef std::complex<float> samp_type;

/***********************************************************************
 * Constants
 **********************************************************************/
static const double tau = 6.28318531;
static const size_t wave_table_len = 8192;
static const size_t num_search_steps = 5;
static const size_t num_search_iters = 7;
static const double default_freq_step = 7.3e6;
static const size_t default_num_samps = 10000;

/***********************************************************************
 * Set standard defaults for devices
 **********************************************************************/
static inline void set_optimum_defaults(uhd::usrp::multi_usrp::sptr usrp){
    uhd::property_tree::sptr tree = usrp->get_device()->get_tree();

    const uhd::fs_path mb_path = "/mboards/0";
    const std::string mb_name = tree->access<std::string>(mb_path / "name").get();
    if (mb_name.find("USRP2") != std::string::npos or mb_name.find("N200") != std::string::npos or mb_name.find("N210") != std::string::npos){
        usrp->set_tx_rate(12.5e6);
        usrp->set_rx_rate(12.5e6);
    }
    else if (mb_name.find("B100") != std::string::npos){
        usrp->set_tx_rate(4e6);
        usrp->set_rx_rate(4e6);
    }
    else if (mb_name.find("E100") != std::string::npos or mb_name.find("E110") != std::string::npos){
        usrp->set_tx_rate(4e6);
        usrp->set_rx_rate(8e6);
    }
    else{
        throw std::runtime_error("self-calibration is not supported for this hardware");
    }

    const uhd::fs_path tx_fe_path = "/mboards/0/dboards/A/tx_frontends/0";
    const std::string tx_name = tree->access<std::string>(tx_fe_path / "name").get();
    if (tx_name.find("WBX") != std::string::npos){
        usrp->set_tx_gain(0);
    }
    else if (tx_name.find("SBX") != std::string::npos){
        usrp->set_tx_gain(0);
    }
    else if (tx_name.find("CBX") != std::string::npos){
        usrp->set_tx_gain(0);
    }
    else if (tx_name.find("RFX") != std::string::npos){
        usrp->set_tx_gain(0);
    }
    else{
        throw std::runtime_error("self-calibration is not supported for this hardware");
    }

    const uhd::fs_path rx_fe_path = "/mboards/0/dboards/A/rx_frontends/0";
    const std::string rx_name = tree->access<std::string>(rx_fe_path / "name").get();
    if (rx_name.find("WBX") != std::string::npos){
        usrp->set_rx_gain(25);
    }
    else if (rx_name.find("SBX") != std::string::npos){
        usrp->set_rx_gain(25);
    }
    else if (rx_name.find("CBX") != std::string::npos){
        usrp->set_rx_gain(25);
    }
    else if (rx_name.find("RFX") != std::string::npos){
        usrp->set_rx_gain(25);
    }
    else{
        throw std::runtime_error("self-calibration is not supported for this hardware");
    }

}

/***********************************************************************
 * Check for empty serial
 **********************************************************************/

void check_for_empty_serial(
    uhd::usrp::multi_usrp::sptr usrp,
    std::string XX,
    std::string xx,
    std::string uhd_args
){

    //extract eeprom
    uhd::property_tree::sptr tree = usrp->get_device()->get_tree();
    const uhd::fs_path db_path = "/mboards/0/dboards/A/" + xx + "_eeprom";
    const uhd::usrp::dboard_eeprom_t db_eeprom = tree->access<uhd::usrp::dboard_eeprom_t>(db_path).get();

    std::string args_str = "";
    if(uhd_args != "") args_str = str(boost::format(" --args=%s") % uhd_args);

    std::string error_string = str(boost::format("This %s dboard has no serial!\n\nPlease see the Calibration documentation for details on how to fix this.") % XX);

    if (db_eeprom.serial.empty()) throw std::runtime_error(error_string);
}

/***********************************************************************
 * Sinusoid wave table
 **********************************************************************/
class wave_table{
public:
    wave_table(const double ampl){
        _table.resize(wave_table_len);
        for (size_t i = 0; i < wave_table_len; i++){
            _table[i] = samp_type(std::polar(ampl, (tau*i)/wave_table_len));
        }
    }

    inline samp_type operator()(const size_t index) const{
        return _table[index % wave_table_len];
    }

private:
    std::vector<samp_type > _table;
};

/***********************************************************************
 * Compute power of a tone
 **********************************************************************/
static inline double compute_tone_dbrms(
    const std::vector<samp_type > &samples,
    const double freq //freq is fractional
){
    //shift the samples so the tone at freq is down at DC
    //and average the samples to measure the DC component
    samp_type average = 0;
    for (size_t i = 0; i < samples.size(); i++){
        average += samp_type(std::polar(1.0, -freq*tau*i)) * samples[i];
    }

    return 20*std::log10(std::abs(average/float(samples.size())));
}

/***********************************************************************
 * Write a dat file
 **********************************************************************/
static inline void write_samples_to_file(
    const std::vector<samp_type > &samples, const std::string &file
){
    std::ofstream outfile(file.c_str(), std::ofstream::binary);
    outfile.write((const char*)&samples.front(), samples.size()*sizeof(samp_type));
    outfile.close();
}

/***********************************************************************
 * Store data to file
 **********************************************************************/
static void store_results(
    uhd::usrp::multi_usrp::sptr usrp,
    const std::vector<result_t> &results,
    const std::string &XX,
    const std::string &xx,
    const std::string &what
){
    //extract eeprom serial
    uhd::property_tree::sptr tree = usrp->get_device()->get_tree();
    const uhd::fs_path db_path = "/mboards/0/dboards/A/" + xx + "_eeprom";
    const uhd::usrp::dboard_eeprom_t db_eeprom = tree->access<uhd::usrp::dboard_eeprom_t>(db_path).get();

    //make the calibration file path
    fs::path cal_data_path = fs::path(uhd::get_app_path()) / ".uhd";
    fs::create_directory(cal_data_path);
    cal_data_path = cal_data_path / "cal";
    fs::create_directory(cal_data_path);
    cal_data_path = cal_data_path / str(boost::format("%s_%s_cal_v0.2_%s.csv") % xx % what % db_eeprom.serial);
    if (fs::exists(cal_data_path)){
        fs::rename(cal_data_path, cal_data_path.string() + str(boost::format(".%d") % time(NULL)));
    }

    //fill the calibration file
    std::ofstream cal_data(cal_data_path.string().c_str());
    cal_data << boost::format("name, %s Frontend Calibration\n") % XX;
    cal_data << boost::format("serial, %s\n") % db_eeprom.serial;
    cal_data << boost::format("timestamp, %d\n") % time(NULL);
    cal_data << boost::format("version, 0, 1\n");
    cal_data << boost::format("DATA STARTS HERE\n");
    cal_data << "lo_frequency, correction_real, correction_imag, measured, delta\n";

    for (size_t i = 0; i < results.size(); i++){
        cal_data
            << results[i].freq << ", "
            << results[i].real_corr << ", "
            << results[i].imag_corr << ", "
            << results[i].best << ", "
            << results[i].delta << "\n"
        ;
    }

    std::cout << "wrote cal data to " << cal_data_path << std::endl;
}

/***********************************************************************
 * Data capture routine
 **********************************************************************/
static void capture_samples(
    uhd::usrp::multi_usrp::sptr usrp,
    uhd::rx_streamer::sptr rx_stream,
    std::vector<samp_type > &buff,
    const size_t nsamps_requested
){
    buff.resize(nsamps_requested);
    uhd::rx_metadata_t md;

    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = buff.size();
    stream_cmd.stream_now = true;
    usrp->issue_stream_cmd(stream_cmd);
    const size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md);

    //validate the received data
    if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
        throw std::runtime_error(str(boost::format(
            "Unexpected error code 0x%x"
        ) % md.error_code));
    }
    //we can live if all the data didnt come in
    if (num_rx_samps > buff.size()/2){
        buff.resize(num_rx_samps);
        return;
    }
    if (num_rx_samps != buff.size()){
        throw std::runtime_error("did not get all the samples requested");
    }
}
