//
// Copyright 2013 Ettus Research LLC
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
#include <uhd/transport/nirio/nirio_driver_iface.h>

namespace nirio_driver_iface {

nirio_status rio_open(
    const std::string& device_path,
    rio_dev_handle_t& device_handle)
{
    return NiRio_Status_FeatureNotSupported;
}

void rio_close(rio_dev_handle_t& device_handle)
{
}

bool rio_isopen(rio_dev_handle_t device_handle)
{
    return false;
}

nirio_status rio_ioctl(
    rio_dev_handle_t device_handle,
    uint32_t ioctl_code,
    const void *write_buf,
    size_t write_buf_len,
    void *read_buf,
    size_t read_buf_len)
{
    return NiRio_Status_FeatureNotSupported;
}

nirio_status rio_mmap(
    rio_dev_handle_t device_handle,
    uint16_t memory_type,
    size_t size,
    bool writable,
    rio_mmap_t &map)
{
    return NiRio_Status_FeatureNotSupported;
}

nirio_status rio_munmap(rio_mmap_t &map)
{
    return NiRio_Status_FeatureNotSupported;
}

}
