//
// Copyright 2011-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Antenna constants
#define ANTSW_IO        ((1 << 15))             // on UNIT_TX, 0 = TX, 1 = RX, on UNIT_RX 0 = main ant, 1 = RX2
#define ANT_TX          0                       //the tx line is transmitting
#define ANT_RX          ANTSW_IO                //the tx line is receiving
#define ANT_TXRX        0                       //the rx line is on txrx
#define ANT_RX2         ANTSW_IO                //the rx line in on rx2

#include "db_wbx_common.hpp"
#include <uhd/utils/static.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;


/***********************************************************************
 * The WBX Simple dboard constants
 **********************************************************************/
static const std::vector<std::string> wbx_tx_antennas = list_of("TX/RX")("CAL");

static const std::vector<std::string> wbx_rx_antennas = list_of("TX/RX")("RX2")("CAL");

/***********************************************************************
 * The WBX simple implementation
 **********************************************************************/
class wbx_simple : public wbx_base{
public:
    wbx_simple(ctor_args_t args);
    virtual ~wbx_simple(void);

private:
    void set_rx_ant(const std::string &ant);
    void set_tx_ant(const std::string &ant);
    std::string _rx_ant;
};

/***********************************************************************
 * Register the WBX simple implementation
 **********************************************************************/
static dboard_base::sptr make_wbx_simple(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new wbx_simple(args));
}

/***********************************************************************
 * ID Numbers for WBX daughterboard combinations.
 **********************************************************************/
UHD_STATIC_BLOCK(reg_wbx_simple_dboards){
    dboard_manager::register_dboard(0x0053, 0x0052, &make_wbx_simple, "WBX");
    dboard_manager::register_dboard(0x0053, 0x004f, &make_wbx_simple, "WBX + Simple GDB");
    dboard_manager::register_dboard(0x0057, 0x0056, &make_wbx_simple, "WBX v3");
    dboard_manager::register_dboard(0x0057, 0x004f, &make_wbx_simple, "WBX v3 + Simple GDB");
    dboard_manager::register_dboard(0x0063, 0x0062, &make_wbx_simple, "WBX v4");
    dboard_manager::register_dboard(0x0063, 0x004f, &make_wbx_simple, "WBX v4 + Simple GDB");
    dboard_manager::register_dboard(0x0081, 0x0080, &make_wbx_simple, "WBX-120");
    dboard_manager::register_dboard(0x0081, 0x004f, &make_wbx_simple, "WBX-120 + Simple GDB");
}

/***********************************************************************
 * Structors
 **********************************************************************/
wbx_simple::wbx_simple(ctor_args_t args) : wbx_base(args){

    ////////////////////////////////////////////////////////////////////
    // Register RX properties
    ////////////////////////////////////////////////////////////////////

    this->get_rx_subtree()->access<std::string>("name").set(
        std::string(str(boost::format("%s+GDB") % this->get_rx_subtree()->access<std::string>("name").get()
    )));
    this->get_rx_subtree()->create<std::string>("antenna/value")
        .add_coerced_subscriber(boost::bind(&wbx_simple::set_rx_ant, this, _1))
        .set("RX2");
    this->get_rx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(wbx_rx_antennas);

    ////////////////////////////////////////////////////////////////////
    // Register TX properties
    ////////////////////////////////////////////////////////////////////
    this->get_tx_subtree()->access<std::string>("name").set(
        std::string(str(boost::format("%s+GDB") % this->get_tx_subtree()->access<std::string>("name").get()
    )));
    this->get_tx_subtree()->create<std::string>("antenna/value")
        .add_coerced_subscriber(boost::bind(&wbx_simple::set_tx_ant, this, _1))
        .set(wbx_tx_antennas.at(0));
    this->get_tx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(wbx_tx_antennas);

    //set the gpio directions and atr controls (antenna switches all under ATR)
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, ANTSW_IO, ANTSW_IO);
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, ANTSW_IO, ANTSW_IO);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, ANTSW_IO, ANTSW_IO);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, ANTSW_IO, ANTSW_IO);

    //setup ATR for the antenna switches (constant)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_IDLE,        ANT_RX, ANTSW_IO);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_RX_ONLY,     ANT_RX, ANTSW_IO);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_TX_ONLY,     ANT_TX, ANTSW_IO);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_FULL_DUPLEX, ANT_TX, ANTSW_IO);

    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_IDLE,        ANT_TXRX, ANTSW_IO);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_TX_ONLY,     ANT_RX2, ANTSW_IO);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_FULL_DUPLEX, ANT_RX2, ANTSW_IO);
}

wbx_simple::~wbx_simple(void){
    /* NOP */
}

/***********************************************************************
 * Antennas
 **********************************************************************/
void wbx_simple::set_rx_ant(const std::string &ant){
    //validate input
    assert_has(wbx_rx_antennas, ant, "wbx rx antenna name");

    //shadow the setting
    _rx_ant = ant;

    //write the new antenna setting to atr regs
    if (_rx_ant == "CAL") {
        this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_TX_ONLY,     ANT_TXRX, ANTSW_IO);
        this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_FULL_DUPLEX, ANT_TXRX, ANTSW_IO);
        this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_RX_ONLY,     ANT_TXRX, ANTSW_IO);
    } 
    else {
        this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_TX_ONLY,     ANT_RX2, ANTSW_IO);
        this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_FULL_DUPLEX, ANT_RX2, ANTSW_IO);
        this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, gpio_atr::ATR_REG_RX_ONLY, ((_rx_ant == "TX/RX")? ANT_TXRX : ANT_RX2), ANTSW_IO);
    }
}

void wbx_simple::set_tx_ant(const std::string &ant){
    assert_has(wbx_tx_antennas, ant, "wbx tx antenna name");

    //write the new antenna setting to atr regs
    if (ant == "CAL") {
        this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_TX_ONLY,     ANT_RX, ANTSW_IO);
        this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_FULL_DUPLEX, ANT_RX, ANTSW_IO);
    } 
    else {
        this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_TX_ONLY,     ANT_TX, ANTSW_IO);
        this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, gpio_atr::ATR_REG_FULL_DUPLEX, ANT_TX, ANTSW_IO);
    }
}
