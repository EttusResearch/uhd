//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/dboard/id.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <map>

using namespace usrp_uhd::usrp::dboard;

std::ostream& operator<<(std::ostream &os, const dboard_id_t &id){
    //map the dboard ids to string representations
    std::map<dboard_id_t, std::string> id_to_str = boost::assign::map_list_of
        (ID_BASIC_TX, "basic tx")
        (ID_BASIC_RX, "basic rx")
    ;

    //get the string representation
    if (id_to_str.count(id) != 0){
        os << id_to_str[id];
    }
    else{
        os << boost::format("dboard id %u") % unsigned(id);
    }
    return os;
}
