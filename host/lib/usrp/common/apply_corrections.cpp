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

#include "apply_corrections.hpp"
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/csv.hpp>
#include <uhd/types/dict.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/mutex.hpp>
#include <cstdio>
#include <complex>
#include <fstream>

namespace fs = boost::filesystem;

struct tx_fe_cal_t{
    double tx_lo_freq;
    double iq_corr_real;
    double iq_corr_imag;
};

static bool tx_fe_cal_comp(tx_fe_cal_t a, tx_fe_cal_t b){
    return (a.tx_lo_freq < b.tx_lo_freq);
}

boost::mutex corrections_mutex;;
static uhd::dict<std::string, std::vector<tx_fe_cal_t> > cache;

static double linear_interp(double x, double x0, double y0, double x1, double y1){
    return y0 + (x - x0)*(y1 - y0)/(x1 - x0);
}

static std::complex<double> get_tx_fe_iq_correction(
    const std::string &key, const double tx_lo_freq
){
    const std::vector<tx_fe_cal_t> &datas = cache[key];

    //search for lo freq
    size_t lo_index = 0;
    size_t hi_index = datas.size()-1;
    for (size_t i = 0; i < datas.size(); i++){
        if (datas[i].tx_lo_freq > tx_lo_freq){
            hi_index = i;
            break;
        }
        lo_index = i;
    }

    if (lo_index == 0) return std::complex<double>(datas[lo_index].iq_corr_real, datas[lo_index].iq_corr_imag);
    if (hi_index == lo_index) return std::complex<double>(datas[hi_index].iq_corr_real, datas[hi_index].iq_corr_imag);

    //interpolation time
    return std::complex<double>(
        linear_interp(tx_lo_freq, datas[lo_index].tx_lo_freq, datas[lo_index].iq_corr_real, datas[hi_index].tx_lo_freq, datas[hi_index].iq_corr_real),
        linear_interp(tx_lo_freq, datas[lo_index].tx_lo_freq, datas[lo_index].iq_corr_imag, datas[hi_index].tx_lo_freq, datas[hi_index].iq_corr_imag)
    );
}

static void _apply_tx_fe_corrections(
    uhd::property_tree::sptr sub_tree, //starts at mboards/x
    const std::string &slot, //name of dboard slot
    const double tx_lo_freq //actual lo freq
){
    //path constants
    const uhd::fs_path tx_db_path = "dboards/" + slot + "/tx_eeprom";
    const uhd::fs_path tx_fe_path = "tx_frontends/" + slot + "/iq_balance/value";

    //extract eeprom serial
    const uhd::usrp::dboard_eeprom_t db_eeprom = sub_tree->access<uhd::usrp::dboard_eeprom_t>(tx_db_path).get();

    //make the calibration file path
    const fs::path cal_data_path = fs::path(uhd::get_app_path()) / ".uhd" / "cal" / ("tx_fe_cal_v0.1_" + db_eeprom.serial + ".csv");
    if (not fs::exists(cal_data_path)) return;

    //parse csv file or get from cache
    if (not cache.has_key(cal_data_path.string())){
        std::ifstream cal_data(cal_data_path.string().c_str());
        const uhd::csv::rows_type rows = uhd::csv::to_rows(cal_data);

        bool read_data = false, skip_next = false;;
        std::vector<tx_fe_cal_t> datas;
        BOOST_FOREACH(const uhd::csv::row_type &row, rows){
            if (not read_data and not row.empty() and row[0] == "DATA STARTS HERE"){
                read_data = true;
                skip_next = true;
                continue;
            }
            if (not read_data) continue;
            if (skip_next){
                skip_next = false;
                continue;
            }
            tx_fe_cal_t data;
            std::sscanf(row[0].c_str(), "%lf" , &data.tx_lo_freq);
            std::sscanf(row[1].c_str(), "%lf" , &data.iq_corr_real);
            std::sscanf(row[2].c_str(), "%lf" , &data.iq_corr_imag);
            datas.push_back(data);
        }
        std::sort(datas.begin(), datas.end(), tx_fe_cal_comp);
        cache[cal_data_path.string()] = datas;
        UHD_MSG(status) << "Loaded " << cal_data_path.string() << std::endl;

    }

    sub_tree->access<std::complex<double> >(tx_fe_path)
        .set(get_tx_fe_iq_correction(cal_data_path.string(), tx_lo_freq));
}

void uhd::usrp::apply_tx_fe_corrections(
    property_tree::sptr sub_tree, //starts at mboards/x
    const std::string &slot, //name of dboard slot
    const double tx_lo_freq //actual lo freq
){
    boost::mutex::scoped_lock l(corrections_mutex);
    try{
        _apply_tx_fe_corrections(sub_tree, slot, tx_lo_freq);
    }
    catch(const std::exception &e){
        UHD_MSG(error) << "Failure in apply_tx_fe_corrections: " << e.what() << std::endl;
    }
}
