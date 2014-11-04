//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_UHD_UTILS_SOFT_REGISTER_HPP
#define INCLUDED_UHD_UTILS_SOFT_REGISTER_HPP

#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/exception.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#define UHD_DEFINE_SOFT_REG_FIELD(name, width, shift) \
    static const uhd::soft_reg_field_t name = (((shift & 0xFF) << 8) | (width & 0xFF))

namespace uhd {

/* A register field is defined as a tuple of the mask and the shift.
 * It can be used to make read-modify-write operations more convenient
 * For efficiency reasons, it is recommended to always use a constant
 * of this type because it will get optimized out by the compiler and
 * will result in zero memory overhead
 */
typedef boost::uint32_t soft_reg_field_t;

namespace soft_reg_field {
    inline size_t width(const soft_reg_field_t field) {
        return (field & 0xFF);
    }

    inline size_t shift(const soft_reg_field_t field) {
        return ((field >> 8) & 0xFF);
    }

    template<typename data_t>
    inline size_t mask(const soft_reg_field_t field) {
        return ((static_cast<data_t>(1)<<width(field))-1)<<shift(field);
    }
}

/*!
 * Soft register object that holds offset, soft-copy and the control iface.
 * Methods give convenient field-level access to soft-copy and the ability
 * to do read-modify-write operations.
 */
template<typename reg_data_t, bool readable, bool writeable>
class UHD_API soft_register_t : public boost::noncopyable {
public:
    typedef boost::shared_ptr< soft_register_t<reg_data_t, readable, writeable> > sptr;

    /*!
     * Generic constructor for all soft_register types
     */
    soft_register_t(wb_iface::wb_addr_type wr_addr, wb_iface::wb_addr_type rd_addr):
        _iface(NULL), _wr_addr(wr_addr), _rd_addr(rd_addr), _soft_copy(0)
    {}

    /*!
     * Constructor for read-only, write-only registers and read-write registers
     * with rd_addr == wr_addr
     */
    soft_register_t(wb_iface::wb_addr_type addr):
        _iface(NULL), _wr_addr(addr), _rd_addr(addr), _soft_copy(0)
    {}

    /*!
     * Initialize the register when the underlying bus is usable.
     * Can be optionally synced with hardware.
     * NOTE: Memory management of the iface is up to the caller
     */
    inline void initialize(wb_iface& iface, bool sync = false)
    {
        _iface = &iface;

        //Synchronize with hardware. For RW register, flush THEN refresh.
        if (sync && writeable) flush();
        if (sync && readable) refresh();
    }

    /*!
     * Update specified field in the soft-copy with the arg value.
     * Performs a read-modify-write operation so all other field are preserved.
     * NOTE: This does not write the value to hardware.
     */
    inline void set(const soft_reg_field_t field, const reg_data_t value)
    {
        _soft_copy = (_soft_copy & ~soft_reg_field::mask<reg_data_t>(field)) |
                     ((value << soft_reg_field::shift(field)) & soft_reg_field::mask<reg_data_t>(field));
    }

    /*!
     * Get the value of the specified field from the soft-copy.
     * NOTE: This does not read anything from hardware.
     */
    inline reg_data_t get(const soft_reg_field_t field)
    {
        return (_soft_copy & soft_reg_field::mask<reg_data_t>(field)) >> soft_reg_field::shift(field);
    }

    /*!
     * Write the contents of the soft-copy to hardware.
     */
    inline void flush()
    {
        if (writeable && _iface) {
            if (sizeof(reg_data_t) <= 2) {
                _iface->poke16(_wr_addr, static_cast<boost::uint16_t>(_soft_copy));
            } else if (sizeof(reg_data_t) <= 4) {
                _iface->poke32(_wr_addr, static_cast<boost::uint32_t>(_soft_copy));
            } else if (sizeof(reg_data_t) <= 8) {
                _iface->poke64(_wr_addr, static_cast<boost::uint64_t>(_soft_copy));
            } else {
                throw uhd::not_implemented_error("soft_register only supports up to 64 bits.");
            }
        } else {
            throw uhd::not_implemented_error("soft_register is not writable.");
        }
    }

    /*!
     * Read the contents of the register from hardware and update the soft copy.
     */
    inline void refresh()
    {
        if (readable && _iface) {
            if (sizeof(reg_data_t) <= 2) {
                _soft_copy = static_cast<reg_data_t>(_iface->peek16(_rd_addr));
            } else if (sizeof(reg_data_t) <= 4) {
                _soft_copy = static_cast<reg_data_t>(_iface->peek32(_rd_addr));
            } else if (sizeof(reg_data_t) <= 8) {
                _soft_copy = static_cast<reg_data_t>(_iface->peek64(_rd_addr));
            } else {
                throw uhd::not_implemented_error("soft_register only supports up to 64 bits.");
            }
        } else {
            throw uhd::not_implemented_error("soft_register is not readable.");
        }
    }

    /*!
     * Shortcut for a set and a flush.
     */
    inline void write(const soft_reg_field_t field, const reg_data_t value)
    {
        set(field, value);
        flush();
    }

    /*!
     * Shortcut for refresh and get
     */
    inline reg_data_t read(const soft_reg_field_t field)
    {
        refresh();
        return get(field);
    }

private:
    wb_iface*                       _iface;
    const wb_iface::wb_addr_type    _wr_addr;
    const wb_iface::wb_addr_type    _rd_addr;
    reg_data_t                      _soft_copy;
};

/*!
 * A synchronized soft register object.
 * All operations in the synchronized register are serialized.
 */
template<typename reg_data_t, bool readable, bool writeable>
class UHD_API soft_register_sync_t : public soft_register_t<reg_data_t, readable, writeable> {
public:
    typedef boost::shared_ptr< soft_register_sync_t<reg_data_t, readable, writeable> > sptr;

    soft_register_sync_t(wb_iface::wb_addr_type wr_addr, wb_iface::wb_addr_type rd_addr):
        soft_register_t<reg_data_t, readable, writeable>(wr_addr, rd_addr), _mutex()
    {}

    soft_register_sync_t(wb_iface::wb_addr_type addr):
        soft_register_t<reg_data_t, readable, writeable>(addr), _mutex()
    {}

    inline void initialize(wb_iface& iface, bool sync = false)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        soft_register_t<reg_data_t, readable, writeable>::initialize(iface, sync);
    }

    inline void set(const soft_reg_field_t field, const reg_data_t value)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        soft_register_t<reg_data_t, readable, writeable>::set(field, value);
    }

    inline reg_data_t get(const soft_reg_field_t field)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        return soft_register_t<reg_data_t, readable, writeable>::get(field);
    }

    inline void flush()
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        soft_register_t<reg_data_t, readable, writeable>::flush();
    }

    inline void refresh()
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        soft_register_t<reg_data_t, readable, writeable>::refresh();
    }

    inline void write(const soft_reg_field_t field, const reg_data_t value)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        soft_register_t<reg_data_t, readable, writeable>::write(field, value);
    }

    inline reg_data_t read(const soft_reg_field_t field)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        return soft_register_t<reg_data_t, readable, writeable>::read(field);
    }

private:
    boost::mutex _mutex;
};

/*
 * Register Shortcut Formats:
 * - soft_reg<bits>_<mode>_t: Soft register object with an unsynchronized soft-copy.
 *                            Thread unsafe but lightweight. Mostly const propagated.
 * - soft_reg<bits>_<mode>_sync_t: Soft register object with a synchronized soft-copy.
 *                                 Thread safe but with memory/speed overhead.
 * where:
 * - <bits> = {16, 32 or 64}
 * - <mode> = {wo(write-only), rw(read-write) or ro(read-only)}
 *
 */

//16-bit shortcuts
typedef soft_register_t<boost::uint16_t, false, true>       soft_reg16_wo_t;
typedef soft_register_t<boost::uint16_t, true, false>       soft_reg16_ro_t;
typedef soft_register_t<boost::uint16_t, true, true>        soft_reg16_rw_t;
typedef soft_register_sync_t<boost::uint16_t, false, true>  soft_reg16_wo_sync_t;
typedef soft_register_sync_t<boost::uint16_t, true, false>  soft_reg16_ro_sync_t;
typedef soft_register_sync_t<boost::uint16_t, true, true>   soft_reg16_rw_sync_t;
//32-bit shortcuts
typedef soft_register_t<boost::uint32_t, false, true>       soft_reg32_wo_t;
typedef soft_register_t<boost::uint32_t, true, false>       soft_reg32_ro_t;
typedef soft_register_t<boost::uint32_t, true, true>        soft_reg32_rw_t;
typedef soft_register_sync_t<boost::uint32_t, false, true>  soft_reg32_wo_sync_t;
typedef soft_register_sync_t<boost::uint32_t, true, false>  soft_reg32_ro_sync_t;
typedef soft_register_sync_t<boost::uint32_t, true, true>   soft_reg32_rw_sync_t;
//64-bit shortcuts
typedef soft_register_t<boost::uint64_t, false, true>       soft_reg64_wo_t;
typedef soft_register_t<boost::uint64_t, true, false>       soft_reg64_ro_t;
typedef soft_register_t<boost::uint64_t, true, true>        soft_reg64_rw_t;
typedef soft_register_sync_t<boost::uint64_t, false, true>  soft_reg64_wo_sync_t;
typedef soft_register_sync_t<boost::uint64_t, true, false>  soft_reg64_ro_sync_t;
typedef soft_register_sync_t<boost::uint64_t, true, true>   soft_reg64_rw_sync_t;


/*
 * Usage example
 *
  //===Define bit width, RW mode, and synchronization using base class===
  class example_reg_t : public soft_reg32_wo_sync_t (or soft_reg32_wo_t) {
  public:
    //===Define all the fields===
    UHD_DEFINE_SOFT_REG_FIELD(FIELD0,  1,  0);  //[0]
    UHD_DEFINE_SOFT_REG_FIELD(FIELD1, 15,  1);  //[15:1]
    UHD_DEFINE_SOFT_REG_FIELD(FIELD2, 16, 16);  //[31:16]

    example_reg_t():    //ctor with no args
      soft_reg32_wo_t(SR_CORE_EXAMPLE_REG_OFFSET)) //===Bind to offset===
    {
      //===Set Initial values===
      set(FIELD0, 0);
      set(FIELD1, 1);
      set(FIELD2, 0xFFFF);
    }
  }; //===Full register definition encapsulated in one class===

  void main() {
    example_reg_t reg_obj;
    reg_obj.initialize(iface);
    reg_obj.write(example_reg_t::FIELD2, 0x1234);

    example_reg_t::sptr reg_sptr = boost::make_shared<example_reg_t>();
    reg_obj->initialize(iface);
    reg_obj->write(example_reg_t::FIELD2, 0x1234);
  }
*/

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_SOFT_REGISTER_HPP */
