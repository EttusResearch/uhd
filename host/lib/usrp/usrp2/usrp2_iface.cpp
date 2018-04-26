//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "usrp2_regs.hpp"
#include "usrp2_impl.hpp"
#include "fw_common.h"
#include "usrp2_iface.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/types/dict.hpp>
#include <boost/asio.hpp> //used for htonl and ntohl
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>
#include <uhd/utils/platform.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace fs = boost::filesystem;

static const double CTRL_RECV_TIMEOUT = 1.0;
static const size_t CTRL_RECV_RETRIES = 3;

//custom timeout error for retry logic to catch/retry
struct timeout_error : uhd::runtime_error
{
    timeout_error(const std::string &what):
        uhd::runtime_error(what)
    {
        //NOP
    }
};

static const uint32_t MIN_PROTO_COMPAT_SPI = 7;
static const uint32_t MIN_PROTO_COMPAT_I2C = 7;
// The register compat number must reflect the protocol compatibility
// and the compatibility of the register mapping (more likely to change).
static const uint32_t MIN_PROTO_COMPAT_REG = 10;
//static const uint32_t MIN_PROTO_COMPAT_UART = 7;

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
        usrp2_ctrl_data_t ctrl_data = usrp2_ctrl_data_t();
        ctrl_data.id = htonl(USRP2_CTRL_ID_WAZZUP_BRO);
        ctrl_data = ctrl_send_and_recv(ctrl_data, 0, ~0);
        if (ntohl(ctrl_data.id) != USRP2_CTRL_ID_WAZZUP_DUDE)
            throw uhd::runtime_error("firmware not responding");
        _protocol_compat = ntohl(ctrl_data.proto_ver);

        mb_eeprom = usrp2_impl::get_mb_eeprom(*this);
    }

    ~usrp2_iface_impl(void){UHD_SAFE_CALL(
        this->lock_device(false);
    )}

/***********************************************************************
 * Device locking
 **********************************************************************/

    void lock_device(bool lock){
        if (lock){
            this->pokefw(U2_FW_REG_LOCK_GPID, get_process_hash());
            _lock_task = task::make(boost::bind(&usrp2_iface_impl::lock_task, this));
        }
        else{
            _lock_task.reset(); //shutdown the task
            this->pokefw(U2_FW_REG_LOCK_TIME, 0); //unlock
        }
    }

    bool is_device_locked(void){
        //never assume lock with fpga image mismatch
        if ((this->peek32(U2_REG_COMPAT_NUM_RB) >> 16) != USRP2_FPGA_COMPAT_NUM) return false;

        uint32_t lock_time = this->peekfw(U2_FW_REG_LOCK_TIME);
        uint32_t lock_gpid = this->peekfw(U2_FW_REG_LOCK_GPID);

        //may not be the right tick rate, but this is ok for locking purposes
        const uint32_t lock_timeout_time = uint32_t(3*100e6);

        //if the difference is larger, assume not locked anymore
        if ((lock_time & 1) == 0) return false; //bit0 says unlocked
        const uint32_t time_diff = this->get_curr_time() - lock_time;
        if (time_diff >= lock_timeout_time) return false;

        //otherwise only lock if the device hash is different that ours
        return lock_gpid != get_process_hash();
    }

    void lock_task(void){
        //re-lock in task
        this->pokefw(U2_FW_REG_LOCK_TIME, this->get_curr_time());
        //sleep for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }

    uint32_t get_curr_time(void){
        return this->peek32(U2_REG_TIME64_LO_RB_IMM) | 1; //bit 1 says locked
    }

/***********************************************************************
 * Peek and Poke
 **********************************************************************/
    void poke32(const wb_addr_type addr, const uint32_t data){
        this->get_reg<uint32_t, USRP2_REG_ACTION_FPGA_POKE32>(addr, data);
    }

    uint32_t peek32(const wb_addr_type addr){
        return this->get_reg<uint32_t, USRP2_REG_ACTION_FPGA_PEEK32>(addr);
    }

    void poke16(const wb_addr_type addr, const uint16_t data){
        this->get_reg<uint16_t, USRP2_REG_ACTION_FPGA_POKE16>(addr, data);
    }

    uint16_t peek16(const wb_addr_type addr){
        return this->get_reg<uint16_t, USRP2_REG_ACTION_FPGA_PEEK16>(addr);
    }

    void pokefw(wb_addr_type addr, uint32_t data)
    {
        this->get_reg<uint32_t, USRP2_REG_ACTION_FW_POKE32>(addr, data);
    }

    uint32_t peekfw(wb_addr_type addr)
    {
        return this->get_reg<uint32_t, USRP2_REG_ACTION_FW_PEEK32>(addr);
    }

    template <class T, usrp2_reg_action_t action>
    T get_reg(wb_addr_type addr, T data = 0){
        //setup the out data
        usrp2_ctrl_data_t out_data = usrp2_ctrl_data_t();
        out_data.id = htonl(USRP2_CTRL_ID_GET_THIS_REGISTER_FOR_ME_BRO);
        out_data.data.reg_args.addr = htonl(addr);
        out_data.data.reg_args.data = htonl(uint32_t(data));
        out_data.data.reg_args.action = action;

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data, MIN_PROTO_COMPAT_REG);
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_OMG_GOT_REGISTER_SO_BAD_DUDE);
        return T(ntohl(in_data.data.reg_args.data));
    }

/***********************************************************************
 * SPI
 **********************************************************************/
    uint32_t transact_spi(
        int which_slave,
        const spi_config_t &config,
        uint32_t data,
        size_t num_bits,
        bool readback
    ){
        static const uhd::dict<spi_config_t::edge_t, int> spi_edge_to_otw = boost::assign::map_list_of
            (spi_config_t::EDGE_RISE, USRP2_CLK_EDGE_RISE)
            (spi_config_t::EDGE_FALL, USRP2_CLK_EDGE_FALL)
        ;

        //setup the out data
        usrp2_ctrl_data_t out_data = usrp2_ctrl_data_t();
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
    void write_i2c(uint16_t addr, const byte_vector_t &buf){
        //setup the out data
        usrp2_ctrl_data_t out_data = usrp2_ctrl_data_t();
        out_data.id = htonl(USRP2_CTRL_ID_WRITE_THESE_I2C_VALUES_BRO);
        out_data.data.i2c_args.addr = uint8_t(addr);
        out_data.data.i2c_args.bytes = buf.size();

        //limitation of i2c transaction size
        UHD_ASSERT_THROW(buf.size() <= sizeof(out_data.data.i2c_args.data));

        //copy in the data
        std::copy(buf.begin(), buf.end(), out_data.data.i2c_args.data);

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data, MIN_PROTO_COMPAT_I2C);
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_COOL_IM_DONE_I2C_WRITE_DUDE);
    }

    byte_vector_t read_i2c(uint16_t addr, size_t num_bytes){
        //setup the out data
        usrp2_ctrl_data_t out_data = usrp2_ctrl_data_t();
        out_data.id = htonl(USRP2_CTRL_ID_DO_AN_I2C_READ_FOR_ME_BRO);
        out_data.data.i2c_args.addr = uint8_t(addr);
        out_data.data.i2c_args.bytes = num_bytes;

        //limitation of i2c transaction size
        UHD_ASSERT_THROW(num_bytes <= sizeof(out_data.data.i2c_args.data));

        //send and recv
        usrp2_ctrl_data_t in_data = this->ctrl_send_and_recv(out_data, MIN_PROTO_COMPAT_I2C);
        UHD_ASSERT_THROW(ntohl(in_data.id) == USRP2_CTRL_ID_HERES_THE_I2C_DATA_DUDE);
        UHD_ASSERT_THROW(in_data.data.i2c_args.bytes == num_bytes);

        //copy out the data
        byte_vector_t result(num_bytes);
        std::copy(in_data.data.i2c_args.data, in_data.data.i2c_args.data + num_bytes, result.begin());
        return result;
    }

/***********************************************************************
 * Send/Recv over control
 **********************************************************************/
    usrp2_ctrl_data_t ctrl_send_and_recv(
        const usrp2_ctrl_data_t &out_data,
        uint32_t lo = USRP2_FW_COMPAT_NUM,
        uint32_t hi = USRP2_FW_COMPAT_NUM
    ){
        boost::mutex::scoped_lock lock(_ctrl_mutex);

        for (size_t i = 0; i < CTRL_RECV_RETRIES; i++){
            try{
                return ctrl_send_and_recv_internal(out_data, lo, hi, CTRL_RECV_TIMEOUT/CTRL_RECV_RETRIES);
            }
            catch(const timeout_error &e){
                UHD_LOGGER_ERROR("USRP2")
                    << "Control packet attempt " << i
                    << ", sequence number " << _ctrl_seq_num
                    << ":\n" << e.what() ;
            }
        }
        throw uhd::runtime_error("link dead: timeout waiting for control packet ACK");
    }

    usrp2_ctrl_data_t ctrl_send_and_recv_internal(
        const usrp2_ctrl_data_t &out_data,
        uint32_t lo, uint32_t hi,
        const double timeout
    ){
        //fill in the seq number and send
        usrp2_ctrl_data_t out_copy = out_data;
        out_copy.proto_ver = htonl(_protocol_compat);
        out_copy.seq = htonl(++_ctrl_seq_num);
        _ctrl_transport->send(boost::asio::buffer(&out_copy, sizeof(usrp2_ctrl_data_t)));

        //loop until we get the packet or timeout
        uint8_t usrp2_ctrl_data_in_mem[udp_simple::mtu]; //allocate max bytes for recv
        const usrp2_ctrl_data_t *ctrl_data_in = reinterpret_cast<const usrp2_ctrl_data_t *>(usrp2_ctrl_data_in_mem);
        while(true){
            size_t len = _ctrl_transport->recv(boost::asio::buffer(usrp2_ctrl_data_in_mem), timeout);
            uint32_t compat = ntohl(ctrl_data_in->proto_ver);
            if(len >= sizeof(uint32_t) and (hi < compat or lo > compat)){
                throw uhd::runtime_error(str(boost::format(
                    "\nPlease update the firmware and FPGA images for your device.\n"
                    "See the application notes for USRP2/N-Series for instructions.\n"
                    "Expected protocol compatibility number %s, but got %d:\n"
                    "The firmware build is not compatible with the host code build.\n"
                    "%s\n"
                ) % ((lo == hi)? (boost::format("%d") % hi) : (boost::format("[%d to %d]") % lo % hi))
                  % compat % this->images_warn_help_message()));
            }
            if (len >= sizeof(usrp2_ctrl_data_t) and ntohl(ctrl_data_in->seq) == _ctrl_seq_num){
                return *ctrl_data_in;
            }
            if (len == 0) break; //timeout
            //didnt get seq or bad packet, continue looking...
        }
        throw timeout_error("no control response, possible packet loss");
    }

    rev_type get_rev(void){
        std::string hw = mb_eeprom["hardware"];
        if (hw.empty()) return USRP_NXXX;
        switch (boost::lexical_cast<uint16_t>(hw)){
        case 0x0300:
        case 0x0301: return USRP2_REV3;
        case 0x0400: return USRP2_REV4;
        case 0x0A00: return USRP_N200;
        case 0x0A01: return USRP_N210;
        case 0x0A10: return USRP_N200_R4;
        case 0x0A11: return USRP_N210_R4;
        }
        return USRP_NXXX; //unknown type
    }

    const std::string get_cname(void){
        switch(this->get_rev()){
        case USRP2_REV3: return "USRP2 r3";
        case USRP2_REV4: return "USRP2 r4";
        case USRP_N200: return "N200";
        case USRP_N210: return "N210";
        case USRP_N200_R4: return "N200r4";
        case USRP_N210_R4: return "N210r4";
        case USRP_NXXX: return "N???";
        }
        UHD_THROW_INVALID_CODE_PATH();
    }

    const std::string get_fw_version_string(void){
        uint32_t minor = this->get_reg<uint32_t, USRP2_REG_ACTION_FW_PEEK32>(U2_FW_REG_VER_MINOR);
        return str(boost::format("%u.%u") % _protocol_compat % minor);
    }

    std::string images_warn_help_message(void){
        //determine the images names
        std::string fw_image, fpga_image;
        switch(this->get_rev()){
        case USRP2_REV3:   fpga_image = "usrp2_fpga.bin";        fw_image = "usrp2_fw.bin";     break;
        case USRP2_REV4:   fpga_image = "usrp2_fpga.bin";        fw_image = "usrp2_fw.bin";     break;
        case USRP_N200:    fpga_image = "usrp_n200_r2_fpga.bin"; fw_image = "usrp_n200_fw.bin"; break;
        case USRP_N210:    fpga_image = "usrp_n210_r2_fpga.bin"; fw_image = "usrp_n210_fw.bin"; break;
        case USRP_N200_R4: fpga_image = "usrp_n200_r4_fpga.bin"; fw_image = "usrp_n200_fw.bin"; break;
        case USRP_N210_R4: fpga_image = "usrp_n210_r4_fpga.bin"; fw_image = "usrp_n210_fw.bin"; break;
        default: break;
        }
        if (fw_image.empty() or fpga_image.empty()) return "";

        //does your platform use sudo?
        std::string sudo;
        #if defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_MACOS)
            sudo = "sudo ";
        #endif


        //look up the real FS path to the images
        std::string fw_image_path, fpga_image_path;
        try{
            fw_image_path = uhd::find_image_path(fw_image);
            fpga_image_path = uhd::find_image_path(fpga_image);
        }
        catch(const std::exception &){
            return str(boost::format("Could not find %s and %s in your images path!\n%s") % fw_image % fpga_image % print_utility_error("uhd_images_downloader.py"));
        }

        //escape char for multi-line cmd + newline + indent?
        #ifdef UHD_PLATFORM_WIN32
            const std::string ml = "^\n    ";
        #else
            const std::string ml = "\\\n    ";
        #endif

        //create the burner commands
        if (this->get_rev() == USRP2_REV3 or this->get_rev() == USRP2_REV4){
            const std::string card_burner = uhd::find_utility("usrp2_card_burner_gui.py");
            const std::string card_burner_cmd = str(boost::format(" %s\"%s\" %s--fpga=\"%s\" %s--fw=\"%s\"") % sudo % card_burner % ml % fpga_image_path % ml % fw_image_path);
            return str(boost::format("%s\n%s") % print_utility_error("uhd_images_downloader.py") % card_burner_cmd);
        }
        else{
            const std::string addr = _ctrl_transport->get_recv_addr();
            const std::string image_loader_path = (fs::path(uhd::get_pkg_path()) / "bin" / "uhd_image_loader").string();
            const std::string image_loader_cmd = str(boost::format(" \"%s\" %s--args=\"type=usrp2,addr=%s\"") % image_loader_path % ml % addr);
            return str(boost::format("%s\n%s") % print_utility_error("uhd_images_downloader.py") % image_loader_cmd);
        }
    }

    void set_time(const time_spec_t&)
    {
        throw uhd::not_implemented_error("Timed commands not supported");
    }

    time_spec_t get_time(void)
    {
        return (0.0);
    }

private:
    //this lovely lady makes it all possible
    udp_simple::sptr _ctrl_transport;

    //used in send/recv
    boost::mutex _ctrl_mutex;
    uint32_t _ctrl_seq_num;
    uint32_t _protocol_compat;

    //lock thread stuff
    task::sptr _lock_task;
};

/***********************************************************************
 * Public make function for usrp2 interface
 **********************************************************************/
usrp2_iface::sptr usrp2_iface::make(udp_simple::sptr ctrl_transport){
    return usrp2_iface::sptr(new usrp2_iface_impl(ctrl_transport));
}

