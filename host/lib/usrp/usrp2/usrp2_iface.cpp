//
// Copyright 2010-2011 Ettus Research LLC
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
#include "fw_common.h"
#include "usrp2_iface.hpp"
#include <uhd/exception.hpp>
#include <uhd/types/dict.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp> //used for htonl and ntohl
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <algorithm>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

static const double CTRL_RECV_TIMEOUT = 1.0;

static const boost::uint32_t MIN_PROTO_COMPAT_SPI = 7;
static const boost::uint32_t MIN_PROTO_COMPAT_I2C = 7;
// The register compat number must reflect the protocol compatibility
// and the compatibility of the register mapping (more likely to change).
static const boost::uint32_t MIN_PROTO_COMPAT_REG = USRP2_FW_COMPAT_NUM;
static const boost::uint32_t MIN_PROTO_COMPAT_UART = 7;

// Map for virtual firmware regs (not very big so we can keep it here for now)
#define U2_FW_REG_LOCK_TIME 0

class usrp2_iface_impl : public usrp2_iface{
public:
/***********************************************************************
 * Structors
 **********************************************************************/
    usrp2_iface_impl(udp_simple::sptr ctrl_transport):
        _ctrl_transport(ctrl_transport),
        _ctrl_seq_num(0),
        _protocol_compat(0) //initialized below...
    {
        //Obtain the firmware's compat number.
        //Save the response compat number for communication.
        //TODO can choose to reject certain older compat numbers
        usrp2_ctrl_data_t ctrl_data;
        ctrl_data.id = htonl(USRP2_CTRL_ID_WAZZUP_BRO);
        ctrl_data = ctrl_send_and_recv(ctrl_data, 0, ~0);
        if (ntohl(ctrl_data.id) != USRP2_CTRL_ID_WAZZUP_DUDE)
            throw uhd::runtime_error("firmware not responding");
        _protocol_compat = ntohl(ctrl_data.proto_ver);

        mb_eeprom = mboard_eeprom_t(*this, mboard_eeprom_t::MAP_N100);
        regs = usrp2_get_regs(); //reg map identical across all boards
    }

    ~usrp2_iface_impl(void){
        this->lock_device(false);
    }

/***********************************************************************
 * Device locking
 **********************************************************************/

    void lock_device(bool lock){
        if (lock){
            boost::barrier spawn_barrier(2);
            _lock_thread_group.create_thread(boost::bind(&usrp2_iface_impl::lock_loop, this, boost::ref(spawn_barrier)));
            spawn_barrier.wait();
        }
        else{
            _lock_thread_group.interrupt_all();
            _lock_thread_group.join_all();
        }
    }

    bool is_device_locked(void){
        boost::uint32_t lock_secs = this->get_reg<boost::uint32_t, USRP2_REG_ACTION_FW_PEEK32>(U2_FW_REG_LOCK_TIME);
        boost::uint32_t curr_secs = this->peek32(this->regs.time64_secs_rb_imm);
        return (curr_secs - lock_secs < 3); //if the difference is larger, assume not locked anymore
    }

    void lock_loop(boost::barrier &spawn_barrier){
        spawn_barrier.wait();

        try{
            while(true){
                //re-lock in loop
                boost::uint32_t curr_secs = this->peek32(this->regs.time64_secs_rb_imm);
                this->get_reg<boost::uint32_t, USRP2_REG_ACTION_FW_POKE32>(U2_FW_REG_LOCK_TIME, curr_secs);
                //sleep for a bit
                boost::this_thread::sleep(boost::posix_time::milliseconds(1500));
            }
        } catch(const boost::thread_interrupted &){}

        //unlock on exit
        this->get_reg<boost::uint32_t, USRP2_REG_ACTION_FW_POKE32>(U2_FW_REG_LOCK_TIME, 0);
    }

/***********************************************************************
 * Peek and Poke
 **********************************************************************/
    void poke32(boost::uint32_t addr, boost::uint32_t data){
        this->get_reg<boost::uint32_t, USRP2_REG_ACTION_FPGA_POKE32>(addr, data);
    }

    boost::uint32_t peek32(boost::uint32_t addr){
        return this->get_reg<boost::uint32_t, USRP2_REG_ACTION_FPGA_PEEK32>(addr);
    }

    void poke16(boost::uint32_t addr, boost::uint16_t data){
        this->get_reg<boost::uint16_t, USRP2_REG_ACTION_FPGA_POKE16>(addr, data);
    }

    boost::uint16_t peek16(boost::uint32_t addr){
        return this->get_reg<boost::uint16_t, USRP2_REG_ACTION_FPGA_PEEK16>(addr);
    }

    template <class T, usrp2_reg_action_t action>
    T get_reg(boost::uint32_t addr, T data = 0){
        //setup the out data
        usrp2_ctrl_data_t out_data;
        out_data.id = htonl(USRP2_CTRL_ID_GET_THIS_REGISTER_FOR_ME_BRO);
        out_data.data.reg_args.addr = htonl(addr);
        out_data.data.reg_args.data = htonl(boost::uint32_t(data));
        out_data.data.reg_args.action = action;

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data, MIN_PROTO_COMPAT_REG);
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_OMG_GOT_REGISTER_SO_BAD_DUDE);
        return T(ntohl(in_data.data.reg_args.data));
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
        out_data.data.spi_args.dev = htonl(which_slave);
        out_data.data.spi_args.miso_edge = spi_edge_to_otw[config.miso_edge];
        out_data.data.spi_args.mosi_edge = spi_edge_to_otw[config.mosi_edge];
        out_data.data.spi_args.readback = (readback)? 1 : 0;
        out_data.data.spi_args.num_bits = num_bits;
        out_data.data.spi_args.data = htonl(data);

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data, MIN_PROTO_COMPAT_SPI);
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
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data, MIN_PROTO_COMPAT_I2C);
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
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data, MIN_PROTO_COMPAT_I2C);
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_HERES_THE_I2C_DATA_DUDE);
        UHD_ASSERT_THROW(in_data.data.i2c_args.addr = num_bytes);

        //copy out the data
        byte_vector_t result(num_bytes);
        std::copy(in_data.data.i2c_args.data, in_data.data.i2c_args.data + num_bytes, result.begin());
        return result;
    }

/***********************************************************************
 * UART
 **********************************************************************/
    void write_uart(boost::uint8_t dev, const std::string &buf){
      //first tokenize the string into 20-byte substrings
      boost::offset_separator f(20, 1, true, true);
      boost::tokenizer<boost::offset_separator> tok(buf, f);
      std::vector<std::string> queue(tok.begin(), tok.end());

      BOOST_FOREACH(std::string item, queue) {
        //setup the out data
        usrp2_ctrl_data_t out_data;
        out_data.id = htonl(USRP2_CTRL_ID_HEY_WRITE_THIS_UART_FOR_ME_BRO);
        out_data.data.uart_args.dev = dev;
        out_data.data.uart_args.bytes = item.size();

        //limitation of uart transaction size
        UHD_ASSERT_THROW(item.size() <= sizeof(out_data.data.uart_args.data));

        //copy in the data
        std::copy(item.begin(), item.end(), out_data.data.uart_args.data);

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data, MIN_PROTO_COMPAT_UART);
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_MAN_I_TOTALLY_WROTE_THAT_UART_DUDE);
      }
    }

    std::string read_uart(boost::uint8_t dev){
      int readlen = 20;
      std::string result;
      while(readlen == 20) { //while we keep receiving full packets
        //setup the out data
        usrp2_ctrl_data_t out_data;
        out_data.id = htonl(USRP2_CTRL_ID_SO_LIKE_CAN_YOU_READ_THIS_UART_BRO);
        out_data.data.uart_args.dev = dev;
        out_data.data.uart_args.bytes = 20;

        //limitation of uart transaction size
        //UHD_ASSERT_THROW(num_bytes <= sizeof(out_data.data.uart_args.data));

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data, MIN_PROTO_COMPAT_UART);
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_I_HELLA_READ_THAT_UART_DUDE);
        readlen = in_data.data.uart_args.bytes;

        //copy out the data
        result += std::string((const char *)in_data.data.uart_args.data, (size_t)readlen);
      }
      return result;
    }
    
    gps_send_fn_t get_gps_write_fn(void) {
        return boost::bind(&usrp2_iface_impl::write_uart, this, 2, _1); //2 is the GPS UART port on USRP2
    }
    
    gps_recv_fn_t get_gps_read_fn(void) {
        return boost::bind(&usrp2_iface_impl::read_uart, this, 2); //2 is the GPS UART port on USRP2
    }

/***********************************************************************
 * Send/Recv over control
 **********************************************************************/
    usrp2_ctrl_data_t ctrl_send_and_recv(
        const usrp2_ctrl_data_t &out_data,
        boost::uint32_t lo = USRP2_FW_COMPAT_NUM,
        boost::uint32_t hi = USRP2_FW_COMPAT_NUM
    ){
        boost::mutex::scoped_lock lock(_ctrl_mutex);

        //fill in the seq number and send
        usrp2_ctrl_data_t out_copy = out_data;
        out_copy.proto_ver = htonl(_protocol_compat);
        out_copy.seq = htonl(++_ctrl_seq_num);
        _ctrl_transport->send(boost::asio::buffer(&out_copy, sizeof(usrp2_ctrl_data_t)));

        //loop until we get the packet or timeout
        boost::uint8_t usrp2_ctrl_data_in_mem[udp_simple::mtu]; //allocate max bytes for recv
        const usrp2_ctrl_data_t *ctrl_data_in = reinterpret_cast<const usrp2_ctrl_data_t *>(usrp2_ctrl_data_in_mem);
        while(true){
            size_t len = _ctrl_transport->recv(boost::asio::buffer(usrp2_ctrl_data_in_mem), CTRL_RECV_TIMEOUT);
            boost::uint32_t compat = ntohl(ctrl_data_in->proto_ver);
            if(len >= sizeof(boost::uint32_t) and (hi < compat or lo > compat)){
                throw uhd::runtime_error(str(boost::format(
                    "\nPlease update the firmware and FPGA images for your device.\n"
                    "See the application notes for USRP2/N-Series for instructions.\n"
                    "Expected protocol compatibility number %s, but got %d:\n"
                    "The firmware build is not compatible with the host code build."
                ) % ((lo == hi)? (boost::format("%d") % hi) : (boost::format("[%d to %d]") % lo % hi)) % compat));
            }
            if (len >= sizeof(usrp2_ctrl_data_t) and ntohl(ctrl_data_in->seq) == _ctrl_seq_num){
                return *ctrl_data_in;
            }
            if (len == 0) break; //timeout
            //didnt get seq or bad packet, continue looking...
        }
        throw uhd::runtime_error("no control response");
    }

    rev_type get_rev(void){
        switch (boost::lexical_cast<boost::uint16_t>(mb_eeprom["rev"])){
        case 0x0300:
        case 0x0301: return USRP2_REV3;
        case 0x0400: return USRP2_REV4;
        case 0x0A00: return USRP_N200;
        case 0x0A01: return USRP_N210;
        }
        return USRP_NXXX; //unknown type
    }

    const std::string get_cname(void){
        switch(this->get_rev()){
        case USRP2_REV3: return "USRP2-REV3";
        case USRP2_REV4: return "USRP2-REV4";
        case USRP_N200: return "USRP-N200";
        case USRP_N210: return "USRP-N210";
        case USRP_NXXX: return "USRP-N???";
        }
        UHD_THROW_INVALID_CODE_PATH();
    }

private:
    //this lovely lady makes it all possible
    udp_simple::sptr _ctrl_transport;

    //used in send/recv
    boost::mutex _ctrl_mutex;
    boost::uint32_t _ctrl_seq_num;
    boost::uint32_t _protocol_compat;

    //lock thread stuff
    boost::thread_group _lock_thread_group;
};

/***********************************************************************
 * Public make function for usrp2 interface
 **********************************************************************/
usrp2_iface::sptr usrp2_iface::make(udp_simple::sptr ctrl_transport){
    return usrp2_iface::sptr(new usrp2_iface_impl(ctrl_transport));
}

