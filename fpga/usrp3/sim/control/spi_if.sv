//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Interface: spi_if
//
// Description:
//
//   Defines a SystemVerilog interface for a basic SPI bus.
//

interface spi_if;
  logic cs_n, sck, mosi, miso;
endinterface
