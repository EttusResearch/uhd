//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_impl.hpp"
#include <uhd/types/wb_iface.hpp>
#include "x300_regs.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>

using namespace uhd;

struct x300_uart_iface : uart_iface
{
    x300_uart_iface(wb_iface::sptr iface):
        _iface(iface),
        rxoffset(0),
        txword32(0),
        _last_device_rxoffset(0)
    {
        txoffset = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_TX_INDEX));
        rxpool = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_RX_ADDR));
        txpool = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_TX_ADDR));
        poolsize = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_WORDS32));
        _rxcache.resize(poolsize);
        //this->write_uart("HELLO UART\n");
        //this->read_uart(0.1);
    }

    void putchar(const char ch)
    {
        const int shift = ((txoffset%4) * 8);
        if (shift == 0) txword32 = 0;
        txword32 |= uint32_t(ch) << shift;
        // Write out full 32 bit words or whatever we have if end of string
        if (txoffset % 4 == 3 or ch == '\n')
        {
            _iface->poke32(SR_ADDR(txpool, txoffset/4), txword32);
        }
        txoffset = (txoffset + 1) % (poolsize*4);
        if (ch == '\n')
        {
            // Tell the X300 to write the string
            _iface->poke32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_TX_INDEX), txoffset);
        }
    }

    void write_uart(const std::string &buff)
    {
        boost::mutex::scoped_lock(_write_mutex);
        for(const char ch:  buff)
        {
            this->putchar(ch);
        }
    }

    int getchar(void)
    {
        if (rxoffset == _last_device_rxoffset)
            return -1;

        int ret = static_cast<int>(_rxcache[((rxoffset)/4) % poolsize] >> ((rxoffset%4)*8) & 0xFF);
        rxoffset++;
        return ret;
    }

    void update_cache(void)
    {
        uint32_t device_rxoffset = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_RX_INDEX));
        uint32_t delta = device_rxoffset - rxoffset;

        while (delta)
        {
            if (delta >= poolsize*4)
            {
                // all the data is new - reload the entire cache
                for (uint32_t i = 0; i < poolsize; i++)
                {
                    _rxcache[i] = _iface->peek32(SR_ADDR(rxpool, i));
                }

                // set the head to the same character as the current device
                // offset (tail) one loop earlier
                rxoffset = device_rxoffset - (poolsize*4);

                // set the tail to the current device offset
                _last_device_rxoffset = device_rxoffset;

                // the string at the head is a partial, so skip it
                for (int c = getchar(); c != '\n' and c != -1; c = getchar()) {}

                // clear the partial string in the buffer, if any
                _rxbuff.clear();
            }
            else if (rxoffset == _last_device_rxoffset)
            {
                // new data was added - refresh the portion of the cache that was updated
                for (uint32_t i = (_last_device_rxoffset/4) % poolsize;
                        i != ((device_rxoffset/4)+1) % poolsize;
                        i = (i+1) % poolsize)
                {
                    _rxcache[i] = _iface->peek32(SR_ADDR(rxpool, i));
                }

                // set the tail to the current device offset
                _last_device_rxoffset = device_rxoffset;
            }
            else
            {
                // there is new data, but we aren't done with what we have - check back later
                break;
            }

            // check again to see if anything changed while we were updating the cache
            device_rxoffset = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_RX_INDEX));
            delta = device_rxoffset - rxoffset;
        }
    }

    std::string read_uart(double timeout)
    {
        boost::mutex::scoped_lock(_read_mutex);
        const boost::system_time exit_time = boost::get_system_time() + boost::posix_time::microseconds(long(timeout*1e6));
        std::string buff;

        while (true)
        {
            // Update cache
            this->update_cache();

            // Get available characters
            for (int ch = this->getchar(); ch != -1; ch = this->getchar())
            {
                // store character to buffer
                _rxbuff.append(1, ch);

                // newline found - return string
                if (ch == '\n')
                {
                    buff.swap(_rxbuff);
                    return buff;
                }
            }

            // no more characters - check time
            if (boost::get_system_time() > exit_time)
                break;
        }

        return buff;
    }

    wb_iface::sptr _iface;
    uint32_t rxoffset, txoffset, txword32, rxpool, txpool, poolsize;
    uint32_t _last_device_rxoffset;
    std::vector<uint32_t> _rxcache;
    std::string _rxbuff;
    boost::mutex _read_mutex;
    boost::mutex _write_mutex;
};

uart_iface::sptr x300_make_uart_iface(wb_iface::sptr iface)
{
    return uart_iface::sptr(new x300_uart_iface(iface));
}
