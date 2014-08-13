//
// Copyright 2010-2014 Ettus Research LLC
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
        _iface(iface), _slaveno(slaveno)
    {
        write_ad9146_reg(0x00, 0x20); // Take DAC into reset.
        write_ad9146_reg(0x00, 0x80); // Enable SPI reads and come out of reset
        write_ad9146_reg(0x1e, 0x01); // Data path config - set for proper operation

        // Calculate N0 to be VCO friendly.
        // Aim for VCO between 1 and 2GHz, assert otherwise.
	//  const int N1 = 4;
	const int N1 = 4;
        int N0_val, N0;
        for (N0_val = 0; N0_val < 3; N0_val++)
        {
            N0 = (1 << N0_val); //1, 2, 4
            if ((refclk * N0 * N1) >= 1e9) break;
        }
        UHD_ASSERT_THROW((refclk * N0 * N1) >= 1e9);
        UHD_ASSERT_THROW((refclk * N0 * N1) <= 2e9);

        /* Start PLL */
        //write_ad9146_reg(0x0C, 0xD1); // Narrow PLL loop filter, Midrange charge pump.
        write_ad9146_reg(0x0D, 0xD1 | (N0_val << 2)); // N1=4, N2=16, N0 as calculated
	//write_ad9146_reg(0x0D, 0x90 | (N0_val << 2)); // N1=2, N2=8, N0 as calculated
        write_ad9146_reg(0x0A, 0xCF); // Auto init VCO band training as per datasheet
        write_ad9146_reg(0x0A, 0xA0); // See above.

        // Verify PLL is Locked. 1 sec timeout. 
        // NOTE: Data sheet inconsistant about which pins give PLL lock status. FIXME!
        const time_spec_t exit_time = time_spec_t::get_system_time() + time_spec_t(1.0);
        while (true)
        {
            const size_t reg_e = read_ad9146_reg(0x0E); /* Expect bit 7 = 1 */
            if ((exit_time < time_spec_t::get_system_time()) && ((reg_e & (1 << 7)) == 0))
	      throw uhd::runtime_error("x300_dac_ctrl: timeout waiting for DAC PLL to lock");
	    else if  ((reg_e & ((1 << 7) | (1 << 6))) != 0) break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        }

        /* Skew DCI signal to find stable data eye */
        //write_ad9146_reg(0x16, 0x04); //Disable delay in DCI
        //write_ad9146_reg(0x16, 0x00); //165ps delay in DCI
        //write_ad9146_reg(0x16, 0x01); //375ps delay in DCI
        write_ad9146_reg(0x16, 0x02); //615ps delay in DCI
        //write_ad9146_reg(0x16, 0x03); //720ps delay in DCI
 
        write_ad9146_reg(0x03, 0x00); // 2's comp, I first, byte wide interface

        //fpga wants I,Q in the sample word:
        //first transaction goes into low bits
        //second transaction goes into high bits
        //therefore, we want Q to go first (bit 6 == 1)
        write_ad9146_reg(0x03, (1 << 6)); //2s comp, i first, byte mode

        write_ad9146_reg(0x10, 0x48); // Disable SYNC mode.
        write_ad9146_reg(0x17, 0x04); // FIFO write pointer offset
        write_ad9146_reg(0x18, 0x02); // Request soft FIFO align
        write_ad9146_reg(0x18, 0x00); // (See above)
        write_ad9146_reg(0x1B, 0xE4); // Bypass: Modulator, InvSinc, IQ Bal

        /* Configure interpolation filters */
        write_ad9146_reg(0x1C, 0x00); // Configure HB1
        write_ad9146_reg(0x1D, 0x00); // Configure HB2

    }


    ~x300_dac_ctrl_impl(void)
    {
        UHD_SAFE_CALL
        (
            write_ad9146_reg(0x1, 0xf); //total power down
            write_ad9146_reg(0x2, 0xf); //total power down
        )
    }

  void arm_dac_sync(void)
  {
    //
    // Attempt to synchronize AD9146's
    //
    write_ad9146_reg(0x10, 0xCF); // Enable SYNC mode. Sync Averaging set to 128.

    const time_spec_t exit_time = time_spec_t::get_system_time() + time_spec_t(1.0);
    while (true)
      {
	const size_t reg_12 = read_ad9146_reg(0x12); /* Expect bit 7 = 0, bit 6 = 1 */
	if ((exit_time < time_spec_t::get_system_time()) && (((reg_12 & (1 << 6)) == 0) || ((reg_12 & (1 << 7)) != 0)))
	  throw uhd::runtime_error("x300_dac_ctrl: timeout waiting for backend synchronization");
	else if (((reg_12 & (1 << 6)) != 0) && ((reg_12 & (1 << 7)) == 0))  break;
	boost::this_thread::sleep(boost::posix_time::milliseconds(10));
      }
  }

private:
    uhd::spi_iface::sptr _iface;
    const size_t _slaveno;
};

/***********************************************************************
 * Public make function for the DAC control
 **********************************************************************/
x300_dac_ctrl::sptr x300_dac_ctrl::make(uhd::spi_iface::sptr iface, const size_t slaveno, const double clock_rate)
{
    return sptr(new x300_dac_ctrl_impl(iface, slaveno, clock_rate));
}
