//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/rfnoc/register_iface_holder.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/time_spec.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace uhd { namespace rfnoc {

/*!  A software interface to access low-level registers in a NoC block, which
 * automatically handles address translation between multiple consecutive
 * instances of the block. (For instance, accessing registers from multiple
 * channels in hardware).
 *
 * This interface supports the following:
 * - Writing and reading registers
 * - Hardware timed delays (for time sequencing operations)
 *
 */
class multichan_register_iface
{
public:
    using sptr = std::shared_ptr<multichan_register_iface>;

    multichan_register_iface(register_iface_holder& reg_iface_holder,
        const uint32_t block_base_addr,
        const size_t block_size)
        : _reg_iface_holder(reg_iface_holder)
        , _block_base_addr(block_base_addr)
        , _block_size(block_size)
    {
    }

    ~multichan_register_iface() = default;

    /*! Write a 32-bit register implemented in the NoC block.
     *
     * \param addr The byte address of the register to write to (truncated to 20 bits).
     * \param data New value of this register.
     * \param instance The index of the block of registers to which the write applies
     * \param time The time at which the transaction should be executed.
     * \param ack Should transaction completion be acknowledged?
     *
     * \throws op_failed if an ACK is requested and the transaction fails
     * \throws op_timeout if an ACK is requested and no response is received
     * \throws op_seqerr if an ACK is requested and a sequence error occurs
     * \throws op_timeerr if an ACK is requested and a time error occurs (late command)
     */
    inline void poke32(uint32_t addr,
        uint32_t data,
        const size_t instance = 0,
        uhd::time_spec_t time = uhd::time_spec_t::ASAP,
        bool ack              = false)
    {
        _reg_iface_holder.regs().poke32(_get_addr(addr, instance), data, time, ack);
    }

    /*! Write two consecutive 32-bit registers implemented in the NoC block from
     * one 64-bit value.
     *
     * Note: This is a convenience call, because all register pokes are 32-bits.
     * This will concatenate two pokes in a block poke, and then return the
     * combined result of the two pokes.
     *
     * \param addr The byte address of the lower 32-bit register to read from
     *             (truncated to 20 bits).
     * \param data New value of the register(s).
     * \param instance The index of the block of registers to which the writes apply.
     * \param time The time at which the transaction should be executed.
     * \param ack Should transaction completion be acknowledged?
     *
     * \throws op_failed if the transaction fails
     * \throws op_timeout if no response is received
     * \throws op_seqerr if a sequence error occurs
     */
    inline void poke64(uint32_t addr,
        uint64_t data,
        const size_t instance = 0,
        time_spec_t time      = uhd::time_spec_t::ASAP,
        bool ack              = false)
    {
        _reg_iface_holder.regs().poke64(_get_addr(addr, instance), data, time, ack);
    }

    /*! Write multiple 32-bit registers implemented in the NoC block.
     *
     * This method should be called when multiple writes need to happen that are
     * at non-consecutive addresses. For consecutive writes, cf. block_poke32().
     *
     * \param addrs The byte addresses of the registers to write to
     *              (each truncated to 20 bits).
     * \param data New values of these registers. The lengths of data and addr
     *             must match.
     * \param instance The index of the block of registers to which the writes apply.
     * \param time The time at which the first transaction should be executed.
     * \param ack Should transaction completion be acknowledged?

     * \throws uhd::value_error if lengths of data and addr don't match
     * \throws op_failed if an ACK is requested and the transaction fails
     * \throws op_timeout if an ACK is requested and no response is received
     * \throws op_seqerr if an ACK is requested and a sequence error occurs
     * \throws op_timeerr if an ACK is requested and a time error occurs (late command)
     */
    inline void multi_poke32(const std::vector<uint32_t> addrs,
        const std::vector<uint32_t> data,
        const size_t instance = 0,
        uhd::time_spec_t time = uhd::time_spec_t::ASAP,
        bool ack              = false)
    {
        std::vector<uint32_t> abs_addrs(addrs.size());
        std::transform(addrs.begin(),
            addrs.end(),
            abs_addrs.begin(),
            [this, instance](
                uint32_t addr) -> uint32_t { return _get_addr(addr, instance); });
        _reg_iface_holder.regs().multi_poke32(abs_addrs, data, time, ack);
    }

    /*! Write multiple consecutive 32-bit registers implemented in the NoC block.
     *
     * This function will only allow writes to adjacent registers, in increasing
     * order. If addr is set to 0, and the length of data is 8, then this method
     * triggers eight writes, in order, to addresses 0, 4, 8, 12, 16, 20, 24, 28.
     * For arbitrary addresses, cf. multi_poke32().
     *
     * Note: There is no guarantee that under the hood, the implementation won't
     * separate the writes.
     *
     * \param first_addr The byte addresses of the first register to write
     * \param data New values of these registers
     * \param instance The index of the block of registers to which the writes apply.
     * \param time The time at which the first transaction should be executed.
     * \param ack Should transaction completion be acknowledged?
     *
     * \throws op_failed if an ACK is requested and the transaction fails
     * \throws op_timeout if an ACK is requested and no response is received
     * \throws op_seqerr if an ACK is requested and a sequence error occurs
     * \throws op_timeerr if an ACK is requested and a time error occurs (late command)
     */
    inline void block_poke32(uint32_t first_addr,
        const std::vector<uint32_t> data,
        const size_t instance = 0,
        uhd::time_spec_t time = uhd::time_spec_t::ASAP,
        bool ack              = false)
    {
        _reg_iface_holder.regs().block_poke32(
            _get_addr(first_addr, instance), data, time, ack);
    }

    /*! Read a 32-bit register implemented in the NoC block.
     *
     * \param addr The byte address of the register to read from (truncated to 20 bits).
     * \param instance The index of the block of registers to which the read applies.
     * \param time The time at which the transaction should be executed.
     *
     * \throws op_failed if the transaction fails
     * \throws op_timeout if no response is received
     * \throws op_seqerr if a sequence error occurs
     */
    inline uint32_t peek32(uint32_t addr,
        const size_t instance = 0,
        time_spec_t time      = uhd::time_spec_t::ASAP)
    {
        return _reg_iface_holder.regs().peek32(_get_addr(addr, instance), time);
    }

    /*! Read two consecutive 32-bit registers implemented in the NoC block
     * and return them as one 64-bit value.
     *
     * Note: This is a convenience call, because all register peeks are 32-bits.
     * This will concatenate two peeks in a block peek, and then return the
     * combined result of the two peeks.
     *
     * \param addr The byte address of the lower 32-bit register to read from
     *             (truncated to 20 bits).
     * \param instance The index of the block of registers to which the reads apply.
     * \param time The time at which the transaction should be executed.
     *
     * \throws op_failed if the transaction fails
     * \throws op_timeout if no response is received
     * \throws op_seqerr if a sequence error occurs
     */
    inline uint64_t peek64(uint32_t addr,
        const size_t instance = 0,
        time_spec_t time      = uhd::time_spec_t::ASAP)
    {
        return _reg_iface_holder.regs().peek64(_get_addr(addr, instance), time);
    }

    /*! Read multiple 32-bit consecutive registers implemented in the NoC block.
     *
     * \param first_addr The byte address of the first register to read from
     *                   (truncated to 20 bits).
     * \param length The number of 32-bit values to read
     * \param instance The index of the block of registers to which the reads apply.
     * \param time The time at which the transaction should be executed.
     * \return data New value of this register.
     *
     * Example: If \p first_addr is set to 0, and length is 8, then this
     * function will return a vector of length 8, with the content of registers
     * at addresses 0, 4, 8, 12, 16, 20, 24, and 28 respectively.
     *
     * Note: There is no guarantee that under the hood, the implementation won't
     * separate the reads.
     *
     * \throws op_failed if the transaction fails
     * \throws op_timeout if no response is received
     * \throws op_seqerr if a sequence error occurs
     */
    inline std::vector<uint32_t> block_peek32(uint32_t first_addr,
        size_t length,
        const size_t instance = 0,
        time_spec_t time      = uhd::time_spec_t::ASAP)
    {
        return _reg_iface_holder.regs().block_peek32(
            _get_addr(first_addr, instance), length, time);
    }

    /*! Poll a 32-bit register until its value for all bits in mask match data&mask
     *
     * This will insert a command into the command queue to wait until a
     * register is of a certain value. This can be used, e.g., to poll for a
     * lock pin before executing the next command. It is related to sleep(),
     * except it has a condition to wait on, rather than an unconditional stall
     * duration. The timeout is hardware-timed.
     * If the register does not attain the requested value within the requested
     * duration, ${something bad happens}.
     *
     * Example: Assume readback register 16 is a status register, and bit 0
     * indicates a lock is in place (i.e., we want it to be 1) and bit 1 is an
     * error flag (i.e., we want it to be 0). The previous command can modify
     * the state of the block, so we give it 1ms to settle. In that case, the
     * call would be thus:
     *
     * ~~~{.cpp}
     * // iface is a multichan_register_iface::sptr:
     * iface->poll32(16, 0x1, 0x3, 1e-3);
     * ~~~
     *
     * \param addr The byte address of the register to read from (truncated to 20 bits).
     * \param data The values that the register must have
     * \param mask The bitmask that is applied before checking the readback
     *             value
     * \param timeout The max duration that the register is allowed to take
     *                before reaching its new state.
     * \param instance The index of the block of registers to which the poll applies.
     * \param time When the poll should be executed
     * \param ack Should transaction completion be acknowledged? This is
     *            typically only necessary if the software needs a condition to
     *            be fulfilled before continueing, or during debugging.
     *
     * \throws op_failed if an ACK is requested and the transaction fails
     * \throws op_timeout if an ACK is requested and no response is received
     * \throws op_seqerr if an ACK is requested and a sequence error occurs
     * \throws op_timeerr if an ACK is requested and a time error occurs (late command)
     */
    inline void poll32(uint32_t addr,
        uint32_t data,
        uint32_t mask,
        time_spec_t timeout,
        const size_t instance = 0,
        time_spec_t time      = uhd::time_spec_t::ASAP,
        bool ack              = false)
    {
        _reg_iface_holder.regs().poll32(
            _get_addr(addr, instance), data, mask, timeout, time, ack);
    }

private:
    register_iface_holder& _reg_iface_holder;
    uint32_t _block_base_addr;
    size_t _block_size;

    inline uint32_t _get_addr(const uint32_t reg_offset, const size_t instance) const
    {
        return _block_base_addr + reg_offset + _block_size * instance;
    }

}; // class multichan_register_iface

}} /* namespace uhd::rfnoc */
