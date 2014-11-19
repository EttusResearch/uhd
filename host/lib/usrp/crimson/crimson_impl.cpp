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
	if (req[req.length()-1] >= '0' && req[req.length()-1] <= '4') {
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
	if (pre[pre.length()-1] >= '0' && pre[pre.length()-1] <= '4') {
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
#define TREE_CREATE(PATH, SET, GET, TYPE, HANDLER)					\
	do { _tree->create<TYPE> (mb_path / PATH)					\
    		.set( get ## HANDLER (GET))						\
		.subscribe(boost::bind(&crimson_impl::set ## HANDLER, this, (SET), _1))	\
		.publish  (boost::bind(&crimson_impl::get ## HANDLER, this, (GET)    ));\
	} while(0)

// Macro to create the tree, all properties created with this are RO properties
#define TREE_CREATE_READ(PATH, SET, GET, TYPE, HANDLER)					\
	do { _tree->create<TYPE> (mb_path / PATH)					\
    		.set( get ## HANDLER (GET))						\
		.publish  (boost::bind(&crimson_impl::get ## HANDLER, this, (GET)    ));\
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

    const fs_path mb_path = "/mboards/0";
    TREE_CREATE("id",         "product,set,id",         "product,get,id",         std::string, _string);
    TREE_CREATE("name",       "product,set,name",       "product,get,name",       std::string, _string);
    TREE_CREATE("serial",     "product,set,serial",     "product,get,serial",     std::string, _string);
    TREE_CREATE("fw_version", "product,set,fw_version", "product,get,fw_version", std::string, _string);
    TREE_CREATE("hw_version", "product,set,hw_version", "product,get,hw_version", std::string, _string);
    TREE_CREATE("sw_version", "product,set,sw_version", "product,get,sw_version", std::string, _string);
    TREE_CREATE("vendor",     "product,set,vendor",     "product,get,vendor",     std::string, _string);    

    TREE_CREATE("time" / "id",         "time,set,id",         "time,get,id",         std::string, _string);
    TREE_CREATE("time" / "name",       "time,set,name",       "time,get,name",       std::string, _string);
    TREE_CREATE("time" / "serial",     "time,set,serial",     "time,get,serial",     std::string, _string);
    TREE_CREATE("time" / "fw_version", "time,set,fw_version", "time,get,fw_version", std::string, _string);
    TREE_CREATE("time" / "hw_version", "time,set,hw_version", "time,get,hw_version", std::string, _string);
    TREE_CREATE("time" / "sw_version", "time,set,sw_version", "time,get,sw_version", std::string, _string);

    TREE_CREATE("rx" / "id",         "rx,set,id",         "rx,get,id",         std::string, _string);
    TREE_CREATE("rx" / "name",       "rx,set,name",       "rx,get,name",       std::string, _string);
    TREE_CREATE("rx" / "serial",     "rx,set,serial",     "rx,get,serial",     std::string, _string);
    TREE_CREATE("rx" / "fw_version", "rx,set,fw_version", "rx,get,fw_version", std::string, _string);
    TREE_CREATE("rx" / "hw_version", "rx,set,hw_version", "rx,get,hw_version", std::string, _string);
    TREE_CREATE("rx" / "sw_version", "rx,set,sw_version", "rx,get,sw_version", std::string, _string);
    TREE_CREATE("rx" / "spec",       "rx,set,spec",       "rx,get,spec",       std::string, _string);

    TREE_CREATE("tx" / "id",         "tx,set,id",         "tx,get,id",         std::string, _string);
    TREE_CREATE("tx" / "name",       "tx,set,name",       "tx,get,name",       std::string, _string);
    TREE_CREATE("tx" / "serial",     "tx,set,serial",     "tx,get,serial",     std::string, _string);
    TREE_CREATE("tx" / "fw_version", "tx,set,fw_version", "tx,get,fw_version", std::string, _string);
    TREE_CREATE("tx" / "hw_version", "tx,set,hw_version", "tx,get,hw_version", std::string, _string);
    TREE_CREATE("tx" / "sw_version", "tx,set,sw_version", "tx,get,sw_version", std::string, _string);
    TREE_CREATE("tx" / "spec",       "tx,set,spec",       "tx,get,spec",       std::string, _string);

    // Link max rate refers to ethernet link rate
    TREE_CREATE("link_max_rate","product,set,link_max_rate", "product,get,link_max_rate", double, _double);

    // This is the master clock rate
    TREE_CREATE("tick_rate", 	"product,set,tick_rate",     "product,get,tick_rate",     double, _double);

    TREE_CREATE("time" / "cmd", "time,set,cmd", "time,get,cmd", time_spec_t, _time_spec);
    TREE_CREATE("time" / "now", "time,set,now", "time,get,now", time_spec_t, _time_spec);
    TREE_CREATE("time" / "pps", "time,set,pps", "time,get,pps", time_spec_t, _time_spec);

    TREE_CREATE("eeprom", "product,set,eeprom", "product,get,eeprom", mboard_eeprom_t, _mboard_eeprom);

    static const std::vector<std::string> time_sources = boost::assign::list_of("internal")("external");
    _tree->create<std::vector<std::string> >(mb_path / "time_source" / "options").set(time_sources);


    // This property chooses internal or external time source
    TREE_CREATE("time_source" / "value",  "time,set,time_src_val", "time,get,time_src_val", std::string, _string);
    TREE_CREATE("time_source" / "output", "time,set,time_src_out", "time,get,time_src_out", bool, _bool);

    static const std::vector<double> external_freq_options = boost::assign::list_of(10e6);
    _tree->create<std::vector<double> >(mb_path / "clock_source" / "external" / "freq" / "options");
    static const std::vector<std::string> clock_source_options = boost::assign::list_of("internal")("external");
    _tree->create<std::vector<std::string> >(mb_path / "clock_source" / "options").set(clock_source_options);

    // This property chooses internal or external clock source
    TREE_CREATE("clock_source" / "value",              "time,set,clk_src_val",    "time,get,clk_src_val",    std::string, _string);
    TREE_CREATE("clock_source" / "external",           "time,set,clk_src_ext",    "time,get,clk_src_ext",    std::string, _string);
    TREE_CREATE("clock_source" / "external" / "value", "time,set,clk_src_ext_val","time,get,clk_src_ext_val",double, _double);
    TREE_CREATE("clock_source" / "output",             "time,set,clk_src_out",    "time,get,clk_src_out",    bool, _bool);

    TREE_CREATE("sensors" / "ref_locked", "product,set,sensors", "product,get,sensors", sensor_value_t, _sensor_value);

    // Subdevs
    _tree->create<subdev_spec_t> ("rx_subdev_spec");
    _tree->create<subdev_spec_t> ("tx_subdev_spec");

    // loop for all RF chains
    for (int chain = 0; chain < 4; chain ++) {
	std::string num  = boost::lexical_cast<std::string>(chain + 1);
	std::string chan = "chan" + num;

	// Actual frequency values
	TREE_CREATE_READ("rx" / chan / "freq" / "value", "rx,set,freq_value_" + num, "rx,get,freq_value_" + num, double, _double);
	TREE_CREATE_READ("tx" / chan / "freq" / "value", "tx,set,freq_value_" + num, "tx,get,freq_value_" + num, double, _double);

	// Codecs, phony properties for Crimson
	const fs_path rx_codec_path = "rx_codecs" / chan;
	const fs_path tx_codec_path = "tx_codecs" / chan;
	TREE_CREATE(rx_codec_path / "gains", "rx,set,codec_gains_" + num, "rx,get,codec_gains_" + num, int, _int);
	TREE_CREATE(rx_codec_path / "name",  "rx,set,codec_name_"  + num, "rx,get,codec_name_"  + num, std::string, _string);

	TREE_CREATE(rx_codec_path / "gains" / "digital" / "range", "rx,set,codec_gains_range_" + num, "rx,get,codec_gains_range_" + num, meta_range_t, _meta_range);
	TREE_CREATE(rx_codec_path / "gains" / "digital" / "value", "rx,set,codec_gains_val_"   + num, "rx,get,codec_gains_val_"   + num, double, _double);

	TREE_CREATE(tx_codec_path / "gains", "tx,set,codec_gains_" + num, "tx,get,codec_gains_" + num, int, _int);
	TREE_CREATE(tx_codec_path / "name",  "tx,set,codec_name_"  + num, "tx,get,codec_name_"  + num,  std::string, _string);

	// Front end corrections
	const fs_path rx_fe_path = "rx_frontends" / chan;
	const fs_path tx_fe_path = "tx_frontends" / chan;
	TREE_CREATE(rx_fe_path / "dc_offset" / "value",  "rx,set,fe_dc_off_val_"    + num, "rx,get,fe_dc_off_val_"    + num, std::complex<double>, _complex_double);
	TREE_CREATE(rx_fe_path / "dc_offset" / "enable", "rx,set,fe_dc_off_enable_" + num, "rx,get,fe_dc_off_enable_" + num, bool, _bool);
	TREE_CREATE(rx_fe_path / "iq_balance" / "value", "rx,set,fe_dc_off_bal_"    + num, "rx,get,fe_dc_off_bal_"    + num, std::complex<double>, _complex_double);

	TREE_CREATE(tx_fe_path / "dc_offset" / "value",  "tx,set,fe_dc_off_val_" + num, "tx,get,fe_dc_off_val_" + num, std::complex<double>, _complex_double);
	TREE_CREATE(tx_fe_path / "iq_balance" / "value", "tx,set,fe_dc_off_bal_" + num, "tx,get,fe_dc_off_bal_" + num, std::complex<double>, _complex_double);

	// Front End
	TREE_CREATE(rx_fe_path / "freq" / "value", "rx,set,fe_freq_val_"  + num, "rx,get,fe_freq_val_"  + num, double, _double);
	TREE_CREATE(rx_fe_path / "freq" / "range", "rx,set,fe_freq_range_"+ num, "rx,get,fe_freq_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(rx_fe_path / "gain" / "value", "rx,set,fe_gain_val_"  + num, "rx,get,fe_gain_val_"  + num, double, _double);
	TREE_CREATE(rx_fe_path / "gain" / "range", "rx,set,fe_gain_range_"+ num, "rx,get,fe_gain_range_"+ num, meta_range_t, _meta_range);

	TREE_CREATE(tx_fe_path / "freq" / "value", "tx,set,fe_freq_val_"  + num, "tx,get,fe_freq_val_"  + num, double, _double);
	TREE_CREATE(tx_fe_path / "freq" / "range", "tx,set,fe_freq_range_"+ num, "tx,get,fe_freq_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(tx_fe_path / "gain" / "value", "tx,set,fe_gain_val_"  + num, "tx,get,fe_gain_val_"  + num, double, _double);
	TREE_CREATE(tx_fe_path / "gain" / "range", "tx,set,fe_gain_range_"+ num, "tx,get,fe_gain_range_"+ num, meta_range_t, _meta_range);

	// DSPs
	const fs_path rx_dsp_path = "rx_dsps" / chan;
	const fs_path tx_dsp_path = "tx_dsps" / chan;
	TREE_CREATE(rx_dsp_path / "rate" / "range", "rx,set,dsp_rate_range_"+ num, "rx,get,dsp_rate_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(rx_dsp_path / "rate" / "value", "rx,set,dsp_rate_val_"  + num, "rx,get,dsp_rate_val_"  + num, double, _double);
	TREE_CREATE(rx_dsp_path / "freq" / "value", "rx,set,dsp_freq_val_"  + num, "rx,get,dsp_freq_val_"  + num, double, _double);
	TREE_CREATE(rx_dsp_path / "freq" / "range", "rx,set,dsp_freq_range_"+ num, "rx,get,dsp_freq_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(rx_dsp_path / "bw" / "value",   "rx,set,dsp_bw_val_"    + num, "rx,get,dsp_bw_val_"    + num, double, _double);
	TREE_CREATE(rx_dsp_path / "bw" / "range",   "rx,set,dsp_bw_range_"  + num, "rx,get,dsp_bw_range_"  + num, meta_range_t, _meta_range);
	TREE_CREATE(rx_dsp_path / "stream_cmd",     "rx,set,dsp_cmd_"       + num, "rx,get,dsp_cmd_"       + num, stream_cmd_t, _stream_cmd);

	TREE_CREATE(tx_dsp_path / "rate" / "range", "tx,set,dsp_rate_range_"+ num, "tx,get,dsp_rate_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(tx_dsp_path / "rate" / "value", "tx,set,dsp_rate_val_"  + num, "tx,get,dsp_rate_val_"  + num, double, _double);
	TREE_CREATE(tx_dsp_path / "freq" / "value", "tx,set,dsp_freq_val_"  + num, "tx,get,dsp_freq_val_"  + num, double, _double);
	TREE_CREATE(tx_dsp_path / "freq" / "range", "tx,set,dsp_freq_range_"+ num, "tx,get,dsp_freq_range_"+ num, meta_range_t, _meta_range);
	TREE_CREATE(tx_dsp_path / "bw" / "value",   "tx,set,dsp_bw_val_"    + num, "tx,get,dsp_bw_val_"    + num, double, _double);
	TREE_CREATE(tx_dsp_path / "bw" / "range",   "tx,set,dsp_bw_range_"  + num, "tx,get,dsp_bw_range_"  + num, meta_range_t, _meta_range);

	// RF Front end iface, these are phony properties for Crimson
	const fs_path db_path = "dboards" / chan;
	TREE_CREATE(db_path / "rx_eeprom",  "product,set,rx_eeprom_" + num, "product,get,rx_eeprom_"  + num, dboard_eeprom_t, _dboard_eeprom);
	TREE_CREATE(db_path / "tx_eeprom",  "product,set,tx_eeprom_" + num, "product,get,tx_eeprom_"  + num, dboard_eeprom_t, _dboard_eeprom);
	TREE_CREATE(db_path / "gdb_eeprom", "product,set,gdb_eeprom_"+ num, "product,get,gdb_eeprom_" + num, dboard_eeprom_t, _dboard_eeprom);

	_tree->create<subdev_spec_t> (mb_path / "dboards" / chan / "rx_frontends");
	_tree->create<subdev_spec_t> (mb_path / "dboards" / chan / "tx_frontends");
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






