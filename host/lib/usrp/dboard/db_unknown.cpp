//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/ranges.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/tuple/tuple.hpp>
#include <vector>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * Utility functions
 **********************************************************************/
static void warn_if_old_rfx(const dboard_id_t &dboard_id, const std::string &xx){
    typedef boost::tuple<std::string, dboard_id_t, dboard_id_t> old_ids_t; //name, rx_id, tx_id
    static const std::vector<old_ids_t> old_rfx_ids = list_of
        (old_ids_t("Flex 400 Classic",  0x0004, 0x0008))
        (old_ids_t("Flex 900 Classic",  0x0005, 0x0009))
        (old_ids_t("Flex 1200 Classic", 0x0006, 0x000a))
        (old_ids_t("Flex 1800 Classic", 0x0030, 0x0031))
        (old_ids_t("Flex 2400 Classic", 0x0007, 0x000b))
    ;
    for(const old_ids_t &old_id:  old_rfx_ids){
        std::string name; dboard_id_t rx_id, tx_id;
        boost::tie(name, rx_id, tx_id) = old_id;
        if (
            (xx == "RX" and rx_id == dboard_id) or
            (xx == "TX" and tx_id == dboard_id)
        ) UHD_LOGGER_WARNING("unknown_db") << boost::format(
            "Detected %s daughterboard %s\n"
            "This board requires modification to use.\n"
            "See the daughterboard application notes.\n"
        ) % xx % name;
    }
}

/***********************************************************************
 * The unknown boards:
 *   Like a basic board, but with only one subdev.
 **********************************************************************/
class unknown_rx : public rx_dboard_base{
public:
    unknown_rx(ctor_args_t args);
};

class unknown_tx : public tx_dboard_base{
public:
    unknown_tx(ctor_args_t args);
};

/***********************************************************************
 * Register the unknown dboards
 **********************************************************************/
static dboard_base::sptr make_unknown_rx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new unknown_rx(args));
}

static dboard_base::sptr make_unknown_tx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new unknown_tx(args));
}

UHD_STATIC_BLOCK(reg_unknown_dboards){
    dboard_manager::register_dboard(0xfff0, &make_unknown_tx, "Unknown TX");
    dboard_manager::register_dboard(0xfff1, &make_unknown_rx, "Unknown RX");
}

/***********************************************************************
 * Unknown RX dboard
 **********************************************************************/
unknown_rx::unknown_rx(ctor_args_t args) : rx_dboard_base(args){
    warn_if_old_rfx(this->get_rx_id(), "RX");

    ////////////////////////////////////////////////////////////////////
    // Register properties
    ////////////////////////////////////////////////////////////////////
    this->get_rx_subtree()->create<std::string>("name").set(
        std::string(str(boost::format("%s - %s")
            % get_rx_id().to_pp_string()
            % get_subdev_name()
        )));
    this->get_rx_subtree()->create<int>("gains"); //phony property so this dir exists
    this->get_rx_subtree()->create<double>("freq/value")
        .set(double(0.0));
    this->get_rx_subtree()->create<meta_range_t>("freq/range")
        .set(freq_range_t(double(0.0), double(0.0)));
    this->get_rx_subtree()->create<std::string>("antenna/value")
        .set("");
    this->get_rx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(list_of(""));
    this->get_rx_subtree()->create<int>("sensors"); //phony property so this dir exists
    this->get_rx_subtree()->create<std::string>("connection")
        .set("IQ");
    this->get_rx_subtree()->create<bool>("enabled")
        .set(true); //always enabled
    this->get_rx_subtree()->create<bool>("use_lo_offset")
        .set(false);
    this->get_rx_subtree()->create<double>("bandwidth/value")
        .set(double(0.0));
    this->get_rx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(0.0, 0.0));
}

/***********************************************************************
 * Unknown TX dboard
 **********************************************************************/
unknown_tx::unknown_tx(ctor_args_t args) : tx_dboard_base(args){
    warn_if_old_rfx(this->get_tx_id(), "TX");

    ////////////////////////////////////////////////////////////////////
    // Register properties
    ////////////////////////////////////////////////////////////////////
    this->get_tx_subtree()->create<std::string>("name").set(
        std::string(str(boost::format("%s - %s")
            % get_tx_id().to_pp_string()
            % get_subdev_name()
        )));
    this->get_tx_subtree()->create<int>("gains"); //phony property so this dir exists
    this->get_tx_subtree()->create<double>("freq/value")
        .set(double(0.0));
    this->get_tx_subtree()->create<meta_range_t>("freq/range")
        .set(freq_range_t(double(0.0), double(0.0)));
    this->get_tx_subtree()->create<std::string>("antenna/value")
        .set("");
    this->get_tx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(list_of(""));
    this->get_tx_subtree()->create<int>("sensors"); //phony property so this dir exists
    this->get_tx_subtree()->create<std::string>("connection")
        .set("IQ");
    this->get_tx_subtree()->create<bool>("enabled")
        .set(true); //always enabled
    this->get_tx_subtree()->create<bool>("use_lo_offset")
        .set(false);
    this->get_tx_subtree()->create<double>("bandwidth/value")
        .set(double(0.0));
    this->get_tx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(0.0, 0.0));
}
