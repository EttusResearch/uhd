//
// Copyright 2013-2014 Ettus Research LLC
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


#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_QUIRKS_H
#define INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_QUIRKS_H

#include <set>
#include <stdint.h>
#include <uhd/utils/log.hpp>

//Quirk#1: We need to verify RX zero-copy data transfers from the RIO
//         driver if we are in full duplex mode.
//         This option allows enabling this quirk.
#define UHD_NIRIO_RX_FIFO_XFER_CHECK_EN 0

namespace uhd { namespace niusrprio {

class nirio_quirks {
public:
    nirio_quirks() : _tx_stream_count(0) {
    }

    UHD_INLINE void register_tx_streams(const uint32_t tx_stream_indices[], size_t size) {
        for (size_t i = 0; i < size; i++) {
            _tx_stream_fifo_indices.insert(tx_stream_indices[i]);
        }
    }

    UHD_INLINE void add_tx_fifo(uint32_t index) {
        if (_tx_stream_fifo_indices.find(index) != _tx_stream_fifo_indices.end()) {
            if (_tx_stream_count == 0) {
                UHD_LOG << "NI-RIO RX FIFO Transfer Check Quirk Enabled.";
            }
            _tx_stream_count++;
        }
    }

    UHD_INLINE void remove_tx_fifo(uint32_t index) {
        if (_tx_stream_fifo_indices.find(index) != _tx_stream_fifo_indices.end()) {
            _tx_stream_count--;
            if (_tx_stream_count == 0) {
                UHD_LOG << "NI-RIO RX FIFO Transfer Check Quirk Disabled.";
            }
        }
    }

    //Quirk#1: We need to verify RX zero-copy data transfers from the RIO
    //         driver if we are in full duplex mode.
    //         This function returns true if the quirk is enabled for the
    //         current uhd_device configuration (dynamic)
    UHD_INLINE bool rx_fifo_xfer_check_en() const {
        //This function must be as fast as possible because it is in a high
        //throughput data path
        return _tx_stream_count > 0;
    }

private:
    std::set<uint32_t>  _tx_stream_fifo_indices;
    size_t              _tx_stream_count;
};

}}

#endif /* INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_QUIRKS_H */
