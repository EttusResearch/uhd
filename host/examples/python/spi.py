#!/usr/bin/env python3
#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Example for X4xx SPI.

This example shows how to work with SPI over the GPIO interface of the X4xx series.
It is a simple example that writes a payload to the SPI peripheral and reads back the response.
The example uses the Discoverable Feature API to get the SPI interface from the radio block.
The example assumes that an SPI peripheral is connected to the GPIO pins of the radio block.
Refer to https://uhd.readthedocs.io/en/latest/page_x400_gpio_api.html for more information
about the GPIO interface and how to configure the pins for SPI operation.

Example usage:
spi.py --args addr=192.168.10.2 --sdo 2 --sdi 1 --cs 3 --clk 0 --payload 0xfefe --length 32 
       --clk-div 4 --miso-edge rise --mosi-edge fall
"""

import argparse
import sys

import uhd

SPI_DEFAULT_CLK_PIN = 0
SPI_DEFAULT_SDI_PIN = 1
SPI_DEFAULT_SDO_PIN = 2
SPI_DEFAULT_CS_PIN = 3
SPI_DEFAULT_PAYLOAD_LENGTH = 32
SPI_DEFAULT_PAYLOAD = "0xfefe"
SPI_DEFAULT_CLK_DIVIDER = None
SPI_DEFAULT_MOSI_EDGE = uhd.types.SPIEdge.EDGE_RISE
SPI_DEFAULT_MISO_EDGE = uhd.types.SPIEdge.EDGE_FALL


def parse_args():
    """Parse the command line arguments."""
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__,
    )
    parser.add_argument("-a", "--args", default="", type=str, help="UHD device address args")
    parser.add_argument(
        "--sdo", default=SPI_DEFAULT_SDO_PIN, type=int, help="Pin number for SPI SDO"
    )
    parser.add_argument(
        "--sdi", default=SPI_DEFAULT_SDI_PIN, type=int, help="Pin number for SPI SDI"
    )
    parser.add_argument("--cs", default=SPI_DEFAULT_CS_PIN, type=int, help="Pin number for SPI CS")
    parser.add_argument(
        "--clk", default=SPI_DEFAULT_CLK_PIN, type=int, help="Pin number for SPI CLK"
    )
    parser.add_argument(
        "--payload", default=SPI_DEFAULT_PAYLOAD, type=str, help="SPI payload as integer value"
    )
    parser.add_argument(
        "--length", default=SPI_DEFAULT_PAYLOAD_LENGTH, type=int, help="SPI payload length in bits"
    )
    parser.add_argument(
        "--clk-div", default=SPI_DEFAULT_CLK_DIVIDER, type=int, help="Clock divider for SPI"
    )
    parser.add_argument(
        "--miso-edge",
        default=SPI_DEFAULT_MISO_EDGE,
        type=spi_edge_type,
        help="SPI Sample MISO at clk edge type",
    )
    parser.add_argument(
        "--mosi-edge",
        default=SPI_DEFAULT_MOSI_EDGE,
        type=spi_edge_type,
        help="SPI Output MOSI at clk edge type",
    )

    return parser.parse_args()


def spi_edge_type(value):
    """Convert a string to a SPIEdge enum value."""
    if value.lower() in ["rise", "rising", "edge_rise"]:
        return uhd.types.SPIEdge.EDGE_RISE
    if value.lower() in ["fall", "falling", "edge_fall"]:
        return uhd.types.SPIEdge.EDGE_FALL
    raise argparse.ArgumentTypeError(
        f"Invalid SPI edge type: {value}. Valid values are: rise, fall, edge_rise, edge_fall"
    )


args = parse_args()
graph = uhd.rfnoc.RfnocGraph(args.args)
radios = graph.find_blocks("Radio")
if not radios:
    print("ERROR: No Radio block found in the RFNoC graph.")
    sys.exit(1)
radio = uhd.rfnoc.RadioControl(graph.get_block(radios[0]))
mbc = graph.get_mb_controller()

if not radio.has_feature(uhd.types.DiscoverableFeatureID.SPI_GETTER_IFACE):
    print("ERROR: This example requires a radio with SPI getter interface support.")
    sys.exit(1)
# Get the SPI getter interface from where we'll get the SPI interface itself
sgi = radio.get_feature(uhd.types.DiscoverableFeatureID.SPI_GETTER_IFACE)

# Create peripheral configuration per peripheral
cfg = uhd.types.SPIPeriphConfig()
cfg.periph_clk = args.clk
cfg.periph_cs = args.cs
cfg.periph_sdi = args.sdi
cfg.periph_sdo = args.sdo

# The vector holds the peripheral configs with index=peripheral number
periph_cfgs = [cfg]

# Set all available pins to SPI for GPIO0 and GPIO1
mbc.set_gpio_src("GPIO0", ["DB0_SPI"] * 12)
mbc.set_gpio_src("GPIO1", ["DB0_SPI"] * 12)

# Set the data direction register
for name, pin in (("clk", args.clk), ("cs", args.cs), ("sdo", args.sdo), ("sdi", args.sdi)):
    if not 0 <= pin <= 31:
        print(f"ERROR: SPI {name} pin must be in range 0..31, got {pin}.")
        sys.exit(1)

outputs = 0x0
outputs |= 1 << cfg.periph_clk
outputs |= 1 << cfg.periph_cs
outputs |= 1 << cfg.periph_sdo
radio.set_gpio_attr("GPIO", "DDR", outputs)

# Now get the SPI engine to operate on
spi = sgi.get_spi_ref(periph_cfgs)

print("Using pins:")
print(f"  Clock = {cfg.periph_clk}")
print(f"  SDI   = {cfg.periph_sdi}")
print(f"  SDO   = {cfg.periph_sdo}")
print(f"  CS    = {cfg.periph_cs}")
print()

payload = int(args.payload, 16)
max_payload_bits = 64
if not 1 <= args.length <= max_payload_bits:
    print(f"ERROR: --length must be between 1 and {max_payload_bits} bits.")
    sys.exit(1)
if not 0 <= payload < (1 << max_payload_bits):
    print(f"ERROR: --payload must fit in {max_payload_bits} bits.")
    sys.exit(1)
mask = (1 << args.length) - 1
if payload & ~mask:
    print(
        f"WARNING: Payload {payload:#x} has bits set outside of the specified "
        f"length {args.length} bits. Remaining payload {(payload & mask):#x}."
    )
payload &= mask
print(f"Writing payload {payload:#x} with length {args.length} bits.")

# The spi_config_t holds items like the clock divider and the SDI and SDO edges
spi_cfg = uhd.types.SPIConfig()
if args.clk_div is not None:
    spi_cfg.divider = args.clk_div
    spi_cfg.use_custom_divider = True
spi_cfg.miso_edge = args.miso_edge
spi_cfg.mosi_edge = args.mosi_edge

# Do the SPI transaction. There are write() and read() methods available, too.
print("Performing SPI transaction...")
if args.length > 32:
    read_data = spi.transact_spi64(0, spi_cfg, payload, args.length, True)
else:
    read_data = spi.transact_spi32(0, spi_cfg, payload, args.length, True)
print(f"Data read: {read_data:#x}")
