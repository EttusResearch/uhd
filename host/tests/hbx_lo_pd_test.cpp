//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include <uhdlib/usrp/dboard/hbx/hbx_lo_pd.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(hbx_lo_pd_basic_test)
{
    // Mock poke function (write to SPI)
    auto mock_poke =
        [](uint32_t addr, uint32_t data, uhd::usrp::hbx::hbx_cpld_ctrl::chan_t chan) {
            (void)addr;
            (void)data;
            (void)chan;
        };

    // Mock peek function (read from SPI)
    // Must return SPI info on first call, then SPI ready bits and ADC data
    auto mock_peek = [](uint32_t addr) -> uint32_t {
        // When reading at offset 0 (SPI info register), return data_width and
        // address_width
        if ((addr & 0xFF) == 0) {
            // Bits 0-7: data_width = 16 (for LO PD ADC)
            // Bits 8-15: address_width = 8
            return (0 << 8) | 16;
        }

        // For SPI status register reads (offset 0x04)
        // Bits 0-15: ADC data (16-bit value)
        // Bit 29: SPI_READY
        // Bit 30: SPI_READ_READY
        const uint32_t SPI_READY_BIT      = 31;
        const uint32_t SPI_READ_READY_BIT = 30;
        const uint16_t adc_value          = 32768; // Mid-scale for 16-bit ADC

        uint32_t status = (1 << SPI_READY_BIT) | (1 << SPI_READ_READY_BIT) | adc_value;
        return status;
    };

    const size_t start_address = 0; // SPI start address for HBX LO PD
    auto lo_pd                 = std::make_shared<uhd::usrp::hbx::hbx_lo_pd>(
        start_address, std::move(mock_poke), std::move(mock_peek));
    BOOST_REQUIRE(lo_pd);

    // Test raw data reading
    uint16_t raw_data = lo_pd->read_data_raw();
    BOOST_CHECK_EQUAL(raw_data, 32768);

    // Test dBm conversion at 1 GHz
    double power_dbm_1 = lo_pd->read_data_dbm(1e9);
    BOOST_CHECK_CLOSE(power_dbm_1, -43.4, 0.1); // Expected value based on calibration

    // Test dBm conversion at 2.4 GHz
    double power_dbm_24 = lo_pd->read_data_dbm(2.4e9);
    BOOST_CHECK_CLOSE(power_dbm_24, -41.9, 0.1); // Expected value based on calibration

    // Test dBm conversion at 3.5 GHz
    double power_dbm_35 = lo_pd->read_data_dbm(3.5e9);
    BOOST_CHECK_CLOSE(power_dbm_35, -39.9, 0.1); // Expected value based on calibration

    // Test dBm conversion at 6 GHz
    double power_dbm_6 = lo_pd->read_data_dbm(6.0e9);
    BOOST_CHECK_CLOSE(power_dbm_6, -33.93, 0.1); // Expected value based on calibration

    // Test dBm conversion at 500 MHz
    double power_dbm_500 = lo_pd->read_data_dbm(500e6);
    BOOST_CHECK_CLOSE(power_dbm_500, -43.34, 0.1); // Expected value based on calibration

    // Test dBm conversion at 7 GHz (more than what we allow the LO to go to)
    BOOST_CHECK_THROW(lo_pd->read_data_dbm(7e9), uhd::runtime_error);

    // Test dBm conversion at 300 MHz
    BOOST_CHECK_THROW(lo_pd->read_data_dbm(300e6), uhd::runtime_error);
}
