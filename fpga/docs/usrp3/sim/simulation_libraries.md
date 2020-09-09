# Simulation Libraries

Several simulation libraries are available for use in your testbenches, in the
form of SystemVerilog packages, classes, and interfaces. The bus functional
models (BFM) are implemented as SystemVerilog classes and use SystemVerilog
interfaces to connect to your device under test (DUT). These classes provide
member functions and tasks for communicating with the BFMs.

A few commonly used SystemVerilog packages are described below. See each
package file for additional documentation.

## PktTestExec

`PktTestExec.sv` contains utilities for testbench reporting, assertions, and
simulation timeouts. The associated header file, `test_exec.svh`, contains
macros for use with PkgTestExec. These macros are used to implement
SystemVerilog assertions. The header also defines `timeunit` and
`timeprecision`.

**Note:** The `timeunit` must be `1ns` in order for PkgTestExec to use
and report times correctly.

### PkgTestExec Tasks and Functions

Below are some of the methods available in PkgTestExec. See `PkgTestExec.sv`
for additional documentation.

- <b>`start_tb(string tb_name, realtime time_limit = 10ms)`</b><br>
Called at the start of the testbench. A time limit can be specified to prevent 
the testbench from never ending.
- <b>`end_tb(bit finish = 1)`</b><br>
Called at the end of the testbench. Displays final results and optionally calls
`$finish`.
- <b>`start_test(string test_name, realtime time_limit = 0)`</b><br>
Called at the start of a test. A time limit can be given to cause an error if
the test takes longer than expected or never ends.
- <b>`end_test(int test_result = 1)`</b><br>
Called at the end of a test. A pass (1) or fail (0) can be passed to indicate
if the test passed or failed. Any failed assertion, using the `test_exec.svh`
macros, will also be considered when deciding if the test passed or failed. If
any fatal or error assertions failed during the test then it will be considered
to have failed.
- <b>`start_timeout(`<br>
     &nbsp;&nbsp;&nbsp;&nbsp;`output timeout_t  handle,`<br>
     &nbsp;&nbsp;&nbsp;&nbsp;`input  realtime   timeout_delay,`<br>
     &nbsp;&nbsp;&nbsp;&nbsp;`input  string     message = "Timeout",`<br>
     &nbsp;&nbsp;&nbsp;&nbsp;`input  severity_t severity = SEV_ERROR)`<br></b>
Called to start a timeout countdown. If the timeout is not ended before the
indicated simulation time elapses then an assertion of the given severity will
be thrown. This is very useful for ensuring that tests complete in a timely
manner and to report which timeout was exceeded.
- <b>`end_timeout(timeout_t handle)`</b><br>
Called to end a timeout countdown when it is no longer needed.

### Macros

The following macros are defined in `test_exec.svh`. These update internal
variables to track the state of the testbench and to report the final results.

- <b>`ASSERT_FATAL(EXPR, MESSAGE)`</b><br>
Encapsulates a SystemVerilog $assert with a $fatal severity.
- <b>`ASSERT_ERROR(EXPR, MESSAGE) `</b><br>
Encapsulates a SystemVerilog $assert with a $error severity.
- <b>`ASSERT_WARNING(EXPR, MESSAGE)`</b><br>
Encapsulates a SystemVerilog $assert with a $warning severity.
- <b>`ASSERT_INFO(EXPR, MESSAGE)`</b><br>
Encapsulates a SystemVerilog $assert with an $info severity.

Where:
- <b>`EXPR`</b> is the condition for the assertion (what you expect to be true)
- <b>`MESSAGE`</b> is the message string to report if the assertion fails

## PkgChdrUtils

The `PkgChdrUtils` package includes various definitions and functions for
interacting with the RFNoC network protocol, called the Condensed Hierarchical
Datagram for RFNoC (CHDR). See `PkgChdrUtils.sv` for additional documentation.

## PkgChdrData

The `PkgChdrData` package contains the CHDR and item data types, as well as
utilities, that are useful for interacting with RFNoC data. An *item* refers to
an RF data sample or whatever unit of data an RFNoC block expects.

For example, the CHDR protocol data word type (`chdr_word_t`) and the RF sample
data type (`item_t`) can be defined using the following example:

    localparam CHDR_W = 64;    // CHDR bus width
    localparam ITEM_W = 32;    // RF data sample size (sc16)
    typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;
    typedef ChdrData #(CHDR_W, ITEM_W)::item_t      item_t;

SystemVerilog queues are used to store CHDR words and data samples. A queue of
CHDR words or data samples can be reorganized into queues of different data
widths using the following example:

    chdr_word_t chdr_words[$];  // Queue of CHDR packet words
    item_t      samples[$];     // Queue of RF data samples
    logic [7:0] bytes[$];       // Queue of bytes
    
    // Convert the CHDR words to samples
    samples = ChdrData#(CHDR_W, ITEM_W)::chdr_to_item(chdr_words);
    
    // Convert the samples to CHDR words
    chdr_words = ChdrData#(CHDR_W, ITEM_W)::item_to_chdr(samples);
    
    // Convert the samples to a queue of bytes
    bytes = ChdrData#(ITEM_W, 8)::chdr_to_item(samples);

    // Convert the bytes to a queue of samples
    bytes = ChdrData#(ITEM_W, 8)::item_to_chdr(samples);

## PkgRfnocBlockCtrlBfm

The `PkgRfnocBlockCtrlBfm` package contains the `RfnocBlockCtrlBfm` bus
functional model (BFM) used to emulate a software block controller for an RFNoC
block. This is the BFM used to interact with an RFNoC block in simulation. See
the \ref md_usrp3_sim_writing_sim_top "testbench example" for an example
of how to instantiate and connect the BFM to your DUT.

To be used, the BFM must be connected to the RFNoC block using an
`RfnocBackendIf` interface for the RFNoC backend interface and `AxiStreamIf`
interfaces for the AXIS-CHDR ports.

Once connected, several member functions are available through the BFM. A few
examples are shown below, but many more are available. Refer to
`PkgRfnocBlockCtrlBfm.sv` for additional documentation.

- <b>`RfnocBlockCtrlBfm::run()`</b><br>
Start the BFMs running. This should be called at the start of the testbench
after `start_tb()` and before the BFM is used.
- <b>`RfnocBlockCtrlBfm::reg_read(input  ctrl_address_t addr, output ctrl_word_t word)`</b><br>
Read a register from the RFNoC block.
- <b>`RfnocBlockCtrlBfm::reg_write(ctrl_address_t addr, ctrl_word_t word)`</b><br>
Write to a register on the RFNoC block.
- <b>`send_items(int port, item_t items[$], chdr_word_t metadata[$] = {}, packet_info_t pkt_info = 0)`</b><br>
Enqueue a packet of samples (or other data items) to be sent by the BFM to the
RFNoC block on the indicated block port.
- <b>`recv(int port, output chdr_word_t data[$], output int data_bytes)`</b><br>
Recv a packet of samples (or other data items) from the indicated port on the
RFNoC block.
- <b>`set_master_stall_prob(int stall_probability = DEF_STALL_PROB)`</b><br>
Set the probability, as value from 0-100, of the BFM's AXI-Stream master
stalling (deasserting TVALID) between transfers.
- <b>`set_slave_stall_prob(int stall_probability = DEF_STALL_PROB)`</b><br>
Set the probability, as value from 0-100, of the BFM's AXI-Stream slave
stalling (deasserting TREADY) on a given clock cycle.

The level of push-back on the flow control used by the AXI-Stream interfaces of
the BFM are controlled by setting a probability. This allows you to easily test
underflow and overflow tolerance in your testbenches.

Low-level tasks and functions are also available for inputting raw packets
directly.

## sim_clock_gen

The `sim_clock_gen` module makes it easy create clocks and synchronous resets,
and to interact with them. See `sim_clock_gen.sv` for additional documentation.

Some features include:

- Start and stop the clock
- Change the frequency
- Change the duty cycle
- Kill the clock (prevent any new simulation events)
- Wait for \em n rising/falling clock edges

## PkgAxiStream

The `PkgAxiStreamBfm` package contains the `AxistreamPacket` and `AxiStreamBfm`
classes which are used to model AXI4-Stream interfaces in the testbenches.
`AxiStreamIf` is the interface used for AXI-Stream. The RFNoC CHDR BFMs inherit
from AxiStreamBfm in order to implement the AXI-Stream interfaces used by
RFNoC.

The AXI-Stream BFMs provide typical SystemVerilog `put()`, `get()`,
`try_put()`, and `try_get()` tasks and functions for interacting with the
models. Many other useful functions are also available. See the package file
for details.