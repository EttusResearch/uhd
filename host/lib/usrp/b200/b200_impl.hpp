//
// Copyright 2012-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_B200_IMPL_HPP
#define INCLUDED_B200_IMPL_HPP

#include "b200_iface.hpp"
#include "b200_uart.hpp"
#include "b200_cores.hpp"
#include <uhd/device.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/utils/pimpl.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/transport/usb_zero_copy.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhdlib/usrp/common/ad9361_ctrl.hpp>
#include <uhdlib/usrp/cores/rx_vita_core_3000.hpp>
#include <uhdlib/usrp/cores/tx_vita_core_3000.hpp>
#include <uhdlib/usrp/cores/time_core_3000.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <uhdlib/usrp/cores/radio_ctrl_core_3000.hpp>
#include <uhdlib/usrp/cores/rx_dsp_core_3000.hpp>
#include <uhdlib/usrp/cores/tx_dsp_core_3000.hpp>
#include <uhdlib/usrp/common/recv_packet_demuxer_3000.hpp>
#include <uhdlib/usrp/common/ad936x_manager.hpp>
#include <uhdlib/usrp/common/adf4001_ctrl.hpp>
#include <boost/assign.hpp>
#include <boost/weak_ptr.hpp>

static const uint8_t  B200_FW_COMPAT_NUM_MAJOR = 8;
static const uint8_t  B200_FW_COMPAT_NUM_MINOR = 0;
static const uint16_t B200_FPGA_COMPAT_NUM = 14;
static const uint16_t B205_FPGA_COMPAT_NUM = 5;
static const double          B200_BUS_CLOCK_RATE = 100e6;
static const uint32_t B200_GPSDO_ST_NONE = 0x83;
static const size_t B200_MAX_RATE_USB2              =  53248000; // bytes/s
static const size_t B200_MAX_RATE_USB3              = 500000000; // bytes/s

#define FLIP_SID(sid) (((sid)<<16)|((sid)>>16))

static const uint32_t B200_CTRL0_MSG_SID = 0x00000010;
static const uint32_t B200_RESP0_MSG_SID = FLIP_SID(B200_CTRL0_MSG_SID);

static const uint32_t B200_CTRL1_MSG_SID = 0x00000020;
static const uint32_t B200_RESP1_MSG_SID = FLIP_SID(B200_CTRL1_MSG_SID);

static const uint32_t B200_TX_DATA0_SID = 0x00000050;
static const uint32_t B200_TX_MSG0_SID = FLIP_SID(B200_TX_DATA0_SID);

static const uint32_t B200_TX_DATA1_SID = 0x00000060;
static const uint32_t B200_TX_MSG1_SID = FLIP_SID(B200_TX_DATA1_SID);

static const uint32_t B200_RX_DATA0_SID = 0x000000A0;
static const uint32_t B200_RX_DATA1_SID = 0x000000B0;

static const uint32_t B200_TX_GPS_UART_SID = 0x00000030;
static const uint32_t B200_RX_GPS_UART_SID = FLIP_SID(B200_TX_GPS_UART_SID);

static const uint32_t B200_LOCAL_CTRL_SID = 0x00000040;
static const uint32_t B200_LOCAL_RESP_SID = FLIP_SID(B200_LOCAL_CTRL_SID);

static const unsigned char B200_USB_CTRL_RECV_INTERFACE = 4;
static const unsigned char B200_USB_CTRL_RECV_ENDPOINT  = 8;
static const unsigned char B200_USB_CTRL_SEND_INTERFACE = 3;
static const unsigned char B200_USB_CTRL_SEND_ENDPOINT  = 4;

static const unsigned char B200_USB_DATA_RECV_INTERFACE = 2;
static const unsigned char B200_USB_DATA_RECV_ENDPOINT  = 6;
static const unsigned char B200_USB_DATA_SEND_INTERFACE = 1;
static const unsigned char B200_USB_DATA_SEND_ENDPOINT  = 2;

/*
 * VID/PID pairs for all B2xx products
 */
static std::vector<uhd::transport::usb_device_handle::vid_pid_pair_t> b200_vid_pid_pairs =
    boost::assign::list_of
        (uhd::transport::usb_device_handle::vid_pid_pair_t(B200_VENDOR_ID, B200_PRODUCT_ID))
        (uhd::transport::usb_device_handle::vid_pid_pair_t(B200_VENDOR_ID, B200MINI_PRODUCT_ID))
        (uhd::transport::usb_device_handle::vid_pid_pair_t(B200_VENDOR_ID, B205MINI_PRODUCT_ID))
        (uhd::transport::usb_device_handle::vid_pid_pair_t(B200_VENDOR_NI_ID, B200_PRODUCT_NI_ID))
        (uhd::transport::usb_device_handle::vid_pid_pair_t(B200_VENDOR_NI_ID, B210_PRODUCT_NI_ID))
    ;

b200_product_t get_b200_product(const uhd::transport::usb_device_handle::sptr& handle, const uhd::usrp::mboard_eeprom_t &mb_eeprom);
std::vector<uhd::transport::usb_device_handle::sptr> get_b200_device_handles(const uhd::device_addr_t &hint);

//! Implementation guts
class b200_impl : public uhd::device
{
public:
    //structors
    b200_impl(const uhd::device_addr_t &, uhd::transport::usb_device_handle::sptr &handle);
    ~b200_impl(void);

    //the io interface
    uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &args);
    uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &args);
    bool recv_async_msg(uhd::async_metadata_t &, double);

    //! Check that the combination of stream args and tick rate are valid.
    //
    // Basically figures out the arguments for enforce_tick_rate_limits()
    // and calls said method. If arguments are invalid, throws a
    // uhd::value_error.
    void check_streamer_args(const uhd::stream_args_t &args, double tick_rate, const std::string &direction = "");

    static uhd::usrp::mboard_eeprom_t get_mb_eeprom(uhd::i2c_iface::sptr);

private:
    b200_product_t  _product;
    size_t          _revision;
    bool            _gpsdo_capable;

    //controllers
    b200_iface::sptr _iface;
    radio_ctrl_core_3000::sptr _local_ctrl;
    uhd::usrp::ad9361_ctrl::sptr _codec_ctrl;
    uhd::usrp::ad936x_manager::sptr _codec_mgr;
    b200_local_spi_core::sptr _spi_iface;
    boost::shared_ptr<uhd::usrp::adf4001_ctrl> _adf4001_iface;
    uhd::gps_ctrl::sptr _gps;

    //transports
    uhd::transport::zero_copy_if::sptr _data_transport;
    uhd::transport::zero_copy_if::sptr _ctrl_transport;
    uhd::usrp::recv_packet_demuxer_3000::sptr _demux;

    boost::weak_ptr<uhd::rx_streamer> _rx_streamer;
    boost::weak_ptr<uhd::tx_streamer> _tx_streamer;

    boost::mutex _transport_setup_mutex;

    //async ctrl + msgs
    uhd::msg_task::sptr _async_task;
    typedef uhd::transport::bounded_buffer<uhd::async_metadata_t> async_md_type;
    struct AsyncTaskData
    {
        boost::shared_ptr<async_md_type> async_md;
        boost::weak_ptr<radio_ctrl_core_3000> local_ctrl;
        boost::weak_ptr<radio_ctrl_core_3000> radio_ctrl[2];
        b200_uart::sptr gpsdo_uart;
    };
    boost::shared_ptr<AsyncTaskData> _async_task_data;
    boost::optional<uhd::msg_task::msg_type_t> handle_async_task(uhd::transport::zero_copy_if::sptr, boost::shared_ptr<AsyncTaskData>);

    void register_loopback_self_test(uhd::wb_iface::sptr iface);
    void set_mb_eeprom(const uhd::usrp::mboard_eeprom_t &);
    void check_fw_compat(void);
    void check_fpga_compat(void);
    uhd::usrp::subdev_spec_t coerce_subdev_spec(const uhd::usrp::subdev_spec_t &);
    void update_subdev_spec(const std::string &tx_rx, const uhd::usrp::subdev_spec_t &);
    void update_time_source(const std::string &);
    void set_time(const uhd::time_spec_t&);
    void sync_times(void);
    void update_clock_source(const std::string &);
    void update_bandsel(const std::string& which, double freq);
    void update_antenna_sel(const size_t which, const std::string &ant);
    uhd::sensor_value_t get_ref_locked(void);
    uhd::sensor_value_t get_fe_pll_locked(const bool is_tx);

    //perifs in the radio core
    struct radio_perifs_t
    {
        radio_ctrl_core_3000::sptr ctrl;
        uhd::usrp::gpio_atr::gpio_atr_3000::sptr atr;
        uhd::usrp::gpio_atr::gpio_atr_3000::sptr fp_gpio;
        time_core_3000::sptr time64;
        rx_vita_core_3000::sptr framer;
        rx_dsp_core_3000::sptr ddc;
        tx_vita_core_3000::sptr deframer;
        tx_dsp_core_3000::sptr duc;
        boost::weak_ptr<uhd::rx_streamer> rx_streamer;
        boost::weak_ptr<uhd::tx_streamer> tx_streamer;
        bool ant_rx2;
    };
    std::vector<radio_perifs_t> _radio_perifs;

    //mapping of AD936x frontends (FE1 and FE2) to radio perif index (0 and 1)
    //FE1 corresponds to the ports labeled "RF B" on the B200/B210
    //FE2 corresponds to the ports labeled "RF A" on the B200/B210
    //the mapping is product and revision specific
    size_t _fe1;
    size_t _fe2;

    /*! \brief Setup the DSP chain for one radio front-end.
     *
     */
    void setup_radio(const size_t radio_index);
    void handle_overflow(const size_t radio_index);

    struct gpio_state {
        uint32_t  tx_bandsel_a, tx_bandsel_b, rx_bandsel_a, rx_bandsel_b, rx_bandsel_c, mimo, ref_sel, swap_atr;

        gpio_state() {
            tx_bandsel_a = 0;
            tx_bandsel_b = 0;
            rx_bandsel_a = 0;
            rx_bandsel_b = 0;
            rx_bandsel_c = 0;
            mimo = 0;
            ref_sel = 0;
            swap_atr = 0;
        }
    } _gpio_state;

    enum time_source_t {GPSDO=0,EXTERNAL=1,INTERNAL=2,NONE=3,UNKNOWN=4} _time_source;

    void update_gpio_state(void);

    void update_enables(void);
    void update_atrs(void);

    double _tick_rate;
    double get_tick_rate(void){return _tick_rate;}
    double set_tick_rate(const double rate);

    /*! \brief Choose a tick rate (master clock rate) that works well for the given sampling rate.
     *
     * This function will try and choose a master clock rate automatically.
     * See the function definition for details on the algorithm.
     *
     * The chosen tick rate is the largest multiple of two that is smaler
     * than the max tick rate.
     * The base rate is either given explicitly, or is the lcm() of the tx
     * and rx sampling rates. In that case, it reads the rates directly
     * from the property tree. It also tries to guess the number of channels
     * (for the max possible tick rate) by checking the available streamers.
     * This value, too, can explicitly be given.
     *
     * \param rate If this is given, it will be used as a minimum rate, or
     *             argument to lcm().
     * \param tree_dsp_path The sampling rate from this property tree path
     *                      will be ignored.
     * \param num_chans If given, specifies the number of channels.
     */
    void set_auto_tick_rate(
            const double rate=0,
            const uhd::fs_path &tree_dsp_path="",
            size_t num_chans=0
    );

    void update_tick_rate(const double);

    /*! Subscriber to the tick_rate property, updates DDCs after tick rate change.
     */
    void update_rx_dsp_tick_rate(const double, rx_dsp_core_3000::sptr, uhd::fs_path rx_dsp_path);

    /*! Subscriber to the tick_rate property, updates DUCs after tick rate change.
     */
    void update_tx_dsp_tick_rate(const double, tx_dsp_core_3000::sptr, uhd::fs_path tx_dsp_path);

    /*! Check if \p tick_rate works with \p chan_count channels.
     *
     * Throws a uhd::value_error if not.
     */
    void enforce_tick_rate_limits(size_t chan_count, double tick_rate, const std::string  &direction = "");
    void check_tick_rate_with_current_streamers(double rate);

    /*! Return the max number of channels on active rx_streamer or tx_streamer objects associated with this device.
     *
     * \param direction Set to "TX" to only check tx_streamers, "RX" to only check
     *                  rx_streamers. Any other value will check if \e any active
     *                  streamers are available.
     * \return Return the number of tx streamers (direction=="TX"), the number of rx
     *         streamers (direction=="RX") or the total number of streamers.
     */
    size_t max_chan_count(const std::string &direction="");

    //! Coercer, attached to the "rate/value" property on the rx dsps.
    double coerce_rx_samp_rate(rx_dsp_core_3000::sptr, size_t, const double);
    void update_rx_samp_rate(const size_t, const double);

    //! Coercer, attached to the "rate/value" property on the tx dsps.
    double coerce_tx_samp_rate(tx_dsp_core_3000::sptr, size_t, const double);
    void update_tx_samp_rate(const size_t, const double);
};

#endif /* INCLUDED_B200_IMPL_HPP */
