//
// Copyright 2014-17 Ettus Research, A National Instruments Company
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
#include <uhd/types/direction.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/safe_call.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/mutex.hpp>
#include <map>
#include "max287x.hpp"

using namespace uhd;
using namespace uhd::usrp;

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
    uint32_t offset;
    uint32_t mask;
    uint32_t width;
    enum {OUTPUT,INPUT} direction;
    bool is_atr_controlled;
    uint32_t atr_idle;
    uint32_t atr_tx;
    uint32_t atr_rx;
    uint32_t atr_full_duplex;
};

struct ubx_gpio_reg_t
{
    bool dirty;
    uint32_t value;
    uint32_t mask;
    uint32_t ddr;
    uint32_t atr_mask;
    uint32_t atr_idle;
    uint32_t atr_tx;
    uint32_t atr_rx;
    uint32_t atr_full_duplex;
};

struct ubx_cpld_reg_t
{
    void set_field(ubx_cpld_field_id_t field, uint32_t val)
    {
        UHD_ASSERT_THROW(val == (val & 0x1));

        if (val)
            value |= uint32_t(1) << field;
        else
            value &= ~(uint32_t(1) << field);
    }

    uint32_t value;
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
#define fMHz (1000000.0)
static const dboard_id_t UBX_PROTO_V3_TX_ID(0x73);
static const dboard_id_t UBX_PROTO_V3_RX_ID(0x74);
static const dboard_id_t UBX_PROTO_V4_TX_ID(0x75);
static const dboard_id_t UBX_PROTO_V4_RX_ID(0x76);
static const dboard_id_t UBX_V1_40MHZ_TX_ID(0x77);
static const dboard_id_t UBX_V1_40MHZ_RX_ID(0x78);
static const dboard_id_t UBX_V1_160MHZ_TX_ID(0x79);
static const dboard_id_t UBX_V1_160MHZ_RX_ID(0x7A);
static const dboard_id_t UBX_V2_40MHZ_TX_ID(0x7B);
static const dboard_id_t UBX_V2_40MHZ_RX_ID(0x7C);
static const dboard_id_t UBX_V2_160MHZ_TX_ID(0x7D);
static const dboard_id_t UBX_V2_160MHZ_RX_ID(0x7E);
static const dboard_id_t UBX_LP_160MHZ_TX_ID(0x0200);
static const dboard_id_t UBX_LP_160MHZ_RX_ID(0x0201);
static const dboard_id_t UBX_TDD_160MHZ_TX_ID(0x0202);
static const dboard_id_t UBX_TDD_160MHZ_RX_ID(0x0203);
static const freq_range_t ubx_freq_range(10e6, 6.0e9);
static const gain_range_t ubx_tx_gain_range(0, 31.5, double(0.5));
static const gain_range_t ubx_rx_gain_range(0, 31.5, double(0.5));
static const std::vector<std::string> ubx_pgas = boost::assign::list_of("PGA-TX")("PGA-RX");
static const std::vector<std::string> ubx_plls = boost::assign::list_of("TXLO")("RXLO");
static const std::vector<std::string> ubx_tx_antennas = boost::assign::list_of("TX/RX")("CAL");
static const std::vector<std::string> ubx_rx_antennas = boost::assign::list_of("TX/RX")("RX2")("CAL");
static const std::vector<std::string> ubx_power_modes = boost::assign::list_of("performance")("powersave");
static const std::vector<std::string> ubx_xcvr_modes = boost::assign::list_of("FDX")("TX")("TX/RX")("RX");

static const ubx_gpio_field_info_t ubx_proto_gpio_info[] = {
    //Field         Unit                  Offset Mask      Width    Direction                   ATR    IDLE,TX,RX,FDX
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
    //Field         Unit                  Offset Mask      Width    Direction                   ATR    IDLE,TX,RX,FDX
    {SPI_ADDR,      dboard_iface::UNIT_TX,   0,  0x7,        3,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {CPLD_RST_N,    dboard_iface::UNIT_TX,   3,  0x1<<3,     1,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {RX_ANT,        dboard_iface::UNIT_TX,   4,  0x1<<4,     1,  ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {TX_EN_N,       dboard_iface::UNIT_TX,   5,  0x1<<5,     1,  ubx_gpio_field_info_t::INPUT,  true,   1,  0,  1,  0},
    {RX_EN_N,       dboard_iface::UNIT_TX,   6,  0x1<<6,     1,  ubx_gpio_field_info_t::INPUT,  true,   1,  1,  0,  0},
    {TXLO1_SYNC,    dboard_iface::UNIT_TX,   7,  0x1<<7,     1,  ubx_gpio_field_info_t::INPUT,  true,   0,  0,  0,  0},
    {TXLO2_SYNC,    dboard_iface::UNIT_TX,   9,  0x1<<9,     1,  ubx_gpio_field_info_t::INPUT,  true,   0,  0,  0,  0},
    {TX_GAIN,       dboard_iface::UNIT_TX,   10, 0x3F<<10,   10, ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0},
    {RX_LO_LOCKED,  dboard_iface::UNIT_RX,   0,  0x1,        1,  ubx_gpio_field_info_t::OUTPUT, false,  0,  0,  0,  0},
    {TX_LO_LOCKED,  dboard_iface::UNIT_RX,   1,  0x1<<1,     1,  ubx_gpio_field_info_t::OUTPUT, false,  0,  0,  0,  0},
    {RXLO1_SYNC,    dboard_iface::UNIT_RX,   5,  0x1<<5,     1,  ubx_gpio_field_info_t::INPUT,  true,   0,  0,  0,  0},
    {RXLO2_SYNC,    dboard_iface::UNIT_RX,   7,  0x1<<7,     1,  ubx_gpio_field_info_t::INPUT,  true,   0,  0,  0,  0},
    {RX_GAIN,       dboard_iface::UNIT_RX,   10, 0x3F<<10,   10, ubx_gpio_field_info_t::INPUT,  false,  0,  0,  0,  0}
};

/***********************************************************************
 * Macros for routing and writing SPI registers
 **********************************************************************/
#define ROUTE_SPI(iface, dest)  \
    set_gpio_field(SPI_ADDR, dest); \
    write_gpio();

#define WRITE_SPI(iface, val)   \
    iface->write_spi(dboard_iface::UNIT_TX, spi_config_t::EDGE_RISE, val, 32);

/***********************************************************************
 * UBX Class Definition
 **********************************************************************/
class ubx_xcvr : public xcvr_dboard_base
{
public:
    ubx_xcvr(ctor_args_t args) : xcvr_dboard_base(args)
    {
        double bw = 40e6;
        double pfd_freq_max = 25e6;

        ////////////////////////////////////////////////////////////////////
        // Setup GPIO hardware
        ////////////////////////////////////////////////////////////////////
        _iface = get_iface();
        dboard_id_t rx_id = get_rx_id();
        dboard_id_t tx_id = get_tx_id();
        size_t revision = 1;    // default to rev A
        // Get revision if programmed
        const std::string revision_str = get_rx_eeprom().revision;
        if (not revision_str.empty())
        {
            revision = boost::lexical_cast<size_t>(revision_str);
        }
        _high_isolation = false;
        if (rx_id == UBX_PROTO_V3_RX_ID and tx_id == UBX_PROTO_V3_TX_ID) {
            _rev = 0;
        }
        else if (rx_id == UBX_PROTO_V4_RX_ID and tx_id == UBX_PROTO_V4_TX_ID) {
            _rev = 1;
        }
        else if (rx_id == UBX_V1_40MHZ_RX_ID and tx_id == UBX_V1_40MHZ_TX_ID) {
            _rev = 1;
        }
        else if (rx_id == UBX_V2_40MHZ_RX_ID and tx_id == UBX_V2_40MHZ_TX_ID) {
            _rev = 2;
            if (revision >= 4)
            {
                _high_isolation = true;
            }
        }
        else if (rx_id == UBX_V1_160MHZ_RX_ID and tx_id == UBX_V1_160MHZ_TX_ID) {
            bw = 160e6;
            _rev = 1;
        }
        else if (rx_id == UBX_V2_160MHZ_RX_ID and tx_id == UBX_V2_160MHZ_TX_ID) {
            bw = 160e6;
            _rev = 2;
            if (revision >= 4)
            {
                _high_isolation = true;
            }
        }
        else if (rx_id == UBX_LP_160MHZ_RX_ID and tx_id == UBX_LP_160MHZ_TX_ID) {
            // The LP version behaves and looks like a regular UBX-160 v2
            bw = 160e6;
            _rev = 2;
        }
        else if (rx_id == UBX_TDD_160MHZ_RX_ID and tx_id == UBX_TDD_160MHZ_TX_ID) {
            bw = 160e6;
            _rev = 2;
            _high_isolation = true;
        }
        else {
            UHD_THROW_INVALID_CODE_PATH();
        }

        switch(_rev)
        {
        case 0:
            for (size_t i = 0; i < sizeof(ubx_proto_gpio_info) / sizeof(ubx_gpio_field_info_t); i++)
                _gpio_map[ubx_proto_gpio_info[i].id] = ubx_proto_gpio_info[i];
            pfd_freq_max = 25e6;
            break;
        case 1:
        case 2:
            for (size_t i = 0; i < sizeof(ubx_v1_gpio_info) / sizeof(ubx_gpio_field_info_t); i++)
                _gpio_map[ubx_v1_gpio_info[i].id] = ubx_v1_gpio_info[i];
            pfd_freq_max = 50e6;
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
        _rx_target_pfd_freq = pfd_freq_max;
        _tx_target_pfd_freq = pfd_freq_max;
        if (_rev >= 1)
        {
            bool can_set_clock_rate = true;
            // set dboard clock rates to as close to the max PFD freq as possible
            if (_iface->get_clock_rate(dboard_iface::UNIT_RX) > pfd_freq_max)
            {
                std::vector<double> rates = _iface->get_clock_rates(dboard_iface::UNIT_RX);
                double highest_rate = 0.0;
                BOOST_FOREACH(double rate, rates)
                {
                    if (rate <= pfd_freq_max and rate > highest_rate)
                        highest_rate = rate;
                }
                try {
                    _iface->set_clock_rate(dboard_iface::UNIT_RX, highest_rate);
                } catch (const uhd::not_implemented_error &) {
                    UHD_MSG(warning) << "Unable to set dboard clock rate - phase will vary" << std::endl;
                    can_set_clock_rate = false;
                }
                _rx_target_pfd_freq = highest_rate;
            }
            if (can_set_clock_rate and _iface->get_clock_rate(dboard_iface::UNIT_TX) > pfd_freq_max)
            {
                std::vector<double> rates = _iface->get_clock_rates(dboard_iface::UNIT_TX);
                double highest_rate = 0.0;
                BOOST_FOREACH(double rate, rates)
                {
                    if (rate <= pfd_freq_max and rate > highest_rate)
                        highest_rate = rate;
                }
                try {
                    _iface->set_clock_rate(dboard_iface::UNIT_TX, highest_rate);
                } catch (const uhd::not_implemented_error &) {
                    UHD_MSG(warning) << "Unable to set dboard clock rate - phase will vary" << std::endl;
                }
                _tx_target_pfd_freq = highest_rate;
            }
        }
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
        _iface->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_IDLE, _tx_gpio_reg.atr_idle);
        _iface->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_TX_ONLY, _tx_gpio_reg.atr_tx);
        _iface->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_RX_ONLY, _tx_gpio_reg.atr_rx);
        _iface->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_FULL_DUPLEX, _tx_gpio_reg.atr_full_duplex);
        _iface->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_IDLE, _rx_gpio_reg.atr_idle);
        _iface->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_TX_ONLY, _rx_gpio_reg.atr_tx);
        _iface->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_RX_ONLY, _rx_gpio_reg.atr_rx);
        _iface->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_FULL_DUPLEX, _rx_gpio_reg.atr_full_duplex);

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
            _txlo1 = max287x_iface::make<max2870>(boost::bind(&ubx_xcvr::write_spi_regs, this, TXLO1, _1));
            _txlo2 = max287x_iface::make<max2870>(boost::bind(&ubx_xcvr::write_spi_regs, this, TXLO2, _1));
            _rxlo1 = max287x_iface::make<max2870>(boost::bind(&ubx_xcvr::write_spi_regs, this, RXLO1, _1));
            _rxlo2 = max287x_iface::make<max2870>(boost::bind(&ubx_xcvr::write_spi_regs, this, RXLO2, _1));
            std::vector<max287x_iface::sptr> los = boost::assign::list_of(_txlo1)(_txlo2)(_rxlo1)(_rxlo2);
            BOOST_FOREACH(max287x_iface::sptr lo, los)
            {
                lo->set_auto_retune(false);
                lo->set_muxout_mode(max287x_iface::MUXOUT_DLD);
                lo->set_ld_pin_mode(max287x_iface::LD_PIN_MODE_DLD);
            }
        }
        else if (_rev == 1 or _rev == 2)
        {
            _txlo1 = max287x_iface::make<max2871>(boost::bind(&ubx_xcvr::write_spi_regs, this, TXLO1, _1));
            _txlo2 = max287x_iface::make<max2871>(boost::bind(&ubx_xcvr::write_spi_regs, this, TXLO2, _1));
            _rxlo1 = max287x_iface::make<max2871>(boost::bind(&ubx_xcvr::write_spi_regs, this, RXLO1, _1));
            _rxlo2 = max287x_iface::make<max2871>(boost::bind(&ubx_xcvr::write_spi_regs, this, RXLO2, _1));
            std::vector<max287x_iface::sptr> los = boost::assign::list_of(_txlo1)(_txlo2)(_rxlo1)(_rxlo2);
            BOOST_FOREACH(max287x_iface::sptr lo, los)
            {
                lo->set_auto_retune(false);
                //lo->set_cycle_slip_mode(true);  // tried it - caused longer lock times
                lo->set_charge_pump_current(max287x_iface::CHARGE_PUMP_CURRENT_5_12MA);
                lo->set_muxout_mode(max287x_iface::MUXOUT_SYNC);
                lo->set_ld_pin_mode(max287x_iface::LD_PIN_MODE_DLD);
            }
        }
        else
        {
            UHD_THROW_INVALID_CODE_PATH();
        }

        // Initialize CPLD register
        _prev_cpld_value = 0xFFFF;
        _cpld_reg.value = 0;
        write_cpld_reg();

        ////////////////////////////////////////////////////////////////////
        // Register power save properties
        ////////////////////////////////////////////////////////////////////
        get_rx_subtree()->create<std::vector<std::string> >("power_mode/options")
            .set(ubx_power_modes);
        get_rx_subtree()->create<std::string>("power_mode/value")
            .add_coerced_subscriber(boost::bind(&ubx_xcvr::set_power_mode, this, _1))
            .set("performance");
        get_rx_subtree()->create<std::vector<std::string> >("xcvr_mode/options")
            .set(ubx_xcvr_modes);
        get_rx_subtree()->create<std::string>("xcvr_mode/value")
            .add_coerced_subscriber(boost::bind(&ubx_xcvr::set_xcvr_mode, this, _1))
            .set("FDX");
        get_tx_subtree()->create<std::vector<std::string> >("power_mode/options")
            .set(ubx_power_modes);
        get_tx_subtree()->create<std::string>("power_mode/value")
            .add_coerced_subscriber(boost::bind(&uhd::property<std::string>::set, &get_rx_subtree()->access<std::string>("power_mode/value"), _1))
            .set_publisher(boost::bind(&uhd::property<std::string>::get, &get_rx_subtree()->access<std::string>("power_mode/value")));
        get_tx_subtree()->create<std::vector<std::string> >("xcvr_mode/options")
            .set(ubx_xcvr_modes);
        get_tx_subtree()->create<std::string>("xcvr_mode/value")
            .add_coerced_subscriber(boost::bind(&uhd::property<std::string>::set, &get_rx_subtree()->access<std::string>("xcvr_mode/value"), _1))
            .set_publisher(boost::bind(&uhd::property<std::string>::get, &get_rx_subtree()->access<std::string>("xcvr_mode/value")));

        ////////////////////////////////////////////////////////////////////
        // Register TX properties
        ////////////////////////////////////////////////////////////////////
        get_tx_subtree()->create<std::string>("name").set("UBX TX");
        get_tx_subtree()->create<device_addr_t>("tune_args")
            .set(device_addr_t());
        get_tx_subtree()->create<sensor_value_t>("sensors/lo_locked")
            .set_publisher(boost::bind(&ubx_xcvr::get_locked, this, "TXLO"));
        get_tx_subtree()->create<double>("gains/PGA0/value")
            .set_coercer(boost::bind(&ubx_xcvr::set_tx_gain, this, _1)).set(0);
        get_tx_subtree()->create<meta_range_t>("gains/PGA0/range")
            .set(ubx_tx_gain_range);
        get_tx_subtree()->create<double>("freq/value")
            .set_coercer(boost::bind(&ubx_xcvr::set_tx_freq, this, _1))
            .set(ubx_freq_range.start());
        get_tx_subtree()->create<meta_range_t>("freq/range")
            .set(ubx_freq_range);
        get_tx_subtree()->create<std::vector<std::string> >("antenna/options")
            .set(ubx_tx_antennas);
        get_tx_subtree()->create<std::string>("antenna/value")
            .add_coerced_subscriber(boost::bind(&ubx_xcvr::set_tx_ant, this, _1))
            .set(ubx_tx_antennas.at(0));
        get_tx_subtree()->create<std::string>("connection")
            .set("QI");
        get_tx_subtree()->create<bool>("enabled")
            .set(true); //always enabled
        get_tx_subtree()->create<bool>("use_lo_offset")
            .set(false);
        get_tx_subtree()->create<double>("bandwidth/value")
            .set(bw);
        get_tx_subtree()->create<meta_range_t>("bandwidth/range")
            .set(freq_range_t(bw, bw));
        get_tx_subtree()->create<int64_t>("sync_delay")
            .add_coerced_subscriber(boost::bind(&ubx_xcvr::set_sync_delay, this, true, _1))
            .set(-8);

        ////////////////////////////////////////////////////////////////////
        // Register RX properties
        ////////////////////////////////////////////////////////////////////
        get_rx_subtree()->create<std::string>("name").set("UBX RX");
        get_rx_subtree()->create<device_addr_t>("tune_args")
            .set(device_addr_t());
        get_rx_subtree()->create<sensor_value_t>("sensors/lo_locked")
            .set_publisher(boost::bind(&ubx_xcvr::get_locked, this, "RXLO"));
        get_rx_subtree()->create<double>("gains/PGA0/value")
            .set_coercer(boost::bind(&ubx_xcvr::set_rx_gain, this, _1))
            .set(0);
        get_rx_subtree()->create<meta_range_t>("gains/PGA0/range")
            .set(ubx_rx_gain_range);
        get_rx_subtree()->create<double>("freq/value")
            .set_coercer(boost::bind(&ubx_xcvr::set_rx_freq, this, _1))
            .set(ubx_freq_range.start());
        get_rx_subtree()->create<meta_range_t>("freq/range")
            .set(ubx_freq_range);
        get_rx_subtree()->create<std::vector<std::string> >("antenna/options")
            .set(ubx_rx_antennas);
        get_rx_subtree()->create<std::string>("antenna/value")
            .add_coerced_subscriber(boost::bind(&ubx_xcvr::set_rx_ant, this, _1)).set("RX2");
        get_rx_subtree()->create<std::string>("connection")
            .set("IQ");
        get_rx_subtree()->create<bool>("enabled")
            .set(true); //always enabled
        get_rx_subtree()->create<bool>("use_lo_offset")
            .set(false);
        get_rx_subtree()->create<double>("bandwidth/value")
            .set(bw);
        get_rx_subtree()->create<meta_range_t>("bandwidth/range")
            .set(freq_range_t(bw, bw));
        get_rx_subtree()->create<int64_t>("sync_delay")
            .add_coerced_subscriber(boost::bind(&ubx_xcvr::set_sync_delay, this, false, _1))
            .set(-8);
    }

    virtual ~ubx_xcvr(void)
    {
        UHD_SAFE_CALL
        (
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
        )
    }

private:
    enum power_mode_t {PERFORMANCE,POWERSAVE};
    enum xcvr_mode_t {FDX, TDD, TX, RX, FAST_TDD};

    /***********************************************************************
    * Helper Functions
    **********************************************************************/
    void write_spi_reg(spi_dest_t dest, uint32_t value)
    {
        boost::mutex::scoped_lock lock(_spi_mutex);
        ROUTE_SPI(_iface, dest);
        WRITE_SPI(_iface, value);
    }

    void write_spi_regs(spi_dest_t dest, std::vector<uint32_t> values)
    {
        boost::mutex::scoped_lock lock(_spi_mutex);
        ROUTE_SPI(_iface, dest);
        BOOST_FOREACH(uint32_t value, values)
            WRITE_SPI(_iface, value);
    }

    void set_cpld_field(ubx_cpld_field_id_t id, uint32_t value)
    {
        _cpld_reg.set_field(id, value);
    }

    void write_cpld_reg()
    {
        if (_cpld_reg.value != _prev_cpld_value)
        {
            write_spi_reg(CPLD, _cpld_reg.value);
            _prev_cpld_value = _cpld_reg.value;
        }
    }

    void set_gpio_field(ubx_gpio_field_id_t id, uint32_t value)
    {
        // Look up field info
        std::map<ubx_gpio_field_id_t,ubx_gpio_field_info_t>::iterator entry = _gpio_map.find(id);
        if (entry == _gpio_map.end())
            return;
        ubx_gpio_field_info_t field_info = entry->second;
        if (field_info.direction == ubx_gpio_field_info_t::OUTPUT)
            return;
        ubx_gpio_reg_t *reg = (field_info.unit == dboard_iface::UNIT_TX ? &_tx_gpio_reg : &_rx_gpio_reg);
        uint32_t _value = reg->value;
        uint32_t _mask = reg->mask;

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

    uint32_t get_gpio_field(ubx_gpio_field_id_t id)
    {
        // Look up field info
        std::map<ubx_gpio_field_id_t,ubx_gpio_field_info_t>::iterator entry = _gpio_map.find(id);
        if (entry == _gpio_map.end())
            return 0;
        ubx_gpio_field_info_t field_info = entry->second;
        if (field_info.direction == ubx_gpio_field_info_t::INPUT)
        {
            ubx_gpio_reg_t *reg = (field_info.unit == dboard_iface::UNIT_TX ? &_tx_gpio_reg : &_rx_gpio_reg);
            return (reg->value >> field_info.offset) & field_info.mask;
        }

        // Read register
        uint32_t value = _iface->read_gpio(field_info.unit);
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

    void sync_phase(uhd::time_spec_t cmd_time, uhd::direction_t dir)
    {
        // Send phase sync signal only if the command time is set
        if (cmd_time != uhd::time_spec_t(0.0))
        {
            // Delay 400 microseconds to allow LOs to lock
            cmd_time += uhd::time_spec_t(0.0004);

            // Phase synchronization for MAX2871 requires that the sync signal
            // is at least 4/(N*PFD_freq) + 2.6ns before the rising edge of the
            // ref clock and 4/(N*PFD_freq) after the rising edge of the ref clock.
            // Since the ref clock, the radio clock, and the VITA time are all
            // synchronized to the 10 MHz clock, use the time spec to move
            // the rising edge of the sync signal away from the 10 MHz edge,
            // which will move it away from the ref clock edge by the same amount.
            // Since the MAX2871 requires the ref freq and PFD freq be the same
            // for phase synchronization, the dboard clock rate is used as the PFD
            // freq and the worst case value of 20 is used for the N value to
            // calculate the offset.
            double pfd_freq = _iface->get_clock_rate(dir == TX_DIRECTION ? dboard_iface::UNIT_TX : dboard_iface::UNIT_RX);
            double tick_rate = _iface->get_codec_rate(dir == TX_DIRECTION ? dboard_iface::UNIT_TX : dboard_iface::UNIT_RX);
            int64_t ticks = cmd_time.to_ticks(tick_rate);
            ticks -= ticks % (int64_t)(tick_rate / 10e6);            // align to 10 MHz clock
            ticks += dir == TX_DIRECTION ? _tx_sync_delay : _rx_sync_delay;
            ticks += std::ceil(tick_rate*4/(20*pfd_freq));  // add required offset (using worst case N value of 20)
            cmd_time = uhd::time_spec_t::from_ticks(ticks, tick_rate);
            _iface->set_command_time(cmd_time);

            // Assert SYNC
            ubx_gpio_field_info_t lo1_field_info = _gpio_map.find(dir == TX_DIRECTION ? TXLO1_SYNC : RXLO1_SYNC)->second;
            ubx_gpio_field_info_t lo2_field_info = _gpio_map.find(dir == TX_DIRECTION ? TXLO2_SYNC : RXLO2_SYNC)->second;
            uint16_t value = (1 << lo1_field_info.offset) | (1 << lo2_field_info.offset);
            uint16_t mask = lo1_field_info.mask | lo2_field_info.mask;
            dboard_iface::unit_t unit = lo1_field_info.unit;
            UHD_ASSERT_THROW(lo1_field_info.unit == lo2_field_info.unit);
            _iface->set_atr_reg(unit, gpio_atr::ATR_REG_IDLE, value, mask);
            cmd_time += uhd::time_spec_t(1/pfd_freq);
            _iface->set_command_time(cmd_time);
            _iface->set_atr_reg(unit, gpio_atr::ATR_REG_TX_ONLY, value, mask);
            cmd_time += uhd::time_spec_t(1/pfd_freq);
            _iface->set_command_time(cmd_time);
            _iface->set_atr_reg(unit, gpio_atr::ATR_REG_RX_ONLY, value, mask);
            cmd_time += uhd::time_spec_t(1/pfd_freq);
            _iface->set_command_time(cmd_time);
            _iface->set_atr_reg(unit, gpio_atr::ATR_REG_FULL_DUPLEX, value, mask);

            // De-assert SYNC
            // Head of line blocking means the command time does not need to be set.
            _iface->set_atr_reg(unit, gpio_atr::ATR_REG_IDLE, 0, mask);
            _iface->set_atr_reg(unit, gpio_atr::ATR_REG_TX_ONLY, 0, mask);
            _iface->set_atr_reg(unit, gpio_atr::ATR_REG_RX_ONLY, 0, mask);
            _iface->set_atr_reg(unit, gpio_atr::ATR_REG_FULL_DUPLEX, 0, mask);
        }
    }

    /***********************************************************************
     * Board Control Handling
     **********************************************************************/
    sensor_value_t get_locked(const std::string &pll_name)
    {
        boost::mutex::scoped_lock lock(_mutex);
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
        boost::mutex::scoped_lock lock(_mutex);
        //validate input
        assert_has(ubx_rx_antennas, ant, "ubx rx antenna name");

        // There can be long transients on TX, so force on the TX PA
        // except when in powersave mode (to save power) or on early
        // boards that had lower TX-RX isolation when the RX antenna
        // is set to TX/RX (to prevent higher noise floor on RX).
        // Setting the xcvr_mode to TDD will force on the PA when
        // not in powersave mode regardless of the board revision.
        if (ant == "TX/RX")
        {
            set_gpio_field(RX_ANT, 0);
            // Force on TX PA for boards with high isolation or if the user sets the TDD mode
            set_cpld_field(TXDRV_FORCEON, (_power_mode == POWERSAVE ? 0 : _high_isolation or _xcvr_mode == TDD ? 1 : 0));
        } else {
            set_gpio_field(RX_ANT, 1);
            set_cpld_field(TXDRV_FORCEON, (_power_mode == POWERSAVE ? 0 : 1));   // Keep PA on
        }
        write_gpio();
        write_cpld_reg();
    }

    /***********************************************************************
     * Gain Handling
     **********************************************************************/
    double set_tx_gain(double gain)
    {
        boost::mutex::scoped_lock lock(_mutex);
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
        boost::mutex::scoped_lock lock(_mutex);
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
        boost::mutex::scoped_lock lock(_mutex);
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
        double target_pfd_freq = _tx_target_pfd_freq;
        if (is_int_n and tune_args.has_key("int_n_step"))
        {
            target_pfd_freq = tune_args.cast<double>("int_n_step", _tx_target_pfd_freq);
            if (target_pfd_freq > _tx_target_pfd_freq)
            {
                UHD_MSG(warning)
                    << boost::format("Requested int_n_step of %f MHz too large, clipping to %f MHz")
                    % (target_pfd_freq/1e6)
                    % (_tx_target_pfd_freq/1e6)
                    << std::endl;
                target_pfd_freq = _tx_target_pfd_freq;
            }
        }

        // Clip the frequency to the valid range
        freq = ubx_freq_range.clip(freq);

        // Power up/down LOs
        if (_txlo1->is_shutdown())
            _txlo1->power_up();
        if (_txlo2->is_shutdown() and (_power_mode == PERFORMANCE or freq < (500*fMHz)))
            _txlo2->power_up();
        else if (freq >= 500*fMHz and _power_mode == POWERSAVE)
            _txlo2->shutdown();

        // Set up LOs for phase sync if command time is set
        uhd::time_spec_t cmd_time = _iface->get_command_time();
        if (cmd_time != uhd::time_spec_t(0.0))
        {
            _txlo1->config_for_sync(true);
            if (not _txlo2->is_shutdown())
                _txlo2->config_for_sync(true);
        }
        else
        {
            _txlo1->config_for_sync(false);
            if (not _txlo2->is_shutdown())
                _txlo2->config_for_sync(false);
        }

        // Set up registers for the requested frequency
        if (freq < (500*fMHz))
        {
            set_cpld_field(TXLO1_FSEL3, 0);
            set_cpld_field(TXLO1_FSEL2, 1);
            set_cpld_field(TXLO1_FSEL1, 0);
            set_cpld_field(TXLB_SEL, 1);
            set_cpld_field(TXHB_SEL, 0);
            // Set LO1 to IF of 2100 MHz (offset from RX IF to reduce leakage)
            freq_lo1 = _txlo1->set_frequency(2100*fMHz, ref_freq, target_pfd_freq, is_int_n);
            _txlo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
            // Set LO2 to IF minus desired frequency
            freq_lo2 = _txlo2->set_frequency(freq_lo1 - freq, ref_freq, target_pfd_freq, is_int_n);
            _txlo2->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
        }
        else if ((freq >= (500*fMHz)) && (freq <= (800*fMHz)))
        {
            set_cpld_field(TXLO1_FSEL3, 0);
            set_cpld_field(TXLO1_FSEL2, 0);
            set_cpld_field(TXLO1_FSEL1, 1);
            set_cpld_field(TXLB_SEL, 0);
            set_cpld_field(TXHB_SEL, 1);
            freq_lo1 = _txlo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
            _txlo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
        }
        else if ((freq > (800*fMHz)) && (freq <= (1000*fMHz)))
        {
            set_cpld_field(TXLO1_FSEL3, 0);
            set_cpld_field(TXLO1_FSEL2, 0);
            set_cpld_field(TXLO1_FSEL1, 1);
            set_cpld_field(TXLB_SEL, 0);
            set_cpld_field(TXHB_SEL, 1);
            freq_lo1 = _txlo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
            _txlo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
        }
        else if ((freq > (1000*fMHz)) && (freq <= (2200*fMHz)))
        {
            set_cpld_field(TXLO1_FSEL3, 0);
            set_cpld_field(TXLO1_FSEL2, 1);
            set_cpld_field(TXLO1_FSEL1, 0);
            set_cpld_field(TXLB_SEL, 0);
            set_cpld_field(TXHB_SEL, 1);
            freq_lo1 = _txlo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
            _txlo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
        }
        else if ((freq > (2200*fMHz)) && (freq <= (2500*fMHz)))
        {
            set_cpld_field(TXLO1_FSEL3, 0);
            set_cpld_field(TXLO1_FSEL2, 1);
            set_cpld_field(TXLO1_FSEL1, 0);
            set_cpld_field(TXLB_SEL, 0);
            set_cpld_field(TXHB_SEL, 1);
            freq_lo1 = _txlo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
            _txlo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
        }
        else if ((freq > (2500*fMHz)) && (freq <= (6000*fMHz)))
        {
            set_cpld_field(TXLO1_FSEL3, 1);
            set_cpld_field(TXLO1_FSEL2, 0);
            set_cpld_field(TXLO1_FSEL1, 0);
            set_cpld_field(TXLB_SEL, 0);
            set_cpld_field(TXHB_SEL, 1);
            freq_lo1 = _txlo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
            _txlo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
        }

        // To reduce the number of commands issued to the device, write to the
        // SPI destination already addressed first.  This avoids the writes to
        // the GPIO registers to route the SPI to the same destination.
        switch (get_gpio_field(SPI_ADDR))
        {
        case TXLO1:
            _txlo1->commit();
            if (freq < (500*fMHz)) _txlo2->commit();
            write_cpld_reg();
            break;
        case TXLO2:
            if (freq < (500*fMHz)) _txlo2->commit();
            _txlo1->commit();
            write_cpld_reg();
        break;
        default:
            write_cpld_reg();
            _txlo1->commit();
            if (freq < (500*fMHz)) _txlo2->commit();
            break;
        }

        if (cmd_time != uhd::time_spec_t(0.0) and _txlo1->can_sync())
        {
            sync_phase(cmd_time, TX_DIRECTION);
        }

        _tx_freq = freq_lo1 - freq_lo2;
        _txlo1_freq = freq_lo1;
        _txlo2_freq = freq_lo2;

        UHD_LOGV(rarely) << boost::format("UBX TX: the actual frequency is %f MHz") % (_tx_freq/1e6) << std::endl;

        return _tx_freq;
    }

    double set_rx_freq(double freq)
    {
        boost::mutex::scoped_lock lock(_mutex);
        double freq_lo1 = 0.0;
        double freq_lo2 = 0.0;
        double ref_freq = _iface->get_clock_rate(dboard_iface::UNIT_RX);
        bool is_int_n = false;

        UHD_LOGV(rarely) << boost::format("UBX RX: the requested frequency is %f MHz") % (freq/1e6) << std::endl;

        property_tree::sptr subtree = this->get_rx_subtree();
        device_addr_t tune_args = subtree->access<device_addr_t>("tune_args").get();
        is_int_n = boost::iequals(tune_args.get("mode_n",""), "integer");
        double target_pfd_freq = _rx_target_pfd_freq;
        if (is_int_n and tune_args.has_key("int_n_step"))
        {
            target_pfd_freq = tune_args.cast<double>("int_n_step", _rx_target_pfd_freq);
            if (target_pfd_freq > _rx_target_pfd_freq)
            {
                UHD_MSG(warning)
                    << boost::format("Requested int_n_step of %f Mhz too large, clipping to %f MHz")
                    % (target_pfd_freq/1e6)
                    % (_rx_target_pfd_freq/1e6)
                    << std::endl;
                target_pfd_freq = _rx_target_pfd_freq;
            }
        }

        // Clip the frequency to the valid range
        freq = ubx_freq_range.clip(freq);

        // Power up/down LOs
        if (_rxlo1->is_shutdown())
            _rxlo1->power_up();
        if (_rxlo2->is_shutdown() and (_power_mode == PERFORMANCE or freq < 500*fMHz))
            _rxlo2->power_up();
        else if (freq >= 500*fMHz and _power_mode == POWERSAVE)
            _rxlo2->shutdown();

        // Set up LOs for phase sync if command time is set
        uhd::time_spec_t cmd_time = _iface->get_command_time();
        if (cmd_time != uhd::time_spec_t(0.0))
        {
            _rxlo1->config_for_sync(true);
            if (not _rxlo2->is_shutdown())
                _rxlo2->config_for_sync(true);
        }
        else
        {
            _rxlo1->config_for_sync(false);
            if (not _rxlo2->is_shutdown())
                _rxlo2->config_for_sync(false);
        }

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
            // Set LO1 to IF of 2380 MHz (2440 MHz filter center minus 60 MHz offset to minimize LO leakage)
            freq_lo1 = _rxlo1->set_frequency(2380*fMHz, ref_freq, target_pfd_freq, is_int_n);
            _rxlo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
            // Set LO2 to IF minus desired frequency
            freq_lo2 = _rxlo2->set_frequency(freq_lo1 - freq, ref_freq, target_pfd_freq, is_int_n);
            _rxlo2->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
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
            // Set LO1 to IF of 2440 (center of filter)
            freq_lo1 = _rxlo1->set_frequency(2440*fMHz, ref_freq, target_pfd_freq, is_int_n);
            _rxlo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
            // Set LO2 to IF minus desired frequency
            freq_lo2 = _rxlo2->set_frequency(freq_lo1 - freq, ref_freq, target_pfd_freq, is_int_n);
            _rxlo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
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
            freq_lo1 = _rxlo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
            _rxlo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
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
            freq_lo1 = _rxlo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
            _rxlo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
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
            freq_lo1 = _rxlo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
            _rxlo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
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
            freq_lo1 = _rxlo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
            _rxlo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
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
            freq_lo1 = _rxlo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
            _rxlo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
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
            freq_lo1 = _rxlo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
            _rxlo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
        }

        // To reduce the number of commands issued to the device, write to the
        // SPI destination already addressed first.  This avoids the writes to
        // the GPIO registers to route the SPI to the same destination.
        switch (get_gpio_field(SPI_ADDR))
        {
        case RXLO1:
            _rxlo1->commit();
            if (freq < (500*fMHz)) _rxlo2->commit();
            write_cpld_reg();
            break;
        case RXLO2:
            if (freq < (500*fMHz)) _rxlo2->commit();
            _rxlo1->commit();
            write_cpld_reg();
        break;
        default:
            write_cpld_reg();
            _rxlo1->commit();
            if (freq < (500*fMHz)) _rxlo2->commit();
            break;
        }

        if (cmd_time != uhd::time_spec_t(0.0) and _rxlo1->can_sync())
        {
            sync_phase(cmd_time, RX_DIRECTION);
        }

        _rx_freq = freq_lo1 - freq_lo2;
        _rxlo1_freq = freq_lo1;
        _rxlo2_freq = freq_lo2;

        UHD_LOGV(rarely) << boost::format("UBX RX: the actual frequency is %f MHz") % (_rx_freq/1e6) << std::endl;

        return _rx_freq;
    }

    /***********************************************************************
    * Setting Modes
    **********************************************************************/
    void set_power_mode(std::string mode)
    {
        boost::mutex::scoped_lock lock(_mutex);
        if (mode == "performance")
        {
            // performance mode attempts to reduce tuning and settling time
            // as much as possible without adding noise.

            // RXLNA2 has a ~100ms warm up time, so the LNAs are forced on
            // here to reduce the settling time as much as possible.  The
            // force on signals are gated by the LNA selection so the LNAs
            // are turned on/off during tuning.  Unfortunately, that means
            // there is still a long settling time when tuning from the high
            // band (>1.5 GHz) to the low band (<1.5 GHz).
            set_cpld_field(RXLNA1_FORCEON, 1);
            set_cpld_field(RXLNA2_FORCEON, 1);

            // Placeholders in case some components need to be forced on to
            // reduce settling time.  Note that some FORCEON lines are still gated
            // by other bits in the CPLD register and are asserted during
            // frequency tuning.
            set_cpld_field(RXAMP_FORCEON, 1);
            set_cpld_field(RXDEMOD_FORCEON, 1);
            set_cpld_field(RXDRV_FORCEON, 1);
            set_cpld_field(RXMIXER_FORCEON, 0);
            set_cpld_field(RXLO1_FORCEON, 1);
            set_cpld_field(RXLO2_FORCEON, 1);
            /*
            //set_cpld_field(TXDRV_FORCEON, 1);  // controlled by RX antenna selection
            set_cpld_field(TXMOD_FORCEON, 0);
            set_cpld_field(TXMIXER_FORCEON, 0);
            set_cpld_field(TXLO1_FORCEON, 0);
            set_cpld_field(TXLO2_FORCEON, 0);
            */
            write_cpld_reg();

            _power_mode = PERFORMANCE;
        }
        else if (mode == "powersave")
        {
            // powersave mode attempts to use the least amount of power possible
            // by powering on components only when needed.  Longer tuning and
            // settling times are expected.

            // Clear the LNA force on bits.
            set_cpld_field(RXLNA1_FORCEON, 0);
            set_cpld_field(RXLNA2_FORCEON, 0);

            /*
            // Placeholders in case other force on bits need to be set or cleared.
            set_cpld_field(RXAMP_FORCEON, 0);
            set_cpld_field(RXDEMOD_FORCEON, 0);
            set_cpld_field(RXDRV_FORCEON, 0);
            set_cpld_field(RXMIXER_FORCEON, 0);
            set_cpld_field(RXLO1_FORCEON, 0);
            set_cpld_field(RXLO2_FORCEON, 0);
            //set_cpld_field(TXDRV_FORCEON, 1);  // controlled by RX antenna selection
            set_cpld_field(TXMOD_FORCEON, 0);
            set_cpld_field(TXMIXER_FORCEON, 0);
            set_cpld_field(TXLO1_FORCEON, 0);
            set_cpld_field(TXLO2_FORCEON, 0);
            */

            write_cpld_reg();

            _power_mode = POWERSAVE;
        }
    }

    void set_xcvr_mode(std::string mode)
    {
        // TO DO:  Add implementation
        // The intent is to add behavior based on whether
        // the board is in TX, RX, or full duplex mode
        // to reduce power consumption and RF noise.
        boost::to_upper(mode);
        if (mode == "FDX")
        {
            _xcvr_mode = FDX;
        }
        else if (mode == "TDD")
        {
            _xcvr_mode = TDD;
            set_cpld_field(TXDRV_FORCEON, 1);
            write_cpld_reg();
        }
        else if (mode == "TX")
        {
            _xcvr_mode = TX;
        }
        else if (mode == "RX")
        {
            _xcvr_mode = RX;
        }
        else
        {
            throw uhd::value_error("invalid xcvr_mode");
        }
    }

    void set_sync_delay(bool is_tx, int64_t value)
    {
        if (is_tx)
            _tx_sync_delay = value;
        else
            _rx_sync_delay = value;
    }

    /***********************************************************************
    * Variables
    **********************************************************************/
    dboard_iface::sptr _iface;
    boost::mutex _spi_mutex;
    boost::mutex _mutex;
    ubx_cpld_reg_t _cpld_reg;
    uint32_t _prev_cpld_value;
    std::map<ubx_gpio_field_id_t,ubx_gpio_field_info_t> _gpio_map;
    boost::shared_ptr<max287x_iface> _txlo1;
    boost::shared_ptr<max287x_iface> _txlo2;
    boost::shared_ptr<max287x_iface> _rxlo1;
    boost::shared_ptr<max287x_iface> _rxlo2;
    double _tx_target_pfd_freq;
    double _rx_target_pfd_freq;
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
    xcvr_mode_t _xcvr_mode;
    size_t _rev;
    ubx_gpio_reg_t _tx_gpio_reg;
    ubx_gpio_reg_t _rx_gpio_reg;
    int64_t _tx_sync_delay;
    int64_t _rx_sync_delay;
    bool _high_isolation;
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
    dboard_manager::register_dboard(UBX_PROTO_V3_RX_ID,  UBX_PROTO_V3_TX_ID,  &make_ubx, "UBX v0.3");
    dboard_manager::register_dboard(UBX_PROTO_V4_RX_ID,  UBX_PROTO_V4_TX_ID,  &make_ubx, "UBX v0.4");
    dboard_manager::register_dboard(UBX_V1_40MHZ_RX_ID,  UBX_V1_40MHZ_TX_ID,  &make_ubx, "UBX-40 v1");
    dboard_manager::register_dboard(UBX_V1_160MHZ_RX_ID, UBX_V1_160MHZ_TX_ID, &make_ubx, "UBX-160 v1");
    dboard_manager::register_dboard(UBX_V2_40MHZ_RX_ID,  UBX_V2_40MHZ_TX_ID,  &make_ubx, "UBX-40 v2");
    dboard_manager::register_dboard(UBX_V2_160MHZ_RX_ID, UBX_V2_160MHZ_TX_ID, &make_ubx, "UBX-160 v2");
    dboard_manager::register_dboard(UBX_LP_160MHZ_RX_ID, UBX_LP_160MHZ_TX_ID, &make_ubx, "UBX-160-LP");
    dboard_manager::register_dboard(UBX_TDD_160MHZ_RX_ID, UBX_TDD_160MHZ_TX_ID, &make_ubx, "UBX-TDD");
}
