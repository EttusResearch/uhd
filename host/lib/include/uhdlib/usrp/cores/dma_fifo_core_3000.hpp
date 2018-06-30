//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_USRP_DMA_FIFO_CORE_3000_HPP
#define INCLUDED_LIBUHD_USRP_DMA_FIFO_CORE_3000_HPP

#include <uhd/config.hpp>
#include <uhd/types/wb_iface.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>


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
    virtual void resize(const uint32_t base_addr, const uint32_t size) = 0;

    /*!
     * Get the (approx) number of bytes currently in the DMA FIFO
     */
    virtual uint32_t get_bytes_occupied() = 0;

    /*!
     * Run the built-in-self-test routine for the DMA FIFO
     */
    virtual uint8_t run_bist(bool finite = true, uint32_t timeout_ms = 500) = 0;

    /*!
     * Is extended BIST supported
     */
    virtual bool ext_bist_supported() = 0;

    /*!
     * Run the built-in-self-test routine for the DMA FIFO (extended BIST only)
     */
    virtual uint8_t run_ext_bist(
        bool finite,
        uint32_t rx_samp_delay,
        uint32_t tx_pkt_delay,
        uint32_t sid,
        uint32_t timeout_ms = 500) = 0;

    /*!
     * Get the throughput measured from the last invocation of the BIST (extended BIST only)
     */
    virtual double get_bist_throughput() = 0;

};

#endif /* INCLUDED_LIBUHD_USRP_DMA_FIFO_CORE_3000_HPP */
