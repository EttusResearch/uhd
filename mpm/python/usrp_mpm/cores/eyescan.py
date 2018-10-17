#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
RX Eye Scan utility for GTX transceivers on 7-series FPGAs.

Introduction:

  A RX eye scan provides a mechanism to validate the receiver's eye margin after the
  transceiver's equalizer in high-speed digital transmission lines.
  The principle of operation of an eye scan is the comparison between the data sampled
  at the nominal center of the eye, and the data sampled at an offset (horizontal and
  vertical) from the said nominal center.
  A miscomparison between the nominal data and the offset data yields a bit error, and
  the Bit Error Rate (BER) is the ratio of bit errors to the total number of samples
  compared. A 2-D statistical eye can be obtained by determining the BER at multiple
  offset coordinates (horizontal, vertical) in a given 2-D range. Each offset coordinate
  has two components: phase offset (horizontal), and voltage offset (vertical).

  The 7-series FPGAs' transceivers include hardware that allows us to perfom parallel
  sampling on the same signal: one at the nominal center, and one with certain offset.
  The hardware consists on two separate samplers, one which signal can be offset.
  Also, a sample counter, that keeps track of the number of samples compared at a given
  offset; as well as an error counter that tracks the miscomparisons detected. These two
  counters are accessible via the transceiver's Dynamic Reconfiguration Port (DRP).
  Finally, a state machine, configurable through the DRP, controls the BER measurement
  at a given offset.
  The second sampler path can apply both a phase and a voltage offset to the signal.
  Thanks to this parallel sampler, one may perform eye scan measurements without
  affecting the link's integrity (i.e. the nominal sampled data remains untouched).


Important considerations:

  - This tool only supports 7-Series FPGAs (GTX only).
  - The tool needs the NI's JESD core IP and MPM driver to access the DRP of the GTs.
  - Currently, a .pes (Python Eye Scan) custom binary file is generated, which includes
    metadata and the sample/error counters results at each offset for the scanned GT(s).
    This file is further processed and visualized with an internal set of LabVIEW VIs.


Using the Eye Scan tool:

  1. Determine the transceiver(s) configuration.
     Some parameters are defined with the GT instantiation in the FPGA. These need to
     be known by the Eye Scan tool in order to function properly:
       Equalization mode.
         Low-Power Mode (LPM) or Decision Feedback Equalization (DFE).
         Please refer to UG476 (p. 184) for further details.
         Valid values for this tool: 'LMP' or 'DFE'.
       RXOUT_DIV.
         This attribute controls the setting for the RX serial clock divider.
         Please refer to UG476 (p. 213) for further details.
         Valid values for this tool: 1, 2, 4, 8, 16.
       RX_INT_DATAWIDTH.
         Width of valid data on Rdata and Sdata buses is RX fabric data width.
         Please refer to UG476 (p. 213) for further details.
         Valid values for this tool: 16, 20, 32, 40.

  2. Determine the Eye Scan measurement configuration.
     The eye scan measurement's confidence/resolution depends on some parameters,
     depending upon their values, the time to perform the scan will change:
       Prescale.
         Controls the prescaling of the sample count to keep both sample count and
         error count in reasonable precision within the 16-bit register range.
         The higher the prescale value, the more time the eye scan takes, but also
         the lower BER floor (i.e. best eye margin statistics).
         Please refer to UG476 (p. 214) for further details.
         Valid values for this tool: 0 to 31.
       Horizontal range.
         Defines the phase offset limits and step that the tool will use to iterate
         in the horizontal axis during the measurement.
         Valid range is -32 to 32 (full range), corresponding to -0.5 UI to 0.5 UI
         Definition example: {'start':-32 , 'stop':32 , 'step': 1}
       Vertical range.
         Defines the voltage offset limits and step that the tool will use to iterate
         in the vertical axis during the measurement.
         Valid range is -127 to 127 (full range), corresponding to 0.39 %
         increments.
         Definition example: {'start':-127, 'stop':127, 'step': 2}

  3. Determine which GT(s) will be scanned.
     The tool supports single scan and parallel multi-lane scan. The previous GT
     configuration (from 1), and the measurement configuration (from 2) applies
     for all the GTs scanned in a single run. Thus, if one requires a different scan
     per GT, a EyeScanTool object must be created each time with different parameters.
     The tool receives a 1-D array, which each element must be a valid GT number that
     the NI JESD core has access to.
     Examples: [0] or [0, 1] or [0, 1, 2, 3].

  4. Obtain a JESD core MPM object.
     The Eye Scan tool relies on the JESD core IP in the FPGA to talk to the GTs
     through their DRP, thus a MPM driver object for this core must be available
     upon creation of the EyeScanTool object.
     Please refer to nijesdcore.py for further documentation.

  5. Create an Eye Scan tool MPM object.
     Once the configuration is defined and a MPM JESD core object is available, one
     may proceed to create and initialize an EyeScanTool object.
     The genrated .pes file will be saved to the location given by the SAVE_DIR, which
     default value is set to a folder named "eyescan" in the home directory.
     One may change the save location by including the SAVE_DIR attribute at init.
     Assuming the EyeScanTool class has been imported to the calling file, here is
     an object creation example:
       args = {'rxout_div': 2, 'rx_int_datawidth': 20, 'eq_mode': 'LPM', 'prescale': 1,
               'SAVE_DIR': "/home/root/my_dir/"}
       eyescan_tool = EyeScanTool(jesdcore=jesdcore_object,
                                  slot_idx=0,
                                  **args)

  6. Perform the Eye Scan measurement!
     With the EyeScanTool object created and initialized, one may proceed to start
     the measurement by calling the function eyescan_full_scan(...).
     This function receives the array of GTs to scan, the horizontal range, and the
     vertical range. It returns the name of the PES file (automatically generated).

     Here is an example on how to start the scan:
       scan_lanes = [0, 1, 2, 3]
       hor_range  = {'start':-32 , 'stop':32 , 'step': 2}
       ver_range  = {'start':-127, 'stop':127, 'step': 2}
       pes_file_name = eyescan_tool.eyescan_full_scan(scan_lanes, hor_range, ver_range)

  7. Process and visualize the PES file.
     The resulting .pes binary file must be manually copied to a known location for
     LabVIEW access (i.e. a Windows machine running LV).
     Currently, two different file post-processing methods are supported:
       Single lane full scans.
         When only one lane is measured (e.g. scan_lanes = [0]), it is recommended to
         use the Single-Lane VI to process/visualize the results.
       Multiple lane full scans.
         When multiple lanes are measured (e.g. scan_lanes = [0, 1, 2, 3]), it is
         recommended to use the Multi-Lane VI to process/visualize the results.
     These VIs are for NI/Ettus internal use only. For further details, please contact
     Humberto Jimenez at humberto.jimenez@ni.com.


Theory of operation:

  As explained in the introductory section, the main two elements to perform a rx
  margin analysis are a sample counter and an error counter. These two are provided
  by the GT instantiation at the FPGA. This tools implements the algorithm to control
  the Eye Scan measurement state machine for the given GT(s) and retreive the counts.
  This task is repetead over and over again through the vertical and horizontal ranges
  specified by the user. The results for each offset coordinate are stored in a custom
  binary file (.pes) that is then processed (BER calculation) and visualized.

  When a EyeScanTool object is created (i.e. __init__ is called), the "fixed"
  configuration parameters for the GT(s) instantiation is defined. Also, the provided
  JESD core object is verified to make sure it contains the method implementations to
  access the Dynamic Reconfiguration Port (DRP) of the desired GT(s).
  With an EyeScanTool object created and initialized, the user only needs to call the
  eyescan_full_scan(...) method; which handles the measurement configuration, the binary
  file creation, the GT(s) configuration, and the measurement sweep across the ranges.


Future work ideas:

  1. Generate the eye scan results in human-readable fashion (i.e. ascii encoded
     instead of binary data).
  2. Add Bit Error Rate (BER) calculation for each offset within the tool, instead of
     just spitting samples and errors counters.
  3. Develop a open-source data visualization tool to enable non-LabVIEW users to
     process and visualize the eye scan results (pes file).
"""

import os
import time
import math
import datetime
from builtins import object
from usrp_mpm.mpmlog import get_logger

class EyeScanTool(object):
    """
    Provides a library to perform Eye Scan measurements using the NI JESD core.
    """

    MGT_TYPE  = "GTX"
    VER_MAJOR = "1"
    VER_MINOR = "0"
    SAVE_DIR  = "/home/root/eyescan/"

    # Set this value according to the status message printing rate desired. (Min=1)
    # E.g. PRINT_STATUS_EVERY = 1 will print a status message every offset measurement.
    PRINT_STATUS_EVERY = 10

    lanes = None
    # Array that defines the available lanes to measure.
    lane_num = None
    # Defines the currently global controled lane number.
    def set_global_lane(self, lane_num=None):
        """
        This method sets the global lane number variable being accessed, as well as
        configures the DRP to target the given lane number.
        """
        # Set the global variable and the DRP target with the given lane number.
        if lane_num is not None:
            self.log.trace("Setting lane %d as the global variable...", lane_num)
            # Set the global variable.
            self.lane_num = lane_num
            # Set the DRP target in the JESD core to the given lane number.
            self.jesdcore.set_drp_target('mgt', self.lane_num)
        else:
            self.log.trace("Unsetting the lane global variable...")
            # Unset the global variable.
            self.lane_num = None
            # Disable DRP target for the given lane number.
            self.jesdcore.disable_drp_target()


    def __init__(self, jesdcore, slot_idx=0, **kwargs):
        def validate_config():
            """
            This function validates the configuration parameters' ranges.
            """
            assert (0 <= self.prescale) and (self.prescale <= 31)
            assert self.rxout_div in (1, 2, 4, 8, 16)
            assert self.rx_int_datawidth in (16, 20, 32, 40)
            assert self.eq_mode.upper() in ('LPM', 'DFE')
            self.log.debug("Valid Eye Scan configuration: prescale=%d rxout_div=%d"
                           " rx_int_datawidth=%d eq_mode=%s",
                           self.prescale, self.rxout_div, self.rx_int_datawidth, self.eq_mode)
        #
        self.slot_idx = slot_idx
        self.log = get_logger("EyeScanTool-{}".format(self.slot_idx))
        self.log.info("Initializing Eye Scan Tool...")
        self.jesdcore = jesdcore
        assert hasattr(self.jesdcore, 'set_drp_target')
        assert hasattr(self.jesdcore, 'disable_drp_target')
        assert hasattr(self.jesdcore, 'drp_access')
        # Some global parameters defined.
        #
        # Control the prescaling of the sample count to keep both sample
        # count and error count in reasonable precision within the 16-bit
        # register range.
        # Valid values: from 0 to 31.
        self.prescale = 0
        #
        # QPLL/CPLL output clock divider D for the RX datapath.
        # Valid values: 1, 2, 4, 8, 16.
        self.rxout_div = 1
        #
        # Defines the width of valid data on Rdata and Sdata buses.
        # Valid values: 16, 20, 32, 40.
        self.rx_int_datawidth = 16
        #
        # Equalizer mode: LPM linear eq. or DFE eq.
        # When in DFE mode (RXLPMEN=0), due to the unrolled first DFE tap,
        # two separate eye scan measurements are needed, one at +UT and
        # one at -UT, to measure the TOTAL BER at a given vertical and
        # horizontal offset.
        # Valid values = 'LPM', 'DFE'.
        self.eq_mode = 'LPM'
        #
        # Overwrite the default configuration parameters with the ones given
        # by the user (host) through kwargs.
        for key, new_val in list(kwargs.items()):
            if hasattr(self, key) and (new_val != getattr(self, key)):
                self.log.trace("Overwriting {0}... default:{1} user:{2}"
                               .format(key, getattr(self, key), new_val))
                setattr(self, key, new_val)
        # Validate configuration attributes' values.
        validate_config()


    def parse_ranges(self,
                     hor_range={'start':-32 , 'stop':32 , 'step': 1},
                     ver_range={'start':-127, 'stop':127, 'step': 2}):
        """
        This function extracts parameters from the Eye Scan phase and voltage ranges
        used in measurement loops and height/width calculation.
        The function returns a list with the following keys:
          parsed_ranges = {'hor_start', 'hor_stop', 'hor_iterations', 'hor_step',
                           'ver_start', 'ver_stop', 'ver_iterations', 'ver_step'}

        Parameters:
          hor_range -> Horizontal phase offset range.
                       This parameter is given as a list of three keyed elements:
                         'start' -> Defines the first point of the range. [-32,32].
                         'stop'  -> Defines the last point of the range. [-32,32].
                         'step'  -> Defines the step at which the range is iterated. [1,2,4,8].
          ver_range -> Vertical volateg offset range.
                       This parameter is given as a list of three keyed elements:
                         'start' -> Defines the first point of the range. [-127,127].
                         'stop'  -> Defines the last point of the range. [-127,127].
                         'step'  -> Defines the step at which the range is iterated. [1,2,4,8].
        """
        self.log.trace("Parsing horizontal/vertical ranges...")
        parsed_ranges = {}
        # Do some input validation.
        assert ('start' in hor_range) and ('stop' in hor_range) and ('step' in hor_range)
        assert ('start' in ver_range) and ('stop' in ver_range) and ('step' in ver_range)
        assert hor_range['step'] in (1, 2, 4, 8)
        assert ver_range['step'] in (1, 2, 4, 8)
        # Parse the horizontal and vertical ranges, and build the output lists.
        parsed_ranges['hor_start'] = self.rxout_div * hor_range['start']
        parsed_ranges['hor_stop' ] = self.rxout_div * hor_range['stop' ]
        parsed_ranges['hor_step' ] = self.rxout_div * hor_range['step' ]
        parsed_ranges['hor_iterations'] = math.ceil( \
          (parsed_ranges['hor_stop'] - parsed_ranges['hor_start'] + 1) / \
            parsed_ranges['hor_step'])
        self.log.trace("hor_start=%d  hor_stop=%d  hor_step=%d  hor_iterations=%d",
                       parsed_ranges['hor_start'], parsed_ranges['hor_stop'],
                       parsed_ranges['hor_step' ], parsed_ranges['hor_iterations'])
        parsed_ranges['ver_start'] = ver_range['start']
        parsed_ranges['ver_stop' ] = ver_range['stop']
        parsed_ranges['ver_step' ] = ver_range['step']
        parsed_ranges['ver_iterations'] = math.ceil( \
          (ver_range['stop'] - ver_range['start'] + 1) / \
           ver_range['step'])
        self.log.trace("ver_start=%d  ver_stop=%d  ver_step=%d  ver_iterations=%d",
                       parsed_ranges['ver_start'], parsed_ranges['ver_stop'],
                       parsed_ranges['ver_step' ], parsed_ranges['ver_iterations'])
        # Return the list with the extracted parameters.
        return parsed_ranges


    def eyescan_config(self,
                       es_qualifier =[0x0000, 0x0000, 0x0000, 0x0000, 0x0000],
                       es_qual_mask =[0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF],
                       es_sdata_mask=[]):
        """
        This function configures the current GT number to enable Eye Scan.
        The following attributes are configured for the given transceiver lane:
          - ES_QUALIFIER
          - ES_QUAL_MASK
          - ES_SDATA_MASK
          - ES_PRESCALE

        Parameters:
          es_qualifier     -> This element must be a 5 elements (16-bit integers) 1D array,
                              containing the 80-bit ES_QUALIFIER value.
                              This element is optional, default values available.
                                es_qualifier[0] -> ES_QUALIFIER[15:0]
                                  ...
                                es_qualifier[4] -> ES_QUALIFIER[79:64]
          es_qual_mask     -> This element must be a 5 elements (16-bit integers) 1D array,
                              containing the 80-bit ES_QUAL_MASK value.
                              This element is optional, default values available.
                                es_qual_mask[0] -> ES_QUAL_MASK[15:0]
                                  ...
                                es_qual_mask[4] -> ES_QUAL_MASK[79:64]
          es_sdata_mask    -> This element must be a 5 elements (16-bit integers) 1D array,
                              containing the 80-bit ES_SDATA_MASK value.
                              If this mask is not given, then an internal default will be
                              assigned for the statistical eye application based on the
                              rx_int_datawidth global parameter value.
                                es_sdata_mask[0] -> ES_SDATA_MASK[15:0]
                                  ...
                                es_sdata_mask[4] -> ES_SDATA_MASK[79:64]
        """
        #
        #                           [15: 0] [31:16] [47:32] [63:48] [79:64]
        ES_SDATA_MASK_DICT = {16 : [0xFFFF, 0x00FF, 0xFF00, 0xFFFF, 0xFFFF],
                              20 : [0xFFFF, 0x000F, 0xFF00, 0xFFFF, 0xFFFF],
                              32 : [0x00FF, 0x0000, 0xFF00, 0xFFFF, 0xFFFF],
                              40 : [0x0000, 0x0000, 0xFF00, 0xFFFF, 0xFFFF]}
        #
        # Configure each lane in the global lanes array.
        for current_lane in self.lanes:
            self.set_global_lane(current_lane)
            self.log.trace("Configuring GT #%d...", self.lane_num)
            # Do some input validation.
            assert len(es_qualifier) == 5
            assert len(es_qual_mask) == 5
            if len(es_sdata_mask) != 5:
                self.log.trace("ES_SDATA_MASK defined based on rx_int_datawidth=%d",
                               self.rx_int_datawidth)
                es_sdata_mask = ES_SDATA_MASK_DICT[self.rx_int_datawidth]
            # Configure the ES_QUALIFIER attribute.
            self.log.trace("Configuring the ES_QUALIFIER attribute for GT #%d...", self.lane_num)
            ES_QUALIFIER_FIRST_ADDR = 0x02C
            ES_QUALIFIER_LAST_ADDR  = 0x030
            index = 0
            for drp_addr in range(ES_QUALIFIER_FIRST_ADDR, ES_QUALIFIER_LAST_ADDR + 0x1):
                self.jesdcore.drp_access(rd=False, addr=drp_addr,
                                         wr_data=es_qualifier[index]); index += 1
            # Configure the ES_QUAL_MASK attribute.
            # According to UG476 pg. 217, ES_QUAL_MASK for a statistical eye is 80 1's,
            # so the sample counter and error counter accumulate on every cycle.
            # Thus, we write this registers to the given GT number to configure the hardware.
            self.log.trace("Configuring the ES_QUAL_MASK attribute for GT #%d...", self.lane_num)
            ES_QUAL_MASK_FIRST_ADDR = 0x031
            ES_QUAL_MASK_LAST_ADDR  = 0x035
            index = 0
            for drp_addr in range(ES_QUAL_MASK_FIRST_ADDR, ES_QUAL_MASK_LAST_ADDR + 0x1):
                self.jesdcore.drp_access(rd=False, addr=drp_addr,
                                         wr_data=es_qual_mask[index]); index += 1
            # Configure the ES_SDATA_MASK attribute.
            self.log.trace("Configuring the ES_SDATA_MASK attribute for GT #%d...", self.lane_num)
            ES_SDATA_MASK_FIRST_ADDR = 0x036
            ES_SDATA_MASK_LAST_ADDR  = 0x03A
            index = 0
            for drp_addr in range(ES_SDATA_MASK_FIRST_ADDR, ES_SDATA_MASK_LAST_ADDR + 0x1):
                self.jesdcore.drp_access(rd=False, addr=drp_addr,
                                         wr_data=es_sdata_mask[index]); index += 1
            # Configure the ES_PRESCALE attribute.
            ES_PRESCALE_ADDR = 0x03B
            self.log.trace("Configuring the ES_PRESCALE attribute for GT #%d...", self.lane_num)
            drp_x03B_rb = self.jesdcore.drp_access(rd=True, addr=ES_PRESCALE_ADDR)
            es_prescale = self.prescale & 0x1F
            drp_x03B_wr = (drp_x03B_rb & ~0xF800) | (es_prescale << 11)
            self.jesdcore.drp_access(rd=False, addr=ES_PRESCALE_ADDR, wr_data=drp_x03B_wr)
            # According to UG476 pg. 220, for a GTX xcvr PMA_RSV2[5] should always be
            # asserted when using Eye Scan; otherwise, the Eye Scan circuitry in the PMA
            # will be powered down.
            PMA_RSV2_ADDR = 0x082
            if self.MGT_TYPE == 'GTX':
                self.log.trace("Asserting that PMA_RSV2[5] bit is high for GTX #%d...", self.lane_num)
                drp_x082_rb = self.jesdcore.drp_access(rd=True, addr=PMA_RSV2_ADDR)
                pma_rsv2_bit5 = (drp_x082_rb >> 5) & 0x1
                if not pma_rsv2_bit5:
                    self.log.error("PMA_RSV2[5] is not asserted for GT#{}, enable it before link bringup."
                                   .format(self.lane_num))
                    raise  RuntimeError('Eye Scan cicuitry is powered down, see log for details.')
            self.log.info("Configured GT #%d!", self.lane_num)
        self.set_global_lane(None)
        return


    def eyescan_control(self, err_det_en=True, run=False, arm=False):
        """
        Configures the eye scan control state machine for the current XCVR lane.
        This function returns True if there was an update in the register map.

        Parameters:
          err_det_en -> Enable error detection.
                        1 -> statistical eye | 0 -> scope and waveform views.
          run        -> Asserting this parameter causes a state transition from the WAIT
                        state to the RESET state, initiating a BER measurement sequence.
          arm        -> Asserting this parameter causes a state transition from the WAIT
                        state to the RESET state, initiating a diagnostic sequence.
                        In the ARMED state, deasserting this bit causes a state transition
                        to the READ state if one of the states of bits x03D[5:2] below is
                        not met.
        """
        ARM_TRIGGER_ON = {"error_detected"   : 0b0001,\
                          "qualifier_pattern": 0b0010,\
                          "es_trigger"       : 0b0100,\
                          "immediate"        : 0b1000}
        EYE_SCAN_EN_VAL = 0b1
        self.log.trace("Eyescan state machine control for MGT #%d", self.lane_num)
        # Read the current register values.
        drp_x03d_rb = self.jesdcore.drp_access(rd=True, addr=0x03D)
        # Determine the GT Channel attributes to be changed.
        es_errdet_en   = int(err_det_en)
        es_eye_scan_en = EYE_SCAN_EN_VAL
        es_control     = (int(run)                         << 0) | \
                         (int(arm)                         << 1) | \
                         (ARM_TRIGGER_ON["error_detected"] << 2)
        self.log.trace("Control attributes... ES_ERRDET_EN:0b{0:b}"
                       " ES_EYE_SCAN_EN:0b{1:b} ES_CONTROL:0b{2:06b}"
                       .format(es_errdet_en, es_eye_scan_en, es_control))
        # Build and write the new register values.
        drp_x03d_wr = ((drp_x03d_rb & ~0x023F) << 0) | \
                      (es_errdet_en            << 9) | \
                      (es_eye_scan_en          << 8) | \
                      (es_control              << 0)
        self.jesdcore.drp_access(rd=False, addr=0x03D, wr_data=drp_x03d_wr)
        return drp_x03d_rb != drp_x03d_wr # Return True when the register changed.


    def eyescan_offset(self, hor_offset=0, ver_offset=0, ut_sign='+UT'):
        """
        Configures the eyescan horizontal phase offset and vertical voltage offset
        for the current XCVR lane number.
        This function returns true if at least one register was updated.

        Parameters:
          hor_offset -> Horizontal phase offset.
                        [-32, 32] corresponding to -0.5 UI to +0.5 UI.
          ver_offset -> Vertical voltage offset.
                        [-127, 127] corresponding to 0.39% increments.
          ut_sign    -> UT tap sign: '+UT' or '-UT'.
        """
        UT_SIGN_BIT = {'+UT': 0b0, '-UT': 0b1}
        self.log.trace("Offset configuration for MGT #{}:".format(self.lane_num))
        # Do some input validation for the given parameters.
        assert ut_sign.upper() in ('+UT', '-UT')
        self.log.trace("GT #%d  Horizontal offset: %d  Vertical offset: %d  Tap: %s",
                       self.lane_num, hor_offset, ver_offset, ut_sign)
        # Read the current register values.
        drp_x03b_rb = self.jesdcore.drp_access(rd=True, addr=0x03B)
        drp_x03c_rb = self.jesdcore.drp_access(rd=True, addr=0x03C)
        # Determine the GT channel attributes to be changed.
        es_vert_offset = ((abs(ver_offset) & 0x007F) << 0) | \
                         ( int(ver_offset < 0)       << 7) | \
                         ( UT_SIGN_BIT[ut_sign]      << 8)
        es_horz_offset = (hor_offset & 0x0FFF)
        self.log.trace("Offset attributes... ES_HORZ_OFFSET:0b{0:012b} ES_VERT_OFFSET:0b{1:09b}"
                       .format(es_horz_offset, es_vert_offset))
        # Build and write new register values.
        drp_x03b_wr = (drp_x03b_rb & ~0x01FF) | (es_vert_offset & 0x01FF)
        drp_x03c_wr = (drp_x03c_rb & ~0x0FFF) | (es_horz_offset & 0x0FFF)
        self.jesdcore.drp_access(rd=False, addr=0x03B, wr_data=drp_x03b_wr)
        self.jesdcore.drp_access(rd=False, addr=0x03C, wr_data=drp_x03c_wr)
        # Return True when at least one of the two registers changed.
        return (drp_x03b_rb != drp_x03b_wr) or (drp_x03c_rb != drp_x03c_wr)


    def eyescan_wait(self, wait_for='END', exit_after=10000):
        """
        This function waits for the eye scan control FSM of the current lane
        number, to transition to the given state (wait_for).
        Returns True when the desired state is reached.

        Parameters:
          wait_for -> State which the function waits the FSM to transition to.
                      {'WAIT','RESET','COUNT','END','ARMED','READ'}
        """
        STATE_DECODE = {'WAIT': 0b000, 'RESET': 0b001, 'COUNT': 0b011, \
                        'END' : 0b010, 'ARMED': 0b101, 'READ' : 0b100}
        self.log.trace("Waiting for %s state at MGT #%d", wait_for, self.lane_num)
        # Validate the state input parameter.
        assert wait_for.upper() in ('WAIT', 'RESET', 'COUNT', 'END', 'ARMED', 'READ')
        # Poll the es_control_status GT attribute until the FSM transistions to
        # the given state.
        state_reached = False
        iterations = 0
        delay = 2 ** (self.prescale - 13) if (self.prescale > 13) else 0
        while not state_reached:
            # Read the status register.
            es_control_status = self.jesdcore.drp_access(rd=True, addr=0x151)
            done = es_control_status & 0x0001
            current_state = (es_control_status & 0x000E) >> 1
            self.log.trace("Current state: 0b{0:03b}  Status: %s"
                           .format(current_state), {0b0:'Not Done!', 0b1: 'Done!'}[done])
            # Compare current state with expected state.
            state_reached = (current_state == STATE_DECODE[wait_for])
            if (iterations >= 100) and (not state_reached) and (iterations % 100 == 0):
                self.log.debug("%s state has not been reached for GT #%d after %d iterations.",
                               wait_for, self.lane_num, iterations)
            time.sleep(delay / 1000.0)
            # Exit after so many iterations, prevneting the application to hang.
            iterations += 1
            if exit_after == iterations:
                break
        # Validate that the expected state was reached.
        if not state_reached:
            self.log.error("%s state was not reached at GT #%d after %d polls.",
                           wait_for, self.lane_num, iterations)
            raise Exception("Eyescan status timed out, see log for details.")
        self.log.trace("%s state reached at GT #%d", wait_for, self.lane_num)
        return state_reached


    def eyescan_counters(self):
        """
        This function reads the error and sample counters for the current lane number.
        Returns a tuple with the error and sample count.
        """
        self.log.trace("Reading counters for GT #%d ...", self.lane_num)
        counters = {'error_count': 0x0000, 'sample_count': 0x0000}
        # Read the error counter.
        counters['error_count' ] = self.jesdcore.drp_access(rd=True, addr=0x14F) & 0xFFFF
        counters['sample_count'] = self.jesdcore.drp_access(rd=True, addr=0x150) & 0xFFFF
        self.log.trace("es_error_count: 0x{:04X}   es_sample_count: 0x{:04X}"
                       .format(counters['error_count'], counters['sample_count']))
        return counters


    def eyescan_acquisition(self, hor_offset=0, ver_offset=0):
        """
        This function performs an acquisition for each lane in the lanes global array.
        Each GT is tested at the same "coordinate". Only after all GTs measurements
        are completed, the function returns.

        Parameters:
          hor_offset -> Horizontal phase offset.
                        [-32, 32] corresponding to -0.5 UI to +0.5 UI.
          ver_offset -> Vertical voltage offset.
                        [-127, 127] corresponding to 0.39% increments.
        """
        self.log.trace("Starting acquisition for GTs {}".format(self.lanes))
        acq_counters = [] # Array that stores multiple sl_counters lists.
        for _ in range(0, max(self.lanes) + 1):
            acq_counters.append({})
        #
        # First eye scan measurement (LPM | DFE)
        # Start the FSM on all the requested lanes.
        for current_lane in self.lanes:
            self.set_global_lane(current_lane)
            self.log.trace("Starting +UT acquisition for GT #%d", self.lane_num)
            # Clear run & arm bits in the Eyescan control.
            self.eyescan_control(err_det_en=True, run=False, arm=False)
            # Set offsets with +UT.
            self.eyescan_offset(hor_offset, ver_offset, ut_sign='+UT')
            # Start eyescan FSM: set run with ErrDet enabled.
            self.eyescan_control(err_det_en=True, run=True, arm=False)
        #
        # Wait for the FSM to complete on each lane, read counters, and
        # start second eye scan measurement (DFE eq. only).
        for current_lane in self.lanes:
            self.set_global_lane(current_lane)
            # Wait for END state.
            self.eyescan_wait(wait_for='END')
            # Clear run & arm bits in the Eyescan control.
            self.eyescan_control(err_det_en=True, run=False, arm=False)
            # Read counters with +UT.
            acq_counters[self.lane_num]['+UT'] = self.eyescan_counters()
            self.log.trace("Results +UT GT #%d... Errors=%d  Samples=%d.", self.lane_num,
                           acq_counters[self.lane_num]['+UT']['error_count'],
                           acq_counters[self.lane_num]['+UT']['sample_count'])
            # Start second eye scan measurement (DFE eq. only).
            if self.eq_mode == 'DFE':
                # Set offsets with -UT.
                self.eyescan_offset(hor_offset, ver_offset, ut_sign='-UT')
                # Start eyescan FSM: set run with ErrDet enabled.
                self.eyescan_control(err_det_en=True, run=True, arm=False)
            else:
                self.log.debug("Single measurement finalized for GT #%d (H=%d, V=%d, %s).",
                               self.lane_num, hor_offset, ver_offset, self.eq_mode)
        #
        # Wait for the FSM (-UT, DFE only) to complete on each lane, and read counters.
        if self.eq_mode == 'DFE':
            for current_lane in self.lanes:
                self.set_global_lane(current_lane)
                # Wait for END state.
                self.eyescan_wait(wait_for='END')
                # Clear run & arm bits in the Eyescan control.
                self.eyescan_control(err_det_en=True, run=False, arm=False)
                # Read counters with -UT.
                acq_counters[self.lane_num]['-UT'] = self.eyescan_counters()
                self.log.trace("Results -UT GT #%d... Errors=%d  Samples=%d.", self.lane_num,
                               acq_counters[self.lane_num]['-UT']['error_count'],
                               acq_counters[self.lane_num]['-UT']['sample_count'])
                self.log.debug("Single measurement finalized for GT #%d (H=%d, V=%d, %s).",
                               self.lane_num, hor_offset, ver_offset, self.eq_mode)
        #
        self.set_global_lane(None)
        # Return the error and sample counters for all lanes, both +UT and -UT.
        return acq_counters


    def eyescan_sweep(self, bin_file, parsed_ranges):
        """
        Performs Eye Scan "measurement loop" (error counting) acquisitions across the
        given phase and voltage offset ranges.

        This function creates a binary file containing the sweep results.
          The binary file is a set of bytes, where file[position] represents a byte
          (8-bit) at the given position. The bytes are saved as follows:

          If eq_mode = 'LPM'...
            file[offset + 4*lanes*i + 4*curr_lane + 0] = sample_count[15:0] (+UT) (ith acquisition)
            file[offset + 4*lanes*i + 4*curr_lane + 2] =  error_count[15:0] (+UT) (ith acquisition)

          If eq_mode = 'DFE'...
            file[offset + 4*lanes*(i*2 + 0) + 4*curr_lane + 0] = sample_count[15:0] (+UT) (ith acquisition)
            file[offset + 4*lanes*(i*2 + 0) + 4*curr_lane + 2] =  error_count[15:0] (+UT) (ith acquisition)
            file[offset + 4*lanes*(i*2 + 1) + 4*curr_lane + 0] = sample_count[15:0] (-UT) (ith acquisition)
            file[offset + 4*lanes*(i*2 + 1) + 4*curr_lane + 2] =  error_count[15:0] (-UT) (ith acquisition)

          Where,
            i         -> single acquisition iteration number, ranging from 0 to
                         (hor_iterations * ver_iterations - 1).
            offset    -> set offset for metadata to be stored at the beginning of
                         the binary file.
            lanes     -> total number of lanes to be scanned. Defined as len(self.lanes).
            curr_lane -> a given lane number. It should be any value in the lanes array.

        Parameters:
          bin_file      -> Binary file reference to write data to. Passed from top level function.
          parsed_ranges -> This is a keyed list with parsed parameters from parse_ranges().
        """
        def write_byte_counters(acq_counters):
            """
            This method writes the acquisition counters for each lane to the binary file.
            """
            # Write the sample and error counters for all given lanes.
            for current_lane in self.lanes:
                # Write the counters for the current lane.
                sl_counters = acq_counters[current_lane]
                self.log.debug("Writing +UT counters for GT #{0}: {1}"
                               .format(current_lane, sl_counters))
                byte_number = (sl_counters['+UT']['sample_count']).to_bytes(2, 'little')
                bin_file.write(byte_number)
                byte_number = (sl_counters['+UT']['error_count' ]).to_bytes(2, 'little')
                bin_file.write(byte_number)
                # -UT results only exists when DFE eq. mode is used.
                if '-UT' in sl_counters:
                    self.log.debug("Writing -UT counters for GT #%d...", current_lane)
                    byte_number = (sl_counters['-UT']['sample_count']).to_bytes(2, 'little')
                    bin_file.write(byte_number)
                    byte_number = (sl_counters['-UT']['error_count' ]).to_bytes(2, 'little')
                    bin_file.write(byte_number)
        #
        gts_string = "GTs {}".format(self.lanes)
        self.log.trace("Starting sweep for %s ...", gts_string)
        # Perform the Eye Scan sweep!
        acq_counters = []
        total_iterations = parsed_ranges['hor_iterations'] * parsed_ranges['ver_iterations']
        iterations = 0
        # Outer loop iterates horizontally.
        for hor_offset in range(parsed_ranges['hor_start'], parsed_ranges['hor_stop'] + 1,
                                parsed_ranges['hor_step']):
            # Inner loop iterates vertically.
            for ver_offset in range(parsed_ranges['ver_start'], parsed_ranges['ver_stop'] + 1,
                                    parsed_ranges['ver_step']):
                # Perform a single acquisition at each "coordinate".
                acq_counters = self.eyescan_acquisition(hor_offset, ver_offset)
                # Write the data to a binary file.
                write_byte_counters(acq_counters)
                # Report Eye Scan progress.
                iterations += 1
                progress = iterations / total_iterations * 100
                # Only print status messages every PRINT_STATUS_EVERY iterations.
                if iterations % self.PRINT_STATUS_EVERY == 0:
                    self.log.info("Eye Scan progress for %s sweep: %.2f %%", gts_string, progress)


    def create_pes_file(self, hor_range, ver_range):
        """
        This function creates a .pes file and writes the metadata header.
        The file object is returned.

          0x00 -> 0x0F : "PythonEyeScanXpY" [16 bytes] (X -> Major, Y -> Minor)
          --- Data offset ---
          0x10 -> 0x11 : data_offset        [ 2 bytes]
          --- Eye Scan run configuration ---
          0x12 -> 0x13 : prescale           [ 2 bytes]
          0x14 -> 0x15 : rxout_div          [ 2 bytes]
          0x16 -> 0x17 : rx_int_datawidth   [ 2 bytes]
          0x18 -> 0x19 : eq_mode            [ 2 bytes] (0 -> LPM / 1 -> DFE)
          --- Phase ranges ---
          0x1A -> 0x1B : hor_start          [ 2 bytes]
          0x1C -> 0x1D : hor_stop           [ 2 bytes]
          0x1E -> 0x1F : hor_step           [ 2 bytes]
          0x20 -> 0x21 : ver_start          [ 2 bytes]
          0x22 -> 0x23 : ver_stop           [ 2 bytes]
          0x24 -> 0x25 : ver_step           [ 2 bytes]
          --- Lane(s) information ---
          0x26 -> 0x27 : number of lanes    [ 2 bytes]
          0x28 -> 0x29 : lane_num array     [ 2 bytes] (Each nibble represents a lane)
        """
        def build_file_name():
            """
            This function builds the file name, which contains the scanned GTs and the
            prescale value used.
            """
            file_name = "eyescan"
            # Include a time stamp.
            now = datetime.datetime.now()
            file_name += ("_%04d%02d%02d%02d%02d" %
                          (now.year, now.month, now.day, now.hour, now.minute))
            # Include the scanned slot.
            file_name += ("_slot%d" % self.slot_idx)
            # Include the scanned GTs.
            file_name += "_gt"
            for lane in self.lanes:
                file_name += ("%d" % lane)
            # Include the prescale value.
            file_name += ("_pre%d" % self.prescale)
            # Include extension.
            file_name += ".pes"
            return file_name
        #
        def write_byte_number(number=0, size=2, offset=None):
            """
            This function writes a number as bytes in the binary file.
            When an offset is given, the data is written at that address, leaving the
            pointer there.
            """
            if offset:
                pes_file.seek(offset)
            byte_number = (number).to_bytes(size, 'little', signed=True)
            pes_file.write(byte_number)
        #
        # Create the directory to save the pes files if it does not exist.
        if not os.path.isdir(self.SAVE_DIR):
            self.log.trace("Creating directory: {}".format(self.SAVE_DIR))
            os.makedirs(self.SAVE_DIR)
        # Open the binary file which data will be saved to.
        file_name = build_file_name()
        pes_file  = open(self.SAVE_DIR + file_name, "wb+")
        # Write the metadata header.
        byte_string = ("PythonEyeScan"+self.VER_MAJOR+"p"+self.VER_MINOR).encode('utf-8')
        pes_file.write(byte_string)                                # 0x00: signature string.
        write_byte_number(number=0x0000)                           # 0x10: data_offset (placeholder).
        write_byte_number(number=self.prescale)                    # 0x12: prescale.
        write_byte_number(number=self.rxout_div)                   # 0x14: rxout_div.
        write_byte_number(number=self.rx_int_datawidth)            # 0x16: rx_int_datawidth.
        write_byte_number(number={'LPM':0, 'DFE':1}[self.eq_mode]) # 0x18: eq_mode.
        write_byte_number(number=hor_range['start'])               # 0x1A: hor_start.
        write_byte_number(number=hor_range['stop'])                # 0x1C: hor_stop.
        write_byte_number(number=hor_range['step'])                # 0x1E: hor_step.
        write_byte_number(number=ver_range['start'])               # 0x20: ver_start.
        write_byte_number(number=ver_range['stop'])                # 0x22: ver_stop.
        write_byte_number(number=ver_range['step'])                # 0x24: ver_step.
        write_byte_number(number=len(self.lanes))                  # 0x26: number of lanes.
        nibble = 0x0000
        for lane_index in range(0, len(self.lanes)):
            nibble |= (self.lanes[lane_index] & 0xF) << lane_index*4
        write_byte_number(number=nibble)                          # 0x28: lane_num array.
        # Write data offset and set pointer ready for data writing.
        data_offset = pes_file.tell()
        write_byte_number(number=data_offset, offset=0x10)
        pes_file.seek(data_offset)
        # Return the opened file.
        return (file_name, pes_file)


    def eyescan_full_scan(self,
                          scan_lanes=[0],
                          hor_range={'start':-32 , 'stop':32 , 'step': 1},
                          ver_range={'start':-127, 'stop':127, 'step': 2}):
        """
        This function performs all the GT configuration and starts the eye scan sweep.
        The binary file should be open here.

        Parameters:
          scan_lanes -> Array that represents which GTs will be scanned. The maximum size
                        of the array is 4, and the valid values for each item are 0,1,2,3.
          hor_range  -> Horizontal phase offset range.
                        This parameter is given as a list of three keyed elements:
                          'start' -> Defines the first point of the range. [-32,32].
                          'stop'  -> Defines the last point of the range. [-32,32].
                          'step'  -> Defines the step at which the range is iterated. [1,2,4,8].
          ver_range  -> Vertical volateg offset range.
                        This parameter is given as a list of three keyed elements:
                          'start' -> Defines the first point of the range. [-127,127].
                          'stop'  -> Defines the last point of the range. [-127,127].
                          'step'  -> Defines the step at which the range is iterated. [1,2,4,8].
        """
        # Set the global lanes variable that defines which lanes will be scanned.
        self.lanes = scan_lanes
        # Extract the needed parameters from the given ranges.
        parsed_ranges = self.parse_ranges(hor_range, ver_range)
        # Create the .pes binary file.
        file_name, pes_file = self.create_pes_file(hor_range, ver_range)
        # Configure the requested lanes.
        self.eyescan_config()
        # Perform the sweep on the requested lanes.
        self.eyescan_sweep(pes_file, parsed_ranges)
        # Close the binary file.
        pes_file.close()
        return file_name
