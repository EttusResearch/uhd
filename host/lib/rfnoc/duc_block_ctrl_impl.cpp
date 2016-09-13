//
// Copyright 2016 Ettus Research
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

#include "dsp_core_utils.hpp"
#include <uhd/rfnoc/duc_block_ctrl.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/convert.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/math/special_functions/round.hpp>
#include <cmath>

using namespace uhd::rfnoc;

// TODO move this to a central location
template <class T> T ceil_log2(T num){
    return std::ceil(std::log(num)/std::log(T(2)));
}

// TODO remove this once we have actual lambdas
static double lambda_forward_prop(uhd::property_tree::sptr tree, uhd::fs_path prop, double value)
{
    return tree->access<double>(prop).set(value).get();
}

static double lambda_forward_prop(uhd::property_tree::sptr tree, uhd::fs_path prop)
{
    return tree->access<double>(prop).get();
}

class duc_block_ctrl_impl : public duc_block_ctrl
{
public:
    static const size_t NUM_HALFBANDS = 2;
    static const size_t CIC_MAX_INTERP = 128;

    UHD_RFNOC_BLOCK_CONSTRUCTOR(duc_block_ctrl)
    {
        // Argument/prop tree hooks
        for (size_t chan = 0; chan < get_input_ports().size(); chan++) {
            double default_freq = get_arg<double>("freq", chan);
            _tree->access<double>(get_arg_path("freq/value", chan))
                .set_coercer(boost::bind(&duc_block_ctrl_impl::set_freq, this, _1, chan))
                .set(default_freq);
            ;
            double default_input_rate = get_arg<double>("input_rate", chan);
            _tree->access<double>(get_arg_path("input_rate/value", chan))
                .set_coercer(boost::bind(&duc_block_ctrl_impl::set_input_rate, this, _1, chan))
                .set(default_input_rate)
            ;
            _tree->access<double>(get_arg_path("output_rate/value", chan))
                .add_coerced_subscriber(boost::bind(&duc_block_ctrl_impl::set_output_rate, this, _1, chan))
            ;

            // Legacy properties (for backward compat w/ multi_usrp)
            const uhd::fs_path dsp_base_path = _root_path / "legacy_api" / chan;
            // Legacy properties
            _tree->create<double>(dsp_base_path / "rate/value")
                .set_coercer(boost::bind(&lambda_forward_prop, _tree, get_arg_path("input_rate/value", chan), _1))
                .set_publisher(boost::bind(&lambda_forward_prop, _tree, get_arg_path("input_rate/value", chan)))
            ;
            _tree->create<uhd::meta_range_t>(dsp_base_path / "rate/range")
                .set_publisher(boost::bind(&duc_block_ctrl_impl::get_input_rates, this))
            ;
            _tree->create<double>(dsp_base_path / "freq/value")
                .set_coercer(boost::bind(&lambda_forward_prop, _tree, get_arg_path("freq/value", chan), _1))
                .set_publisher(boost::bind(&lambda_forward_prop, _tree, get_arg_path("freq/value", chan)))
            ;
            _tree->create<uhd::meta_range_t>(dsp_base_path / "freq/range")
                .set_publisher(boost::bind(&duc_block_ctrl_impl::get_freq_range, this))
            ;
            _tree->access<uhd::time_spec_t>("time/cmd")
                .add_coerced_subscriber(boost::bind(&block_ctrl_base::set_command_time, this, _1, chan))
            ;
            if (_tree->exists("tick_rate")) {
                const double tick_rate = _tree->access<double>("tick_rate").get();
                set_command_tick_rate(tick_rate, chan);
                _tree->access<double>("tick_rate")
                    .add_coerced_subscriber(boost::bind(&block_ctrl_base::set_command_tick_rate, this, _1, chan))
                ;
            }

            // Rate 1:1 by default
            sr_write("N", 1, chan);
            sr_write("M", 1, chan);
            sr_write("CONFIG", 1, chan); // Enable clear EOB
        }
    } // end ctor
    virtual ~duc_block_ctrl_impl() {};

    double get_input_scale_factor(size_t port=ANY_PORT)
    {
        port = (port == ANY_PORT) ? 0 : port;
        if (not (_tx_streamer_active.count(port) and _tx_streamer_active.at(port))) {
            return SCALE_UNDEFINED;
        }
        return get_arg<double>("scalar_correction", port);
    }

    double get_input_samp_rate(size_t port=ANY_PORT)
    {
        port = (port == ANY_PORT) ? 0 : port;
        if (not (_tx_streamer_active.count(port) and _tx_streamer_active.at(port))) {
            return RATE_UNDEFINED;
        }
        return get_arg<double>("input_rate", port);
    }

    double get_output_samp_rate(size_t port=ANY_PORT)
    {
        port = (port == ANY_PORT) ? 0 : port;
        if (not (_tx_streamer_active.count(port) and _tx_streamer_active.at(port))) {
            return RATE_UNDEFINED;
        }
        return get_arg<double>("output_rate", port == ANY_PORT ? 0 : port);
    }

    void issue_stream_cmd(
            const uhd::stream_cmd_t &stream_cmd_,
            const size_t chan
    ) {
        UHD_RFNOC_BLOCK_TRACE() << "duc_block_ctrl_base::issue_stream_cmd()" << std::endl;

        uhd::stream_cmd_t stream_cmd = stream_cmd_;
        if (stream_cmd.stream_mode == uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE or
            stream_cmd.stream_mode == uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE) {
            size_t interpolation = get_arg<double>("output_rate", chan) / get_arg<double>("input_rate", chan);
            stream_cmd.num_samps *= interpolation;
        }

        BOOST_FOREACH(const node_ctrl_base::node_map_pair_t upstream_node, list_upstream_nodes()) {
            source_node_ctrl::sptr this_upstream_block_ctrl =
                boost::dynamic_pointer_cast<source_node_ctrl>(upstream_node.second.lock());
            this_upstream_block_ctrl->issue_stream_cmd(stream_cmd, chan);
        }
    }

private:

    //! Set the CORDIC frequency shift the signal to \p requested_freq
    double set_freq(const double requested_freq, const size_t chan)
    {
        const double output_rate = get_arg<double>("output_rate");
        double actual_freq;
        int32_t freq_word;
        get_freq_and_freq_word(requested_freq, output_rate, actual_freq, freq_word);
        // Xilinx CORDIC uses a different format for the phase increment, hence the divide-by-four:
        sr_write("CORDIC_FREQ", uint32_t(freq_word/4), chan);
        return actual_freq;
    }

    //! Return a range of valid frequencies the CORDIC can tune to
    uhd::meta_range_t get_freq_range(void)
    {
        const double output_rate = get_arg<double>("output_rate");
        return uhd::meta_range_t(
                -output_rate/2,
                +output_rate/2,
                output_rate/std::pow(2.0, 32)
        );
    }

    uhd::meta_range_t get_input_rates(void)
    {
        uhd::meta_range_t range;
        const double output_rate = get_arg<double>("output_rate");
        for (int rate = 512; rate > 256; rate -= 4){
            range.push_back(uhd::range_t(output_rate/rate));
        }
        for (int rate = 256; rate > 128; rate -= 2){
            range.push_back(uhd::range_t(output_rate/rate));
        }
        for (int rate = 128; rate >= 1; rate -= 1){
            range.push_back(uhd::range_t(output_rate/rate));
        }
        return range;
    }

    double set_input_rate(const int requested_rate, const size_t chan)
    {
        const double output_rate = get_arg<double>("output_rate", chan);
        const size_t interp_rate = boost::math::iround(output_rate/get_input_rates().clip(requested_rate, true));
        size_t interp = interp_rate;

        uint32_t hb_enable = 0;
        while ((interp % 2 == 0) and hb_enable < NUM_HALFBANDS) {
            hb_enable++;
            interp /= 2;
        }
        UHD_ASSERT_THROW(hb_enable <= NUM_HALFBANDS);
        UHD_ASSERT_THROW(interp > 0 and interp <= CIC_MAX_INTERP);
        // hacky hack: Unlike the DUC, the DUC actually simply has 2
        // flags to enable either halfband.
        uint32_t hb_enable_word = hb_enable;
        if (hb_enable == 2) {
            hb_enable_word = 3;
        }
        hb_enable_word <<= 8;
        // What we can't cover with halfbands, we do with the CIC
        sr_write("INTERP_WORD", hb_enable_word | (interp & 0xff), chan);

        // Rate change = M/N
        sr_write("N", 1, chan);
        sr_write("M", std::pow(2.0, double(hb_enable)) * (interp & 0xff), chan);

        if (interp > 1 and hb_enable == 0) {
            UHD_MSG(warning) << boost::format(
                "The requested interpolation is odd; the user should expect passband CIC rolloff.\n"
                "Select an even interpolation to ensure that a halfband filter is enabled.\n"
                "interpolation = dsp_rate/samp_rate -> %d = (%f MHz)/(%f MHz)\n"
            ) % interp_rate % (output_rate/1e6) % (requested_rate/1e6);
        }

        // Calculate algorithmic gain of CIC for a given interpolation
        // For Ettus CIC R=interp, M=1, N=4. Gain = (R * M) ^ (N - 1)
        const int CIC_N = 4;
        const double rate_pow = std::pow(double(interp & 0xff), CIC_N - 1);

        // Experimentally determined value to scale the output to [-1, 1]
        // This must also encompass the CORDIC gain
        static const double CONSTANT_GAIN = 1.1644;

        const double scaling_adjustment = std::pow(2, ceil_log2(rate_pow))/(CONSTANT_GAIN*rate_pow);
        update_scalar(scaling_adjustment, chan);
        return output_rate/interp_rate;
    }

    //! Set frequency and interpolation again
    void set_output_rate(const double /* rate */, const size_t chan)
    {
        const double desired_freq = _tree->access<double>(get_arg_path("freq", chan) / "value").get_desired();
        set_arg<double>("freq", desired_freq, chan);
        const double desired_input_rate = _tree->access<double>(get_arg_path("input_rate", chan) / "value").get_desired();
        set_arg<double>("input_rate", desired_input_rate, chan);
    }

    // Calculate compensation gain values for algorithmic gain of CORDIC and CIC taking into account
    // gain compensation blocks already hardcoded in place in DUC (that provide simple 1/2^n gain compensation).
    // Further more factor in OTW format which adds further gain factor to weight output samples correctly.
    void update_scalar(const double scalar, const size_t chan)
    {
        const double target_scalar = (1 << 15) * scalar;
        const int32_t actual_scalar = boost::math::iround(target_scalar);
        // Calculate the error introduced by using integer representation for the scalar
        const double scalar_correction =
            actual_scalar / target_scalar * (double(1 << 15) - 1.0) // Rounding error, normalized to 1.0
            * get_arg<double>("fullscale"); // Scaling requested by host
        set_arg<double>("scalar_correction", scalar_correction, chan);
        // Write DUC with scaling correction for CIC and CORDIC that maximizes dynamic range in 32/16/12/8bits.
        sr_write("SCALE_IQ", actual_scalar, chan);
    }
};

UHD_RFNOC_BLOCK_REGISTER(duc_block_ctrl, "DUC");

