//
// Copyright 2010-2011,2017 Ettus Research, A National Instruments Company
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
    dboard_key_t(const dboard_id_t &id = dboard_id_t::none(), bool restricted = false):
        _rx_id(id), _tx_id(id), _xcvr(false), _restricted(restricted) {}

    dboard_key_t(const dboard_id_t &rx_id, const dboard_id_t &tx_id, bool restricted = false):
        _rx_id(rx_id), _tx_id(tx_id), _xcvr(true), _restricted(restricted) {}

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

    bool is_restricted(void) const{
        return this->_restricted;
    }

private:
    dboard_id_t _rx_id, _tx_id;
    bool _xcvr;
    bool _restricted;
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
//dboard registry tuple: dboard constructor, canonical name, subdev names, container constructor
typedef boost::tuple<dboard_manager::dboard_ctor_t, std::string, std::vector<std::string>, dboard_manager::dboard_ctor_t> args_t;

//map a dboard id to a dboard constructor
typedef uhd::dict<dboard_key_t, args_t> id_to_args_map_t;
UHD_SINGLETON_FCN(id_to_args_map_t, get_id_to_args_map)

static void register_dboard_key(
    const dboard_key_t &dboard_key,
    dboard_manager::dboard_ctor_t db_subdev_ctor,
    const std::string &name,
    const std::vector<std::string> &subdev_names,
    dboard_manager::dboard_ctor_t db_container_ctor
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
    get_id_to_args_map()[dboard_key] = args_t(db_subdev_ctor, name, subdev_names, db_container_ctor);
}

void dboard_manager::register_dboard(
    const dboard_id_t &dboard_id,
    dboard_ctor_t db_subdev_ctor,
    const std::string &name,
    const std::vector<std::string> &subdev_names,
    dboard_ctor_t db_container_ctor
){
    register_dboard_key(dboard_key_t(dboard_id), db_subdev_ctor, name, subdev_names, db_container_ctor);
}

void dboard_manager::register_dboard(
    const dboard_id_t &rx_dboard_id,
    const dboard_id_t &tx_dboard_id,
    dboard_ctor_t db_subdev_ctor,
    const std::string &name,
    const std::vector<std::string> &subdev_names,
    dboard_ctor_t db_container_ctor
){
    register_dboard_key(dboard_key_t(rx_dboard_id, tx_dboard_id), db_subdev_ctor, name, subdev_names, db_container_ctor);
}

void dboard_manager::register_dboard_restricted(
    const dboard_id_t &dboard_id,
    dboard_ctor_t db_subdev_ctor,
    const std::string &name,
    const std::vector<std::string> &subdev_names,
    dboard_ctor_t db_container_ctor
){
    register_dboard_key(dboard_key_t(dboard_id, true), db_subdev_ctor, name, subdev_names, db_container_ctor);
}

void dboard_manager::register_dboard_restricted(
    const dboard_id_t &rx_dboard_id,
    const dboard_id_t &tx_dboard_id,
    dboard_ctor_t db_subdev_ctor,
    const std::string &name,
    const std::vector<std::string> &subdev_names,
    dboard_ctor_t db_container_ctor
){
    register_dboard_key(dboard_key_t(rx_dboard_id, tx_dboard_id, true), db_subdev_ctor, name, subdev_names, db_container_ctor);
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
 * dboard manager implementation class
 **********************************************************************/
class dboard_manager_impl : public dboard_manager{

public:
    dboard_manager_impl(
        dboard_eeprom_t rx_eeprom,
        dboard_eeprom_t tx_eeprom,
        dboard_iface::sptr iface,
        property_tree::sptr subtree,
        bool defer_db_init
    );
    virtual ~dboard_manager_impl(void);

    inline const std::vector<std::string>& get_rx_frontends() const {
        return _rx_frontends;
    }

    inline const std::vector<std::string>& get_tx_frontends() const {
        return _tx_frontends;
    }

    void initialize_dboards();

private:
    void init(dboard_eeprom_t, dboard_eeprom_t, property_tree::sptr, bool);
    //list of rx and tx dboards in this dboard_manager
    //each dboard here is actually a subdevice proxy
    //the subdevice proxy is internal to the cpp file
    uhd::dict<std::string, dboard_base::sptr> _rx_dboards;
    uhd::dict<std::string, dboard_base::sptr> _tx_dboards;
    std::vector<dboard_base::sptr>            _rx_containers;
    std::vector<dboard_base::sptr>            _tx_containers;
    std::vector<std::string>                  _rx_frontends;
    std::vector<std::string>                  _tx_frontends;
    dboard_iface::sptr _iface;
    void set_nice_dboard_if(void);
};

/***********************************************************************
 * make routine for dboard manager
 **********************************************************************/
dboard_manager::sptr dboard_manager::make(
    dboard_id_t rx_dboard_id,
    dboard_id_t tx_dboard_id,
    dboard_id_t gdboard_id,
    dboard_iface::sptr iface,
    property_tree::sptr subtree,
    bool defer_db_init
){
    dboard_eeprom_t rx_eeprom;
    dboard_eeprom_t tx_eeprom;
    rx_eeprom.id = rx_dboard_id;
    tx_eeprom.id = (gdboard_id == dboard_id_t::none()) ? tx_dboard_id : gdboard_id;
    return dboard_manager::sptr(
        new dboard_manager_impl(
            rx_eeprom,
            tx_eeprom,
            iface, subtree, defer_db_init
        )
    );
}

dboard_manager::sptr dboard_manager::make(
    dboard_eeprom_t rx_eeprom,
    dboard_eeprom_t tx_eeprom,
    dboard_eeprom_t gdb_eeprom,
    dboard_iface::sptr iface,
    property_tree::sptr subtree,
    bool defer_db_init
){
    return dboard_manager::sptr(
        new dboard_manager_impl(
            rx_eeprom,
            (gdb_eeprom.id == dboard_id_t::none())? tx_eeprom : gdb_eeprom,
            iface, subtree, defer_db_init
        )
    );
}

/***********************************************************************
 * implementation class methods
 **********************************************************************/
dboard_manager_impl::dboard_manager_impl(
    dboard_eeprom_t rx_eeprom,
    dboard_eeprom_t tx_eeprom,
    dboard_iface::sptr iface,
    property_tree::sptr subtree,
    bool defer_db_init
):
    _iface(iface)
{
    try{
        this->init(rx_eeprom, tx_eeprom, subtree, defer_db_init);
    }
    catch(const std::exception &e){
        UHD_MSG(error) << boost::format(
            "The daughterboard manager encountered a recoverable error in init.\n"
            "Loading the \"unknown\" daughterboard implementations to continue.\n"
            "The daughterboard cannot operate until this error is resolved.\n"
        ) << e.what() << std::endl;
        //clean up the stuff added by the call above
        if (subtree->exists("rx_frontends")) subtree->remove("rx_frontends");
        if (subtree->exists("tx_frontends")) subtree->remove("tx_frontends");
        if (subtree->exists("iface"))        subtree->remove("iface");
        dboard_eeprom_t dummy_eeprom;
        dummy_eeprom.id = dboard_id_t::none();
        this->init(dummy_eeprom, dummy_eeprom, subtree, false);
    }
}

void dboard_manager_impl::init(
    dboard_eeprom_t rx_eeprom,
    dboard_eeprom_t tx_eeprom,
    property_tree::sptr subtree,
    bool defer_db_init
){
    //find the dboard key matches for the dboard ids
    dboard_key_t rx_dboard_key, tx_dboard_key, xcvr_dboard_key;
    BOOST_FOREACH(const dboard_key_t &key, get_id_to_args_map().keys()){
        if (key.is_xcvr()){
            if (rx_eeprom.id == key.rx_id() and tx_eeprom.id == key.tx_id()) xcvr_dboard_key = key;
            if (rx_eeprom.id == key.rx_id()) rx_dboard_key = key; //kept to handle warning
            if (rx_eeprom.id == key.tx_id()) tx_dboard_key = key; //kept to handle warning
        }
        else{
            if (rx_eeprom.id == key.xx_id()) rx_dboard_key = key;
            if (tx_eeprom.id == key.xx_id()) tx_dboard_key = key;
        }
    }

    //warn for invalid dboard id xcvr combinations
    if (not xcvr_dboard_key.is_xcvr() and (rx_dboard_key.is_xcvr() or tx_dboard_key.is_xcvr())){
        UHD_MSG(warning) << boost::format(
            "Unknown transceiver board ID combination.\n"
            "Is your daughter-board mounted properly?\n"
            "RX dboard ID: %s\n"
            "TX dboard ID: %s\n"
        ) % rx_eeprom.id.to_pp_string() % tx_eeprom.id.to_pp_string();
    }

    //initialize the gpio pins before creating subdevs
    set_nice_dboard_if();

    //conditionally register the dboard iface in the tree
    if (not (rx_dboard_key.is_restricted() or tx_dboard_key.is_restricted() or xcvr_dboard_key.is_restricted())) {
        subtree->create<dboard_iface::sptr>("iface").set(_iface);
    }

    //dboard constructor args
    dboard_ctor_args_t db_ctor_args;
    db_ctor_args.db_iface = _iface;

    //make xcvr subdevs
    if (xcvr_dboard_key.is_xcvr()){

        //extract data for the xcvr dboard key
        dboard_ctor_t subdev_ctor; std::string name; std::vector<std::string> subdevs; dboard_ctor_t container_ctor;
        boost::tie(subdev_ctor, name, subdevs, container_ctor) = get_id_to_args_map()[xcvr_dboard_key];

        //create the container class.
        //a container class exists per N subdevs registered in a register_dboard* call
        db_ctor_args.sd_name    = "common";
        db_ctor_args.rx_eeprom  = rx_eeprom;
        db_ctor_args.tx_eeprom  = tx_eeprom;
        db_ctor_args.rx_subtree = subtree->subtree("rx_frontends/" + db_ctor_args.sd_name);
        db_ctor_args.tx_subtree = subtree->subtree("tx_frontends/" + db_ctor_args.sd_name);
        if (container_ctor) {
            db_ctor_args.rx_container = container_ctor(&db_ctor_args);
        } else {
            db_ctor_args.rx_container = dboard_base::sptr();
        }
        db_ctor_args.tx_container = db_ctor_args.rx_container;  //Same TX and RX container

        //create the xcvr object for each subdevice
        BOOST_FOREACH(const std::string &subdev, subdevs){
            db_ctor_args.sd_name = subdev;
            db_ctor_args.rx_subtree = subtree->subtree("rx_frontends/" + db_ctor_args.sd_name);
            db_ctor_args.tx_subtree = subtree->subtree("tx_frontends/" + db_ctor_args.sd_name);
            dboard_base::sptr xcvr_dboard = subdev_ctor(&db_ctor_args);
            _rx_dboards[subdev] = xcvr_dboard;
            _tx_dboards[subdev] = xcvr_dboard;
            xcvr_dboard->initialize();
        }

        //initialize the container after all subdevs have been created
        if (container_ctor) {
            if (defer_db_init) {
                _rx_containers.push_back(db_ctor_args.rx_container);
            } else {
                db_ctor_args.rx_container->initialize();
            }
        }

        //Populate frontend names in-order.
        //We cannot use _xx_dboards.keys() here because of the ordering requirement
        _rx_frontends = subdevs;
        _tx_frontends = subdevs;
    }

    //make tx and rx subdevs (separate subdevs for rx and tx dboards)
    else
    {
        //force the rx key to the unknown board for bad combinations
        if (rx_dboard_key.is_xcvr() or rx_dboard_key.xx_id() == dboard_id_t::none()){
            rx_dboard_key = dboard_key_t(0xfff1);
        }

        //extract data for the rx dboard key
        dboard_ctor_t rx_dboard_ctor; std::string rx_name; std::vector<std::string> rx_subdevs; dboard_ctor_t rx_cont_ctor;
        boost::tie(rx_dboard_ctor, rx_name, rx_subdevs, rx_cont_ctor) = get_id_to_args_map()[rx_dboard_key];

        //create the container class.
        //a container class exists per N subdevs registered in a register_dboard* call
        db_ctor_args.sd_name    = "common";
        db_ctor_args.rx_eeprom  = rx_eeprom;
        db_ctor_args.tx_eeprom.id = dboard_id_t::none();
        db_ctor_args.rx_subtree = subtree->subtree("rx_frontends/" + db_ctor_args.sd_name);
        db_ctor_args.tx_subtree = property_tree::sptr();
        if (rx_cont_ctor) {
            db_ctor_args.rx_container = rx_cont_ctor(&db_ctor_args);
        } else {
            db_ctor_args.rx_container = dboard_base::sptr();
        }

        //make the rx subdevs
        BOOST_FOREACH(const std::string &subdev, rx_subdevs){
            db_ctor_args.sd_name = subdev;
            db_ctor_args.rx_subtree = subtree->subtree("rx_frontends/" + db_ctor_args.sd_name);
            _rx_dboards[subdev] = rx_dboard_ctor(&db_ctor_args);
            _rx_dboards[subdev]->initialize();
        }

        //initialize the container after all subdevs have been created
        if (rx_cont_ctor) {
            if (defer_db_init) {
                _rx_containers.push_back(db_ctor_args.rx_container);
            } else {
                db_ctor_args.rx_container->initialize();
            }
        }

        //force the tx key to the unknown board for bad combinations
        if (tx_dboard_key.is_xcvr() or tx_dboard_key.xx_id() == dboard_id_t::none()){
            tx_dboard_key = dboard_key_t(0xfff0);
        }

        //extract data for the tx dboard key
        dboard_ctor_t tx_dboard_ctor; std::string tx_name; std::vector<std::string> tx_subdevs; dboard_ctor_t tx_cont_ctor;
        boost::tie(tx_dboard_ctor, tx_name, tx_subdevs, tx_cont_ctor) = get_id_to_args_map()[tx_dboard_key];

        //create the container class.
        //a container class exists per N subdevs registered in a register_dboard* call
        db_ctor_args.sd_name    = "common";
        db_ctor_args.rx_eeprom.id = dboard_id_t::none();
        db_ctor_args.tx_eeprom  = tx_eeprom;
        db_ctor_args.rx_subtree = property_tree::sptr();
        db_ctor_args.tx_subtree = subtree->subtree("tx_frontends/" + db_ctor_args.sd_name);
        if (tx_cont_ctor) {
            db_ctor_args.tx_container = tx_cont_ctor(&db_ctor_args);
        } else {
            db_ctor_args.tx_container = dboard_base::sptr();
        }

        //make the tx subdevs
        BOOST_FOREACH(const std::string &subdev, tx_subdevs){
            db_ctor_args.sd_name = subdev;
            db_ctor_args.tx_subtree = subtree->subtree("tx_frontends/" + db_ctor_args.sd_name);
            _tx_dboards[subdev] = tx_dboard_ctor(&db_ctor_args);
            _tx_dboards[subdev]->initialize();
        }

        //initialize the container after all subdevs have been created
        if (tx_cont_ctor) {
            if (defer_db_init) {
                _tx_containers.push_back(db_ctor_args.tx_container);
            } else {
                db_ctor_args.tx_container->initialize();
            }
        }

        //Populate frontend names in-order.
        //We cannot use _xx_dboards.keys() here because of the ordering requirement
        _rx_frontends = rx_subdevs;
        _tx_frontends = tx_subdevs;
    }
}

void dboard_manager_impl::initialize_dboards(void) {
    BOOST_FOREACH(dboard_base::sptr& _rx_container, _rx_containers) {
        _rx_container->initialize();
    }

    BOOST_FOREACH(dboard_base::sptr& _tx_container, _tx_containers) {
        _tx_container->initialize();
    }
}

dboard_manager_impl::~dboard_manager_impl(void){UHD_SAFE_CALL(
    set_nice_dboard_if();
)}

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
}
