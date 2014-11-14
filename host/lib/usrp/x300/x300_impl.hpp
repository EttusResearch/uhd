//
// Copyright 2013-2014 Ettus Research LLC
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

#ifndef INCLUDED_X300_IMPL_HPP
#define INCLUDED_X300_IMPL_HPP

#include <uhd/property_tree.hpp>
#include "../device3/device3_impl.hpp"
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/types/sensors.hpp>
#include "x300_clock_ctrl.hpp"
#include "x300_fw_common.h"
#include <uhd/transport/udp_simple.hpp> //mtu
#include "spi_core_3000.hpp"
#include "x300_adc_ctrl.hpp"
#include "x300_dac_ctrl.hpp"
#include "rx_vita_core_3000.hpp"
#include "tx_vita_core_3000.hpp"
#include "time_core_3000.hpp"
#include "rx_dsp_core_3000.hpp"
#include "tx_dsp_core_3000.hpp"
#include "i2c_core_100_wb32.hpp"
#include "radio_ctrl_core_3000.hpp"
#include "rx_frontend_core_200.hpp"
#include "tx_frontend_core_200.hpp"
#include "gpio_core_200.hpp"
#include <boost/weak_ptr.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/transport/vrt_if_packet.hpp>
#include "recv_packet_demuxer_3000.hpp"
///////////// RFNOC /////////////////////
#include <uhd/usrp/rfnoc/block_ctrl.hpp>
///////////// RFNOC /////////////////////


static const std::string X300_FW_FILE_NAME  = "usrp_x300_fw.bin";

static const double X300_DEFAULT_TICK_RATE      = 200e6;        //Hz
static const double X300_BUS_CLOCK_RATE         = 166.666667e6; //Hz

static const size_t X300_TX_HW_BUFF_SIZE        = 520*1024;      //512K SRAM buffer + 8K 2Clk FIFO

static const size_t X300_RX_SW_BUFF_SIZE_ETH        = 0x2000000;//32MiB    For an ~8k frame size any size >32MiB is just wasted buffer space
static const size_t X300_RX_SW_BUFF_SIZE_ETH_MACOS  = 0x100000; //1Mib

//The FIFO closest to the DMA controller is 1023 elements deep for RX and 1029 elements deep for TX
//where an element is 8 bytes. For best throughput ensure that the data frame fits in these buffers.
//Also ensure that the kernel has enough frames to hold buffered TX and RX data
static const size_t X300_PCIE_RX_DATA_FRAME_SIZE    = 8184;     //bytes
static const size_t X300_PCIE_TX_DATA_FRAME_SIZE    = 8192;     //bytes
static const size_t X300_PCIE_DATA_NUM_FRAMES       = 2048;
static const size_t X300_PCIE_MSG_FRAME_SIZE        = 256;      //bytes
static const size_t X300_PCIE_MSG_NUM_FRAMES        = 32;

static const size_t X300_10GE_DATA_FRAME_MAX_SIZE   = 8000;     //bytes
static const size_t X300_1GE_DATA_FRAME_MAX_SIZE    = 1472;     //bytes
static const size_t X300_ETH_MSG_FRAME_SIZE         = uhd::transport::udp_simple::mtu;  //bytes

static const size_t X300_ETH_MSG_NUM_FRAMES         = 32;
static const size_t X300_ETH_DATA_NUM_FRAMES        = 32;
static const double X300_DEFAULT_SYSREF_RATE        = 10e6;

static const size_t X300_MAX_RATE_PCIE              = 800000000; // bytes/s
static const size_t X300_MAX_RATE_10GIGE            = 800000000; // bytes/s
static const size_t X300_MAX_RATE_1GIGE             = 100000000; // bytes/s

#define X300_RADIO_DEST_PREFIX_TX 0

#define X300_XB_DST_E0  0
#define X300_XB_DST_E1  1
#define X300_XB_DST_PCI 2
#define X300_XB_DST_R0  3 // Radio 0 -> Slot A
#define X300_XB_DST_R1  4 // Radio 1 -> Slot B
#define X300_XB_DST_CE0 5

#define X300_DEVICE_THERE 2
#define X300_DEVICE_HERE 0

//eeprom addrs for various boards
enum
{
    X300_DB0_RX_EEPROM = 0x5,
    X300_DB0_TX_EEPROM = 0x4,
    X300_DB0_GDB_EEPROM = 0x1,
    X300_DB1_RX_EEPROM = 0x7,
    X300_DB1_TX_EEPROM = 0x6,
    X300_DB1_GDB_EEPROM = 0x3,
};

struct x300_dboard_iface_config_t
{
    gpio_core_200::sptr gpio;
    spi_core_3000::sptr spi;
    size_t rx_spi_slaveno;
    size_t tx_spi_slaveno;
    i2c_core_100_wb32::sptr i2c;
    x300_clock_ctrl::sptr clock;
    x300_clock_which_t which_rx_clk;
    x300_clock_which_t which_tx_clk;
    boost::uint8_t dboard_slot;
};

uhd::usrp::dboard_iface::sptr x300_make_dboard_iface(const x300_dboard_iface_config_t &);
uhd::uart_iface::sptr x300_make_uart_iface(uhd::wb_iface::sptr iface);

uhd::wb_iface::sptr x300_make_ctrl_iface_enet(uhd::transport::udp_simple::sptr udp);
uhd::wb_iface::sptr x300_make_ctrl_iface_pcie(uhd::niusrprio::niriok_proxy::sptr drv_proxy);

class x300_impl : public uhd::usrp::device3_impl
{
public:

    x300_impl(const uhd::device_addr_t &);
    void setup_mb(const size_t which, const uhd::device_addr_t &);
    ~x300_impl(void);

    // used by x300_find_with_addr to find X300 devices.
    static boost::mutex claimer_mutex;  //All claims and checks in this process are serialized
    static bool is_claimed(uhd::wb_iface::sptr);

    enum x300_mboard_t {
        USRP_X300_MB, USRP_X310_MB, UNKNOWN
    };
    static x300_mboard_t get_mb_type_from_pcie(const std::string& resource, const std::string& rpc_port);
    static x300_mboard_t get_mb_type_from_eeprom(const uhd::usrp::mboard_eeprom_t& mb_eeprom);

private:

    //perifs in the radio core
    struct radio_perifs_t
    {
        radio_ctrl_core_3000::sptr ctrl;
        spi_core_3000::sptr spi;
        x300_adc_ctrl::sptr adc;
        x300_dac_ctrl::sptr dac;
        time_core_3000::sptr time64;
        rx_vita_core_3000::sptr framer;
        rx_dsp_core_3000::sptr ddc;
        tx_vita_core_3000::sptr deframer;
        tx_dsp_core_3000::sptr duc;
        gpio_core_200_32wo::sptr leds;
        rx_frontend_core_200::sptr rx_fe;
        tx_frontend_core_200::sptr tx_fe;
    };

    //vector of member objects per motherboard
    struct mboard_members_t
    {
        bool initialization_done;
        uhd::task::sptr claimer_task;
        std::string addr;
        std::string xport_path;
        int router_dst_here;
        uhd::device_addr_t send_args;
        uhd::device_addr_t recv_args;
        bool if_pkt_is_big_endian;
        uhd::niusrprio::niusrprio_session::sptr  rio_fpga_interface;

        //perifs in the zpu
        uhd::wb_iface::sptr zpu_ctrl;
        spi_core_3000::sptr zpu_spi;
        i2c_core_100_wb32::sptr zpu_i2c;

        //perifs in each radio
        radio_perifs_t radio_perifs[2]; //!< This is hardcoded s.t. radio_perifs[0] points to slot A and [1] to B
        uhd::usrp::dboard_eeprom_t db_eeproms[8];
        //! Return the index of a radio component, given a slot name. This means DSPs, radio_perifs
        size_t get_radio_index(const std::string &slot_name) {
             UHD_ASSERT_THROW(slot_name == "A" or slot_name == "B");
             return slot_name == "A" ? 0 : 1;
        }

        //other perifs on mboard
        x300_clock_ctrl::sptr clock;
        uhd::gps_ctrl::sptr gps;
        gpio_core_200::sptr fp_gpio;

        //clock control register bits
        int clock_control_regs_clock_source;
        int clock_control_regs_pps_select;
        int clock_control_regs_pps_out_enb;
        int clock_control_regs_tcxo_enb;
        int clock_control_regs_gpsdo_pwr;

        //which FPGA image is loaded
        std::string loaded_fpga_image;

        size_t hw_rev;
        std::string current_refclk_src;
    };
    std::vector<mboard_members_t> _mb;

    //task for periodically reclaiming the device from others
    void claimer_loop(uhd::wb_iface::sptr);

    void register_loopback_self_test(uhd::wb_iface::sptr iface);

     /*! \brief Initialize the radio component on a given slot.
      *
      * Call this function once per slot (A and B) and motherboard to initialize all the radio components.
      * This will:
      * - Reset and init DACs and ADCs
      * - Setup controls for DAC, ADC, SPI and LEDs
      * - Self test ADC
      * - Sync DACs (for MIMO)
      * - Initialize the property tree for control objects etc. (gain, rate...)
      * - Populate the radio ctrl object for RFNoC operation
      *
      * \param mb_i Motherboard index
      * \param slot_name Slot name (A or B).
      */
    void setup_radio(const size_t, const std::string &slot_name);

    uhd::sid_t allocate_sid(mboard_members_t &mb, const uhd::sid_t &address);
    both_xports_t make_transport(
        const uhd::sid_t &address,
        const xport_type_t xport_type,
        const uhd::device_addr_t& args
    );

    struct frame_size_t
    {
        size_t recv_frame_size;
        size_t send_frame_size;
    };
    frame_size_t _max_frame_sizes;

    /*!
     * Automatically determine the maximum frame size available by sending a UDP packet
     * to the device and see which packet sizes actually work. This way, we can take
     * switches etc. into account which might live between the device and the host.
     */
    frame_size_t determine_max_frame_size(const std::string &addr, const frame_size_t &user_mtu);

    ////////////////////////////////////////////////////////////////////
    //
    //Caching for transport interface re-use -- like sharing a DMA.
    //The cache is optionally used by make_transport by use-case.
    //The cache maps an ID string to a transport-ish object.
    //The ID string identifies a purpose for the transport.
    //
    //For recv, there is a demux cache, which maps a ID string
    //to a recv demux object. When a demux is used, the underlying transport
    //must never be used outside of the demux. Use demux->make_proxy(sid).
    //
    uhd::dict<std::string, uhd::usrp::recv_packet_demuxer_3000::sptr> _demux_cache;
    //
    //For send, there is a shared send xport, which maps an ID string
    //to a transport capable of sending buffers. Send transports
    //can be shared amongst multiple callers, unlike recv.
    //
    uhd::dict<std::string, uhd::transport::zero_copy_if::sptr> _send_cache;
    //
    ////////////////////////////////////////////////////////////////////

    uhd::dict<std::string, uhd::usrp::dboard_manager::sptr> _dboard_managers;
    uhd::dict<std::string, uhd::usrp::dboard_iface::sptr> _dboard_ifaces;

    void set_rx_fe_corrections(const uhd::fs_path &mb_path, const std::string &fe_name, const double lo_freq);
    void set_tx_fe_corrections(const uhd::fs_path &mb_path, const std::string &fe_name, const double lo_freq);
    bool _ignore_cal_file;


    /*! Update the IQ MUX settings for the radio peripheral according to given subdev spec.
     *
     * Also checks if the given subdev is valid for this device and updates the channel to DSP mapping.
     *
     * \param tx_rx "tx" or "rx", depending where you're setting the subdev spec
     * \param mb_i Mainboard index number.
     * \param spec Subdev spec
     */
    void update_subdev_spec(const std::string &tx_rx, const size_t mb_i, const uhd::usrp::subdev_spec_t &spec);

    void set_tick_rate(mboard_members_t &, const double);
    //! Returns the tick rate for device with index \p mb_i
    double _get_tick_rate(const size_t mb_i);
    void update_tick_rate(const size_t, const double);
    void update_rx_samp_rate(mboard_members_t&, const size_t, const double);
    void update_tx_samp_rate(mboard_members_t&, const size_t, const double);

    void update_clock_control(mboard_members_t&);
    void initialize_clock_control(mboard_members_t &mb);
    void set_time_source_out(mboard_members_t&, const bool);
    void update_clock_source(mboard_members_t&, const std::string &);
    void update_time_source(mboard_members_t&, const std::string &);
    void reset_radios(mboard_members_t&);

    uhd::sensor_value_t get_ref_locked(uhd::wb_iface::sptr);
    bool wait_for_ref_locked(uhd::wb_iface::sptr, double timeout = 0.0);
    bool is_pps_present(uhd::wb_iface::sptr);

    void set_db_eeprom(uhd::i2c_iface::sptr i2c, const size_t, const uhd::usrp::dboard_eeprom_t &);
    void set_mb_eeprom(uhd::i2c_iface::sptr i2c, const uhd::usrp::mboard_eeprom_t &);

    void check_fw_compat(const uhd::fs_path &mb_path, uhd::wb_iface::sptr iface);
    void check_fpga_compat(const uhd::fs_path &mb_path, uhd::wb_iface::sptr iface);

    void update_atr_leds(gpio_core_200_32wo::sptr, const std::string &ant);
    boost::uint32_t get_fp_gpio(gpio_core_200::sptr, const std::string &);
    void set_fp_gpio(gpio_core_200::sptr, const std::string &, const boost::uint32_t);

    // Loopback stuff
    void test_rfnoc_loopback(size_t mb_index, int ce_index);

    //**PRECONDITION**
    //This function assumes that all the VITA times in "radios" are synchronized
    //to a common reference. Currently, this function is called in get_tx_stream
    //which also has the same precondition.
    static void synchronize_dacs(const std::vector<radio_perifs_t*>& mboards);

    /// More IO stuff
    uhd::device_addr_t get_tx_hints(size_t mb_index);
    uhd::device_addr_t get_rx_hints(size_t mb_index);
    uhd::endianness_t get_transport_endianness(size_t mb_index) {
        return _mb[mb_index].if_pkt_is_big_endian ? uhd::ENDIANNESS_BIG : uhd::ENDIANNESS_LITTLE;
    };

    // TODO do the sync dacs in here
    void post_streamer_hooks();
};

#endif /* INCLUDED_X300_IMPL_HPP */
// vim: sw=4 expandtab:
