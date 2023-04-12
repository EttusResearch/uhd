//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "usrp2/clock_ctrl.hpp"
#include "ad9510_regs.hpp"
#include "ad9516_regs.hpp"
#include "kintex7sdr_clk_regs.hpp"
#include "kintex7sdr_regs.hpp" //spi slave constants
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <cmath>
#include <cstdint>

using namespace uhd;

static const bool enb_test_clk = false;

/*!
 * A kintex7sdr clock control specific to the ad9510 and ad9516 ic.
 */
class kintex7sdr_clock_ctrl_impl : public usrp2_clock_ctrl {
public:
    kintex7sdr_clock_ctrl_impl(usrp2_iface::sptr iface, uhd::spi_iface::sptr spiface) {
        _iface = iface;
        _spiface = spiface;
        clk_regs = kintex7sdr_clk_regs_t(_iface->get_rev());

        _ad9510_regs.cp_current_setting = ad9510_regs_t::CP_CURRENT_SETTING_3_0MA;
        // this->write_reg(clk_regs.pll_3);

        // Setup the clock registers to 100MHz:
        //  This was already done by the firmware (or the host couldnt communicate).
        //  We could remove this part, and just leave it to the firmware.
        //  But why not leave it in for those who want to mess with clock settings?
        //  100mhz = 10mhz/R * (P*B + A)

        _ad9510_regs.pll_power_down = ad9510_regs_t::PLL_POWER_DOWN_NORMAL;
        _ad9510_regs.prescaler_value = ad9510_regs_t::PRESCALER_VALUE_DIV2;

        _ad9510_regs.acounter = 0;

        _ad9510_regs.bcounter_msb = 0;
        _ad9510_regs.bcounter_lsb = 5;

        _ad9510_regs.ref_counter_msb = 0;
        _ad9510_regs.ref_counter_lsb = 1; // r divider = 1

        _ad9516_regs.pll_power_down = ad9516_regs_t::PLL_POWER_DOWN_NORMAL;
        _ad9516_regs.prescaler_value = ad9516_regs_t::PRESCALER_VALUE_DIV2;

        _ad9516_regs.acounter = 0;

        _ad9516_regs.bcounter_msb = 0;
        _ad9516_regs.bcounter_lsb = 5;

        _ad9516_regs.ref_counter_msb = 0;
        _ad9516_regs.ref_counter_lsb = 1; // r divider = 2


        /* regs will be updated in commands below */

        switch (_iface->get_rev()) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                this->write_reg(clk_regs.pll_PFD);
                this->write_reg(clk_regs.pll_1);
                break;
            default:
                this->write_reg(clk_regs.pll_3);
                this->write_reg(clk_regs.pll_4);
        }

        this->write_reg(clk_regs.acounter);
        this->write_reg(clk_regs.bcounter_msb);
        this->write_reg(clk_regs.bcounter_lsb);
        this->write_reg(clk_regs.ref_counter_msb);
        this->write_reg(clk_regs.ref_counter_lsb);

        this->enable_external_ref(false);
        this->enable_rx_dboard_clock(false);
        this->enable_tx_dboard_clock(false);
        this->enable_mimo_clock_out(false);

        /* private clock enables, must be set here */
        this->enable_dac_clock(true);
        this->enable_adc_clock(true);
        this->enable_test_clock(enb_test_clk);
    }

    ~kintex7sdr_clock_ctrl_impl(void) override {
        UHD_SAFE_CALL(
                // power down clock outputs
                this->enable_external_ref(false);
        this->enable_rx_dboard_clock(false);
        this->enable_tx_dboard_clock(false);
        this->enable_dac_clock(false);
        this->enable_adc_clock(false);
        this->enable_mimo_clock_out(false);
        this->enable_test_clock(false);)
    }

    void enable_mimo_clock_out(bool enb) override {
        // calculate the low and high dividers
        size_t divider = size_t(this->get_master_clock_rate() / 10e6);
        size_t high = divider / 2;
        size_t low = divider - high;

        switch (clk_regs.exp) {
            case 2: // U2 rev 3
                _ad9510_regs.power_down_lvpecl_out2 =
                        enb ? ad9510_regs_t::POWER_DOWN_LVPECL_OUT2_NORMAL
                            : ad9510_regs_t::POWER_DOWN_LVPECL_OUT2_SAFE_PD;
                _ad9510_regs.output_level_lvpecl_out2 =
                        ad9510_regs_t::OUTPUT_LEVEL_LVPECL_OUT2_810MV;
                // set the registers (divider - 1)
                _ad9510_regs.divider_low_cycles_out2 = uhd::narrow_cast<uint8_t>(low - 1);
                _ad9510_regs.divider_high_cycles_out2 =
                        uhd::narrow_cast<uint8_t>(high - 1);
                _ad9510_regs.bypass_divider_out2 = 0;
                break;

            case 5: // U2 rev 4
                _ad9510_regs.power_down_lvds_cmos_out5 = enb ? 0 : 1;
                _ad9510_regs.lvds_cmos_select_out5 =
                        ad9510_regs_t::LVDS_CMOS_SELECT_OUT5_LVDS;
                _ad9510_regs.output_level_lvds_out5 =
                        ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT5_1_75MA;
                // set the registers (divider - 1)
                _ad9510_regs.divider_low_cycles_out5 = uhd::narrow_cast<uint8_t>(low - 1);
                _ad9510_regs.divider_high_cycles_out5 =
                        uhd::narrow_cast<uint8_t>(high - 1);
                _ad9510_regs.bypass_divider_out5 = 0;
                break;

            case 6: // U2+
                _ad9510_regs.power_down_lvds_cmos_out6 = enb ? 0 : 1;
                _ad9510_regs.lvds_cmos_select_out6 =
                        ad9510_regs_t::LVDS_CMOS_SELECT_OUT6_LVDS;
                _ad9510_regs.output_level_lvds_out6 =
                        ad9510_regs_t::OUTPUT_LEVEL_LVDS_OUT6_1_75MA;
                // set the registers (divider - 1)
                _ad9510_regs.divider_low_cycles_out6 = uhd::narrow_cast<uint8_t>(low - 1);
                _ad9510_regs.divider_high_cycles_out6 =
                        uhd::narrow_cast<uint8_t>(high - 1);
                _ad9510_regs.bypass_divider_out5 = 0;
                break;

            default:
                break;
        }
        this->write_reg(clk_regs.output(clk_regs.exp));
        this->write_reg(clk_regs.div_lo(clk_regs.exp));
        this->update_regs();
    }

    // uses output clock 7 (cmos)
    void enable_rx_dboard_clock(bool enb) override {
        switch (_iface->get_rev()) {
            case kintex7sdr_iface::USRP_N210_XK:
                _ad9516_regs.power_down_lvds_cmos_out6 = enb ? 0 : 1;
                _ad9516_regs.lvds_cmos_select_out6 =
                        ad9516_regs_t::LVDS_CMOS_SELECT_OUT6_LVDS;
                _ad9516_regs.output_level_lvds_out6 =
                        ad9516_regs_t::OUTPUT_LEVEL_LVDS_OUT6_1_75MA;
                this->write_reg(clk_regs.output(clk_regs.rx_db));
                break;
            case kintex7sdr_iface::USRP_N210_XA:
                _ad9516_regs.power_down_lvds_cmos_out6 = enb ? 0 : 1;
                _ad9516_regs.lvds_cmos_select_out6 =
                        ad9516_regs_t::LVDS_CMOS_SELECT_OUT6_LVDS;
                _ad9516_regs.output_level_lvds_out6 =
                        ad9516_regs_t::OUTPUT_LEVEL_LVDS_OUT6_3_5MA;
                this->write_reg(clk_regs.output(clk_regs.rx_db));
                break;
            default:
                throw uhd::not_implemented_error("enable_rx_dboard_clock: unknown hardware version");
        }
        this->update_regs();
    }

    void set_rate_rx_dboard_clock(double rate) override {
        assert_has(get_rates_rx_dboard_clock(), rate, "rx dboard clock rate");
        size_t divider = size_t(get_master_clock_rate() / rate);

        // calculate the low and high dividers
        size_t high = (int) divider / 2;
        size_t low = divider - high;

        switch (_iface->get_rev()) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                // bypass when the divider ratio is one
                _ad9516_regs.divider3_bypass = (divider == 1) ? 1 : 0;
                _ad9516_regs.divider3_low_cycles = low - 1;
                _ad9516_regs.divider3_high_cycles = high - 1;

                // ad9516_write_reg(0x19C, 0x20);  // Bypass 3.2 divider
                // UHD_MSG(status) << boost::format("OLOLOLOL: %x") %divider << std::endl;
                // UHD_MSG(status) << boost::format("OLOLOLOL: %x") %data << std::endl;

                this->write_reg(clk_regs.div_lo(clk_regs.rx_db) + 0x03);
                break;
            default:
                throw uhd::not_implemented_error("set_rate_rx_dboard_clock: unknown hardware version");
        }

        // write the registers
        this->write_reg(clk_regs.div_lo(clk_regs.rx_db));
        this->write_reg(clk_regs.div_hi(clk_regs.rx_db));
        this->update_regs();

        // ad9516_write_reg(0x19C, 0x30);  // Bypass 3.2 divider
        // ad9516_write_reg(0x199, 0x00);  // Set 3.1 divider to 2
        // ad9516_write_reg(0x232, 0x01);i
    }

    std::vector<double> get_rates_rx_dboard_clock(void) override {
        std::vector<double> rates;
        for (size_t i = 1; i <= 16 + 16; i++)
            rates.push_back(get_master_clock_rate() / i);
        return rates;
    }

    // uses output clock 6 (cmos) on USRP2, output clock 5 (cmos) on N200/N210 r3,
    // and output clock 5 (lvds) on N200/N210 r4
    void enable_tx_dboard_clock(bool enb) override {
        switch (_iface->get_rev()) {
            case kintex7sdr_iface::USRP_N210_XK:
                _ad9516_regs.power_down_lvds_cmos_out7 = enb ? 0 : 1;
                _ad9516_regs.lvds_cmos_select_out7 =
                        ad9516_regs_t::LVDS_CMOS_SELECT_OUT7_LVDS;
                _ad9516_regs.output_level_lvds_out7 =
                        ad9516_regs_t::OUTPUT_LEVEL_LVDS_OUT7_1_75MA;
                break;
            case kintex7sdr_iface::USRP_N210_XA:
                _ad9516_regs.power_down_lvds_cmos_out7 = enb ? 0 : 1;
                _ad9516_regs.lvds_cmos_select_out7 =
                        ad9516_regs_t::LVDS_CMOS_SELECT_OUT7_LVDS;
                _ad9516_regs.output_level_lvds_out7 =
                        ad9516_regs_t::OUTPUT_LEVEL_LVDS_OUT7_3_5MA;
                break;
            default:
                throw uhd::not_implemented_error("enable_tx_dboard_clock: unknown hardware version");
        }

        this->write_reg(clk_regs.output(clk_regs.tx_db));
        this->update_regs();
    }

    void set_rate_tx_dboard_clock(double rate) override {
        assert_has(get_rates_tx_dboard_clock(), rate, "tx dboard clock rate");
        size_t divider = size_t(get_master_clock_rate() / rate);

        size_t high = divider / 2;
        size_t low = divider - high;

        switch (_iface->get_rev()) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                // bypass when the divider ratio is one
                _ad9516_regs.divider3_bypass = (divider == 1) ? 1 : 0;
                _ad9516_regs.divider3_low_cycles = low - 1;
                _ad9516_regs.divider3_high_cycles = high - 1;
                this->write_reg(clk_regs.div_lo(clk_regs.rx_db) + 0x03);
                break;
            default:
                throw uhd::not_implemented_error("set_rate_tx_dboard_clock: unknown hardware version");
        }

        // write the registers
        this->write_reg(clk_regs.div_hi(clk_regs.tx_db));
        this->write_reg(clk_regs.div_lo(clk_regs.tx_db));
        this->update_regs();
    }

    std::vector<double> get_rates_tx_dboard_clock(void) override {
        return get_rates_rx_dboard_clock(); // same master clock, same dividers...
    }

    void enable_test_clock(bool enb) override {
        switch (_iface->get_rev()) {
            case kintex7sdr_iface::USRP_N210_XK:
                break;
            case kintex7sdr_iface::USRP_N210_XA:
                _ad9516_regs.power_down_lvpecl_out5 =
                        (enb) ? ad9516_regs_t::POWER_DOWN_LVPECL_OUT5_NORMAL
                              : ad9516_regs_t::POWER_DOWN_LVPECL_OUT5_SAFE_PD;
                _ad9516_regs.output_level_lvpecl_out5 =
                        ad9516_regs_t::OUTPUT_LEVEL_LVPECL_OUT5_780MV;
                _ad9516_regs.divider2_bypass = 1;
                break;
            default:
                throw uhd::not_implemented_error("set_rate_tx_dboard_clock: unknown hardware version");
        }
    }

    /*!
     * If we are to use an external reference, enable the charge pump.
     * \param enb true to enable the CP
     */
    void enable_external_ref(bool enb) override {
        switch (_iface->get_rev()) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                if (enb) {
                    _ad9516_regs.charge_pump_mode =
                            ad9516_regs_t::CHARGE_PUMP_MODE_NORMAL;
                    _ad9516_regs.cp_out_set_half_vss = 0;
                    _ad9516_regs.ref1_power_on = ad9516_regs_t::REF1_POWER_ON_ON;
                } else {
                    _ad9516_regs.charge_pump_mode =
                            ad9516_regs_t::CHARGE_PUMP_MODE_3STATE;
                    _ad9516_regs.cp_out_set_half_vss = 1;
                    _ad9516_regs.ref1_power_on = ad9516_regs_t::REF1_POWER_ON_OFF;
                }
                _ad9516_regs.status_pin_control = ad9516_regs_t::STATUS_PIN_CONTROL_DLD;
                _ad9516_regs.pfd_polarity = ad9516_regs_t::PFD_POLARITY_POS;
                this->write_reg(clk_regs.pll_1);
                this->write_reg(clk_regs.pll_2);
                this->write_reg(clk_regs.pll_7);
                this->write_reg(clk_regs.pll_PFD);
                this->update_regs();
                break;
            default:
                throw uhd::not_implemented_error("enable_external_ref: unknown hardware version");
        }
    }

    double get_master_clock_rate(void) override {
        return 100e6;
    }

    void set_mimo_clock_delay(double delay) override {
        //TODO: Проверить действительно ли не нужна имплементация
    }

private:
    /*!
     * Write a single register to the spi regs.
     * \param addr the address to write
     */
    void write_reg(uint16_t addr) {
        uint32_t data = 0;
        switch (_iface->get_rev()) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                data = _ad9516_regs.get_write_reg(addr);
                _spiface->write_spi(SPI_SS_AD9516, spi_config_t::EDGE_RISE, data, 24);
                break;
            default:
                throw uhd::not_implemented_error("write_reg: unknown hardware version");
        }
    }

    void ad9516_write_reg(int regno, uint8_t value) {
        uint32_t inst = (0 << 15) | (regno & 0x0fff);
        uint32_t v = (inst << 8) | (value & 0xff);
        _spiface->write_spi(SPI_SS_AD9516, spi_config_t::EDGE_RISE, v, 24);
    }

    /*!
     * Tells the ad9510 to latch the settings into the operational registers.
     */
    void update_regs(void) {
        switch (_iface->get_rev()) {
            case kintex7sdr_iface::USRP_N210_XK:
            case kintex7sdr_iface::USRP_N210_XA:
                _ad9516_regs.update_registers = 1;
                break;
            default:
                throw uhd::not_implemented_error("update_regs: unknown hardware version");
        }
        this->write_reg(clk_regs.update);
    }

    // uses output clock 3 (pecl)
    // this is the same between USRP2 and USRP2+ and doesn't get a switch statement
    void enable_dac_clock(bool enb) {
        switch (_iface->get_rev()) {
            case kintex7sdr_iface::USRP_N210_XK:
                _ad9516_regs.power_down_lvpecl_out2 =
                        (enb) ? ad9516_regs_t::POWER_DOWN_LVPECL_OUT2_NORMAL
                              : ad9516_regs_t::POWER_DOWN_LVPECL_OUT2_SAFE_PD;
                _ad9516_regs.output_level_lvpecl_out2 =
                        ad9516_regs_t::OUTPUT_LEVEL_LVPECL_OUT2_780MV;
                _ad9516_regs.divider1_bypass = 1;

                _ad9516_regs.power_down_lvpecl_out3 =
                        (enb) ? ad9516_regs_t::POWER_DOWN_LVPECL_OUT3_NORMAL
                              : ad9516_regs_t::POWER_DOWN_LVPECL_OUT3_SAFE_PD;
                _ad9516_regs.output_level_lvpecl_out3 = ad9516_regs_t::OUTPUT_LEVEL_LVPECL_OUT3_780MV;
                break;
            case kintex7sdr_iface::USRP_N210_XA:
                _ad9516_regs.power_down_lvpecl_out0 =
                        (enb) ? ad9516_regs_t::POWER_DOWN_LVPECL_OUT0_NORMAL
                              : ad9516_regs_t::POWER_DOWN_LVPECL_OUT0_SAFE_PD;
                _ad9516_regs.output_level_lvpecl_out0 = ad9516_regs_t::OUTPUT_LEVEL_LVPECL_OUT0_780MV;
                _ad9516_regs.divider0_bypass = 1;
                break;
            default:
                throw uhd::not_implemented_error("enable_dac_clock: unknown hardware version");
        }


        this->write_reg(clk_regs.output(clk_regs.dac));
        this->write_reg(clk_regs.div_hi(clk_regs.dac));
        this->write_reg(clk_regs.div_lo(clk_regs.dac)); // TODO: Проверить, в новом UHD нет этой строки
        this->update_regs();
    }

    // uses output clock 4 (lvds) on USRP2 and output clock 2 (lvpecl) on USRP2+
    void enable_adc_clock(bool enb) {
        if (_iface->get_rev() != kintex7sdr_iface::USRP_N210_XK &&
            _iface->get_rev() != kintex7sdr_iface::USRP_N210_XA)
            throw uhd::not_implemented_error("enable_dac_clock: unknown hardware version");


        _ad9516_regs.power_down_lvpecl_out1 = enb ? ad9516_regs_t::POWER_DOWN_LVPECL_OUT1_NORMAL
                                                  : ad9516_regs_t::POWER_DOWN_LVPECL_OUT1_SAFE_PD;
        _ad9516_regs.output_level_lvpecl_out1 = ad9516_regs_t::OUTPUT_LEVEL_LVPECL_OUT1_780MV;
        _ad9516_regs.divider0_bypass = 1;

        this->write_reg(clk_regs.output(clk_regs.adc));
        this->write_reg(clk_regs.div_hi(clk_regs.adc));
        this->write_reg(clk_regs.div_lo(clk_regs.adc)); // TODO: Проверить, в новом UHD нет этой строки
        this->update_regs();
    }

    kintex7sdr_iface::sptr _iface;
    uhd::spi_iface::sptr _spiface;
    kintex7sdr_clk_regs_t clk_regs;
    ad9510_regs_t _ad9510_regs;
    ad9516_regs_t _ad9516_regs;
};
