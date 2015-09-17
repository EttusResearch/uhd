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

#ifndef INCLUDED_LIBUHD_USRP_DMA_FIFO_CORE_3000_HPP
#define INCLUDED_LIBUHD_USRP_DMA_FIFO_CORE_3000_HPP

#include <uhd/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <uhd/types/wb_iface.hpp>


class dma_fifo_core_3000 : boost::noncopyable
{
public:
    typedef boost::shared_ptr<dma_fifo_core_3000> sptr;
    virtual ~dma_fifo_core_3000(void) = 0;

    /*!
     * Create a DMA FIFO controller using the given bus, settings and readback base
     * Throws uhd::runtime_error if a DMA FIFO is not instantiated in the FPGA
     */
    static sptr make(uhd::wb_iface::sptr iface, const size_t set_base, const size_t rb_addr);

    /*!
     * Check if a DMA FIFO is instantiated in the FPGA
     */
    static bool check(uhd::wb_iface::sptr iface, const size_t set_base, const size_t rb_addr);

    /*!
     * Flush the DMA FIFO. Will clear all contents.
     */
    virtual void flush() = 0;

    /*!
     * Resize and rebase the DMA FIFO. Will clear all contents.
     */
    virtual void resize(const boost::uint32_t base_addr, const boost::uint32_t size) = 0;

    /*!
     * Get the (approx) number of bytes currently in the DMA FIFO
     */
    virtual boost::uint32_t get_bytes_occupied() = 0;

    /*!
     * Run the built-in-self-test routine for the DMA FIFO
     */
    virtual boost::uint8_t run_bist(bool finite = true, boost::uint32_t timeout_ms = 500) = 0;

    /*!
     * Is extended BIST supported
     */
    virtual bool ext_bist_supported() = 0;

    /*!
     * Run the built-in-self-test routine for the DMA FIFO (extended BIST only)
     */
    virtual boost::uint8_t run_ext_bist(
        bool finite,
        boost::uint32_t rx_samp_delay,
        boost::uint32_t tx_pkt_delay,
        boost::uint32_t sid,
        boost::uint32_t timeout_ms = 500) = 0;

    /*!
     * Get the throughput measured from the last invocation of the BIST (extended BIST only)
     */
    virtual double get_bist_throughput(double fifo_clock_rate) = 0;

};

#endif /* INCLUDED_LIBUHD_USRP_DMA_FIFO_CORE_3000_HPP */
