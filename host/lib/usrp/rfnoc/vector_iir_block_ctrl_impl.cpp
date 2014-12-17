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

#include <uhd/usrp/rfnoc/vector_iir_block_ctrl.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>
#include <cmath>
#include <boost/math/special_functions/round.hpp>

using namespace uhd::rfnoc;

class vector_iir_block_ctrl_impl : public vector_iir_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(vector_iir_block_ctrl),
        _item_type("sc16"), // We only support sc16 in this block
        _bpi(uhd::convert::get_bytes_per_item("sc16")),
        _vector_len(DEFAULT_VECTOR_LEN),
        _alpha(DEFAULT_ALPHA),
        _beta(DEFAULT_BETA)
    {

        // TODO: Read the default vector length, beta, and alpha from the block definition
        // TODO: Register the vector length, beta, and alpha into the property tree

        // This also sets the stream signatures
        set_vector_len(_vector_len);
        set_alpha(_alpha);
        set_beta(_beta);
    }

    void set_vector_len(size_t vector_len)
    {
        //// 1. Sanity check
        const size_t requested_vector_len = vector_len;
        // Check vector length is within bounds
        if (vector_len < 1 or vector_len > 2048) {
            // TODO read this bounds from the prop tree (block def)
            throw uhd::value_error("Vector IIR length must be within [1, 2048]");
        }

        //// 2. Update block
        // TODO FFT scaling set conservatively (1/N), need method to allow user to set
        sr_write(SR_VECTOR_LEN, requested_vector_len);
        _vector_len = vector_len;

        //// 3. Set stream signatures
        stream_sig_t stream_sig(
                _item_type,
                _vector_len,
                _vector_len * _bpi,
                false
        );
        // The stream signature is identical on input & output
        _tree->access<stream_sig_t>(_root_path / "input_sig/0").set(stream_sig);
        _tree->access<stream_sig_t>(_root_path / "output_sig/0").set(stream_sig);
    } /* set_vector_len() */

    size_t get_vector_len() const
    {
        return _vector_len;
    }

    void set_alpha(double alpha)
    {
        //// 1. Sanity check
        const double requested_alpha = alpha;
        // Check vector length is within bounds
        if (alpha > 1.0 or alpha < -1.0) {
            // TODO read this bounds from the prop tree (block def)
            throw uhd::value_error("Vector IIR Alpha constant must be within [-1.0, 1.0]");
        }

        //// 2. Update block
        const boost::int32_t requested_alpha_q1_31 = boost::math::iround(requested_alpha*pow(2.0,31.0));
        // sr_write expects unsigned int data
        sr_write(SR_ALPHA, static_cast<boost::uint32_t>(requested_alpha_q1_31));
        _alpha = alpha;
    } /* set_alpha() */

    double get_alpha() const
    {
        return _alpha;
    }

    void set_beta(double beta)
    {
        //// 1. Sanity check
        const double requested_beta = beta;
        // Check vector length is within bounds
        if (beta > 1.0 or beta < -1.0) {
            // TODO read this bounds from the prop tree (block def)
            throw uhd::value_error("Vector IIR Beta constant must be within [-1.0, 1.0]");
        }

        //// 2. Update block
        const boost::int32_t requested_beta_q1_31 = boost::math::iround(requested_beta*pow(2.0,31.0));
        // sr_write expects unsigned int data
        sr_write(SR_BETA, static_cast<boost::uint32_t>(requested_beta_q1_31));
        _beta = beta;
    } /* set_beta() */

    double get_beta() const
    {
        return _beta;
    }


    bool set_input_signature(const stream_sig_t &stream_sig, size_t port=0)
    {
        UHD_RFNOC_BLOCK_TRACE() << "vector_iir_block::set_input_signature()" << std::endl;
        UHD_ASSERT_THROW(port == 0);
        if (stream_sig.get_item_type() != _item_type
            or (stream_sig.vlen != 0 and stream_sig.vlen != _vector_len)) {
            UHD_MSG(status) << "not valid." << std::endl;
            return false;
        }

        return true;
    }

    bool set_output_signature(const stream_sig_t &stream_sig, size_t port=0)
    {
        UHD_RFNOC_BLOCK_TRACE() << "vector_iir_block::set_output_signature()" << std::endl;
        UHD_ASSERT_THROW(port == 0);
        if (stream_sig.get_item_type() != _item_type
            or (stream_sig.vlen != 0 and stream_sig.vlen != _vector_len)) {
            return false;
        }

        return true;
    }

protected:
    void _post_args_hook()
    {
        UHD_RFNOC_BLOCK_TRACE() << "_post_args_hook()" << std::endl;
        if (_args.has_key("vector_len")) {
            size_t req_vector_len = _args.cast<size_t>("vector_len", _vector_len);
            if (req_vector_len != _vector_len) {
                set_vector_len(req_vector_len);
            }
        }

        if (_args.has_key("alpha")) {
            double req_alpha = _args.cast<double>("alpha", _alpha);
            if (req_alpha != _alpha) {
                set_alpha(req_alpha);
            }
        }

        if (_args.has_key("beta")) {
            double req_beta = _args.cast<double>("beta", _beta);
            if (req_beta != _beta) {
                set_beta(req_beta);
            }
        }

        if (_args.has_key("spp")) {
            size_t spp = _args.cast<size_t>("spp", _vector_len);
            if (spp != _vector_len) {
                throw uhd::value_error("In the Vector IIR block, spp cannot differ from the vector length.");
            }
        }
    }

    void _init_rx(uhd::stream_args_t &args)
    {
        UHD_RFNOC_BLOCK_TRACE() << "vector_iir_block::_init_rx()" << std::endl;
        if (args.otw_format != "sc16") {
            throw uhd::value_error("Vector IIR only supports otw_format sc16");
        }
        // Check if the downstream block wants a specific spp.
        // If it's not the vector length, throw. Otherwise, tell the upstream
        // block about what spp we need.
        if (not args.args.has_key("spp")) {
            args.args["spp"] = str(boost::format("%d") % _vector_len);
        } else {
            size_t req_spp = args.args.cast<size_t>("spp", _vector_len);
            if (req_spp != _vector_len) {
                throw uhd::value_error("In the Vector IIR block, spp cannot differ from the vector length (downstream block requested other spp value)");
            }
        }
    }

    void _init_tx(uhd::stream_args_t &args)
    {
        UHD_RFNOC_BLOCK_TRACE() << "vector_iir_block::_init_tx()" << std::endl;
        if (args.otw_format != "sc16") {
            throw uhd::value_error("Vector IIR only supports otw_format sc16");
        }
        // Check if the upstream block wants a specific spp.
        // If it's not the vector length, throw. Otherwise, tell the downstream
        // block about what spp we need.
        if (not args.args.has_key("spp")) {
            args.args["spp"] = str(boost::format("%d") % _vector_len);
        } else {
            size_t req_spp = args.args.cast<size_t>("spp", _vector_len);
            if (req_spp != _vector_len) {
                throw uhd::value_error("In the Vector IIR block, spp cannot differ from the vector length (downstream block requested other spp value)");
            }
        }
    }

private:
    const std::string _item_type;
    //! Bytes per item (bytes per sample)
    const size_t _bpi;
    size_t _vector_len;
    double _alpha;
    double _beta;
};

UHD_RFNOC_BLOCK_REGISTER(vector_iir_block_ctrl, "VectorIIR");
