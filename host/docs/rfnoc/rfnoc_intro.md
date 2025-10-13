\page page_rfnoc_intro Overview

\tableofcontents

# What is RF Network-on-Chip (RFNoC™)?

RFNoC is a heterogeneous processing framework that can be used to implement high throughput DSP in the FPGA, for Software Defined Radio (SDR) systems, in an easy-to-use and flexible way. RFNoC and GNU Radio can be used to implement heterogenous DSP systems that can span CPU-based hosts, embedded systems and FPGAs.

RFNoC can be used to implement DSP "flow-graphs" where DSP algorithms and IP blocks are represented as nodes in the graph and the data-flow between them as edges. RFNoC, which is a network-on-chip architecture, abstracts away the setup associated with the nodes and edges of the graph and provides seamless and consistent interfaces to implement IP in the FPGA and software.

# RFNoC Basics


As a network-on-chip architecture, RFNoC employs the following design philosophies for its choice of topology, routing, flow and microarchitecture.

## Components

RFNoC flow graphs have the following components:

- *NoC Block*: A core processing block that implements user-defined IP like DSP, radio communication, hardware communication, etc.

- *Stream Endpoint*: A block that serves at the starting point or termination point for a data or control stream.

- *Transport Adapter*: An abstraction for physical transports like Ethernet, USB, PCIe, etc. Transport adapters are typically specific to the hardware that RFNoC is running on.

- *Routers*: Modules that connect NoC Blocks, Stream Endpoints and Transport Adapters to allow the user to build a DSP flow-graph.

Each NoC block has two communication planes: 1) Data and 2) Control. The control plane is used for setup and configuration and is assumed to be a low-throughput transaction-based interface. The data plane is a high-throughput streaming interface for samples, bits, etc. It is possible to inject optional, high-throughput metadata into the data-plane.

## Topology

The topology is defined as the set of connections between the various RFNoC components. The topology of an RFNoC network is completely user-defined, given that the network meets the bandwidth and resource requirements of the underlying hardware. RFNoC allows the user to connect their own DSP blocks to the available Ettus Research SDR-specific blocks in a flexible and arbitrary fashion to create any custom flow graph. RFNoC also provides the ability to reconfigure the graph within certain user specified constraints. Reconfigurability can fall into the following categories:

1.  *Run-time Reconfiguration*: A part of the topology can be modified at runtime by changing software settings or physical connections between USRPs and FPGA accelerators. Run-time reconfiguration allows the software application to change the topology dynamically.

2.  *Build-time Reconfiguration*: A part of the topology is hard-coded into the FPGA image and requires an FPGA rebuild (or partial bitstream download using partial reconfiguration) to reconfigure.

3.  *No Reconfiguration*: There are hard-coded connections, primarily due to hardware design decisions, that do not allow certain parts of the topology to be modified.

Run-time reconfiguration provides the most flexibility but has a higher implementation cost in terms of FPGA resources and upper limits on processing blocks. Build-time reconfiguration provides less flexibility but reduces some of the resource costs. RFNoC allows users to choose between build-time and run-time topology reconfiguration. Automated tools and scripts will allow users to make these tradeoffs in an easy-to-use way.

## Routing

The routing backbone in RFNoC is responsible for moving data from block to block using a clearly defined strategy. RFNoC uses the following routing strategies for the control and data plane.

- *Source Routing*: A routing algorithm that chooses the entire path at the source. For source routing to be possible, the source must know every hop that a transaction will take and the local router port at each hop. This is different from, say, distributed or incremental routing, where the transit decision is taken locally at each router instead of globally.

- *Deterministic Routing*: If there are two paths from the source to the destination, then the source routing algorithm will pick the path deterministically.

- (*Data Only*) *Circuit Switched*: A circuit (a path between a source and destination) must be established and reserved when a stream between two ports on NoC blocks is active. When a circuit is reserved, the source port cannot talk to a different destination.

- (*Control Only*) *Packet Switched*: Any NoC block can send and receive control transactions from any other NoC block without restrictions. The source and destination are encoded in the packet.

## Flow

The smallest unit of transfer in RFNoC is a packet or datagram, the Condensed Hierarchical Datagram for RFNoC (CHDR). Both data-plane and control-plane traffic is packetized in the CHDR format, and the packet-type is encoded within the packet. Data streams are always bidirectional. Within the FPGA, data flows in AMBA AXI4-Stream packets and uses the standard ready/valid flow control scheme (flit-buffer flow control). For lossy transports, the stream endpoint implements a high-level flow control scheme which is packet based (packet-buffer flow control).

# The RFNoC Flow Graph

As shown in the figure below an RFNoC flow graph has the following major components:

- NoC Blocks

- Stream Endpoints

- Transport Adapters

- Routing Core (Routers and Crossbars).

\anchor a_typical_rfnoc_flow_graph_anchor
\image html rfnoc_fg_typ.png "A typical RFNoC flow graph." width=900px


## NoC Block

A NoC block contains the core processing IP (user logic) sandboxed from the rest of the blocks and from the framework. The user logic interacts with the RFNoC infrastructure using the NoC Shell module. The NoC shell provides a separate control and data interface that the user logic can use to send and receive control transactions and processing data, respectively. The details of each interface will be covered in later sections. A NoC block may also interface with outside logic or IO that is unmanaged by RFNoC. An RFNoC flow graph can have at most about 1000 NoC blocks per device (if they fit in the FPGA). *This maximum number of ports in each FPGA is limited by a 10-bit address field which is shared for blocks, stream endpoints and transports.*

## Stream Endpoint

A stream endpoint serves as the start and end for a unique sample stream. The number of stream endpoints in a USRP design must scale with the number of parallel streams of data to/from the device. A stream endpoint can exist in the FPGA or in software. A bidirectional stream can be initiated between any two endpoints dynamically at any point in the application. Streams can be destroyed and recreated without having to rebuild or partially reconfigure the FPGA image. RFNoC implements flow control between stream endpoints, so they can flow over any transport. An RFNoC flow graph can have a user-selectable number of stream endpoints. The number of stream endpoints is independent of the number NoC blocks. The stream endpoint can optionally support multiple virtual streams that are multiplexed through the same physical transport. The multiplexing and demultiplexing will be performed by the framework using the "virtual channel" field in the packet header.

## Transport Adapter

A transport adapter is a wrapper around a specific transport implementation like Ethernet, Aurora, PCI Express, etc. Transport adapters provide logic to enable RFNoC-formatted dataflow between two FPGAs, or FPGA and software in a hardware-transparent way. The number of stream endpoints is independent of the number of transport adapters; a transport is capable of multiplexing multiple streams of data.

## Routing Core

The routing core handles connecting NoC blocks, stream endpoints and transport adapters. The routing core has three main routers:

- *CHDR Crossbar*: This crossbar is a full-bandwidth full-mesh dynamic crossbar. It connects the transport adapters to the stream endpoints. The CHDR crossbar enables communication within an FPGA between any two of its crossbar ports. This allows communication between two stream endpoints or between a stream endpoint and another FPGA through a transport adapter.

- *Control Crossbar*: This crossbar is a local crossbar, also full-mesh, but with reduced bandwidth. It allows control transactions to be sent between any two of its ports. This allows control transactions to be sent from software to a NoC block, from a NoC block to software, between two NoC blocks, or from a NoC block to another FPGA.

- *Static Router*: The static router encodes a fixed topology between data ports of NoC blocks. This topology can only be reconfigured by rebuilding the FPGA image. A static router requires significantly fewer FPGA resources than a dynamic router.

## Example Topology

\anchor example_topology_for_a_multi_usrp_comaptible_image_anchor
\image html rfnoc_fg_x310.png "Example topology for a multi_usrp compatible image (X310_XG)." width=900px

This \ref example_topology_for_a_multi_usrp_comaptible_image_anchor "Figure" shows an example topology for the USRP-X310 that can function with the multi_usrp API and a UBX daughter board (i.e., it has all the necessary radio and DSP blocks to implement the API). This design has:

- *3 transport adapters*: 2 for 10 GigE and one for PCIe

- *2 stream endpoints*: Each X310 supports 2 UBX daughter boards with a total 2 transmit and 2 receive channels. So, we instantiate 2 (bidirectional) stream endpoints.

- *8 NoC blocks*: We have 2 each of the radio, DMA FIFO, DDC and DUC blocks. Together they form 4 chains (subgraphs). These chains hook up two of the 4 ports (TX and RX) of the stream endpoints.

- *2 crossbars*: The 8 blocks in this image tunnel through 2 stream endpoints into the 5-port CHDR crossbar. The control crossbar has 11 ports for full control connectivity between blocks and endpoints.

# Workflow

The primary goal for RFNoC is to allow DSP engineers to build heterogenous applications that may be comprised of standard blocks provided by Ettus Research, as well as custom user-authored blocks. The framework provides tools to 1) allow users to create custom blocks and 2) assemble an FPGA image and a software application that uses standard or custom blocks.

The general workflow for a user to build an RFNoC application thus is:

1.  Partition the DSP/algorithm problem into software components and FPGA components (this can be done iteratively).

2.  For the FPGA components, partition the problem into basic functions i.e. blocks.

3.  Identify if any of the basic functions (blocks) are already available. Blocks can be found in the standard Ettus Research repositories or in third-party/open-source repositories.

4.  Develop the FPGA and software source code for each new block using the tools provided by RFNoC.

    a.  FPGA: Develop the core acceleration algorithm in Verilog, VHDL, SystemVerilog or Vivado HLS

    b.  FPGA: Write testbenches for the block using the RFNoC framework

    c.  Software: Write a block definition and an optional C++ controller to command and control the FPGA block from UHD software

5.  Use the provided RFNoC tools to assemble an FPGA image that contains all the necessary blocks to implement the desired application.

    a.  Connections between blocks can be fixed in the FPGA (for performance) or dynamic (for flexibility)

    b.  Blocks can also be connected to transports on the USRP to build multi-FPGA applications

6.  Once an FPGA image is ready, write an application in UHD (in C, C++ or Python) or GNU Radio to control and connect the dynamic blocks in the design to implement the desired application.

Usage Guidelines

- The user develops individual blocks, so the user interface in the FPGA and software will be abstracted at a block level.

- Blocks have a control and data plane, and those planes will be the primary interface points in the software and the FPGA.

- To build an application, the user must compose blocks in a specified topology, so the framework will provide tools to do so on the FPGA and provide APIs in the software to build a graph of blocks.

- Device specific details and the board support package for a USRP will be abstracted away by the framework.

*NOTE: RFNoC has several features that are marked as "advanced" that may be disabled or not exposed in the standard interfaces for performance or efficiency reasons. The advanced features will allow users to implement more complex applications but that may require detailed understanding of the framework.*