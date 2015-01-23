//
// Copyright 2014-15 Ettus Research LLC
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

/***********************************************************************
 * Included Files and Libraries
 **********************************************************************/
#include <uhd/types/device_addr.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <map>

using namespace uhd;
using namespace uhd::usrp;

#define fMHz (1000000.0)
#define UBX_PROTO_V3_TX_ID   0x73
#define UBX_PROTO_V3_RX_ID   0x74
#define UBX_PROTO_V4_TX_ID   0x75
#define UBX_PROTO_V4_RX_ID   0x76
#define UBX_V1_40MHZ_TX_ID   0x77
#define UBX_V1_40MHZ_RX_ID   0x78
#define UBX_V1_160MHZ_TX_ID  0x79
#define UBX_V1_160MHZ_RX_ID  0x7a

/***********************************************************************
 * UBX Synthesizers
 **********************************************************************/
#include "max2870_regs.hpp"
#include "max2871_regs.hpp"

typedef boost::function<void(std::vector<boost::uint32_t>)> max287x_write_fn;

class max287x_synthesizer_iface
{
public:
    virtual bool is_shutdown(void) = 0;
    virtual void shutdown(void) = 0;
    virtual void power_up(void) = 0;
    virtual double set_freq_and_power(double target_freq, double ref_freq, bool is_int_n, int output_power) = 0;
};

class max287x : public max287x_synthesizer_iface
{
public:
    max287x(max287x_write_fn write_fn) : _write_fn(write_fn) {};
    virtual ~max287x() {};

protected:
    virtual std::set<boost::uint32_t> get_changed_addrs(void) = 0;
    virtual boost::uint32_t get_reg(boost::uint32_t addr) = 0;
    virtual void save_state(void) = 0;

    void write_regs(void)
    {
        std::vector<boost::uint32_t> regs;
        std::set<boost::uint32_t> changed_regs;

        // Get only regs with changes
        try {
            changed_regs = get_changed_addrs();
        } catch (uhd::runtime_error& e) {
            // No saved state - write all regs
            for (int addr = 5; addr >= 0; addr--)
                changed_regs.insert(boost::uint32_t(addr));
        }

        for (int addr = 5; addr >= 0; addr--)
        {
            if (changed_regs.find(boost::uint32_t(addr)) != changed_regs.end())
                regs.push_back(get_reg(boost::uint32_t(addr)));
        }

        // writing reg 0 initiates VCO auto select, so this makes sure it is written
        if (changed_regs.size() and changed_regs.find(0) == changed_regs.end())
            regs.push_back(get_reg(0));

        _write_fn(regs);
        save_state();
    }

    double calculate_freq_settings(
        double target_freq,
        double ref_freq,
        double target_pfd_freq,
        bool is_int_n,
        double &pfd_freq,
        int& T,
        int& D,
        int& R,
        int& BS,
        int& N,
        int& FRAC,
        int& MOD,
        int& RFdiv)
    {
        //map mode setting to valid integer divider (N) values
        static const uhd::range_t int_n_mode_div_range(16,4095,1);
        static const uhd::range_t frac_n_mode_div_range(19,4091,1);

        double actual_freq = 0.0;

        T = 0;
        D = ref_freq <= 10.0e6 ? 1 : 0;
        R = 0;
        BS = 0;
        N = 0;
        FRAC = 0;
        MOD = 4095;
        RFdiv = 1;

        //increase RF divider until acceptable VCO frequency (MIN freq for MAX287x VCO is 3GHz)
        double vco_freq = target_freq;
        while (vco_freq < 3e9)
        {
            vco_freq *= 2;
            RFdiv *= 2;
        }

        /*
         * The goal here is to loop though possible R dividers,
         * band select clock dividers, N (int) dividers, and FRAC
         * (frac) dividers.
         *
         * Calculate the N and F dividers for each set of values.
         * The loop exits when it meets all of the constraints.
         * The resulting loop values are loaded into the registers.
         *
         * f_pfd = f_ref*(1+D)/(R*(1+T))
         * f_vco = (N + (FRAC/MOD))*f_pfd
         *     N = f_vco/f_pfd - FRAC/MOD = f_vco*((R*(T+1))/(f_ref*(1+D))) - FRAC/MOD
         * f_rf  = f_vco/RFdiv
         */
        for(R = int(ref_freq*(1+D)/(target_pfd_freq*(1+T))); R <= 1023; R++)
        {
            //PFD input frequency = f_ref/R ... ignoring Reference doubler/divide-by-2 (D & T)
            pfd_freq = ref_freq*(1+D)/(R*(1+T));

            //keep the PFD frequency at or below target
            if (pfd_freq > target_pfd_freq)
                continue;

            //ignore fractional part of tuning
            N = int(vco_freq/pfd_freq);

            //Fractional-N calculation
            FRAC = int(boost::math::round((vco_freq/pfd_freq - N)*MOD));

            if(is_int_n)
            {
                if (FRAC > (MOD / 2)) //Round integer such that actual freq is closest to target
                    N++;
                FRAC = 0;
            }

            //keep N within int divider requirements
            if(is_int_n)
            {
                if(N < int_n_mode_div_range.start()) continue;
                if(N > int_n_mode_div_range.stop()) continue;
            }
            else
            {
                if(N < frac_n_mode_div_range.start()) continue;
                if(N > frac_n_mode_div_range.stop()) continue;
            }

            //keep pfd freq low enough to achieve 50kHz BS clock
            BS = std::ceil(pfd_freq / 50e3);
            if(BS <= 1023) break;
        }
        UHD_ASSERT_THROW(R <= 1023);

        //Reference divide-by-2 for 50% duty cycle
        // if R even, move one divide by 2 to to regs.reference_divide_by_2
        if(R % 2 == 0)
        {
            T = 1;
            R /= 2;
        }

        //actual frequency calculation
        actual_freq = double((N + (double(FRAC)/double(MOD)))*ref_freq*(1+int(D))/(R*(1+int(T)))/RFdiv);

        UHD_LOGV(rarely)
            << boost::format("MAX287x: Intermediates: ref=%0.2f, outdiv=%f, fbdiv=%f"
                ) % (ref_freq*(1+int(D))/(R*(1+int(T)))) % double(RFdiv*2) % double(N + double(FRAC)/double(MOD)) << std::endl
            << boost::format("MAX287x: tune: R=%d, BS=%d, N=%d, FRAC=%d, MOD=%d, T=%d, D=%d, RFdiv=%d, type=%s"
                ) % R % BS % N % FRAC % MOD % T % D % RFdiv % ((is_int_n) ? "Integer-N" : "Fractional") << std::endl
            << boost::format("MAX287x: Frequencies (MHz): REQ=%0.2f, ACT=%0.2f, VCO=%0.2f, PFD=%0.2f, BAND=%0.2f"
                ) % (pfd_freq/1e6) % (actual_freq/1e6) % (vco_freq/1e6) % (pfd_freq/1e6) % (pfd_freq/BS/1e6) << std::endl;

        return actual_freq;
    }

    max287x_write_fn _write_fn;
};

class max2870 : public max287x
{
public:
    max2870(max287x_write_fn write_fn) : max287x(write_fn), _first_tune(true)
    {
        // initialize register values (override defaults)
        _regs.retune = max2870_regs_t::RETUNE_DISABLED;
        _regs.clock_div_mode = max2870_regs_t::CLOCK_DIV_MODE_FAST_LOCK;

        // MAX2870 data sheet says that all registers must be written twice
        // with at least a 20ms delay between writes upon power up.  One
        // write and a 20ms wait are done in power_up().  The second write
        // is done when any other function that does a write to the registers
        // is called.  To ensure all registers are written the second time, the
        // state of the registers is not saved during the first write.
        _save_state = false;
        power_up();
        _save_state = true;
    };

    ~max2870()
    {
        shutdown();
    };

    bool is_shutdown(void)
    {
        return (_regs.power_down == max2870_regs_t::POWER_DOWN_SHUTDOWN);
    };

    void shutdown(void)
    {
        _regs.rf_output_enable = max2870_regs_t::RF_OUTPUT_ENABLE_DISABLED;
        _regs.aux_output_enable = max2870_regs_t::AUX_OUTPUT_ENABLE_DISABLED;
        _regs.power_down = max2870_regs_t::POWER_DOWN_SHUTDOWN;
        _regs.muxout = max2870_regs_t::MUXOUT_LOW;
        _regs.ld_pin_mode = max2870_regs_t::LD_PIN_MODE_LOW;
        write_regs();
    };

    void power_up(void)
    {
        _regs.muxout = max2870_regs_t::MUXOUT_DLD;
        _regs.ld_pin_mode = max2870_regs_t::LD_PIN_MODE_DLD;
        _regs.power_down = max2870_regs_t::POWER_DOWN_NORMAL;
        write_regs();

        // MAX270 data sheet says to wait at least 20 ms after exiting low power mode
        // before programming final VCO frequency
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));

        _first_tune = true;
    };

    double set_freq_and_power(double target_freq, double ref_freq, bool is_int_n, int output_power)
    {
        //map rf divider select output dividers to enums
        static const uhd::dict<int, max2870_regs_t::rf_divider_select_t> rfdivsel_to_enum =
            boost::assign::map_list_of
            (1,   max2870_regs_t::RF_DIVIDER_SELECT_DIV1)
            (2,   max2870_regs_t::RF_DIVIDER_SELECT_DIV2)
            (4,   max2870_regs_t::RF_DIVIDER_SELECT_DIV4)
            (8,   max2870_regs_t::RF_DIVIDER_SELECT_DIV8)
            (16,  max2870_regs_t::RF_DIVIDER_SELECT_DIV16)
            (32,  max2870_regs_t::RF_DIVIDER_SELECT_DIV32)
            (64,  max2870_regs_t::RF_DIVIDER_SELECT_DIV64)
            (128, max2870_regs_t::RF_DIVIDER_SELECT_DIV128);

        int T = 0;
        int D = ref_freq <= 10.0e6 ? 1 : 0;
        int R, BS, N, FRAC, MOD, RFdiv;
        double pfd_freq = 25e6;

        double actual_freq = calculate_freq_settings(
            target_freq, ref_freq, 25e6, is_int_n, pfd_freq, T, D, R, BS, N, FRAC, MOD, RFdiv);

        //load the register values
        _regs.rf_output_enable = max2870_regs_t::RF_OUTPUT_ENABLE_ENABLED;

        if(is_int_n) {
            _regs.cpl = max2870_regs_t::CPL_DISABLED;
            _regs.ldf = max2870_regs_t::LDF_INT_N;
            _regs.cpoc = max2870_regs_t::CPOC_ENABLED;
            _regs.int_n_mode = max2870_regs_t::INT_N_MODE_INT_N;
        } else {
            _regs.cpl = max2870_regs_t::CPL_ENABLED;
            _regs.ldf = max2870_regs_t::LDF_FRAC_N;
            _regs.cpoc = max2870_regs_t::CPOC_DISABLED;
            _regs.int_n_mode = max2870_regs_t::INT_N_MODE_FRAC_N;
        }

        _regs.lds = pfd_freq <= 32e6 ? max2870_regs_t::LDS_SLOW : max2870_regs_t::LDS_FAST;

        _regs.frac_12_bit = FRAC;
        _regs.int_16_bit = N;
        _regs.mod_12_bit = MOD;
        _regs.clock_divider_12_bit = std::max(1, int(std::ceil(400e-6*pfd_freq/MOD)));
        _regs.feedback_select = (target_freq >= 3.0e9) ?
            max2870_regs_t::FEEDBACK_SELECT_DIVIDED :
            max2870_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;
        _regs.r_counter_10_bit = R;
        _regs.reference_divide_by_2 = T ?
            max2870_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED :
            max2870_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
        _regs.reference_doubler = D ?
            max2870_regs_t::REFERENCE_DOUBLER_ENABLED :
            max2870_regs_t::REFERENCE_DOUBLER_DISABLED;
        _regs.band_select_clock_div = BS;
        _regs.bs_msb = (BS & 0x300) >> 8;
        UHD_ASSERT_THROW(rfdivsel_to_enum.has_key(RFdiv));
        _regs.rf_divider_select = rfdivsel_to_enum[RFdiv];

        switch (output_power)
        {
        case -4:
            _regs.output_power = max2870_regs_t::OUTPUT_POWER_M4DBM;
            break;
        case -1:
            _regs.output_power = max2870_regs_t::OUTPUT_POWER_M1DBM;
            break;
        case 2:
            _regs.output_power = max2870_regs_t::OUTPUT_POWER_2DBM;
            break;
        case 5:
            _regs.output_power = max2870_regs_t::OUTPUT_POWER_5DBM;
            break;
        }

        // Write the register values
        write_regs();

        // MAX2870 needs a 20ms delay after tuning for the first time
        // for the lock detect to be reliable.
        if (_first_tune)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(20));
            _first_tune = false;
        }

        return actual_freq;
    };

private:
    std::set<boost::uint32_t> get_changed_addrs()
    {
        return _regs.get_changed_addrs<boost::uint32_t>();
    };

    boost::uint32_t get_reg(boost::uint32_t addr)
    {
        return _regs.get_reg(addr);
    };

    void save_state()
    {
        if (_save_state)
            _regs.save_state();
    }

    max2870_regs_t _regs;
    bool _save_state;
    bool _first_tune;
};

class max2871 : public max287x
{
public:
    max2871(max287x_write_fn write_fn) : max287x(write_fn), _first_tune(true)
    {
        // initialize register values (override defaults)
        _regs.retune = max2871_regs_t::RETUNE_DISABLED;
        //_regs.csm = max2871_regs_t::CSM_ENABLED;  // tried it - caused long lock times
        _regs.charge_pump_current = max2871_regs_t::CHARGE_PUMP_CURRENT_5_12MA;

        // MAX2871 data sheet says that all registers must be written twice
        // with at least a 20ms delay between writes upon power up.  One
        // write and a 20ms wait are done in power_up().  The second write
        // is done when any other function that does a write to the registers
        // is called.  To ensure all registers are written the second time, the
        // state of the registers is not saved during the first write.
        _save_state = false;
        power_up();
        _save_state = true;
    };

    ~max2871()
    {
        shutdown();
    };

    bool is_shutdown(void)
    {
        return (_regs.power_down == max2871_regs_t::POWER_DOWN_SHUTDOWN);
    };

    void shutdown(void)
    {
        _regs.rf_output_enable = max2871_regs_t::RF_OUTPUT_ENABLE_DISABLED;
        _regs.aux_output_enable = max2871_regs_t::AUX_OUTPUT_ENABLE_DISABLED;
        _regs.power_down = max2871_regs_t::POWER_DOWN_SHUTDOWN;
        _regs.ld_pin_mode = max2871_regs_t::LD_PIN_MODE_LOW;
        _regs.muxout = max2871_regs_t::MUXOUT_TRI_STATE;
        write_regs();
    };

    void power_up(void)
    {
        _regs.ld_pin_mode = max2871_regs_t::LD_PIN_MODE_DLD;
        _regs.power_down = max2871_regs_t::POWER_DOWN_NORMAL;
        _regs.muxout = max2871_regs_t::MUXOUT_TRI_STATE;
        write_regs();

        // MAX271 data sheet says to wait at least 20 ms after exiting low power mode
        // before programming final VCO frequency
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));

        _first_tune = true;
    };

    double set_freq_and_power(double target_freq, double ref_freq, bool is_int_n, int output_power)
    {
        //map rf divider select output dividers to enums
        static const uhd::dict<int, max2871_regs_t::rf_divider_select_t> rfdivsel_to_enum =
            boost::assign::map_list_of
            (1,   max2871_regs_t::RF_DIVIDER_SELECT_DIV1)
            (2,   max2871_regs_t::RF_DIVIDER_SELECT_DIV2)
            (4,   max2871_regs_t::RF_DIVIDER_SELECT_DIV4)
            (8,   max2871_regs_t::RF_DIVIDER_SELECT_DIV8)
            (16,  max2871_regs_t::RF_DIVIDER_SELECT_DIV16)
            (32,  max2871_regs_t::RF_DIVIDER_SELECT_DIV32)
            (64,  max2871_regs_t::RF_DIVIDER_SELECT_DIV64)
            (128, max2871_regs_t::RF_DIVIDER_SELECT_DIV128);

        int T = 0;
        int D = ref_freq <= 10.0e6 ? 1 : 0;
        int R, BS, N, FRAC, MOD, RFdiv;
        double pfd_freq = 50e6;

        double actual_freq = calculate_freq_settings(
            target_freq, ref_freq, 50e6, is_int_n, pfd_freq, T, D, R, BS, N, FRAC, MOD, RFdiv);

        //load the register values
        _regs.rf_output_enable = max2871_regs_t::RF_OUTPUT_ENABLE_ENABLED;

        if(is_int_n) {
            _regs.cpl = max2871_regs_t::CPL_DISABLED;
            _regs.ldf = max2871_regs_t::LDF_INT_N;
            _regs.int_n_mode = max2871_regs_t::INT_N_MODE_INT_N;
        } else {
            _regs.cpl = max2871_regs_t::CPL_ENABLED;
            _regs.ldf = max2871_regs_t::LDF_FRAC_N;
            _regs.int_n_mode = max2871_regs_t::INT_N_MODE_FRAC_N;
        }

        _regs.lds = pfd_freq <= 32e6 ? max2871_regs_t::LDS_SLOW : max2871_regs_t::LDS_FAST;

        _regs.frac_12_bit = FRAC;
        _regs.int_16_bit = N;
        _regs.mod_12_bit = MOD;
        _regs.clock_divider_12_bit = std::max(1, int(std::ceil(400e-6*pfd_freq/MOD)));
        _regs.feedback_select = (target_freq >= 3.0e9) ?
            max2871_regs_t::FEEDBACK_SELECT_DIVIDED :
            max2871_regs_t::FEEDBACK_SELECT_FUNDAMENTAL;
        _regs.r_counter_10_bit = R;
        _regs.reference_divide_by_2 = T ?
            max2871_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED :
            max2871_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
        _regs.reference_doubler = D ?
            max2871_regs_t::REFERENCE_DOUBLER_ENABLED :
            max2871_regs_t::REFERENCE_DOUBLER_DISABLED;
        _regs.band_select_clock_div = BS;
        _regs.bs_msb = (BS & 0x300) >> 8;
        UHD_ASSERT_THROW(rfdivsel_to_enum.has_key(RFdiv));
        _regs.rf_divider_select = rfdivsel_to_enum[RFdiv];

        switch (output_power)
        {
        case -4:
            _regs.output_power = max2871_regs_t::OUTPUT_POWER_M4DBM;
            break;
        case -1:
            _regs.output_power = max2871_regs_t::OUTPUT_POWER_M1DBM;
            break;
        case 2:
            _regs.output_power = max2871_regs_t::OUTPUT_POWER_2DBM;
            break;
        case 5:
            _regs.output_power = max2871_regs_t::OUTPUT_POWER_5DBM;
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
            break;
        }

        write_regs();

        // MAX2871 needs a 20ms delay after tuning for the first time
        // for the lock detect to be reliable.
        if (_first_tune)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(20));
            _first_tune = false;
        }

        return actual_freq;
    };

private:
    std::set<boost::uint32_t> get_changed_addrs()
    {
        return _regs.get_changed_addrs<boost::uint32_t>();
    };

    boost::uint32_t get_reg(boost::uint32_t addr)
    {
        return _regs.get_reg(addr);
    };

    void save_state()
    {
        if (_save_state)
            _regs.save_state();
    }

    max2871_regs_t _regs;
    bool _save_state;
    bool _first_tune;
};

/***********************************************************************
 * UBX Data Structures
 **********************************************************************/
enum ubx_gpio_field_id_t
{
    SPI_ADDR,
    TX_EN_N,
    RX_EN_N,
    RX_ANT,
    TX_LO_LOCKED,
    RX_LO_LOCKED,
    CPLD_RST_N,
    TX_GAIN,
    RX_GAIN,
    RXLO1_SYNC,
    RXLO2_SYNC,
    TXLO1_SYNC,
    TXLO2_SYNC
};

enum ubx_cpld_field_id_t
{
    TXHB_SEL = 0,
    TXLB_SEL = 1,
    TXLO1_FSEL1 = 2,
    TXLO1_FSEL2 = 3,
    TXLO1_FSEL3 = 4,
    RXHB_SEL = 5,
    RXLB_SEL = 6,
    RXLO1_FSEL1 = 7,
    RXLO1_FSEL2 = 8,
    RXLO1_FSEL3 = 9,
    SEL_LNA1 = 10,
    SEL_LNA2 = 11,
    TXLO1_FORCEON = 12,
    TXLO2_FORCEON = 13,
    TXMOD_FORCEON = 14,
    TXMIXER_FORCEON = 15,
    TXDRV_FORCEON = 16,
    RXLO1_FORCEON = 17,
    RXLO2_FORCEON = 18,
    RXDEMOD_FORCEON = 19,
    RXMIXER_FORCEON = 20,
    RXDRV_FORCEON = 21,
    RXAMP_FORCEON = 22,
    RXLNA1_FORCEON = 23,
    RXLNA2_FORCEON = 24
};

struct ubx_gpio_field_info_t
{
    ubx_gpio_field_id_t id;
    dboard_iface::unit_t unit;
    boost::uint32_t offset;
    boost::uint32_t mask;
    boost::uint32_t width;
    enum direction_t {OUTPUT,INPUT} direction;
    bool is_atr_controlled;
    boost::uint32_t atr_idle;
    boost::uint32_t atr_tx;
    boost::uint32_t atr_rx;
    boost::uint32_t atr_full_duplex;
};

struct ubx_gpio_reg_t
{
    bool dirty;
    boost::uint32_t value;
    boost::uint32_t mask;
    boost::uint32_t ddr;
    boost::uint32_t atr_mask;
    boost::uint32_t atr_idle;
    boost::uint32_t atr_tx;
    boost::uint32_t atr_rx;
    boost::uint32_t atr_full_duplex;
};

struct ubx_cpld_reg_t
{
    void set_field(ubx_cpld_field_id_t field, boost::uint32_t val)
    {
        UHD_ASSERT_THROW(val == (val & 0x1));

        if (val)
            value |= boost::uint32_t(1) << field;
        else
            value &= ~(boost::uint32_t(1) << field);
    }

    boost::uint32_t value;
};

enum spi_dest_t {
    TXLO1 = 0x0,    // 0x00: TXLO1, the main TXLO from 400MHz to 6000MHz
    TXLO2 = 0x1,    // 0x01: TXLO2, the low band mixer TXLO 10MHz to 400MHz
    RXLO1 = 0x2,    // 0x02: RXLO1, the main RXLO from 400MHz to 6000MHz
    RXLO2 = 0x3,    // 0x03: RXLO2, the low band mixer RXLO 10MHz to 400MHz
    CPLD = 0x4      // 0x04: CPLD SPI Register
    };

/***********************************************************************
 * UBX Constants
 **********************************************************************/
static const freq_range_t ubx_freq_range(1.0e7, 6.0e9);
static const gain_range_t ubx_tx_gain_range(0, 31.5, double(0.5));
static const gain_range_t ubx_rx_gain_range(0, 31.5, double(0.5));
static const std::vector<std::string> ubx_pgas = boost::assign::list_of("PGA-TX")("PGA-RX");
static const std::vector<std::string> ubx_plls = boost::assign::list_of("TXLO")("RXLO");
static const std::vector<std::string> ubx_tx_antennas = boost::assign::list_of("TX/RX")("CAL");
static const std::vector<std::string> ubx_rx_antennas = boost::assign::list_of("TX/RX")("RX2")("CAL");
static const std::vector<std::string> ubx_power_modes = boost::assign::list_of("performance")("powersave");
static const std::vector<std::string> ubx_xcvr_modes = boost::assign::list_of("FDX")("TX")("TX/RX")("RX");

static const ubx_gpio_field_info_t ubx_proto_gpio_info[] = {
    {SPI_ADDR,      dboard_iface::UNIT_TX,  0,  0x7,        3,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {TX_EN_N,       dboard_iface::UNIT_TX,  3,  0x1<<3,     1,  ubx_gpio_field_info_t::INPUT,  true,   1,  0,  1,  0},
    {RX_EN_N,       dboard_iface::UNIT_TX,  4,  0x1<<4,     1,  ubx_gpio_field_info_t::INPUT,  true,   1,  1,  0,  0},
    {RX_ANT,        dboard_iface::UNIT_TX,  5,  0x1<<5,     1,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {TX_LO_LOCKED,  dboard_iface::UNIT_TX,  6,  0x1<<6,     1,  ubx_gpio_field_info_t::OUTPUT, false,  0,  0,  0,  0},
    {RX_LO_LOCKED,  dboard_iface::UNIT_TX,  7,  0x1<<7,     1,  ubx_gpio_field_info_t::OUTPUT, false,  0,  0,  0,  0},
    {CPLD_RST_N,    dboard_iface::UNIT_TX,  9,  0x1<<9,     1,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {TX_GAIN,       dboard_iface::UNIT_TX,  10, 0x3F<<10,   10, ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {RX_GAIN,       dboard_iface::UNIT_RX,  10, 0x3F<<10,   10, ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0}
};

static const ubx_gpio_field_info_t ubx_v1_gpio_info[] = {
    {SPI_ADDR,      dboard_iface::UNIT_TX,   0,  0x7,        3,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {CPLD_RST_N,    dboard_iface::UNIT_TX,   3,  0x1<<3,     1,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {RX_ANT,        dboard_iface::UNIT_TX,   4,  0x1<<4,     1,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {TX_EN_N,       dboard_iface::UNIT_TX,   5,  0x1<<5,     1,  ubx_gpio_field_info_t::INPUT,  true,   1,  0,  1,  0},
    {RX_EN_N,       dboard_iface::UNIT_TX,   6,  0x1<<6,     1,  ubx_gpio_field_info_t::INPUT,  true,   1,  1,  0,  0},
    {TXLO1_SYNC,    dboard_iface::UNIT_TX,   7,  0x1<<7,     1,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {TXLO2_SYNC,    dboard_iface::UNIT_TX,   9,  0x1<<9,     1,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {TX_GAIN,       dboard_iface::UNIT_TX,   10, 0x3F<<10,   10, ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {RX_LO_LOCKED,  dboard_iface::UNIT_RX,   0,  0x1,        1,  ubx_gpio_field_info_t::OUTPUT, false,  0,  0,  0,  0},
    {TX_LO_LOCKED,  dboard_iface::UNIT_RX,   1,  0x1<<1,     1,  ubx_gpio_field_info_t::OUTPUT, false,  0,  0,  0,  0},
    {RXLO1_SYNC,    dboard_iface::UNIT_RX,   5,  0x1<<5,     1,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {RXLO2_SYNC,    dboard_iface::UNIT_RX,   7,  0x1<<7,     1,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {RX_GAIN,       dboard_iface::UNIT_RX,   10, 0x3F<<10,   10, ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0}
};

/***********************************************************************
 * Macros and helper functions for routing and writing SPI registers
 **********************************************************************/
#define ROUTE_SPI(iface, dest)  \
    iface->set_gpio_out(dboard_iface::UNIT_TX, dest, 0x7);

#define WRITE_SPI(iface, val)   \
    iface->write_spi(dboard_iface::UNIT_TX, spi_config_t::EDGE_RISE, val, 32);

UHD_INLINE void write_spi_reg(dboard_iface::sptr iface, spi_dest_t dest, boost::uint32_t value)
{
    ROUTE_SPI(iface, dest);
    WRITE_SPI(iface, value);
}

UHD_INLINE void write_spi_regs(dboard_iface::sptr iface, spi_dest_t dest, std::vector<boost::uint32_t> values)
{
    ROUTE_SPI(iface, dest);
    for (size_t i = 0; i < values.size(); i++)
        WRITE_SPI(iface, values[i]);
}

/***********************************************************************
 * UBX Class Definition
 **********************************************************************/
class ubx_xcvr : public xcvr_dboard_base
{
public:
    ubx_xcvr(ctor_args_t args) : xcvr_dboard_base(args)
    {
        ////////////////////////////////////////////////////////////////////
        // Setup GPIO hardware
        ////////////////////////////////////////////////////////////////////
        _iface = get_iface();
        dboard_id_t rx_id = get_rx_id();
        dboard_id_t tx_id = get_tx_id();
        if (rx_id == UBX_PROTO_V3_RX_ID and tx_id == UBX_PROTO_V3_TX_ID)
            _rev = 0;
        if (rx_id == UBX_PROTO_V4_RX_ID and tx_id == UBX_PROTO_V4_TX_ID)
            _rev = 1;
        else if (rx_id == UBX_V1_40MHZ_RX_ID and tx_id == UBX_V1_40MHZ_TX_ID)
            _rev = 1;
        else if (rx_id == UBX_V1_160MHZ_RX_ID and tx_id == UBX_V1_160MHZ_TX_ID)
            _rev = 1;
        else
            UHD_THROW_INVALID_CODE_PATH();

        switch(_rev)
        {
        case 0:
            for (size_t i = 0; i < sizeof(ubx_proto_gpio_info) / sizeof(ubx_gpio_field_info_t); i++)
                _gpio_map[ubx_proto_gpio_info[i].id] = ubx_proto_gpio_info[i];
            break;
        case 1:
            for (size_t i = 0; i < sizeof(ubx_v1_gpio_info) / sizeof(ubx_gpio_field_info_t); i++)
                _gpio_map[ubx_v1_gpio_info[i].id] = ubx_v1_gpio_info[i];
            break;
        }

        // Initialize GPIO registers
        memset(&_tx_gpio_reg,0,sizeof(ubx_gpio_reg_t));
        memset(&_rx_gpio_reg,0,sizeof(ubx_gpio_reg_t));
        for (std::map<ubx_gpio_field_id_t,ubx_gpio_field_info_t>::iterator entry = _gpio_map.begin(); entry != _gpio_map.end(); entry++)
        {
            ubx_gpio_field_info_t info = entry->second;
            ubx_gpio_reg_t *reg = (info.unit == dboard_iface::UNIT_TX ? &_tx_gpio_reg : &_rx_gpio_reg);
            if (info.direction == ubx_gpio_field_info_t::INPUT)
                reg->ddr |= info.mask;
            if (info.is_atr_controlled)
            {
                reg->atr_mask |= info.mask;
                reg->atr_idle |= (info.atr_idle << info.offset) & info.mask;
                reg->atr_tx |= (info.atr_tx << info.offset) & info.mask;
                reg->atr_rx |= (info.atr_rx << info.offset) & info.mask;
                reg->atr_full_duplex |= (info.atr_full_duplex << info.offset) & info.mask;
            }
        }

        // Enable the reference clocks that we need
        _iface->set_clock_enabled(dboard_iface::UNIT_TX, true);
        _iface->set_clock_enabled(dboard_iface::UNIT_RX, true);

        // Set direction of GPIO pins (1 is input to UBX, 0 is output)
        _iface->set_gpio_ddr(dboard_iface::UNIT_TX, _tx_gpio_reg.ddr);
        _iface->set_gpio_ddr(dboard_iface::UNIT_RX, _rx_gpio_reg.ddr);

        // Set default GPIO values
        set_gpio_field(TX_GAIN, 0);
        set_gpio_field(CPLD_RST_N, 0);
        set_gpio_field(RX_ANT, 1);
        set_gpio_field(TX_EN_N, 1);
        set_gpio_field(RX_EN_N, 1);
        set_gpio_field(SPI_ADDR, 0x7);
        set_gpio_field(RX_GAIN, 0);
        set_gpio_field(TXLO1_SYNC, 0);
        set_gpio_field(TXLO2_SYNC, 0);
        set_gpio_field(RXLO1_SYNC, 0);
        set_gpio_field(RXLO1_SYNC, 0);
        write_gpio();

        // Configure ATR
        _iface->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_IDLE, _tx_gpio_reg.atr_idle);
        _iface->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_TX_ONLY, _tx_gpio_reg.atr_tx);
        _iface->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_RX_ONLY, _tx_gpio_reg.atr_rx);
        _iface->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_FULL_DUPLEX, _tx_gpio_reg.atr_full_duplex);
        _iface->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_IDLE, _rx_gpio_reg.atr_idle);
        _iface->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_TX_ONLY, _rx_gpio_reg.atr_tx);
        _iface->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_RX_ONLY, _rx_gpio_reg.atr_rx);
        _iface->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_FULL_DUPLEX, _rx_gpio_reg.atr_full_duplex);

        // Engage ATR control (1 is ATR control, 0 is manual control)
        _iface->set_pin_ctrl(dboard_iface::UNIT_TX, _tx_gpio_reg.atr_mask);
        _iface->set_pin_ctrl(dboard_iface::UNIT_RX, _rx_gpio_reg.atr_mask);

        // bring CPLD out of reset
        boost::this_thread::sleep(boost::posix_time::milliseconds(20)); // hold CPLD reset for minimum of 20 ms

        set_gpio_field(CPLD_RST_N, 1);
        write_gpio();

        // Initialize LOs
        if (_rev == 0)
        {
            _txlo1.reset(new max2870(boost::bind(&write_spi_regs, _iface, TXLO1, _1)));
            _txlo2.reset(new max2870(boost::bind(&write_spi_regs, _iface, TXLO2, _1)));
            _rxlo1.reset(new max2870(boost::bind(&write_spi_regs, _iface, RXLO1, _1)));
            _rxlo2.reset(new max2870(boost::bind(&write_spi_regs, _iface, RXLO2, _1)));
        }
        else if (_rev == 1)
        {
            _txlo1.reset(new max2871(boost::bind(&write_spi_regs, _iface, TXLO1, _1)));
            _txlo2.reset(new max2871(boost::bind(&write_spi_regs, _iface, TXLO2, _1)));
            _rxlo1.reset(new max2871(boost::bind(&write_spi_regs, _iface, RXLO1, _1)));
            _rxlo2.reset(new max2871(boost::bind(&write_spi_regs, _iface, RXLO2, _1)));
        }
        else
        {
            UHD_THROW_INVALID_CODE_PATH();
        }

        // Initialize CPLD register
        _cpld_reg.value = 0;
        write_cpld_reg();

        ////////////////////////////////////////////////////////////////////
        // Register power save properties
        ////////////////////////////////////////////////////////////////////
        get_rx_subtree()->create<std::vector<std::string> >("power_mode/options")
            .set(ubx_power_modes);
        get_rx_subtree()->create<std::string>("power_mode/value")
            .subscribe(boost::bind(&ubx_xcvr::set_power_mode, this, _1))
            .set("performance");
        get_rx_subtree()->create<std::vector<std::string> >("xcvr_mode/options")
            .set(ubx_xcvr_modes);
        get_rx_subtree()->create<std::string>("xcvr_mode/value")
            .subscribe(boost::bind(&ubx_xcvr::set_xcvr_mode, this, _1))
            .set("FDX");

        ////////////////////////////////////////////////////////////////////
        // Register TX properties
        ////////////////////////////////////////////////////////////////////
        get_tx_subtree()->create<std::string>("name").set("UBX TX");
        get_tx_subtree()->create<device_addr_t>("tune_args")
            .set(device_addr_t());
        get_tx_subtree()->create<sensor_value_t>("sensors/lo_locked")
            .publish(boost::bind(&ubx_xcvr::get_locked, this, "TXLO"));
        get_tx_subtree()->create<double>("gains/PGA0/value")
            .coerce(boost::bind(&ubx_xcvr::set_tx_gain, this, _1)).set(0);
        get_tx_subtree()->create<meta_range_t>("gains/PGA0/range")
            .set(ubx_tx_gain_range);
        get_tx_subtree()->create<double>("freq/value")
            .coerce(boost::bind(&ubx_xcvr::set_tx_freq, this, _1))
            .set(ubx_freq_range.start());
        get_tx_subtree()->create<meta_range_t>("freq/range")
            .set(ubx_freq_range);
        get_tx_subtree()->create<std::vector<std::string> >("antenna/options")
            .set(ubx_tx_antennas);
        get_tx_subtree()->create<std::string>("antenna/value")
            .subscribe(boost::bind(&ubx_xcvr::set_tx_ant, this, _1))
            .set(ubx_tx_antennas.at(0));
        get_tx_subtree()->create<std::string>("connection")
            .set("QI");
        get_tx_subtree()->create<bool>("enabled")
            .set(true); //always enabled
        get_tx_subtree()->create<bool>("use_lo_offset")
            .set(false);
        get_tx_subtree()->create<double>("bandwidth/value")
            .set(2*20.0e6); //20MHz low-pass, complex double-sided, so it should be 2x20MHz=40MHz
        get_tx_subtree()->create<meta_range_t>("bandwidth/range")
            .set(freq_range_t(2*20.0e6, 2*20.0e6));

        ////////////////////////////////////////////////////////////////////
        // Register RX properties
        ////////////////////////////////////////////////////////////////////
        get_rx_subtree()->create<std::string>("name").set("UBX RX");
        get_rx_subtree()->create<device_addr_t>("tune_args")
            .set(device_addr_t());
        get_rx_subtree()->create<sensor_value_t>("sensors/lo_locked")
            .publish(boost::bind(&ubx_xcvr::get_locked, this, "RXLO"));
        get_rx_subtree()->create<double>("gains/PGA0/value")
            .coerce(boost::bind(&ubx_xcvr::set_rx_gain, this, _1))
            .set(0);
        get_rx_subtree()->create<meta_range_t>("gains/PGA0/range")
            .set(ubx_rx_gain_range);
        get_rx_subtree()->create<double>("freq/value")
            .coerce(boost::bind(&ubx_xcvr::set_rx_freq, this, _1))
            .set(ubx_freq_range.start());
        get_rx_subtree()->create<meta_range_t>("freq/range")
            .set(ubx_freq_range);
        get_rx_subtree()->create<std::vector<std::string> >("antenna/options")
            .set(ubx_rx_antennas);
        get_rx_subtree()->create<std::string>("antenna/value")
            .subscribe(boost::bind(&ubx_xcvr::set_rx_ant, this, _1)).set("RX2");
        get_rx_subtree()->create<std::string>("connection")
            .set("IQ");
        get_rx_subtree()->create<bool>("enabled")
            .set(true); //always enabled
        get_rx_subtree()->create<bool>("use_lo_offset")
            .set(false);
        get_rx_subtree()->create<double>("bandwidth/value")
            .set(2*20.0e6); //20MHz low-pass, complex double-sided, so it should be 2x20MHz=40MHz
        get_rx_subtree()->create<meta_range_t>("bandwidth/range")
            .set(freq_range_t(2*20.0e6, 2*20.0e6));
    }

    ~ubx_xcvr(void)
    {
        // Shutdown synthesizers
        _txlo1->shutdown();
        _txlo2->shutdown();
        _rxlo1->shutdown();
        _rxlo2->shutdown();

        // Reset CPLD values
        _cpld_reg.value = 0;
        write_cpld_reg();

        // Reset GPIO values
        set_gpio_field(TX_GAIN, 0);
        set_gpio_field(CPLD_RST_N, 0);
        set_gpio_field(RX_ANT, 1);
        set_gpio_field(TX_EN_N, 1);
        set_gpio_field(RX_EN_N, 1);
        set_gpio_field(SPI_ADDR, 0x7);
        set_gpio_field(RX_GAIN, 0);
        set_gpio_field(TXLO1_SYNC, 0);
        set_gpio_field(TXLO2_SYNC, 0);
        set_gpio_field(RXLO1_SYNC, 0);
        set_gpio_field(RXLO1_SYNC, 0);
        write_gpio();
    }

private:
    enum power_mode_t {PERFORMANCE,POWERSAVE};

    /***********************************************************************
    * Helper Functions
    **********************************************************************/
    void set_cpld_field(ubx_cpld_field_id_t id, boost::uint32_t value)
    {
        _cpld_reg.set_field(id, value);
    }

    void write_cpld_reg()
    {
        write_spi_reg(_iface, CPLD, _cpld_reg.value);
    }

    void set_gpio_field(ubx_gpio_field_id_t id, boost::uint32_t value)
    {
        // Look up field info
        std::map<ubx_gpio_field_id_t,ubx_gpio_field_info_t>::iterator entry = _gpio_map.find(id);
        if (entry == _gpio_map.end())
            return;
        ubx_gpio_field_info_t field_info = entry->second;
        if (field_info.direction == ubx_gpio_field_info_t::OUTPUT)
            return;
        ubx_gpio_reg_t *reg = (field_info.unit == dboard_iface::UNIT_TX ? &_tx_gpio_reg : &_rx_gpio_reg);
        boost::uint32_t _value = reg->value;
        boost::uint32_t _mask = reg->mask;

        // Set field and mask
        _value &= ~field_info.mask;
        _value |= (value << field_info.offset) & field_info.mask;
        _mask |= field_info.mask;

        // Mark whether register is dirty or not
        if (_value != reg->value)
        {
            reg->value = _value;
            reg->mask = _mask;
            reg->dirty = true;
        }
    }

    boost::uint32_t get_gpio_field(ubx_gpio_field_id_t id)
    {
        // Look up field info
        std::map<ubx_gpio_field_id_t,ubx_gpio_field_info_t>::iterator entry = _gpio_map.find(id);
        if (entry == _gpio_map.end())
            return 0;
        ubx_gpio_field_info_t field_info = entry->second;
        if (field_info.direction == ubx_gpio_field_info_t::INPUT)
            return 0;

        // Read register
        boost::uint32_t value = _iface->read_gpio(field_info.unit);
        value &= field_info.mask;
        value >>= field_info.offset;

        // Return field value
        return value;
    }

    void write_gpio()
    {
        if (_tx_gpio_reg.dirty)
        {
            _iface->set_gpio_out(dboard_iface::UNIT_TX, _tx_gpio_reg.value, _tx_gpio_reg.mask);
            _tx_gpio_reg.dirty = false;
            _tx_gpio_reg.mask = 0;
        }
        if (_rx_gpio_reg.dirty)
        {
            _iface->set_gpio_out(dboard_iface::UNIT_RX, _rx_gpio_reg.value, _rx_gpio_reg.mask);
            _rx_gpio_reg.dirty = false;
            _rx_gpio_reg.mask = 0;
        }
    }

    /***********************************************************************
     * Board Control Handling
     **********************************************************************/
    sensor_value_t get_locked(const std::string &pll_name)
    {
        assert_has(ubx_plls, pll_name, "ubx pll name");

        if(pll_name == "TXLO")
        {
            _txlo_locked = (get_gpio_field(TX_LO_LOCKED) != 0);
            return sensor_value_t("TXLO", _txlo_locked, "locked", "unlocked");
        }
        else if(pll_name == "RXLO")
        {
            _rxlo_locked = (get_gpio_field(RX_LO_LOCKED) != 0);
            return sensor_value_t("RXLO", _rxlo_locked, "locked", "unlocked");
        }

        return sensor_value_t("Unknown", false, "locked", "unlocked");
    }

    void set_tx_ant(const std::string &ant)
    {
        //validate input
        assert_has(ubx_tx_antennas, ant, "ubx tx antenna name");
    }

    // Set RX antennas
    void set_rx_ant(const std::string &ant)
    {
        //validate input
        assert_has(ubx_rx_antennas, ant, "ubx rx antenna name");

        if(ant == "RX2")
            set_gpio_field(RX_ANT, 1);
        else if(ant == "TX/RX")
            set_gpio_field(RX_ANT, 0);
        else if (ant == "CAL")
            set_gpio_field(RX_ANT, 1);
        write_gpio();
    }

    /***********************************************************************
     * Gain Handling
     **********************************************************************/
    double set_tx_gain(double gain)
    {
        gain = ubx_tx_gain_range.clip(gain);
        int attn_code = int(std::floor(gain * 2));
        _ubx_tx_atten_val = ((attn_code & 0x3F) << 10);
        set_gpio_field(TX_GAIN, attn_code);
        write_gpio();
        UHD_LOGV(rarely) << boost::format("UBX TX Gain: %f dB, Code: %d, IO Bits 0x%04x") % gain % attn_code % _ubx_tx_atten_val << std::endl;
        _tx_gain = gain;
        return gain;
    }

    double set_rx_gain(double gain)
    {
        gain = ubx_rx_gain_range.clip(gain);
        int attn_code = int(std::floor(gain * 2));
        _ubx_rx_atten_val = ((attn_code & 0x3F) << 10);
        set_gpio_field(RX_GAIN, attn_code);
        write_gpio();
        UHD_LOGV(rarely) << boost::format("UBX RX Gain: %f dB, Code: %d, IO Bits 0x%04x") % gain % attn_code % _ubx_rx_atten_val << std::endl;
        _rx_gain = gain;
        return gain;
    }

    /***********************************************************************
    * Frequency Handling
    **********************************************************************/
    double set_tx_freq(double freq)
    {
        double freq_lo1 = 0.0;
        double freq_lo2 = 0.0;
        double ref_freq = _iface->get_clock_rate(dboard_iface::UNIT_TX);
        bool is_int_n = false;

        /*
         * If the user sets 'mode_n=integer' in the tuning args, the user wishes to
         * tune in Integer-N mode, which can result in better spur
         * performance on some mixers. The default is fractional tuning.
         */
        property_tree::sptr subtree = this->get_tx_subtree();
        device_addr_t tune_args = subtree->access<device_addr_t>("tune_args").get();
        is_int_n = boost::iequals(tune_args.get("mode_n",""), "integer");
        UHD_LOGV(rarely) << boost::format("UBX TX: the requested frequency is %f MHz") % (freq/1e6) << std::endl;

        // Clip the frequency to the valid range
        freq = ubx_freq_range.clip(freq);

        // Power up/down LOs
        if (_txlo1->is_shutdown())
            _txlo1->power_up();
        if (_txlo2->is_shutdown() and (_power_mode == PERFORMANCE or freq < (500*fMHz)))
            _txlo2->power_up();
        else if (freq >= 500*fMHz and _power_mode == POWERSAVE)
            _txlo2->shutdown();

        // Set up registers for the requested frequency
        if (freq < (500*fMHz))
        {
            set_cpld_field(TXLO1_FSEL3, 0);
            set_cpld_field(TXLO1_FSEL2, 1);
            set_cpld_field(TXLO1_FSEL1, 0);
            set_cpld_field(TXLB_SEL, 1);
            set_cpld_field(TXHB_SEL, 0);
            write_cpld_reg();
            // Set LO1 to IF of 2100 MHz (offset from RX IF to reduce leakage)
            freq_lo1 = _txlo1->set_freq_and_power(2100*fMHz, ref_freq, is_int_n, 5);
            // Set LO2 to IF minus desired frequency
            freq_lo2 = _txlo2->set_freq_and_power(freq_lo1 - freq, ref_freq, is_int_n, 2);
        }
        else if ((freq >= (500*fMHz)) && (freq <= (800*fMHz)))
        {
            set_cpld_field(TXLO1_FSEL3, 0);
            set_cpld_field(TXLO1_FSEL2, 0);
            set_cpld_field(TXLO1_FSEL1, 1);
            set_cpld_field(TXLB_SEL, 0);
            set_cpld_field(TXHB_SEL, 1);
            write_cpld_reg();
            freq_lo1 = _txlo1->set_freq_and_power(freq, ref_freq, is_int_n, 2);
        }
        else if ((freq > (800*fMHz)) && (freq <= (1000*fMHz)))
        {
            set_cpld_field(TXLO1_FSEL3, 0);
            set_cpld_field(TXLO1_FSEL2, 0);
            set_cpld_field(TXLO1_FSEL1, 1);
            set_cpld_field(TXLB_SEL, 0);
            set_cpld_field(TXHB_SEL, 1);
            write_cpld_reg();
            freq_lo1 = _txlo1->set_freq_and_power(freq, ref_freq, is_int_n, 5);
        }
        else if ((freq > (1000*fMHz)) && (freq <= (2200*fMHz)))
        {
            set_cpld_field(TXLO1_FSEL3, 0);
            set_cpld_field(TXLO1_FSEL2, 1);
            set_cpld_field(TXLO1_FSEL1, 0);
            set_cpld_field(TXLB_SEL, 0);
            set_cpld_field(TXHB_SEL, 1);
            write_cpld_reg();
            freq_lo1 = _txlo1->set_freq_and_power(freq, ref_freq, is_int_n, 2);
        }
        else if ((freq > (2200*fMHz)) && (freq <= (2500*fMHz)))
        {
            set_cpld_field(TXLO1_FSEL3, 0);
            set_cpld_field(TXLO1_FSEL2, 1);
            set_cpld_field(TXLO1_FSEL1, 0);
            set_cpld_field(TXLB_SEL, 0);
            set_cpld_field(TXHB_SEL, 1);
            write_cpld_reg();
            freq_lo1 = _txlo1->set_freq_and_power(freq, ref_freq, is_int_n, 2);
        }
        else if ((freq > (2500*fMHz)) && (freq <= (6000*fMHz)))
        {
            set_cpld_field(TXLO1_FSEL3, 1);
            set_cpld_field(TXLO1_FSEL2, 0);
            set_cpld_field(TXLO1_FSEL1, 0);
            set_cpld_field(TXLB_SEL, 0);
            set_cpld_field(TXHB_SEL, 1);
            write_cpld_reg();
            freq_lo1 = _txlo1->set_freq_and_power(freq, ref_freq, is_int_n, 5);
        }

        _tx_freq = freq_lo1 - freq_lo2;
        _txlo1_freq = freq_lo1;
        _txlo2_freq = freq_lo2;

        UHD_LOGV(rarely) << boost::format("UBX TX: the actual frequency is %f MHz") % (_tx_freq/1e6) << std::endl;

        return _tx_freq;
    }

    double set_rx_freq(double freq)
    {
        double freq_lo1 = 0.0;
        double freq_lo2 = 0.0;
        double ref_freq = _iface->get_clock_rate(dboard_iface::UNIT_RX);
        bool is_int_n = false;

        UHD_LOGV(rarely) << boost::format("UBX RX: the requested frequency is %f MHz") % (freq/1e6) << std::endl;

        property_tree::sptr subtree = this->get_rx_subtree();
        device_addr_t tune_args = subtree->access<device_addr_t>("tune_args").get();
        is_int_n = boost::iequals(tune_args.get("mode_n",""), "integer");

        // Clip the frequency to the valid range
        freq = ubx_freq_range.clip(freq);

        // Power up/down LOs
        if (_rxlo1->is_shutdown())
            _rxlo1->power_up();
        if (_rxlo2->is_shutdown() and (_power_mode == PERFORMANCE or freq < 500*fMHz))
            _rxlo2->power_up();
        else if (freq >= 500*fMHz and _power_mode == POWERSAVE)
            _rxlo2->shutdown();

        // Work with frequencies
        if (freq < 100*fMHz)
        {
            set_cpld_field(SEL_LNA1, 0);
            set_cpld_field(SEL_LNA2, 1);
            set_cpld_field(RXLO1_FSEL3, 1);
            set_cpld_field(RXLO1_FSEL2, 0);
            set_cpld_field(RXLO1_FSEL1, 0);
            set_cpld_field(RXLB_SEL, 1);
            set_cpld_field(RXHB_SEL, 0);
            write_cpld_reg();
            // Set LO1 to IF of 2380 MHz (2440 MHz filter center minus 60 MHz offset to minimize LO leakage)
            freq_lo1 = _rxlo1->set_freq_and_power(2380*fMHz, ref_freq, is_int_n, 5);
            // Set LO2 to IF minus desired frequency
            freq_lo2 = _rxlo2->set_freq_and_power(freq_lo1 - freq, ref_freq, is_int_n, 2);
        }
        else if ((freq >= 100*fMHz) && (freq < 500*fMHz))
        {
            set_cpld_field(SEL_LNA1, 0);
            set_cpld_field(SEL_LNA2, 1);
            set_cpld_field(RXLO1_FSEL3, 1);
            set_cpld_field(RXLO1_FSEL2, 0);
            set_cpld_field(RXLO1_FSEL1, 0);
            set_cpld_field(RXLB_SEL, 1);
            set_cpld_field(RXHB_SEL, 0);
            write_cpld_reg();
            // Set LO1 to IF of 2440 (center of filter)
            freq_lo1 = _rxlo1->set_freq_and_power(2440*fMHz, ref_freq, is_int_n, 5);
            // Set LO2 to IF minus desired frequency
            freq_lo2 = _rxlo2->set_freq_and_power(freq_lo1 - freq, ref_freq, is_int_n, 2);
        }
        else if ((freq >= 500*fMHz) && (freq < 800*fMHz))
        {
            set_cpld_field(SEL_LNA1, 0);
            set_cpld_field(SEL_LNA2, 1);
            set_cpld_field(RXLO1_FSEL3, 0);
            set_cpld_field(RXLO1_FSEL2, 0);
            set_cpld_field(RXLO1_FSEL1, 1);
            set_cpld_field(RXLB_SEL, 0);
            set_cpld_field(RXHB_SEL, 1);
            write_cpld_reg();
            freq_lo1 = _rxlo1->set_freq_and_power(freq, ref_freq, is_int_n, 2);
        }
        else if ((freq >= 800*fMHz) && (freq < 1000*fMHz))
        {
            set_cpld_field(SEL_LNA1, 0);
            set_cpld_field(SEL_LNA2, 1);
            set_cpld_field(RXLO1_FSEL3, 0);
            set_cpld_field(RXLO1_FSEL2, 0);
            set_cpld_field(RXLO1_FSEL1, 1);
            set_cpld_field(RXLB_SEL, 0);
            set_cpld_field(RXHB_SEL, 1);
            write_cpld_reg();
            freq_lo1 = _rxlo1->set_freq_and_power(freq, ref_freq, is_int_n, 5);
        }
        else if ((freq >= 1000*fMHz) && (freq < 1500*fMHz))
        {
            set_cpld_field(SEL_LNA1, 0);
            set_cpld_field(SEL_LNA2, 1);
            set_cpld_field(RXLO1_FSEL3, 0);
            set_cpld_field(RXLO1_FSEL2, 1);
            set_cpld_field(RXLO1_FSEL1, 0);
            set_cpld_field(RXLB_SEL, 0);
            set_cpld_field(RXHB_SEL, 1);
            write_cpld_reg();
            freq_lo1 = _rxlo1->set_freq_and_power(freq, ref_freq, is_int_n, 2);
        }
        else if ((freq >= 1500*fMHz) && (freq < 2200*fMHz))
        {
            set_cpld_field(SEL_LNA1, 1);
            set_cpld_field(SEL_LNA2, 0);
            set_cpld_field(RXLO1_FSEL3, 0);
            set_cpld_field(RXLO1_FSEL2, 1);
            set_cpld_field(RXLO1_FSEL1, 0);
            set_cpld_field(RXLB_SEL, 0);
            set_cpld_field(RXHB_SEL, 1);
            write_cpld_reg();
            freq_lo1 = _rxlo1->set_freq_and_power(freq, ref_freq, is_int_n, 2);
        }
        else if ((freq >= 2200*fMHz) && (freq < 2500*fMHz))
        {
            set_cpld_field(SEL_LNA1, 1);
            set_cpld_field(SEL_LNA2, 0);
            set_cpld_field(RXLO1_FSEL3, 0);
            set_cpld_field(RXLO1_FSEL2, 1);
            set_cpld_field(RXLO1_FSEL1, 0);
            set_cpld_field(RXLB_SEL, 0);
            set_cpld_field(RXHB_SEL, 1);
            write_cpld_reg();
            freq_lo1 = _rxlo1->set_freq_and_power(freq, ref_freq, is_int_n, 2);
        }
        else if ((freq >= 2500*fMHz) && (freq <= 6000*fMHz))
        {
            set_cpld_field(SEL_LNA1, 1);
            set_cpld_field(SEL_LNA2, 0);
            set_cpld_field(RXLO1_FSEL3, 1);
            set_cpld_field(RXLO1_FSEL2, 0);
            set_cpld_field(RXLO1_FSEL1, 0);
            set_cpld_field(RXLB_SEL, 0);
            set_cpld_field(RXHB_SEL, 1);
            write_cpld_reg();
            freq_lo1 = _rxlo1->set_freq_and_power(freq, ref_freq, is_int_n, 5);
        }

        freq = freq_lo1 - freq_lo2;

        UHD_LOGV(rarely) << boost::format("UBX RX: the actual frequency is %f MHz") % (freq/1e6) << std::endl;

        return freq;
    }

    /***********************************************************************
    * Setting Modes
    **********************************************************************/
    void set_power_mode(std::string mode)
    {
        if (mode == "performance")
        {
            // FIXME:  Response to ATR change is too slow for some components,
            // so certain components are forced on here.  Force on does not
            // necessarily mean immediately.  Some FORCEON lines are still gated
            // by other bits in the CPLD register that are asserted during
            // frequency tuning.
            set_cpld_field(RXAMP_FORCEON, 1);
            set_cpld_field(RXDEMOD_FORCEON, 1);
            set_cpld_field(RXDRV_FORCEON, 1);
            set_cpld_field(RXMIXER_FORCEON, 1);
            set_cpld_field(RXLO1_FORCEON, 1);
            set_cpld_field(RXLO2_FORCEON, 1);
            set_cpld_field(RXLNA1_FORCEON, 1);
            set_cpld_field(RXLNA2_FORCEON, 1);
            set_cpld_field(TXDRV_FORCEON, 1);
            set_cpld_field(TXMOD_FORCEON, 1);
            set_cpld_field(TXMIXER_FORCEON, 1);
            set_cpld_field(TXLO1_FORCEON, 1);
            set_cpld_field(TXLO2_FORCEON, 1);
            _power_mode = PERFORMANCE;
        }
        else if (mode == "powersave")
        {
            set_cpld_field(RXAMP_FORCEON, 0);
            set_cpld_field(RXDEMOD_FORCEON, 0);
            set_cpld_field(RXDRV_FORCEON, 0);
            set_cpld_field(RXMIXER_FORCEON, 0);
            set_cpld_field(RXLO1_FORCEON, 0);
            set_cpld_field(RXLO2_FORCEON, 0);
            set_cpld_field(RXLNA1_FORCEON, 0);
            set_cpld_field(RXLNA2_FORCEON, 0);
            set_cpld_field(TXDRV_FORCEON, 0);
            set_cpld_field(TXMOD_FORCEON, 0);
            set_cpld_field(TXMIXER_FORCEON, 0);
            set_cpld_field(TXLO1_FORCEON, 0);
            set_cpld_field(TXLO2_FORCEON, 0);
            _power_mode = POWERSAVE;
        }
        write_cpld_reg();
    }

    void set_xcvr_mode(std::string mode)
    {
        // TO DO:  Add implementation
        // The intent is to add behavior based on whether
        // the board is in TX, RX, or full duplex mode
        // to reduce power consumption and RF noise.
        _xcvr_mode = mode;
    }

    /***********************************************************************
    * Variables
    **********************************************************************/
    dboard_iface::sptr _iface;
    ubx_cpld_reg_t _cpld_reg;
    boost::shared_ptr<max287x_synthesizer_iface> _txlo1;
    boost::shared_ptr<max287x_synthesizer_iface> _txlo2;
    boost::shared_ptr<max287x_synthesizer_iface> _rxlo1;
    boost::shared_ptr<max287x_synthesizer_iface> _rxlo2;
    double _tx_gain;
    double _rx_gain;
    double _tx_freq;
    double _txlo1_freq;
    double _txlo2_freq;
    double _rx_freq;
    double _rxlo1_freq;
    double _rxlo2_freq;
    bool _rxlo_locked;
    bool _txlo_locked;
    std::string _rx_ant;
    int _ubx_tx_atten_val;
    int _ubx_rx_atten_val;
    power_mode_t _power_mode;
    std::string _xcvr_mode;
    size_t _rev;
    double _prev_tx_freq;
    double _prev_rx_freq;
    std::map<ubx_gpio_field_id_t,ubx_gpio_field_info_t> _gpio_map;
    ubx_gpio_reg_t _tx_gpio_reg;
    ubx_gpio_reg_t _rx_gpio_reg;
};

/***********************************************************************
 * Register the UBX dboard (min freq, max freq, rx div2, tx div2)
 **********************************************************************/
static dboard_base::sptr make_ubx(dboard_base::ctor_args_t args)
{
    return dboard_base::sptr(new ubx_xcvr(args));
}

UHD_STATIC_BLOCK(reg_ubx_dboards)
{
    dboard_manager::register_dboard(0x0074, 0x0073, &make_ubx, "UBX v0.3");
    dboard_manager::register_dboard(0x0076, 0x0075, &make_ubx, "UBX v0.4");
    dboard_manager::register_dboard(0x0078, 0x0077, &make_ubx, "UBX-40 v1");
    dboard_manager::register_dboard(0x007a, 0x0079, &make_ubx, "UBX-160 v1");
}
