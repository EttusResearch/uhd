//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_DBOARD_TWINRX_IO_HPP
#define INCLUDED_DBOARD_TWINRX_IO_HPP

#include <uhd/types/wb_iface.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/utils/soft_register.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <boost/thread.hpp>

namespace uhd { namespace usrp { namespace dboard { namespace twinrx {

static const uint32_t SET_ALL_BITS = 0xFFFFFFFF;

namespace cpld {
static wb_iface::wb_addr_type addr(uint8_t cpld_num, uint8_t cpld_addr) {
    //Decode CPLD addressing for the following bitmap:
    // {CPLD1_EN, CPLD2_EN, CPLD3_EN, CPLD4_EN, CPLD_ADDR[2:0]}
    uint8_t addr = 0;
    switch (cpld_num) {
        case 1: addr = 0x8 << 3; break;
        case 2: addr = 0x4 << 3; break;
        case 3: addr = 0x2 << 3; break;
        case 4: addr = 0x1 << 3; break;
        default: UHD_THROW_INVALID_CODE_PATH();
    }
    return static_cast<wb_iface::wb_addr_type>(addr | (cpld_addr & 0x7));
}

static uint32_t get_reg(wb_iface::wb_addr_type addr) {
    return static_cast<uint32_t>(addr) & 0x7;
}
}

class twinrx_gpio : public timed_wb_iface {
public:
    typedef boost::shared_ptr<twinrx_gpio> sptr;

    //----------------------------------------------
    //Public GPIO fields
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_LO2_CE_CH1,     /*width*/ 1, /*shift*/  0);     //GPIO[0]       OUT
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_LO2_CE_CH2,     /*width*/ 1, /*shift*/  1);     //GPIO[1]       OUT
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_LO2_MUXOUT_CH1, /*width*/ 1, /*shift*/  2);     //GPIO[2]       IN
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_LO2_MUXOUT_CH2, /*width*/ 1, /*shift*/  3);     //GPIO[3]       IN
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_LO2_LD_CH1,     /*width*/ 1, /*shift*/  4);     //GPIO[4]       IN
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_LO2_LD_CH2,     /*width*/ 1, /*shift*/  5);     //GPIO[5]       IN
    // NO CONNECT                                                                   //GPIO[8:6]
    // PRIVATE                                                                      //GPIO[15:9]
    // NO CONNECT                                                                   //GPIO[16]
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_LO1_CE_CH1,     /*width*/ 1, /*shift*/ 17);     //GPIO[17]      OUT
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_LO1_CE_CH2,     /*width*/ 1, /*shift*/ 18);     //GPIO[18]      OUT
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_LO1_MUXOUT_CH1, /*width*/ 1, /*shift*/ 19);     //GPIO[19]      IN
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_LO1_MUXOUT_CH2, /*width*/ 1, /*shift*/ 20);     //GPIO[20]      IN
    // NO CONNECT                                                                   //GPIO[21:23]
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_SWPS_CLK,       /*width*/ 1, /*shift*/ 24);     //GPIO[24]      IN
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_SWPS_PWR_GOOD,  /*width*/ 1, /*shift*/ 25);     //GPIO[25]      IN
    UHD_DEFINE_SOFT_REG_FIELD(FIELD_SWPS_EN,        /*width*/ 1, /*shift*/ 26);     //GPIO[26]      OUT
    // PRIVATE                                                                      //GPIO[27:31]
    //----------------------------------------------

    twinrx_gpio(dboard_iface::sptr iface) : _db_iface(iface) {
        _db_iface->set_gpio_ddr(dboard_iface::UNIT_BOTH, GPIO_OUTPUT_MASK, SET_ALL_BITS);
        _db_iface->set_pin_ctrl(dboard_iface::UNIT_BOTH, GPIO_PINCTRL_MASK, SET_ALL_BITS);
        _db_iface->set_gpio_out(dboard_iface::UNIT_BOTH, 0, ~GPIO_PINCTRL_MASK);
    }

    ~twinrx_gpio() {
        _db_iface->set_gpio_ddr(dboard_iface::UNIT_BOTH, ~GPIO_OUTPUT_MASK, SET_ALL_BITS);
    }

    void set_field(const uhd::soft_reg_field_t field, const uint32_t value) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        using namespace soft_reg_field;

        _db_iface->set_gpio_out(dboard_iface::UNIT_BOTH,
            (value << shift(field)),
            mask<uint32_t>(field));
    }

    uint32_t get_field(const uhd::soft_reg_field_t field) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        using namespace soft_reg_field;
        return (_db_iface->read_gpio(dboard_iface::UNIT_BOTH) & mask<uint32_t>(field)) >> shift(field);
    }

    // CPLD register write-only interface
    void poke32(const wb_addr_type addr, const uint32_t data) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        using namespace soft_reg_field;

        //Step 1: Write the reg offset and data to the GPIO bus and de-assert all enables
        _db_iface->set_gpio_out(dboard_iface::UNIT_BOTH,
            (cpld::get_reg(addr) << shift(CPLD_FULL_ADDR)) | (data << shift(CPLD_DATA)),
            mask<uint32_t>(CPLD_FULL_ADDR)|mask<uint32_t>(CPLD_DATA));
        //Sleep for 166ns to ensure that we don't toggle the enables too quickly
        //The underlying sleep function rounds to microsecond precision.
        _db_iface->sleep(boost::chrono::nanoseconds(166));
        //Step 2: Write the reg offset and data, and assert the necessary enable
        _db_iface->set_gpio_out(dboard_iface::UNIT_BOTH,
            (static_cast<uint32_t>(addr) << shift(CPLD_FULL_ADDR)) | (data << shift(CPLD_DATA)),
            mask<uint32_t>(CPLD_FULL_ADDR)|mask<uint32_t>(CPLD_DATA));
    }

    // Timed command interface
    inline time_spec_t get_time() {
        return _db_iface->get_command_time();
    }

    void set_time(const time_spec_t& t) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        _db_iface->set_command_time(t);
    }

private:    //Members/definitions
    static const uint32_t GPIO_OUTPUT_MASK   = 0xFC06FE03;
    static const uint32_t GPIO_PINCTRL_MASK  = 0x00000000;

    //Private GPIO fields
    UHD_DEFINE_SOFT_REG_FIELD(CPLD_FULL_ADDR, /*width*/ 7, /*shift*/  9);     //GPIO[15:9]
    UHD_DEFINE_SOFT_REG_FIELD(CPLD_DATA,      /*width*/ 5, /*shift*/ 27);     //GPIO[31:27]

    //Members
    dboard_iface::sptr  _db_iface;
    boost::mutex        _mutex;
};

class twinrx_cpld_regmap : public uhd::soft_regmap_t {
public:
    typedef boost::shared_ptr<twinrx_cpld_regmap> sptr;

    //----------------------------------------------
    // IF CCA: CPLD 1
    //----------------------------------------------
    class if0_reg0_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(AMP_HB_IF1_EN_CH1,    /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(AMP_LO2_EN_CH1,       /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(AMP_LO2_EN_CH2,       /*width*/ 1, /*shift*/ 2);
        UHD_DEFINE_SOFT_REG_FIELD(SW19_CTRL_CH2,        /*width*/ 1, /*shift*/ 3);
        UHD_DEFINE_SOFT_REG_FIELD(SW20_CTRL_CH2,        /*width*/ 1, /*shift*/ 4);

        if0_reg0_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 1, /*reg*/ 0), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } if0_reg0;

    class if0_reg1_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW20_CTRL_CH1,        /*width*/ 1, /*shift*/ 2);

        if0_reg1_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 1, /*reg*/ 1), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } if0_reg1;

    class if0_reg2_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(AMP_LB_IF1_EN_CH2,    /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(AMP_LB_IF1_EN_CH1,    /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(LO2_LE_CH1,           /*width*/ 1, /*shift*/ 3);
        UHD_DEFINE_SOFT_REG_FIELD(LO2_LE_CH2,           /*width*/ 1, /*shift*/ 4);

        if0_reg2_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 1, /*reg*/ 2), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } if0_reg2;

    class if0_reg3_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW24_CTRL_CH1,        /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(SW13_CTRL_CH1,        /*width*/ 1, /*shift*/ 2);
        UHD_DEFINE_SOFT_REG_FIELD(IF1_IF2_EN_CH1,       /*width*/ 1, /*shift*/ 3);

        if0_reg3_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 1, /*reg*/ 3), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } if0_reg3;

    class if0_reg4_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW21_CTRL_CH2,        /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SW25_CTRL,            /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(IF1_IF2_EN_CH2,       /*width*/ 1, /*shift*/ 2);
        UHD_DEFINE_SOFT_REG_FIELD(SW19_CTRL_CH1,        /*width*/ 1, /*shift*/ 3);
        UHD_DEFINE_SOFT_REG_FIELD(SW21_CTRL_CH1,        /*width*/ 1, /*shift*/ 4);

        if0_reg4_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 1, /*reg*/ 4), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } if0_reg4;

    class if0_reg6_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(AMP_HB_IF1_EN_CH2,    /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SW13_CTRL_CH2,        /*width*/ 1, /*shift*/ 2);

        if0_reg6_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 1, /*reg*/ 6), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } if0_reg6;

    class if0_reg7_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW24_CTRL_CH2,        /*width*/ 1, /*shift*/ 0);

        if0_reg7_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 1, /*reg*/ 7), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } if0_reg7;

    //----------------------------------------------
    // RF CCA: CPLD 2
    //----------------------------------------------
    class rf0_reg0_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(ATTEN_IN_CH1,         /*width*/ 5, /*shift*/ 0);

        rf0_reg0_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 2, /*reg*/ 0), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf0_reg0;

    class rf0_reg1_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SWPA1_CTL_CH1,        /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(HB_PREAMP_EN_CH1,     /*width*/ 1, /*shift*/ 2);
        UHD_DEFINE_SOFT_REG_FIELD(LB_PREAMP_EN_CH1,     /*width*/ 1, /*shift*/ 3);
        UHD_DEFINE_SOFT_REG_FIELD(SWPA3_CTRL_CH2,       /*width*/ 1, /*shift*/ 4);

        rf0_reg1_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 2, /*reg*/ 1), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf0_reg1;

    class rf0_reg2_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW6_CTRL_CH1,         /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SW5_CTRL_CH1,         /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(SW4_CTRL_CH1,         /*width*/ 1, /*shift*/ 2);
        UHD_DEFINE_SOFT_REG_FIELD(LO1_LE_CH1,           /*width*/ 1, /*shift*/ 3);
        UHD_DEFINE_SOFT_REG_FIELD(LO1_LE_CH2,           /*width*/ 1, /*shift*/ 4);

        rf0_reg2_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 2, /*reg*/ 2), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf0_reg2;

    class rf0_reg3_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW9_CTRL_CH2,         /*width*/ 2, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SW7_CTRL_CH1,         /*width*/ 2, /*shift*/ 2);

        rf0_reg3_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 2, /*reg*/ 3), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf0_reg3;

    class rf0_reg4_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(ATTEN_IN_CH2,         /*width*/ 5, /*shift*/ 0);

        rf0_reg4_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 2, /*reg*/ 4), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf0_reg4;

    class rf0_reg5_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW9_CTRL_CH1,         /*width*/ 2, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(HB_PREAMP_EN_CH2,     /*width*/ 1, /*shift*/ 2);
        UHD_DEFINE_SOFT_REG_FIELD(SW3_CTRL_CH1,         /*width*/ 1, /*shift*/ 4);

        rf0_reg5_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 2, /*reg*/ 5), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf0_reg5;

    class rf0_reg6_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW6_CTRL_CH2,         /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SW5_CTRL_CH2,         /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(SW4_CTRL_CH2,         /*width*/ 1, /*shift*/ 2);
        UHD_DEFINE_SOFT_REG_FIELD(SWPA4_CTRL_CH2,       /*width*/ 1, /*shift*/ 4);

        rf0_reg6_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 2, /*reg*/ 6), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf0_reg6;

    class rf0_reg7_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SWPA1_CTRL_CH2,       /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SWPA3_CTRL_CH1,       /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(SW3_CTRL_CH2,         /*width*/ 1, /*shift*/ 2);
        UHD_DEFINE_SOFT_REG_FIELD(SW7_CTRL_CH2,         /*width*/ 2, /*shift*/ 3);

        rf0_reg7_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 2, /*reg*/ 7), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf0_reg7;

    //----------------------------------------------
    // RF CCA: CPLD 3
    //----------------------------------------------
    class rf1_reg0_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(ATTEN_HB_CH1,         /*width*/ 5, /*shift*/ 0);

        rf1_reg0_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 3, /*reg*/ 0), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf1_reg0;

    class rf1_reg1_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW17_CTRL_CH1,        /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(AMP_LO1_EN_CH1,       /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(SW16_CTRL_CH1,        /*width*/ 1, /*shift*/ 2);
        UHD_DEFINE_SOFT_REG_FIELD(SW15_CTRL_CH1,        /*width*/ 1, /*shift*/ 3);
        UHD_DEFINE_SOFT_REG_FIELD(SW14_CTRL_CH1,        /*width*/ 1, /*shift*/ 4);

        rf1_reg1_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 3, /*reg*/ 1), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf1_reg1;

    class rf1_reg2_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW12_CTRL_CH1,        /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(AMP_HB_EN_CH1,        /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(HB_PRESEL_PGA_EN_CH2, /*width*/ 1, /*shift*/ 2);

        rf1_reg2_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 3, /*reg*/ 2), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf1_reg2;

    class rf1_reg3_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW23_CTRL,            /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SW22_CTRL_CH1,        /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(SW10_CTRL_CH1,        /*width*/ 2, /*shift*/ 2);

        rf1_reg3_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 3, /*reg*/ 3), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf1_reg3;

    class rf1_reg4_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(ATTEN_HB_CH2,         /*width*/ 5, /*shift*/ 0);

        rf1_reg4_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 3, /*reg*/ 4), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf1_reg4;

    class rf1_reg5_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(AMP_LO1_EN_CH2,       /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SW15_CTRL_CH2,        /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(SW14_CTRL_CH2,        /*width*/ 1, /*shift*/ 2);
        UHD_DEFINE_SOFT_REG_FIELD(SW18_CTRL_CH1,        /*width*/ 1, /*shift*/ 4);

        rf1_reg5_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 3, /*reg*/ 5), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf1_reg5;

    class rf1_reg6_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(HB_PRESEL_PGA_EN_CH1, /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SW17_CTRL_CH2,        /*width*/ 1, /*shift*/ 2);
        UHD_DEFINE_SOFT_REG_FIELD(SW16_CTRL_CH2,        /*width*/ 1, /*shift*/ 3);
        UHD_DEFINE_SOFT_REG_FIELD(PREAMP2_EN_CH2,       /*width*/ 1, /*shift*/ 4);

        rf1_reg6_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 3, /*reg*/ 6), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf1_reg6;

    class rf1_reg7_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW22_CTRL_CH2,        /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SW10_CTRL_CH2,        /*width*/ 2, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(SW12_CTRL_CH2,        /*width*/ 1, /*shift*/ 3);
        UHD_DEFINE_SOFT_REG_FIELD(AMP_HB_EN_CH2,        /*width*/ 1, /*shift*/ 4);

        rf1_reg7_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 3, /*reg*/ 7), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf1_reg7;

    //----------------------------------------------
    // RF CCA: CPLD 4
    //----------------------------------------------
    class rf2_reg0_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(ATTEN_LB_CH1,         /*width*/ 5, /*shift*/ 0);

        rf2_reg0_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 4, /*reg*/ 0), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf2_reg0;

    class rf2_reg2_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SW11_CTRL_CH1,        /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(AMP_LB_EN_CH1,        /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(SWPA2_CTRL_CH1,       /*width*/ 1, /*shift*/ 2);

        rf2_reg2_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 4, /*reg*/ 2), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf2_reg2;

    class rf2_reg3_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(PREAMP2_EN_CH1,       /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SW18_CTRL_CH2,        /*width*/ 1, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(SW8_CTRL_CH1,         /*width*/ 2, /*shift*/ 2);

        rf2_reg3_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 4, /*reg*/ 3), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf2_reg3;

    class rf2_reg4_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(ATTEN_LB_CH2,         /*width*/ 5, /*shift*/ 0);

        rf2_reg4_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 4, /*reg*/ 4), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf2_reg4;

    class rf2_reg5_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SWPA2_CTRL_CH2,       /*width*/ 1, /*shift*/ 0);

        rf2_reg5_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 4, /*reg*/ 5), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf2_reg5;

    class rf2_reg6_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(LB_PREAMP_EN_CH2,     /*width*/ 1, /*shift*/ 0);

        rf2_reg6_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 4, /*reg*/ 6), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf2_reg6;

    class rf2_reg7_t : public uhd::soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SWPA4_CTRL_CH1,       /*width*/ 1, /*shift*/ 0);
        UHD_DEFINE_SOFT_REG_FIELD(SW8_CTRL_CH2,         /*width*/ 2, /*shift*/ 1);
        UHD_DEFINE_SOFT_REG_FIELD(SW11_CTRL_CH2,        /*width*/ 1, /*shift*/ 3);
        UHD_DEFINE_SOFT_REG_FIELD(AMP_LB_EN_CH2,        /*width*/ 1, /*shift*/ 4);

        rf2_reg7_t(): uhd::soft_reg32_wo_t(cpld::addr(/*cpld*/ 4, /*reg*/ 7), OPTIMIZED_FLUSH) {
            set(REGISTER, 0);
        }
    } rf2_reg7;

    twinrx_cpld_regmap() : soft_regmap_t("twinrx_cpld") {
        // IF CCA: CPLD 1
        add_to_map(if0_reg0, "if0_reg0");
        add_to_map(if0_reg1, "if0_reg1");
        add_to_map(if0_reg2, "if0_reg2");
        add_to_map(if0_reg3, "if0_reg3");
        add_to_map(if0_reg4, "if0_reg4");
        add_to_map(if0_reg6, "if0_reg6");
        add_to_map(if0_reg7, "if0_reg7");
        // RF CCA: CPLD 2
        add_to_map(rf0_reg0, "rf0_reg0");
        add_to_map(rf0_reg1, "rf0_reg1");
        add_to_map(rf0_reg2, "rf0_reg2");
        add_to_map(rf0_reg3, "rf0_reg3");
        add_to_map(rf0_reg4, "rf0_reg4");
        add_to_map(rf0_reg5, "rf0_reg5");
        add_to_map(rf0_reg6, "rf0_reg6");
        add_to_map(rf0_reg7, "rf0_reg7");
        // RF CCA: CPLD 3
        add_to_map(rf1_reg0, "rf1_reg0");
        add_to_map(rf1_reg1, "rf1_reg1");
        add_to_map(rf1_reg2, "rf1_reg2");
        add_to_map(rf1_reg3, "rf1_reg3");
        add_to_map(rf1_reg4, "rf1_reg4");
        add_to_map(rf1_reg5, "rf1_reg5");
        add_to_map(rf1_reg6, "rf1_reg6");
        add_to_map(rf1_reg7, "rf1_reg7");
        // RF CCA: CPLD 4
        add_to_map(rf2_reg0, "rf2_reg0");
        add_to_map(rf2_reg2, "rf2_reg2");
        add_to_map(rf2_reg3, "rf2_reg3");
        add_to_map(rf2_reg4, "rf2_reg4");
        add_to_map(rf2_reg5, "rf2_reg5");
        add_to_map(rf2_reg6, "rf2_reg6");
        add_to_map(rf2_reg7, "rf2_reg7");
    }
};

}}}} //namespaces

#endif /* INCLUDED_DBOARD_TWINRX_IO_HPP */
