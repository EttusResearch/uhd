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

#include "usrp2_regs.hpp"
#include "usrp2_iface.hpp"
#include <uhd/utils/assert.hpp>
#include <uhd/types/dict.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp> //used for htonl and ntohl
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <stdexcept>
#include <algorithm>

using namespace uhd;
using namespace uhd::transport;

/*!
 * FIXME: large timeout, ethernet pause frames...
 *
 * Use a large timeout to work-around the fact that
 * flow-control may throttle outgoing control packets
 * due to its use of ethernet pause frames.
 *
 * This will be fixed when host-based flow control is implemented,
 * along with larger incoming send buffers using the on-board SRAM.
 */
static const size_t CONTROL_TIMEOUT_MS = 3000; //3 seconds

class usrp2_iface_impl : public usrp2_iface{
public:
/***********************************************************************
 * Structors
 **********************************************************************/
    usrp2_iface_impl(udp_simple::sptr ctrl_transport){
        _ctrl_transport = ctrl_transport;

         //check the fpga compatibility number
        const boost::uint32_t fpga_compat_num = this->peek32(U2_REG_COMPAT_NUM_RB);
        if (fpga_compat_num != USRP2_FPGA_COMPAT_NUM){
            throw std::runtime_error(str(boost::format(
                "Expected fpga compatibility number %d, but got %d:\n"
                "The fpga build is not compatible with the host code build."
            ) % int(USRP2_FPGA_COMPAT_NUM) % fpga_compat_num));
        }
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

    pair64 peek64(boost::uint32_t addrlo, boost::uint32_t addrhi){
        //setup the out data
        usrp2_ctrl_data_t out_data;
        out_data.id = htonl(USRP2_CTRL_ID_PEEK_AT_THIS_REGISTER_FOR_ME_BRO);
        out_data.data.poke_args.addr = htonl(addrlo);
        out_data.data.poke_args.addrhi = htonl(addrhi);
        out_data.data.poke_args.num_bytes = sizeof(boost::uint64_t);

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data);
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_WOAH_I_DEFINITELY_PEEKED_IT_DUDE);
        return pair64(ntohl(in_data.data.poke_args.data), ntohl(in_data.data.poke_args.datahi));
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
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_OMG_TRANSACTED_SPI_DUDE);

        return ntohl(in_data.data.spi_args.data);
    }

/***********************************************************************
 * I2C
 **********************************************************************/
    void write_i2c(boost::uint8_t addr, const byte_vector_t &buf){
        //setup the out data
        usrp2_ctrl_data_t out_data;
        out_data.id = htonl(USRP2_CTRL_ID_WRITE_THESE_I2C_VALUES_BRO);
        out_data.data.i2c_args.addr = addr;
        out_data.data.i2c_args.bytes = buf.size();

        //limitation of i2c transaction size
        UHD_ASSERT_THROW(buf.size() <= sizeof(out_data.data.i2c_args.data));

        //copy in the data
        std::copy(buf.begin(), buf.end(), out_data.data.i2c_args.data);

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data);
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_COOL_IM_DONE_I2C_WRITE_DUDE);
    }

    byte_vector_t read_i2c(boost::uint8_t addr, size_t num_bytes){
        //setup the out data
        usrp2_ctrl_data_t out_data;
        out_data.id = htonl(USRP2_CTRL_ID_DO_AN_I2C_READ_FOR_ME_BRO);
        out_data.data.i2c_args.addr = addr;
        out_data.data.i2c_args.bytes = num_bytes;

        //limitation of i2c transaction size
        UHD_ASSERT_THROW(num_bytes <= sizeof(out_data.data.i2c_args.data));

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data);
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_HERES_THE_I2C_DATA_DUDE);
        UHD_ASSERT_THROW(in_data.data.i2c_args.addr = num_bytes);

        //copy out the data
        byte_vector_t result(num_bytes);
        std::copy(in_data.data.i2c_args.data, in_data.data.i2c_args.data + num_bytes, result.begin());
        return result;
    }

/***********************************************************************
 * Send/Recv over control
 **********************************************************************/
    usrp2_ctrl_data_t ctrl_send_and_recv(const usrp2_ctrl_data_t &out_data){
        boost::mutex::scoped_lock lock(_ctrl_mutex);

        //fill in the seq number and send
        usrp2_ctrl_data_t out_copy = out_data;
        out_copy.proto_ver = htonl(USRP2_FW_COMPAT_NUM);
        out_copy.seq = htonl(++_ctrl_seq_num);
        _ctrl_transport->send(boost::asio::buffer(&out_copy, sizeof(usrp2_ctrl_data_t)));

        //loop until we get the packet or timeout
        boost::uint8_t usrp2_ctrl_data_in_mem[udp_simple::mtu]; //allocate max bytes for recv
        const usrp2_ctrl_data_t *ctrl_data_in = reinterpret_cast<const usrp2_ctrl_data_t *>(usrp2_ctrl_data_in_mem);
        while(true){
            size_t len = _ctrl_transport->recv(boost::asio::buffer(usrp2_ctrl_data_in_mem), CONTROL_TIMEOUT_MS);
            if(len >= sizeof(boost::uint32_t) and ntohl(ctrl_data_in->proto_ver) != USRP2_FW_COMPAT_NUM){
                throw std::runtime_error(str(boost::format(
                    "Expected protocol compatibility number %d, but got %d:\n"
                    "The firmware build is not compatible with the host code build."
                ) % int(USRP2_FW_COMPAT_NUM) % ntohl(ctrl_data_in->proto_ver)));
            }
            if (len >= sizeof(usrp2_ctrl_data_t) and ntohl(ctrl_data_in->seq) == _ctrl_seq_num){
                return *ctrl_data_in;
            }
            if (len == 0) break; //timeout
            //didnt get seq or bad packet, continue looking...
        }
        throw std::runtime_error("usrp2 no control response");
    }

private:
    //this lovely lady makes it all possible
    udp_simple::sptr _ctrl_transport;

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
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_OMG_POKED_REGISTER_SO_BAD_DUDE);
    }

    template <class T> T peek(boost::uint32_t addr){
        //setup the out data
        usrp2_ctrl_data_t out_data;
        out_data.id = htonl(USRP2_CTRL_ID_PEEK_AT_THIS_REGISTER_FOR_ME_BRO);
        out_data.data.poke_args.addr = htonl(addr);
        out_data.data.poke_args.num_bytes = sizeof(T);

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data);
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_WOAH_I_DEFINITELY_PEEKED_IT_DUDE);
        return T(ntohl(in_data.data.poke_args.data));
    }

};

/***********************************************************************
 * Public make function for usrp2 interface
 **********************************************************************/
usrp2_iface::sptr usrp2_iface::make(udp_simple::sptr ctrl_transport){
    return usrp2_iface::sptr(new usrp2_iface_impl(ctrl_transport));
}
