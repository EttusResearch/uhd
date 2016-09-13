//
// Copyright 2014-2016 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_IMPL_HPP

#include "rx_vita_core_3000.hpp"
#include "tx_vita_core_3000.hpp"
#include "time_core_3000.hpp"
#include "gpio_atr_3000.hpp"
#include <uhd/rfnoc/radio_ctrl.hpp>
#include <uhd/types/direction.hpp>
#include <boost/thread.hpp>

//! Shorthand for radio block constructor
#define UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR_DECL(CLASS_NAME) \
    CLASS_NAME##_impl(const make_args_t &make_args);

#define UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(CLASS_NAME) \
    CLASS_NAME##_impl::CLASS_NAME##_impl( \
        const make_args_t &make_args \
    ) : block_ctrl_base(make_args), radio_ctrl_impl()

namespace uhd {
    namespace rfnoc {

/*! \brief Provide access to a radio.
 *
 */
class radio_ctrl_impl : public radio_ctrl
{
public:
    /************************************************************************
     * Structors
     ***********************************************************************/
    radio_ctrl_impl();
    virtual ~radio_ctrl_impl() {};

    /************************************************************************
     * Public Radio API calls
     ***********************************************************************/
    virtual double set_rate(double rate);
    virtual void set_tx_antenna(const std::string &ant, const size_t chan);
    virtual void set_rx_antenna(const std::string &ant, const size_t chan);
    virtual double set_tx_frequency(const double freq, const size_t chan);
    virtual double set_rx_frequency(const double freq, const size_t chan);
    virtual double set_tx_gain(const double gain, const size_t chan);
    virtual double set_rx_gain(const double gain, const size_t chan);
    virtual void set_time_sync(const uhd::time_spec_t &time);

    virtual double get_rate() const;
    virtual std::string get_tx_antenna(const size_t chan) /* const */;
    virtual std::string get_rx_antenna(const size_t chan) /* const */;
    virtual double get_tx_frequency(const size_t) /* const */;
    virtual double get_rx_frequency(const size_t) /* const */;
    virtual double get_tx_gain(const size_t) /* const */;
    virtual double get_rx_gain(const size_t) /* const */;

    void set_time_now(const time_spec_t &time_spec);
    void set_time_next_pps(const time_spec_t &time_spec);
    time_spec_t get_time_now();
    time_spec_t get_time_last_pps();

    /***********************************************************************
     * Block control API calls
     **********************************************************************/
    void set_rx_streamer(bool active, const size_t port);
    void set_tx_streamer(bool active, const size_t port);

    void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd, const size_t port);

    virtual double get_input_samp_rate(size_t /* port */) { return get_rate(); }
    virtual double get_output_samp_rate(size_t /* port */) { return get_rate(); }
    double _get_tick_rate() { return get_rate(); }

    std::vector<size_t> get_active_rx_ports();
    bool in_continuous_streaming_mode(const size_t chan) { return _continuous_streaming.at(chan); }
    void rx_ctrl_clear_cmds(const size_t port) { sr_write(regs::RX_CTRL_CLEAR_CMDS, 0, port); }

protected: // TODO see what's protected and what's private
    void _register_loopback_self_test(size_t chan);

    /***********************************************************************
     * Registers
     **********************************************************************/
    struct regs {
        static inline boost::uint32_t sr_addr(const boost::uint32_t offset)
        {
            return offset * 4;
        }

        static const uint32_t BASE       = 128;

        // defined in radio_core_regs.vh
        static const uint32_t TIME                 = 128; // time hi - 128, time lo - 129, ctrl - 130
        static const uint32_t CLEAR_CMDS           = 131; // Any write to this reg clears the command FIFO
        static const uint32_t LOOPBACK             = 132;
        static const uint32_t TEST                 = 133;
        static const uint32_t CODEC_IDLE           = 134;
        static const uint32_t TX_CTRL_ERROR_POLICY = 144;
        static const uint32_t RX_CTRL_CMD          = 152;
        static const uint32_t RX_CTRL_TIME_HI      = 153;
        static const uint32_t RX_CTRL_TIME_LO      = 154;
        static const uint32_t RX_CTRL_HALT         = 155;
        static const uint32_t RX_CTRL_MAXLEN       = 156;
        static const uint32_t RX_CTRL_CLEAR_CMDS   = 157;
        static const uint32_t MISC_OUTS            = 160;
        static const uint32_t DACSYNC              = 161;
        static const uint32_t SPI                  = 168;
        static const uint32_t LEDS                 = 176;
        static const uint32_t FP_GPIO              = 184;
        static const uint32_t GPIO                 = 192;
        // NOTE: Upper 32 registers (224-255) are reserved for the output settings bus for use with
        //       device specific front end control

        // frontend control: needs rethinking TODO
        //static const uint32_t TX_FRONT             = BASE + 96;
        //static const uint32_t RX_FRONT             = BASE + 112;
        //static const uint32_t READBACK             = BASE + 127;

        static const uint32_t RB_TIME_NOW        = 0;
        static const uint32_t RB_TIME_PPS        = 1;
        static const uint32_t RB_TEST            = 2;
        static const uint32_t RB_CODEC_READBACK  = 3;
        static const uint32_t RB_RADIO_NUM       = 4;
        static const uint32_t RB_MISC_IO         = 16;
        static const uint32_t RB_SPI             = 17;
        static const uint32_t RB_LEDS            = 18;
        static const uint32_t RB_DB_GPIO         = 19;
        static const uint32_t RB_FP_GPIO         = 20;
    };

    /***********************************************************************
     * Block control API calls
     **********************************************************************/
    void _update_spp(int spp);

    inline size_t _get_num_radios() const {
       return  std::max(_num_rx_channels, _num_tx_channels);
    }

    inline timed_wb_iface::sptr _get_ctrl(size_t radio_num) const {
        return _perifs.at(radio_num).ctrl;
    }

    inline bool _is_streamer_active(uhd::direction_t dir, const size_t chan) const {
        switch (dir) {
        case uhd::TX_DIRECTION:
            return _tx_streamer_active.at(chan);
        case uhd::RX_DIRECTION:
            return _rx_streamer_active.at(chan);
        case uhd::DX_DIRECTION:
            return _rx_streamer_active.at(chan) and _tx_streamer_active.at(chan);
        default:
            return false;
        }
    }

    virtual bool check_radio_config() { return true; };

    //! There is always only one time core per radio
    time_core_3000::sptr         _time64;

    boost::mutex _mutex;

private:
    /************************************************************************
     * Peripherals
     ***********************************************************************/
    //! Stores pointers to all streaming-related radio cores
    struct radio_perifs_t
    {
        timed_wb_iface::sptr     ctrl;
    };
    std::map<size_t, radio_perifs_t> _perifs;

    size_t _num_tx_channels;
    size_t _num_rx_channels;

    // Cached values
    double _tick_rate;
    std::map<size_t, std::string> _tx_antenna;
    std::map<size_t, std::string> _rx_antenna;
    std::map<size_t, double> _tx_freq;
    std::map<size_t, double> _rx_freq;
    std::map<size_t, double> _tx_gain;
    std::map<size_t, double> _rx_gain;

    std::vector<bool> _continuous_streaming;
}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:
