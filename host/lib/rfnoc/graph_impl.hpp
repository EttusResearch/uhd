//
// Copyright 2016 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_RFNOC_GRAPH_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_GRAPH_IMPL_HPP

#include <uhd/rfnoc/graph.hpp>
#include <uhd/device3.hpp>

namespace uhd { namespace rfnoc {

class graph_impl : public graph
{
public:
    /*!
     * \param name An optional name to describe this graph
     * \param device_ptr Weak pointer to the originating device3
     * \param msg_handler Pointer to the async message handler
     */
    graph_impl(
            const std::string &name,
            boost::weak_ptr<uhd::device3> device_ptr
            //async_msg_handler::sptr msg_handler
    );
    virtual ~graph_impl() {};

    /************************************************************************
     * Connection API
     ***********************************************************************/
    void connect(
            const block_id_t &src_block,
            size_t src_block_port,
            const block_id_t &dst_block,
            size_t dst_block_port,
            const size_t pkt_size = 0
    );

    void connect(
            const block_id_t &src_block,
            const block_id_t &dst_block
    );

    /************************************************************************
     * Utilities
     ***********************************************************************/
    std::string get_name() const { return _name; }


private:

    //! Optional: A string to describe this graph
    const std::string _name;

    //! Reference to the generating device object
    const boost::weak_ptr<uhd::device3> _device_ptr;

};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_GRAPH_IMPL_HPP */
// vim: sw=4 et:
