
========================
10GE MAC Core
========================


------------------------
1. Directory Structure
------------------------

The directory structure for this project is shown below.

.
|-- doc                 - Documentation files
|
|-- rtl
|   |-- include         - Verilog defines and utils
|   `-- verilog         - Verilog source files for xge_mac
|
|-- sim
|   |-- systemc         - SystemC simulation directory
|   `-- verilog         - Verilog simulation directory
|
`-- tbench
    |-- systemc         - SystemC test-bench source files
    `-- verilog         - Verilog test-bench source files



------------------------
2. Simulation
------------------------

There are two simulation environments that can be used to validate the code.
The verilog simulation is very basic and meant for those who want to look
at how the MAC operates without going through the effort of setting up SystemC.
The SystemC environment is more sophisticated and covers all features of the MAC.



------------------------
2.1 Verilog Simulation
------------------------

To run the verilog simulation, compile all project files under rtl/verilog along with
top level testbench file:

  - tbench/verilog/tb_xge_mac.v

There is a Modelsim "do" file called "sim.do" under sim/verilog for those using Modelsim.
Once all the files are compiled, start simulation using entity "tb".


The verilog simulation reads packets from "packet_tx.txt" and writes them to the MAC
transmit fifo using the packet transmit interface (pkt_tx_data). As frames become
available in the transmit fifo, the MAC calulates the CRC and sends them out on xgmii_tx.
The xgmii_tx interface is looped-back to xgmii_rx in the testbench. The frames are thus
processed by the MAC receive engine and stored in the receive fifo. The testbench reads
frames from the receive interface (pkt_rx_data) and prints out the results.



------------------------
2.2 SystemC Simulation
------------------------

In order to use the SystemC environment it is required to first install SystemC from
www.systemc.org. Free membership may be required to download the core SystemC files.

The testbench was developed and tested with Verilator, a free HDL simulator that
compiles verilog into C++ or SystemC code. You can download Verilator from
www.veripool.org. You also need to install SystemPerl and Verilog-Perl for waveform
traces.


Once all the required tools are installed:

  - Move to directory sim/systemc

  - Type "./compile.sh"

  - Type "./run.sh"


If the simulation is running correctly you should see messages from the scoreboard
as packets are transmited and received on the various interfaces.

Simulation output:

    -----------------------
    Packet size 
    -----------------------
    SCOREBOARD XGMII INTERFACE TX (60)
    SCOREBOARD XGMII INTERFACE TX (60)
    SCOREBOARD PACKET INTERFACE TX (50)
    SCOREBOARD XGMII INTERFACE TX (60)
    SCOREBOARD PACKET INTERFACE TX (51)
    SCOREBOARD XGMII INTERFACE TX (60)
    SCOREBOARD PACKET INTERFACE RX (TX SIZE=60  RX SIZE=60)
    ...



