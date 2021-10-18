//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <vector>

namespace uhd { namespace rfnoc { namespace x400 {

// The name of the X400's GPIO bank
extern const char* GPIO_BANK_NAME;

/*! Abstract X400's GPIO control to match the "gpio_attr" control scheme.
 *
 * The front panel has two ports on it, labelled GPIO0 and GPIO1. The registers
 * to control all of the GPIOs contain 24 bits, split between bit indices
 * [31:16] and [11:0]. Additionally, the underlying radio control registers
 * support a full 16-entry lookup table for the ATR state, with the 4 bits
 * being a combination of the ATR state for the two channels. The classic
 * "gpio_attr" control scheme only considers channels independently - i.e.,
 * a single 4-entry lookup table for each channel. X400 supports this behaviour
 * as well via the "classic ATR" switch, which this class uses.
 *
 * All of the public values are exposed as a single 24-bit wide field, [23:0]
 *
 * The data direction registers (DDR) have to be set in two places: Both in the
 * internal radio control registers, as well as in MPM to configure the DIO
 * board.
 */
class gpio_control {
public:
    using sptr = std::shared_ptr<gpio_control>;

    /*! Constructs a gpio_control given the given offset. Assumes that the
     * 16-table ATR entry begins at address 0x0 in \p iface.
     *
     * \param rpcc RPC object to talk to MPM
     * \param iface wb_iface to talk to the radio registers
     */
    gpio_control(uhd::usrp::x400_rpc_iface::sptr rpcc, wb_iface::sptr iface);

    /*! Set the given GPIO attribute. See gpio_atr_3000 for details.
     */
    void set_gpio_attr(const usrp::gpio_atr::gpio_attr_t attr, const uint32_t value);

    /*! Get the given GPIO attribute. See gpio_atr_3000 for details.
     */
    uint32_t get_gpio_attr(const usrp::gpio_atr::gpio_attr_t attr);

private:
    /*! Converts from the public-facing [23:0] format to the internal [31:16],
     * [11:0] format.
     */
    static uint32_t internalize_value(const uint32_t value);

    /*! Converts from the internal [31:16], [11:0] format to the public-facing
     * [23:0] format.
     */
    static uint32_t publicize_value(const uint32_t value);

    /*! Convert from the internal FPGA pin mapping to the "DIO" mapping. This
     * matches the "DIO_PORT_MAP" field in MPM's x4xx_periphs.py file.
     */
    static uint32_t unmap_dio(const uint32_t bank, const uint32_t raw_form);

    /*! Returns whether the given attribute is setting one of the ATR entries.
     */
    static bool is_atr_attr(const usrp::gpio_atr::gpio_attr_t attr);

    uhd::usrp::x400_rpc_iface::sptr _rpcc;
    wb_iface::sptr _regs;

    // There are two GPIOs, one for each channel. These two are set in unison.
    std::vector<usrp::gpio_atr::gpio_atr_3000::sptr> _gpios;
};

}}} // namespace uhd::rfnoc::x400
