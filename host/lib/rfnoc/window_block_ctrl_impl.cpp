//
// Copyright 2014-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/window_block_ctrl.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/log.hpp>

using namespace uhd::rfnoc;

class window_block_ctrl_impl : public window_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(window_block_ctrl),
        _item_type("sc16"), // We only support sc16 in this block
        _bpi(uhd::convert::get_bytes_per_item("sc16"))
    {
        _max_len = uint32_t(user_reg_read64(RB_MAX_WINDOW_LEN));
        UHD_LOGGER_DEBUG(unique_id())
            << "window_block::window_block() max_len ==" << _max_len << std::endl;
        UHD_ASSERT_THROW(_max_len);

        // TODO we need a coercer to check that spp on the prop tree doesn't get set to anything invalid
        _set_default_window(std::min<size_t>(get_arg<int>("spp"), _max_len));
    }

    //! Set window coefficients and length
    void set_window(const std::vector<int> &coeffs)
    {
        UHD_LOGGER_TRACE(unique_id())
            << "window_block::set_window()" << std::endl;
        if (coeffs.size() > _max_len) {
            throw uhd::value_error(str(
                boost::format("window_block::set_window(): Too many window "
                              "coefficients! Provided %d, window allows up to %d.\n")
                % coeffs.size() % _max_len
            ));
        }

        size_t window_len = coeffs.size();

        // Window block can take complex coefficients in sc16 format, but typical usage is
        // to have real(coeffs) == imag(coeffs)
        std::vector<uint32_t> coeffs_;
        for (size_t i = 0; i < window_len - 1; i++) {
            if (coeffs[i] > 32767 || coeffs[i] < -32768) {
                throw uhd::value_error(str(
                    boost::format("window_block::set_window(): Coefficient %d "
                                  "(index %d) outside coefficient range [-32768,32767].\n")
                    % coeffs[i] % i));
            }
            coeffs_.push_back(coeffs[i]);
        }

        // Write coefficients via the load bus
        for (size_t i = 0; i < window_len - 1; i++) {
            sr_write(AXIS_WINDOW_LOAD, coeffs_[i]);
        }
        // Assert tlast when sending the final coefficient (sorry, no joke here)
        sr_write(AXIS_WINDOW_LOAD_TLAST, coeffs_.back());
        // Set the window length
        sr_write(SR_WINDOW_LEN, window_len);

        // This block requires spp to match the window length:
        set_arg<int>("spp", int(window_len));
    }

    //! Returns the maximum window length of this block.
    size_t get_max_len() const
    {
        return _max_len;
    }

    size_t get_window_len() const
    {
        return size_t(get_arg<int>("spp"));
    }


private:
    const std::string _item_type;
    const size_t _bpi;
    size_t _max_len;

    //! Default is a rectangular window
    void _set_default_window(size_t window_len) {
        std::vector<int> default_coeffs(window_len, (1 << 15)-1);
        set_window(default_coeffs);
    }
};

UHD_RFNOC_BLOCK_REGISTER(window_block_ctrl, "Window");
