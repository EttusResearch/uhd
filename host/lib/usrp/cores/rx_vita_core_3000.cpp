//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhdlib/usrp/cores/rx_vita_core_3000.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/date_time.hpp>
#include <thread>
#include <chrono>

#define REG_FRAMER_MAXLEN    _base + 4*4 + 0
#define REG_FRAMER_SID       _base + 4*4 + 4

#define REG_CTRL_CMD           _base + 0
#define REG_CTRL_TIME_HI       _base + 4
#define REG_CTRL_TIME_LO       _base + 8

#define REG_FC_WINDOW       _base + 6*4 + 0
#define REG_FC_ENABLE       _base + 6*4 + 4

using namespace uhd;

rx_vita_core_3000::~rx_vita_core_3000(void){
    /* NOP */
}

struct rx_vita_core_3000_impl : rx_vita_core_3000
{
    rx_vita_core_3000_impl(
        wb_iface::sptr iface,
        const size_t base
    ):
        _iface(iface),
        _base(base),
        _continuous_streaming(false),
        _is_setup(false)
    {
        this->set_tick_rate(1); //init to non zero
        this->set_nsamps_per_packet(100); //init to non zero
        this->clear();
    }

    ~rx_vita_core_3000_impl(void)
    {
        UHD_SAFE_CALL
        (
            this->clear();
        )
    }

    void configure_flow_control(const size_t window_size)
    {
        // The window needs to be disabled in the case where this object is
        // uncleanly destroyed and the FC window is left enabled
        _iface->poke32(REG_FC_ENABLE, 0);

        // Sleep for a large amount of time to allow the source flow control
        // module in the FPGA to flush all the packets buffered upstream.
        // At 1 ms * 200 MHz = 200k cycles, 8 bytes * 200k cycles = 1.6 MB
        // of flushed data, when the typical amount of data buffered
        // is on the order of kilobytes
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        _iface->poke32(REG_FC_WINDOW, window_size-1);
        _iface->poke32(REG_FC_ENABLE, window_size?1:0);
    }

    void clear(void)
    {
        // FC should never be disabled, this will actually become
        // impossible in the future
        //this->configure_flow_control(0); //disable fc
    }

    void set_nsamps_per_packet(const size_t nsamps)
    {
        _iface->poke32(REG_FRAMER_MAXLEN, nsamps);
    }

    void issue_stream_command(const uhd::stream_cmd_t &stream_cmd)
    {
        if (not _is_setup)
        {
            //UHD_LOGGER_WARNING("CORES") << "rx vita core 3000 issue stream command - not setup yet!";
            return;
        }
        UHD_ASSERT_THROW(stream_cmd.num_samps <= 0x0fffffff);
        _continuous_streaming = stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS;

        //setup the mode to instruction flags
        typedef boost::tuple<bool, bool, bool, bool> inst_t;
        static const uhd::dict<stream_cmd_t::stream_mode_t, inst_t> mode_to_inst = boost::assign::map_list_of
                                                                //reload, chain, samps, stop
            (stream_cmd_t::STREAM_MODE_START_CONTINUOUS,   inst_t(true,  true,  false, false))
            (stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS,    inst_t(false, false, false, true))
            (stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE, inst_t(false, false, true,  false))
            (stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE, inst_t(false, true,  true,  false))
        ;

        //setup the instruction flag values
        bool inst_reload, inst_chain, inst_samps, inst_stop;
        boost::tie(inst_reload, inst_chain, inst_samps, inst_stop) = mode_to_inst[stream_cmd.stream_mode];

        //calculate the word from flags and length
        uint32_t cmd_word = 0;
        cmd_word |= uint32_t((stream_cmd.stream_now)? 1 : 0) << 31;
        cmd_word |= uint32_t((inst_chain)?            1 : 0) << 30;
        cmd_word |= uint32_t((inst_reload)?           1 : 0) << 29;
        cmd_word |= uint32_t((inst_stop)?             1 : 0) << 28;
        cmd_word |= (inst_samps)? stream_cmd.num_samps : ((inst_stop)? 0 : 1);

        //issue the stream command
        _iface->poke32(REG_CTRL_CMD, cmd_word);
        const uint64_t ticks = (stream_cmd.stream_now)? 0 : stream_cmd.time_spec.to_ticks(_tick_rate);
        _iface->poke32(REG_CTRL_TIME_HI, uint32_t(ticks >> 32));
        _iface->poke32(REG_CTRL_TIME_LO, uint32_t(ticks >> 0)); //latches the command
    }

    void set_tick_rate(const double rate)
    {
        _tick_rate = rate;
    }

    void set_sid(const uint32_t sid)
    {
        _iface->poke32(REG_FRAMER_SID, sid);
    }

    void handle_overflow(void)
    {
        if (_continuous_streaming) this->issue_stream_command(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    }

    void setup(const uhd::stream_args_t &)
    {
        _is_setup = true;
    }

    bool in_continuous_streaming_mode(void)
    {
        return _continuous_streaming;
    }

    wb_iface::sptr _iface;
    const size_t _base;
    double _tick_rate;
    bool _continuous_streaming;
    bool _is_setup;
};

rx_vita_core_3000::sptr rx_vita_core_3000::make(
    wb_iface::sptr iface,
    const size_t base
)
{
    return rx_vita_core_3000::sptr(new rx_vita_core_3000_impl(iface, base));
}
