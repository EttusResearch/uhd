//
// Copyright 2010-2012 Ettus Research LLC
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

// No RX IO Pins Used

// RX IO Functions

#include "max2118_regs.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/math/special_functions/round.hpp>
#include <utility>
#include <cmath>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The DBSRX constants
 **********************************************************************/
static const freq_range_t dbsrx_freq_range(0.8e9, 2.4e9);

//Multiplied by 2.0 for conversion to complex bandpass from lowpass
static const freq_range_t dbsrx_bandwidth_range(2.0*4.0e6, 2.0*33.0e6);

static const freq_range_t dbsrx_pfd_freq_range(0.15e6, 2.01e6);

static const std::vector<std::string> dbsrx_antennas = list_of("J3");

static const uhd::dict<std::string, gain_range_t> dbsrx_gain_ranges = map_list_of
    ("GC1", gain_range_t(0, 56, 0.5))
    ("GC2", gain_range_t(0, 24, 1))
;

static const double usrp1_gpio_clock_rate_limit = 4e6;

/***********************************************************************
 * The DBSRX dboard class
 **********************************************************************/
class dbsrx : public rx_dboard_base{
public:
    dbsrx(ctor_args_t args);
    ~dbsrx(void);

private:
    double _lo_freq;
    double _bandwidth;
    uhd::dict<std::string, double> _gains;
    max2118_write_regs_t _max2118_write_regs;
    max2118_read_regs_t _max2118_read_regs;
    boost::uint8_t _max2118_addr(void){
        return (this->get_iface()->get_special_props().mangle_i2c_addrs)? 0x65 : 0x67;
    };

    double set_lo_freq(double target_freq);
    double set_gain(double gain, const std::string &name);
    double set_bandwidth(double bandwidth);

    void send_reg(boost::uint8_t start_reg, boost::uint8_t stop_reg){
        start_reg = boost::uint8_t(uhd::clip(int(start_reg), 0x0, 0x5));
        stop_reg = boost::uint8_t(uhd::clip(int(stop_reg), 0x0, 0x5));

        for(boost::uint8_t start_addr=start_reg; start_addr <= stop_reg; start_addr += sizeof(boost::uint32_t) - 1){
            int num_bytes = int(stop_reg - start_addr + 1) > int(sizeof(boost::uint32_t)) - 1 ? sizeof(boost::uint32_t) - 1 : stop_reg - start_addr + 1;

            //create buffer for register data (+1 for start address)
            byte_vector_t regs_vector(num_bytes + 1);

            //first byte is the address of first register
            regs_vector[0] = start_addr;

            //get the register data
            for(int i=0; i<num_bytes; i++){
                regs_vector[1+i] = _max2118_write_regs.get_reg(start_addr+i);
                UHD_LOGV(often) << boost::format(
                    "DBSRX: send reg 0x%02x, value 0x%04x, start_addr = 0x%04x, num_bytes %d"
                ) % int(start_addr+i) % int(regs_vector[1+i]) % int(start_addr) % num_bytes << std::endl;
            }

            //send the data
            this->get_iface()->write_i2c(
                _max2118_addr(), regs_vector
            );
        }
    }

    void read_reg(boost::uint8_t start_reg, boost::uint8_t stop_reg){
        static const boost::uint8_t status_addr = 0x0;
        start_reg = boost::uint8_t(uhd::clip(int(start_reg), 0x0, 0x1));
        stop_reg = boost::uint8_t(uhd::clip(int(stop_reg), 0x0, 0x1));

        for(boost::uint8_t start_addr=start_reg; start_addr <= stop_reg; start_addr += sizeof(boost::uint32_t)){
            int num_bytes = int(stop_reg - start_addr + 1) > int(sizeof(boost::uint32_t)) ? sizeof(boost::uint32_t) : stop_reg - start_addr + 1;

            //create buffer for register data
            byte_vector_t regs_vector(num_bytes);

            //read from i2c
            regs_vector = this->get_iface()->read_i2c(
                _max2118_addr(), num_bytes
            );

            for(boost::uint8_t i=0; i < num_bytes; i++){
                if (i + start_addr >= status_addr){
                    _max2118_read_regs.set_reg(i + start_addr, regs_vector[i]);
                }
                UHD_LOGV(often) << boost::format(
                    "DBSRX: read reg 0x%02x, value 0x%04x, start_addr = 0x%04x, num_bytes %d"
                ) % int(start_addr+i) % int(regs_vector[i]) % int(start_addr) % num_bytes << std::endl;
            }
        }
    }

    /*!
     * Get the lock detect status of the LO.
     * \return sensor for locked
     */
    sensor_value_t get_locked(void){
        read_reg(0x0, 0x0);

        //mask and return lock detect
        bool locked = 5 >= _max2118_read_regs.adc and _max2118_read_regs.adc >= 2;

        UHD_LOGV(often) << boost::format(
            "DBSRX: locked %d"
        ) % locked << std::endl;

        return sensor_value_t("LO", locked, "locked", "unlocked");
    }
};

/***********************************************************************
 * Register the DBSRX dboard
 **********************************************************************/
static dboard_base::sptr make_dbsrx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new dbsrx(args));
}

UHD_STATIC_BLOCK(reg_dbsrx_dboard){
    //register the factory function for the rx dbid (others version)
    dboard_manager::register_dboard(0x000D, &make_dbsrx, "DBSRX");
    //register the factory function for the rx dbid (USRP1 version)
    dboard_manager::register_dboard(0x0002, &make_dbsrx, "DBSRX");
}

/***********************************************************************
 * Structors
 **********************************************************************/
dbsrx::dbsrx(ctor_args_t args) : rx_dboard_base(args){
    //warn user about incorrect DBID on USRP1, requires R193 populated
    if (this->get_iface()->get_special_props().soft_clock_divider and this->get_rx_id() == 0x000D)
        UHD_MSG(warning) << boost::format(
                "DBSRX: incorrect dbid\n"
                "Expected dbid 0x0002 and R193\n"
                "found dbid == %d\n"
                "Please see the daughterboard app notes" 
                ) % this->get_rx_id().to_pp_string();

    //warn user about incorrect DBID on non-USRP1, requires R194 populated
    if (not this->get_iface()->get_special_props().soft_clock_divider and this->get_rx_id() == 0x0002)
        UHD_MSG(warning) << boost::format(
                "DBSRX: incorrect dbid\n"
                "Expected dbid 0x000D and R194\n"
                "found dbid == %d\n"
                "Please see the daughterboard app notes" 
                ) % this->get_rx_id().to_pp_string();

    //send initial register settings
    this->send_reg(0x0, 0x5);

    //set defaults for LO, gains, and filter bandwidth
    double codec_rate = this->get_iface()->get_codec_rate(dboard_iface::UNIT_RX);
    _bandwidth = 0.8*codec_rate/2.0; // default to anti-alias at different codec_rate

    ////////////////////////////////////////////////////////////////////
    // Register properties
    ////////////////////////////////////////////////////////////////////
    this->get_rx_subtree()->create<std::string>("name")
        .set("DBSRX");
    this->get_rx_subtree()->create<sensor_value_t>("sensors/lo_locked")
        .publish(boost::bind(&dbsrx::get_locked, this));
    BOOST_FOREACH(const std::string &name, dbsrx_gain_ranges.keys()){
        this->get_rx_subtree()->create<double>("gains/"+name+"/value")
            .coerce(boost::bind(&dbsrx::set_gain, this, _1, name))
            .set(dbsrx_gain_ranges[name].start());
        this->get_rx_subtree()->create<meta_range_t>("gains/"+name+"/range")
            .set(dbsrx_gain_ranges[name]);
    }
    this->get_rx_subtree()->create<double>("freq/value")
        .coerce(boost::bind(&dbsrx::set_lo_freq, this, _1));
    this->get_rx_subtree()->create<meta_range_t>("freq/range")
        .set(dbsrx_freq_range);
    this->get_rx_subtree()->create<std::string>("antenna/value")
        .set(dbsrx_antennas.at(0));
    this->get_rx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(dbsrx_antennas);
    this->get_rx_subtree()->create<std::string>("connection")
        .set("IQ");
    this->get_rx_subtree()->create<bool>("enabled")
        .set(true); //always enabled
    this->get_rx_subtree()->create<bool>("use_lo_offset")
        .set(false);
    this->get_rx_subtree()->create<double>("bandwidth/value")
        .coerce(boost::bind(&dbsrx::set_bandwidth, this, _1));
    this->get_rx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(dbsrx_bandwidth_range);

    //enable only the clocks we need
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    //set the gpio directions and atr controls (identically)
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, 0x0); // All unused in atr
    if (this->get_iface()->get_special_props().soft_clock_divider){
        this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, 0x1); // GPIO0 is clock when on USRP1
    }
    else{
        this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, 0x0); // All Inputs
    }

    //now its safe to set inital freq and bw
    this->get_rx_subtree()->access<double>("freq/value")
        .set(dbsrx_freq_range.start());
    this->get_rx_subtree()->access<double>("bandwidth/value")
        .set(2.0*_bandwidth); //_bandwidth in lowpass, convert to complex bandpass
}

dbsrx::~dbsrx(void){
}


/***********************************************************************
 * Tuning
 **********************************************************************/
double dbsrx::set_lo_freq(double target_freq){
    target_freq = dbsrx_freq_range.clip(target_freq);

    double actual_freq=0.0, pfd_freq=0.0, ref_clock=0.0;
    int R=0, N=0, r=0, m=0;
    bool update_filter_settings = false;
    //choose refclock
    std::vector<double> clock_rates = this->get_iface()->get_clock_rates(dboard_iface::UNIT_RX);
    const double max_clock_rate = uhd::sorted(clock_rates).back();
    BOOST_FOREACH(ref_clock, uhd::reversed(uhd::sorted(clock_rates))){
        //USRP1 feeds the DBSRX clock from a FPGA GPIO line.
        //make sure that this clock does not exceed rate limit.
        if (this->get_iface()->get_special_props().soft_clock_divider){
            if (ref_clock > usrp1_gpio_clock_rate_limit) continue;
        }
        if (ref_clock > 27.0e6) continue;
        if (size_t(max_clock_rate/ref_clock)%2 == 1) continue; //reject asymmetric clocks (odd divisors)

        //choose m_divider such that filter tuning constraint is met
        m = 31;
        while ((ref_clock/m < 1e6 or ref_clock/m > 2.5e6) and m > 0){ m--; }

        UHD_LOGV(often) << boost::format(
            "DBSRX: trying ref_clock %f and m_divider %d"
        ) % (ref_clock) % m << std::endl;

        if (m >= 32) continue;

        //choose R
        for(r = 0; r <= 6; r += 1) {
            //compute divider from setting
            R = 1 << (r+1);
            UHD_LOGV(often) << boost::format("DBSRX R:%d\n") % R << std::endl;

            //compute PFD compare frequency = ref_clock/R
            pfd_freq = ref_clock / R;

            //constrain the PFD frequency to specified range
            if ((pfd_freq < dbsrx_pfd_freq_range.start()) or (pfd_freq > dbsrx_pfd_freq_range.stop())) continue;

            //compute N
            N = int(std::floor(target_freq/pfd_freq));

            //constrain N to specified range
            if ((N < 256) or (N > 32768)) continue;

            goto done_loop;
        }
    } 

    done_loop:

    //Assert because we failed to find a suitable combination of ref_clock, R and N 
    UHD_ASSERT_THROW(ref_clock <= 27.0e6 and ref_clock >= 0.0);
    UHD_ASSERT_THROW(ref_clock/m >= 1e6 and ref_clock/m <= 2.5e6);
    UHD_ASSERT_THROW((pfd_freq >= dbsrx_pfd_freq_range.start()) and (pfd_freq <= dbsrx_pfd_freq_range.stop()));
    UHD_ASSERT_THROW((N >= 256) and (N <= 32768));

    UHD_LOGV(often) << boost::format(
        "DBSRX: choose ref_clock (current: %f, new: %f) and m_divider %d"
    ) % (this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX)) % ref_clock % m << std::endl;

    //if ref_clock or m divider changed, we need to update the filter settings
    if (ref_clock != this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX) or m != _max2118_write_regs.m_divider) update_filter_settings = true;

    //compute resulting output frequency
    actual_freq = pfd_freq * N;

    //apply ref_clock, R, and N settings
    this->get_iface()->set_clock_rate(dboard_iface::UNIT_RX, ref_clock);
    ref_clock = this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX);
    _max2118_write_regs.m_divider = m;
    _max2118_write_regs.r_divider = (max2118_write_regs_t::r_divider_t) r;
    _max2118_write_regs.set_n_divider(N);
    _max2118_write_regs.ade_vco_ade_read = max2118_write_regs_t::ADE_VCO_ADE_READ_ENABLED;
    
    //compute prescaler variables
    int scaler = actual_freq > 1125e6 ? 2 : 4;
    _max2118_write_regs.div2 = scaler == 4 ? max2118_write_regs_t::DIV2_DIV4 : max2118_write_regs_t::DIV2_DIV2;

    UHD_LOGV(often) << boost::format(
        "DBSRX: scaler %d, actual_freq %f MHz, register bit: %d"
    ) % scaler % (actual_freq/1e6) % int(_max2118_write_regs.div2) << std::endl;

    //compute vco frequency and select vco
    double vco_freq = actual_freq * scaler;
    if (vco_freq < 2433e6)
        _max2118_write_regs.osc_band = 0;
    else if (vco_freq < 2711e6)
        _max2118_write_regs.osc_band = 1;
    else if (vco_freq < 3025e6)
        _max2118_write_regs.osc_band = 2;
    else if (vco_freq < 3341e6)
        _max2118_write_regs.osc_band = 3;
    else if (vco_freq < 3727e6)
        _max2118_write_regs.osc_band = 4;
    else if (vco_freq < 4143e6)
        _max2118_write_regs.osc_band = 5;
    else if (vco_freq < 4493e6)
        _max2118_write_regs.osc_band = 6;
    else
        _max2118_write_regs.osc_band = 7;

    //send settings over i2c
    send_reg(0x0, 0x4);

    //check vtune for lock condition
    read_reg(0x0, 0x0);

    UHD_LOGV(often) << boost::format(
        "DBSRX: initial guess for vco %d, vtune adc %d"
    ) % int(_max2118_write_regs.osc_band) % int(_max2118_read_regs.adc) << std::endl;

    //if we are out of lock for chosen vco, change vco
    while ((_max2118_read_regs.adc == 0) or (_max2118_read_regs.adc == 7)){

        //vtune is too low, try lower frequency vco
        if (_max2118_read_regs.adc == 0){
            if (_max2118_write_regs.osc_band == 0){
                UHD_MSG(warning) << boost::format(
                        "DBSRX: Tuning exceeded vco range, _max2118_write_regs.osc_band == %d\n" 
                        ) % int(_max2118_write_regs.osc_band);
                UHD_ASSERT_THROW(_max2118_read_regs.adc != 0); //just to cause a throw
            }
            if (_max2118_write_regs.osc_band <= 0) break;
            _max2118_write_regs.osc_band -= 1;
        }

        //vtune is too high, try higher frequency vco
        if (_max2118_read_regs.adc == 7){
            if (_max2118_write_regs.osc_band == 7){
                UHD_MSG(warning) << boost::format(
                        "DBSRX: Tuning exceeded vco range, _max2118_write_regs.osc_band == %d\n" 
                        ) % int(_max2118_write_regs.osc_band);
                UHD_ASSERT_THROW(_max2118_read_regs.adc != 7); //just to cause a throw
            }
            if (_max2118_write_regs.osc_band >= 7) break;
            _max2118_write_regs.osc_band += 1;
        }

        UHD_LOGV(often) << boost::format(
            "DBSRX: trying vco %d, vtune adc %d"
        ) % int(_max2118_write_regs.osc_band) % int(_max2118_read_regs.adc) << std::endl;

        //update vco selection and check vtune
        send_reg(0x2, 0x2);
        read_reg(0x0, 0x0);

        //allow for setup time before checking condition again
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }
      
    UHD_LOGV(often) << boost::format(
        "DBSRX: final vco %d, vtune adc %d"
    ) % int(_max2118_write_regs.osc_band) % int(_max2118_read_regs.adc) << std::endl;

    //select charge pump bias current
    if (_max2118_read_regs.adc <= 2) _max2118_write_regs.cp_current = max2118_write_regs_t::CP_CURRENT_I_CP_100UA;
    else if (_max2118_read_regs.adc >= 5) _max2118_write_regs.cp_current = max2118_write_regs_t::CP_CURRENT_I_CP_400UA;
    else _max2118_write_regs.cp_current = max2118_write_regs_t::CP_CURRENT_I_CP_200UA;
    
    //update charge pump bias current setting
    send_reg(0x2, 0x2);

    //compute actual tuned frequency
    _lo_freq = this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX) / std::pow(2.0,(1 + _max2118_write_regs.r_divider)) * _max2118_write_regs.get_n_divider();

    //debug output of calculated variables
    UHD_LOGV(often)
        << boost::format("DBSRX tune:\n")
        << boost::format("    VCO=%d, CP=%d, PFD Freq=%fMHz\n") % int(_max2118_write_regs.osc_band) % _max2118_write_regs.cp_current % (pfd_freq/1e6)
        << boost::format("    R=%d, N=%f, scaler=%d, div2=%d\n") % R % N % scaler % int(_max2118_write_regs.div2)
        << boost::format("    Ref    Freq=%fMHz\n") % (ref_clock/1e6)
        << boost::format("    Target Freq=%fMHz\n") % (target_freq/1e6)
        << boost::format("    Actual Freq=%fMHz\n") % (_lo_freq/1e6)
        << boost::format("    VCO    Freq=%fMHz\n") % (vco_freq/1e6)
        << std::endl;

    if (update_filter_settings) set_bandwidth(_bandwidth);
    get_locked();

    return _lo_freq;
}

/***********************************************************************
 * Gain Handling
 **********************************************************************/
/*!
 * Convert a requested gain for the GC2 vga into the integer register value.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return 5 bit the register value
 */
static int gain_to_gc2_vga_reg(double &gain){
    int reg = 0;
    gain = dbsrx_gain_ranges["GC2"].clip(gain);

    // Half dB steps from 0-5dB, 1dB steps from 5-24dB
    if (gain < 5) {
        reg = boost::math::iround(31.0 - gain/0.5);
        gain = double(boost::math::iround(gain) * 0.5);
    } else {
        reg = boost::math::iround(22.0 - (gain - 4.0));
        gain = double(boost::math::iround(gain));
    }

    UHD_LOGV(often) << boost::format(
        "DBSRX GC2 Gain: %f dB, reg: %d"
    ) % gain % reg << std::endl;

    return reg;
}

/*!
 * Convert a requested gain for the GC1 rf vga into the dac_volts value.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return dac voltage value
 */
static double gain_to_gc1_rfvga_dac(double &gain){
    //clip the input
    gain = dbsrx_gain_ranges["GC1"].clip(gain);

    //voltage level constants
    static const double max_volts = 1.2, min_volts = 2.7;
    static const double slope = (max_volts-min_volts)/dbsrx_gain_ranges["GC1"].stop();

    //calculate the voltage for the aux dac
    double dac_volts = gain*slope + min_volts;

    UHD_LOGV(often) << boost::format(
        "DBSRX GC1 Gain: %f dB, dac_volts: %f V"
    ) % gain % dac_volts << std::endl;

    //the actual gain setting
    gain = (dac_volts - min_volts)/slope;

    return dac_volts;
}

double dbsrx::set_gain(double gain, const std::string &name){
    assert_has(dbsrx_gain_ranges.keys(), name, "dbsrx gain name");
    if (name == "GC2"){
        _max2118_write_regs.gc2 = gain_to_gc2_vga_reg(gain);
        send_reg(0x5, 0x5);
    }
    else if(name == "GC1"){
        //write the new voltage to the aux dac
        this->get_iface()->write_aux_dac(dboard_iface::UNIT_RX, dboard_iface::AUX_DAC_A, gain_to_gc1_rfvga_dac(gain));
    }
    else UHD_THROW_INVALID_CODE_PATH();
    _gains[name] = gain;

    return gain;
}

/***********************************************************************
 * Bandwidth Handling
 **********************************************************************/
double dbsrx::set_bandwidth(double bandwidth){
    //convert complex bandpass to lowpass bandwidth
    bandwidth = bandwidth/2.0;

    //clip the input
    bandwidth = dbsrx_bandwidth_range.clip(bandwidth);

    double ref_clock = this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX);
    
    //NOTE: _max2118_write_regs.m_divider set in set_lo_freq

    //compute f_dac setting
    _max2118_write_regs.f_dac = uhd::clip<int>(int((((bandwidth*_max2118_write_regs.m_divider)/ref_clock) - 4)/0.145),0,127);

    //determine actual bandwidth
    _bandwidth = double((ref_clock/(_max2118_write_regs.m_divider))*(4+0.145*_max2118_write_regs.f_dac));

    UHD_LOGV(often) << boost::format(
        "DBSRX Filter Bandwidth: %f MHz, m: %d, f_dac: %d\n"
    ) % (_bandwidth/1e6) % int(_max2118_write_regs.m_divider) % int(_max2118_write_regs.f_dac) << std::endl;

    this->send_reg(0x3, 0x4);

    //convert lowpass back to complex bandpass bandwidth
    return 2.0*_bandwidth;
}
