//
// Copyright 2014 Per Vices Corporation
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

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include "apply_corrections.hpp"
#include <uhd/utils/static.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/images.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/functional/hash.hpp>
#include <boost/assign/list_of.hpp>
#include <fstream>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/transport/udp_constants.hpp>
#include <uhd/transport/nirio_zero_copy.hpp>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/utils/platform.hpp>
#include <string>
#include <vector>
#include <uhd/types/ranges.hpp>
#include "crimson_impl.hpp"

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
// seperates the input data into the vector tokens based on delim
void csv_parse(std::vector<std::string> &tokens, char* data, const char delim) {
	int i = 0;
	while (data[i]) {
		std::string token = "";
		while (data[i] && data[i] != delim) {
			token.push_back(data[i]);
			if (data[i+1] == 0 || data[i+1] == delim)
				tokens.push_back(token);
			i++;
		}
		i++;
	}
	return;
}

// base wrapper that calls the simple UDP interface to get messages to and from Crimson
std::string crimson_impl::get_string(std::string req) {
    	_iface -> poke_str(req);
	std::string ret = _iface -> peek_str();
	if (ret == "TIMEOUT") 	throw uhd::runtime_error("crimson_impl::get_string - UDP resp. timed out");
	else 			return ret;
}
void crimson_impl::set_string(const std::string pre, std::string data) {
	_iface -> poke_str(pre + "," + data);

	// read anyways for error check, since Crimson will reply back
	std::string ret = _iface -> peek_str();
	if (ret == "TIMEOUT" || ret == "ERROR")
		throw uhd::runtime_error("crimson_impl::get_string - UDP resp. timed out");
	else
		return;
}

// wrapper for type <double> through the ASCII Crimson interface
double crimson_impl::get_double(std::string req) {
	try { return boost::lexical_cast<double>( get_string(req) );
	} catch (...) { return 0; }
}
void crimson_impl::set_double(const std::string pre, double data){
	try { set_string(pre, boost::lexical_cast<std::string>(data));
	} catch (...) { }
}

// wrapper for type <bool> through the ASCII Crimson interface
bool crimson_impl::get_bool(std::string req) {
	try { return boost::lexical_cast<bool>( get_string(req) );
	} catch (...) { return 0; }
}
void crimson_impl::set_bool(const std::string pre, bool data){
	try { set_string(pre, boost::lexical_cast<std::string>(data));
	} catch (...) { }
}

// wrapper for type <int> through the ASCII Crimson interface
int crimson_impl::get_int(std::string req) {
	try { return boost::lexical_cast<int>( get_string(req) );
	} catch (...) { return 0; }
}
void crimson_impl::set_int(const std::string pre, int data){
	try { set_string(pre, boost::lexical_cast<std::string>(data));
	} catch (...) { }
}

// wrapper for type <mboard_eeprom_t> through the ASCII Crimson interface
mboard_eeprom_t crimson_impl::get_mboard_eeprom(std::string req) {
	mboard_eeprom_t temp;
	temp["name"]     = get_string("product,get,name");
	temp["vendor"]   = get_string("product,get,vendor");
	temp["serial"]   = get_string("product,get,serial");
	return temp;
}
void crimson_impl::set_mboard_eeprom(const std::string pre, mboard_eeprom_t data) {
	// no eeprom settings on Crimson
	return;
}

// wrapper for type <dboard_eeprom_t> through the ASCII Crimson interface
dboard_eeprom_t crimson_impl::get_dboard_eeprom(std::string req) {
	dboard_eeprom_t temp;
	//temp.id       = dboard_id_t( boost::lexical_cast<boost::uint16_t>(get_string("product,get,serial")) );
	temp.serial   = "";//get_string("product,get,serial");
	//temp.revision = get_string("product,get,hw_version");
	return temp;
}
void crimson_impl::set_dboard_eeprom(const std::string pre, dboard_eeprom_t data) {
	// no eeprom settings on Crimson
	return;
}

// wrapper for type <sensor_value_t> through the ASCII Crimson interface
sensor_value_t crimson_impl::get_sensor_value(std::string req) {
	// no sensors on Crimson
	return sensor_value_t("NA", "0", "NA");
}
void crimson_impl::set_sensor_value(const std::string pre, sensor_value_t data) {
	// no sensors on Crimson
	return;
}

// wrapper for type <meta_range_t> through the ASCII Crimson interface
meta_range_t crimson_impl::get_meta_range(std::string req) {
	meta_range_t temp;
	if (req[req.length()-1] >= 'A' && req[req.length()-1] <= 'D') {
		temp = meta_range_t(	
			get_double( req.substr(0, req.length()-1) + "start_" + req.substr(req.length()-1, 1)),
			get_double( req.substr(0, req.length()-1) + "stop_"  + req.substr(req.length()-1, 1)),
			get_double( req.substr(0, req.length()-1) + "step_"  + req.substr(req.length()-1, 1))
		);
	} else {
		temp = meta_range_t(	
			get_double( req + "_start"),
			get_double( req + "_stop"),
			get_double( req + "_step")
		);
	}
	return temp;
}
void crimson_impl::set_meta_range(const std::string pre, meta_range_t data) {
	if (pre[pre.length()-1] >= 'A' && pre[pre.length()-1] <= 'D') {
		set_double( pre.substr(0, pre.length()-1) + "start_" + pre.substr(pre.length()-1, 1), data.start());
		set_double( pre.substr(0, pre.length()-1) + "stop_"  + pre.substr(pre.length()-1, 1), data.stop());
		set_double( pre.substr(0, pre.length()-1) + "step_"  + pre.substr(pre.length()-1, 1), data.step());
	} else {
		set_double( pre + "_start", data.start());
		set_double( pre + "_stop",  data.stop() );
		set_double( pre + "_step",  data.step() );
	}
	return;
}

// wrapper for type <complex<double>> through the ASCII Crimson interface
std::complex<double>  crimson_impl::get_complex_double(std::string req) {
	std::complex<double> temp;
	return temp;
}
void crimson_impl::set_complex_double(const std::string pre, std::complex<double> data) {
	return;
}

// wrapper for type <stream_cmd_t> through the ASCII Crimson interface
stream_cmd_t crimson_impl::get_stream_cmd(std::string req) {
	stream_cmd_t::stream_mode_t mode = stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
	stream_cmd_t temp = stream_cmd_t(mode);
	return temp;
}
void crimson_impl::set_stream_cmd(const std::string pre, stream_cmd_t data) {
	return;
}

// wrapper for type <time_spec_t> through the ASCII Crimson interface
time_spec_t crimson_impl::get_time_spec(std::string req) {
	time_spec_t temp = time_spec_t(get_double(req));
	return temp;
}
void crimson_impl::set_time_spec(const std::string pre, time_spec_t data) {
	set_double(pre, data.get_frac_secs());
	return;
}

/***********************************************************************
 * Discovery over the udp transport
 **********************************************************************/
// This find function will be called if a hint is passed onto the find function
static device_addrs_t crimson_find_with_addr(const device_addr_t &hint)
{
    // temporarily make a UDP device only to look for devices
    udp_simple::sptr comm = udp_simple::make_broadcast(
        hint["addr"], BOOST_STRINGIZE(CRIMSON_FW_COMMS_UDP_PORT));

    //send request for echo
    comm->send(asio::buffer("product,1,get,serial", sizeof("product,1,get,serial")));

    //loop for replies from the broadcast until it times out
    device_addrs_t addrs;
    while (true)
    {
        char buff[CRIMSON_FW_COMMS_MTU] = {};
        const size_t nbytes = comm->recv(asio::buffer(buff), 0.050);
        if (nbytes == 0) break;

	// parse the return buffer and store it in a vector
	std::vector<std::string> tokens;
	csv_parse(tokens, buff, ',');
	if (tokens[0] == "ERROR") break;

        device_addr_t new_addr;
        new_addr["type"]    = "crimson";
        new_addr["addr"]    = comm->get_recv_addr();
	new_addr["name"]    = "";
        new_addr["serial"]  = tokens[1];

        //filter the discovered device below by matching optional keys
        if (
            (not hint.has_key("name")    or hint["name"]    == new_addr["name"]) and
            (not hint.has_key("serial")  or hint["serial"]  == new_addr["serial"]) and
            (not hint.has_key("product") or hint["product"] == new_addr["product"])
        ){
            addrs.push_back(new_addr);
        }
    }

    return addrs;
}

// This is the core find function that will be called when uhd:device find() is called because this is registered
static device_addrs_t crimson_find(const device_addr_t &hint_)
{
    //handle the multi-device discovery
    device_addrs_t hints = separate_device_addr(hint_);
    if (hints.size() > 1)
    {
        device_addrs_t found_devices;
        std::string error_msg;
        BOOST_FOREACH(const device_addr_t &hint_i, hints)
        {
            device_addrs_t found_devices_i = crimson_find(hint_i);
            if (found_devices_i.size() != 1) error_msg += str(boost::format(
                "Could not resolve device hint \"%s\" to a single device."
            ) % hint_i.to_string());
            else found_devices.push_back(found_devices_i[0]);
        }
        if (found_devices.empty()) return device_addrs_t();
        if (not error_msg.empty()) throw uhd::value_error(error_msg);

        return device_addrs_t(1, combine_device_addrs(found_devices));
    }

    //initialize the hint for a single device case
    UHD_ASSERT_THROW(hints.size() <= 1);
    hints.resize(1); //in case it was empty
    device_addr_t hint = hints[0];
    device_addrs_t addrs;
    if (hint.has_key("type") and hint["type"] != "crimson") return addrs;


    //use the address given
    if (hint.has_key("addr"))
    {
        device_addrs_t reply_addrs;
        try
        {
            reply_addrs = crimson_find_with_addr(hint);
        }
        catch(const std::exception &ex)
        {
            UHD_MSG(error) << "CRIMSON Network discovery error " << ex.what() << std::endl;
        }
        catch(...)
        {
            UHD_MSG(error) << "CRIMSON Network discovery unknown error " << std::endl;
        }
        BOOST_FOREACH(const device_addr_t &reply_addr, reply_addrs)
        {
            device_addrs_t new_addrs = crimson_find_with_addr(reply_addr);
            addrs.insert(addrs.begin(), new_addrs.begin(), new_addrs.end());
        }
        return addrs;
    }

    if (!hint.has_key("resource"))
    {
        //otherwise, no address was specified, send a broadcast on each interface
        BOOST_FOREACH(const if_addrs_t &if_addrs, get_if_addrs())
        {
            //avoid the loopback device
            if (if_addrs.inet == asio::ip::address_v4::loopback().to_string()) continue;

            //create a new hint with this broadcast address
            device_addr_t new_hint = hint;
            new_hint["addr"] = if_addrs.bcast;

            //call discover with the new hint and append results
            device_addrs_t new_addrs = crimson_find(new_hint);
            addrs.insert(addrs.begin(), new_addrs.begin(), new_addrs.end());
        }
    }

    return addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
// Returns a pointer to the Crimson device, casted to the UHD base class
static device::sptr crimson_make(const device_addr_t &device_addr)
{
    return device::sptr(new crimson_impl(device_addr));
}

// This is the core function that registers itself with uhd::device base class. The base device class
// will have a reference to all the registered devices and upon device find/make it will loop through
// all the registered devices' find and make functions.
UHD_STATIC_BLOCK(register_crimson_device)
{
    device::register_device(&crimson_find, &crimson_make, device::CRIMSON);
}

/***********************************************************************
 * Structors
 **********************************************************************/
// Macro to create the tree, all properties created with this are R/W properties
#define TREE_CREATE(PATH, CATEGORY, PROP, TYPE, HANDLER)							\
	do { _tree->create<TYPE> (PATH)										\
    		.set( get ## HANDLER (CATEGORY ",get," PROP))							\
		.subscribe(boost::bind(&crimson_impl::set ## HANDLER, this, (CATEGORY ",set," PROP), _1))	\
		.publish  (boost::bind(&crimson_impl::get ## HANDLER, this, (CATEGORY ",get," PROP)    ));	\
	} while(0)

// Macro to create the tree, all properties created with this are RO properties
#define TREE_CREATE_READ(PATH, CATEGORY, PROP, TYPE, HANDLER)							\
	do { _tree->create<TYPE> (PATH)										\
    		.set( get ## HANDLER (CATEGORY ",get," PROP))							\
		.publish  (boost::bind(&crimson_impl::get ## HANDLER, this, (CATEGORY ",get," PROP)    ));	\
	} while(0)

crimson_impl::crimson_impl(const device_addr_t &dev_addr)
{
    UHD_MSG(status) << "Opening a Crimson device..." << std::endl;
    _type = device::CRIMSON;

    // Makes the UDP comm connection
    _iface = crimson_iface::make( udp_simple::make_connected( dev_addr["addr"],
    			BOOST_STRINGIZE(CRIMSON_FW_COMMS_UDP_PORT)) );

    // TODO make transports for each RX/TX chain

    // TODO check if locked already

    // lock the Crimson device to this process, this will prevent the Crimson device being used by another program
    set_string("product,set,locked", "1");

    // Create the file tree of properties.
    // Crimson only has support for one mother board, and the RF chains will show up individually as daughter boards.
    // All the initial settings are read from the current status of the board.
    _tree = uhd::property_tree::make();
    _tree->create<std::string>("/name").set("Crimson Device");

    const fs_path mb_path   = "/mboards/0";
    const fs_path time_path = mb_path / "time";
    const fs_path tx_path   = mb_path / "tx";
    const fs_path rx_path   = mb_path / "rx";

    TREE_CREATE(mb_path / "id",         "product", "id",         std::string, _string);
    TREE_CREATE(mb_path / "name",       "product", "name",       std::string, _string);
    TREE_CREATE(mb_path / "serial",     "product", "serial",     std::string, _string);
    TREE_CREATE(mb_path / "fw_version", "product", "fw_version", std::string, _string);
    TREE_CREATE(mb_path / "hw_version", "product", "hw_version", std::string, _string);
    TREE_CREATE(mb_path / "sw_version", "product", "sw_version", std::string, _string);
    TREE_CREATE(mb_path / "vendor",     "product", "vendor",     std::string, _string);

    TREE_CREATE(time_path / "id",         "time", "id",         std::string, _string);
    TREE_CREATE(time_path / "name",       "time", "name",       std::string, _string);
    TREE_CREATE(time_path / "serial",     "time", "serial",     std::string, _string);
    TREE_CREATE(time_path / "fw_version", "time", "fw_version", std::string, _string);
    TREE_CREATE(time_path / "hw_version", "time", "hw_version", std::string, _string);
    TREE_CREATE(time_path / "sw_version", "time", "sw_version", std::string, _string);

    TREE_CREATE(rx_path / "id",         "rx", "id",         std::string, _string);
    TREE_CREATE(rx_path / "name",       "rx", "name",       std::string, _string);
    TREE_CREATE(rx_path / "serial",     "rx", "serial",     std::string, _string);
    TREE_CREATE(rx_path / "fw_version", "rx", "fw_version", std::string, _string);
    TREE_CREATE(rx_path / "hw_version", "rx", "hw_version", std::string, _string);
    TREE_CREATE(rx_path / "sw_version", "rx", "sw_version", std::string, _string);
    TREE_CREATE(rx_path / "spec",       "rx", "spec",       std::string, _string);

    TREE_CREATE(tx_path / "id",         "tx", "id",         std::string, _string);
    TREE_CREATE(tx_path / "name",       "tx", "name",       std::string, _string);
    TREE_CREATE(tx_path / "serial",     "tx", "serial",     std::string, _string);
    TREE_CREATE(tx_path / "fw_version", "tx", "fw_version", std::string, _string);
    TREE_CREATE(tx_path / "hw_version", "tx", "hw_version", std::string, _string);
    TREE_CREATE(tx_path / "sw_version", "tx", "sw_version", std::string, _string);
    TREE_CREATE(tx_path / "spec",       "tx", "spec",       std::string, _string);

    // Link max rate refers to ethernet link rate
    TREE_CREATE(mb_path / "link_max_rate","product", "link_max_rate", double, _double);

    // This is the master clock rate
    TREE_CREATE(mb_path / "tick_rate", "product", "tick_rate",     double, _double);

    TREE_CREATE(time_path / "cmd", "time", "cmd", time_spec_t, _time_spec);
    TREE_CREATE(time_path / "now", "time", "now", time_spec_t, _time_spec);
    TREE_CREATE(time_path / "pps", "time", "pps", time_spec_t, _time_spec);

    TREE_CREATE(mb_path / "eeprom", "product", "eeprom", mboard_eeprom_t, _mboard_eeprom);

    static const std::vector<std::string> time_sources = boost::assign::list_of("internal")("external");
    _tree->create<std::vector<std::string> >(mb_path / "time_source" / "options").set(time_sources);


    // This property chooses internal or external time source
    TREE_CREATE(mb_path / "time_source" / "value",  "time", "time_src_val", std::string, _string);
    TREE_CREATE(mb_path / "time_source" / "output", "time", "time_src_out", bool, _bool);

    static const std::vector<double> external_freq_options = boost::assign::list_of(10e6);
    _tree->create<std::vector<double> >(mb_path / "clock_source" / "external" / "freq" / "options");
    static const std::vector<std::string> clock_source_options = boost::assign::list_of("internal")("external");
    _tree->create<std::vector<std::string> >(mb_path / "clock_source" / "options").set(clock_source_options);

    // This property chooses internal or external clock source
    TREE_CREATE(mb_path / "clock_source" / "value",              "time", "clk_src_val",    std::string, _string);
    TREE_CREATE(mb_path / "clock_source" / "external",           "time", "clk_src_ext",    std::string, _string);
    TREE_CREATE(mb_path / "clock_source" / "external" / "value", "time", "clk_src_ext_val",double, _double);
    TREE_CREATE(mb_path / "clock_source" / "output",             "time", "clk_src_out",    bool, _bool);

    TREE_CREATE(mb_path / "sensors" / "ref_locked", "product", "sensors", sensor_value_t, _sensor_value);

    // loop for all RF chains
    for (int chain = 0; chain < 4; chain ++) {
	std::string num  = boost::lexical_cast<std::string>((char)(chain + 65));
	std::string chan = "Channel_" + num;

	// Actual frequency values
	TREE_CREATE_READ(rx_path / chan / "freq" / "value", "rx", "freq_value_" + num, double, _double);
	TREE_CREATE_READ(tx_path / chan / "freq" / "value", "tx", "freq_value_" + num, double, _double);

	// Codecs, phony properties for Crimson
	const fs_path rx_codec_path = mb_path / "rx_codecs" / num;
	const fs_path tx_codec_path = mb_path / "tx_codecs" / num;
	TREE_CREATE(rx_codec_path / "gains", "rx", "codec_gains_" + num, int, _int);
	TREE_CREATE(rx_codec_path / "name",  "rx", "codec_name_"  + num, std::string, _string);

	TREE_CREATE(tx_codec_path / "gains", "tx", "codec_gains_" + num, int, _int);
	TREE_CREATE(tx_codec_path / "name",  "tx", "codec_name_"  + num,  std::string, _string);

	// Duaghter Boards' Frontend Settings
	const fs_path rx_fe_path = mb_path / "dboards" / num / "rx_frontends" / chan;
	const fs_path tx_fe_path = mb_path / "dboards" / num / "tx_frontends" / chan;
	TREE_CREATE(rx_fe_path / "name",  "rx", "name_" + num, std::string, _string);
	TREE_CREATE(tx_fe_path / "name",  "tx", "name_" + num, std::string, _string);

	TREE_CREATE(rx_fe_path / "gains" / "ADRF" / "range",  "rx", "fe_gain_range_" + num, meta_range_t, _meta_range);
	TREE_CREATE(tx_fe_path / "gains" / "RFSA" / "range",  "tx", "fe_gain_range_" + num, meta_range_t, _meta_range);

	TREE_CREATE(rx_fe_path / "freq",  "rx", "fe_freq_range_" + num, meta_range_t, _meta_range);
	TREE_CREATE(tx_fe_path / "freq",  "tx", "fe_freq_range_" + num, meta_range_t, _meta_range);

	TREE_CREATE(rx_fe_path / "dc_offset" / "value",  "rx", "fe_dc_off_val_"    + num, std::complex<double>, _complex_double);
	TREE_CREATE(rx_fe_path / "dc_offset" / "enable", "rx", "fe_dc_off_enable_" + num, bool, _bool);
	TREE_CREATE(rx_fe_path / "iq_balance" / "value", "rx", "fe_dc_off_bal_"    + num, std::complex<double>, _complex_double);

	TREE_CREATE(tx_fe_path / "dc_offset" / "value",  "tx", "fe_dc_off_val_" + num, std::complex<double>, _complex_double);
	TREE_CREATE(tx_fe_path / "iq_balance" / "value", "tx", "fe_dc_off_bal_" + num, std::complex<double>, _complex_double);

        static const std::vector<std::string> antenna_options = boost::assign::list_of("SMA")("None");
        _tree->create<std::vector<std::string> >(rx_fe_path / "antenna" / "options").set(antenna_options);
        _tree->create<std::vector<std::string> >(tx_fe_path / "antenna" / "options").set(antenna_options);

        static const std::vector<std::string> sensor_options = boost::assign::list_of("lo_locked");
        _tree->create<std::vector<std::string> >(rx_fe_path / "sensors").set(sensor_options);
        _tree->create<std::vector<std::string> >(tx_fe_path / "sensors").set(sensor_options);

	TREE_CREATE(rx_fe_path / "connection",  "rx", "connection", std::string, _string);
	TREE_CREATE(tx_fe_path / "connection",  "tx", "connection", std::string, _string);

	TREE_CREATE(rx_fe_path / "use_lo_offset",  "rx", "lo_off_" + num, bool, _bool);
	TREE_CREATE(tx_fe_path / "use_lo_offset",  "tx", "lo_off_" + num, bool, _bool);

	TREE_CREATE(rx_fe_path / "freq" / "value", "rx", "fe_freq_val_"  + num, double, _double);
	TREE_CREATE(rx_fe_path / "freq" / "range", "rx", "fe_freq_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(rx_fe_path / "gain" / "value", "rx", "fe_gain_val_"  + num, double, _double);
	TREE_CREATE(rx_fe_path / "gain" / "range", "rx", "fe_gain_range_"+ num, meta_range_t, _meta_range);

	TREE_CREATE(tx_fe_path / "freq" / "value", "tx", "fe_freq_val_"  + num, double, _double);
	TREE_CREATE(tx_fe_path / "freq" / "range", "tx", "fe_freq_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(tx_fe_path / "gain" / "value", "tx", "fe_gain_val_"  + num, double, _double);
	TREE_CREATE(tx_fe_path / "gain" / "range", "tx", "fe_gain_range_"+ num, meta_range_t, _meta_range);

	// these are phony properties for Crimson
	const fs_path db_path = mb_path / "dboards" / num;
	TREE_CREATE(db_path / "rx_eeprom",  "product", "rx_eeprom_"  + num, dboard_eeprom_t, _dboard_eeprom);
	TREE_CREATE(db_path / "tx_eeprom",  "product", "tx_eeprom_"  + num, dboard_eeprom_t, _dboard_eeprom);
	TREE_CREATE(db_path / "gdb_eeprom", "product", "gdb_eeprom_" + num, dboard_eeprom_t, _dboard_eeprom);

	// DSPs
	const fs_path rx_dsp_path = mb_path / "rx_dsps" / chan;
	const fs_path tx_dsp_path = mb_path / "tx_dsps" / chan;
	TREE_CREATE(rx_dsp_path / "rate" / "range", "rx", "dsp_rate_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(rx_dsp_path / "rate" / "value", "rx", "dsp_rate_val_"  + num, double, _double);
	TREE_CREATE(rx_dsp_path / "freq" / "value", "rx", "dsp_freq_val_"  + num, double, _double);
	TREE_CREATE(rx_dsp_path / "freq" / "range", "rx", "dsp_freq_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(rx_dsp_path / "bw" / "value",   "rx", "dsp_bw_val_"    + num, double, _double);
	TREE_CREATE(rx_dsp_path / "bw" / "range",   "rx", "dsp_bw_range_"  + num, meta_range_t, _meta_range);
	TREE_CREATE(rx_dsp_path / "stream_cmd",     "rx", "dsp_cmd_"       + num, stream_cmd_t, _stream_cmd);

	TREE_CREATE(tx_dsp_path / "rate" / "range", "tx", "dsp_rate_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(tx_dsp_path / "rate" / "value", "tx", "dsp_rate_val_"  + num, double, _double);
	TREE_CREATE(tx_dsp_path / "freq" / "value", "tx", "dsp_freq_val_"  + num, double, _double);
	TREE_CREATE(tx_dsp_path / "freq" / "range", "tx", "dsp_freq_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(tx_dsp_path / "bw" / "value",   "tx", "dsp_bw_val_"    + num, double, _double);
	TREE_CREATE(tx_dsp_path / "bw" / "range",   "tx", "dsp_bw_range_"  + num, meta_range_t, _meta_range);
    }
}

crimson_impl::~crimson_impl(void)
{
    // TODO send commands to mute all radio chains, mute everything
    // unlock the Crimson device to this process
    set_string("product,set,locked", "0");
}

tx_streamer::sptr crimson_impl::get_tx_stream(const stream_args_t &args) {
    throw uhd::not_implemented_error("get_tx_stream not implemented");
}

rx_streamer::sptr crimson_impl::get_rx_stream(const stream_args_t &args) {
    throw uhd::not_implemented_error("get_rx_stream not implemented");
}

bool crimson_impl::recv_async_msg(async_metadata_t &async_metadata, double timeout) {
    throw uhd::not_implemented_error("recv_async_msg not implemented");
}






