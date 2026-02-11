\page page_rfnoc_fpga RFNoC FPGA Specification

\tableofcontents

# Basics

## Block Capabilities

Fundamentally, an RFNoC block has three main types of interfaces:

- *Control*: A transaction-based interface that can be used for low-speed control through software or other blocks. The three basic transaction types are *register read*, *register write* and *bus sleep*. More complex transactions are possible, but most applications should be possible with the basic three. All transactions on this interface can be deterministic and executed at user-specified times.

- *Data*: A streaming interface that can be used for high-speed and low-latency data movement between blocks. This interface also supports deterministic and timed streaming with optional (advanced) capabilities to insert inline metadata in the stream.

- *External*: A block may need access to other IO in addition to control and data. Blocks that control USRP hardware (advanced) can have access to low-level pins. Blocks can also get access to time to implement hardware timed operations. More advanced blocks can get access to user-defined IO ports. *Most RFNoC processing blocks will not need the miscellaneous interfaces.*

Each block has *one* bidirectional slave (or master and slave) control interface, *zero or more* data ports and *zero or more* external IO.

RFNoC is a network-on-chip and has a packetized transport network. Utilities are available to abstract packets into simple interfaces (discussed later), however the understanding of the data flow and packet formats should allow users to build better and more efficient applications. RFNoC provides the following capabilities for the control and data planes:

### Control-Plane Capabilities

- The control plane is transaction based. RFNoC has pre-defined transactions like reads, writes and sleeps, but it is possible to add more transactions (advanced). Transactions have a bit width of 32 bits and each transaction has a 20-bit address and a payload of up to eight 32-bit data words.

- Transactions are blocking and have an optional execution status.

- Transactions can be executed immediately or have an associated timestamp for deterministic execution or alignment with data samples.

- Any block can send transactions to any other block at any time. Blocks within an FPGA have connectivity through a control crossbar, so other blocks can be addressed with a 10-bit "port" whereas blocks on remote FPGAs can be addressed through a stream endpoint by specifying an "endpoint-ID" and a "port" on the remote FPGA.

### Data-Plane Capabilities

- The data plane has a streaming interface where it is possible to stream "bursts" of "vectors" of "items". An *item* is defined as a single atomic data word with a user-defined bit-width (e.g., a common item would be an RF data sample). A *vector* is a 1-dimensional collection of items. A *burst* is a collection of vectors.

- Data is received in packets which is independent of the items, vectors and bursts. The parameters of a packet, like the size, are hardware dependent and can be used to make low-level throughput/latency tradeoffs.

- Each packet can have user-defined metadata (advanced)

- Each packet can have a 64-bit timestamp (which is a counter in a time-base clock domain)

- It is possible to build a sequence of packets using an embedded sequence number field.

## Integration with USRP Hardware

RFNoC provides seamless integration with USRP Hardware. As an SDR, a USRP has the following external input/output interfaces:

- ADCs/DACs

- RF Signal Chain Control

- Memory Interfaces (DDR, SRAM, etc.)

- Digital IO

- Transports (Ethernet, PCIe, etc.)

Each USRP will come equipped with NoC Blocks that seamlessly connect to the above IO. Transports will have corresponding transport adapters. It is possible to reassign that IO to other blocks in the design but that is an advanced feature.

# RFNoC Packet Network

Before looking at the FPGA interfaces, it is important to understand how data flows between blocks and stream endpoints. With the provided RFNoC tools, it is possible to choose between a simple interface that abstracts the data movement or a low-level interface that gives the block full control (and responsibility).

## CHDR Overview

The Condensed Hierarchical Datagram for RFNoC (CHDR) is a protocol that defines the fundamental unit of data transfer in an RFNoC network. As shown in the table below, it has a header that encodes packet info, routing info, metadata and the data payload. CHDR is used as a transport protocol between stream endpoints. CHDR can handle control, data, flow control and status messages. The format is dependent on the width of the CHDR bus in the FPGA (CHDR_W). *NOTE: CHDR_W can be a power of 2 that is equal to or greater than 64 bits.*

\anchor memory_layout_of_a_chdr_packet_anchor
<div align="center">
<table>
  <caption>Memory layout of a CHDR packet.</caption>
    <tr>
      <th align="center">#</th>
      <th align="center" colspan="8"> Memory Layout <br> `<--------------` CHDR_W = 64 bits `------------->`</th>
      <th align="center">Required?</th>
    </tr>
    <tr>
      <td align="center">0</td>
      <td align="center">VC <br> (6)</td>
      <td align="center">EOB <br> (1)</td>
      <td align="center">EOV <br> (1)</td>
      <td align="center">PktType <br> (3)</td>
      <td align="center">NumMData <br> (5) <br> Value=M</td>
      <td align="center">SeqNum <br> (16)</td>
      <td align="center">Length <br> (16) <br> Value=L</td>
      <td align="center">DstEPID <br> (16)</td>
      <td align="center">Y</td>
      </tr>
    <tr>
      <td align="center">1</td>
      <td align="center" colspan="8">Timestamp (64)</td>
      <td align="center">N</td>
    </tr>
    <tr>
      <td align="center">2</td>
      <td align="center" colspan="8">Metadata[0] (CHDR_W)</td>
      <td align="center">N</td>
    </tr>
    <tr>
      <td align="center">.</td>
      <td align="center" colspan="8">...</td>
      <td align="center">.</td>
    </tr>
    <tr>
      <td align="center">M+1</td>
      <td align="center" colspan="8">Metadata[M-1] (CHDR_W)</td>
      <td align="center">N</td>
    </tr>
    <tr>
      <td align="center">M+2</td>
      <td align="center" colspan="8">Payload[0] (CHDR_W)</td>
      <td align="center">Y</td>
    </tr>
    <tr>
      <td align="center">.</td>
      <td align="center" colspan="8">...</td>
      <td align="center">.</td>
    </tr>
    <tr>
      <td align="center">M+N+1</td>
      <td align="center" colspan="8">Payload[N-1] (CHDR_W)</td>
      <td align="center">N</td>
    </tr>
 </table>
 </div>

The individual fields are described in detail in the table below.

\anchor chdr_field_descriptions
<div align="center">
<table>
  <caption>CHDR field descriptions.</caption>
    <tr>
      <th>Field</th>
      <th>Width</th>
      <th>Description</th>
      <th>Type</th>
    </tr>
    <tr>
      <td> Virtual <br> Channel <br> (VC)</td>
      <td> 6</td>
      <td> The virtual channel number for a stream. It is possible to <br>
          have multiple virtual streams flowing over the same <br> 
          physical stream (EPID-pair). This field identifies the <br>
          index of the virtual stream. The default value of this field <br>
          is zero. <br>
          *NOTE: Any virtual streams that are incorrectly <br>
          addressed will go to port 0.*
       </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> Delimiters <br> (EOV/EOB)</td>
      <td> 2</td>
      <td> Delimiter flags for the user logic to use. These bits are <br>
          unused by the core framework but have the following <br>
          definitions: <br>
          <ul>
            <li> Delimiter[0] = EOV (End of Vector) </li>
            <li> Delimiter[1] = EOB (End of Burst) </li>
          </ul>
          <br>
          *NOTE: Data in RFNoC has three kinds of delimiters: <br>
          1) Packets, 2) Vectors and 3) Bursts. A vector is a <br>
          collection of packets (of items), and a burst is a <br>
          collection of vectors.*
       </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> PktType </td>
      <td> 3</td>
      <td> The type of this CHDR packet. Can be one of the following: <br>
           0x0 = Management  <br>
           0x1 = Stream Status <br>
           0x2 = Stream Command <br>
           0x3 = \[Reserved\] <br>
           0x4 = Control Transaction <br>
           0x5 = \[Reserved\] <br>
           0x6 = Data Packet without a Timestamp <br>
           0x7 = Data Packet with a Timestamp
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> NumMData</td>
      <td> 5</td>
      <td> The number of metadata words in this packet. Each  <br>
           metadata word is CHDR_W bits wide. If NumMData is <br>
           zero, then the packet has no metadata. The maximum <br>
           value for NumMData is 30. <br>
           *NOTE: Metadata is considered to be an advanced <br>
           feature of RFNoC, and its interpretation is assumed to <br>
           be block-specific. The framework will provide the ability <br>
           for the user logic to extract and insert metadata into a <br>
           packet but the user logic in the block is responsible for <br>
           defining its format.*
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> SeqNum</td>
      <td> 16</td>
      <td> Packet sequence number. The value shall start at 0 and <br>
          increment by 1 for every packet of a given type in a  <br>
          stream. The counter shall roll over to 0 after 65535  <br>
          (2<sup>16</sup>-1). <br>
          *NOTE: The sequence number is useful for detecting  <br>
          gaps and reordering issues in a stream. During error- <br>
          free operation, the sequence number will increase  <br>
          monotonically (by 1) for every packet for each:
          <ul>
             <li>Stream (unique source and dest. endpoints) </li>
             <li>Packet type (Data packets with and without <br>
                 timestamps are considered the same type with <br>
                 respect to the sequence number.) 
              </li>
          </ul>
          The sequence should thus be independently monotonic  <br>
          for each stream and each packet type. A gap in the  <br>
          sequence number at any point is considered a sequence  <br>
          error.*
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> Length</td>
      <td> 16</td>
      <td> Length of the packet in bytes. This includes the header, <br>
           timestamp, metadata and payload.</td>
      <td> Required</td>
    </tr>
    <tr>
      <td> DstEPID</td>
      <td> 16</td>
      <td> The Endpoint ID of the stream endpoint that this packet <br>
           is destined for. The EPID is used to make routing <br>
           decisions. <br>
           (The details of routing are covered in the following <br>
           sections) <br>
           *NOTE: EPID = 0 is reserved and may not be used.*
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> Timestamp</td>
      <td> 64</td>
      <td> A 64-bit integer timestamp for the payload in the packet. <br>
           This field is valid only when the packet type is “Data <br>
           Packet with a Timestamp” 
      </td>
      <td> Optional</td>
    </tr>
    <tr>
      <td> Metadata</td>
      <td> Variable</td>
      <td> User-defined metadata. These bits are unused by the <br>
           core framework and their format is undefined. The <br>
           definition of the format can be block-specific.
      </td>
      <td> Optional</td>
    </tr>
    <tr>
      <td> Payload</td>
      <td> Variable</td>
      <td> User-defined payload <br>
           *NOTE: Every CHDR packet must have at least one line <br>
           of payload.*
      </td>
      <td> Required</td>
    </tr>
 </table>
 </div>




The memory layout for various CHDR widths and configurations is shown below.

<div align="center">
<table>
  <caption> Memory layout for CHDR_W = 64 (Example without a timestamp and 2 metadata words).</caption>
    <tr>
      <th align="center">Byte</th>
      <th align="center">
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         CHDR_W = 64
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         </th>
    </tr>
    <tr>
      <td align="center"> 0 </td>
      <td align="center"> HEADER (64)</td>
    </tr>
    <tr>
      <td align="center"> 8 </td>
      <td align="center"> METADATA\[0\] </td>
    </tr>
    <tr>
      <td align="center"> 16 </td>
      <td align="center"> METADATA\[1\]</td>
    </tr>
    <tr>
      <td align="center"> 24 </td>
      <td align="center"> PAYLOAD\[0\]</td>
    </tr>
    <tr>
      <td align="center"> 32 </td>
      <td align="center"> PAYLOAD\[1\]</td>
    </tr>
    <tr>
      <td align="center"> ... </td>
      <td align="center"> ...</td>
    </tr>
    <tr>
      <td align="center"> ... </td>
      <td align="center"> PAYLOAD\[N-1\]</td>
    </tr>
</table>
</div>

<br>

<div align="center">
<table>
  <caption> Memory layout for CHDR_W = 64 (Example with a timestamp and 2 metadata words).
  </caption>
    <tr>
      <th align="center">Byte</th>
      <th align="center"> 
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         CHDR_W = 64
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
      </th>
    </tr>
    <tr>
      <td align="center"> 0 </td>
      <td align="center"> HEADER (64)</td>
    </tr>
    <tr>
      <td align="center"> 8 </td>
      <td align="center"> TIMESTAMP (64) </td>
    </tr>
    <tr>
      <td align="center"> 16 </td>
      <td align="center"> METADATA\[0\] </td>
    </tr>
    <tr>
      <td align="center"> 24 </td>
      <td align="center"> METADATA\[1\]</td>
    </tr>
    <tr>
      <td align="center"> 32 </td>
      <td align="center"> PAYLOAD\[0\]</td>
    </tr>
    <tr>
      <td align="center"> 40 </td>
      <td align="center"> PAYLOAD\[1\]</td>
    </tr>
    <tr>
      <td align="center"> ... </td>
      <td align="center"> ...</td>
    </tr>
    <tr>
      <td align="center"> ... </td>
      <td align="center"> PAYLOAD\[N-1\]</td>
    </tr>
</table>
</div>

<br>

<div align="center">
<table>
  <caption>  Memory layout for CHDR_W = 128 (Example with a timestamp and 2 metadata words).</caption>
    <tr>
      <th align="center">Byte</th>
      <th align="center" colspan="2">
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         CHDR_W = 128
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
      </th>
    </tr>
    <tr>
      <td align="center"> 0 </td>
      <td align="center"> TIMESTAMP (64)</td>
      <td align="center"> HEADER (64)</td>
    </tr>
    <tr>
      <td align="center"> 16 </td>
      <td align="center" colspan="2"> METADATA\[0\] </td>
    </tr>
    <tr>
      <td align="center"> 32 </td>
      <td align="center" colspan="2"> METADATA\[1\]</td>
    </tr>
    <tr>
      <td align="center"> 48 </td>
      <td align="center" colspan="2"> PAYLOAD\[0\]</td>
    </tr>
    <tr>
      <td align="center"> 64 </td>
      <td align="center" colspan="2"> PAYLOAD\[1\]</td>
    </tr>
    <tr>
      <td align="center"> ... </td>
      <td align="center" colspan="2"> ...</td>
    </tr>
    <tr>
      <td align="center"> ... </td>
      <td align="center" colspan="2"> PAYLOAD\[N-1\]</td>
    </tr>
</table>
</div>

<br>

<div align="center">
<table>
  <caption>  Memory layout for CHDR_W = 256 (Example with a timestamp and 2 metadata words).</caption>
    <tr>
      <th align="center">Byte</th>
      <th align="center" colspan="3">
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         CHDR_W = 256 or higher
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
         &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
      </th>
    </tr>
    <tr>
      <td align="center"> 0 </td>
      <td align="center"> RESERVED</td>
      <td align="center"> TIMESTAMP (64)</td>
      <td align="center"> HEADER (64)</td>
    </tr>
    <tr>
      <td align="center"> 32 </td>
      <td align="center" colspan="3"> METADATA\[0\] </td>
    </tr>
    <tr>
      <td align="center"> 64 </td>
      <td align="center" colspan="3"> METADATA\[1\]</td>
    </tr>
    <tr>
      <td align="center"> 96 </td>
      <td align="center" colspan="3"> PAYLOAD\[0\]</td>
    </tr>
    <tr>
      <td align="center"> 128 </td>
      <td align="center" colspan="3"> PAYLOAD\[1\]</td>
    </tr>
    <tr>
      <td align="center"> ... </td>
      <td align="center" colspan="3"> ...</td>
    </tr>
    <tr>
      <td align="center"> ... </td>
      <td align="center" colspan="3"> PAYLOAD\[N-1\]</td>
    </tr>
</table>
</div>

The amount of metadata in a packet depends on the NumMData field and the width of the CHDR bus. For compatibility between different CHDR widths, it is recommended to limit the amount of metadata to 248 bytes, the maximum amount supported by the smallest CHDR width, CHDR_W = 64.

## Data Packets

When the CHDR PktType field is 0x6 or 0x7, the payload is interpreted as a data packet. The data packet is the simplest type of CHDR packet because the format is flexible, and the payload is defined by the blocks generating and consuming it. When the PktType is 0x7, the header contains a valid timestamp. When the PktType is 0x6, the timestamp word is ignored. Note that when the PktType is 0x6 and CHDR_W is 64, there is no timestamp word and the first word of metadata or payload immediately follows the header word.

The stream endpoints separate control traffic from data traffic so that the *AXIS-CHDR Data* ports on the client side of the stream endpoint only carry data packets (see this \ref a_typical_rfnoc_flow_graph_anchor "Figure"). Data packets are designed to have the lowest overhead to enable low-latency and high-throughput streaming of samples.

### Timestamps and Data Bursts

The exact meaning of the timestamp field in data packets is a block-dependent feature. For example, the radio will add the current timestamp to each outgoing packet but will interpret the timestamp on incoming packets as an instruction to start sending at this time. Other blocks may also have block-specific behavior regarding timestamps. To harmonize the usage of timestamps, the following conventions should be used, where possible, to design blocks and/or software that uses timestamps:

- A burst is understood to be a contiguous string of samples or other data units. For example, the software might request 10000 samples from a radio, at a packet size of 1000 samples per packet. The burst will thus consist of 10 packets of 1000 samples each.

- The last packet of a burst **must** be tagged with an end-of-burst (EOB) marker.

- Assuming the burst is carrying timestamps, the first packet of the burst must carry the timestamp (the PktType field must be set to 0x7, and the 64-bit timestamp must be filled).

- The following packets of the burst *are not required* to carry a timestamp. The assumption is that timestamps can be calculated in the receiver, since the number of samples is known per packet.

- If mid-burst timestamps are set, then it is up to the downstream consumer to make use of them or ignore them.

  - Example: The DDC block will calculate timestamps internally within a burst. This is because the DDC typically comes directly after a radio, and thus the input to the DDC is predictable. The RX Streamer (in software) however *does* read all incoming timestamps and passes them on the user. This is because the data link between the FPGA and the host computer can be lossy (e.g., when using UDP), and thus, the host software will not assume it can internally calculate new timestamps.

- The first packet after an EOB must carry a timestamp again, if the new burst is timed.

The rationale for not requiring timestamps mid-burst is twofold: First, timestamps mid-burst are redundant, and thus leaving them out might make block designs simpler, and potentially reduce bandwidth usage. The second reason is due to the fixed-point nature of timestamps. Take the example of a radio block producing data at a rate of 200 Msps, which is in the same clock domain as the timekeeper, running at 200 MHz. Following the radio block is a fractional resampler which turns the 200 Msps into a 122.88 Msps stream. Due to the fractional relationship between input and output rates at the resampler, it will not be able to calculate mid-burst timestamps without rounding errors. The timestamp in the first packet, however, does not need to be converted, since the beginning of the packet keeps the same time regardless of the sampling rate. The redundancy of the mid-burst timestamps is thus used to avoid potential pitfalls of fixed-point rounding errors.

## Control Packets

When the CHDR PktType field is 0x4, the payload is interpreted as a control packet. The control packet encodes memory-mapped transactions. It has a variable length that can range from 16 bytes (no timestamp and NumData = 1) to 80 bytes (timestamp and NumData = 15).

The table below shows the format of the CHDR payload of a control packet. For simplicity, the rest of the CHDR packet is not shown. Note that a timestamp may be present in both the CHDR packet header and in the control packet contents. This simplifies the parsing of control and data packets.

\anchor mem_layout_chdr_payload_ctrl_anchor
<div align="center">
<table>
  <caption>  Memory layout of the CHDR payload of a control packet.
  </caption>
    <tr>
      <th align="center"> # </th>
      <th align="center" colspan="8"> Memory Layout <br>
          `<--------------` CHDR_W = 64 bits `------------->` </th>
      <th align="center"> Required? </th>
    </tr>
    <tr>
      <td align="center"> 0 </td>
      <td align="center"> Reserved <br> (16) </td>
      <td align="center"> SrcEPID<br> (16) </td>
      <td align="center"> IsACK<br> (1) </td>
      <td align="center"> HasTime<br> (1) </td>
      <td align="center"> SeqNum<br> (6) </td>
      <td align="center"> NumData<br> (4) </td>
      <td align="center"> SrcPort<br> (10) </td>
      <td align="center"> DstPort<br> (10) </td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> 1 </td>
      <td align="center" colspan="8"> Timestamp (64)</td>
      <td align="center"> N </td>
    </tr>
    <tr>
      <td align="center"> 2 </td>
      <td align="center" colspan="3"> Data\[0\] <br> (32) </td>
      <td align="center"> Status<br> (2) </td>
      <td align="center"> Reserved<br> (2) </td>
      <td align="center"> OpCode<br> (4) </td>
      <td align="center"> ByteEnable<br> (4) </td>
      <td align="center"> Address<br> (20) </td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> 3 </td>
      <td align="center" colspan="4"> Data\[2\] <br> (32) </td>
      <td align="center" colspan="4"> Data\[1\] <br> (32) </td>
      <td align="center"> N </td>
    </tr>
    <tr>
      <td align="center"> ... </td>
      <td align="center" colspan="4"> ... </td>
      <td align="center" colspan="4"> ... </td>
      <td align="center"> ... </td>
    </tr>
    <tr>
      <td align="center"> 9 </td>
      <td align="center" colspan="4"> Data\[14\] <br> (32) </td>
      <td align="center" colspan="4"> Data\[13\] <br> (32) </td>
      <td align="center"> N </td>
    </tr>
</table>
</div>

A detailed description of the fields is listed in the table below. Each control packet has the source and destination stream endpoint. The packet also has a source and destination port which allows addressing up to 1024 NoC blocks from each endpoint.

\anchor chdr_control_field_definitions_anchor
<div align="center">
<table>
  <caption>CHDR Control field definitions.</caption>
    <tr>
      <th>Field</th>
      <th>Width</th>
      <th>Description</th>
      <th>Type</th>
    </tr>
    <tr>
      <td> SrcEPID</td>
      <td> 16</td>
      <td> The ID of the stream endpoint that this packet is <br>
           originated from. <br>
           *Note: EPID = 0 is reserved*
       </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> IsACK</td>
      <td> 1</td>
      <td> Is this an acknowledgement of a transaction <br>
          completion? (See following section on ACKs)
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> SeqNum </td>
      <td> 6</td>
      <td> Packet sequence number. For each master, the <br>
           value shall start at 0, increment by 1 and roll over <br>
           to 0 after 63 (2<sup>6</sup>-1). This control-specific sequence <br>
           number is independent of the CHDR sequence <br>
           number. <br>
           *NOTE: The sequence number may not be <br>
           sequential over the wire in a multi-master case. It <br>
           will be sequential in the masters’ ingress queue <br>
           because the slave and the transport modules will <br>
           not modify it.*
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> NumMData</td>
      <td> 4</td>
      <td> Number of 32-bit lines in the Data field <br>
           *NOTE: NumData = 0 is reserved.*
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> SrcPort</td>
      <td> 10</td>
      <td> The port within the source stream endpoint that <br>
          this transaction originated from.
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> DstPort</td>
      <td> 10</td>
      <td> The port within the stream endpoint that <br>
           this transaction needs to go to 
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> Timestamp</td>
      <td> 64</td>
      <td> If the transaction is timed, then this field signifies <br> 
           the start time of the transaction. The Timestamp <br>
           word is not present if HasTime is 0.
      </td>
      <td> Optional</td>
    </tr>
    <tr>
      <td> Status</td>
      <td> 2</td>
      <td>  When IsACK is high, this field indicates <br>
            the transaction completion status: <br>
            0x0 = OKAY (Transaction successful) <br>
            0x1 = CMDERR (Slave asserted a command error) <br>
            0x2 = TSERR (Slave asserted a timestamp error)<br>
            0x3 = WARNING (Slave asserted a non-critical error)<br>
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> OpCode</td>
      <td> 4</td>
      <td> The operation code of this transaction. See <br>
           OpCode definitions below.
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> ByteEnable</td>
      <td> 4</td>
      <td> A bitmask of the bytes to keep from the Data field.
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> Address</td>
      <td> 20</td>
      <td> The byte address for the transaction.
      </td>
      <td> Required</td>
    </tr>
    <tr>
      <td> Data\[i\]</td>
      <td> Variable</td>
      <td> The transaction data. Number of data values <br>
           depends on the NumData field and their <br>
           interpretation depends on the OpCode.
      </td>
      <td> Optional</td>
    </tr>
 </table>
 </div>

A control transaction is a memory mapped transaction that contains a 20-bit Address field and a 4-bit byte-enable field (with behavior similar to tkeep/tstrb in AXI4). It may have one to fifteen 32-bit data fields. A transaction can be timed, i.e., only executed when the sample timestamp matches a command timestamp. The OpCode determines the behavior of the transaction. All register transactions must be acknowledged after they are consumed. The packet size of the response will be the same as the packet size of the request. Using this information, the sender is responsible for flow controlling control transactions to ensure that the control packet FIFO is not overrun.

Note that the use of some control transaction features is block-dependent. For example, some NoC blocks may ignore ByteEnable and/or the Timestamp if those blocks do not support those features. This allows NoC blocks to be simpler if such features are not required.

The table below shows the meaning of the OpCode field values.

<div align="center">
<table>
  <caption>OpCode definitions for control transactions.</caption>
    <tr>
      <th>OpCode</th>
      <th>Operation</th>
      <th>Arguments</th>
      <th>Description</th>
    </tr>
    <tr>
      <td> 0</td>
      <td> Sleep</td>
      <td> \[0\]: Stall cycles</td>
      <td> Do nothing and stall the control endpoint for <br>
           *Data\[0\]* clock cycles of the control interface <br>
           clock.
      </td>
    </tr>
    <tr>
      <td> 1</td>
      <td> Write</td>
      <td> \[0\]: Data</td>
      <td> Write Data to a single register at *Address* at <br>
           all bytes p where by *ByteEnable*\[p\] = 1.
      </td>
    </tr>    
    <tr>
      <td> 2</td>
      <td> Read</td>
      <td> \[0\]: Scratch</td>
      <td> Read a single register at *Address*.
      </td>
    </tr>    
    <tr>
      <td> 3</td>
      <td> Read then <br> 
           Write
      </td>
      <td> \[0\]: Data</td>
      <td> Read the register at *Address* then Write <br>
           *Data* to it at all bytes p where by <br>
           *ByteEnable*\[p\] = 1.
      </td>
    </tr>    
    <tr>
      <td> 4</td>
      <td> Block Write</td>
      <td> \[0\]: Data\[0\] <br>
           ... <br>
           \[N-1\]: Data\[N-1\] 
      </td>
      <td> Write Data[n] to registers sequentially at <br>
           (*Address + 4n*) at all bytes p where by <br>
           *ByteEnable*\[p\] = 1 where n = 0 .. N-1.
      </td>
    </tr>    
    <tr>
      <td> 5</td>
      <td> Block Read</td>
      <td> \[0\]: Scratch\[0\] <br>
           ... <br>
           \[N-1\]: Scratch\[N-1\] 
      </td>
      <td> Read sequentially from registers at <br>
           (*Address + 4n*) where n = 0 .. N-1.
      </td>
    </tr>    
    <tr>
      <td> 6</td>
      <td> Poll</td>
      <td> \[0\]: Data <br>
           \[1\]: Mask <br>
           \[2\]: Timeout
      </td>
      <td> Poll on *Address* until its value for all bits in <br>
           *Mask* matches *Data&Mask*, or until *Timeout* <br>
           cycles of control interface clock have <br>
           elapsed. Acknowledge with CMDERR if <br>
           timeout occurs, otherwise with OKAY. 
      </td>
    </tr>    
    <tr>
      <td> 7-9</td>
      <td> Reserved</td>
      <td> Reserved</td>
      <td> Reserved</td>
    </tr>    
    <tr>
      <td> >9 </td>
      <td> User Defined</td>
      <td> User Defined</td>
      <td> 6 opcodes are reserved for user-specific <br>
           implementation.
      </td>
    </tr>
 </table>
 </div>


### AXI-Stream Control (AXIS-Ctrl) Interface

The CHDR Control packet is an example of a hierarchical packet format because the control payload itself forms another packet type, called AXIS-Ctrl, that is routed through the control infrastructure. AXIS-Ctrl is a 32-bit bus which is a serialized version of the payload of a CHDR Control packet. The stream endpoint will serialize CHDR to AXIS-Ctrl, where it is passed to the control crossbar. Each NoC Block will also receive and send control transactions/responses in the AXIS-Ctrl format. The stream endpoint will then de-serialize these transactions back to CHDR.

***NOTE:*** The AXIS-Ctrl data width is always 32 bits, regardless of the value of CHDR_W.

## Control Packet Acknowledgements (ACKs)

Control packets are typically acknowledged by the consumer, e.g., an RFNoC block
receiving a control packet will send out an acknowledgement after the control
packet has been moved out of the the RFNoC block's internal control packet queue.

Acknowledgements have the exact same structure as regular control packets, with
the following requirements:

- The `IsACK` flag must be asserted
- The `Address`, `OpCode`, and `SeqNum` fields must have the same values as the
  control packet that is being acknowledged. These fields may be used to
  validate an acknowledgement packet.

## Stream Status Packets \[Internal Only\]

***NOTE: This is an internal-only packet, i.e., the NoC blocks will never see this type of packet. The RFNoC infrastructure is responsible for generating and consuming this packet type.***

When the CHDR PktType field is 0x1, the payload is interpreted as a stream status packet. Data streams in RFNoC are always bidirectional. Stream status packets always flow in the opposite direction of a data packet stream to communicate stream health and flow control information.

The following is a 64-bit serialized representation of the stream status packet. For CHDR widths larger than 64, serialization/de-serialization to 64 bits is done least-significant word first.

<div align="center">
<table>
  <caption>  Memory layout of the CHDR payload of a stream status packet.
  </caption>
    <tr>
      <th align="center"> # </th>
      <th align="center" colspan="4"> Memory Layout <br>
          `<--------------` 64-bits `------------->` </th>
      <th align="center"> Required? </th>
    </tr>
    <tr>
      <td align="center"> 0 </td>
      <td align="center"> CapacityBytes <br> (40) </td>
      <td align="center"> Reserved<br> (4) </td>
      <td align="center"> Status<br> (4) </td>
      <td align="center"> SrcEPID<br> (16) </td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> 1 </td>
      <td align="center" colspan="2"> XferCountPkts <br> (40)</td>
      <td align="center" colspan="2"> CapacityPkts <br> (24)</td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> 2 </td>
      <td align="center" colspan="4"> XferCountBytes <br> (64)</td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> 3 </td>
      <td align="center" colspan="2"> StatusInfo <br> (48)</td>
      <td align="center" colspan="2"> BuffInfo <br> (16)</td>
      <td align="center"> Y </td>
    </tr>
</table>
</div>

<br>

<div align="center">
<table>
  <caption>Stream status packet field definitions.</caption>
    <tr>
      <th>Field</th>
      <th>Width</th>
      <th>Description</th>
      <th>Type</th>
    </tr>
    <tr>
      <td> Capacity <br> bytes </td>
      <td> 40 </td>
      <td> The buffer capacity of the downstream endpoint in bytes.</td>
      <td> Required </td>
    </tr>
    <tr>
      <td> Status </td>
      <td> 4 </td>
      <td> The current status of the stream. Possible values: <br>
           0x0 = Okay (No Error) <br>
           0x1 = Command Error (Command execution failed) <br>
           0x2 = Sequence Error (Sequence number discontinuity) <br>
           0x3 = Data Error (Data integrity check failed) <br>
           0x4 = Routing Error (Unexpected destination) <br>
           Others = Reserved
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> SrcEPID</td>
      <td> 16 </td>
      <td> Endpoint ID of the source of this message <br>
           *NOTE: The endpoint ID of the destination is present in <br>
           the CHDR header.*
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> XferCount <br> Pkts</td>
      <td> 40 </td>
      <td> Number of packets received by the destination <br>
           stream endpoint.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> Capacity <br> Pkts</td>
      <td> 24 </td>
      <td> The buffer capacity of the downstream endpoint in <br>
           packets.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> XferCount <br> Bytes</td>
      <td> 64 </td>
      <td> Number of bytes received by the destination stream <br>
           endpoint. 
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> StatusInfo </td>
      <td> 48 </td>
      <td> Extended information about the status. <br>
           *NOTE: The format of this field is unspecified. It shall be <br>
           used for diagnostics only.*
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> BuffInfo </td>
      <td> 16 </td>
      <td> Extended information about the buffer state. <br>
           *NOTE: The format of this field is unspecified. It shall be <br>
           used for diagnostics only.*
      </td>
      <td> Required </td>
    </tr>
</table>
 </div>

## Stream Command Packets \[Internal Only\]

***NOTE: This is an internal-only packet, i.e., the NoC blocks will never see this type of packet. The RFNoC infrastructure is responsible for generating and consuming this packet type.***

When the CHDR PktType field is `0x2`, the payload is interpreted as a stream command. Data streams in RFNoC are always bidirectional. Stream command packets always flow in the direction of a data packet stream to trigger stream state changes.

The following is a 64-bit serialized representation of the stream status packet. For CHDR widths larger than 64, serialization/de-serialization to 64 bits is done least-significant word first.

<div align="center">
<table>
  <caption>  Memory layout of the CHDR payload of a stream command packet.
  </caption>
    <tr>
      <th align="center"> # </th>
      <th align="center" colspan="4"> Memory Layout <br>
          `<--------------` 64-bits `------------->` </th>
      <th align="center"> Required? </th>
    </tr>
    <tr>
      <td align="center"> 0 </td>
      <td align="center"> NumPkts <br> (40) </td>
      <td align="center"> OpData<br> (4) </td>
      <td align="center"> OpCode<br> (4) </td>
      <td align="center"> SrcEPID<br> (16) </td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> 1 </td>
      <td align="center" colspan="4"> NumBytes <br> (64)</td>
      <td align="center"> Y </td>
    </tr>
</table>
</div>

<br>

<div align="center">
<table>
  <caption>Stream command packet field definitions.
  </caption>
    <tr>
      <th>Field</th>
      <th>Width</th>
      <th>Description</th>
      <th>Type</th>
    </tr>
    <tr>
      <td> NumPkts </td>
      <td>  40 </td>
      <td> 
         The number of packets associated with the operation. <br>
         The exact interpretation of this field depends on the <br>
         OpCode.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> OpData  </td>
      <td> 4 </td>
      <td> 
         The data associated with the operation. The exact <br>
         interpretation of this field depends on the OpCode.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td valign="top"> OpCode </td>
      <td valign="top"> 4 </td>
      <td valign="top"> 
        A code that describes what needs to be done. <br>
        <table>
          <tr>
             <th> Value </th>
             <th> Operation </th>
          </tr>
          <tr>
             <td> 0x0 </td>
             <td> Initialize stream <br>
                  Flush buffers and reset stream state. <br>
                  <br>
                 *NOTE: When an Initialize stream command <br>
                 packet with NumBytes==0 and NumPkts==0 is <br>
                 received by the RFNoC infrastructure, one <br>
                 and only one stream status packet shall be <br>
                 sent in response. No flow control stream <br>
                 status packets shall be sent in response to <br>
                 incoming data on the given stream until an <br>
                 Initialize stream command packet with either <br>
                 NumBytes>0 or NumPkts>0 is received.*
             </td>
          </tr>
          <tr>
             <td> 0x1 </td>
             <td> Ping <br>
                  *Trigger a stream status response at endpoint.* 
             </td>
          </tr>
          <tr>
             <td> 0x2 </td>
             <td>
                Resynchronize flow control <br>
                *Use NumPkts and NumBytes to resync <br>
                flow control.*
             </td>
          </tr>
          <tr>
             <td> Others </td>
             <td> Reserved </td>
          </tr>
        </table>
      </td>
      <td valign="top"> Required </td>
    </tr>
    <tr>
      <td>  SrcEPID </td>
      <td>  16 </td>
      <td> 
         Endpoint ID of the source of this message. <br>
         *NOTE: The endpoint ID of the destination is present <br> 
         in the CHDR header.*
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td>  NumBytes </td>
      <td>  64 </td>
      <td> 
         The number of bytes associated with the operation. <br>
         The exact interpretation of this field depends on the <br>
         OpCode.
      </td>
      <td> Required </td>
    </tr>
</table>
 </div>

## Management Packets \[Internal Only\]

***NOTE: This is an internal-only packet, i.e., the NoC blocks will never see this type of packet. The RFNoC infrastructure is responsible for generating and consuming this packet type.***

When the CHDR PktType field is 0x0, the payload is interpreted as a management packet. Management packets are sent and received by internal RFNoC framework components for discovery and internal configuration. The following information can be discovered:

- The RFNoC protocol version and capabilities

- The physical connection topology including all transport endpoints and routers

A management packet can configure and discover information on the various nodes in the network. Nodes can be transport endpoints, crossbars and stream endpoints. The packet is a multi-hop transaction where operations are encoded in layers that are "peeled off" as they are consumed by the various nodes. A hop may contain several operations to execute (with a minimum of one). Each operation has an 8-bit opcode and a 48-bit payload. The interpretation of the payload is operation specific. The various opcodes defined below can allow the following:

- Discovering the RFNoC connection topology one node at a time (in DFS or a BFS manner)

- Configuring transport endpoints to setup EPID-specific settings

- Configuring stream endpoints with flow-control and other settings

Configuration is done via a basic memory mapped writes with a 16-bit address and 32-bit data. In the case of a route setup, the management packet can be configured to terminate at the stream endpoint. For other situations, it can be configured to return to the host.

The following is a 64-bit serialized representation of a management packet. For CHDR widths larger than 64, only the lower 64 bits of each management packet word are used and the upper bits will be ignored. Management packets are NOT serialized.

<div align="center">
<table>
  <caption>  Memory layout of the CHDR payload of a management packet.
  </caption>
    <tr>
      <th align="center"> # </th>
      <th align="center" colspan="5"> Memory Layout <br>
          `<--------------` 64-bits `------------->` </th>
      <th align="center"> Required? </th>
    </tr>
    <tr>
      <td align="center"> 0 </td>
      <td align="center"> ProtoVer <br> (16) </td>
      <td align="center"> CHDRWidth<br> (3) </td>
      <td align="center"> Reserved<br> (19) </td>
      <td align="center"> NumHops<br> (10) </td>
      <td align="center"> SrcEPID<br> (16) </td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> 1 </td>
      <td align="center" colspan="3"> OpPayload <br> (48)</td>
      <td align="center"> OpCode <br> (8) </td>
      <td align="center"> OpsPending <br> (8) </td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> ... </td>
      <td align="center" colspan="3"> ...</td>
      <td align="center"> ...  </td>
      <td align="center"> ...  </td>
      <td align="center"> N </td>
    </tr>
    <tr>
      <td align="center"> N-1 </td>
      <td align="center" colspan="3"> OpPayload <br> (48)</td>
      <td align="center"> OpCode <br> (8) </td>
      <td align="center"> OpsPending <br> (8) </td>
      <td align="center"> N </td>
    </tr>
</table>
</div>

<br>

<div align="center">
<table>
  <caption>Management packet field definitions.
  </caption>
    <tr>
      <th>Field</th>
      <th>Width</th>
      <th>Description</th>
      <th>Type</th>
    </tr>
    <tr>
      <td> ProtoVer </td>
      <td>  16 </td>
      <td> 
         RFNoC protocol version <br>
         The top 8 bits represent the major version, and the <br>
         bottom 8 bits represent the minor version.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> CHDRWidth  </td>
      <td> 3 </td>
      <td> 
         RFNoC CHDR bus width (CHDR_W) <br>
         0x0 = 64 bits <br>
         0x1 = 128 bits <br>
         0x2 = 256 bits <br>
         0x3 = 512 bits <br>
         Others = Reserved
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> NumHops </td>
      <td> 10 </td>
      <td> 
        Number of hops that this management packet will <br> 
        take before it is consumed completely.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> SrcEPID </td>
      <td> 16 </td>
      <td> 
        Endpoint ID of the source of this message.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> OpsPending </td>
      <td> 8 </td>
      <td> 
        Number of operations left to be executed for the  <br>
        current node/hop. Each node (hop) must have at <br>
        least one operation associated with it.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> OpCode </td>
      <td> 8 </td>
      <td> 
        Operation code (what to do) <br>
        0x0 = No-op <br>
        0x1 = Advertise <br>
        0x2 = Select Destination <br>
        0x3 = Return To Sender <br>
        0x4 = Node Info Request <br>
        0x5 = Node Info Response <br>
        0x6 = Config Write <br>
        0x7 = Config Read Request <br>
        0x8 = Config Read Response <br>
        Others = Reserved
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td valign="top"> OpPayload </td>
      <td valign="top">  48 </td>
      <td valign="top"> 
        The payload associated with the specified operation <br>
        (instruction). The format of the payload is operation <br> 
        specific.
        <table>
          <tr>
            <th> Operation </th>
            <th> Format </th>
          </tr> 
          <tr>
            <td> No-op</td>
            <td> N/A</td>
          </tr> 
          <tr>
            <td> Advertise</td>
            <td> N/A</td>
          </tr> 
          <tr>
            <td> Select <br> Destination</td>
            <td> Dest = OpPayload \[9:0\] </td>
          </tr> 
          <tr>
            <td> Return to Sender</td>
            <td> N/A </td>
          </tr> 
          <tr>
            <td> Node Info <br> Request</td>
            <td> N/A</td>
          </tr> 
          <tr>
            <td> Node Info <br> Response</td>
            <td> 
               DeviceID = OpPayload\[15:0\] <br>
               NodeType = OpPayload\[19:16\] <br>
               NodeInst = OpPayload\[29:20\] <br>
               ExtendedInfo = OpPayload\[47:30\]
            </td>
          </tr> 
          <tr>
            <td> Config Write</td>
            <td> 
               Address = OpPayload\[15:0\] <br>
               Data = OpPayload\[47:16\]
            </td>
          </tr> 
          <tr>
            <td> Config <br> Read Req</td>
            <td> Address = OpPayload\[15:0\]</td>
          </tr> 
          <tr>
            <td> Config <br> Read Resp</td>
            <td> 
              Address = OpPayload\[15:0\] <br>
              Data = OpPayload[47:16]
            </td>
          </tr>        
        </table>
      </td>
      <td valign="top"> 
        Required
      </td>
  </tr>
</table>
</div>

### Management Operations

<ul>
  <li>
      *No-op*: Do nothing. The minimum number of operations per hop is 1, and a no-op can be used to meet that requirement. 
  </li>
  <li>
      *Advertise*: The operation is effectively a no-op but it asserts a strobe that advertises the passing management packet to the outside logic. An advertisement includes the associated source and destinations EPIDs. 
  </li>
  <li>
     *Select Destination*: Select the downstream destination for this management packet. Useful for situations where a router is expected downstream but it has not been configured yet. The select destination command can be used to temporarily allocate a route to send this packet to the specified Dest port.  
  </li>
  <li>
      *Return to Sender*: Turn the packet around and return it to the sender. The return command can be coupled with a Node Info Request or a Config Read Request to allow an upstream node to query data from a downstream node. 
  </li>
  <li>
      *Node Info Request*: Request the current node/hop to return information about itself. This operation will route the packet back to the sender. 
  </li>
  <li>
      *Node Info Response*: This is the response to the above info request. Depending on NodeType, ExtendedInfo can be decoded as: <br>
      <table>
         <tr>
           <td>1 (Crossbar)</td>
           <td>
              NPorts = ExtendedInfo\[7:0\] <br>
              NPortsMgmt = ExtendedInfo\[15:8\] <br>
              ExtRtCfgPort = ExtendedInfo\[16\]
           </td>
         </tr>
         <tr>
           <td>2 (Stream Endpoint)</td>
           <td>
              AxisCtrlEn = ExtendedInfo\[0\] <br>
              AxisDataEn = ExtendedInfo\[1\] <br>
              NumDataI = ExtendedInfo\[7:2\] <br>
              NumDataO = ExtendedInfo\[13:8\] <br>
              ReportStreamErrs = ExtendedInfo\[14\] <br>
           </td>
         </tr>
         <tr>
           <td>3 (Transport Adapter)</td>
           <td>
              NodeSubtype = ExtendedInfo\[7:0\]
           </td>
         </tr>
      </table>
  </li>
  <li>
      *Config Write*: Perform a Control-Port write using the specified Address and Data. 
  </li>
  <li>
      *Config Read Request*: Request a read of the specified Address. 
  </li>
  <li>
      *Config Read Response*: The read data for the last read request. 
  </li>
</ul>


\anchor noc_block_user_interface_anchor
# NoC Block User Interface

The figure shows the anatomy of a NoC Block in the FPGA. It consists of two main components: 1) the user logic and 2) the NoC Shell. The NoC Shell is the user logic's interface to the rest of the RFNoC framework. A NoC Shell is custom generated for each block based on user-specified interface options. It is also possible to generate IO interfaces to outside logic from a NoC Block, but that feature is advanced. RFNoC provides a utility (see RFNoC ModTool below) to generate a unique instantiation of a NoC Shell that is custom for each block. Depending on the level of abstraction desired, for most interfaces, there is an option for a simple but potentially less featured interface and a low-level but full-featured interface.

\image html rfnoc_blk_anat.png "Anatomy of a NoC Block (FPGA)" width=700px

## Basic Signals

### Bus Widths

Each block can choose the CHDR width that it wishes to support. A block will generally have a fixed CHDR width and a block can only be used in designs that use the same CHDR width. In an FPGA design, the CHDR widths of all blocks and the device must be the same.

### Clocks and Resets

#### RFNoC Clocks

The following two clocks are always available for the user logic to use:

- `rfnoc_chdr_clk` <br>
  This is the clock for the `rfnoc_chdr` port (described in Section [Backend RFNoC Interface](#backend_rfnoc_interface_anchor)).

- `rfnoc_ctrl_clk` <br>
  This is the clock for the `rfnoc_ctrl` port (Section [Backend RFNoC Interface](#backend_rfnoc_interface_anchor)).

These are always-on clocks that will be used by the framework for data movement. Their frequencies are USRP device dependent.

#### RFNoC Resets

Two resets are exposed through the user interface, named `rfnoc_chdr_rst` and `rfnoc_ctrl_rst`. These resets are both synchronous to their respective clocks and are driven by the backend interface toward user logic. Both resets will assert for at least 32 of their respective clock cycles to ensure a sufficiently long reset for connected user IP. These resets should be used to reset user IP so that the entire block is reset when a reset is requested by the backend interface. A synchronizer may be used to import these resets to other clock domains, if needed.

#### User Clocks

If a block needs additional clocks, it is possible to add additional clock ports to a block. User clocks for a block must be driven by device clocks when a design is assembled. Frequency ranges can be specified on clocks to ensure that block requirements are met. RFNoC assumes asynchronous data processing so, it is not possible specify the phase or synchronization of optional clocks. If there is a need for low level synchronization with hardware or other blocks, then the advanced IO Ports must be used. These are described in Section [Generic IO Ports](#generic_io_ports_anchor).

\anchor noc_shell_generator_options_anchor
### NoC Shell Generator Options

RFNoC ModTool has the following options to generate the basic interface for a NoC block.

- CHDR Width (`chdr_width`)

  - Definition: Width of the CHDR bus

  - Options: 64, 128, 256, \...

  - Constraints: None

- Optional Clocks

  - Definition: An option that indicates if additional clocks are needed by the block

  - Options: A list of clock names and frequency ranges

  - Constraints: None

\anchor control_plane_anchor
## Control Plane 


The control-plane in the FPGA can be exposed using a low-level AXI4-Stream interface called *AXI-Stream CTRL* or using a simpler abstracted interface called *Control Port*.

### AXI-Stream Control (Low-level Interface)

AXI-Stream Control (AXIS-Ctrl) defines an interface and a packet format to encode control transactions in a standard 32-bit wide AXI-Stream bus. Regardless of the CHDR widths, AXIS-Ctrl will always be 32-bit wide. The data transferred over this interface is identical to the payload of a CHDR control packet except for the top 32 bits of the first payload line. All other fields are identical. Table \ref memory_layout_of_an_axis_ctrl_packet_anchor "Memory layout of an AXIS-Ctrl packet" shows the various fields of an AXIS-Ctrl packets formatted with a 32-bit word width. Note that the payload is identical to that of the \ref mem_layout_chdr_payload_ctrl_anchor "CHDR payload of a control packet", except for the second line in the packet. The fields are described in \ref chdr_control_field_definitions_anchor "CHDR control field definitions" and Table \ref additional_axis_ctrl_field_definitions_anchor "Additional AXIS-Ctrl field definitions".

AXIS-Ctrl packets traverse over the control network which consists of the control crossbar. This network is different for the typical CHDR network in RFNoC. It allows transactions to originate from and terminate in any NoC block in the device, despite the static data connections. The host software can issue an AXIS-Ctrl transaction going to any FPGA block and any FPGA block can send a transaction to any other FPGA block or to software. It is also possible to communicate with blocks in different devices. These are defined as *remote transactions* and require the use of two additional fields, `RemDstEPID` and `RemDstPort`.

\anchor memory_layout_of_an_axis_ctrl_packet_anchor
<div align="center">
<table>
  <caption>  Memory layout of an AXIS-Ctrl packet.
  </caption>
    <tr>
      <th align="center"> # </th>
      <th align="center" colspan="6"> Memory Layout <br>
          `<-------------- 32-bits ------------->` </th>
      <th align="center"> Required? </th>
    </tr>
    <tr>
      <td align="center"> 0 </td>
      <td align="center"> IsACK <br> (1) </td>
      <td align="center"> HasTime<br> (1) </td>
      <td align="center"> SeqNum<br> (6) </td>
      <td align="center"> NumData<br> (4) </td>
      <td align="center"> SrcPort<br> (10) </td>
      <td align="center"> DstPort<br> (10) </td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> 1 </td>
      <td align="center" colspan="2"> Reserved <br> (6)</td>
      <td align="center" colspan="2"> RemDstPort <br> (10) </td>
      <td align="center" colspan="2"> RemDstEPID <br> (16) </td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> 2 </td>
      <td align="center" colspan="6"> Timestamp\[31:0\] (32)</td>
      <td align="center"> N </td>
    </tr>
    <tr>
      <td align="center"> 3 </td>
      <td align="center" colspan="6"> Timestamp\[63:32\] (32)</td>
      <td align="center"> N </td>
    </tr>
    <tr>
      <td align="center"> 4 </td>
      <td align="center"> Status <br> (2) </td>
      <td align="center"> Reserved<br> (2) </td>
      <td align="center"> OpCode<br> (4) </td>
      <td align="center"> ByteEnable<br> (4) </td>
      <td align="center" colspan="2"> Address<br> (20) </td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> 5 </td>
      <td align="center" colspan="6"> Data\[0\] (32)</td>
      <td align="center"> Y </td>
    </tr>
    <tr>
      <td align="center"> ... </td>
      <td align="center" colspan="6"> ...</td>
      <td align="center"> N </td>
    </tr>
    <tr>
      <td align="center"> 19 </td>
      <td align="center" colspan="6"> Data\[14\] (32)</td>
      <td align="center"> N </td>
    </tr>    
</table>
</div>

<br>

\anchor additional_axis_ctrl_field_definitions_anchor
<div align="center">
<table>
  <caption>Additional AXIS-Ctrl field definitions.
  </caption>
    <tr>
      <th>Field</th>
      <th>Width</th>
      <th>Description</th>
      <th>Type</th>
    </tr>
    <tr>
      <td> REmDstEPID </td>
      <td>  16 </td>
      <td> 
         Remote Destination Endpoint ID: The ID of the <br>
         remote stream endpoint that this packet is destined <br>
         towards. <br>
         *Note: EPID = 0 implies that the transaction is local*
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> RemDstPort  </td>
      <td> 10 </td>
      <td> 
         The port index of the crossbar downstream of the <br>
         remote stream endpoint that this packet is destined <br>
         towards.
      </td>
      <td> Required </td>
    </tr>
</table>
</div>



For the NoC block interface, AXIS-Ctrl is a simple 32-bit AXI-Stream interface. Users can request this interface in a clock domain of their choice and are responsible for implementing the framer/de-framer for control packets. When the AXIS-Ctrl port is instantiated, the NoC Shell will expose the following signals for the user-logic to use:

- `axis_ctrl_clk` <br>
  This is the clock that all the control port signals are synchronous to. The user may choose which clock source drives this clock. This is an output of the NoC Shell.

- `axis_ctrl_rst` <br>
  This is the synchronous reset for the AXIS-Ctrl logic. This is an output of the NoC Shell. This reset will be asserted for at least one clock cycle after which the client logic will have 100 us to complete the following tasks:

  - Abort all pending transactions. Pending transactions may not be acknowledged

  - Reset all software configuration block state to the initial powerup/startup values

- `m_axis_ctrl_<signal>` <br>
  This is the master AXI-Stream port from which the user logic will receive all requests for incoming transactions and responses for outgoing ones. This is an output of the NoC Shell. \<signal\> refers to the following standard AXI4-Stream signals: tdata (32 bits), tvalid, tready and tlast. Each AXI-Stream packet will contain the contents of the \ref mem_layout_chdr_payload_ctrl_anchor "CHDR payload of a control packet", that the user logic will have to interpret manually.

- `s_axis_ctrl_<signal>` <br>
  This is the slave AXI-Stream port where the user logic will send all requests for outgoing transactions and responses for incoming ones. This is an input to the NoC Shell. \<signal\> refers to the following standard AXI4-Stream signals: tdata (32 bits), tvalid, tready and tlast. Each AXI-Stream packet will contain the contents of the \ref mem_layout_chdr_payload_ctrl_anchor "CHDR payload of a control packet", that the user logic will have to interpret manually.

### Control Port (Simple Interface)

The control port provides a simpler interface to generate and consume control transactions. This interface supports blocking reads/writes, timed commands, backpressure and (N)ACKs, and allows the users to not worry about parsing the AXIS-Ctrl packet. The NoC Shell will internally de-frame AXIS-Ctrl packets, post a transaction on the slave bus and then frame the response back to AXIS-Ctrl. The simplicity of the interface does yield the following limitations:

- Only the read, write and sleep (trivially) opcodes are supported

- Block reads and writes will be split into multiple single reads and writes respectively

- The priority bit is not supported

When the control port is instantiated, NoC Shell will expose the following ports for the user-logic to use:

- `ctrlport_clk` <br>
  This is the clock that all the control port signals are synchronous to. The user may choose which clock source drives this clock. This is an output of the NoC Shell.

- `ctrlport_rst` <br>
  This is the synchronous reset for the control port logic. This reset will be asserted for at least one clock cycle and the client logic will have 100 us to complete the following tasks:

  - Abort all pending transactions. Pending transactions may not be acknowledged

  - Reset all software configuration block state to the initial powerup/startup values

- `m_ctrlport_<signal>` <br>
  This is the master control port from which the user logic will receive all transaction requests and to which the user logic will send responses. A slave port is always instantiated. The table below shows the various signals represented by \<signal\>.

- `s_ctrlport_<signal>` <br>
  This is the slave control port to which the user logic will send all transaction requests and from which it will receive responses. The slave port is optional. The table below shows the various signals represented by \<signal\>

\anchor control_port_signal_definitions_anchor
<div align="center">
<table>
  <caption>Control Port signal definitions.
  </caption>
    <tr>
      <th>Signal</th>
      <th>Direction <br> (Master) </th>
      <th>Width</th>
      <th>Purpose</th>
      <th>Usage</th>
    </tr>
    <tr>
      <td> req_wr </td>
      <td> out  </td>
      <td> 1  </td>
      <td> 
         A single-cycle strobe that indicates the <br>
         start of a write transaction.
      </td>
      <td>
         Required 
      </td>
    </tr>
    <tr>
      <td>  req_rd</td>
      <td>  out </td>
      <td>  1 </td>
      <td> 
         A single-cycle strobe that indicates the <br>
         start of a read transaction.
      </td>
      <td>
         Required 
      </td>
    </tr>
    <tr>
      <td> req_addr </td>
      <td> out  </td>
      <td> 20  </td>
      <td> 
         Address for transaction. <br>
         *This field is valid only when req_rd or <br>
         req_wr is high.*
      </td>
      <td> 
         Required
      </td>
    </tr>
    <tr>
      <td> req_portid </td>
      <td> out  </td>
      <td> 10  </td>
      <td>
          Port ID within the device to send the <br>
          transaction to. This is the local port <br>
          number. <br>
          *This field is valid only when req_rd or <br>
          req_wr is high.*
      </td>
      <td> 
         Required <br> (Master <br> Only)
      </td>
    </tr>
    <tr>
      <td> req_rem_epid </td>
      <td> out  </td>
      <td> 16  </td>
      <td> 
         Endpoint ID of the stream endpoint to <br>
         send the transaction to. <br>
         *This field is valid only when req_rd or <br>
         req_wr is high.*
      </td>
      <td> 
         Required <br> (Remote Master Only)
      </td>
    </tr>
    <tr>
      <td> req_rem_portid </td>
      <td> out  </td>
      <td> 10  </td>
      <td> 
         Port ID within the stream endpoint to <br>
         send the transaction to. <br>
         *This field is valid only when req_rd or <br>
         req_wr is high.*
      </td>
      <td> 
         Required <br> (Remote Master Only)
      </td>
    </tr>
    <tr>
      <td> req_data </td>
      <td> out  </td>
      <td> 32  </td>
      <td> 
         Data for write transaction. <br>
         *This field is valid only when req_wr is high.*
      </td>
      <td> 
         Required
      </td>
    </tr>
    <tr>
      <td> req_byte_en </td>
      <td> out  </td>
      <td> 4  </td>
      <td> 
         A bitmask indicating which of the 4 <br>
         bytes to use for transaction. If bit ‘i’ is <br>
         high in keep then byte ‘i’ will be used <br> 
         from req_data. <br>
         (If not present, use all 32 bits) <br>
         *This field is valid only when req_rd or <br>
         req_wr is high.*
      </td>
      <td> 
         Optional
      </td>
    </tr>
    <tr>
      <td> req_has_time </td>
      <td> out  </td>
      <td> 1  </td>
      <td> 
         Does the transaction need to happen at <br>
         a given time?  <br>
         (If not present, perform transaction <br>
         ASAP) <br>
         *This field is valid only when req_rd or <br>
         req_wr is high.*
      </td>
      <td> 
         Optional
      </td>
    </tr>
    <tr>
      <td> req_time </td>
      <td> out  </td>
      <td> 64  </td>
      <td> 
         Timestamp to execute the transaction at. <br>
         (If not present, perform transaction ASAP) <br>
         *This field is valid only when req_rd or <br>
         req_wr is high.*
      </td>
      <td> 
         Optional
      </td>
    </tr>
    <tr>
      <td> resp_ack </td>
      <td> in  </td>
      <td> 1  </td>
      <td> 
         A strobe that indicates transaction <br>
         completion.
      </td>
      <td> 
         Required
      </td>
    </tr>
    <tr>
      <td> resp_status </td>
      <td> in  </td>
      <td> 2  </td>
      <td> 
      </td>
         The status associated with the <br>
         transaction ack. The interpretation of <br>
         these bits is defined in <br>
         \ref chdr_control_field_definitions_anchor "CHDR control field definitions". <br>
         (If not present, the value is 0 i.e. OKAY) <br>
         *This field is valid only when resp_ack is <br>
         high.*
      <td> 
         Optional
      </td>
    </tr>
    <tr>
      <td> resp_data </td>
      <td> in  </td>
      <td> 32  </td>
      <td> 
         Response data for a read transaction. <br>
         *This field is valid only when resp_ack is <br> 
         high.*
      </td>
      <td> 
         Required
      </td>
    </tr>
</table>
</div>


**READ and WRITE Transaction**

A write transaction is defined as the assertion of reg_wr for 1 clock cycle and a read transaction is defined as a similar assertion of reg_rd. The value of reg_addr and reg_data (and other optional signals) can be used as arguments for the write. An untimed write will start executing in the same cycle as the assertion of reg_wr. The example in the first figure below shows two writes (A0, A1) that execute in 0 clock cycles and one write that takes multiple cycles to execute. The second figure below shows two 0 cycle reads and one multi-cycle read.

Control-Port Transaction Rules

- After the transaction completes, the client must assert resp_ack (along with other optional response signals) to indicate transaction completion. For a read, the resp_data is used for the readback data. resp_ack must be asserted at least 1 clock cycle after the assertion of the req_wr or req_rd signal.

- It is permissible for a read or write to take multiple clocks cycles. Regardless of the execution time, the ack must be asserted 1 clock cycle after completion.

- There is no upper limit on the execution time of a transaction; this allows blocking transactions that wait on hardware, but it also requires flow control on the sender's part to guarantee that transactions don't clog upstream routers.

- After a response ACK, the ctrlport slave must be ready to receive the next transaction in the next clock cycle.

- If reg_wr and reg_rd are asserted in the same clock cycle, then the read must be executed before the write.

\image html rfnoc_td_ctrl_write.png "ctrlport write transaction" width=800px

<br>

\image html rfnoc_td_ctrl_read.png "ctrlport read transaction" width=800px

**Transaction Status**

It is possible for a control slave to acknowledge a transaction with an optional status. The status bits must have the appropriate value when resp_ack is high. This \ref read_completion_status_success_failure_anchor "Figure" shows two transaction where the first one was successful and the second one failed.

\anchor read_completion_status_success_failure_anchor
\image html rfnoc_td_read_compl.png "Read completion status (Success and Failure)" width=650px


**Timed Transactions**

A transaction (read or write) can also be timed i.e. the execution of the transaction will begin at the specified time. The optional signals req_has_time will be asserted to indicate that a transaction is timed. The contents of req_time will be used as the timestamp at which transaction execution should start. It is permissible for the transaction to take multiple clock cycles to finish executing, after which the resp_ack must be asserted. This \ref timed_write_transactions_anchor "Figure" shows three timed transactions: The first one executes immediately (because time = req_time) and executes in 1 clock cycle. The second one must wait for the time to tick up to 2000 at which point it executes (in 1 clock cycle) and asserts an ack. The third one is late and responds with a Timestamp error (TSERR).

\anchor timed_write_transactions_anchor
\image html rfnoc_td_timed_write.png "Timed write transactions" width=900px

\anchor noc_shell_generation_options_control_anchor
### NoC Shell Generation Options

RFNoC ModTool has the following options to generate the control interface for the NoC Shell of a NoC block.

- Control Interface (`fpga_iface`)

  - Definition: Which HDL interface to expose

  - Options: "AXIS-Ctrl" (`axis_ctrl`) or "Control Port" (`ctrlport`)

  - Constraints: None

- Interface Direction (`interface_direction`)

  - Definition: Direction of the interface

  - Options: "Slave Only" (`slave`), "Master and Slave" (`master_slave`), or "Remote-Master and Slave" (`remote_master_slave`)

  - Constraints: "Slave Only" not allowed for "AXIS-Ctrl" interface

- Buffer Depth (`fifo_depth`)

  - Definition: Depth of the input AXI-Stream Control FIFO in words

  - Options: 32 -- 4096 (in powers of 2)

  - Constraints: None

- Clock Domain (`clk_domain`)

  - Definition: Clock domain to export the interface in

  - Options: All available RFNoC and User clocks

  - Constraints: None

- Control Port Settings

  - Byte Mode (`byte_mode`)

    - Definition: Expose the "req_byte_en" field on the interface.

    - Options: "On" (`True`) or "Off" (`False`) (Off implies 32-bit mode)

    - Constraints: None

  - Timed Commands (`timed`)

    - Definition: Expose the "req_has_time" and "req_time" fields on the interface.

    - Options: "On" (`True`) or "Off" (`False`) (Off implies immediate or non-timed commands)

    - Constraints: None

  - Transaction Status (`has_status`)

    - Definition: Expose the "resp_status" field on the interface.

    - Options: "On" (`True`) or "Off" (`False`) (Off implies transactions that are always successful)

    - Constraints: None

\anchor data_plane_anchor
## Data Plane

The data plane in the FPGA can be exposed using a low-level AXI4-Stream interface called *AXI-Stream CHDR* (AXIS-CHDR) or using the simpler abstracted interfaces *AXI-Stream Payload Context and AXI-Stream Data*. This plane of communication is intended for high-throughput data transfer between blocks. The CHDR header information is retained in the data plane so blocks can attach additional information like metadata and timestamps to packets. The CHDR header information (\ref chdr_field_descriptions "CHDR field descriptions") must be accurate for all packets entering and leaving a block except for the destination endpoint ID (`DstEPID`). The destination endpoint is used for routing between stream endpoints and is not relevant between adjacent blocks; the value of this field is reserved and will be overwritten by the framework.

### Data Item and Component Ordering

The width of the bus presented to the user is configurable. It is therefore possible to receive multiple data items per clock cycle on the data bus. The first item shall be placed in the least-significant position of the data bus. For complex data types, the real or in-phase (I) component shall be placed in the most-significant position within the data item and the imaginary or quadrature (Q) component shall be placed in the least significant position within the data item. The figures below illustrate the item and complex component ordering within the bus.

*Note: Third-party IP may use a different IQ order and/or format than that used by RFNoC. For example, many Xilinx IP blocks put the real (I) component in the least-significant position and imaginary (Q) component in the most-significant position, and the format may not be SC16 by default.*

<div align="center">
<table>
  <caption>32-bit bus with SC16 data items.
  </caption>
    <tr>
      <td align="center">31 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;16</td>
      <td align="center">15 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0</td>
    </tr>
    <tr>
      <td colspan="2" align="center">  Item 0</td>
    </tr>
    <tr>
      <td align="center">  Real (I)</td>
      <td align="center">  Imaginary (Q) </td>
    </tr>
</table>
</div>

<br>

<div align="center">
<table>
  <caption>64-bit bus with SC16 data items.
  </caption>
    <tr>
      <td align="center"> 63 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;      48</td>
      <td align="center"> 47 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;      32</td>
      <td align="center"> 31 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;      16</td>
      <td align="center"> 16 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;      0</td>
    </tr>
    <tr>
      <td colspan="2" align="center">  Item 1</td>
      <td colspan="2" align="center">  Item 2</td>
    </tr>
    <tr>
      <td align="center">  Real (I)</td>
      <td align="center">  Imaginary (Q) </td>
      <td align="center">  Real (I)</td>
      <td align="center">  Imaginary (Q) </td>
    </tr>
</table>
</div>



### AXI-Stream CHDR (Low-level Interface)

The AXI-Stream CHDR (AXIS-CHDR) interface provides direct access to the data ports. The client can request this interface for maximum control over the stream, but the client is responsible for implementing the framer/de-framer for CHDR packets.

A block may have between 0 and 64 input/output data ports. For a block with P input ports, the NoC Shell will contain P separate slave CHDR streams. For a block with Q output ports, the NoC Shell will contain Q separate master CHDR streams. All the streams share the same clock and reset.

When the AXI-Stream CHDR interface is used, the NoC Shell will expose the ports listed below for the user-logic to connect to. In this list, \<name\> refers to the name provided by the user for this port and \<signal\> refers to one of the standard AXI4-Stream signals: tdata (CHDR width), tvalid, tready and tlast. Additionally, these signals may be a concatenation of multiple data streams if a parameter is used to define the number of ports. For example, the signal s_myports_chdr_tvalid\[1\] would refer to tvalid of the slave stream for port 1 of "myports".

- `axis_chdr_clk` <br>
  This is the clock that all the axis_chdr signals are synchronous to. The user may choose which clock source drives this clock. This is an output of the NoC Shell.

- `axis_chdr_rst`<br>
  This is the synchronous reset for the data-path logic. This is an output of the NoC Shell. This reset will be asserted for at least one clock cycle after which the client logic will have 1 ms to complete the following tasks:

  - Reset the data-path state to the initial powerup/startup values

  - Stop generating data on the master interface

  - Drop all data on the slave interface (Note that the slave interface may have partial CHDR packets that need to be dropped)

- `s_<name>_chdr_<signal>`<br>
  This is the slave interface to which the user logic will send all outgoing items/samples. Each AXI-Stream packet must be of the format described in the \ref memory_layout_of_a_chdr_packet_anchor "Memory layout of a CHDR packet" and must be a CHDR Data Packet (PktType = 6 or 7).

- `m_<name>_chdr_<signal>`<br>
  This is the master interface from which the user logic will receive incoming items/samples. Each AXI-Stream packet will be in the format described in the \ref memory_layout_of_a_chdr_packet_anchor "Memory layout of a CHDR packet" and will be a CHDR Data Packet (PktType = 6 or 7). The user logic will be required to parse this packet format.

### AXI-Stream Payload Context (Simple Interface)

The payload context interface provides a simpler interface to connect processing IP. The payload context interface abstracts away the CHDR stream into two separate AXI-Stream interfaces: Payload and context. The payload stream contains the payload data of a CHDR packet and can often be directly connected to processing blocks that support AXI-Stream. The payload stream is comprised of *items* (the smallest processing unit; e.g., a data sample) and can deliver one or more items per cycle. The context stream contains additional information about the payload stream such as the header, timestamp and metadata. Splitting the payload and context streams allows separate (but coupled) state machines for data and header processing. The following abbreviations are used below:

- CHDR_W: The bit-width of the CHDR bus that the block can support.

- ITEM_W: The bit-width of a raw data item. ITEM_W must be a multiple of 8 (AXI-Stream requires transfers to be in bytes).

- NIPC: The number of items delivered per cycle between the interface and the processing IP.

A block may have 0 to 64 input/output data ports. For a block with P input ports, the NoC shell will contain P separate master CHDR streams. For a block with Q output ports, the NoC shell will contain Q separate slave CHDR streams. All the streams share the same clock and reset.

When the AXI-Stream Payload Context interface is used, the NoC Shell will expose the ports listed below for the user-logic to connect to. In this list, \<name\> refers to the name provided by the user for this port and \<signal\> refers to one of the standard AXI4-Stream signals: tdata (CHDR width), tvalid, tready and tlast. Additionally, these signals may be a concatenation of multiple data streams if a parameter is used to define the number of ports. For example, the signal s_myports_payload_tvalid\[1\] would refer to tvalid of the slave payload stream for port 1 of "myports".

- `axis_data_clk`<br>
  This is the clock that all the AXI-Stream signals are synchronous to. The user may choose which clock source drives this clock. This is an output of the NoC Shell.

- `axis_data_rst`<br>
  This is the synchronous reset for the data-path logic. This is an output of the NoC Shell. This reset will be asserted for at least one clock cycle after which the client logic will have 1 ms to complete the following tasks:

  - Reset the data-path state to the initial powerup/startup values

  - Stop generating data on the master interface

  - Drop all data on the slave interface (Note that the slave interface may have partial CHDR packets that need to be dropped)

- `s_<name>_payload_<signal>`, `s_<name>_context_<signal>`<br>
  These are the slave interfaces to which the user logic will send outgoing items. The table below shows the various signals represented by \<signal\>.

- `m_<name>_payload_<signal>`, `m_<name>_context_<signal>`<br>
  These are the master interfaces from which the user logic will receive incoming items. The table below shows the various signals represented by `<signal>`.

<div align="center">
<table>
  <caption>AXI-Stream Payload Context port signal definitions.
  </caption>
    <tr>
      <th>Signal</th>
      <th>Direction <br> (Master)</th>
      <th>Width</th>
      <th>Purpose</th>
      <th>Usage</th>
    </tr>
    <tr>
      <td> payload_tdata </td>
      <td>  out </td>
      <td> NIPC * <br> ITEM_W</td>
      <td> 
         The primary data payload word for <br>
         this transfer.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> payload_tkeep </td>
      <td>  out </td>
      <td> NIPC</td>
      <td> 
         An item qualifier that indicates <br>
         whether the content of the associated <br>
         item in tdata is processed in the <br>
         stream. <br>
         *NOTE: The granularity of this field is <br>
         item and not byte. This behavior is <br> 
         different from the standard AXI4-<br>
         Stream tkeep. <br>
         NOTE: This may only used to indicate <br>
         trailing items at the end of a packet.*
      </td>
      <td> Required <br> for <br> NIPC&nbsp;\>&nbsp;1 </td>
    </tr>
    <tr>
      <td> payload_tlast </td>
      <td> out </td>
      <td> 1</td>
      <td> 
         Indicates the last word (transfer) in the <br>
         current payload packet.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> payload_tvalid </td>
      <td>  out </td>
      <td> 1</td>
      <td> 
        Indicates that the master is driving a <br>
        valid packet payload word (transfer).
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> payload_tready </td>
      <td>  in </td>
      <td> 1 </td>
      <td>
         Indicates that the slave can accept a <br>
         payload word (transfer) in the current <br>
         cycle. 
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> context_tdata </td>
      <td> out </td>
      <td> CHDR_W</td>
      <td>
         The primary context word for this <br>
         transfer. 
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td valign="top"> context_tuser </td>
      <td valign="top"> out </td>
      <td valign="top"> 4</td>
      <td>
         Indicates the type of context word. <br>
         <table>
              <tr>
                <th> Value</th>
                <th> Type</th>
              </tr>
              <tr>
                <td> 0x0</td>
                <td> CHDR Header (HDR)</td>
              </tr>
              <tr>
                <td> 0x1</td>
                <td> CHDR Header + <br> Timestamp (HDR_TS)</td>
              </tr>
              <tr>
                <td> 0x2</td>
                <td> Timestamp Only (TS)</td>
              </tr>
              <tr>
                <td> 0x3</td>
                <td> 	Metadata (MDATA)</td>
              </tr>
              <tr>
                <td> Rest</td>
                <td> Reserved</td>
              </tr>
           </table>
      </td>
      <td valign="top"> Required </td>
    </tr>
    <tr>
      <td> context_tlast </td>
      <td>  out </td>
      <td> 1</td>
      <td>
          Indicates the last word (transfer) in the <br>
          current context packet.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> context_tvalid </td>
      <td>  out </td>
      <td> 1</td>
      <td>
         Indicates that the master is driving a <br>
         valid context word (transfer). 
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> context_tready </td>
      <td>  in </td>
      <td> 1</td>
      <td>
          Indicates that the slave can accept a <br>
          context word (transfer) in the current <br>
          cycle.
      </td>
      <td> Required </td>
    </tr>
</table>
</div>


**NOTE:** The data in a context packet represents CHDR header information and thus must be in the same order as the CHDR field. The following sequences on context_tuser are valid, and all others will be regarded as a protocol violation for the context port.


<ul>
  <li>
     Packet with no timestamp and no metadata (all CHDR Widths) <br>
     <table>
         <tr>
           <td>HDR &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
         </tr>
       </table>
  </li>
  <li>
     Packet with timestamp and no metadata (CHDR Width = 64)
     <table>
         <tr>
           <td>HDR &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
           <td>TS &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
         </tr>
       </table>
  </li>
  <li>
     Packet with timestamp and no metadata (CHDR Width \> 64)
     <table>
         <tr>
           <td>HDR_TS</td>
         </tr>
       </table>
  </li>
  <li>
     Packet with timestamp and metadata (CHDR Width = 64)
     <table>
         <tr>
           <td>HDR &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
           <td>TS &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
           <td>MDATA &nbsp;</td>
           <td>... &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
           <td>MDATA &nbsp;</td>
         </tr>
       </table>
  </li>
  <li>
     Packet with timestamp and metadata (CHDR Width \> 64)
     <table>
         <tr>
           <td>HDR_TS</td>
           <td>MDATA &nbsp;</td>
           <td>... &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
           <td>MDATA &nbsp;</td>
         </tr>
       </table>
  </li>
  <li>
    Packet with no timestamp and metadata (all CHDR Widths)
     <table>
         <tr>
           <td>HDR &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
           <td>MDATA &nbsp;</td>
           <td>... &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
           <td>MDATA &nbsp;</td>
         </tr>
       </table>
  </li>
  </ul>

 

\image html rfnoc_td_head.png "A 4-word packet with only the header on AXIS Payload Context port." width=600px

<br>

\image html rfnoc_td_head_ts.png "A 4-word packet with a header and timestamp on the AXIS Payload Context port (CHDR_W = 64)." width=700px

<br>

\image html rfnoc_td_head_ts_meta.png "A 4-word packet with a header, timestamp and 2 metadata words on the AXIS Payload Context port (CHDR_W = 64)." width=800px

<br>

\image html rfnoc_td_head_ts_meta_gap.png "A 4-word packet on the AXIS Payload Context port with a gap between the context and payload (CHDR_W = 64)." width=900px

<br>

\image html rfnoc_td_two_packets.png "Two back-to-back packets on the AXIS Payload Context port (with header prefetching; CHDR_W = 64)." width=1000px



### AXI-Stream Data (Simple Interface)

The AXI-Stream Data interface provides another simple user interface. It uses an AXI-Stream data interface but does not require the user to packetize header information. It also supports timestamps, EOB, and EOV. The following abbreviations are used below:

- CHDR_W: The bit-width of the CHDR bus that the block can support.

- ITEM_W: The bit-width of a raw data item. ITEM_W must be a multiple of 8 (AXI-Stream requires transfers to be in bytes).

- NIPC: The number of items delivered per cycle between the interface and the processing IP.

A block may have 0 to 64 input/output data ports. For a block with P input ports, the NoC shell will contain P separate master CHDR streams. For a block with Q output ports, the NoC shell will contain Q separate slave CHDR streams. All the streams share the same clock and reset.

When the AXI-Stream Data interface is used, the NoC Shell will expose the ports listed below for the user-logic to connect to. In this list, \<name\> refers to the name provided by the user for this port and `signal` refers to one of the standard AXI4-Stream signals: tdata (CHDR width), tvalid, tready and tlast. Additionally, these signals may be a concatenation of multiple data streams if a parameter is used to define the number of ports. For example, the signal `s_myports_axis_tvalid[1]` would refer to tvalid of the slave stream for port 1 of "myports".

- `axis_data_clk` <br>
  This is the clock that all the AXI-Stream signals are synchronous to. The user may choose which clock source drives this clock. This is an output of the NoC Shell.

- `axis_data_rst`<br>
  This is the synchronous reset for the data-path logic. This reset will be asserted for at least one clock cycle and the client logic will have 1 ms to complete the following tasks:

  - Reset the data-path state to the initial powerup/startup values

  - Stop generating data on the master interface

  - Drop all data on the slave interface (Note that the slave interface may have partial CHDR packets that need to be dropped)

- `s_<name>_axis_<signal>`<br>
  This is the slave interface to which the user logic will send outgoing items. The table below shows the various signals represented by `<signal>`.

- `m_<name>_axis_<signal>`<br>
  This is the master interface from which the user logic will receive incoming items. The table below shows the various signals represented by `<signal>`.

The signals `tlength`, `ttimestamp`, `thas_time`, `teov`, and `teob` are sideband signals and behave like tuser in traditional AXI4-Stream. Rather than having a single `tuser` signal, these signals have been separated into individual signals for ease of use. On the NoC Shell's master data interface, these signals are valid for the duration of the packet (i.e., whenever `tvalid` is true).

When the sideband signals are read by the NoC Shell's slave data interface depends on the `SIDEBAND_AT_END` parameter. If `SIDEBAND_AT_END` is True then these signals must be valid on the last transfer of each packet (i.e., when `tlast` is asserted) and tlength is calculated automatically by the NoC Shell. If SIDEBAND_AT_END is False, then these signals must be valid on the first transfer of each packet and tlength must be provided as an input to indicate the length of the data packet being input to the NoC Shell.

The `SIDEBAND_AT_END = True` setting is required in situations where one or more items associated with the CHDR header (e.g., length, timestamp, EOB, EOV) are not known until the end of the packet is ready to be output. An important side-effect of this setting is that all output packets sent to the NoC Shell's slave interface will be completely buffered before they are sent out. This adds latency to the packets and requires that the NoC Shell implement an MTU-sized buffer to store outgoing packets.

<div align="center">
<table>
  <caption>AXI-Stream Data port signal definitions.
  </caption>
    <tr>
      <th>Signal</th>
      <th>Direction <br> (Master)</th>
      <th>Width</th>
      <th>Purpose</th>
      <th>Usage</th>
    </tr>
    <tr>
      <td> t_data</td>
      <td>  out </td>
      <td> NIPC * <br> ITEM_W</td>
      <td> 
         The data payload word for  this <br>
         transfer.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> tkeep </td>
      <td>  out </td>
      <td> NIPC</td>
      <td> 
         An item qualifier that indicates <br>
         whether the content of the associated <br>
         item in tdata is processed in the <br>
         stream. <br>
         *NOTE: The granularity of this field is <br>
         item and not byte. This behavior is <br> 
         different from the standard AXI4-<br>
         Stream tkeep. <br>
         NOTE: This may only used to indicate <br>
         trailing items at the end of a packet.*
      </td>
      <td> Required <br> for <br> NIPC&nbsp;\>&nbsp;1 </td>
    </tr>
    <tr>
      <td> tlast </td>
      <td> out </td>
      <td> 1</td>
      <td> 
         Indicates the last word (transfer) in the <br>
         current payload packet.
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> tvalid </td>
      <td>  out </td>
      <td> 1</td>
      <td> 
        Indicates that the master is driving a <br>
        valid packet payload word (transfer).
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> tready </td>
      <td>  in </td>
      <td> 1 </td>
      <td>
         Indicates that the slave can accept a <br>
         payload word (transfer) in the current <br>
         cycle. 
      </td>
      <td> Required </td>
    </tr>
    <tr>
      <td> ttimestamp </td>
      <td> out </td>
      <td> 64</td>
      <td>
         The timestamp for the first item in the <br>
         packet. 
      </td>
      <td> Optional </td>
    </tr>
    <tr>
      <td> thas_time </td>
      <td> out </td>
      <td> 1</td>
      <td>
         Indicates if the `ttimstamp` field is <br>
         being used. This will be 0 if there is <br>
         not timestamp for the current packet. 
      </td>
      <td> Optional </td>
    </tr>
    <tr>
      <td>tlength </td>
      <td> out </td>
      <td> 16</td>
      <td>
          The byte length of the data packet <br>
          (i.e., the byte length of the CHDR <br>
          payload, which excludes the header <br>
          and metadata).<br>
          *NOTE: This port is read by the slave <br>
          interface when `SIDEBAND_AT_END` is <br>
          True and is ignored by the slave <br>
          interface when `SIDEBAND_AT_END` is <br>
          False.*
      </td>
      <td>Optional</td>
    </tr>
    <tr>
      <td> teov</td>
      <td> out</td>
      <td> 1</td>
      <td> 
         Indicates if the EOV bit was set in the <br>
         packet.
      </td>
      <td> Optional</td>
    </tr>
    <tr>
      <td>teob </td>
      <td> out</td>
      <td> 1</td>
      <td>
          Indicates if the EOB bit was set in the <br>
          packet.
      </td>
      <td> Optional</td>
    </tr>
</table>
</div>

\anchor noc_shell_generation_options_data_anchor
### NoC Shell Generation Options

RFNoC Modtool has the following options to generate the data interface for the NoC Shell of a NoC block.

- Data Interface (`fpga_iface`)

  - Definition: Which HDL interface to expose

  - Options: "AXI-Stream CHDR" (`axis_chdr`), "AXI-Stream Payload Context" (`axis_pyld_ctxt`), or "AXI-Stream Data" (`axis_data`)

  - Constraints: None

- Number of Input Ports

  - Definition: The number of input ports

  - Options: 0 - 64

  - Constraints: None

- Number of Output Ports

  - Definition: The number of output ports

  - Options: 0 - 64

  - Constraints: None

- Port Specific Settings (for each input and output port)

  - Clock Domain (`clk_domain`)

    - Definition: The clock domain for the payload or data interface

    - Options: All available RFNoC and User clocks

    - Constraints: None

  - Item Width (`item_width`)

    - Definition: Bit width of each data item

    - Options: 32, 64, 128, etc.

    - Constraints: Only valid when using the AXI-Stream Payload Context or AXI-Stream Data interfaces

  - Number of Items per Cycle (`nipc`)

    - Definition: Number of data items to deliver per clock cycle

    - Options: 1-256 (in powers of 2)

    - Constraints: Only valid when using the AXI-Stream Payload Context or AXI-Stream Data interfaces

  - Payload FIFO Depth (`payload_fifo_depth`)

    - Definition: Depth of the AXI-Stream buffer for the payload data path

    - Options: 1 or larger (in powers of 2)

    - Constraints: Only valid when using the AXI-Stream Payload Context or AXI-Stream Data interfaces

  - Context FIFO Depth (`context_fifo_depth`)

    - Definition: Depth of the AXI-Stream buffer for the context data path

    - Options: 1 or larger (in powers of 2)

    - Constraints: Only valid when using the AXI-Stream Payload Context interface

  - Info FIFO Depth (`info_fifo_depth`)

    - Definition: Depth of the AXI-Stream buffer for queued packet information

    - Options: 1 or larger (in powers of 2)

    - Constraints: Only valid when using the AXI-Stream Data interface

  - Context Prefetching

    - Definition: Allow prefetching context data for the next packet when the current packet is in flight.

    - Options: "On" or "Off"

    - Constraints: Only valid when using the AXI-Stream Payload Context interface

## IO Ports (Advanced)

IO Ports are interfaces to the user-logic that don't interact with the RFNoC framework. IO Ports may interact with other blocks in an assembled design (for backdoor inter-block communication) or with IO on the USRP device.

### Hardware Timestamp Interface

The user logic can get access to a hardware time-base and timestamp. The capabilities of a hardware time-base are device specific. The timestamp can be used with real-time blocks like the radio which interfaces with ADCs/DACs.

- `tb_clk`: The time-base clock.

- `tb_rst`: A synchronous reset in tb_clk domain. tb_rst = 1 indicates that the time-base is disabled.

- `tb_timestamp`: A 64-bit global timestamp that is synchronous to *tb_clk*. The timestamp is a counter that may start at an arbitrary value and count up by one every clock cycle of tb_clk after tb_rst is released.

- `tb_period_ns_q32`: A 64-bit fixed point number in the Q32 format that represents the period of the time-base in nanoseconds.

\image html rfnoc_td_time_base_reconf.png "An example of a time-base reconfiguration from 200 MHz to 160 MHz" width=800px

\anchor generic_io_ports_anchor
### Generic IO Ports

It is possible to add more generic IO to a NoC block. A generic IO port is a collection of signals, their types, widths and directions. This collection is called an IO Signature. An IO Signature can be inherited from a specific USRP device or be user-defined. Each IO Signature contains the following information:

- *Name*: A unique name for this IO Signature

- *Drive*: The drive direction of this IO Port. The driver direction can be "Slave" (driven by a master), "Master" (driven a single slave), "Listener" (a special slave with only inputs) or "Broadcaster" (a special master with only outputs).

- *Port List*: A list of signals each with the following properties:

  - *Name*: Name of the signal

  - *Type*: Is this a "Clock", "Reset" or "Generic" signal?

  - *Direction*: Is this an input or an output on the master?

  - *Width*: The bit-width of the signal

If a block defines a generic IO Port, then the IO port must be assigned to another IO Port with the same signature during design image assembly. The other IO Port may be a part of the USRP device or an IO Port on another block. The following connection rules apply:

- A Master can drive exactly one Slave

- A Slave can be driven by exactly one Master

- A Broadcaster can drive zero or more listeners

- A Listener must be driven by at least one Broadcaster

- A Master cannot drive a Listener

- A Broadcaster cannot drive a Slave

\anchor backend_rfnoc_interface_anchor
## Backend RFNoC Interface

Because NoC Shell is a part of the user NoC block, there will be certain interfaces exposed as inputs/outputs from the block that the user logic can ignore. These interfaces are termed as "backend" and are used by NoC Shell to communicate with the rest of the framework.

### CHDR

- `rfnoc_chdr_clk` <br>
  This is the clock for the rfnoc_chdr port (described below).

- `rfnoc_chdr_rst`<br>
  This is the synchronous reset for the rfnoc_chdr port. This reset is driven by the backend interface.

- `s_rfnoc_chdr_<signal>`<br>
  The slave rfnoc_chdr port. This interface accepts CHDR packets from the framework. \<signal\> refers to the following standard AXI4-Stream signals: tdata (CHDR_W bits), tvalid, tready and tlast. The widths of these signals depend on the number of input ports and the CHDR_W setting.

- `m_rfnoc_chdr_<signal>`<br>
  The master rfnoc_chdr port. This interface outputs CHDR packets to the framework. \<signal\> refers to the following standard AXI4-Stream signals: tdata (CHDR_W bits), tvalid, tready and tlast. The widths of these signals depend on the number of output ports and the CHDR_W setting.

### Control

- `rfnoc_ctrl_clk` <br>
  This is the clock for the rfnoc_ctrl port (described below).

- `rfnoc_ctrl_rst` <br>
  This is the synchronous reset for the rfnoc_ctrl port. This reset is driven by the backend interface.

- `s_rfnoc_ctrl_<signal>` <br>
  The slave rfnoc_ctrl port. This interface accepts AXIS-Ctrl packets from the framework. \<signal\> refers to the following standard AXI4-Stream signals: tdata (32 bits), tvalid, tready and tlast.

- `m_rfnoc_ctrl_<signal>` <br>
  The master rfnoc_ctrl port. This interface outputs AXIS-Ctrl packets to the framework. \<signal\> refers to the following standard AXI4-Stream signals: tdata (32 bits), tvalid, tready and tlast.

### Configuration and Status

- `rfnoc_core_config`<br>
  A 512-bit interface for the framework to configure the state of the NoC shell logic. The interpretation of the bits in this bus is determined by the framework. Client logic is not expected to use this signal.

- `rfnoc_core_status` <br>
  A 512-bit interface for the framework to read the state of the NoC shell logic. The interpretation of the bits in this bus is determined by the framework. Client logic is not expected to use this signal.

# RFNoC FPGA Image

The RFNoC FPGA image is a standalone design for a USRP that has a collection of block instantiations and a partial topology preconfigured in the FPGA (static connections). This design can be configured using software to create a full flow-graph or be a part of a multi-USRP flow-graph.

## Workflow

After all blocks are developed, the RFNoC framework has built-in tools to generate an FPGA image with user-specified block instantiations and a central router core with user-specified block connections. The following user information will be used to define a topology of the blocks and build an FPGA bitfile.

- USRP Device Info

- CHDR Width

- Number of Stream endpoints

  - Number of Input and Output ports for each stream endpoint

  - Optional: Buffer size for each endpoint

- NoC blocks to Build in the FPGA Image

  - Optional: Clock choices for each block

  - Optional: IO Port connections for each block

- Static connections between blocks, stream endpoints and transport adapters

\anchor design_assembly_toolflow_anchor
## Design Assembly Toolflow

The following information is inferred based on the user-specified preferences, device info (part of the board support package) and meta-data from each block:

- CHDR Width (W<sub>chdr</sub>)

- Stream endpoints (M)

  - Buffer size for each endpoint (B<sub>ep</sub>)

- NoC blocks (N<sub>user</sub>)

  - Number of input and output ports

  - Datapath connection topology

  - Clock choices for the data and control clock

- Number of transport adapters (P)

- Number of IO-based NoC blocks (Radio, DDR-based blocks, etc.) (N<sub>fixed</sub>)

The code generator will determine the following parameters using that info:

- CHDR Crossbar

  - Number of ports = P + M

  - Data Width = W<sub>chdr</sub>

- Stream Endpoints

  - Number of endpoints = M

  - Data Width = W<sub>chdr</sub>

  - Buffer Size = B<sub>ep</sub>

- Control Crossbar

  - Number of Ports = (M + N<sub>fixed</sub> + N<sub>user</sub> + 1)

- Static Router

  - Number of Ports = (M + Ports(N<sub>fixed</sub>) + Ports(N<sub>user</sub>))

  - Inter-port connections

  - Adjacency list

Using this info, the code generator will do the following

1.  For a given device, look the number of transport adapters (P) and read the number of requested stream endpoints (M), then instantiate a CHDR Crossbar with P+M ports

2.  Instantiate M stream endpoints

3.  Connect the P transport adapters to the first P ports of the chdr_crossbar and the next M ports to the stream endpoints

4.  Read the number of NoC blocks (N) and instantiate a Control Crossbar with N<sub>fixed</sub> + N<sub>user</sub> + M + 1 ports

5.  Connect the first port of the control crossbar to a core config endpoint, then connect the next M ports to the M control endpoints and then connect the remaining N ports to the NoC blocks' control interfaces.

6.  Generate a static router with M + Ports(N<sub>fixed</sub>) + Ports(N<sub>user</sub>) ports and hook it up to the M stream endpoints and X NoC block ports (each block may have an arbitrary number of ports)

7.  Read in the inter-block connections and build a table of the connections as an adjacency list that is readable by UHD.

## Initialization and Usage

The RFNoC software will ensure that all the blocks in the FPGA image are initialized before use. RFNoC defines the following as three phases of initialization:

1.  *All Blocks Idle*: Each block in the design (regardless of its previous state) must first be put in an idle streaming state, i.e. no data is streaming though the data-path.

2.  *All Blocks Reset*: Each block in the design (regardless of its previous state) must then be put into a known state for settings and software configurable registers.

3.  *Network Ready*: All blocks are initialized, and the core framework is ready to begin executing an application.

RFNoC is a network that may consist of multiple FPGA designs so "All Blocks" above refers to all blocks in the collection of USRPs controlled by the RFNoC software. Both, the RFNoC framework and the individual blocks share the responsibility for initialization. The control and data path resets (Sections [Control Plane](#control_plane_anchor) and [Data Plane](#data_plane_anchor)) will be asserted for each block to begin the reset operations and the framework imposes a time limit to allow the block to finish its reset procedure. Before the resets are asserted, the framework will also *flush* data at the input and output of each block. Flushing is an internal framework operation (not visible to the NoC blocks or the user) that ensures that no data is generated downstream of the flush point and all data is consumed at the flush point. The figure below shows the full initialization sequence for an image with multiple blocks (Block 0 ... Block N) and multiple stream endpoints (SEP 0 ... SEP N).

\anchor initialization_sequence_for_an_rfnoc_flow_graph_anchor
\image html rfnoc_fg_init.png "Initialization sequence for an RFNoC flow-graph" width=900px
