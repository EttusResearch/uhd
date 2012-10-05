//
// Copyright 2011-2012 Ettus Research LLC
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

#include "rx_dsp_core_200.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/algorithm.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/thread/thread.hpp> //thread sleep
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <algorithm>
#include <cmath>

#define REG_DSP_RX_FREQ       _dsp_base + 0
#define REG_DSP_RX_SCALE_IQ   _dsp_base + 4
#define REG_DSP_RX_DECIM      _dsp_base + 8
#define REG_DSP_RX_MUX        _dsp_base + 12

#define FLAG_DSP_RX_MUX_SWAP_IQ   (1 << 0)
#define FLAG_DSP_RX_MUX_REAL_MODE (1 << 1)

#define REG_RX_CTRL_STREAM_CMD     _ctrl_base + 0
#define REG_RX_CTRL_TIME_HI        _ctrl_base + 4
#define REG_RX_CTRL_TIME_LO        _ctrl_base + 8
#define REG_RX_CTRL_FORMAT         _ctrl_base + 12
#define REG_RX_CTRL_VRT_HDR        _ctrl_base + 16
#define REG_RX_CTRL_VRT_SID        _ctrl_base + 20
#define REG_RX_CTRL_VRT_TLR        _ctrl_base + 24
#define REG_RX_CTRL_NSAMPS_PP      _ctrl_base + 28
#define REG_RX_CTRL_NCHANNELS      _ctrl_base + 32

template <class T> T ceil_log2(T num){
    return std::ceil(std::log(num)/std::log(T(2)));
}

using namespace uhd;

class rx_dsp_core_200_impl : public rx_dsp_core_200{
public:
    rx_dsp_core_200_impl(
        wb_iface::sptr iface,
        const size_t dsp_base, const size_t ctrl_base,
        const boost::uint32_t sid, const bool lingering_packet
    ):
        _iface(iface), _dsp_base(dsp_base), _ctrl_base(ctrl_base), _sid(sid)
    {
        //init to something so update method has reasonable defaults
        _scaling_adjustment = 1.0;
        _dsp_extra_scaling = 1.0;

        //This is a hack/fix for the lingering packet problem.
        //The caller should also flush the recv transports
        if (lingering_packet){
            stream_cmd_t stream_cmd(stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
            stream_cmd.num_samps = 1;
            issue_stream_command(stream_cmd);
            boost::this_thread::sleep(boost::posix_time::milliseconds(10)); //lets lingering pkt propagate
        }

        this->clear();
    }

    ~rx_dsp_core_200_impl(void)
    {
        UHD_SAFE_CALL
        (
            //shutdown any possible streaming
            this->clear();
        )
    }

    void clear(void){
        _iface->poke32(REG_RX_CTRL_NCHANNELS, 0); //also reset
        _iface->poke32(REG_RX_CTRL_VRT_HDR, 0
            | (0x1 << 28) //if data with stream id
            | (0x1 << 26) //has trailer
            | (0x1 << 20) //fractional time sample count
        );
        _iface->poke32(REG_RX_CTRL_VRT_SID, _sid);
        _iface->poke32(REG_RX_CTRL_VRT_TLR, 0);
    }

    void set_nsamps_per_packet(const size_t nsamps){
        _iface->poke32(REG_RX_CTRL_NSAMPS_PP, nsamps);
    }

    void issue_stream_command(const stream_cmd_t &stream_cmd){
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
        boost::uint32_t cmd_word = 0;
        cmd_word |= boost::uint32_t((stream_cmd.stream_now)? 1 : 0) << 31;
        cmd_word |= boost::uint32_t((inst_chain)?            1 : 0) << 30;
        cmd_word |= boost::uint32_t((inst_reload)?           1 : 0) << 29;
        cmd_word |= boost::uint32_t((inst_stop)?             1 : 0) << 28;
        cmd_word |= (inst_samps)? stream_cmd.num_samps : ((inst_stop)? 0 : 1);

        //issue the stream command
        _iface->poke32(REG_RX_CTRL_STREAM_CMD, cmd_word);
        const boost::uint64_t ticks = (stream_cmd.stream_now)? 0 : stream_cmd.time_spec.to_ticks(_tick_rate);
        _iface->poke32(REG_RX_CTRL_TIME_HI, boost::uint32_t(ticks >> 32));
        _iface->poke32(REG_RX_CTRL_TIME_LO, boost::uint32_t(ticks >> 0)); //latches the command
    }

    void set_mux(const std::string &mode, const bool fe_swapped){
        static const uhd::dict<std::string, boost::uint32_t> mode_to_mux = boost::assign::map_list_of
            ("IQ", 0)
            ("QI", FLAG_DSP_RX_MUX_SWAP_IQ)
            ("I", FLAG_DSP_RX_MUX_REAL_MODE)
            ("Q", FLAG_DSP_RX_MUX_SWAP_IQ | FLAG_DSP_RX_MUX_REAL_MODE)
        ;
        _iface->poke32(REG_DSP_RX_MUX, mode_to_mux[mode] ^ (fe_swapped? FLAG_DSP_RX_MUX_SWAP_IQ : 0));
    }

    void set_tick_rate(const double rate){
        _tick_rate = rate;
    }

    void set_link_rate(const double rate){
        //_link_rate = rate/sizeof(boost::uint32_t); //in samps/s
        _link_rate = rate/sizeof(boost::uint16_t); //in samps/s (allows for 8sc)
    }

    uhd::meta_range_t get_host_rates(void){
        meta_range_t range;
        for (int rate = 512; rate > 256; rate -= 4){
            range.push_back(range_t(_tick_rate/rate));
        }
        for (int rate = 256; rate > 128; rate -= 2){
            range.push_back(range_t(_tick_rate/rate));
        }
        for (int rate = 128; rate >= int(std::ceil(_tick_rate/_link_rate)); rate -= 1){
            range.push_back(range_t(_tick_rate/rate));
        }
        return range;
    }

    double set_host_rate(const double rate){
        const size_t decim_rate = boost::math::iround(_tick_rate/this->get_host_rates().clip(rate, true));
        size_t decim = decim_rate;

        //determine which half-band filters are activated
        int hb0 = 0, hb1 = 0;
        if (decim % 2 == 0){
            hb0 = 1;
            decim /= 2;
        }
        if (decim % 2 == 0){
            hb1 = 1;
            decim /= 2;
        }

        _iface->poke32(REG_DSP_RX_DECIM, (hb1 << 9) | (hb0 << 8) | (decim & 0xff));

        if (decim > 1 and hb0 == 0 and hb1 == 0)
        {
            UHD_MSG(warning) << boost::format(
                "The requested decimation is odd; the user should expect CIC rolloff.\n"
                "Select an even decimation to ensure that a halfband filter is enabled.\n"
                "decimation = dsp_rate/samp_rate -> %d = (%f MHz)/(%f MHz)\n"
            ) % decim_rate % (_tick_rate/1e6) % (rate/1e6);
        }

        // Calculate CIC decimation (i.e., without halfband decimators)
        // Calculate closest multiplier constant to reverse gain absent scale multipliers
        const double rate_pow = std::pow(double(decim & 0xff), 4);
        _scaling_adjustment = std::pow(2, ceil_log2(rate_pow))/(1.65*rate_pow);
        this->update_scalar();

        return _tick_rate/decim_rate;
    }

    void update_scalar(void){
        const double factor = 1.0 + std::max(ceil_log2(_scaling_adjustment), 0.0);
        const double target_scalar = (1 << 17)*_scaling_adjustment/_dsp_extra_scaling/factor;
        const boost::int32_t actual_scalar = boost::math::iround(target_scalar);
        _fxpt_scalar_correction = target_scalar/actual_scalar*factor; //should be small
        _iface->poke32(REG_DSP_RX_SCALE_IQ, actual_scalar);
    }

    double get_scaling_adjustment(void){
        return _fxpt_scalar_correction*_host_extra_scaling/32767.;
    }

    double set_freq(const double freq_){
        //correct for outside of rate (wrap around)
        double freq = std::fmod(freq_, _tick_rate);
        if (std::abs(freq) > _tick_rate/2.0)
            freq -= boost::math::sign(freq)*_tick_rate;

        //calculate the freq register word (signed)
        UHD_ASSERT_THROW(std::abs(freq) <= _tick_rate/2.0);
        static const double scale_factor = std::pow(2.0, 32);
        const boost::int32_t freq_word = boost::int32_t(boost::math::round((freq / _tick_rate) * scale_factor));

        //update the actual frequency
        const double actual_freq = (double(freq_word) / scale_factor) * _tick_rate;

        _iface->poke32(REG_DSP_RX_FREQ, boost::uint32_t(freq_word));

        return actual_freq;
    }

    uhd::meta_range_t get_freq_range(void){
        return uhd::meta_range_t(-_tick_rate/2, +_tick_rate/2, _tick_rate/std::pow(2.0, 32));
    }

    void handle_overflow(void){
        if (_continuous_streaming) issue_stream_command(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    }

    void setup(const uhd::stream_args_t &stream_args){
        if (not stream_args.args.has_key("noclear")) this->clear();

        unsigned format_word = 0;
        if (stream_args.otw_format == "sc16"){
            format_word = 0;
            _dsp_extra_scaling = 1.0;
            _host_extra_scaling = 1.0;
        }
        else if (stream_args.otw_format == "sc8"){
            format_word = (1 << 0);
            double peak = stream_args.args.cast<double>("peak", 1.0);
            peak = std::max(peak, 1.0/256);
            _host_extra_scaling = peak*256;
            _dsp_extra_scaling = peak*256;
        }
        else throw uhd::value_error("USRP RX cannot handle requested wire format: " + stream_args.otw_format);

        _host_extra_scaling *= stream_args.args.cast<double>("fullscale", 1.0);

        this->update_scalar();

        _iface->poke32(REG_RX_CTRL_FORMAT, format_word);
    }

private:
    wb_iface::sptr _iface;
    const size_t _dsp_base, _ctrl_base;
    double _tick_rate, _link_rate;
    bool _continuous_streaming;
    double _scaling_adjustment, _dsp_extra_scaling, _host_extra_scaling, _fxpt_scalar_correction;
    const boost::uint32_t _sid;
};

rx_dsp_core_200::sptr rx_dsp_core_200::make(wb_iface::sptr iface, const size_t dsp_base, const size_t ctrl_base, const boost::uint32_t sid, const bool lingering_packet){
    return sptr(new rx_dsp_core_200_impl(iface, dsp_base, ctrl_base, sid, lingering_packet));
}
