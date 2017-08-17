//
// Copyright 2010-2016 Ettus Research LLC
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

#include "x300_dac_ctrl.hpp"
#include "x300_regs.hpp"
#include <uhd/types/time_spec.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/exception.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp> //sleep

#define X300_DAC_FRONTEND_SYNC_FAILURE_FATAL

using namespace uhd;

#define write_ad9146_reg(addr, data) \
    _iface->write_spi(_slaveno, spi_config_t::EDGE_RISE, ((addr) << 8) | (data), 16)
#define read_ad9146_reg(addr) \
    (_iface->read_spi(_slaveno, spi_config_t::EDGE_RISE, ((addr) << 8) | (1 << 15), 16) & 0xff)

x300_dac_ctrl::~x300_dac_ctrl(void){
    /* NOP */
}

/*!
 * A X300 codec control specific to the ad9146 ic.
 */
class x300_dac_ctrl_impl : public x300_dac_ctrl
{
public:
    x300_dac_ctrl_impl(uhd::spi_iface::sptr iface, const size_t slaveno, const double refclk):
        _iface(iface), _slaveno(slaveno), _refclk(refclk)
    {
        //Power up all DAC subsystems
        write_ad9146_reg(0x01, 0x10); //Up: I DAC, Q DAC, Receiver, Voltage Ref, Clocks
        write_ad9146_reg(0x02, 0x00); //No extended delays. Up: Voltage Ref, PLL, DAC, FIFO, Filters

        reset();
    }

    ~x300_dac_ctrl_impl(void)
    {
        UHD_SAFE_CALL
        (
            //Power down all DAC subsystems
            write_ad9146_reg(0x01, 0xEF); //Down: I DAC, Q DAC, Receiver, Voltage Ref, Clocks
            write_ad9146_reg(0x02, 0x1F); //No extended delays. Down: Voltage Ref, PLL, DAC, FIFO, Filters
        )
    }

    void reset()
    {
        //ADI recommendations:
        //- soft reset the chip before configuration
        //- put the chip in sleep mode during configuration and wake it up when done
        //- configure synchronization settings when sleeping
        _soft_reset();
        _sleep_mode(true);
        _init();
        //We run backend sync regardless of whether we need to sync multiple DACs
        //because we use the internal DAC FIFO to meet system synchronous timing
        //and we need to guarantee that the FIFO is not empty.
        _backend_sync();
        _sleep_mode(false);
    }

    void sync()
    {
        try {
            // Just return if PLL is locked and backend is synchronized
            _check_pll();
            _check_dac_sync();
            return;
        } catch (...) {}

        std::string err_str;

        // Try 3 times to sync before giving up
        for (size_t retries = 0; retries < 3; retries++)
        {
            try {
                _sleep_mode(true);
                _init();
                _backend_sync();
                _sleep_mode(false);
                return;
            } catch (const uhd::runtime_error &e) {
                err_str = e.what();
            }
        }
        throw uhd::runtime_error(err_str);
    }

    void verify_sync()
    {
        _check_pll();
        _check_dac_sync();
#ifdef X300_DAC_FRONTEND_SYNC_FAILURE_FATAL
        _check_frontend_sync(true);
#else
        _check_frontend_sync(false);
#endif
    }

    //
    // Setup all non-synchronization related DAC parameters
    //
    void _init()
    {
        write_ad9146_reg(0x1e, 0x01);   //Datasheet: "Set 1 for proper operation"
        write_ad9146_reg(0x06, 0xFF);   //Clear all event flags

        // Calculate N0 to be VCO friendly.
        // Aim for VCO between 1 and 2GHz, assert otherwise.
        const int N1 = 4;
        int N0_val, N0;
        for (N0_val = 0; N0_val < 3; N0_val++)
        {
            N0 = (1 << N0_val); //1, 2, 4
            if ((_refclk * N0 * N1) >= 1e9) break;
        }
        UHD_ASSERT_THROW((_refclk * N0 * N1) >= 1e9);
        UHD_ASSERT_THROW((_refclk * N0 * N1) <= 2e9);

        // Start PLL
        write_ad9146_reg(0x06, 0xC0);   //Clear PLL event flags
        write_ad9146_reg(0x0C, 0xD1); // Narrow PLL loop filter, Midrange charge pump.
        write_ad9146_reg(0x0D, 0xD1 | (N0_val << 2)); // N1=4, N2=16, N0 as calculated
        write_ad9146_reg(0x0A, 0xCF); // Auto init VCO band training as per datasheet
        write_ad9146_reg(0x0A, 0xA0); // See above.

        _check_pll();

        // Configure digital interface settings
        // Bypass DCI delay. We center the clock edge in the data
        // valid window in the FPGA by phase shifting the DCI going
        // to the DAC.
        write_ad9146_reg(0x16, 0x04);
        // 2's comp, I first, byte wide interface
        write_ad9146_reg(0x03, 0x00);
        // FPGA wants I,Q in the sample word:
        // - First transaction goes into low bits
        // - Second transaction goes into high bits
        //   therefore, we want Q to go first (bit 6 == 1)
        write_ad9146_reg(0x03, (1 << 6)); //2s comp, i first, byte mode

        // Configure interpolation filters
        write_ad9146_reg(0x1C, 0x00); // Configure HB1
        write_ad9146_reg(0x1D, 0x00); // Configure HB2
        write_ad9146_reg(0x1B, 0xE4); // Bypass: Modulator, InvSinc, IQ Bal

        // Disable sync mode by default (may get turned on later)
        write_ad9146_reg(0x10, 0x40); // Disable SYNC mode.
    }

    //
    // Attempt to synchronize AD9146's
    //
    void _backend_sync(void)
    {
        write_ad9146_reg(0x10, 0x40);   // Disable SYNC mode to reset state machines.

        //SYNC Settings:
        //- SYNC = Enabled
        //- Data Rate Mode: Synchronize at the rate at which data is consumed and not at
        //                  the granularity of the FIFO
        //- Falling edge sync: For the X300, DACCLK is generated using RefClk. Within the
        //                     DAC, the RefClk is sampled by DACCLK to sync interpolation
        //                     stages across multiple DACs. To ensure that we capture the
        //                     RefClk when it is not transitioning, we sample on the falling
        //                     edge of DACCLK
        //- Averaging = MAX
        write_ad9146_reg(0x10, 0xC7);   // Enable SYNC mode. Falling edge sync. Averaging set to 128.

        //Wait for backend SYNC state machine to lock before proceeding. This guarantees that the
        //inputs and output of the FIFO have synchronized clocks
        _check_dac_sync();

        //FIFO write pointer offset
        //One of ADI's requirements to use data-rate synchronization in PLL mode is to meet
        //setup and hold times for RefClk -> DCI clock which we *do not* currently meet in
        //the FPGA. The DCI clock reaches a full RefClk cycle later which results in the
        //FIFO popping before the first push. This results in a steady-state FIFO fullness
        //of pointer - 1. To reach the optimal FIFO fullness of 4 we set the pointer to 5.
        //FIXME: At some point we should meet timing on this interface
        write_ad9146_reg(0x17, 0x05);

        // We are requesting a soft FIFO align just to put the FIFO
        // in a known state. The FRAME will actually sync the
        // FIFO correctly when a stream is created
        write_ad9146_reg(0x18, 0x02); // Request soft FIFO align
        write_ad9146_reg(0x18, 0x00); // (See above)
    }

    //
    // Check for PLL lock. Fatal if not locked within timeout
    //
    void _check_pll()
    {
        //Clear PLL event flags
        write_ad9146_reg(0x06, 0xC0);

        // Verify PLL is Locked. 1 sec timeout.
        // NOTE: Data sheet inconsistent about which pins give PLL lock status. FIXME!
        const time_spec_t exit_time = time_spec_t::get_system_time() + time_spec_t(1.0);
        while (true)
        {
            const size_t reg_e = read_ad9146_reg(0x0E); // PLL Status (Expect bit 7 = 1)
            const size_t reg_6 = read_ad9146_reg(0x06); // Event Flags (Expect bit 7 = 0 and bit 6 = 1)
            if ((((reg_e >> 7) & 0x1) == 0x1) && (((reg_6 >> 6) & 0x3) == 0x1))
                break;
            if (time_spec_t::get_system_time() > exit_time)
                throw uhd::runtime_error("x300_dac_ctrl: timeout waiting for DAC PLL to lock");
            if (reg_6 & (1 << 7))               // Lock lost?
                write_ad9146_reg(0x06, 0xC0);   // Clear PLL event flags
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        }
    }

    //
    // Check for DAC sync. Fatal if not synced within timeout
    //
    void _check_dac_sync()
    {
        // Clear Sync event flags
        write_ad9146_reg(0x06, 0x30);
        write_ad9146_reg(0x12, 0x00);

        const time_spec_t exit_time = time_spec_t::get_system_time() + time_spec_t(1.0);
        while (true)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));  // wait for sync to complete
            const size_t reg_12 = read_ad9146_reg(0x12);    // Sync Status (Expect bit 7 = 0, bit 6 = 1)
            const size_t reg_6 = read_ad9146_reg(0x06);     // Event Flags (Expect bit 5 = 0 and bit 4 = 1)
            if ((((reg_12 >> 6) & 0x3) == 0x1) && (((reg_6 >> 4) & 0x3) == 0x1))
                break;
            if (time_spec_t::get_system_time() > exit_time)
                throw uhd::runtime_error("x300_dac_ctrl: timeout waiting for backend synchronization");
            if (reg_6 & (1 << 5))
                write_ad9146_reg(0x06, 0x30);   // Clear Sync event flags
#ifdef X300_DAC_RETRY_BACKEND_SYNC
            if (reg_12 & (1 << 7)) {            // Sync acquired and lost?
                write_ad9146_reg(0x10, 0xC7);   // Enable SYNC mode. Falling edge sync. Averaging set to 128.
                write_ad9146_reg(0x12, 0x00);   // Clear Sync event flags
            }
#endif
        }
    }

    //
    // Check FIFO thermometer.
    //
    void _check_frontend_sync(bool failure_is_fatal)
    {
        // Register 0x19 has a thermometer indicator of the FIFO depth
        const size_t reg_19 = read_ad9146_reg(0x19);
        if ((reg_19 & 0xFF) != 0xF) {
            std::string msg((boost::format("x300_dac_ctrl: front-end sync failed. unexpected FIFO depth [0x%x]") % (reg_19 & 0xFF)).str());
            if (failure_is_fatal) {
                throw uhd::runtime_error(msg);
            } else {
                UHD_MSG(warning) << msg;
            }
        }
    }

    void _sleep_mode(bool sleep)
    {
        uint8_t sleep_val = sleep ? (1<<7) : 0x00;
        //Set sleep word and default fullscale value
        write_ad9146_reg(0x41, sleep_val | 0x01);    //I DAC
        write_ad9146_reg(0x45, sleep_val | 0x01);    //Q DAC
    }

    void _soft_reset()
    {
        write_ad9146_reg(0x00, 0x20); // Take DAC into reset.
        write_ad9146_reg(0x00, 0x80); // Enable SPI reads and come out of reset
    }

private:
    uhd::spi_iface::sptr _iface;
    const size_t _slaveno;
    const double _refclk;
};

/***********************************************************************
 * Public make function for the DAC control
 **********************************************************************/
x300_dac_ctrl::sptr x300_dac_ctrl::make(uhd::spi_iface::sptr iface, const size_t slaveno, const double clock_rate)
{
    return sptr(new x300_dac_ctrl_impl(iface, slaveno, clock_rate));
}
