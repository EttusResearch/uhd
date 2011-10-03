//
// Copyright 2011 Ettus Research LLC
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

#include <uhd/convert.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <boost/cstdint.hpp>
#include <boost/format.hpp>
#include <complex>

using namespace uhd;

bool convert::operator==(const convert::id_type &lhs, const convert::id_type &rhs){
    return
        (lhs.input_markup == rhs.input_markup) and
        (lhs.num_inputs == rhs.num_inputs) and
        (lhs.output_markup == rhs.output_markup) and
        (lhs.num_outputs == rhs.num_outputs) and
        (lhs.args == rhs.args)
    ;
}

std::string convert::id_type::to_pp_string(void) const{
    return str(boost::format(
        "conversion ID\n"
        "  Input markup: %s\n"
        "  Num inputs: %d\n"
        "  Output markup: %s\n"
        "  Num outputs: %d\n"
        "  Optional args: %s\n"
    )
        % this->input_markup
        % this->num_inputs
        % this->output_markup
        % this->num_outputs
        % this->args
    );
}

/***********************************************************************
 * Define types for the function tables
 **********************************************************************/
struct fcn_table_entry_type{
    convert::priority_type prio;
    convert::function_type fcn;
};

/***********************************************************************
 * Setup the table registry
 **********************************************************************/
typedef uhd::dict<convert::id_type, fcn_table_entry_type> fcn_table_type;
UHD_SINGLETON_FCN(fcn_table_type, get_table);

/***********************************************************************
 * The registry functions
 **********************************************************************/
void uhd::convert::register_converter(
    const id_type &id,
    function_type fcn,
    priority_type prio
){
    //get a reference to the function table
    fcn_table_type &table = get_table();

    //register the function if higher priority
    if (not table.has_key(id) or table[id].prio < prio){
        table[id].fcn = fcn;
        table[id].prio = prio;
    }

    //----------------------------------------------------------------//
    UHD_LOGV(always) << "register_converter: " << id.to_pp_string() << std::endl
        << "    prio: " << prio << std::endl
        << std::endl
    ;
    //----------------------------------------------------------------//
}

/***********************************************************************
 * The converter functions
 **********************************************************************/
convert::function_type convert::get_converter(const id_type &id){
    if (get_table().has_key(id)) return get_table()[id].fcn;
    throw uhd::key_error("Cannot find a conversion routine for " + id.to_pp_string());
}

/***********************************************************************
 * Mappings for item markup to byte size for all items we can
 **********************************************************************/
typedef uhd::dict<std::string, size_t> item_size_type;
UHD_SINGLETON_FCN(item_size_type, get_item_size_table);

void register_bytes_per_item(
    const std::string &markup, const size_t size
){
    get_item_size_table()[markup] = size;
}

size_t convert::get_bytes_per_item(const std::string &markup){
    if (get_item_size_table().has_key(markup)) return get_item_size_table()[markup];
    throw uhd::key_error("Cannot find an item size " + markup);
}

UHD_STATIC_BLOCK(convert_register_item_sizes){
    //register standard complex types
    get_item_size_table()["fc64"] = sizeof(std::complex<double>);
    get_item_size_table()["fc32"] = sizeof(std::complex<float>);
    get_item_size_table()["sc32"] = sizeof(std::complex<boost::int32_t>);
    get_item_size_table()["sc16"] = sizeof(std::complex<boost::int16_t>);
    get_item_size_table()["sc8"] = sizeof(std::complex<boost::int8_t>);

    //register standard real types
    get_item_size_table()["f64"] = sizeof(double);
    get_item_size_table()["f32"] = sizeof(float);
    get_item_size_table()["s32"] = sizeof(boost::int32_t);
    get_item_size_table()["s16"] = sizeof(boost::int16_t);
    get_item_size_table()["s8"] = sizeof(boost::int8_t);
}
