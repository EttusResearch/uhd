#include "spi_lock.h"

spi_lock::spi_lock(uint8_t spidev_index) :
    spidev_index(spidev_index)
{

}

uint8_t spi_lock::get_spidev() const
{
    return spidev_index;
}

void spi_lock::lock()
{
    spi_mutex.lock();
}
void spi_lock::unlock()
{
    spi_mutex.unlock();
}

spi_lock::sptr spi_lock::make(uint8_t spidev_index)
{
    return std::make_shared<spi_lock>(spidev_index);
}