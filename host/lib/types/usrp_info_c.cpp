//
// Copyright 2015 Ettus Research LLC
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

#include <uhd/types/usrp_info.h>

uhd_error uhd_usrp_rx_info_free(uhd_usrp_rx_info_t *rx_info){
    free(rx_info->mboard_id);
    free(rx_info->mboard_name);
    free(rx_info->mboard_serial);
    free(rx_info->rx_id);
    free(rx_info->rx_subdev_name);
    free(rx_info->rx_subdev_spec);
    free(rx_info->rx_serial);
    free(rx_info->rx_antenna);

    return UHD_ERROR_NONE;
}

uhd_error uhd_usrp_tx_info_free(uhd_usrp_tx_info_t *tx_info){
    free(tx_info->mboard_id);
    free(tx_info->mboard_name);
    free(tx_info->mboard_serial);
    free(tx_info->tx_id);
    free(tx_info->tx_subdev_name);
    free(tx_info->tx_subdev_spec);
    free(tx_info->tx_serial);
    free(tx_info->tx_antenna);

    return UHD_ERROR_NONE;
}
