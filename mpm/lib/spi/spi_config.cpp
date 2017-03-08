#include "spi_config.h"


spi_config_t::spi_config_t(const spi_hwd_settings_t hwd_settings, const spi_device_settings_t device_settings) :
    full_settings({
        _convert_to_adi_settings(hwd_settings.chip_select_index, device_settings),
        hwd_settings })
{

}

spiSettings_t spi_config_t::_convert_to_adi_settings(const uint8_t chip_select_index, const spi_device_settings_t device_settings)
{
    return {
        chip_select_index,
        device_settings.writeBitPolarity,
        device_settings.longInstructionWord,
        device_settings.MSBFirst,
        device_settings.CPHA,
        device_settings.CPOL,
        device_settings.enSpiStreaming,
        device_settings.autoIncAddrUp,
        device_settings.fourWireMode,
        device_settings.spiClkFreq_Hz,
    };
}

const spiSettings_t* spi_config_t::get_spi_settings() const
{
    return &(full_settings.adi_settings);
}

const spi_full_settings_t* spi_config_t::recover_full_spi_settings(const spiSettings_t* settings)
{
    // TODO: make this better
    return reinterpret_cast<const spi_full_settings_t*>(settings);
}
