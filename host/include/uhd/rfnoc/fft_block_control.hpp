//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

enum class fft_shift { NORMAL, REVERSE, NATURAL, BIT_REVERSE };
enum class fft_direction { REVERSE, FORWARD };
enum class fft_magnitude { COMPLEX, MAGNITUDE, MAGNITUDE_SQUARED };

// Custom property keys
static const std::string PROP_KEY_MAGNITUDE          = "magnitude";
static const std::string PROP_KEY_DIRECTION          = "direction";
static const std::string PROP_KEY_LENGTH             = "length";
static const std::string PROP_KEY_FFT_SCALING        = "fft_scaling";
static const std::string PROP_KEY_FFT_SCALING_FACTOR = "fft_scaling_factor";
static const std::string PROP_KEY_SHIFT_CONFIG       = "shift_config";
static const std::string PROP_KEY_BYPASS_MODE        = "bypass_mode";
static const std::string PROP_KEY_CP_INSERTION_LIST  = "cp_insertion_list";
static const std::string PROP_KEY_CP_REMOVAL_LIST    = "cp_removal_list";
static const std::string PROP_KEY_NIPC               = "nipc";
static const std::string PROP_KEY_MAX_LENGTH         = "max_length";
static const std::string PROP_KEY_MAX_CP_LENGTH      = "max_cp_length";
static const std::string PROP_KEY_MAX_CP_INSERTION_LIST_LENGTH =
    "max_cp_insertion_list_length";
static const std::string PROP_KEY_MAX_CP_REMOVAL_LIST_LENGTH =
    "max_cp_removal_list_length";
// clang-format off
/*! FFT Block (with Cyclic prefix insertion and removal) Control Class
 *
 * \ingroup rfnoc_blocks
 * 
 * # Overview
 * The FFT block is an RFNoC block that accepts signed complex 16-bit data
 * at its input and computes the forward or reverse FFT of the input data,
 * outputting signed complex 16-bit data at its output.
 *
 * The FFT length is configured via the length parameter, up to a maximum
 * which depends on the instantiation on the FPGA. Use the function
 * get_max_fft_length to determine the maximum supported FFT length.
 *
 * The length will be coerced to the closest power of two which is smaller
 * than length. The block will output packets of the same length in the
 * desired format as configured via the API.
 *
 * The block can be configured to add cyclic prefixes (typically when
 * performing an inverse FFT, i.e. repeating a part of the generated time domain
 * signal) or to remove cyclic prefixes (typically when performing a forward
 * FFT, i.e. removing a part of the input time domain signal). This feature
 * makes this block suitable for OFDM (de-)modulation.
 * 
 * See <a href="https://github.com/EttusResearch/uhd/blob/master/host/examples/python/rfnoc_txrx_fft_block_loopback.py"> RFNoC FFT block example </a> 
 * in the uhd repository for how to use the API.
 *
 *
 *
 * # Features

 *
 * 1. Implements FFT and iFFT operations.
 *     - Based on Xilinx FFT IP core (<a href="https://docs.amd.com/r/en-US/pg109-xfft/Architecture-Options"> Xilinx Product Guide </a>) 
 *     - Configured for continuous streaming through the FFT block
 *     - Runtime-configurable transform sizes: \f$N=2^m, m∈\{1,2,…16\}\f$
 * 2. Computes an (i)FFT across the full instantaneous bandwidth of any USRP
 *    through parallel multi-sample per FPGA clock cycle processing (added in UHD
 *    4.9). 
 * 3. Cyclic prefix (CP) insertion and removal (added in UHD 4.8), enabling OFDM
 *    modulation
 *     - CP lengths can be varied as a function of the OFDM symbol index through
 *       a runtime-configurable "CP schedule"
 * 4. Compile-time parameters enable / disable inclusion of FPGA logic to 
 *    (examples)
 *     - compute the magnitude or square magnitude of the FFT output (useful for
 *       power spectral density estimation)
 *     - switch between computing or bypassing the (i)FFT at runtime
 *     - add/remove cyclic prefixes
 * 5. The FFT window can span across multiple CHDR packets (added in UHD 4.8)
 *
 *
 * # Theory of Operation
 *
 * The RFNoC FFT block can operate on signals at very wide bandwidth by processing
 * multiple (NIPC , Number of Items Per Cycle) samples per cycle. That is, it can
 * be used at master clock rates (MCR) that well exceed the FPGA clock rate. This
 * capability is achieved by instantiating multiple, so called, (i)FFT pipelines
 * in parallel.
 *
 *
 * <img src="rfnoc_fft_pipelines.svg"  width="600" >
 *
 * An (i)FFT pipeline computes either an iFFT or an FFT and can implement
 * additional operations. Some of these operations require inclusion of FPGA logic
 * at compile time. Some of these operations are run-time configurable as can be 
 * seen in the software interface documentation below. 
 * The RFNoC FFT block also offers the option to bypass
 * the (i)FFT operations (pass-through of the input signal). 
 *
 * An (i)FFT pipeline is internally structured as shown below. Some of the blocks
 * are present only if the respective compile-time parameter is enabled.
 *
 * <img src="rfnoc_fft_dsp.svg"  width="800" >
 *
 *
 * These are the operations each block implements:
 * - CP removal: removal of a configurable number of samples preceding each FFT
 *   window, as required in an OFDM receiver
 * - (i)FFT: FFT or iFFT operation, implemented by wrapping a Xilinx FFT block
 *   configured in pipelined streaming I/O architecture.
 *   - FFT (forward FFT): the frequency domain samples X(k) are computed from the
 *     time domain sequence x(n) through. 
 *     \f$ X(k)=1/S ∑_{n=0}^{N-1} x(n) e^{-j2πnk/N}, k=0,1,…,N-1 \f$ <br>
 *     S is the scaling factor.
 *   - iFFT (inverse FFT):
 *     \f$ x(n)=1/S ∑_{k=0}^{N-1} X(k) e^{j2πnk/N}, n=0,1,…,N-1\f$
 * - Bit reversal: Xilinx IP is configured for outputs in bit-reversed order. This
 *   block arranges the output into natural order. 
 * - FFT shift: shifts DC to the center of the FFT output.
 * - CP insertion: prepending a copy of a configurable number of last samples of an
 *   iFFT output as required in an OFDM transmitter.
 * - (Squared) Magnitude (\f$| |, | |^2\f$): computes the (squared) magnitude of
 *   each output sample.
 *
 *
 * # FPGA Compile-Time Configuration
 *
 * Users can configure the blocks' capabilities at FPGA compile-time by configuring a 
 * set of parameters in the RFNoC image core yaml file where the block is instantiated.
 *
 * ## User-Definable Parameters
 *
 * User-definable FPGA compile-time parameters are documented in the blocks' top-level 
 * system verilog file rfnoc_block_fft.sv and listed below. 
 * \snippet fpga/usrp3/lib/rfnoc/blocks/rfnoc_block_fft/rfnoc_block_fft.sv FFT_FPGA_DOCU
 *
 * The block description file fft.yml lists the default parameter values.
 * \snippet host/include/uhd/rfnoc/blocks/fft.yml FFT_YML
 *
 * The table below summarizes the valid parameter ranges. A range without upper bound 
 * implies that the maximum value for a parameter is dictated by the available 
 * FPGA resources, but not by the FPGA IP design.
 *
 * | Compile-Time Parameter | Range                          |  Comment                                             |
 * |------------------------|--------------------------------|------------------------------------------------------|
 * | NIPC                   | \f$ 2^N, N=\{0, 1, 2, ...\}\f$ |                                                      |
 * | NUM_PORTS              | 1, 2, 3, ...                   | Must be integer-divisible by NUM_CORES               |
 * | NUM_CORES              | 1, 2, 3, ...                   |                                                      |
 * | MAX_FFT_SIZE_LOG2      | 10, 11,…,16                    | 1k FFT … 64 k FFT                                    |
 * | EN_CP_REMOVAL          | 0, 1                           |                                                      |
 * | EN_CP_INSERTION        | 0, 1                           |                                                      |
 * | MAX_CP_LIST_LEN_INS_LOG2 | 1, 2, 3, ...                 |                                                      |
 * | MAX_CP_LIST_LEN_REM_LOG2 | 1, 2, 3, ...                 |                                                      |
 * | CP_INSERTION_REPEAT    | 0, 1                           |                                                      |
 * | CP_REMOVAL_REPEAT      | 0, 1                           |                                                      |
 * | EN_FFT_BYPASS          | 0, 1                           |                                                      |
 * | EN_FFT_ORDER           | 0, 1                           |                                                      |
 * | EN_MAGNITUDE           | 0, 1                           |                                                      |
 * | EN_MAGNITUDE_SQ        | 0, 1                           |                                                      |
 * | USE_APPROX_MAG         | 0, 1                           | Estimate of the magnitude that saves FPGA resources. |
 *
 * ## Choosing Parameter Values
 *
 * It is important to understand the high-level architecture of the RFNoC FFT 
 * block to pick proper compile-time parameters. The figure below introduces the 
 * different components that an RFNoC FFT block is composed of. 
 *
 * <img src="rfnoc_fft_impl.svg"  width="700" >
 *
 * \b NUM_CORES, \b NUM_PORTS, \b NIPC
 *
 * These parameters allow to reduce FPGA resource utilization by exploiting that 
 * multiple channels may be processed using the same compile-time or run-time 
 * parameters.
 *
 * An RFNoC FFT block can be configured to contain multiple (i)FFT pipelines. 
 * Multiple (i)FFT pipelines are useful to process multiple channels within a 
 * single RFNoC FFT block. This is more resource-efficient compared to processing
 * each channel in a dedicated RFNoC FFT block. More specifically, an RFNoC FFT 
 * is internally sub-divided into one or multiple FFT cores. Each FFT core is 
 * further sub divided into one or multiple FFT pipeline wrappers.  Finally, each
 * FFT pipeline wrapper can contain one or multiple FFT pipelines as shown in the
 * figure above.
 *
 * All **FFT cores** share the same compile time configuration but can have 
 * individual runtime configurations. For instance, the FFT direction and size or
 * the CP length may be chosen differently per FFT core, at runtime. The maximum 
 * FFT size on the other hand is determined at compile time and shared across all
 * FFT cores. The number of input ports NUM_PORTS to the RFnoC FFT block must be 
 * integer-divisible by the number of FFT cores. The first set of 
 * NUM_CHAN=NUM_PORTS / NUM_CORES ports will be processed by the first FFT core, 
 * the second set of NUM_PORTS / NUM_CORES ports by the second FFT core and so 
 * forth.
 *
 * All channels allocated to an FFT core must be used simultaneously, 
 * otherwise the FFT core will stall.
 *
 * Each channel processed by an FFT core is assigned its dedicated FFT pipeline 
 * wrapper. The parameter NIPC controls the number samples per FPGA clock cycle 
 * to be processed by the FFT pipeline wrapper. The number of FFT pipelines per 
 * FFT core equals NIPC. NIPC can be computed from ceil(MCR / CE). CE is the 
 * compute engine clock rate (see: 
 * https://kb.ettus.com/RFNoC_Frequently_Asked_Questions#What_are_the_clock_frequencies.3F)  
 * and MCR the master clock rate.  Also, some margin is needed. That is, as a 
 * rule of thumb ceil(MCR / CE) /  (MCR / CE) > 1.05 should be satisfied. 
 *
 * Example: Assume we are using an MCR of 491.52 MS/s on USRP X410 ( CE = 
 * 266.667 MHz). MCR / CE = 1.84, ceil(MCR / CE)=2  and ceil(MCR / CE) /  
 * (MCR / CE) = 1.09. NIPC=2 is sufficient in that case.
 *
 * <b> Choosing compile-time configurations to minimize FPGA resource utilization
 * </b>.
 *
 * - Enable only the logic that is required (example: CP removal / addition, 
 *   (squared) magnitude computation, pick the minimal maximum FFT size required 
 *   by the application)
 * - Process as many channels as possible within a single RFNoC FFT block and FFT
 *   core. Suppose you want to process N channels. Then the most 
 *   resource-efficient configuration is to use a single RFNoC FFT block with 
 *   NUM_PORTS = N and NUM_CORES = 1. Of course, this is only possible if all 
 *   channels share the same compile and runtime configuration and are processed 
 *   simultaneously.
 *
 * <b> Dependent compile-time parameters  </b>
 * - EN_FFT_ORDER and EN_CP_INSERTION: The  bit reversal, FFT shift and CP 
 *   insertion blocks share FPGA logic. 
 * - EN_CP_INSERTION = 1 requires EN_FFT_ORDER = 1, otherwise the FPGA compile will 
 * fail. 
 *
 *
 * ## Example FPGA Compile-Time Configuration
 * 
 * An example of a typical OFDM transmitter and receiver configuration covering 
 * 400 MHz of RF bandwidth on an USRP X410 is shown below. A maximum 8192 size 
 * FFT is configured leading to a minimal subcarrier spacing of 60 kHz. At the 
 * transmit side, cyclic prefix insertion is enabled. At the receive side, cyclic
 * prefix removal is enabled. 
 * 
 * | Compile-Time Parameter | Value Transmitter | Value Receiver |
 * |------------------------|-------------------|----------------|
 * | NIPC                   | 2                 | 2              |
 * | NUM_PORTS              | 2                 | 2              |
 * | MAX_FFT_SIZE_LOG2      | 13                | 13             |
 * | EN_CP_REMOVAL          | 0                 | 1              |
 * | EN_CP_INSERTION        | 1                 | 0              |
 * | EN_MAGNITUDE           | 0                 | 0              |
 * | EN_MAGNITUDE_SQ        | 0                 | 0              |
 *
 * # Advanced Topics
 *
 * ## Xilinx FFT Block Configuration
 *
 * The RFNoC FFT block is based on a Xilinx FFT block. Different configurations 
 * for different maximum FFT sizes have been prepared. The respective Xilinx 
 * settings for IP core generation can be obtained by opening the respective .xci
 * files in Xilinx Vivado (example for the maximal 16k FFT: <a 
 * href="https://github.com/EttusResearch/uhd/blob/master/fpga/usrp3/lib/ip/xfft_16k_16b/xfft_16k_16b.xci"> 
 * xfft_16k_16b.xci </a>)
 *
 * An example is shown below.
 *
 * <img src="rfnoc_fft_xilinx.png" >
 *
 * ## Implementing Different Subcarrier Spacings in OFDM Systems
 *
 * When implementing an OFDM system, there is a target subcarrier spacing 
 * (inter FFT frequency bin spacing), typically.
 *
 * <b> Case 1 </b>: FPGA design <b> does not </b> implement sample rate conversion 
 * through DUC/DDC
 *
 * <img src="rfnoc_fft_scs_no_ddc.svg"  width="700" >
 *
 * Assume the (i)FFT block is directly connected to the radio block, i.e., the 
 * input rate to the FFT block or the output rate of the FFT block equals the 
 * master clock rate (MCR). The subcarrier spacing \f$ Δf_{sc} \f$ is <br>
 *
 * <div align="center">
 * \f$ Δf_{sc} = MCR / N_{FFT} \f$
 * </div>
 *
 * For instance, on USRP X410 a master clock rate of 245.76 MHz and an FFT size 
 * \f$ N_{FFT} \f$ of 8192 result in a \f$ Δf_{sc}=30 kHz \f$ subcarrier 
 * spacing.
 *
 * <b> Case 2 </b>: FPGA design <b> does </b> implement sample rate conversion 
 * through DUC/DDC
 *
 * <img src="rfnoc_fft_scs_ddc.svg"  width="700" >
 *
 * Assume the (i)FFT block is connected to the radio block through a DDC/DUC 
 * block implementing a sample rate conversion factor R. The subcarrier spacing 
 * \f$ Δf_{sc} \f$ is
 *
 * <div align="center">
 * \f$ Δf_{sc} = MCR / {R N_{FFT}} \f$
 * </div>
 *
 * For instance, on USRP X410 a master clock rate of 245.76 MHz, an FFT size of
 * 4096 and a sample rate conversion factor of R=2 result in a 30 kHz subcarrier
 * spacing. 
 *
 * ## Approximate Processing Latency
 *
 * The approximate processing latency of the RFNoC FFT block in cycles 
 * \f$ N_{Lat} \f$ is <br>
 *
 * <div align="center">
 * \f$ N_{Lat}≈NIPC⋅(N_{FFT}+N_{CP}) \f$
 * </div>
 *
 * The latency \f$ T_{Lat} \f$ in seconds depends on the compute engine clock rate 
 * CE <br>
 *
 * <div align="center">
 * \f$ T_{Lat}≈N_{Lat}/CE \f$
 * </div>
 *
 * Example for a 400 MHz USRP X410 OFDM configuration: 
 * 	- MCR = 491.52 MS/s 
 * 	- CE = 266.667 MHz
 * 	- NIPC = 2
 * 	- N_FFT=8192, N_CP=576 (subcarrier spacing = 60 kHz) <br>
 *
 * <div align="center">
 *   \f$ T_{Lat}≈N_{Lat}/CE≈17536/(266.667⋅10e6) s≈66us \f$
 * </div>
 *
 * ## Time Stamping
 *
 * CHDR data packets containing FFT input or output data can comprise time 
 * stamps. The figure below illustrates how these are computed for the example:
 *  - OFDM configuration – cyclic prefixes are added or removed
 *  - Burst containing 2 OFDM symbols
 *
 * <img src="rfnoc_fft_timestamp.svg"  width="800" >
 *
 *  1.	For the first packet in a burst: <br> 
 *      The incoming packet contains a time stamp for the start of the burst 
 *      that is provided by either the host or the radio block. This time stamp 
 *      is in units of the master clock rate. The packet at the output of the 
 *      (i)FFT block contains this exact same timestamp without modifications. 
 *     In other words, the FFT block doesn't adjust the timestamp to 
 *     compensate for the insertion or removal of the cyclic prefix.
 *  2.	For subsequent packets in a burst: <br>
 *     The FFT block increments the time stamp for the start of the burst by 
 *     the number of samples per packet (SPP).
 *
 *
 * ## Behavior In Case of Streaming Errors
 *
 * It is possible that transmit or receive packets are dropped on the transport
 * link that connects USRP and host computer. This section discusses how to 
 * detect and handle 
 * these situations.
 *
 * <b> RX Streaming (FFT operation) </b>
 * 1.	After each receive call, check for the out_of_sequence flag in the stream
 *    metadata.
 * 2.	If the receive call metadata indicates the data is out of sequence, then 
 *    the application can use the time_spec to determine where this data goes in 
 *    the sequence of expected data and take appropriate action (e.g., discard 
 *    the data up to the start of the next symbol).
 * 3.	Streaming can continue as before after this point.
 *
 * <b> TX Streaming (iFFT operation) </b>
 * 1.	The application sets up a thread to receive and monitor asynchronous 
 *    messages ( rev_async_msg() ) from the TX streamer.
 * 2.	When an async message reports an underflow or sequence error, then the 
 *    application should end the current burst and wait for the burst ack.
 *     - The FFT block will automatically end the data stream and flush any data
 *       out of the FFT block. This may cause the last packet to be sent with 
 *       corrupted data, since part of an input symbol was lost, but should 
 *       leave the FFT block in a good state for the next burst.
 *     - If the application doesn't receive a burst ack, then it should reset 
 *       the FFT block, and reconfigure it before proceeding.
 * 3.	After this the application can restart streaming from where it left off 
 *    or at some later point in time as needed by the application.
 *
 *
 *
 * ## Timed Commands
 * The FFT block does not support timed commands. That is, none of its 
 * runtime-configurable properties can be configured in a deterministically timed 
 * fashion.
 *
 * ## Configuring the CHDR Packet Lengths for Initial Debugging
 * Debugging can be simplified by configuring the CHDR packet length such that 
 * an integer multiple of the CHDR packet length equals the FFT size. 
 * The FFT functionality does not break if this recommendation is not followed. 
 * Example: In case of a 4096 point FFT, the CHDR packet length could be chosen 
 * equal to 1024 samples per packet.
 *
 *
 * # Known Limitations
 * 1.	MAX_FFT_SIZE_LOG2 = 16 (64k FFT) can be configured but has not been 
 *    validated in hardware
 * 2.	MAX_FFT_SIZE_LOG2 = 15 (32k FFT): using this maximum-size FFT 
 *    implementation in an 8 point FFT configuration does not work properly. 
 *
 */
// clang-format on
class UHD_API fft_block_control : public uhd::rfnoc::noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(fft_block_control)

    //! Register addresses
    static const uint32_t REG_COMPAT_ADDR;
    static const uint32_t REG_PORT_CONFIG_ADDR;
    static const uint32_t REG_CAPABILITIES_ADDR;
    static const uint32_t REG_CAPABILITIES2_ADDR;
    static const uint32_t REG_RESET_ADDR;
    static const uint32_t REG_LENGTH_LOG2_ADDR;
    static const uint32_t REG_SCALING_ADDR;
    static const uint32_t REG_DIRECTION_ADDR;
    static const uint32_t REG_CP_INS_LEN_ADDR;
    static const uint32_t REG_CP_INS_LIST_LOAD_ADDR;
    static const uint32_t REG_CP_INS_LIST_CLR_ADDR;
    static const uint32_t REG_CP_INS_LIST_OCC_ADDR;
    static const uint32_t REG_CP_REM_LEN_ADDR;
    static const uint32_t REG_CP_REM_LIST_LOAD_ADDR;
    static const uint32_t REG_CP_REM_LIST_CLR_ADDR;
    static const uint32_t REG_CP_REM_LIST_OCC_ADDR;
    static const uint32_t REG_OVERFLOW_ADDR;
    static const uint32_t REG_BYPASS_ADDR;
    static const uint32_t REG_ORDER_ADDR;
    static const uint32_t REG_MAGNITUDE_ADDR;

    //! Register addresses of the FFT block version 1
    static const uint32_t REG_RESET_ADDR_V1;
    static const uint32_t REG_LENGTH_LOG2_ADDR_V1;
    static const uint32_t REG_MAGNITUDE_ADDR_V1;
    static const uint32_t REG_DIRECTION_ADDR_V1;
    static const uint32_t REG_SCALING_ADDR_V1;
    static const uint32_t REG_ORDER_ADDR_V1;

    /*! Set the FFT direction
     *
     * Sets the direction of the FFT, either forward (FORWARD) or inverse
     * (REVERSE).
     *
     * \param direction FFT direction
     */
    virtual void set_direction(const fft_direction direction) = 0;

    /*! Get the FFT direction
     *
     * Returns the current direction of the FFT.
     *
     * \returns FFT direction
     */
    virtual fft_direction get_direction() const = 0;

    /*! Set the format of the returned FFT output data
     *
     * Sets the format in which the FFT output data is returned. The following
     * formats are supported:
     *
     *     * amplitude/phase data (COMPLEX)
     *     * magnitude data (MAGNITUDE)
     *     * mag-squared data (MAGNITUDE_SQUARED)
     *
     * \param magnitude Format of the returned FFT output data
     */
    virtual void set_magnitude(const fft_magnitude magnitude) = 0;

    /*! Get the format of the returned FFT output data
     *
     * Returns the current output format of the FFT data.
     *
     * \returns Format of the returned FFT output data
     */
    virtual fft_magnitude get_magnitude() const = 0;

    /*! Set the shift configuration of the output FFT data
     *
     * Sets how the FFT output data is shifted (to get the zero frequency bin
     * to the center of the output data). The following output data shift
     * configurations are supported:
     *
     *     * Negative frequencies first, then positive frequencies (NORMAL)
     *     * Positive frequencies first, then negative frequencies (REVERSE)
     *     * Bypass the shift altogether, leaving the zero frequency bin
     *       returned first (NATURAL).
     *     * Bit-reversed order (BIT_REVERSE). This is typically the native
     *       order output by the FFT. In other words, selecting this mode may
     *       mean that the data from the FFT is not reordered. In this mode,
     *       the indices of the FFT are bit-reversed. For example, for a size
     *       16 FFT, instead of outputting bins in the order 0000, 0001, 0010,
     *       0011, etc., it outputs bins 0000, 1000, 0100, 1100, etc.
     *
     * \param shift Configuration for shifting FFT output data
     */
    virtual void set_shift_config(const fft_shift shift) = 0;

    /*! Get the shift configuration of the output FFT data
     *
     * Returns the current shift configuration of the output FFT data.
     *
     * \returns Shift configuration of the output FFT data
     */
    virtual fft_shift get_shift_config() const = 0;

    /*! Set the scaling factor for the FFT block
     *
     * This is a convenience function which can be used instead of
     * set_scaling(). Based on the given factor, it automatically sets the
     * scaling mask by evenly distributing the scaling across the active FFT
     * stages
     *
     * Examples:
     * - factor = 1.0 -> no scaling
     * - factor = 0.5 (1/2) -> the scaling will be set to 0b000000000001
     *   -> scale by 2 in the first FFT stage
     * - factor = 0.0625 (1/16) -> the scaling will be set to 0b000000001010
     *   -> scale by 4 in both the first and the second FFT stage
     * - factor = 0.03125 (1/32) -> the scaling will be set to 0b000000011010
     *   -> scale by 4 in both the first and the second FFT stage
     *   and by 2 in the third FFT stage
     *
     * \param factor Desired scaling factor
     */
    virtual void set_scaling_factor(const double factor) = 0;

    /*! Set the scaling schedule for the FFT block
     *
     * Sets the scaling for each stage of the FFT. This value maps directly
     * to the scale schedule field in the configuration channel data that is
     * passed to the Xilinx AXI FFT IP. For more information on the format
     * of this data, see Xilinx document PG109, Fast Fourier Transform
     * LogiCORE IP Product Guide.
     *
     * Examples:
     * - scaling = 0b000000000000 -> no scaling
     * - scaling = 0b000000000001 -> scale by 2 in the first FFT stage
     * - scaling = 0b000000001010 -> scale by 4 in both the first and the second
     *   FFT stage
     * - scaling = 0b000000011010 -> scale by 4 in both the first and the second
     *   FFT stage and by 2 in the third FFT stage
     *
     * \param scaling Scaling schedule for the FFT block
     */
    virtual void set_scaling(const uint32_t scaling) = 0;

    /*! Get the scaling schedule for the FFT block
     *
     * Returns the current scaling schedule for the FFT block.
     *
     * \returns Scaling schedule for the FFT block
     */
    virtual uint32_t get_scaling() const = 0;

    /*! Set the length of the FFT
     *
     * Sets the length of the FFT in number of samples. Note that the FFT
     * IP requires a power-of-two number of samples; the incoming value will
     * be coerced to the closest smaller power of two.
     *
     * \param length Desired FFT length
     */
    virtual void set_length(const uint32_t length) = 0;

    /*! Get the length of the FFT
     *
     * Returns the current length of the FFT.
     *
     * \returns Current FFT length
     */
    virtual uint32_t get_length() const = 0;

    /*! Set the bypass mode of the FFT
     *
     * Enable FFT bypass mode. Set true to enable, false to disable. When
     * enabled, the data is passed through without any FFT processing. Note that
     * cyclic prefix insertion will not work in bypass mode, because insertion
     * is handled by the FFT core, but cyclic prefix removal will work.
     *
     * \param bypass FFT bypass moe
     */
    virtual void set_bypass_mode(const bool bypass) = 0;

    /*! Get the bypass mode of the FFT
     *
     * Returns the current bypass mode.
     *
     * \returns Current FFT bypass mode
     */
    virtual bool get_bypass_mode() const = 0;

    /*! Get the number of items per clock cycle (NIPC)
     *
     * Returns the number of items per clock cycle (NIPC) that this block is
     * configured to process. Packet sizes and cyclic prefix lengths must a
     * multiple of this value.
     *
     * \returns NIPC
     */
    virtual uint32_t get_nipc() const = 0;

    /*! Get the maximum supported length of the FFT
     *
     * Returns the maximum supported length of the FFT.
     *
     * \returns Maximum supported FFT length
     */
    virtual uint32_t get_max_length() const = 0;

    /*! Get the maximum supported cyclic prefix length
     *
     * Returns the maximum supported cyclic prefix length.
     *
     * \returns Maximum supported cyclic prefix length
     */
    virtual uint32_t get_max_cp_length() const = 0;

    /*! Get the maximum supported number of values that can be written to the
     * cyclic prefix removal list
     *
     * Returns the maximum supported number of values that can be written to
     * the cyclic prefix removal list.
     *
     * \returns Maximum number of values for the cyclic prefix removal list
     */
    virtual uint32_t get_max_cp_removal_list_length() const = 0;

    /*! Get the maximum supported number of values that can be written to the
     * cyclic prefix insertion list
     *
     * Returns the maximum supported number of values that can be written to
     * the cyclic prefix insertion list.
     *
     * \returns Maximum number of values for the cyclic prefix insertion list
     */
    virtual uint32_t get_max_cp_insertion_list_length() const = 0;

    /*! Load values to the cyclic prefix insertion list.
     *
     * Loads values to the cyclic prefix insertion list. Each value represents
     * the length of a cyclic prefix that is prepended to the output of the
     * (typically inverse) FFT operation (typically the time domain signal). If
     * the length of the cyclic prefix insertion list is m, then the cyclic
     * prefix length that is added to the output signal of symbol n is:
     *
     * cp_length[n] = cp_length[n mod m]
     *
     * \param cp_lengths The cyclic prefix lengths to be written to the list
     */
    virtual void set_cp_insertion_list(const std::vector<uint32_t> cp_lengths) = 0;

    /*! Gets the values from the cyclic prefix insertion list.
     *
     * Gets values to the cyclic prefix insertion list. After initialization,
     * no CP insertion values are configured. Use the function
     * set_cp_insertion_list to set the values.
     */
    virtual std::vector<uint32_t> get_cp_insertion_list() const = 0;

    /*! Load values to the cyclic prefix removal list.
     *
     * Loads values to the cyclic prefix removal list. Each value represents
     * the length of a cyclic prefix that is removed from the input signal
     * (typically the time domain signal) before performing the (typically
     * forward) FFT operation. If the length of the cyclic prefix removal list
     * is m, then the cyclic prefix length that is removed from the input signal
     * of symbol n is:
     *
     * cp_length[n] = cp_length[n mod m]
     *
     * \param cp_lengths The cyclic prefix lengths to be written to the list
     */
    virtual void set_cp_removal_list(const std::vector<uint32_t> cp_lengths) = 0;

    /*! Gets the values from the cyclic prefix removal list.
     *
     * Gets values to the cyclic prefix removal list. After initialization,
     * no CP removal values are configured. Use the function
     * set_cp_removal_list to set the values.
     */
    virtual std::vector<uint32_t> get_cp_removal_list() const = 0;
};

}} // namespace uhd::rfnoc
