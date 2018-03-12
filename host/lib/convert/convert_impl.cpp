//
// Copyright 2011,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <stdint.h>
#include <boost/format.hpp>
#include <complex>

using namespace uhd;

convert::converter::~converter(void){
    /* NOP */
}

bool convert::operator==(const convert::id_type &lhs, const convert::id_type &rhs){
    return true
        and (lhs.input_format  == rhs.input_format)
        and (lhs.num_inputs    == rhs.num_inputs)
        and (lhs.output_format == rhs.output_format)
        and (lhs.num_outputs   == rhs.num_outputs)
    ;
}

std::string convert::id_type::to_pp_string(void) const{
    return str(boost::format(
        "conversion ID\n"
        "  Input format:  %s\n"
        "  Num inputs:    %d\n"
        "  Output format: %s\n"
        "  Num outputs:   %d\n"
    )
        % this->input_format
        % this->num_inputs
        % this->output_format
        % this->num_outputs
    );
}

std::string convert::id_type::to_string(void) const{
    return str(boost::format("%s (%d) -> %s (%d)")
        % this->input_format
        % this->num_inputs
        % this->output_format
        % this->num_outputs
    );
}

/***********************************************************************
 * Setup the table registry
 **********************************************************************/
typedef uhd::dict<convert::id_type, uhd::dict<convert::priority_type, convert::function_type> > fcn_table_type;
UHD_SINGLETON_FCN(fcn_table_type, get_table);

/***********************************************************************
 * The registry functions
 **********************************************************************/
void uhd::convert::register_converter(
    const id_type &id,
    const function_type &fcn,
    const priority_type prio
){
    get_table()[id][prio] = fcn;

    //----------------------------------------------------------------//
    //UHD_LOG_TRACE("CONVERT", boost::format("register_converter: %s prio: %s") % id.to_string() % prio)
    //----------------------------------------------------------------//
}

/***********************************************************************
 * The converter functions
 **********************************************************************/
convert::function_type convert::get_converter(
    const id_type &id,
    const priority_type prio
){
    if (not get_table().has_key(id)) throw uhd::key_error(
        "Cannot find a conversion routine for " + id.to_pp_string());

    //find a matching priority
    priority_type best_prio = -1;
    for(priority_type prio_i:  get_table()[id].keys()){
        if (prio_i == prio) {
            //----------------------------------------------------------------//
            UHD_LOGGER_DEBUG("CONVERT") << "get_converter: For converter ID: " << id.to_pp_string()
                                        << " Using prio: " << prio;
            ;
            //----------------------------------------------------------------//
            return get_table()[id][prio];
        }
        best_prio = std::max(best_prio, prio_i);
    }

    //wanted a specific prio, didnt find
    if (prio != -1) throw uhd::key_error(
        "Cannot find a conversion routine [with prio] for " + id.to_pp_string());

    //----------------------------------------------------------------//
    UHD_LOGGER_DEBUG("CONVERT") << "get_converter: For converter ID: " << id.to_pp_string()
                                << " Using prio: " << best_prio;
    //----------------------------------------------------------------//

    //otherwise, return best prio
    return get_table()[id][best_prio];
}

/***********************************************************************
 * Mappings for item format to byte size for all items we can
 **********************************************************************/
typedef uhd::dict<std::string, size_t> item_size_type;
UHD_SINGLETON_FCN(item_size_type, get_item_size_table);

void convert::register_bytes_per_item(
    const std::string &format, const size_t size
){
    get_item_size_table()[format] = size;
}

size_t convert::get_bytes_per_item(const std::string &format){
    if (get_item_size_table().has_key(format)) return get_item_size_table()[format];

    //OK. I am sorry about this.
    //We didnt find a match, so lets find a match for the first term.
    //This is partially a hack because of the way I append strings.
    //But as long as life is kind, we can keep this.
    const size_t pos = format.find("_");
    if (pos != std::string::npos){
        return get_bytes_per_item(format.substr(0, pos));
    }

    throw uhd::key_error("Cannot find an item size:\n" + format);
}

UHD_STATIC_BLOCK(convert_register_item_sizes){
    //register standard complex types
    convert::register_bytes_per_item("fc64", sizeof(std::complex<double>));
    convert::register_bytes_per_item("fc32", sizeof(std::complex<float>));
    convert::register_bytes_per_item("sc64", sizeof(std::complex<int64_t>));
    convert::register_bytes_per_item("sc32", sizeof(std::complex<int32_t>));
    convert::register_bytes_per_item("sc16", sizeof(std::complex<int16_t>));
    convert::register_bytes_per_item("sc8", sizeof(std::complex<int8_t>));

    //register standard real types
    convert::register_bytes_per_item("f64", sizeof(double));
    convert::register_bytes_per_item("f32", sizeof(float));
    convert::register_bytes_per_item("s64", sizeof(int64_t));
    convert::register_bytes_per_item("s32", sizeof(int32_t));
    convert::register_bytes_per_item("s16", sizeof(int16_t));
    convert::register_bytes_per_item("s8", sizeof(int8_t));
    convert::register_bytes_per_item("u8", sizeof(uint8_t));

    //register VITA types
    convert::register_bytes_per_item("item32", sizeof(int32_t));
}
