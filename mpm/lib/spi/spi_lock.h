#pragma once

#include <boost/noncopyable.hpp>
#include <memory>
#include <mutex>

class spi_lock : public boost::noncopyable
{
public:
    using sptr = std::shared_ptr<spi_lock>;
    static sptr make(uint8_t spidev_index);

    spi_lock(uint8_t spidev_index);

    uint8_t get_spidev() const;

private:
    const uint8_t spidev_index;

    // BasicLockable implementation for lock_guard
    mutable std::mutex spi_mutex;
    friend class std::lock_guard<spi_lock>;
    void lock();
    void unlock();
};