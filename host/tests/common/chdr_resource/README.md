## RFNoC Traces
This file contains various traces or chdr packet conversations obtained using wireshark.

#### hardcoded_packets
This file contains handwritten C++ objects which represent the expected deserialization result of packets contained in the traces.
They have been specifically chosen to provide good test coverage over the packet types and the different values that each packet type may contain.

#### rfnoc_packets_ctrl_mgmt
This file contains a trace that was created by connecting to a usrp over ethernet and running `uhd_usrp_probe`.
It contains a mix of Management and Control packets.

#### rfnoc_packets_data
This file contains a trace that was created by connecting to a usrp over ethernet and running `rx_samples_to_file`.
It contains many Data packets, as well as some Stream Status and Stream Command packets.

### Steps to Reproduce
1. Install Wireshark. Installing the RFNoC Wireshark can be helpful for identifying which packets are CHDR packets, but it isn't required.
2. Conduct a wireshark capture on your network interface and run an operation with the radio (`uhd_usrp_probe`, `benchmark_rate`, etc.)
3. Find an RFNoC Packet in the capture windows, right click on it, and select Follow -> UDP Stream. This will gather all packets in that "conversation". With a large trace, this may take a few moments.
4. In the bottom of the window that opens, select Show and save data as -> C Arrays.
5. Save the trace with a descriptive name in the `host/tests/common/chdr_resource` directory as a `.c` file
6. In the `host/tests/common/chdr_resource` directory, run `./format_trace.py {filename}`, where `{filename}` is the name of the `.c` file you save the trace as.

This will create a `.cpp` file and a `.py` file of the same name, which contains the trace data inside a namespace matching the filename. The trace can now be accessed in unit tests with `#include <chdr_resource/{filename}.cpp>` in C++ and `from chdr_resource import {filename}` in Python.
