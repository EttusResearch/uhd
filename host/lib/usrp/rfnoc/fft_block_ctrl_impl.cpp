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

#include <uhd/usrp/rfnoc/fft_block_ctrl.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>

using namespace uhd::rfnoc;

class fft_block_ctrl_impl : public fft_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(fft_block_ctrl),
        _item_type("sc16"), // We only support sc16 in this block
        _bpi(uhd::convert::get_bytes_per_item("sc16")),
        _fft_size(DEFAULT_FFT_SIZE)
    {

        // TODO: Read the default initial FFT size from the block definition
        // TODO: Register the FFT size into the property tree

        // TODO: Remove this reset. Currently used as a workaround due to deal
        //       with the fact the FFT can receive packets that are not
        //       fft_size and can put the FFT core in a bad state that is only
        //       recoverable with a reset.
        sr_write(SR_FFT_RESET, 1);

        // This also sets the stream signatures:
        set_fft_size(_fft_size);
    }

    void set_fft_size(size_t fft_size)
    {
        //// 1. Sanity checks
        const size_t requested_fft_size = fft_size;
        // Check fft_size is within bounds
        if (fft_size < 16 or fft_size > 2048) {
            // TODO read this bounds from the prop tree (block def)
            throw uhd::value_error("FFT size must be a power of two and within [16, 2048]");
        }
        boost::uint32_t log2_fft_size = 0;
        // Calculate log2(fft_size) and make sure fft_size is a power of 2
        while ( (fft_size & 1) == 0 and (fft_size > 1) ) {
            fft_size >>= 1;
            log2_fft_size++;
        }
        if (fft_size != 1) {
            // Not a power of 2
            throw uhd::value_error("FFT size must be a power of two and within [16, 2048]");
        }

        //// 2. Update block
        // TODO FFT scaling set conservatively (1/N), need method to allow user to set
        sr_write(AXIS_CONFIG_BUS, (0x6AA << 9) + (0 << 8) + log2_fft_size);
        _fft_size = requested_fft_size;

        //// 3. Set stream signatures
        stream_sig_t stream_sig(
                _item_type,
                _fft_size, // Vector length equals FFT size
                _fft_size * _bpi,
                false
        );
        // The stream signature is identical on input & output
        _tree->access<stream_sig_t>(_root_path / "input_sig/0").set(stream_sig);
        _tree->access<stream_sig_t>(_root_path / "output_sig/0").set(stream_sig);
    } /* set_fft_size() */

    size_t get_fft_size() const
    {
        return _fft_size;
    }

    bool set_input_signature(const stream_sig_t &stream_sig, size_t port=0)
    {
        UHD_MSG(status) << "fft_block::set_input_signature()" << std::endl;
        UHD_ASSERT_THROW(port == 0);
        if (stream_sig.get_item_type() != _item_type
            //or (stream_sig.packet_size != 0 and stream_sig.packet_size != _fft_size * _bpi) FIXME put this back in
            or (stream_sig.vlen != 0 and stream_sig.vlen != _fft_size)) {
            UHD_MSG(status) << "not valid." << std::endl;
            return false;
        }

        return true;
    }

    bool set_output_signature(const stream_sig_t &stream_sig, size_t port=0)
    {
        UHD_ASSERT_THROW(port == 0);
        if (stream_sig.get_item_type() != _item_type
            //or (stream_sig.packet_size != 0 and stream_sig.packet_size != _fft_size * _bpi) FIXME put this back in
            or (stream_sig.vlen != 0 and stream_sig.vlen != _fft_size)) {
            return false;
        }

        return true;
    }

protected:
    void _post_args_hook()
    {
        UHD_MSG(status) << "[" << get_block_id() << "] _post_args_hook()" << std::endl;
        if (_args.has_key("fftsize")) {
            size_t req_fft_size = _args.cast<size_t>("fftsize", _fft_size);
            if (req_fft_size != _fft_size) {
                set_fft_size(req_fft_size);
            }
        }

        if (_args.has_key("spp")) {
            size_t spp = _args.cast<size_t>("spp", _fft_size);
            if (spp != _fft_size) {
                throw uhd::value_error("In the FFT block, spp cannot differ from the FFT size.");
            }
        }
    }

    void _init_rx(uhd::stream_args_t &args)
    {
        UHD_MSG(status) << "[" << get_block_id() << "] fft_block::_init_rx()" << std::endl;
        if (args.otw_format != "sc16") {
            throw uhd::value_error("FFT only supports otw_format sc16");
        }
        // Check if the downstream block wants a specific spp.
        // If it's not the FFT size, throw. Otherwise, tell the upstream
        // block about what spp we need.
        if (not args.args.has_key("spp")) {
            args.args["spp"] = str(boost::format("%d") % _fft_size);
        } else {
            size_t req_spp = args.args.cast<size_t>("spp", _fft_size);
            if (req_spp != _fft_size) {
                throw uhd::value_error("In the FFT block, spp cannot differ from the FFT size (downstream block requested other spp value)");
            }
        }
    }

    void _init_tx(uhd::stream_args_t &args)
    {
        UHD_MSG(status) << "[" << get_block_id() << "] fft_block::_init_tx()" << std::endl;
        if (args.otw_format != "sc16") {
            throw uhd::value_error("FFT only supports otw_format sc16");
        }
        // Check if the upstream block wants a specific spp.
        // If it's not the FFT size, throw. Otherwise, tell the downstream
        // block about what spp we need.
        if (not args.args.has_key("spp")) {
            args.args["spp"] = str(boost::format("%d") % _fft_size);
        } else {
            size_t req_spp = args.args.cast<size_t>("spp", _fft_size);
            if (req_spp != _fft_size) {
                throw uhd::value_error("In the FFT block, spp cannot differ from the FFT size (downstream block requested other spp value)");
            }
        }
    }

private:
    const std::string _item_type;
    //! Bytes per item (bytes per sample)
    const size_t _bpi;
    size_t _fft_size;
};

UHD_RFNOC_BLOCK_REGISTER(fft_block_ctrl, "FFT");
