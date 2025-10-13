\page page_rfnoc_sw RFNoC Software Specification

\tableofcontents

# Basics

On the software side (UHD) RFNoC has the following interfaces:

- *Block Controller*: This is a control interface to each block in the design. Block controllers are discovered automatically by UHD based on the blocks in an FPGA, and they can be retrieved by the user to send and receive commands from blocks.

- *Graph*: The graph object manages a topology of blocks in an application. The static connections in the FPGA and the dynamic connections made by the user will be reflected in the graph. The graph will ensure state integrity between blocks if there are inherent dependencies.

- *Streamers*: Streamers are used to send/receive data to/from blocks in the FPGA.

\image html rfnoc_fg_FPGA_sw.png "Example FPGA and SW objects in an RFNoC graph" width=900px


\anchor block_controller_anchor
# Block Controller

The *block controller* is the UHD (C++) counterpart of the NoC Shell. It is a block-level API to interface with the clients of NoC shell. UHD has a default block controller object but it is possible to override it (via inheritance) with a custom controller C++ class. UHD maintains a list of block controllers for each block type and dynamically creates instances of it in software when the block is present in the FPGA image for the target USRP.

## Block IDs

An identifier is needed to retrieve a block controller from UHD. Each block has two kinds of identification tags:

1.  Each block in the FPGA must have a *NoC ID* which serves as a unique identifier of the function of the block. *NoC IDs* are 32 bits wide and can take on any value as long as it is unique among blocks. For example, the FIR filter block will have a unique NoC ID that is different from the FFT block NoC ID. Multiple instances of the FIR block have the same NoC ID.

2.  Each instance of a block in an FPGA has a *Block ID*. It is used to locate a specific block in an RFNoC network. For instance, if the same NoC block is instantiated twice, then the two instances will have the same *NoC ID* but different *Block IDs*. The syntax of a *Block ID* is as follows: `<Device>/<BlockName>:<BlockInstance>` 

### C++ API
\li \ref uhd::rfnoc::block_id_t "block_id_t class"

## Registers

Each block has a unique register space for low-level configuration. In the FPGA, this space is accessible using the AXIS-Ctrl port or the CtrlPort interface. Registers have a fixed bit width of 32 bits. Each block in software will provide an implementation for the rfnoc::register_iface interface that can be used to access registers in the FPGA. The rfnoc::register_iface interface supports the following:

- Peek/poke functionality for 32-bit registers

- Sleep functionality to time-sequence operations

- Timed commands

- Callbacks for asynchronous messages from the block

- Optional resilience parameters to trade throughput for robustness

### C++ API
\li \ref uhd::rfnoc::register_iface "register_iface class"

## Block Properties

\anchor props_intro
### Introduction
One of the mechanisms that RFNoC blocks can use to control their configuration
are *properties*. There are two types of properties within a block: User
properties, and edge properties.

To illustrate these types of properties, we start with an example: Take DDC
block (uhd::rfnoc::ddc_block_control), which can take in a signal of a certain
sampling rate, shift it in frequency, and resample to a lower sampling rate.

First, we take a look at the *user properties*. User properties are properties
which are attributes of a block, they simply define certain characteristics of
the block. Here, we focus on two user properties: The frequency and the decimation.
The former is a floating point value which stores, in Hz, the amount by which
the incoming signal is shifted in frequency. The decimation is the integer
value by which the incoming sampling rate is divided. Put differently, the user
properties control the behaviour of the DDC block.

On top of that, the DDC block also has an *edge property* for the sampling rate
on both the outgoing edge and the incoming edge. This property describes
something about the data on this graph edge.

```
           ┌──────────────┐
           │  DDC Block   │
 samp_rate │              │ samp_rate
───────────> - freq       ├───────────>
           │ - decim      │
           │              │
           └──────────────┘
```

The key thing, which separates properties from other attributes associated with
the block, is that we can describe relationships between the properties. In this
example, the following relationships exist:

- The `samp_rate` property on the outgoing edge equals the `samp_rate` property
  on the incoming edge divided by the `decim` value.
- The normalized frequency (required to program the digital hardware) is
  calculated by dividing the `freq` user property by the `samp_rate` edge
  property on the incoming edge.

User properties also act as an API for RFNoC blocks. It is possible to access
user properties using the uhd::rfnoc::node_t::get_property() and
uhd::rfnoc::node_t::set_property() and uhd::rfnoc::node_t::set_properties() APIs.
A list of user properties can be read by calling uhd::rfnoc::node_t::get_property_ids().

Often, block controllers will provide explicit C++ APIs on top of the user
properties. For example, the DDC block control has an API call
uhd::rfnoc::ddc_block_control::set_freq(), which provides additional features on top of the
user property (e.g., it can also set a command time). Direct access of user
properties is thus often not necessary, and can be considered a lower-level API
than directly calling block APIs.


\anchor props_propprop
### Property Propagation
By itself, these relationships between properties are already useful to describe
something about the block, but the real power comes when connecting blocks. This
allows blocks to communicate their settings, even without knowing which block
they're connected to. Consider the following example, using a radio block
(uhd::rfnoc::radio_control), the DDC block, and a hypothetical Modem Block, which
expects incoming samples at a fixed data rate of 20 Msps. Assume we are running
this flow graph in a USRP X310, with a master clock rate of 200 Msps:

```
  ┌─────────────┐          ┌──────────────┐          ┌──────────────┐
  │ Radio Block │          │  DDC Block   │          │ Modem Block  │
  │             │samp_rate │              │ samp_rate│              │
  │             ├──────────> - freq       ├──────────>              │
  │             │          │ - decim      │          │              │
  │             │          │              │          │              │
  └─────────────┘          └──────────────┘          └──────────────┘
```

Upon initialization, the radio block will configure its hardware clocks, and
set a sampling rate of 200 Msps on its outgoing edge property (`samp_rate`).
This property is also the incoming edge property of the DDC block, which means
it is now initialized its sampling rate to 200 Msps. Setting the `freq` property
will now work accurately, because it can calculate normalized frequencies.

The Modem Block will set its incoming edge property `samp_rate` to 20 Msps, which
is also the outgoing edge property of the DDC block. The DDC block must now find
a way to reconcile these edge properties, which it can achieve by setting the
`decim` user property to 20. Now, the DDC has automatically set a decimation
without anyone calling into its rate-changing APIs, and without the Modem Block
or the Radio Block knowing anything about how the DDC block works.

Similarly, if the Modem Block would have requested a sampling rate of 30.72 Msps,
the DDC would not have been able to resolve the incoming and outgoing sampling
rates with an integer decimation factor. It would have requested the radio to
produce an integer multiple of 30.72 Msps, which the radio can't provide. Using
properties, UHD would be able to throw an exception, telling the user that the
requested combination is not achievable.

\anchor  props_multichan
### Properties on multi-channel blocks and resource types

The DDC block is configurable to have a variable number of channels, and the
same is true for the properties. To accommodate for this, both edge and user
properties have an index value. A multi-channel DDC block is more accurately
depicted as such with regard to its properties:

```
             ┌──────────────┐
             │  DDC Block   │
samp_rate[i] │              │ samp_rate[i]
──────────── > - freq[i]    ├──────────────>
             │ - decim[i]   │
             │              │
             └──────────────┘
```

\b Note: The index of edge properties is tightly coupled to the actual edge
being used (i.e., `samp_rate` on input edge 0 also has a property index of 0).
However, user properties do not enforce this relationship. In the DDC block,
it would be possible for the user property `decim[0]` to control channel 1 (which
it doesn't). This is useful for properties that don't map directly to channels
(e.g., a simpler version of the DDC block might only have a single `decim` user
property for all channels).

In summary, to accurately identify a property, not only its name is required, but
also its type (input edge, user property, or output edge) and its index. For the
sake of convenience, the type and index are sometimes combined into a
uhd::rfnoc::res_source_info object.

\anchor props_define
### Defining Properties

To define properties in an RFNoC block, the block controller needs to declare
class attributes of type uhd::rfnoc::property_t. This is a template type which can
take in different types of C++ types (e.g., the `decim` user property is an
integer type, and the `freq` user property is of type `double`). Upon creation
of the class attribute, properties are configured with their property name (e.g.
`freq`, `decim`), optionally a default value, their type (user or edge property),
and their index. The uhd::rfnoc::ddc_block_control defines its frequency property
attribute variable as such:

~~~{.cpp}
class ddc_block_control_impl : public ddc_block_control
{
   // ...all the other declarations ...
private:
    // Declare the property_t attributes for the frequency
    std::vector<property_t<double>> _freq;
};
~~~

Next, the attribute needs to be initialized, like any other class attribute.
If we were to hard-code a 2-channel DDC with a default frequency shift of 0 Hz,
this would be a valid initialization:
~~~{.cpp}
// ... other initializations ...
_freq.reserve(2);
_freq.push_back(property_t<double>("freq", 0.0, {res_source_info::USER, 0}));
_freq.push_back(property_t<double>("freq", 0.0, {res_source_info::USER, 1}));
~~~

\b Note: It is important to choose a container for properties which does not
modify their memory locations! If a `std::vector<>` is chosen as a container, it
is required to `reserve()` memory before filling the container.

Up until now, the property is just a class attribute. For UHD to make it
available for property propagation, it needs to be registered by calling
uhd::rfnoc::node_t::register_property():

~~~{.cpp}
// ... this is still happening within the context of the block controller:
register_property(&_freq[0]);
register_property(&_freq[1]);
~~~

The final step of creating properties is to define the relationship between
them, which is covered in the following section.

\anchor props_resolvers
### Property Resolvers and Property Resolution

A property resolver is a function object that is associated with an input list
of properties and an output list of properties. The function object will be
executed when any property on the input list changes. It may modify any property
in the output list. For more details, see uhd::rfnoc::node_t::add_property_resolver().

As an example, here is a shortened version of one of the resolvers in
uhd::rfnoc::ddc_block_control:

~~~{.cpp}
add_property_resolver(
    {&decim}, // Input list: We trigger this when the user changes `decim`
    {&decim, &samp_rate_out, &samp_rate_in}, // Output list: These will be changed
    // The resolver function is passed in as a lambda:
    [this, chan, ...]() { // The capture list was cropped for better readability
	RFNOC_LOG_TRACE("Calling resolver for `decim'@" << chan);
	decim = coerce_decim(double(decim.get())); // Only some decimations are valid
	if (decim.is_dirty()) { // If nothing changes, don't poke any registers
	    set_decim(decim.get(), chan); // This will control hardware registers
	}
	if (samp_rate_in.is_valid()) {
	    samp_rate_out = samp_rate_in.get() / decim.get();
	} else if (samp_rate_out.is_valid()) {
	    samp_rate_in = samp_rate_out.get() * decim.get();
	}
    });
~~~

A few noteworthy comments:
- Even though this function object is called when modifying `decim`, it is still
  possible to coerce `decim` to a different, valid value.
- Properties may be in an invalid state (e.g., if they were never initialized)
  which needs to be checked before accessing such properties. See also
  uhd::rfnoc::property_base_t::is_valid().
- Properties can be modified by assigning values to them which match their
  template type.
- We can tell if a property was modified by checking its clean/dirty state
  (see also uhd::rfnoc::property_base_t::is_dirty()).
- When the resolver function completes, we have ensured that `samp_rate_in`,
  `samp_rate_out`, and `decim` are in a consistent state.

\anchor props_resolvers_cldrty
#### Clean/Dirty Attributes of Properties
To keep track of which properties have been modified, every property has a "dirty"
flag that is set when a property's value is changed. This dirty flagged is also
used to determine which property resolver functions need to be called. A property
resolver may modify (and thus dirty) other properties, but resolvers may never
set properties to conflicting values. For example, the DDC block controller has
a different resolver that is called when its input sampling rate is modified,
but it uses the same equations to relate decimation, input rate, and output rate
so the resulting values are consistent.

Imagine a block that has both an `fft_size` and `bin_width` user property. They
are related by `bin_width = sampling_rate_in / fft_size`, where `sampling_rate_in`
is an input edge property. Adding two separate resolvers like this is valid:

~~~{.cpp}
add_property_resolver(
    {&fft_size},
    {&bin_width},
    [this, &fft_size, &bin_width, &samp_rate_in]() {
    	// This will trigger the other resolver
    	bin_width = samp_rate_in.get() / fft_size.get();
    });
add_property_resolver(
    {&bin_width},
    {&fft_size},
    [this, &fft_size, &bin_width, &samp_rate_in]() {
    	// This will trigger the other resolver
    	fft_size = samp_rate_in.get() / bin_width.get();
    });
~~~

Both resolvers will modify the input for the other resolver. However, because
they use the same equations, this does not incur a circular resolution.

\b Note: When using floating-point type properties, it is possible to incur
a circular resolution when floating point rounding errors occur, so they need
to be accounted for. See, for example, the implementation of
uhd::rfnoc::ddc_block_control.

When all resolvers with dirty inputs have been run, and no conflicts have
occurred, properties are flagged clean. When this happens, a clean callback is
executed which can trigger further actions (it may not, however, modify
properties. See uhd::rfnoc::node_t::register_property()).

As an example, assume a block configures an FFT size. It first checks the FFT
size is as power of two, then writes the log2 of the FFT size to a hardware
register.

The following two implementations have the same effect. First, we use the resolver
to write to the hardware:
~~~{.cpp}
register_property(&fft_size);
add_property_resolver(
    {&fft_size},
    {&fft_size},
    [this, &fft_size]() {
    	fft_size = coerce_to_power_of_2(fft_size.get());
	if (fft_size.is_dirty()) {
	    // FFT_SIZE_REG stores address of FFT size register
	    this->regs().poke32(FFT_SIZE_REG, log2(fft_size.get()));
	}
    });
~~~

Alternatively, we poke the register as part of cleaning the property:
~~~{.cpp}
register_property(
    &fft_size,
    [this, &fft_size](){ this->regs().poke32(FFT_SIZE_REG, log2(fft_size.get()); }
);
add_property_resolver(
    {&fft_size},
    {&fft_size},
    [this, &fft_size]() {
    	fft_size = coerce_to_power_of_2(fft_size.get());
    });
~~~

The differences are subtle:
- The second approach separates the hardware interaction from the
  coercion/resolution logic. Here, the resolver is *only* responsible for
  maintaining the property in a valid state.
- In the first approach, the register is poked as soon as the new value is
  known. In the second approach, the poke happens only after the resolution is
  complete.
- The manual step of checking the clean/dirty flag is used to avoid unnecessary
  writes to hardware in the first approach. It is not required in the second
  approach, because the write to hardware is coupled with the act of cleaning
  the variable.

In this particular instance, the second approach is the more readable and is
recommended. However, not always does changing properties map to poking
registers (or other actions) in a clean, one-to-one manner, which is when the
first approach may be better suited.

\anchor props_graph_resolution
### Graph Property Resolution
In an RFNoC graph, after calling `uhd::rfnoc::rfnoc_graph::commit()`, edge
properties are used to resolve properties of a whole graph. When any property is
changed, the corresponding block's properties are resolved as explained before.
However, by modifying the edge properties of a block, other blocks' properties
may be dirtied as well. In this case, the resolver algorithm will keep resolving
blocks until either all properties are clean, or a conflict is detected.

\anchor props_graph_resolution_back_edges
#### Back edges

The graph object uses a topological sort to identify the order in which blocks
are resolved. However, in RFNoC, it is OK to have loops, or *back edges*.

Consider the following graph:

```

  ┌─────────────┐          ┌──────────────┐          ┌──────────────┐
  │             │          │              │          │              │
  │ Radio Block ├──────────> DDC Block    ├──────────> Custom DSP   │
  │             │          │              │          │              │
  └─────^───────┘          └──────────────┘          └──────┬───────┘
        │                                                   │
        │                back edge                          │
        └───────────────────────────────────────────────────┘
```

In this application, we receive a signal, use the DDC block to correct a
frequency offset digitally, and then apply some custom DSP before transmitting
the signal again, using the same radio block. In order to create a valid graph
that UHD can handle, one of the edges needs to be declared a back-edge.

When declaring an edge as a back-edge, this signals to UHD that this edge should
not be used for viewing the graph as a directed, acyclic graph (DAG), which is
important during property propagation. In the graph above, UHD would sort blocks
topologically (Radio Block, DDC Block, Custom DSP Block) by ignoring back-edges.
Then, properties are forwarded in topological order first across forward edges,
then across back edges.

This can lead to resolution errors.  For example, if the DDC were to be
configured to decimate the sampling rate, and the custom DSP block would not
correct for that, the output edge property `samp_rate` of the custom DSP block
would be mismatched compared to the input edge property `samp_rate` of the radio.
This could either re-trigger a new cascade of property settings, which means
the algorithm can't converge, and UHD would detect that and throw an exception.
Or the radio block might not accept a sample rate update on its input port,
which would also trigger an exception.

It is highly recommended to not declare edges as back-edges unless
necessary. In the graph above, any of the three edges could have been declared
a back-edge to result in a topologically valid graph that can still resolve its
properties, but the one chosen is the most intuitive selection.

\anchor props_unknown
### Handling unknown properties
Since every block can define its own edge- and user properties, it is likely that
a block may not have defined an edge property that an up- or downstream block
has.

Consider the example of the previous section. The "Custom DSP" block may not
have defined edge properties for the sampling rate. The Radio and DDC blocks
however, do have a `samp_rate` edge property defined.

The way blocks handle such properties is by setting a *forwarding policy* (see
uhd::rfnoc::node_t::set_prop_forwarding_policy() and
uhd::rfnoc::node_t::set_prop_forwarding_map()).

There are several forwarding policies (see uhd::rfnoc::node_t::forwarding_policy_t).
The most common forwarding policy is uhd::rfnoc::node_t::forwarding_policy_t::ONE_TO_ONE.
Here, properties that are applied to an input edge are forwarded to the
corresponding output edge. For more special use cases,
uhd::rfnoc::node_t::forwarding_policy_t::USE_MAP can be used to define
forwarding rules for non-static cases (e.g., see uhd::rfnoc::switchboard_block_control).

\anchor props_common_props
### Common Properties
There are some properties that are commonly used, and blocks should use these
property names if appropriate:

Edge Properties:
- `type`: It is recommended to always use this property on an edge. It should
  describe the data type on this edge so the graph can see if types match. The
  type descriptions are the same as with CPU and OTW types (e.g., sc16, fc32,
  sc8, u8, ...).
- `samp_rate`: Whenever a "sampling rate" is applicable, edges should set this
  property. DDC, DUC, and Radio blocks all use this for configuring rates and
  verifying sample rates match.
- `scaling`: This property describes how a block has distorted a signal with
  respect to amplitude. A scaling value of 0.99 means that a signal that used
  to be fullscale (max. amplitude of 1.0) will only have a maximum amplitude of
  0.99. For example, the DDC and DUC blocks use this property to signal how
  much they have affected the amplitude. The TX and RX streamer objects can then
  apply the inverse of this scaling factor to correct for the scaling before
  returning the signal to the user.
- `atomic_item_size`: Blocks (e.g. uhd::rfnoc::radio_control) might need a 
  non-dividable amount of data per clock cycle, e.g. the radio block needs
  `sample_per_cycle` times the size of the type bytes of data in each cycle.
  This property allows a block to specify this amount of data per cycle which
  allows other blocks or the streamer objects to adapt the samples per packet
  accordingly.
  Note: Because all blocks in a graph must agree on one value for this property
  the blocks have to calculate the `atomic_item_size` as the least common
  multiple of their own `atomic_item_size` and the edge property to accommodate 
  for other blocks up- or downstream.

User properties:
- `spp`: Blocks that produce data in chunks of samples can use this to describe
  their "sample per packet" values. Example: uhd::rfnoc::radio_control
- `interp`, `decim`: Interpolation and decimation factors when doing rational
  resampling. Examples: uhd::rfnoc::ddc_block_control, uhd::rfnoc::duc_block_control

See also the following sections for properties that receive special treatment by
the framework.

\anchor props_special_props
### Special Properties
There is a small number of edge properties that treated differently by the RFNoC
framework.

\anchor props_special_props_tickrate
#### Tick Rate (\`tick_rate\`)
The "tick rate" is the rate that is used for converting floating point timestamps
into integer ticks. The tick rate has the additional constraint that within a
graph, the tick rate must be the same for all blocks. Therefore, the tick rate
is defined by the framework for all blocks that derive from uhd::rfnoc::noc_block_base,
and block authors cannot register properties named `tick_rate`.

The tick rate property is propagated to all edges to enforce all blocks having
the same tick rate. Some blocks (like the radio blocks) set the tick rate via
their direct access to the hardware, but most blocks use the property propagation
mechanism to distribute the tick rate.

To read back the tick rate, use uhd::rfnoc::noc_block_base::get_tick_rate().
Within a block implementation, the uhd::rfnoc::noc_block_base::set_tick_rate()
API call can be used to update the tick rate (most blocks do not have to do
this).

\anchor props_special_props_mtu
#### MTU (\`mtu\`)
Because the maximum transmission unit (MTU) is part of the RFNoC framework (it
is constrained by the buffer size chosen between blocks, and is thus read back
from a register in the FPGA), its property is also created by the RFNoC
framework for all blocks, and block authors cannot use the name `mtu` for their
properties.

However, MTUs can differ within a graph, and blocks might have additional
constraints on MTU. For this reason, accessing the MTU properties directly is
not possible from within a block controller, but rfnoc::noc_block_base provides
several APIs to interact with MTUs:

- noc_block_base::get_mtu(): Reading back the MTU determined by property
  propagation on a particular edge.
- noc_block_base::set_mtu(): Reduce the MTU on a particular edge. *Increasing*
  the MTU is not possible.
- noc_block_base::set_mtu_forwarding_policy(): Set the MTU forwarding policy.
- noc_block_base::get_mtu_prop_ref(): Request access to a MTU property reference,
  which can be used to trigger property propagation off of an MTU change.

Many blocks (e.g., uhd::rfnoc::ddc_block_control, uhd::rfnoc::duc_block_control)
do not resize blocks as they pass through them. That means the input MTU of
these blocks equals the output MTU. Thus, they set the MTU forwarding policy to
uhd::rfnoc::node_t::forwarding_policy_t::ONE_TO_ONE.


### C++ API
\li \ref uhd::rfnoc::property_t "property_t class"

## Block Actions

An action is an ephemeral operation that can be performed on a block. An action does not change the state of a block (although the block can implement an action handler that will change its state). Like a property, an action may propagate across the graph. Actions provide a mechanism for blocks to communicate in the software domain (i.e., not by sending command CHDR packets, but by calling into APIs provided by the graph). An example of an action is a stream command. A stream command that issues on a block that is not a source or a sink will eventually propagate through the topology to a source or sink, and be executed there. Actions are exposed through the public API via an action code and an action payload. Actions are defined by the block designer, and they can be handled internally in the block or be forwarded on an upstream or downstream port.

### C++ API

The uhd::rfnoc::noc_block_base  class provides functionality to access registers, properties and to execute actions on a block. The API for the block is detailed below (Note that all fields/functions that are public are intended for the users (clients) of a block; and all fields/functions that are protected are intended for the designers of custom block controllers).

\li \ref uhd::rfnoc::noc_block_base "noc_block_base class"

## Custom Block Controllers

Custom block controllers can be built by inheriting from uhd::rfnoc::noc_block_base . The protected functions in the base class must be used to correctly register properties and handle actions. UHD will provide a mechanism to retrieve controllers for discovered blocks, and it will have the capability to ```dynamic_cast``` the generic block controller to the user-defined type. The following is an example of a sample custom block controller.

```cpp
class null_block_control_impl : public null_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(null_block_control)
    {
        uint32_t initial_state = regs().peek32(REG_CTRL_STATUS);
        _nipc = (initial_state >> 24) & 0xFF;
        _item_width = (initial_state >> 16) & 0xFF;
        register_property(&_lpp);
        add_property_resolver(
            {&_lpp}, // Input/trigger list
            {&_lpp}, // Output list
            [this](){
                set_lines_per_packet(_lpp.get());
                _lpp = get_lines_per_packet();
            });
        register_issue_stream_cmd();
    }

    void issue_stream_cmd(const stream_cmd_t& stream_cmd)
    {
        /* Implement functionality to start and stop streaming */
    }

    void set_lines_per_packet(const uint32_t lpp)
    {
        const uint32_t reg_val = /* figure out from lpp */;
        regs().poke32(REG_SRC_LINES_PER_PKT, reg_val);
    }

    uint32_t get_lines_per_packet()
    {
        return regs().peek32(REG_SRC_LINES_PER_PKT) + 2;
    }

private:

    /*! Action API: Register a handler for stream commands
     */
    void register_issue_stream_cmd()
    {
        register_action_handler(ACTION_KEY_STREAM_CMD,
            [this](const res_source_info& src, action_info::sptr action) {
                stream_cmd_action_info::sptr stream_cmd_action =
                    std::dynamic_pointer_cast<stream_cmd_action_info>(action);
                if (!stream_cmd_action) {
                    throw uhd::runtime_error(
                        "Received stream_cmd of invalid action type!");
                }
                if (src.instance != 0 || src.type != res_source_info::OUTPUT_EDGE) {
                    throw uhd::runtime_error(
                        "The null source can only stream from output port 0!");
                }
                RFNOC_LOG_DEBUG("Received stream command action request!");
                issue_stream_cmd(stream_cmd_action->stream_cmd);
            });
    }

    void deinit()
    {
        // This is the last time we can do any kind of peek or poke
        issue_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    }


    /**************************************************************************
     * Attributes
     *************************************************************************/
    property_t<int> _lpp{"lpp", 100, {res_source_info::USER}};

    //! Number of items per clock
    uint32_t _nipc;

    //! Bits per item
    uint32_t _item_width;
};
```

# RFNoC Graph

The graph object in UHD allows users to build functional applications using individual blocks by connecting them together in a meaningful topology. The RFNoC graph has the following properties:

- The *edges* of the graph represent the data flow paths. It is thus directed.

- There can be multiple connections between nodes (e.g., for blocks that handle MIMO applications). It is thus a multigraph.

- A block can have multiple inputs or outputs (e.g., a summation block might have two inputs and one output). A property of an edge is thus which port of a node it is connected to. If there are multiple edges between the same nodes, they are not interchangeable.

- It is *disconnected*, meaning there can be multiple, disconnected sub-graphs.

- It cannot have *loose (unconnected) edges*.

- If the blocks support such a usage, data connections can be cyclic.

- All nodes have a unique identifier of type ```string```.

## Capabilities

There are three types of connections (edges) supported by the graph:

- Block to Block: A connection between two blocks that are connected by some transport

- Block to Stream: A connection between a block in the FPGA and a host-resident RX streamer

- Stream to Block: A connection between a host-resident TX streamer and a block in the FPGA

The graph object is responsible for making the appropriate data connections between blocks and/or the host streamers. It is also responsible for propagating port (edge) properties between blocks. Multi-block state resolution is handled by the framework and is not the responsibility of the application designer.

*Note*: The architecture of RFNoC does not preclude software running on a remote computer that acts like an RFNoC device. From the perspective of the graph, this would also register as a block controller, even if the remote computer is using a streamer object. When the software is running on the same context as the UHD session, it is considered a streamer.

## C++ API
\li \ref uhd::rfnoc::rfnoc_graph "rfnoc_graph class"

# Streamers

Streamers allow users to send data to or receive data from an RFNoC block. The API for streamers will be the same as what multi_usrp has today.

## C++ API
\li \ref uhd::rx_streamer "rx_streamer class"
\li \ref uhd::tx_streamer "tx_streamer class"


# Motherboard Controllers

The motherboards are represented by a "motherboard controller". This object can be used to configure, modify, or query properties that are specific to the motherboard itself, and are not tied to any particular block. A very common motherboard-level setting is the source for time and/or clock reference. The timekeepers are also controlled on the motherboard level and are controlled through the motherboard controller.

The indexing for motherboard controllers is consistent between block IDs and device arguments. For example, suppose an uhd::rfnoc::rfnoc_graph was created with the arguments ```addr0=192.168.10.2```, ```addr1=192.168.10.3```, in which case there will be two motherboards in this RFNoC graph. A call to ```get_mb_controller(0)``` will return the motherboard controller for the first motherboard (the one who's IP address ends in 10.2 in this example), and the block ID 0/Radio#0 will correspond to the first radio on this same motherboard.

In RFNoC versions prior to UHD 4.0, the only API to interact with the graph were the block controllers. This left an API gap, and some API calls that affected the motherboard were attached to the radio block controllers instead. By splitting up block controllers and motherboard controllers, API calls are attached to the actual component they're controlling.

However, block controllers may request access to the motherboard controllers themselves, which they sometimes require (e.g., to control or query clocks on the motherboard). This mechanism may also allow blocks, such as the radio blocks, to expose functionality that is technically tied to the motherboard.

# uhd::multi_usrp API

An RFNoC capable USRP must work out of the box using the multi_usrp API. To do so, multi_usrp will expect a default image with Radios, DDCs, DUCs and buffering to implement the native USRP API. Internally, multi_usrp will build a graph to make the appropriate connections, much like a user application.