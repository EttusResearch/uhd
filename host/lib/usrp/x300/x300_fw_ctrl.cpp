//
// Copyright 2013 Ettus Research LLC
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

#include <uhd/types/wb_iface.hpp>
#include "x300_fw_common.h"
#include <uhd/transport/udp_simple.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <boost/thread/mutex.hpp>
#include <uhd/transport/nirio/status.h>
#include <uhd/transport/nirio/niriok_proxy.h>
#include "x300_regs.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

using namespace uhd;
using namespace uhd::niusrprio;

class x300_ctrl_iface : public wb_iface
{
public:
    enum {num_retries = 3};

    void flush(void)
    {
        boost::mutex::scoped_lock lock(reg_access);
        __flush();
    }

    void poke32(const wb_addr_type addr, const boost::uint32_t data)
    {
        for (size_t i = 1; i <= num_retries; i++)
        {
            boost::mutex::scoped_lock lock(reg_access);
            try
            {
                return this->__poke32(addr, data);
            }
            catch(const std::exception &ex)
            {
                const std::string error_msg = str(boost::format(
                    "x300 fw communication failure #%u\n%s") % i % ex.what());
                UHD_MSG(error) << error_msg << std::endl;
                if (i == num_retries) throw uhd::io_error(error_msg);
            }
        }
    }

    boost::uint32_t peek32(const wb_addr_type addr)
    {
        for (size_t i = 1; i <= num_retries; i++)
        {
            boost::mutex::scoped_lock lock(reg_access);
            try
            {
                boost::uint32_t data = this->__peek32(addr);
                return data;
            }
            catch(const std::exception &ex)
            {
                const std::string error_msg = str(boost::format(
                    "x300 fw communication failure #%u\n%s") % i % ex.what());
                UHD_MSG(error) << error_msg << std::endl;
                if (i == num_retries) throw uhd::io_error(error_msg);
            }
        }
        return 0;
    }

protected:
    virtual void __poke32(const wb_addr_type addr, const boost::uint32_t data) = 0;
    virtual boost::uint32_t __peek32(const wb_addr_type addr) = 0;
    virtual void __flush() = 0;

    boost::mutex reg_access;
};


//-----------------------------------------------------
// Ethernet impl
//-----------------------------------------------------
class x300_ctrl_iface_enet : public x300_ctrl_iface
{
public:
    x300_ctrl_iface_enet(uhd::transport::udp_simple::sptr udp):
        udp(udp), seq(0)
    {
        try
        {
            this->peek32(0);
        }
        catch(...){}
    }

protected:
    virtual void __poke32(const wb_addr_type addr, const boost::uint32_t data)
    {
        //load request struct
        x300_fw_comms_t request = x300_fw_comms_t();
        request.flags = uhd::htonx<boost::uint32_t>(X300_FW_COMMS_FLAGS_ACK | X300_FW_COMMS_FLAGS_POKE32);
        request.sequence = uhd::htonx<boost::uint32_t>(seq++);
        request.addr = uhd::htonx(addr);
        request.data = uhd::htonx(data);

        //send request
        __flush();
        udp->send(boost::asio::buffer(&request, sizeof(request)));

        //recv reply
        x300_fw_comms_t reply = x300_fw_comms_t();
        const size_t nbytes = udp->recv(boost::asio::buffer(&reply, sizeof(reply)), 1.0);
        if (nbytes == 0) throw uhd::io_error("x300 fw poke32 - reply timed out");

        //sanity checks
        const size_t flags = uhd::ntohx<boost::uint32_t>(reply.flags);
        UHD_ASSERT_THROW(nbytes == sizeof(reply));
        UHD_ASSERT_THROW(not (flags & X300_FW_COMMS_FLAGS_ERROR));
        UHD_ASSERT_THROW(flags & X300_FW_COMMS_FLAGS_POKE32);
        UHD_ASSERT_THROW(flags & X300_FW_COMMS_FLAGS_ACK);
        UHD_ASSERT_THROW(reply.sequence == request.sequence);
        UHD_ASSERT_THROW(reply.addr == request.addr);
        UHD_ASSERT_THROW(reply.data == request.data);
    }

    virtual boost::uint32_t __peek32(const wb_addr_type addr)
    {
        //load request struct
        x300_fw_comms_t request = x300_fw_comms_t();
        request.flags = uhd::htonx<boost::uint32_t>(X300_FW_COMMS_FLAGS_ACK | X300_FW_COMMS_FLAGS_PEEK32);
        request.sequence = uhd::htonx<boost::uint32_t>(seq++);
        request.addr = uhd::htonx(addr);
        request.data = 0;

        //send request
        __flush();
        udp->send(boost::asio::buffer(&request, sizeof(request)));

        //recv reply
        x300_fw_comms_t reply = x300_fw_comms_t();
        const size_t nbytes = udp->recv(boost::asio::buffer(&reply, sizeof(reply)), 1.0);
        if (nbytes == 0) throw uhd::io_error("x300 fw peek32 - reply timed out");

        //sanity checks
        const size_t flags = uhd::ntohx<boost::uint32_t>(reply.flags);
        UHD_ASSERT_THROW(nbytes == sizeof(reply));
        UHD_ASSERT_THROW(not (flags & X300_FW_COMMS_FLAGS_ERROR));
        UHD_ASSERT_THROW(flags & X300_FW_COMMS_FLAGS_PEEK32);
        UHD_ASSERT_THROW(flags & X300_FW_COMMS_FLAGS_ACK);
        UHD_ASSERT_THROW(reply.sequence == request.sequence);
        UHD_ASSERT_THROW(reply.addr == request.addr);

        //return result!
        return uhd::ntohx<boost::uint32_t>(reply.data);
    }

    virtual void __flush(void)
    {
        char buff[X300_FW_COMMS_MTU] = {};
        while (udp->recv(boost::asio::buffer(buff), 0.0)){} //flush
    }

private:
    uhd::transport::udp_simple::sptr udp;
    size_t seq;
};


//-----------------------------------------------------
// PCIe impl
//-----------------------------------------------------
class x300_ctrl_iface_pcie : public x300_ctrl_iface
{
public:
    x300_ctrl_iface_pcie(niriok_proxy::sptr drv_proxy):
        _drv_proxy(drv_proxy)
    {
        nirio_status status = 0;
        nirio_status_chain(_drv_proxy->set_attribute(RIO_ADDRESS_SPACE, BUS_INTERFACE), status);

        //Verify that the Ettus FPGA loaded in the device. This may not be true if the
        //user is switching to UHD after using LabVIEW FPGA.
        boost::uint32_t pcie_fpga_signature = 0;
        _drv_proxy->peek(FPGA_PCIE_SIG_REG, pcie_fpga_signature);
        if (pcie_fpga_signature != FPGA_X3xx_SIG_VALUE)
            throw uhd::io_error("cannot create x300_ctrl_iface_pcie. incorrect/no fpga image");

        //Also, poll on the ZPU_STATUS bit to ensure all the state machines in the FPGA are
        //ready to accept register transaction requests.
        boost::uint32_t reg_data = 0xffffffff;
        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration elapsed;

        do {
            boost::this_thread::sleep(boost::posix_time::microsec(500)); //Avoid flooding the bus
            elapsed = boost::posix_time::microsec_clock::local_time() - start_time;
            nirio_status_chain(_drv_proxy->peek(PCIE_ZPU_STATUS_REG(0), reg_data), status);
        } while (
            nirio_status_not_fatal(status) &&
            (reg_data & PCIE_ZPU_STATUS_SUSPENDED) &&
            elapsed.total_milliseconds() < INIT_TIMEOUT_IN_MS);

        nirio_status_to_exception(status, "Could not initialize x300_ctrl_iface_pcie.");

        try
        {
            this->peek32(0);
        }
        catch(...){}
    }

protected:
    virtual void __poke32(const wb_addr_type addr, const boost::uint32_t data)
    {
        nirio_status status = 0;
        boost::uint32_t reg_data = 0xffffffff;
        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration elapsed;

        nirio_status_chain(_drv_proxy->poke(PCIE_ZPU_DATA_REG(addr), data), status);
        if (nirio_status_not_fatal(status)) {
            do {
                boost::this_thread::sleep(boost::posix_time::microsec(50)); //Avoid flooding the bus
                elapsed = boost::posix_time::microsec_clock::local_time() - start_time;
                nirio_status_chain(_drv_proxy->peek(PCIE_ZPU_STATUS_REG(addr), reg_data), status);
            } while (
                nirio_status_not_fatal(status) &&
                ((reg_data & (PCIE_ZPU_STATUS_BUSY | PCIE_ZPU_STATUS_SUSPENDED)) != 0) &&
                elapsed.total_milliseconds() < READ_TIMEOUT_IN_MS);
        }

        if (nirio_status_fatal(status))
            throw uhd::io_error("x300 fw poke32 - hardware IO error");
        if (elapsed.total_milliseconds() > READ_TIMEOUT_IN_MS)
            throw uhd::io_error("x300 fw poke32 - operation timed out");
    }

    virtual boost::uint32_t __peek32(const wb_addr_type addr)
    {
        nirio_status status = 0;
        boost::uint32_t reg_data = 0xffffffff;
        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration elapsed;

        nirio_status_chain(_drv_proxy->poke(PCIE_ZPU_READ_REG(addr), PCIE_ZPU_READ_START), status);
        if (nirio_status_not_fatal(status)) {
            do {
                boost::this_thread::sleep(boost::posix_time::microsec(50)); //Avoid flooding the bus
                elapsed = boost::posix_time::microsec_clock::local_time() - start_time;
                nirio_status_chain(_drv_proxy->peek(PCIE_ZPU_STATUS_REG(addr), reg_data), status);
            } while (
                nirio_status_not_fatal(status) &&
                ((reg_data & (PCIE_ZPU_STATUS_BUSY | PCIE_ZPU_STATUS_SUSPENDED)) != 0) &&
                elapsed.total_milliseconds() < READ_TIMEOUT_IN_MS);
        }
        nirio_status_chain(_drv_proxy->peek(PCIE_ZPU_DATA_REG(addr), reg_data), status);

        if (nirio_status_fatal(status))
            throw uhd::io_error("x300 fw peek32 - hardware IO error");
        if (elapsed.total_milliseconds() > READ_TIMEOUT_IN_MS)
            throw uhd::io_error("x300 fw peek32 - operation timed out");

        return reg_data;
    }

    virtual void __flush(void)
    {
        __peek32(0);
    }

private:
    niriok_proxy::sptr _drv_proxy;
    static const boost::uint32_t READ_TIMEOUT_IN_MS = 10;
    static const boost::uint32_t INIT_TIMEOUT_IN_MS = 5000;
};

wb_iface::sptr x300_make_ctrl_iface_enet(uhd::transport::udp_simple::sptr udp)
{
    return wb_iface::sptr(new x300_ctrl_iface_enet(udp));
}

wb_iface::sptr x300_make_ctrl_iface_pcie(niriok_proxy::sptr drv_proxy)
{
    return wb_iface::sptr(new x300_ctrl_iface_pcie(drv_proxy));
}
