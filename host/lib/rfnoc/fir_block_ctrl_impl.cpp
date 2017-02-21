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

#include <uhd/rfnoc/fir_block_ctrl.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/log.hpp>

using namespace uhd::rfnoc;

class fir_block_ctrl_impl : public fir_block_ctrl
{
public:
    static const uint32_t RB_NUM_TAPS     = 0;
    static const uint32_t SR_RELOAD       = 128;
    static const uint32_t SR_RELOAD_TLAST = 129;
    static const uint32_t SR_CONFIG       = 130;

    UHD_RFNOC_BLOCK_CONSTRUCTOR(fir_block_ctrl),
        _item_type("sc16") // We only support sc16 in this block
    {
        _n_taps = uint32_t(user_reg_read64(RB_NUM_TAPS));
        UHD_LOGGER_DEBUG("RFNOC") << "fir_block::fir_block() n_taps ==" << _n_taps << std::endl;
        UHD_ASSERT_THROW(_n_taps);

        // Default to Dirac impulse
        std::vector<int> default_taps(1, 20000);
        set_taps(default_taps);
    }

    void set_taps(const std::vector<int> &taps_)
    {
        UHD_RFNOC_BLOCK_TRACE() << "fir_block::set_taps()" << std::endl;
        if (taps_.size() > _n_taps) {
            throw uhd::value_error(str(
                boost::format("FIR block: Too many filter coefficients! Provided %d, FIR allows %d.\n")
                % taps_.size() % _n_taps
            ));
        }
        for (size_t i = 0; i < taps_.size(); i++) {
            if (taps_[i] > 32767 || taps_[i] < -32768) {
                throw uhd::value_error(str(
                    boost::format("FIR block: Coefficient %d out of range! Value %d, Allowed range [-32768,32767].\n")
                    % i % taps_[i]));
            }
        }
        std::vector<int> taps = taps_;
        if (taps.size() < _n_taps) {
            taps.resize(_n_taps, 0);
        }

        // Write taps via the reload bus
        for (size_t i = 0; i < taps.size() - 1; i++) {
            sr_write(SR_RELOAD, uint32_t(taps[i]));
        }
        // Assert tlast when sending the spinal tap (haha, it's actually the final tap).
        sr_write(SR_RELOAD_TLAST, uint32_t(taps.back()));
        // Send the configuration word to replace the existing coefficients with the new ones.
        // Note: This configuration bus does not require tlast
        sr_write(SR_CONFIG, 0);
    }

    //! Returns the number of filter taps in this block.
    size_t get_n_taps() const
    {
        return _n_taps;
    }

private:
    const std::string _item_type;
    size_t _n_taps;
};

UHD_RFNOC_BLOCK_REGISTER(fir_block_ctrl, "FIR");
