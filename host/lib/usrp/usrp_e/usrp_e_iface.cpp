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

#include "usrp_e_iface.hpp"
#include <sys/ioctl.h> //ioctl
#include <linux/usrp_e.h> //ioctl structures and constants
#include <boost/format.hpp>
#include <stdexcept>

class usrp_e_iface_impl : public usrp_e_iface{
public:

    /*******************************************************************
     * Structors
     ******************************************************************/
    usrp_e_iface_impl(int node_fd){
        _node_fd = node_fd;
    }

    ~usrp_e_iface_impl(void){
        /* NOP */
    }

    /*******************************************************************
     * IOCTL: provides the communication base for all other calls
     ******************************************************************/
    void ioctl(int request, void *mem){
        if (::ioctl(_node_fd, request, mem) < 0){
            throw std::runtime_error(str(
                boost::format("ioctl failed with request %d") % request
            ));
        }
    }

    /*******************************************************************
     * Peek and Poke
     ******************************************************************/
    void poke32(boost::uint32_t addr, boost::uint32_t value){
        //load the data struct
        usrp_e_ctl32 data;
        data.offset = addr;
        data.count = 1;
        data.buf[0] = value;

        //call the ioctl
        this->ioctl(USRP_E_WRITE_CTL32, &data);
    }

    void poke16(boost::uint32_t addr, boost::uint16_t value){
        //load the data struct
        usrp_e_ctl16 data;
        data.offset = addr;
        data.count = 1;
        data.buf[0] = value;

        //call the ioctl
        this->ioctl(USRP_E_WRITE_CTL16, &data);
    }

    boost::uint32_t peek32(boost::uint32_t addr){
        //load the data struct
        usrp_e_ctl32 data;
        data.offset = addr;
        data.count = 1;

        //call the ioctl
        this->ioctl(USRP_E_READ_CTL32, &data);

        return data.buf[0];
    }

    boost::uint16_t peek16(boost::uint32_t addr){
        //load the data struct
        usrp_e_ctl16 data;
        data.offset = addr;
        data.count = 1;

        //call the ioctl
        this->ioctl(USRP_E_READ_CTL16, &data);

        return data.buf[0];
    }

    /*******************************************************************
     * SPI
     ******************************************************************/
    boost::uint32_t transact_spi(
        int which_slave,
        const uhd::spi_config_t &config,
        boost::uint32_t bits,
        size_t num_bits,
        bool readback
    ){
        //load data struct
        usrp_e_spi data;
        data.readback = (readback)? UE_SPI_TXRX : UE_SPI_TXONLY;
        data.slave = which_slave;
        data.length = num_bits;
        data.data = bits;

        //load the flags
        data.flags = 0;
        data.flags |= (config.miso_edge == uhd::spi_config_t::EDGE_RISE)? UE_SPI_LATCH_RISE : UE_SPI_LATCH_FALL;
        data.flags |= (config.mosi_edge == uhd::spi_config_t::EDGE_RISE)? UE_SPI_PUSH_FALL  : UE_SPI_PUSH_RISE;

        //call the spi ioctl
        this->ioctl(USRP_E_SPI, &data);

        //unload the data
        return data.data;
    }

private: int _node_fd;
};

/***********************************************************************
 * Public Make Function
 **********************************************************************/
usrp_e_iface::sptr usrp_e_iface::make(int node_fd){
    return sptr(new usrp_e_iface_impl(node_fd));
}
