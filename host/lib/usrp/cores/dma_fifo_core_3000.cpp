//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/soft_register.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/cores/dma_fifo_core_3000.hpp>
#include <boost/format.hpp>
#include <chrono>
#include <thread>

using namespace uhd;

#define SR_DRAM_BIST_BASE 16

dma_fifo_core_3000::~dma_fifo_core_3000(void) {
    /* NOP */
}

class dma_fifo_core_3000_impl : public dma_fifo_core_3000
{
protected:
    class rb_addr_reg_t : public soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(ADDR, /*width*/ 3, /*shift*/ 0);  //[2:0]

        static const uint32_t RB_FIFO_STATUS     = 0;
        static const uint32_t RB_BIST_STATUS     = 1;
        static const uint32_t RB_BIST_XFER_CNT   = 2;
        static const uint32_t RB_BIST_CYC_CNT    = 3;
        static const uint32_t RB_BUS_CLK_RATE    = 4;

        rb_addr_reg_t(uint32_t base):
            soft_reg32_wo_t(base + 0)
        {
            //Initial values
            set(ADDR, RB_FIFO_STATUS);
        }
    };

    class fifo_ctrl_reg_t : public soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(CLEAR_FIFO,           /*width*/  1, /*shift*/  0);  //[0]
        UHD_DEFINE_SOFT_REG_FIELD(RD_SUPPRESS_EN,       /*width*/  1, /*shift*/  1);  //[1]
        UHD_DEFINE_SOFT_REG_FIELD(BURST_TIMEOUT,        /*width*/ 12, /*shift*/  4);  //[15:4]
        UHD_DEFINE_SOFT_REG_FIELD(RD_SUPPRESS_THRESH,   /*width*/ 16, /*shift*/ 16);  //[31:16]

        fifo_ctrl_reg_t(uint32_t base):
            soft_reg32_wo_t(base + 4)
        {
            //Initial values
            set(CLEAR_FIFO, 1);
            set(RD_SUPPRESS_EN, 0);
            set(BURST_TIMEOUT, 256);
            set(RD_SUPPRESS_THRESH, 0);
        }
    };

    class base_addr_reg_t : public soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(BASE_ADDR, /*width*/ 30, /*shift*/ 0);  //[29:0]

        base_addr_reg_t(uint32_t base):
            soft_reg32_wo_t(base + 8)
        {
            //Initial values
            set(BASE_ADDR, 0x00000000);
        }
    };

    class addr_mask_reg_t : public soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(ADDR_MASK, /*width*/ 30, /*shift*/ 0);  //[29:0]

        addr_mask_reg_t(uint32_t base):
            soft_reg32_wo_t(base + 12)
        {
            //Initial values
            set(ADDR_MASK, 0xFF000000);
        }
    };

    class bist_ctrl_reg_t : public soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(GO,               /*width*/ 1, /*shift*/ 0);  //[0]
        UHD_DEFINE_SOFT_REG_FIELD(CONTINUOUS_MODE,  /*width*/ 1, /*shift*/ 1);  //[1]
        UHD_DEFINE_SOFT_REG_FIELD(TEST_PATT,        /*width*/ 2, /*shift*/ 4);  //[5:4]

        static const uint32_t TEST_PATT_ZERO_ONE     = 0;
        static const uint32_t TEST_PATT_CHECKERBOARD = 1;
        static const uint32_t TEST_PATT_COUNT        = 2;
        static const uint32_t TEST_PATT_COUNT_INV    = 3;

        bist_ctrl_reg_t(uint32_t base):
            soft_reg32_wo_t(base + 16)
        {
            //Initial values
            set(GO, 0);
            set(CONTINUOUS_MODE, 0);
            set(TEST_PATT, TEST_PATT_ZERO_ONE);
        }
    };

    class bist_cfg_reg_t : public soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(MAX_PKTS,         /*width*/ 18, /*shift*/ 0);  //[17:0]
        UHD_DEFINE_SOFT_REG_FIELD(MAX_PKT_SIZE,     /*width*/ 13, /*shift*/ 18); //[30:18]
        UHD_DEFINE_SOFT_REG_FIELD(PKT_SIZE_RAMP,    /*width*/ 1,  /*shift*/ 31); //[31]

        bist_cfg_reg_t(uint32_t base):
            soft_reg32_wo_t(base + 20)
        {
            //Initial values
            set(MAX_PKTS, 0);
            set(MAX_PKT_SIZE, 0);
            set(PKT_SIZE_RAMP, 0);
        }
    };

    class bist_delay_reg_t : public soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(TX_PKT_DELAY,     /*width*/ 16, /*shift*/ 0);  //[15:0]
        UHD_DEFINE_SOFT_REG_FIELD(RX_SAMP_DELAY,    /*width*/  8, /*shift*/ 16); //[23:16]

        bist_delay_reg_t(uint32_t base):
            soft_reg32_wo_t(base + 24)
        {
            //Initial values
            set(TX_PKT_DELAY, 0);
            set(RX_SAMP_DELAY, 0);
        }
    };

    class bist_sid_reg_t : public soft_reg32_wo_t {
    public:
        UHD_DEFINE_SOFT_REG_FIELD(SID,     /*width*/ 32, /*shift*/ 0);  //[31:0]

        bist_sid_reg_t(uint32_t base):
            soft_reg32_wo_t(base + 28)
        {
            //Initial values
            set(SID, 0);
        }
    };

public:
    class fifo_readback {
    public:
        fifo_readback(wb_iface::sptr iface,  const size_t base, const size_t rb_addr) :
            _iface(iface), _addr_reg(base), _rb_addr(rb_addr)
        {
            _addr_reg.initialize(*iface, true);
        }

        bool is_fifo_instantiated() {
            boost::lock_guard<boost::mutex> lock(_mutex);
            _addr_reg.write(rb_addr_reg_t::ADDR, rb_addr_reg_t::RB_FIFO_STATUS);
            return _iface->peek32(_rb_addr) & 0x80000000;
        }

        uint32_t get_occupied_cnt() {
            boost::lock_guard<boost::mutex> lock(_mutex);
            _addr_reg.write(rb_addr_reg_t::ADDR, rb_addr_reg_t::RB_FIFO_STATUS);
            return _iface->peek32(_rb_addr) & 0x7FFFFFF;
        }

        uint32_t is_fifo_busy() {
            boost::lock_guard<boost::mutex> lock(_mutex);
            _addr_reg.write(rb_addr_reg_t::ADDR, rb_addr_reg_t::RB_FIFO_STATUS);
            return _iface->peek32(_rb_addr) & 0x40000000;
        }

        struct bist_status_t {
            bool running;
            bool finished;
            uint8_t error;
        };

        bist_status_t get_bist_status() {
            boost::lock_guard<boost::mutex> lock(_mutex);
            _addr_reg.write(rb_addr_reg_t::ADDR, rb_addr_reg_t::RB_BIST_STATUS);
            uint32_t st32 = _iface->peek32(_rb_addr) & 0xF;
            bist_status_t status;
            status.running = st32 & 0x1;
            status.finished = st32 & 0x2;
            status.error = static_cast<uint8_t>((st32>>2) & 0x3);
            return status;
        }

        bool is_ext_bist_supported() {
            boost::lock_guard<boost::mutex> lock(_mutex);
            _addr_reg.write(rb_addr_reg_t::ADDR, rb_addr_reg_t::RB_BIST_STATUS);
            return _iface->peek32(_rb_addr) & 0x80000000;
        }

        double get_xfer_ratio() {
            boost::lock_guard<boost::mutex> lock(_mutex);
            uint32_t xfer_cnt = 0, cyc_cnt = 0;
            _addr_reg.write(rb_addr_reg_t::ADDR, rb_addr_reg_t::RB_BIST_XFER_CNT);
            xfer_cnt = _iface->peek32(_rb_addr);
            _addr_reg.write(rb_addr_reg_t::ADDR, rb_addr_reg_t::RB_BIST_CYC_CNT);
            cyc_cnt = _iface->peek32(_rb_addr);
            return (static_cast<double>(xfer_cnt)/cyc_cnt);
        }

        double get_bus_clk_rate() {
            uint32_t bus_clk_rate = 0;
            boost::lock_guard<boost::mutex> lock(_mutex);
            _addr_reg.write(rb_addr_reg_t::ADDR, rb_addr_reg_t::RB_BUS_CLK_RATE);
            bus_clk_rate = _iface->peek32(_rb_addr);
            return (static_cast<double>(bus_clk_rate));
        }

    private:
        wb_iface::sptr  _iface;
        rb_addr_reg_t   _addr_reg;
        const size_t    _rb_addr;
        boost::mutex    _mutex;
    };

public:
    dma_fifo_core_3000_impl(wb_iface::sptr iface, const size_t base, const size_t readback):
        _iface(iface), _fifo_readback(iface, base, readback),
        _fifo_ctrl_reg(base), _base_addr_reg(base), _addr_mask_reg(base),
        _bist_ctrl_reg(base), _bist_cfg_reg(base), _bist_delay_reg(base), _bist_sid_reg(base)
    {
        _fifo_ctrl_reg.initialize(*iface, true);
        _base_addr_reg.initialize(*iface, true);
        _addr_mask_reg.initialize(*iface, true);
        _bist_ctrl_reg.initialize(*iface, true);
        _bist_cfg_reg.initialize(*iface, true);
        _has_ext_bist = _fifo_readback.is_ext_bist_supported();
        if (_has_ext_bist) {
            _bist_delay_reg.initialize(*iface, true);
            _bist_sid_reg.initialize(*iface, true);
        }
        flush();
    }

    virtual ~dma_fifo_core_3000_impl()
    {
    }

    virtual void flush() {
        //Clear the FIFO and hold it in that state
        _fifo_ctrl_reg.write(fifo_ctrl_reg_t::CLEAR_FIFO, 1);
        //Re-arm the FIFO
        _wait_for_fifo_empty();
        _fifo_ctrl_reg.write(fifo_ctrl_reg_t::CLEAR_FIFO, 0);
    }

    virtual void resize(const uint32_t base_addr, const uint32_t size) {
        //Validate parameters
        if (size < 8192) throw uhd::runtime_error("DMA FIFO must be larger than 8KiB");
        uint32_t size_mask = size - 1;
        if (size & size_mask) throw uhd::runtime_error("DMA FIFO size must be a power of 2");

        //Clear the FIFO and hold it in that state
        _fifo_ctrl_reg.write(fifo_ctrl_reg_t::CLEAR_FIFO, 1);
        //Write base address and mask
        _base_addr_reg.write(base_addr_reg_t::BASE_ADDR, base_addr);
        _addr_mask_reg.write(addr_mask_reg_t::ADDR_MASK, ~size_mask);

        //Re-arm the FIFO
        flush();
    }

    virtual uint32_t get_bytes_occupied() {
        return _fifo_readback.get_occupied_cnt() * 8;
    }

    virtual bool ext_bist_supported() {
        return _fifo_readback.is_ext_bist_supported();
    }

    virtual uint8_t run_bist(bool finite = true, uint32_t timeout_ms = 500) {
        return run_ext_bist(finite, 0, 0, 0, timeout_ms);
    }

    virtual uint8_t run_ext_bist(
        bool finite,
        uint32_t rx_samp_delay,
        uint32_t tx_pkt_delay,
        uint32_t sid,
        uint32_t timeout_ms = 500
    ) {
        boost::lock_guard<boost::mutex> lock(_mutex);

        _wait_for_bist_done(timeout_ms, true);          //Stop previous BIST and wait (if running)
        _bist_ctrl_reg.write(bist_ctrl_reg_t::GO, 0);   //Reset

        _bist_cfg_reg.set(bist_cfg_reg_t::MAX_PKTS, (2^18)-1);
        _bist_cfg_reg.set(bist_cfg_reg_t::MAX_PKT_SIZE, 8000);
        _bist_cfg_reg.set(bist_cfg_reg_t::PKT_SIZE_RAMP, 0);
        _bist_cfg_reg.flush();

        if (_has_ext_bist) {
            _bist_delay_reg.set(bist_delay_reg_t::RX_SAMP_DELAY, rx_samp_delay);
            _bist_delay_reg.set(bist_delay_reg_t::TX_PKT_DELAY, tx_pkt_delay);
            _bist_delay_reg.flush();

            _bist_sid_reg.write(bist_sid_reg_t::SID, sid);
        } else {
            if (rx_samp_delay != 0 || tx_pkt_delay != 0 || sid != 0) {
                throw uhd::not_implemented_error(
                    "dma_fifo_core_3000: Runtime delay and SID support only available on FPGA images with extended BIST enabled");
            }
        }

        _bist_ctrl_reg.set(bist_ctrl_reg_t::TEST_PATT, bist_ctrl_reg_t::TEST_PATT_COUNT);
        _bist_ctrl_reg.set(bist_ctrl_reg_t::CONTINUOUS_MODE, finite ? 0 : 1);
        _bist_ctrl_reg.write(bist_ctrl_reg_t::GO, 1);

        if (!finite) {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        }

        _wait_for_bist_done(timeout_ms, !finite);
        if (!_fifo_readback.get_bist_status().finished) {
            throw uhd::runtime_error("dma_fifo_core_3000: DRAM BIST state machine is in a bad state.");
        }

        return _fifo_readback.get_bist_status().error;
    }

    virtual double get_bist_throughput() {
        if (_has_ext_bist) {
            _wait_for_bist_done(1000);
            static const double BYTES_PER_CYC = 8;
            double bus_clk_rate = _fifo_readback.get_bus_clk_rate();
            return _fifo_readback.get_xfer_ratio() * bus_clk_rate * BYTES_PER_CYC;
        } else {
            throw uhd::not_implemented_error(
                "dma_fifo_core_3000: Throughput counter only available on FPGA images with extended BIST enabled");
        }
    }

private:
    void _wait_for_fifo_empty()
    {
        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration elapsed;

        while (_fifo_readback.is_fifo_busy()) {
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            elapsed = boost::posix_time::microsec_clock::local_time() - start_time;
            if (elapsed.total_milliseconds() > 100) break;
        }
    }

    void _wait_for_bist_done(uint32_t timeout_ms, bool force_stop = false)
    {
        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration elapsed;

        while (_fifo_readback.get_bist_status().running) {
            if (force_stop) {
                _bist_ctrl_reg.write(bist_ctrl_reg_t::GO, 0);
                force_stop = false;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            elapsed = boost::posix_time::microsec_clock::local_time() - start_time;
            if (elapsed.total_milliseconds() > timeout_ms) break;
        }
    }

private:
    wb_iface::sptr  _iface;
    boost::mutex    _mutex;
    bool            _has_ext_bist;

    fifo_readback       _fifo_readback;
    fifo_ctrl_reg_t     _fifo_ctrl_reg;
    base_addr_reg_t     _base_addr_reg;
    addr_mask_reg_t     _addr_mask_reg;
    bist_ctrl_reg_t     _bist_ctrl_reg;
    bist_cfg_reg_t      _bist_cfg_reg;
    bist_delay_reg_t    _bist_delay_reg;
    bist_sid_reg_t      _bist_sid_reg;
};

//
// Static make function
//
dma_fifo_core_3000::sptr dma_fifo_core_3000::make(wb_iface::sptr iface, const size_t set_base, const size_t rb_addr)
{
    if (check(iface, set_base, rb_addr)) {
        return sptr(new dma_fifo_core_3000_impl(iface, set_base, rb_addr));
    } else {
        throw uhd::runtime_error("");
    }
}

bool dma_fifo_core_3000::check(wb_iface::sptr iface, const size_t set_base, const size_t rb_addr)
{
    dma_fifo_core_3000_impl::fifo_readback fifo_rb(iface, set_base, rb_addr);
    return fifo_rb.is_fifo_instantiated();
}
