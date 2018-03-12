//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_SOFT_REGISTER_HPP
#define INCLUDED_UHD_UTILS_SOFT_REGISTER_HPP

#include <stdint.h>
#include <boost/noncopyable.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/dirty_tracked.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/unordered_map.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <list>

/*! \file soft_register.hpp
 * Utilities to access and index hardware registers.
 *
 * This file contains three main utilities:
 * - A soft_register wrapper class that can manage a soft-copy,
 *   do dirty tracking and allow symbolic access to various field
 *   of a register.
 * - A register map class that can own multiple soft registers that
 *   share the same underlying control interface.
 * - A register map database that can be used to collect multiple
 *   register maps and other databases to create a hierarchy of
 *   registers that can be accessed using the UHD register API.
 */

//==================================================================
// Soft Register Definition
//==================================================================

#define UHD_DEFINE_SOFT_REG_FIELD(name, width, shift) \
    static const uhd::soft_reg_field_t name = (((shift & 0xFF) << 8) | (width & 0xFF))

namespace uhd {

//TODO: These hints were added to boost 1.53.

/** \brief hint for the branch prediction */
UHD_INLINE bool likely(bool expr)
{
#ifdef __GNUC__
    return __builtin_expect(expr, true);
#else
    return expr;
#endif
    }

/** \brief hint for the branch prediction */
UHD_INLINE bool unlikely(bool expr)
{
#ifdef __GNUC__
    return __builtin_expect(expr, false);
#else
    return expr;
#endif
}

/*!
 * A register field is defined as a tuple of the mask and the shift.
 * It can be used to make read-modify-write operations more convenient
 * For efficiency reasons, it is recommended to always use a constant
 * of this type because it will get optimized out by the compiler and
 * will result in zero memory overhead
 */
typedef uint32_t soft_reg_field_t;

namespace soft_reg_field {
    UHD_INLINE size_t width(const soft_reg_field_t field) {
        return (field & 0xFF);
    }

    UHD_INLINE size_t shift(const soft_reg_field_t field) {
        return ((field >> 8) & 0xFF);
    }

    template<typename data_t>
    UHD_INLINE data_t mask(const soft_reg_field_t field) {
        static const data_t ONE = static_cast<data_t>(1);
        //Behavior for the left shift operation is undefined in C++
        //if the shift amount is >= bitwidth of the datatype
        //So we treat that as a special case with a branch predicition hint
        if (likely((sizeof(data_t)*8) != width(field)))
            return ((ONE<<width(field))-ONE)<<shift(field);
        else
            return (0-ONE)<<shift(field);
    }
}

class soft_register_base : public boost::noncopyable {
public:
    virtual ~soft_register_base() {}

    virtual void initialize(wb_iface& iface, bool sync = false) = 0;
    virtual void flush() = 0;
    virtual void refresh() = 0;
    virtual size_t get_bitwidth() = 0;
    virtual bool is_readable() = 0;
    virtual bool is_writable() = 0;

    /*!
     * Cast the soft_register generic reference to a more specific type
     */
    template <typename soft_reg_t>
    UHD_INLINE static soft_reg_t& cast(soft_register_base& reg) {
        soft_reg_t* ptr = dynamic_cast<soft_reg_t*>(&reg);
        if (ptr) {
            return *ptr;
        } else {
            throw uhd::type_error("failed to cast register to specified type");
        }
    }
};

enum soft_reg_flush_mode_t { OPTIMIZED_FLUSH, ALWAYS_FLUSH };

/*!
 * Soft register object that holds offset, soft-copy and the control iface.
 * Methods give convenient field-level access to soft-copy and the ability
 * to do read-modify-write operations.
 */
template<typename reg_data_t, bool readable, bool writable>
class UHD_API soft_register_t : public soft_register_base {
public:
    typedef boost::shared_ptr< soft_register_t<reg_data_t, readable, writable> > sptr;

    //Reserved field. Represents all bits in the register.
    UHD_DEFINE_SOFT_REG_FIELD(REGISTER, sizeof(reg_data_t)*8, 0);  //[WIDTH-1:0]

    /*!
     * Generic constructor for all soft_register types
     */
    soft_register_t(
            wb_iface::wb_addr_type wr_addr,
            wb_iface::wb_addr_type rd_addr,
            soft_reg_flush_mode_t mode = ALWAYS_FLUSH):
        _iface(NULL), _wr_addr(wr_addr), _rd_addr(rd_addr), _soft_copy(0), _flush_mode(mode)
    {}

    /*!
     * Constructor for read-only, write-only registers and read-write registers
     * with rd_addr == wr_addr
     */
    explicit soft_register_t(
            wb_iface::wb_addr_type addr,
            soft_reg_flush_mode_t mode = ALWAYS_FLUSH):
        _iface(NULL), _wr_addr(addr), _rd_addr(addr), _soft_copy(0), _flush_mode(mode)
    {}

    /*!
     * Initialize the register when the underlying bus is usable.
     * Can be optionally synced with hardware.
     * NOTE: Memory management of the iface is up to the caller
     */
    UHD_INLINE void initialize(wb_iface& iface, bool sync = false)
    {
        _iface = &iface;

        //Synchronize with hardware. For RW register, flush THEN refresh.
        if (sync && writable) flush();
        if (sync && readable) refresh();
    }

    /*!
     * Update specified field in the soft-copy with the arg value.
     * Performs a read-modify-write operation so all other field are preserved.
     * NOTE: This does not write the value to hardware.
     */
    UHD_INLINE void set(const soft_reg_field_t field, const reg_data_t value)
    {
        _soft_copy = (_soft_copy & ~soft_reg_field::mask<reg_data_t>(field)) |
                     ((value << soft_reg_field::shift(field)) & soft_reg_field::mask<reg_data_t>(field));
    }

    /*!
     * Get the value of the specified field from the soft-copy.
     * NOTE: This does not read anything from hardware.
     */
    UHD_INLINE reg_data_t get(const soft_reg_field_t field)
    {
        return (_soft_copy & soft_reg_field::mask<reg_data_t>(field)) >> soft_reg_field::shift(field);
    }

    /*!
     * Write the contents of the soft-copy to hardware.
     */
    UHD_INLINE void flush()
    {
        if (writable && _iface) {
            //If optimized flush then poke only if soft copy is dirty
            //If flush mode is ALWAYS, the dirty flag should get optimized
            //out by the compiler because it is never read
            if (_flush_mode == ALWAYS_FLUSH || _soft_copy.is_dirty()) {
                if (get_bitwidth() <= 16) {
                    _iface->poke16(_wr_addr, static_cast<uint16_t>(_soft_copy));
                } else if (get_bitwidth() <= 32) {
                    _iface->poke32(_wr_addr, static_cast<uint32_t>(_soft_copy));
                } else if (get_bitwidth() <= 64) {
                    _iface->poke64(_wr_addr, static_cast<uint64_t>(_soft_copy));
                } else {
                    throw uhd::not_implemented_error("soft_register only supports up to 64 bits.");
                }
                _soft_copy.mark_clean();
            }
        } else {
            throw uhd::not_implemented_error("soft_register is not writable or uninitialized.");
        }
    }

    /*!
     * Read the contents of the register from hardware and update the soft copy.
     */
    UHD_INLINE void refresh()
    {
        if (readable && _iface) {
            if (get_bitwidth() <= 16) {
                _soft_copy = static_cast<reg_data_t>(_iface->peek16(_rd_addr));
            } else if (get_bitwidth() <= 32) {
                _soft_copy = static_cast<reg_data_t>(_iface->peek32(_rd_addr));
            } else if (get_bitwidth() <= 64) {
                _soft_copy = static_cast<reg_data_t>(_iface->peek64(_rd_addr));
            } else {
                throw uhd::not_implemented_error("soft_register only supports up to 64 bits.");
            }
            _soft_copy.mark_clean();
        } else {
            throw uhd::not_implemented_error("soft_register is not readable or uninitialized.");
        }
    }

    /*!
     * Shortcut for a set and a flush.
     */
    UHD_INLINE void write(const soft_reg_field_t field, const reg_data_t value)
    {
        set(field, value);
        flush();
    }

    /*!
     * Shortcut for refresh and get
     */
    UHD_INLINE reg_data_t read(const soft_reg_field_t field)
    {
        refresh();
        return get(field);
    }

    /*!
     * Get bitwidth for this register
     */
    UHD_INLINE size_t get_bitwidth()
    {
        static const size_t BITS_IN_BYTE = 8;
        return sizeof(reg_data_t) * BITS_IN_BYTE;
    }

    /*!
     * Is the register readable?
     */
    UHD_INLINE bool is_readable()
    {
        return readable;
    }

    /*!
     * Is the register writable?
     */
    UHD_INLINE bool is_writable()
    {
        return writable;
    }

private:
    wb_iface*                       _iface;
    const wb_iface::wb_addr_type    _wr_addr;
    const wb_iface::wb_addr_type    _rd_addr;
    dirty_tracked<reg_data_t>       _soft_copy;
    const soft_reg_flush_mode_t     _flush_mode;
};

/*!
 * A synchronized soft register object.
 * All operations in the synchronized register are serialized.
 */
template<typename reg_data_t, bool readable, bool writable>
class UHD_API soft_register_sync_t : public soft_register_t<reg_data_t, readable, writable> {
public:
    typedef boost::shared_ptr< soft_register_sync_t<reg_data_t, readable, writable> > sptr;

    soft_register_sync_t(
            wb_iface::wb_addr_type wr_addr,
            wb_iface::wb_addr_type rd_addr,
            soft_reg_flush_mode_t mode = ALWAYS_FLUSH):
        soft_register_t<reg_data_t, readable, writable>(wr_addr, rd_addr, mode), _mutex()
    {}

    explicit soft_register_sync_t(
            wb_iface::wb_addr_type addr,
            soft_reg_flush_mode_t mode = ALWAYS_FLUSH):
        soft_register_t<reg_data_t, readable, writable>(addr, mode), _mutex()
    {}

    UHD_INLINE void initialize(wb_iface& iface, bool sync = false)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        soft_register_t<reg_data_t, readable, writable>::initialize(iface, sync);
    }

    UHD_INLINE void set(const soft_reg_field_t field, const reg_data_t value)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        soft_register_t<reg_data_t, readable, writable>::set(field, value);
    }

    UHD_INLINE reg_data_t get(const soft_reg_field_t field)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        return soft_register_t<reg_data_t, readable, writable>::get(field);
    }

    UHD_INLINE void flush()
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        soft_register_t<reg_data_t, readable, writable>::flush();
    }

    UHD_INLINE void refresh()
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        soft_register_t<reg_data_t, readable, writable>::refresh();
    }

    UHD_INLINE void write(const soft_reg_field_t field, const reg_data_t value)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        soft_register_t<reg_data_t, readable, writable>::write(field, value);
    }

    UHD_INLINE reg_data_t read(const soft_reg_field_t field)
    {
        boost::lock_guard<boost::mutex> lock(_mutex);
        return soft_register_t<reg_data_t, readable, writable>::read(field);
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
typedef soft_register_t<uint16_t, false, true>       soft_reg16_wo_t;
typedef soft_register_t<uint16_t, true, false>       soft_reg16_ro_t;
typedef soft_register_t<uint16_t, true, true>        soft_reg16_rw_t;
typedef soft_register_sync_t<uint16_t, false, true>  soft_reg16_wo_sync_t;
typedef soft_register_sync_t<uint16_t, true, false>  soft_reg16_ro_sync_t;
typedef soft_register_sync_t<uint16_t, true, true>   soft_reg16_rw_sync_t;
//32-bit shortcuts
typedef soft_register_t<uint32_t, false, true>       soft_reg32_wo_t;
typedef soft_register_t<uint32_t, true, false>       soft_reg32_ro_t;
typedef soft_register_t<uint32_t, true, true>        soft_reg32_rw_t;
typedef soft_register_sync_t<uint32_t, false, true>  soft_reg32_wo_sync_t;
typedef soft_register_sync_t<uint32_t, true, false>  soft_reg32_ro_sync_t;
typedef soft_register_sync_t<uint32_t, true, true>   soft_reg32_rw_sync_t;
//64-bit shortcuts
typedef soft_register_t<uint64_t, false, true>       soft_reg64_wo_t;
typedef soft_register_t<uint64_t, true, false>       soft_reg64_ro_t;
typedef soft_register_t<uint64_t, true, true>        soft_reg64_rw_t;
typedef soft_register_sync_t<uint64_t, false, true>  soft_reg64_wo_sync_t;
typedef soft_register_sync_t<uint64_t, true, false>  soft_reg64_ro_sync_t;
typedef soft_register_sync_t<uint64_t, true, true>   soft_reg64_rw_sync_t;


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
}

//==================================================================
// Soft Register Map and Database Definition
//==================================================================

namespace uhd {

class UHD_API soft_regmap_accessor_t {
public:
    typedef boost::shared_ptr<soft_regmap_accessor_t> sptr;

    virtual ~soft_regmap_accessor_t() {};
    virtual soft_register_base& lookup(const std::string& path) const = 0;
    virtual std::vector<std::string> enumerate() const = 0;
    virtual const std::string& get_name() const = 0;
};

/*!
 * A regmap is a collection of registers that share the same
 * bus (control iface). A regmap must have an identifier.
 * A regmap must manage storage for each register.
 * The recommended way to use a regmap is to define individual registers
 * within the scope of the regmap and instantiate them in the ragmap.
 * Soft register object that holds offset, soft-copy and the control iface.
 * Methods give convenient field-level access to soft-copy and the ability
 * to do read-modify-write operations.
 */
class UHD_API soft_regmap_t : public soft_regmap_accessor_t, public boost::noncopyable {
public:
    soft_regmap_t(const std::string& name) : _name(name) {}
    virtual ~soft_regmap_t() {};

    /*!
     * Get the name of this register map
     */
    virtual UHD_INLINE const std::string& get_name() const { return _name; }

    /*!
     * Initialize all registers in this register map using a bus.
     * Optionally synchronize the register with hardware.
     * The order of initialization is the same as the order in
     * which registers were added to the map.
     */
    void initialize(wb_iface& iface, bool sync = false) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        BOOST_FOREACH(soft_register_base* reg, _reglist) {
            reg->initialize(iface, sync);
        }
    }

    /*!
     * Flush all registers to hardware.
     * The order of writing is the same as the order in
     * which registers were added to the map.
     */
    void flush() {
        boost::lock_guard<boost::mutex> lock(_mutex);
        BOOST_FOREACH(soft_register_base* reg, _reglist) {
            reg->flush();
        }
    }

    /*!
     * Refresh all register soft-copies from hardware.
     * The order of reading is the same as the order in
     * which registers were added to the map.
     */
    void refresh() {
        boost::lock_guard<boost::mutex> lock(_mutex);
        BOOST_FOREACH(soft_register_base* reg, _reglist) {
            reg->refresh();
        }
    }

    /*!
     * Lookup a register object by name.
     * If a register with "name" is not found, runtime_error is thrown
     */
    virtual soft_register_base& lookup(const std::string& name) const {
        regmap_t::const_iterator iter = _regmap.find(name);
        if (iter != _regmap.end()) {
            return *(iter->second);
        } else {
            throw uhd::runtime_error("register not found in map: " + name);
        }
    }

    /*!
     * Enumerate all the registers in this map.
     * Return fully qualified paths.
     */
    virtual std::vector<std::string> enumerate() const {
        std::vector<std::string> temp;
        BOOST_FOREACH(const regmap_t::value_type& reg, _regmap) {
            temp.push_back(_name + "/" + reg.first);
        }
        return temp;
    }

protected:
    enum visibility_t {
        PUBLIC,     //Is accessible through the soft_regmap_accessor_t interface
        PRIVATE     //Is NOT accessible through the soft_regmap_accessor_t interface
    };

    /*!
     * Add a register to this map with an identifier "name" and visibility
     */
    UHD_INLINE void add_to_map(soft_register_base& reg, const std::string& name, const visibility_t visible = PRIVATE) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        if (visible == PUBLIC) {
            //Only add to the map if this register is publicly visible
            if (not _regmap.insert(regmap_t::value_type(name, &reg)).second) {
                throw uhd::assertion_error("cannot add two registers with the same name to regmap: " + name);
            }
        }
        _reglist.push_back(&reg);
    }

private:
    typedef boost::unordered_map<std::string, soft_register_base*> regmap_t;
    typedef std::list<soft_register_base*>                         reglist_t;

    const std::string   _name;
    regmap_t            _regmap;    //For lookups
    reglist_t           _reglist;   //To maintain order
    boost::mutex        _mutex;
};


/*!
 * A regmap database is a collection of regmaps or other regmap databases
 * this allows for efficient encapsulation for multiple registers in a hierarchical
 * fashion.
 * A regmap_db *does not* manage storage for regmaps. It is simply a wrapper.
 */
class UHD_API soft_regmap_db_t : public soft_regmap_accessor_t, public boost::noncopyable {
public:
    typedef boost::shared_ptr<soft_regmap_db_t> sptr;

    /*!
     * Use the default constructor if this is the top-level DB
     */
    soft_regmap_db_t() : _name("") {}

    /*!
     * Use this constructor if this is a nested DB
     */
    soft_regmap_db_t(const std::string& name) : _name(name) {}

    /*!
     * Get the name of this register map
     */
    const std::string& get_name() const { return _name; }

    /*!
     * Add a regmap to this map with an identifier "name" and visibility
     */
    void add(soft_regmap_t& regmap) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        _regmaps.push_back(&regmap);
    }

    /*!
     * Add a level of regmap_db to this map with an identifier "name" and visibility
     */
    void add(soft_regmap_db_t& db) {
        boost::lock_guard<boost::mutex> lock(_mutex);
        if (&db == this) {
            throw uhd::assertion_error("cannot add regmap db to itself" + _name);
        } else {
            _regmap_dbs.push_back(&db);
        }
    }

    /*!
     * Lookup a register by path.
     * A path is defined as a string of "/" separated tokens that scope a register.
     * The leaf (last token) is the name of the register
     * The token immediately before the leaf is the name of the register map
     * If a nested regmap_db is used, the token before the regmap is the db name.
     * For every nested db, the path has an additional token.
     * For example:
     *   radio0/spi_regmap/spi_control_reg
     */
    soft_register_base& lookup(const std::string& path) const
    {
        //Turn the slash separated path string into tokens
        std::list<std::string> tokens;
        BOOST_FOREACH(
            const std::string& node,
            boost::tokenizer< boost::char_separator<char> >(path, boost::char_separator<char>("/")))
        {
            tokens.push_back(node);
        }
        if ((tokens.size() > 2 && tokens.front() == _name) ||   //If this is a nested DB
            (tokens.size() > 1 && _name == "")) {               //If this is a top-level DB
            if (_name != "") tokens.pop_front();
            if (tokens.size() == 2) {                   //2 tokens => regmap/register path
                BOOST_FOREACH(const soft_regmap_accessor_t* regmap, _regmaps) {
                    if (regmap->get_name() == tokens.front()) {
                        return regmap->lookup(tokens.back());
                    }
                }
                throw uhd::runtime_error("could not find register map: " + path);
            } else if (not _regmap_dbs.empty()) {       //>2 tokens => <1 or more dbs>/regmap/register
                //Reconstruct path from tokens
                std::string newpath;
                BOOST_FOREACH(const std::string& node, tokens) {
                    newpath += ("/" + node);
                }
                //Dispatch path to hierarchical DB
                BOOST_FOREACH(const soft_regmap_accessor_t* db, _regmap_dbs) {
                    try {
                        return db->lookup(newpath.substr(1));
                    } catch (std::exception&) {
                        continue;
                    }
                }
            }
        }
        throw uhd::runtime_error("could not find register: " + path);
    }

    /*!
     * Enumerate the paths of all registers that this DB can access
     */
    virtual std::vector<std::string> enumerate() const {
        std::vector<std::string> paths;
        BOOST_FOREACH(const soft_regmap_accessor_t* regmap, _regmaps) {
            const std::vector<std::string>& regs = regmap->enumerate();
            paths.insert(paths.end(), regs.begin(), regs.end());
        }
        BOOST_FOREACH(const soft_regmap_accessor_t* db, _regmap_dbs) {
            const std::vector<std::string>& regs = db->enumerate();
            paths.insert(paths.end(), regs.begin(), regs.end());
        }
        return paths;
    }

private:
    typedef std::list<soft_regmap_accessor_t*> db_t;

    const std::string   _name;
    db_t                _regmaps;
    db_t                _regmap_dbs;
    boost::mutex        _mutex;
};

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_SOFT_REGISTER_HPP */
