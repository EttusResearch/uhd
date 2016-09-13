//
// Copyright 2012-2013,2016 Ettus Research LLC
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

#ifndef INCLUDED_UHD_UTILS_ATOMIC_HPP
#define INCLUDED_UHD_UTILS_ATOMIC_HPP

#include <uhd/config.hpp>
#include <uhd/types/time_spec.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/interprocess/detail/atomic.hpp>

#include <boost/version.hpp>
#define BOOST_IPC_DETAIL boost::interprocess::ipcdetail

namespace uhd{

    //! A 32-bit integer that can be atomically accessed
    class UHD_API atomic_uint32_t{
    public:

        //! Create a new atomic 32-bit integer, initialized to zero
        UHD_INLINE atomic_uint32_t(void){
            this->write(0);
        }

        //! Compare with cmp, swap with newval if same, return old value
        UHD_INLINE boost::uint32_t cas(boost::uint32_t newval, boost::uint32_t cmp){
            return BOOST_IPC_DETAIL::atomic_cas32(&_num, newval, cmp);
        }

        //! Sets the atomic integer to a new value
        UHD_INLINE void write(const boost::uint32_t newval){
            BOOST_IPC_DETAIL::atomic_write32(&_num, newval);
        }

        //! Gets the current value of the atomic integer
        UHD_INLINE boost::uint32_t read(void){
            return BOOST_IPC_DETAIL::atomic_read32(&_num);
        }

        //! Increment by 1 and return the old value
        UHD_INLINE boost::uint32_t inc(void){
            return BOOST_IPC_DETAIL::atomic_inc32(&_num);
        }

        //! Decrement by 1 and return the old value
        UHD_INLINE boost::uint32_t dec(void){
            return BOOST_IPC_DETAIL::atomic_dec32(&_num);
        }

    private: volatile boost::uint32_t _num;
    };

    /*!
     * A reusable barrier to sync multiple threads.
     * All threads spin on wait() until count is reset.
     */
    class UHD_API reusable_barrier{
    public:

        reusable_barrier():_size (0) {}

        reusable_barrier(const size_t size):_size(size) {}

        //! Resize the barrier for N threads
        void resize(const size_t size){
            _size = size;
        }

        /*!
         * Force the barrier wait to throw a boost::thread_interrupted
         * The threads were not getting the interruption_point on windows.
         */
        void interrupt(void)
        {
            _done.inc();
        }

        //! Wait on the barrier condition
        UHD_INLINE void wait(void)
        {
            if (_size == 1) return;

            //entry barrier with condition variable
            _entry_counter.inc();
            _entry_counter.cas(0, _size);
            boost::mutex::scoped_lock lock(_mutex);
            while (_entry_counter.read() != 0)
            {
                this->check_interrupt();
                _cond.timed_wait(lock, boost::posix_time::milliseconds(1));
            }
            lock.unlock(); //unlock before notify
            _cond.notify_one();

            //exit barrier to ensure known condition of entry count
            _exit_counter.inc();
            _exit_counter.cas(0, _size);
            while (_exit_counter.read() != 0) this->check_interrupt();
        }

        //! Wait on the barrier condition
        UHD_INLINE void wait_others(void)
        {
            while (_entry_counter.read() != (_size-1)) this->check_interrupt();
        }

    private:
        size_t _size;
        atomic_uint32_t _entry_counter;
        atomic_uint32_t _exit_counter;
        atomic_uint32_t _done;
        boost::mutex _mutex;
        boost::condition_variable _cond;

        UHD_INLINE void check_interrupt(void)
        {
            if (_done.read() != 0) throw boost::thread_interrupted();
            boost::this_thread::interruption_point();
            boost::this_thread::yield();
        }
    };

    /*!
     * Spin-wait on a condition with a timeout.
     * \param cond an atomic variable to compare
     * \param value compare to atomic for true/false
     * \param timeout the timeout in seconds
     * \return true for cond == value, false for timeout
     */
    UHD_INLINE bool spin_wait_with_timeout(
        atomic_uint32_t &cond,
        boost::uint32_t value,
        const double timeout
    ){
        if (cond.read() == value) return true;
        const time_spec_t exit_time = time_spec_t::get_system_time() + time_spec_t(timeout);
        while (cond.read() != value){
            if (time_spec_t::get_system_time() > exit_time) return false;
            boost::this_thread::interruption_point();
            boost::this_thread::yield();
        }
        return true;
    }

    /*!
     * Claimer class to provide synchronization for multi-thread access.
     * Claiming enables buffer classes to be used with a buffer queue.
     */
    class simple_claimer{
    public:
        simple_claimer(void){
            this->release();
        }

        UHD_INLINE void release(void){
            _locked.write(0);
        }

        UHD_INLINE bool claim_with_wait(const double timeout){
            if (spin_wait_with_timeout(_locked, 0, timeout)){
                _locked.write(1);
                return true;
            }
            return false;
        }

    private:
        atomic_uint32_t _locked;
    };

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_ATOMIC_HPP */
