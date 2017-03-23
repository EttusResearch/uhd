#pragma once

#include <uhd/types/serial.hpp>

class mock_spi : public virtual uhd::spi_iface
{
public:
    typedef boost::shared_ptr<spi> sptr;
    static sptr make(const std::string &device);
};
