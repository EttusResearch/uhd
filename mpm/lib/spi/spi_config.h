#pragma once

#include "../mykonos/adi/common.h"

// contains information about the spi configuration

struct spi_device_settings_t
{
    uint8_t writeBitPolarity;
    uint8_t longInstructionWord;    ///< 1 = 16bit instruction word, 0 = 8bit instruction word
    uint8_t MSBFirst;               ///< 1 = MSBFirst, 0 = LSBFirst
    uint8_t CPHA;                   ///< clock phase, sets which clock edge the data updates (valid 0 or 1)
    uint8_t CPOL;                   ///< clock polarity 0 = clock starts low, 1 = clock starts high
    uint8_t enSpiStreaming;         ///< Not implemented. SW feature to improve SPI throughput.
    uint8_t autoIncAddrUp;          ///< Not implemented. For SPI Streaming, set address increment direction. 1= next addr = addr+1, 0:addr = addr-1
    uint8_t fourWireMode;           ///< 1: Use 4-wire SPI, 0: 3-wire SPI (SDIO pin is bidirectional). NOTE: ADI's FPGA platform always uses 4-wire mode.
    uint32_t spiClkFreq_Hz;
};

struct spi_hwd_settings_t
{
    uint8_t spidev_index;
    uint8_t chip_select_index;
};

struct spi_full_settings_t
{
    spiSettings_t adi_settings;
    spi_hwd_settings_t hwd_settings;
};

class spi_config_t
{
public:
    spi_config_t(spi_hwd_settings_t hwd_settings, spi_device_settings_t device_settings);

private:
    const spi_full_settings_t full_settings;
    static spiSettings_t _convert_to_adi_settings(uint8_t chip_select_index, spi_device_settings_t device_settings);

public:
    const spiSettings_t* get_spi_settings() const;
    static const spi_full_settings_t* recover_full_spi_settings(const spiSettings_t* settings);
};
