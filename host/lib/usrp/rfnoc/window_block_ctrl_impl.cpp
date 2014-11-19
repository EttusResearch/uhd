//
// Copyright 2014 Ettus Research LLC
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

#include <uhd/usrp/rfnoc/window_block_ctrl.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd::rfnoc;

class window_block_ctrl_impl : public window_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(window_block_ctrl),
        _item_type("sc16"), // We only support sc16 in this block
        _bpi(uhd::convert::get_bytes_per_item("sc16")),
        _window_len(DEFAULT_WINDOW_LEN)
    {
        _max_len = boost::uint32_t(user_reg_read64(RB_MAX_WINDOW_LEN));
        UHD_MSG(status) << "window_block::window_block() max_len ==" << _max_len << std::endl;
        UHD_ASSERT_THROW(_max_len);

        // Default to rectangular window
        std::vector<int> default_coeffs(_window_len, (((1 << 15)-1) << 16) + ((1 << 15)-1));
        set_window(default_coeffs);
    }

    void set_window(const std::vector<int> &coeffs)
    {
        if (coeffs.size() > _max_len) {
            throw uhd::value_error(str(
                boost::format("Window block: Too many window coefficients! Provided %d, window allows up to %d.\n")
                % coeffs.size() % _max_len
            ));
        }

        _window_len = coeffs.size();

        // Write coefficients via the load bus
        for (size_t i = 0; i < _window_len - 1; i++) {
            sr_write(AXIS_WINDOW_LOAD, boost::uint32_t(coeffs[i]));
        }
        // Assert tlast when sending the final coefficient (sorry, no joke here)
        sr_write(AXIS_WINDOW_LOAD_TLAST, boost::uint32_t(coeffs.back()));
        // Set the window length
        sr_write(SR_WINDOW_LEN, coeffs.size());

        // Set stream signatures
        stream_sig_t stream_sig(
                _item_type,
                _window_len, // Vector length equals window size
                _window_len * _bpi,
                false
        );
        // The stream signature is identical on input & output
        _tree->access<stream_sig_t>(_root_path / "input_sig/0").set(stream_sig);
        _tree->access<stream_sig_t>(_root_path / "output_sig/0").set(stream_sig);
    }

    //! Returns the maximum window length of this block.
    size_t get_max_len() const
    {
        return _max_len;
    }

    size_t get_window_len() const
    {
        return _window_len;
    }

    bool set_input_signature(const stream_sig_t &stream_sig, size_t port=0)
    {
        UHD_MSG(status) << "window_block::set_input_signature()" << std::endl;
        UHD_ASSERT_THROW(port == 0);
        if (stream_sig.get_item_type() != _item_type
            //or (stream_sig.packet_size != 0 and stream_sig.packet_size != _window_len * _bpi) FIXME put this back in
            or (stream_sig.vlen != 0 and stream_sig.vlen != _window_len)) {
            UHD_MSG(status) << "not valid." << std::endl;
            return false;
        }

        return true;
    }

    bool set_output_signature(const stream_sig_t &stream_sig, size_t port=0)
    {
        UHD_ASSERT_THROW(port == 0);
        if (stream_sig.get_item_type() != _item_type
            //or (stream_sig.packet_size != 0 and stream_sig.packet_size != _window_len * _bpi) FIXME put this back in
            or (stream_sig.vlen != 0 and stream_sig.vlen != _window_len)) {
            return false;
        }

        return true;
    }

protected:
    void _post_args_hook()
    {
        if (_args.has_key("spp")) {
            size_t spp = _args.cast<size_t>("spp", _window_len);
            if (spp != _window_len) {
                throw uhd::value_error("In the window block, spp cannot differ from the window length.");
            }
        }
    }

    void _init_rx(uhd::stream_args_t &args)
    {
        UHD_MSG(status) << "window_block::init_rx()" << std::endl;
        if (args.otw_format != "sc16") {
            throw uhd::value_error("FFT only supports otw_format sc16");
        }
        // Check if the downstream block wants a specific spp.
        // If it's not the window length, throw. Otherwise, tell the upstream
        // block about what spp we need.
        if (not args.args.has_key("spp")) {
            args.args["spp"] = str(boost::format("%d") % _window_len);
        } else {
            size_t req_spp = args.args.cast<size_t>("spp", _window_len);
            if (req_spp != _window_len) {
                throw uhd::value_error("In the window block, spp cannot differ from the window length (upstream block requested other spp value)");
            }
        }
    }

    void _init_tx(uhd::stream_args_t &args)
    {
        UHD_MSG(status) << "window_block::init_tx()" << std::endl;
        if (args.otw_format != "sc16") {
            throw uhd::value_error("FFT only supports otw_format sc16");
        }
        // Check if the upstream block wants a specific spp.
        // If it's not the window length, throw. Otherwise, tell the downstream
        // block about what spp we need.
        if (not args.args.has_key("spp")) {
            args.args["spp"] = str(boost::format("%d") % _window_len);
        } else {
            size_t req_spp = args.args.cast<size_t>("spp", _window_len);
            if (req_spp != _window_len) {
                throw uhd::value_error("In the FFT block, spp cannot differ from the window length (downstream block requested other spp value)");
            }
        }
    }

private:
    const std::string _item_type;
    const size_t _bpi;
    size_t _max_len;
    size_t _window_len;
};

UHD_RFNOC_BLOCK_REGISTER(window_block_ctrl, "Window");
