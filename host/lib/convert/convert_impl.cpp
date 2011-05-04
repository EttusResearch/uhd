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
#include <uhd/exception.hpp>

using namespace uhd;

#include "convert_pred.hpp"

/***********************************************************************
 * Define types for the function tables
 **********************************************************************/
struct fcn_table_entry_type{
    convert::priority_type prio;
    convert::function_type fcn;
    fcn_table_entry_type(void)
    : prio(convert::PRIORITY_EMPTY), fcn(NULL){
        /* NOP */
    }
};
typedef std::vector<fcn_table_entry_type> fcn_table_type;

/***********************************************************************
 * Setup the table registry
 **********************************************************************/
UHD_SINGLETON_FCN(fcn_table_type, get_cpu_to_otw_table);
UHD_SINGLETON_FCN(fcn_table_type, get_otw_to_cpu_table);

fcn_table_type &get_table(dir_type dir){
    switch(dir){
    case DIR_OTW_TO_CPU: return get_otw_to_cpu_table();
    case DIR_CPU_TO_OTW: return get_cpu_to_otw_table();
    }
    UHD_THROW_INVALID_CODE_PATH();
}

/***********************************************************************
 * The registry functions
 **********************************************************************/
void uhd::convert::register_converter(
    const std::string &markup,
    function_type fcn,
    priority_type prio
){
    //extract the predicate and direction from the markup
    dir_type dir;
    pred_type pred = make_pred(markup, dir);

    //get a reference to the function table
    fcn_table_type &table = get_table(dir);

    //resize the table so that its at least pred+1
    if (table.size() <= pred) table.resize(pred+1);

    //register the function if higher priority
    if (table[pred].prio < prio){
        table[pred].fcn = fcn;
        table[pred].prio = prio;
    }

    //----------------------------------------------------------------//
    UHD_LOGV(always) << "register_converter: " << markup << std::endl
        << "    prio: " << prio << std::endl
        << "    pred: " << pred << std::endl
        << "    dir: " << dir << std::endl
        << std::endl
    ;
    //----------------------------------------------------------------//
}

/***********************************************************************
 * The converter functions
 **********************************************************************/
const convert::function_type &convert::get_converter_cpu_to_otw(
    const io_type_t &io_type,
    const otw_type_t &otw_type,
    size_t num_input_buffs,
    size_t num_output_buffs
){
    pred_type pred = make_pred(io_type, otw_type, num_input_buffs, num_output_buffs);
    return get_cpu_to_otw_table().at(pred).fcn;
}

const convert::function_type &convert::get_converter_otw_to_cpu(
    const io_type_t &io_type,
    const otw_type_t &otw_type,
    size_t num_input_buffs,
    size_t num_output_buffs
){
    pred_type pred = make_pred(io_type, otw_type, num_input_buffs, num_output_buffs);
    return get_otw_to_cpu_table().at(pred).fcn;
}
