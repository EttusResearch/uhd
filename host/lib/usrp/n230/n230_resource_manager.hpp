//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_RESOURCE_MANAGER_HPP
#define INCLUDED_N230_RESOURCE_MANAGER_HPP

#include "n230_fw_ctrl_iface.hpp"
#include "n230_clk_pps_ctrl.hpp"
#include "n230_cores.hpp"
#include "n230_fpga_defs.h"
#include "n230_frontend_ctrl.hpp"
#include "n230_uart.hpp"

#include <uhdlib/usrp/cores/radio_ctrl_core_3000.hpp>
#include <uhdlib/usrp/cores/spi_core_3000.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <uhdlib/usrp/cores/rx_vita_core_3000.hpp>
#include <uhdlib/usrp/cores/tx_vita_core_3000.hpp>
#include <uhdlib/usrp/cores/time_core_3000.hpp>
#include <uhdlib/usrp/cores/rx_dsp_core_3000.hpp>
#include <uhdlib/usrp/cores/tx_dsp_core_3000.hpp>
#include <uhdlib/usrp/cores/user_settings_core_3000.hpp>
#include <uhdlib/usrp/common/ad9361_ctrl.hpp>
#include <uhdlib/usrp/common/ad936x_manager.hpp>

#include <uhd/utils/tasks.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/soft_register.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/usrp/gps_ctrl.hpp>

namespace uhd { namespace usrp { namespace n230 {

enum n230_eth_port_t {
    ETH0,
    ETH1
};

enum n230_eth_pref_t {
    PRI_ETH,
    SEC_ETH
};

enum n230_endpoint_t {
    RADIO_TX_DATA,
    RADIO_RX_DATA,
    RADIO_CONTROL,
    CORE,
    GPS_UART
};

enum n230_ver_src_t {
    SOFTWARE,
    FIRMWARE,
    FPGA
};

enum n230_version_t {
    COMPAT_MAJOR,
    COMPAT_MINOR
};

enum n230_data_dir_t {
    RX_DATA, TX_DATA
};

//Radio resources
class radio_resource_t : public boost::noncopyable {
public:
    radio_ctrl_core_3000::sptr      ctrl;
    gpio_atr::gpio_atr_3000::sptr   gpio_atr;
    time_core_3000::sptr            time;
    rx_vita_core_3000::sptr         framer;
    rx_dsp_core_3000::sptr          ddc;
    tx_vita_core_3000::sptr         deframer;
    tx_dsp_core_3000::sptr          duc;
    user_settings_core_3000::sptr   user_settings;
};

class n230_resource_manager : public boost::noncopyable
{
public:     //Methods
    n230_resource_manager(const std::vector<std::string> ip_addrs, const bool safe_mode);
    virtual ~n230_resource_manager();

    static bool is_device_claimed(n230_fw_ctrl_iface::sptr fw_ctrl);

    inline bool is_device_claimed() {
        if (_fw_ctrl.get()) {
            return is_device_claimed(_fw_ctrl);
        } else {
            return false;
        }
    }

    inline uint32_t get_version(n230_ver_src_t src, n230_version_t type) {
        switch (src) {
            case FPGA:      return _fpga_version.get(type);
            case FIRMWARE:  return _fw_version.get(type);
            default:        return 0;
        }
    }

    inline const std::string get_version_hash(n230_ver_src_t src) {
        switch (src) {
            case FPGA:      return _fpga_version.get_hash_str();
            case FIRMWARE:  return _fw_version.get_hash_str();
            default:        return "";
        }
    }

    //Firmware control interface
    inline wb_iface& get_fw_ctrl() const {
        return *_fw_ctrl;
    }
    inline wb_iface::sptr get_fw_ctrl_sptr() {
        return _fw_ctrl;
    }

    //Core settings control interface
    inline radio_ctrl_core_3000& get_core_ctrl() const {
        return *_core_ctrl;
    }
    inline radio_ctrl_core_3000::sptr get_core_ctrl_sptr() {
        return _core_ctrl;
    }

    //AD931 control interface
    inline ad9361_ctrl& get_codec_ctrl() const {
        return *_codec_ctrl;
    }
    inline ad9361_ctrl::sptr get_codec_ctrl_sptr() {
        return _codec_ctrl;
    }
    inline uhd::usrp::ad936x_manager& get_codec_mgr() const {
        return *_codec_mgr;
    }

    //Clock PPS controls
    inline n230_ref_pll_ctrl& get_ref_pll_ctrl() const {
        return *_ref_pll_ctrl;
    }
    inline n230_ref_pll_ctrl::sptr get_ref_pll_ctrl_sptr() {
        return _ref_pll_ctrl;
    }

    //Clock PPS controls
    inline n230_clk_pps_ctrl& get_clk_pps_ctrl() const {
        return *_clk_pps_ctrl;
    }
    inline n230_clk_pps_ctrl::sptr get_clk_pps_ctrl_sptr() {
        return _clk_pps_ctrl;
    }

    //Front-end control
    inline n230_frontend_ctrl& get_frontend_ctrl() const {
        return *_frontend_ctrl;
    }
    inline n230_frontend_ctrl::sptr get_frontend_ctrl_sptr() {
        return _frontend_ctrl;
    }

    //MiniSAS GPIO control
    inline gpio_atr::gpio_atr_3000::sptr get_minisas_gpio_ctrl_sptr(size_t idx) {
        return idx == 0 ? _ms0_gpio : _ms1_gpio;
    }

    inline gpio_atr::gpio_atr_3000& get_minisas_gpio_ctrl(size_t idx) {
        return *get_minisas_gpio_ctrl_sptr(idx);
    }

    //GPSDO control
    inline bool is_gpsdo_present() {
        return _gps_ctrl.get() and _gps_ctrl->gps_detected();
    }

    inline uhd::gps_ctrl::sptr get_gps_ctrl(void) {
        return _gps_ctrl;
    }

    inline radio_resource_t& get_radio(size_t instance) {
        return _radios[instance];
    }

    //Transport to stream data
    transport::zero_copy_if::sptr create_transport(
        n230_data_dir_t direction, size_t radio_instance,
        const device_addr_t &params, sid_t& sid,
        transport::udp_zero_copy::buff_params& buff_out_params);

    //Misc
    inline double get_max_link_rate() {
        return fpga::N230_LINK_RATE_BPS * _eth_conns.size();
    }

private:
    struct ver_info_t {
        uint8_t  compat_major;
        uint16_t compat_minor;
        uint32_t version_hash;

        uint32_t get(n230_version_t type) {
            switch (type) {
                case COMPAT_MAJOR: return compat_major;
                case COMPAT_MINOR: return compat_minor;
                default:           return 0;
            }
        }

        const std::string get_hash_str() {
            return (str(boost::format("%07x%s")
                % (version_hash & 0x0FFFFFFF)
                % ((version_hash & 0xF0000000) ? "(modified)" : "")));
        }
    };

    struct n230_eth_conn_t {
        std::string ip_addr;
        n230_eth_port_t type;
    };

    //-- Functions --

    void _claimer_loop();

    void _initialize_radio(size_t instance);

    std::string _get_fpga_upgrade_msg();
    void _check_fw_compat();
    void _check_fpga_compat();

    const sid_t _generate_sid(
        const n230_endpoint_t type, const n230_eth_port_t xport, size_t instance = 0);

    transport::zero_copy_if::sptr _create_transport(
        const n230_eth_conn_t& eth_conn,
        const sid_t& sid, const device_addr_t &buff_params,
        transport::udp_zero_copy::buff_params& buff_params_out);

    void _program_dispatcher(
        transport::zero_copy_if& xport, const n230_eth_port_t port, const sid_t& sid);

    void _reset_codec_digital_interface();

    bool _radio_register_loopback_self_test(wb_iface::sptr iface);

    bool _radio_data_loopback_self_test(wb_iface::sptr iface);

    inline const n230_eth_conn_t& _get_conn(const n230_eth_pref_t pref) {
        if (_eth_conns.size() == 1)
            return _eth_conns[0];
        else
            return _eth_conns[(pref==PRI_ETH)?0:1];
    }

    //-- Members --

    std::vector<n230_eth_conn_t>    _eth_conns;
    const bool                      _safe_mode;
    ver_info_t                      _fw_version;
    ver_info_t                      _fpga_version;

    //Firmware register interface
    n230_fw_ctrl_iface::sptr        _fw_ctrl;
    uhd::task::sptr                 _claimer_task;
    static boost::mutex             _claimer_mutex;  //All claims and checks in this process are serialized

    //Transport
    uint8_t                         _last_host_enpoint;

    //Radio settings interface
    radio_ctrl_core_3000::sptr      _core_ctrl;
    n230_core_spi_core::sptr        _core_spi_ctrl;
    ad9361_ctrl::sptr               _codec_ctrl;
    uhd::usrp::ad936x_manager::sptr _codec_mgr;

    //Core Registers
    fpga::core_radio_ctrl_reg_t     _core_radio_ctrl_reg;
    fpga::core_misc_reg_t           _core_misc_reg;
    fpga::core_pps_sel_reg_t        _core_pps_sel_reg;
    fpga::core_status_reg_t         _core_status_reg;

    //Radio peripherals
    radio_resource_t                _radios[fpga::NUM_RADIOS];

    //Misc IO peripherals
    n230_ref_pll_ctrl::sptr         _ref_pll_ctrl;
    n230_clk_pps_ctrl::sptr         _clk_pps_ctrl;
    n230_frontend_ctrl::sptr        _frontend_ctrl;

    //miniSAS GPIO
    gpio_atr::gpio_atr_3000::sptr   _ms0_gpio;
    gpio_atr::gpio_atr_3000::sptr   _ms1_gpio;

    //GPSDO
    n230_uart::sptr                 _gps_uart;
    uhd::gps_ctrl::sptr             _gps_ctrl;

};

}}} //namespace

#endif //INCLUDED_N230_RESOURCE_MANAGER_HPP
