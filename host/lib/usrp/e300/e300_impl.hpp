//
// Copyright 2013-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_E300_IMPL_HPP
#define INCLUDED_E300_IMPL_HPP

#include "../device3/device3_impl.hpp"
#include <uhd/property_tree.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/sensors.hpp>

#include <boost/weak_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/dynamic_bitset.hpp>
#include <string>
#include "e300_fifo_config.hpp"

#include "e300_global_regs.hpp"
#include "e300_i2c.hpp"
#include "e300_eeprom_manager.hpp"
#include "e300_sensor_manager.hpp"

/* if we don't compile with gpsd support, don't bother */
#ifdef E300_GPSD
#include "gpsd_iface.hpp"
#endif

#include <atomic>

namespace uhd { namespace usrp { namespace e300 {

static const std::string E300_FPGA_FILE_NAME = "usrp_e300_fpga.bit";
static const std::string E310_SG1_FPGA_FILE_NAME = "usrp_e310_fpga.bit";
static const std::string E310_SG3_FPGA_FILE_NAME = "usrp_e310_fpga_sg3.bit";

static const std::string E3XX_SG1_FPGA_IDLE_FILE_NAME = "usrp_e3xx_fpga_idle.bit";
static const std::string E3XX_SG3_FPGA_IDLE_FILE_NAME = "usrp_e3xx_fpga_idle_sg3.bit";

static const std::string E300_TEMP_SYSFS = "iio:device0";
static const std::string E300_SPIDEV_DEVICE  = "/dev/spidev0.1";
static const std::string E300_I2CDEV_DEVICE  = "/dev/i2c-0";

static std::string E300_SERVER_RX_PORT0    = "21756";
static std::string E300_SERVER_TX_PORT0    = "21757";
static std::string E300_SERVER_CTRL_PORT0  = "21758";

static std::string E300_SERVER_RX_PORT1    = "21856";
static std::string E300_SERVER_TX_PORT1    = "21857";
static std::string E300_SERVER_CTRL_PORT1  = "21858";


static std::string E300_SERVER_CODEC_PORT  = "21759";
static std::string E300_SERVER_GREGS_PORT  = "21760";
static std::string E300_SERVER_I2C_PORT    = "21761";
static std::string E300_SERVER_SENSOR_PORT = "21762";

static const double E300_RX_SW_BUFF_FULLNESS = 0.9;        //Buffer should be half full
static const size_t E300_RX_FC_REQUEST_FREQ = 5; // per flow ctrl window
static const size_t E300_TX_FC_RESPONSE_FREQ = 8; // per flow ctrl window

// crossbar settings
static const uint8_t E300_RADIO_DEST_PREFIX_TX   = 0;
static const uint8_t E300_RADIO_DEST_PREFIX_CTRL = 1;
static const uint8_t E300_RADIO_DEST_PREFIX_RX   = 2;

static const uint8_t E300_XB_DST_AXI = 0;
static const uint8_t E300_XB_DST_RADIO  = 1;
static const uint8_t E300_XB_DST_R1  = 2;
// RFNoC blocks are connected to the first port
// after the last radio (there might be less than 2
// radios).

static const uint8_t E300_DEVICE_THERE = 2;
static const uint8_t E300_DEVICE_HERE  = 0;

static const size_t E300_R0_CTRL_STREAM    = (0 << 2) | E300_RADIO_DEST_PREFIX_CTRL;
static const size_t E300_R0_TX_DATA_STREAM = (0 << 2) | E300_RADIO_DEST_PREFIX_TX;
static const size_t E300_R0_RX_DATA_STREAM = (0 << 2) | E300_RADIO_DEST_PREFIX_RX;

static const size_t E300_R1_CTRL_STREAM    = (1 << 2) | E300_RADIO_DEST_PREFIX_CTRL;
static const size_t E300_R1_TX_DATA_STREAM = (1 << 2) | E300_RADIO_DEST_PREFIX_TX;
static const size_t E300_R1_RX_DATA_STREAM = (1 << 2) | E300_RADIO_DEST_PREFIX_RX;

uhd::device_addrs_t e300_find(const uhd::device_addr_t &multi_dev_hint);
void get_e3x0_fpga_images(const uhd::device_addr_t &device_args,
                          std::string &fpga_image,
                          std::string &idle_image);

/*!
 * USRP-E300 implementation guts:
 * The implementation details are encapsulated here.
 * Handles properties on the mboard, dboard, dsps...
 */
class e300_impl : public uhd::usrp::device3_impl
{
public:
    /************************************************************************
     * Structors
     ***********************************************************************/
    e300_impl(const uhd::device_addr_t &);
    virtual ~e300_impl(void);

private: // types
    enum compat_t {FPGA_MAJOR, FPGA_MINOR};

protected: // methods
    /************************************************************************
     * Legacy device3 stuff
     ***********************************************************************/
    void subdev_to_blockid(
            const uhd::usrp::subdev_spec_pair_t &spec, const size_t mb_i,
            rfnoc::block_id_t &block_id, uhd::device_addr_t &block_args
    );
    uhd::usrp::subdev_spec_pair_t blockid_to_subdev(
            const rfnoc::block_id_t &blockid, const device_addr_t &block_args
    );

    /************************************************************************
     * Transport related
     ***********************************************************************/
    uhd::device_addr_t get_rx_hints(size_t);

private: // methods
    /************************************************************************
     * Initialization
     ***********************************************************************/
    void _register_loopback_self_test(wb_iface::sptr iface, uint32_t w_addr, uint32_t r_addr);

    uint32_t _get_version(compat_t which);
    std::string _get_version_hash(void);

    /************************************************************************
     * Transport related
     ***********************************************************************/
    uhd::sid_t _allocate_sid(const uhd::sid_t &address);

    void _setup_dest_mapping(
        const uhd::sid_t &sid,
        const size_t which_stream);

    /*! Return the first free AXI channel pair.
     *
     * \throws uhd::runtime_error if no free channel pairs are available.
     */
    size_t _get_axi_dma_channel_pair();

    // For network mode
    uint16_t _get_udp_port(
        uint8_t destination,
        uint8_t prefix);

    uhd::both_xports_t make_transport(
        const uhd::sid_t &address,
        const xport_type_t type,
        const uhd::device_addr_t &args
    );

    uhd::endianness_t get_transport_endianness(size_t) {
        return uhd::ENDIANNESS_LITTLE;
    };

    /************************************************************************
     * Helpers
     ***********************************************************************/
    void _update_clock_source(const std::string &);

private: // members
    const uhd::device_addr_t               _device_addr;
    xport_t                                _xport_path;
    e300_fifo_interface::sptr              _fifo_iface;
    std::atomic<size_t>                    _sid_framer;
    boost::dynamic_bitset<>                _dma_chans_available;
    global_regs::sptr                      _global_regs;
    e300_sensor_manager::sptr              _sensor_manager;
    e300_eeprom_manager::sptr              _eeprom_manager;
    uhd::transport::zero_copy_xport_params _data_xport_params;
    uhd::transport::zero_copy_xport_params _ctrl_xport_params;
    std::string                            _idle_image;
    bool                                   _do_not_reload;
#ifdef E300_GPSD
    gpsd_iface::sptr                       _gps;
    static const size_t                    _GPS_TIMEOUT = 5;
#endif
};

}}} // namespace uhd::usrp::e300

#endif /* INCLUDED_E300_IMPL_HPP */
