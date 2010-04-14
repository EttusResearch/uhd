//
// Copyright 2010 Ettus Research LLC
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

#include "usrp2_iface.hpp"
#include <uhd/utils/assert.hpp>
#include <uhd/types/dict.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp> //used for htonl and ntohl
#include <boost/assign/list_of.hpp>
#include <stdexcept>

using namespace uhd;
using namespace uhd::usrp;

class usrp2_iface_impl : public usrp2_iface{
public:
/***********************************************************************
 * Structors
 **********************************************************************/
    usrp2_iface_impl(transport::udp_simple::sptr ctrl_transport){
        _ctrl_transport = ctrl_transport;
    }

    ~usrp2_iface_impl(void){
        /* NOP */
    }

/***********************************************************************
 * Peek and Poke
 **********************************************************************/
    void poke32(boost::uint32_t addr, boost::uint32_t data){
        return this->poke<boost::uint32_t>(addr, data);
    }

    boost::uint32_t peek32(boost::uint32_t addr){
        return this->peek<boost::uint32_t>(addr);
    }

    void poke16(boost::uint32_t addr, boost::uint16_t data){
        return this->poke<boost::uint16_t>(addr, data);
    }

    boost::uint16_t peek16(boost::uint32_t addr){
        return this->peek<boost::uint16_t>(addr);
    }

/***********************************************************************
 * SPI
 **********************************************************************/
    boost::uint32_t transact_spi(
        int which_slave,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits,
        bool readback
    ){
        static const uhd::dict<spi_config_t::edge_t, int> spi_edge_to_otw = boost::assign::map_list_of
            (spi_config_t::EDGE_RISE, USRP2_CLK_EDGE_RISE)
            (spi_config_t::EDGE_FALL, USRP2_CLK_EDGE_FALL)
        ;

        //setup the out data
        usrp2_ctrl_data_t out_data;
        out_data.id = htonl(USRP2_CTRL_ID_TRANSACT_ME_SOME_SPI_BRO);
        out_data.data.spi_args.dev = which_slave;
        out_data.data.spi_args.miso_edge = spi_edge_to_otw[config.miso_edge];
        out_data.data.spi_args.mosi_edge = spi_edge_to_otw[config.mosi_edge];
        out_data.data.spi_args.readback = (readback)? 1 : 0;
        out_data.data.spi_args.num_bits = num_bits;
        out_data.data.spi_args.data = htonl(data);

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data);
        ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_OMG_TRANSACTED_SPI_DUDE);

        return ntohl(out_data.data.spi_args.data);
    }

/***********************************************************************
 * Send/Recv over control
 **********************************************************************/
    usrp2_ctrl_data_t ctrl_send_and_recv(const usrp2_ctrl_data_t &out_data){
        boost::mutex::scoped_lock lock(_ctrl_mutex);

        //fill in the seq number and send
        usrp2_ctrl_data_t out_copy = out_data;
        out_copy.seq = htonl(++_ctrl_seq_num);
        _ctrl_transport->send(boost::asio::buffer(&out_copy, sizeof(usrp2_ctrl_data_t)));

        //loop until we get the packet or timeout
        while(true){
            usrp2_ctrl_data_t in_data;
            size_t len = _ctrl_transport->recv(boost::asio::buffer(&in_data, sizeof(in_data)));
            if (len >= sizeof(usrp2_ctrl_data_t) and ntohl(in_data.seq) == _ctrl_seq_num){
                return in_data;
            }
            if (len == 0) break; //timeout
            //didnt get seq or bad packet, continue looking...
        }
        throw std::runtime_error("usrp2 no control response");
    }

/***********************************************************************
 * Master Clock! Ahhhhh
 **********************************************************************/
    double get_master_clock_freq(void){
        return 100e6;
    }

private:
    //this lovely lady makes it all possible
    transport::udp_simple::sptr _ctrl_transport;

    //used in send/recv
    boost::mutex _ctrl_mutex;
    boost::uint32_t _ctrl_seq_num;

/***********************************************************************
 * Private Templated Peek and Poke
 **********************************************************************/
    template <class T> void poke(boost::uint32_t addr, T data){
        //setup the out data
        usrp2_ctrl_data_t out_data;
        out_data.id = htonl(USRP2_CTRL_ID_POKE_THIS_REGISTER_FOR_ME_BRO);
        out_data.data.poke_args.addr = htonl(addr);
        out_data.data.poke_args.data = htonl(boost::uint32_t(data));
        out_data.data.poke_args.num_bytes = sizeof(T);

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data);
        ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_OMG_POKED_REGISTER_SO_BAD_DUDE);
    }

    template <class T> T peek(boost::uint32_t addr){
        //setup the out data
        usrp2_ctrl_data_t out_data;
        out_data.id = htonl(USRP2_CTRL_ID_PEEK_AT_THIS_REGISTER_FOR_ME_BRO);
        out_data.data.poke_args.addr = htonl(addr);
        out_data.data.poke_args.num_bytes = sizeof(T);

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data);
        ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_WOAH_I_DEFINITELY_PEEKED_IT_DUDE);
        return T(ntohl(out_data.data.poke_args.data));
    }

};

/***********************************************************************
 * Public make function for usrp2 interface
 **********************************************************************/
usrp2_iface::sptr usrp2_iface::make(transport::udp_simple::sptr ctrl_transport){
    return usrp2_iface::sptr(new usrp2_iface_impl(ctrl_transport));
}
