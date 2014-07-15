//
// Copyright 2013 Ettus Research LLC
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

#include "x300_impl.hpp"
#include <uhd/types/wb_iface.hpp>
#include "x300_regs.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>

using namespace uhd;

struct x300_uart_iface : uart_iface
{
    x300_uart_iface(wb_iface::sptr iface):
        rxoffset(0), txoffset(0), txword32(0), rxpool(0), txpool(0), poolsize(0)
    {
        _iface = iface;
        rxoffset = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_RX_INDEX));
        txoffset = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_TX_INDEX));
        rxpool = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_RX_ADDR));
        txpool = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_TX_ADDR));
        poolsize = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_WORDS32));
        _rxcache.resize(poolsize);
        _last_device_rxoffset = rxoffset;
        //this->write_uart("HELLO UART\n");
        //this->read_uart(0.1);
    }

    void putchar(const char ch)
    {
        txoffset = (txoffset + 1) % (poolsize*4);
        const int shift = ((txoffset%4) * 8);
        if (shift == 0) txword32 = 0;
        txword32 |= boost::uint32_t(ch) << shift;
        _iface->poke32(SR_ADDR(txpool, txoffset/4), txword32);
        _iface->poke32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_TX_INDEX), txoffset);
    }

    void write_uart(const std::string &buff)
    {
        boost::mutex::scoped_lock(_write_mutex);
        BOOST_FOREACH(const char ch, buff)
        {
            if (ch == '\n') this->putchar('\r');
            this->putchar(ch);
        }
    }

    int getchar(void)
    {
        if (rxoffset == _last_device_rxoffset)
            return -1;

        rxoffset++;
        return static_cast<int>(_rxcache[(rxoffset/4) % poolsize] >> ((rxoffset%4)*8) & 0xFF);
    }

    void update_cache(void)
    {
        boost::uint32_t device_rxoffset = _iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_UART_RX_INDEX));
        boost::uint32_t delta = device_rxoffset - rxoffset;

        while (delta)
        {
            if (delta >= poolsize*4)
            {
                // all the data is new - reload the entire cache
                for (boost::uint32_t i = 0; i < poolsize; i++)
                    _rxcache[i] = _iface->peek32(SR_ADDR(rxpool, i));

                // set rxoffset to the end of the first string
                rxoffset = device_rxoffset - (poolsize*4) + 1;
                while (static_cast<char>((_rxcache[(rxoffset/4) % poolsize] >> ((rxoffset%4)*8) & 0xFF)) != '\n')
                    ++rxoffset;

                // clear the partial string in the buffer;
                _rxbuff.clear();
            }
            else if (rxoffset == _last_device_rxoffset)
            {
                // new data was added - refresh the portion of the cache that was updated
                for (boost::uint32_t i = ((_last_device_rxoffset+1)/4) % poolsize; i != (((device_rxoffset)/4)+1) % poolsize; i = (i+1) % poolsize)
                {
                    _rxcache[i] = _iface->peek32(SR_ADDR(rxpool, i));
                }
            } else {
                // there is new data, but we aren't done with what we have - check back later
                break;
            }

            _last_device_rxoffset = device_rxoffset;

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
                // skip carriage returns
                if (ch == '\r')
                    continue;

                // store character to buffer
                _rxbuff += std::string(1, (char)ch);

                // newline found - return string
                if (ch == '\n')
                {
                    buff = _rxbuff;
                    _rxbuff.clear();
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
    boost::uint32_t rxoffset, txoffset, txword32, rxpool, txpool, poolsize;
    boost::uint32_t _last_device_rxoffset;
    std::vector<boost::uint32_t> _rxcache;
    std::string _rxbuff;
    boost::mutex _read_mutex;
    boost::mutex _write_mutex;
};

uart_iface::sptr x300_make_uart_iface(wb_iface::sptr iface)
{
    return uart_iface::sptr(new x300_uart_iface(iface));
}
