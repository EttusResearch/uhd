//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/device_addr.hpp>
#include <uhd/types/time_spec.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace uhd { namespace rfnoc {

/*! custom_register_space doesn't store start_addr, as start_addr should be stored as the
 * key value for the map that keeps track of custom register spaces
 */
struct custom_register_space
{
    uint32_t end_addr;
    std::function<void(uint32_t, uint32_t)> poke_fn;
    std::function<uint32_t(uint32_t)> peek_fn;
};

/*!  A software interface to access low-level registers in a NoC block.
 *
 * This interface supports the following:
 * - Writing and reading registers
 * - Hardware timed delays (for time sequencing operations)
 * - Asynchronous messages (where a block requests a "register write" in software)
 *
 * This class has no public factory function or constructor.
 */
class register_iface
{
public:
    using sptr = std::shared_ptr<register_iface>;

    virtual ~register_iface() = default;

    /*! Callback function for validating an asynchronous message.
     *
     *  When a block in the FPGA sends an asynchronous message to the software,
     *  the async message validator function is called. An async message can be
     *  modelled as a simple register write (key-value pair with addr/data) that
     *  is initiated by the FPGA.
     *  If this message returns true, the message is considered valid.
     */
    using async_msg_validator_t =
        std::function<bool(uint32_t addr, const std::vector<uint32_t>& data)>;

    /*! Callback function for acting upon an asynchronous message.
     *
     *  When a block in the FPGA sends an asynchronous message to the software,
     *  and it has been validated, the async message callback function is called.
     *  An async message can be modelled as a simple register write (key-value
     *  pair with addr/data) that is initiated by the FPGA.
     *
     *  When this message is called, the async message was previously verified
     *  by calling the async message validator callback.
     */
    using async_msg_callback_t = std::function<void(
        uint32_t addr, const std::vector<uint32_t>& data, boost::optional<uint64_t>)>;

    /*! Write a 32-bit register implemented in the NoC block.
     *
     * \param addr The byte address of the register to write to (truncated to 20 bits).
     * \param data New value of this register.
     * \param time The time at which the transaction should be executed.
     * \param ack Should transaction completion be acknowledged?
     *
     * \throws op_failed if an ACK is requested and the transaction fails
     * \throws op_timeout if an ACK is requested and no response is received
     * \throws op_seqerr if an ACK is requested and a sequence error occurs
     * \throws op_timeerr if an ACK is requested and a time error occurs (late command)
     */
    virtual void poke32(uint32_t addr,
        uint32_t data,
        uhd::time_spec_t time = uhd::time_spec_t::ASAP,
        bool ack              = false) = 0;

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
     * \param time The time at which the transaction should be executed.
     * \param ack Should transaction completion be acknowledged?
     *
     * \throws op_failed if the transaction fails
     * \throws op_timeout if no response is received
     * \throws op_seqerr if a sequence error occurs
     */
    void poke64(uint32_t addr,
        uint64_t data,
        time_spec_t time = uhd::time_spec_t::ASAP,
        bool ack         = false)
    {
        block_poke32(addr,
            {uint32_t(data & 0xFFFFFFFF), uint32_t((data >> 32) & 0xFFFFFFFF)},
            time,
            ack);
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
     * \param time The time at which the first transaction should be executed.
     * \param ack Should transaction completion be acknowledged?
     *
     * \throws uhd::value_error if lengths of data and addr don't match
     * \throws op_failed if an ACK is requested and the transaction fails
     * \throws op_timeout if an ACK is requested and no response is received
     * \throws op_seqerr if an ACK is requested and a sequence error occurs
     * \throws op_timeerr if an ACK is requested and a time error occurs (late command)
     */
    virtual void multi_poke32(const std::vector<uint32_t> addrs,
        const std::vector<uint32_t> data,
        uhd::time_spec_t time = uhd::time_spec_t::ASAP,
        bool ack              = false) = 0;

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
     * \param time The time at which the first transaction should be executed.
     * \param ack Should transaction completion be acknowledged?
     *
     * \throws op_failed if an ACK is requested and the transaction fails
     * \throws op_timeout if an ACK is requested and no response is received
     * \throws op_seqerr if an ACK is requested and a sequence error occurs
     * \throws op_timeerr if an ACK is requested and a time error occurs (late command)
     */
    virtual void block_poke32(uint32_t first_addr,
        const std::vector<uint32_t> data,
        uhd::time_spec_t time = uhd::time_spec_t::ASAP,
        bool ack              = false) = 0;

    /*! Read a 32-bit register implemented in the NoC block.
     *
     * \param addr The byte address of the register to read from (truncated to 20 bits).
     * \param time The time at which the transaction should be executed.
     *
     * \throws op_failed if the transaction fails
     * \throws op_timeout if no response is received
     * \throws op_seqerr if a sequence error occurs
     */
    virtual uint32_t peek32(uint32_t addr, time_spec_t time = uhd::time_spec_t::ASAP) = 0;

    /*! Read two consecutive 32-bit registers implemented in the NoC block
     * and return them as one 64-bit value.
     *
     * Note: This is a convenience call, because all register peeks are 32-bits.
     * This will concatenate two peeks in a block peek, and then return the
     * combined result of the two peeks.
     *
     * \param addr The byte address of the lower 32-bit register to read from
     *             (truncated to 20 bits).
     * \param time The time at which the transaction should be executed.
     *
     * \throws op_failed if the transaction fails
     * \throws op_timeout if no response is received
     * \throws op_seqerr if a sequence error occurs
     */
    uint64_t peek64(uint32_t addr, time_spec_t time = uhd::time_spec_t::ASAP)
    {
        const auto vals = block_peek32(addr, 2, time);
        return uint64_t(vals[0]) | (uint64_t(vals[1]) << 32);
    }

    /*! Read multiple 32-bit consecutive registers implemented in the NoC block.
     *
     * \param first_addr The byte address of the first register to read from
     *                   (truncated to 20 bits).
     * \param length The number of 32-bit values to read
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
    virtual std::vector<uint32_t> block_peek32(uint32_t first_addr,
        size_t length,
        time_spec_t time = uhd::time_spec_t::ASAP) = 0;

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
     * // iface is a register_iface::sptr:
     * iface->poll32(16, 0x1, 0x3, 1e-3);
     * ~~~
     *
     * \param addr The byte address of the register to read from (truncated to 20 bits).
     * \param data The values that the register must have
     * \param mask The bitmask that is applied before checking the readback
     *             value
     * \param timeout The max duration that the register is allowed to take
     *                before reaching its new state.
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
    virtual void poll32(uint32_t addr,
        uint32_t data,
        uint32_t mask,
        time_spec_t timeout,
        time_spec_t time = uhd::time_spec_t::ASAP,
        bool ack         = false) = 0;


    /*! Send a command to halt (block) the control bus for a specified time.
     * This is a hardware-timed sleep.
     *
     * \param duration The amount of time to sleep.
     * \param ack Should transaction completion be acknowledged?
     *
     * \throws op_failed if an ACK is requested and the transaction fails
     * \throws op_timeout if an ACK is requested and no response is received
     * \throws op_seqerr if an ACK is requested and a sequence error occurs
     */
    virtual void sleep(time_spec_t duration, bool ack = false) = 0;

    /*! Register a callback function to validate a received async message
     *
     * The purpose of this callback is to provide a method to the framework to
     * make sure a received async message is valid. If this callback is
     * provided, the framework will first pass the message to the validator for
     * validation. If the validator returns true, the async message is ACK'd
     * with a ctrl_status_t::CMD_OKAY response, and then the async message is
     * executed. If the validator returns false, then the async message is ACK'd
     * with a ctrl_status_t::CMD_CMDERR, and the async message handler is not
     * excecuted.
     *
     * This callback may not communicate with the device, it can only look at
     * the data and make a valid/not valid decision.
     *
     * Only one callback function can be registered. When calling this multiple
     * times, only the last callback will be accepted.
     *
     * \param callback_f The function to call when an asynchronous message is received.
     */
    virtual void register_async_msg_validator(async_msg_validator_t callback_f) = 0;

    /*! Register a callback function for when an async message is received
     *
     * Only one callback function can be registered. When calling this multiple
     * times, only the last callback will be accepted.
     *
     * \param callback_f The function to call when an asynchronous message is received.
     */
    virtual void register_async_msg_handler(async_msg_callback_t callback_f) = 0;

    /*! Set a policy that governs the operational parameters of this register bus.
     *  Policies can be used to make tradeoffs between performance, resilience, latency,
     *  etc.
     *
     * \param name The name of the policy to apply
     * \param args Additional arguments to pass to the policy governor
     */
    virtual void set_policy(const std::string& name, const uhd::device_addr_t& args) = 0;

    /*! Get the endpoint ID of the software counterpart of this register interface.
     *  This information is useful to send async messages to the host.
     *
     * \return The 16-bit endpoint ID
     */
    virtual uint16_t get_src_epid() const = 0;

    /*! Get the port number of the software counterpart of this register interface.
     *  This information is useful to send async messages to the host.
     *
     * \return The 10-bit port number
     */
    virtual uint16_t get_port_num() const = 0;

    /*! Define a custom register space that overrides peek and poke operations,
     *  directing peek and poke operations that fall within the custom space to
     *  the provided custom peek and poke functions instead.
     *
     *  Callers should be aware of the NoC block's register space, so that peeks and
     *  pokes do not unintentionally override existing functionality.
     *
     * \param start_addr The start address of the custom register space
     * \param length The length of the custom address space
     * \param poke_fn The function to call when the custom register space is poked
     * \param peek_fn The function to call when the custom register space is peeked
     */
    virtual void define_custom_register_space(const uint32_t start_addr,
        const uint32_t length,
        std::function<void(uint32_t, uint32_t)> poke_fn,
        std::function<uint32_t(uint32_t)> peek_fn) = 0;

}; // class register_iface

}} /* namespace uhd::rfnoc */
