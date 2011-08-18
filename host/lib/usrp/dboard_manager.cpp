//
// Copyright 2010-2011 Ettus Research LLC
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

#include "dboard_ctor_args.hpp"
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/dict.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/assign/list_of.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * dboard key class to use for look-up
 **********************************************************************/
class dboard_key_t{
public:
    dboard_key_t(const dboard_id_t &id = dboard_id_t::none()):
        _rx_id(id), _tx_id(id), _xcvr(false){}

    dboard_key_t(const dboard_id_t &rx_id, const dboard_id_t &tx_id):
        _rx_id(rx_id), _tx_id(tx_id), _xcvr(true){}

    dboard_id_t xx_id(void) const{
        UHD_ASSERT_THROW(not this->is_xcvr());
        return this->_rx_id;
    }

    dboard_id_t rx_id(void) const{
        UHD_ASSERT_THROW(this->is_xcvr());
        return this->_rx_id;
    }

    dboard_id_t tx_id(void) const{
        UHD_ASSERT_THROW(this->is_xcvr());
        return this->_tx_id;
    }

    bool is_xcvr(void) const{
        return this->_xcvr;
    }

private:
    dboard_id_t _rx_id, _tx_id;
    bool _xcvr;
};

bool operator==(const dboard_key_t &lhs, const dboard_key_t &rhs){
    if (lhs.is_xcvr() and rhs.is_xcvr())
        return lhs.rx_id() == rhs.rx_id() and lhs.tx_id() == rhs.tx_id();
    if (not lhs.is_xcvr() and not rhs.is_xcvr())
        return lhs.xx_id() == rhs.xx_id();
    return false;
}

/***********************************************************************
 * storage and registering for dboards
 **********************************************************************/
//dboard registry tuple: dboard constructor, canonical name, subdev names
typedef boost::tuple<dboard_manager::dboard_ctor_t, std::string, prop_names_t> args_t;

//map a dboard id to a dboard constructor
typedef uhd::dict<dboard_key_t, args_t> id_to_args_map_t;
UHD_SINGLETON_FCN(id_to_args_map_t, get_id_to_args_map)

static void register_dboard_key(
    const dboard_key_t &dboard_key,
    dboard_manager::dboard_ctor_t dboard_ctor,
    const std::string &name,
    const prop_names_t &subdev_names
){
    UHD_LOGV(always) << "registering: " << name << std::endl;
    if (get_id_to_args_map().has_key(dboard_key)){

        if (dboard_key.is_xcvr()) throw uhd::key_error(str(boost::format(
            "The dboard id pair [%s, %s] is already registered to %s."
        ) % dboard_key.rx_id().to_string() % dboard_key.tx_id().to_string() % get_id_to_args_map()[dboard_key].get<1>()));

        else throw uhd::key_error(str(boost::format(
            "The dboard id %s is already registered to %s."
        ) % dboard_key.xx_id().to_string() % get_id_to_args_map()[dboard_key].get<1>()));

    }
    get_id_to_args_map()[dboard_key] = args_t(dboard_ctor, name, subdev_names);
}

void dboard_manager::register_dboard(
    const dboard_id_t &dboard_id,
    dboard_ctor_t dboard_ctor,
    const std::string &name,
    const prop_names_t &subdev_names
){
    register_dboard_key(dboard_key_t(dboard_id), dboard_ctor, name, subdev_names);
}

void dboard_manager::register_dboard(
    const dboard_id_t &rx_dboard_id,
    const dboard_id_t &tx_dboard_id,
    dboard_ctor_t dboard_ctor,
    const std::string &name,
    const prop_names_t &subdev_names
){
    register_dboard_key(dboard_key_t(rx_dboard_id, tx_dboard_id), dboard_ctor, name, subdev_names);
}

std::string dboard_id_t::to_cname(void) const{
    std::string cname;
    BOOST_FOREACH(const dboard_key_t &key, get_id_to_args_map().keys()){
        if (
            (not key.is_xcvr() and *this == key.xx_id()) or
            (key.is_xcvr() and (*this == key.rx_id() or *this == key.tx_id()))
        ){
            if (not cname.empty()) cname += ", ";
            cname += get_id_to_args_map()[key].get<1>();
        }
    }
    return (cname.empty())? "Unknown" : cname;
}

std::string dboard_id_t::to_pp_string(void) const{
    return str(boost::format("%s (%s)") % this->to_cname() % this->to_string());
}

/***********************************************************************
 * internal helper classes
 **********************************************************************/
/*!
 * A special wax proxy object that forwards calls to a subdev.
 * A sptr to an instance will be used in the properties structure. 
 */
class subdev_proxy : boost::noncopyable, public wax::obj{
public:
    typedef boost::shared_ptr<subdev_proxy> sptr;
    enum type_t{RX_TYPE, TX_TYPE};

    //structors
    subdev_proxy(dboard_base::sptr subdev, type_t type):
        _subdev(subdev), _type(type)
    {
        /* NOP */
    }

private:
    dboard_base::sptr   _subdev;
    type_t              _type;

    //forward the get calls to the rx or tx
    void get(const wax::obj &key, wax::obj &val){
        switch(_type){
        case RX_TYPE: return _subdev->rx_get(key, val);
        case TX_TYPE: return _subdev->tx_get(key, val);
        }
    }

    //forward the set calls to the rx or tx
    void set(const wax::obj &key, const wax::obj &val){
        switch(_type){
        case RX_TYPE: return _subdev->rx_set(key, val);
        case TX_TYPE: return _subdev->tx_set(key, val);
        }
    }
};

/***********************************************************************
 * dboard manager implementation class
 **********************************************************************/
class dboard_manager_impl : public dboard_manager{

public:
    dboard_manager_impl(
        dboard_id_t rx_dboard_id,
        dboard_id_t tx_dboard_id,
        dboard_iface::sptr iface
    );
    ~dboard_manager_impl(void);

    //dboard_iface
    prop_names_t get_rx_subdev_names(void);
    prop_names_t get_tx_subdev_names(void);
    wax::obj get_rx_subdev(const std::string &subdev_name);
    wax::obj get_tx_subdev(const std::string &subdev_name);

private:
    void init(dboard_id_t, dboard_id_t);
    //list of rx and tx dboards in this dboard_manager
    //each dboard here is actually a subdevice proxy
    //the subdevice proxy is internal to the cpp file
    uhd::dict<std::string, subdev_proxy::sptr> _rx_dboards;
    uhd::dict<std::string, subdev_proxy::sptr> _tx_dboards;
    dboard_iface::sptr _iface;
    void set_nice_dboard_if(void);
};

/***********************************************************************
 * make routine for dboard manager
 **********************************************************************/
dboard_manager::sptr dboard_manager::make(
    dboard_id_t rx_dboard_id,
    dboard_id_t tx_dboard_id,
    dboard_iface::sptr iface
){
    return dboard_manager::sptr(
        new dboard_manager_impl(rx_dboard_id, tx_dboard_id, iface)
    );
}

/***********************************************************************
 * implementation class methods
 **********************************************************************/
dboard_manager_impl::dboard_manager_impl(
    dboard_id_t rx_dboard_id,
    dboard_id_t tx_dboard_id,
    dboard_iface::sptr iface
):
    _iface(iface)
{
    try{
        this->init(rx_dboard_id, tx_dboard_id);
    }
    catch(const std::exception &e){
        UHD_MSG(error) << "The daughterboard manager encountered a recoverable error in init" << std::endl << e.what();
        this->init(dboard_id_t::none(), dboard_id_t::none());
    }
}

void dboard_manager_impl::init(
    dboard_id_t rx_dboard_id, dboard_id_t tx_dboard_id
){
    //find the dboard key matches for the dboard ids
    dboard_key_t rx_dboard_key, tx_dboard_key, xcvr_dboard_key;
    BOOST_FOREACH(const dboard_key_t &key, get_id_to_args_map().keys()){
        if (key.is_xcvr()){
            if (rx_dboard_id == key.rx_id() and tx_dboard_id == key.tx_id()) xcvr_dboard_key = key;
            if (rx_dboard_id == key.rx_id()) rx_dboard_key = key; //kept to handle warning
            if (tx_dboard_id == key.tx_id()) tx_dboard_key = key; //kept to handle warning
        }
        else{
            if (rx_dboard_id == key.xx_id()) rx_dboard_key = key;
            if (tx_dboard_id == key.xx_id()) tx_dboard_key = key;
        }
    }

    //warn for invalid dboard id xcvr combinations
    if (not xcvr_dboard_key.is_xcvr() and (rx_dboard_key.is_xcvr() or tx_dboard_key.is_xcvr())){
        UHD_MSG(warning) << boost::format(
            "Unknown transceiver board ID combination.\n"
            "Is your daughter-board mounted properly?\n"
            "RX dboard ID: %s\n"
            "TX dboard ID: %s\n"
        ) % rx_dboard_id.to_pp_string() % tx_dboard_id.to_pp_string();
    }

    //initialize the gpio pins before creating subdevs
    set_nice_dboard_if();

    //dboard constructor args
    dboard_ctor_args_t db_ctor_args;
    db_ctor_args.db_iface = _iface;

    //make xcvr subdevs
    if (xcvr_dboard_key.is_xcvr()){

        //extract data for the xcvr dboard key
        dboard_ctor_t dboard_ctor; std::string name; prop_names_t subdevs;
        boost::tie(dboard_ctor, name, subdevs) = get_id_to_args_map()[xcvr_dboard_key];

        //create the xcvr object for each subdevice
        BOOST_FOREACH(const std::string &subdev, subdevs){
            db_ctor_args.sd_name = subdev;
            db_ctor_args.rx_id = rx_dboard_id;
            db_ctor_args.tx_id = tx_dboard_id;
            dboard_base::sptr xcvr_dboard = dboard_ctor(&db_ctor_args);
            //create a rx proxy for this xcvr board
            _rx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(xcvr_dboard, subdev_proxy::RX_TYPE)
            );
            //create a tx proxy for this xcvr board
            _tx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(xcvr_dboard, subdev_proxy::TX_TYPE)
            );
        }
    }

    //make tx and rx subdevs (separate subdevs for rx and tx dboards)
    else{

        //force the rx key to the unknown board for bad combinations
        if (rx_dboard_key.is_xcvr() or rx_dboard_key.xx_id() == dboard_id_t::none()){
            rx_dboard_key = dboard_key_t(0xfff1);
        }

        //extract data for the rx dboard key
        dboard_ctor_t rx_dboard_ctor; std::string rx_name; prop_names_t rx_subdevs;
        boost::tie(rx_dboard_ctor, rx_name, rx_subdevs) = get_id_to_args_map()[rx_dboard_key];

        //make the rx subdevs
        BOOST_FOREACH(const std::string &subdev, rx_subdevs){
            db_ctor_args.sd_name = subdev;
            db_ctor_args.rx_id = rx_dboard_id;
            db_ctor_args.tx_id = dboard_id_t::none();
            dboard_base::sptr rx_dboard = rx_dboard_ctor(&db_ctor_args);
            //create a rx proxy for this rx board
            _rx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(rx_dboard, subdev_proxy::RX_TYPE)
            );
        }

        //force the tx key to the unknown board for bad combinations
        if (tx_dboard_key.is_xcvr() or tx_dboard_key.xx_id() == dboard_id_t::none()){
            tx_dboard_key = dboard_key_t(0xfff0);
        }

        //extract data for the tx dboard key
        dboard_ctor_t tx_dboard_ctor; std::string tx_name; prop_names_t tx_subdevs;
        boost::tie(tx_dboard_ctor, tx_name, tx_subdevs) = get_id_to_args_map()[tx_dboard_key];

        //make the tx subdevs
        BOOST_FOREACH(const std::string &subdev, tx_subdevs){
            db_ctor_args.sd_name = subdev;
            db_ctor_args.rx_id = dboard_id_t::none();
            db_ctor_args.tx_id = tx_dboard_id;
            dboard_base::sptr tx_dboard = tx_dboard_ctor(&db_ctor_args);
            //create a tx proxy for this tx board
            _tx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(tx_dboard, subdev_proxy::TX_TYPE)
            );
        }
    }
}

dboard_manager_impl::~dboard_manager_impl(void){UHD_SAFE_CALL(
    set_nice_dboard_if();
)}

prop_names_t dboard_manager_impl::get_rx_subdev_names(void){
    return _rx_dboards.keys();
}

prop_names_t dboard_manager_impl::get_tx_subdev_names(void){
    return _tx_dboards.keys();
}

wax::obj dboard_manager_impl::get_rx_subdev(const std::string &subdev_name){
    if (not _rx_dboards.has_key(subdev_name)) throw uhd::key_error(
        str(boost::format("Unknown rx subdev name %s") % subdev_name)
    );
    //get a link to the rx subdev proxy
    return _rx_dboards[subdev_name]->get_link();
}

wax::obj dboard_manager_impl::get_tx_subdev(const std::string &subdev_name){
    if (not _tx_dboards.has_key(subdev_name)) throw uhd::key_error(
        str(boost::format("Unknown tx subdev name %s") % subdev_name)
    );
    //get a link to the tx subdev proxy
    return _tx_dboards[subdev_name]->get_link();
}

void dboard_manager_impl::set_nice_dboard_if(void){
    //make a list of possible unit types
    std::vector<dboard_iface::unit_t> units = boost::assign::list_of
        (dboard_iface::UNIT_RX)
        (dboard_iface::UNIT_TX)
    ;

    //set nice settings on each unit
    BOOST_FOREACH(dboard_iface::unit_t unit, units){
        _iface->set_gpio_ddr(unit, 0x0000); //all inputs
        _iface->set_gpio_out(unit, 0x0000); //all low
        _iface->set_pin_ctrl(unit, 0x0000); //all gpio
        _iface->set_clock_enabled(unit, false); //clock off
    }

    //disable all rx subdevices
    BOOST_FOREACH(const std::string &sd_name, this->get_rx_subdev_names()){
        this->get_rx_subdev(sd_name)[SUBDEV_PROP_ENABLED] = false;
    }

    //disable all tx subdevices
    BOOST_FOREACH(const std::string &sd_name, this->get_tx_subdev_names()){
        this->get_tx_subdev(sd_name)[SUBDEV_PROP_ENABLED] = false;
    }
}

/***********************************************************************
 * Populate a properties tree from a subdev waxy object
 **********************************************************************/
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>

static sensor_value_t get_sensor(wax::obj subdev, const std::string &name){
    return subdev[named_prop_t(SUBDEV_PROP_SENSOR, name)].as<sensor_value_t>();
}

static void set_gain(wax::obj subdev, const std::string &name, const double gain){
    subdev[named_prop_t(SUBDEV_PROP_GAIN, name)] = gain;
}

static double get_gain(wax::obj subdev, const std::string &name){
    return subdev[named_prop_t(SUBDEV_PROP_GAIN, name)].as<double>();
}

static meta_range_t get_gain_range(wax::obj subdev, const std::string &name){
    return subdev[named_prop_t(SUBDEV_PROP_GAIN_RANGE, name)].as<meta_range_t>();
}

static void set_freq(wax::obj subdev, const double freq){
    subdev[SUBDEV_PROP_FREQ] = freq;
}

static double get_freq(wax::obj subdev){
    return subdev[SUBDEV_PROP_FREQ].as<double>();
}

static meta_range_t get_freq_range(wax::obj subdev){
    return subdev[SUBDEV_PROP_FREQ_RANGE].as<meta_range_t>();
}

static void set_ant(wax::obj subdev, const std::string &ant){
    subdev[SUBDEV_PROP_ANTENNA] = ant;
}

static std::string get_ant(wax::obj subdev){
    return subdev[SUBDEV_PROP_ANTENNA].as<std::string>();
}

static std::vector<std::string> get_ants(wax::obj subdev){
    return subdev[SUBDEV_PROP_ANTENNA_NAMES].as<std::vector<std::string> >();
}

static std::string get_conn(wax::obj subdev){
    switch(subdev[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()){
    case SUBDEV_CONN_COMPLEX_IQ: return "IQ";
    case SUBDEV_CONN_COMPLEX_QI: return "QI";
    case SUBDEV_CONN_REAL_I: return "I";
    case SUBDEV_CONN_REAL_Q: return "Q";
    }
    UHD_THROW_INVALID_CODE_PATH();
}

static bool get_use_lo_off(wax::obj subdev){
    return subdev[SUBDEV_PROP_USE_LO_OFFSET].as<bool>();
}

static bool get_set_enb(wax::obj subdev, const bool enb){
    subdev[SUBDEV_PROP_ENABLED] = enb;
    return subdev[SUBDEV_PROP_ENABLED].as<bool>();
}

static void set_bw(wax::obj subdev, const double freq){
    subdev[SUBDEV_PROP_BANDWIDTH] = freq;
}

static double get_bw(wax::obj subdev){
    return subdev[SUBDEV_PROP_BANDWIDTH].as<double>();
}

void dboard_manager::populate_prop_tree_from_subdev(
    property_tree::sptr subtree, wax::obj subdev
){
    subtree->create<std::string>("name").set(subdev[SUBDEV_PROP_NAME].as<std::string>());

    const prop_names_t sensor_names = subdev[SUBDEV_PROP_SENSOR_NAMES].as<prop_names_t>();
    subtree->create<int>("sensors"); //phony property so this dir exists
    BOOST_FOREACH(const std::string &name, sensor_names){
        subtree->create<sensor_value_t>("sensors/" + name)
            .publish(boost::bind(&get_sensor, subdev, name));
    }

    const prop_names_t gain_names = subdev[SUBDEV_PROP_GAIN_NAMES].as<prop_names_t>();
    subtree->create<int>("gains"); //phony property so this dir exists
    BOOST_FOREACH(const std::string &name, gain_names){
        subtree->create<double>("gains/" + name + "/value")
            .publish(boost::bind(&get_gain, subdev, name))
            .subscribe(boost::bind(&set_gain, subdev, name, _1));
        subtree->create<meta_range_t>("gains/" + name + "/range")
            .publish(boost::bind(&get_gain_range, subdev, name));
    }

    subtree->create<double>("freq/value")
        .publish(boost::bind(&get_freq, subdev))
        .subscribe(boost::bind(&set_freq, subdev, _1));

    subtree->create<meta_range_t>("freq/range")
        .publish(boost::bind(&get_freq_range, subdev));

    subtree->create<std::string>("antenna/value")
        .publish(boost::bind(&get_ant, subdev))
        .subscribe(boost::bind(&set_ant, subdev, _1));

    subtree->create<std::vector<std::string> >("antenna/options")
        .publish(boost::bind(&get_ants, subdev));

    subtree->create<std::string>("connection")
        .publish(boost::bind(&get_conn, subdev));

    subtree->create<bool>("enabled")
        .coerce(boost::bind(&get_set_enb, subdev, _1));

    subtree->create<bool>("use_lo_offset")
        .publish(boost::bind(&get_use_lo_off, subdev));

    subtree->create<double>("bandwidth/value")
        .publish(boost::bind(&get_bw, subdev))
        .subscribe(boost::bind(&set_bw, subdev, _1));
}
