//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: reconfig_engine
//
// Description:
//
//   This file implements the registers and the state machine to interface with
//   Intel's IP for the Max 10 FPGA that allows in-field updates to the primary
//   FPGA image. This state machine has been designed to provide a level of
//   abstraction between the register interface provided to user and the
//   details of interfacing with Intel's On-Chip Flash IP block. The user
//   simply needs to instruct this state machine to enable/disable write
//   protection and perform read/write/erase operations accordingly to load and
//   verify a new primary FPGA image. Since the purpose of this file is to
//   allow modification to an FPGA image care has been taken to mitigate data
//   corruption.
//
//   The interface to Intel's On-Chip Flash IP block implemented in this file
//   is based on the information found in the Max 10 User Flash Memory User
//   Guide found at the link below.
//
//   https://www.intel.com/content/dam/www/programmable/us/en/pdfs/literature/hb/max-10/ug_m10_ufm.pdf
//
// Parameters:
//
//   BASE_ADDRESS  : Base address for CtrlPort registers.
//   NUM_ADDRESSES : Number of bytes of address space to use.
//   MEM_INIT      : Memory initialization enabled. Set to 0 if MAX10 internal
//                   configuration set to single compressed image. Set to 1 if
//                   MAX10 internal configuration set to single compressed
//                   image with memory initialization.
//

`default_nettype none


module reconfig_engine #(
  parameter BASE_ADDRESS  = 0,
  parameter NUM_ADDRESSES = 32,
  parameter MEM_INIT      = 0
) (
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  /// Request
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  // Response
  output reg         s_ctrlport_resp_ack,
  output reg  [ 1:0] s_ctrlport_resp_status,
  output reg  [31:0] s_ctrlport_resp_data,

  // Interface to On-Chip Flash IP
  output reg         csr_addr,
  output reg         csr_read,
  output reg  [31:0] csr_writedata,
  output reg         csr_write,
  input  wire [31:0] csr_readdata,
  output reg  [16:0] data_addr,
  output reg         data_read,
  output reg  [31:0] data_writedata,
  output reg         data_write,
  input  wire [31:0] data_readdata,
  input  wire        data_waitrequest,
  input  wire        data_readdatavalid
);

  `include "regmap/reconfig_regmap_utils.vh"
  `include "../../../lib/rfnoc/core/ctrlport.vh"

  // Check MAX10 variant target (10M04, 10M08 or XO3)
  `ifdef VARIANT_10M04
    localparam FLASH_PRIMARY_IMAGE_START_ADDR_MEM_INIT = FLASH_PRIMARY_IMAGE_START_ADDR_MEM_INIT_10M04;
    localparam FLASH_PRIMARY_IMAGE_START_ADDR          = FLASH_PRIMARY_IMAGE_START_ADDR_10M04;
    localparam FLASH_PRIMARY_IMAGE_END_ADDR            = FLASH_PRIMARY_IMAGE_END_ADDR_10M04;
    localparam CFM0_WP_OFFSET_MSB                      = 26; // From Max 10 Flash Memory User Guide.
    localparam CFM0_WP_OFFSET_LSB                      = 24; // From Max 10 Flash Memory User Guide.
  `elsif VARIANT_10M08
    localparam FLASH_PRIMARY_IMAGE_START_ADDR_MEM_INIT = FLASH_PRIMARY_IMAGE_START_ADDR_MEM_INIT_10M08;
    localparam FLASH_PRIMARY_IMAGE_START_ADDR          = FLASH_PRIMARY_IMAGE_START_ADDR_10M08;
    localparam FLASH_PRIMARY_IMAGE_END_ADDR            = FLASH_PRIMARY_IMAGE_END_ADDR_10M08;
    localparam CFM0_WP_OFFSET_MSB                      = 27; // From Max 10 Flash Memory User Guide.
    localparam CFM0_WP_OFFSET_LSB                      = 25; // From Max 10 Flash Memory User Guide.
  // The reconfiguration engine via flash is not supported in the XO3 variant.
  `elsif VARIANT_XO3
    localparam FLASH_PRIMARY_IMAGE_START_ADDR_MEM_INIT = 0;
    localparam FLASH_PRIMARY_IMAGE_START_ADDR          = 0;
    localparam FLASH_PRIMARY_IMAGE_END_ADDR            = 0;
    localparam CFM0_WP_OFFSET_MSB                      = 0;
    localparam CFM0_WP_OFFSET_LSB                      = 0;
  `else
    ERROR_MAX10_variant_must_be_defined();
    localparam FLASH_PRIMARY_IMAGE_START_ADDR_MEM_INIT = FLASH_PRIMARY_IMAGE_START_ADDR_MEM_INIT_10M04;
    localparam FLASH_PRIMARY_IMAGE_START_ADDR          = FLASH_PRIMARY_IMAGE_START_ADDR_10M04;
    localparam FLASH_PRIMARY_IMAGE_END_ADDR            = FLASH_PRIMARY_IMAGE_END_ADDR_10M04;
    localparam CFM0_WP_OFFSET_MSB                      = 26; // From Max 10 Flash Memory User Guide.
    localparam CFM0_WP_OFFSET_LSB                      = 24; // From Max 10 Flash Memory User Guide.
  `endif


  //----------------------------------------------------------
  //  Flash Interface between Registers and State Machine
  //----------------------------------------------------------

  // Flash Data Interface
  reg [16:0] flash_addr       = 0;
  reg [31:0] flash_write_data = 0;
  reg [31:0] flash_read_data;

  // Flash Control Interface - Control
  reg       flash_read_stb       = 1'b0;
  reg       flash_write_stb      = 1'b0;
  reg       flash_erase_stb      = 1'b0;
  reg       flash_enable_wp_stb  = 1'b0;
  reg       flash_disable_wp_stb = 1'b0;
  reg [2:0] flash_sector         = 3'b0;

  // Flash Control Interface - Status
  reg flash_wp_enabled;
  reg flash_read_idle;
  reg flash_write_idle;
  reg flash_erase_idle;
  reg flash_read_err;
  reg flash_write_err;
  reg flash_erase_err;
  reg clear_flash_read_err_stb  = 1'b0;
  reg clear_flash_write_err_stb = 1'b0;
  reg clear_flash_erase_err_stb = 1'b0;

  //----------------------------------------------------------
  // Address Calculation
  //----------------------------------------------------------

  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) &&
                          (s_ctrlport_req_addr < BASE_ADDRESS + NUM_ADDRESSES);

  //----------------------------------------------------------
  // Handling of ControlPort Requests
  //----------------------------------------------------------

  always @(posedge ctrlport_clk) begin
    // Default assignments
    s_ctrlport_resp_ack <= 1'b0;

    flash_read_stb            <= 1'b0;
    flash_write_stb           <= 1'b0;
    flash_erase_stb           <= 1'b0;
    flash_enable_wp_stb       <= 1'b0;
    flash_disable_wp_stb      <= 1'b0;
    clear_flash_read_err_stb  <= 1'b0;
    clear_flash_write_err_stb <= 1'b0;
    clear_flash_erase_err_stb <= 1'b0;

    // Do not acknowledge on reset
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack     <= 1'b0;
      s_ctrlport_resp_data    <= {32{1'bx}};
      s_ctrlport_resp_status  <= CTRL_STS_OKAY;

    // Write requests
    end else begin
      if (s_ctrlport_req_wr) begin
        // Always issue an ack and no data
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_data    <= {32{1'bx}};
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + FLASH_CONTROL_REG: begin
            flash_read_stb            <= s_ctrlport_req_data[FLASH_READ_STB];
            flash_write_stb           <= s_ctrlport_req_data[FLASH_WRITE_STB];
            flash_erase_stb           <= s_ctrlport_req_data[FLASH_ERASE_STB];
            flash_enable_wp_stb       <= s_ctrlport_req_data[FLASH_ENABLE_WP_STB];
            flash_disable_wp_stb      <= s_ctrlport_req_data[FLASH_DISABLE_WP_STB];
            clear_flash_read_err_stb  <= s_ctrlport_req_data[CLEAR_FLASH_READ_ERROR_STB];
            clear_flash_write_err_stb <= s_ctrlport_req_data[CLEAR_FLASH_WRITE_ERROR_STB];
            clear_flash_erase_err_stb <= s_ctrlport_req_data[CLEAR_FLASH_ERASE_ERROR_STB];
            flash_sector              <= s_ctrlport_req_data[FLASH_ERASE_SECTOR_MSB:FLASH_ERASE_SECTOR];
          end

          BASE_ADDRESS + FLASH_ADDR_REG: begin
            flash_addr <= s_ctrlport_req_data[FLASH_ADDR_MSB:FLASH_ADDR];
          end

          BASE_ADDRESS + FLASH_WRITE_DATA_REG: begin
            flash_write_data <= s_ctrlport_req_data[FLASH_WRITE_DATA_MSB:FLASH_WRITE_DATA];
          end

          // Error on undefined address
          default: begin
            if (address_in_range) begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;

            // No response if out of range
            end else begin
              s_ctrlport_resp_ack <= 1'b0;
            end
          end
        endcase

      // Read request
      end else if (s_ctrlport_req_rd) begin
        // Default assumption: valid request
        s_ctrlport_resp_ack <= 1'b1;
        s_ctrlport_resp_status <= CTRL_STS_OKAY;

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + FLASH_STATUS_REG: begin
            s_ctrlport_resp_data <= {CTRLPORT_DATA_W {1'b0}};
            s_ctrlport_resp_data[FLASH_WP_ENABLED]       <= flash_wp_enabled;
            s_ctrlport_resp_data[FLASH_READ_IDLE]        <= flash_read_idle;
            s_ctrlport_resp_data[FLASH_READ_ERR]         <= flash_read_err;
            s_ctrlport_resp_data[FLASH_ERASE_IDLE]       <= flash_erase_idle;
            s_ctrlport_resp_data[FLASH_ERASE_ERR]        <= flash_erase_err;
            s_ctrlport_resp_data[FLASH_WRITE_IDLE]       <= flash_write_idle;
            s_ctrlport_resp_data[FLASH_WRITE_ERR]        <= flash_write_err;
            s_ctrlport_resp_data[FLASH_MEM_INIT_ENABLED] <= MEM_INIT ? 1'b1 : 1'b0;
          end

          BASE_ADDRESS + FLASH_ADDR_REG: begin
            s_ctrlport_resp_data <= {CTRLPORT_DATA_W {1'b0}};
            s_ctrlport_resp_data[FLASH_ADDR_MSB:FLASH_ADDR] <= flash_addr;
          end

          BASE_ADDRESS + FLASH_READ_DATA_REG: begin
            s_ctrlport_resp_data <= flash_read_data;
          end

          BASE_ADDRESS + FLASH_CFM0_START_ADDR_REG: begin
            s_ctrlport_resp_data <= MEM_INIT ? FLASH_PRIMARY_IMAGE_START_ADDR_MEM_INIT : 
                                               FLASH_PRIMARY_IMAGE_START_ADDR;
          end

          BASE_ADDRESS + FLASH_CFM0_END_ADDR_REG: begin
            s_ctrlport_resp_data <= FLASH_PRIMARY_IMAGE_END_ADDR;
          end

          // Error on undefined address
          default: begin
            s_ctrlport_resp_data <= {32{1'bx}};
            if (address_in_range) begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;

            // No response if out of range
            end else begin
              s_ctrlport_resp_ack <= 1'b0;
            end
          end
        endcase
      end
    end
  end

  //----------------------------------------------------------
  // State Machine Constants
  //----------------------------------------------------------

  // Local state
  localparam IDLE                     = 4'h0;
  localparam WP_DISABLED              = 4'h1;
  localparam WAIT_FOR_READ_DATA_VALID = 4'h2;
  localparam GET_READ_STATUS          = 4'h3;
  localparam CHECK_READ_STATUS        = 4'h4;
  localparam WAIT_FOR_WRITE_COMPLETE  = 4'h5;
  localparam GET_WRITE_STATUS         = 4'h6;
  localparam CHECK_WRITE_STATUS       = 4'h7;
  localparam ERASE_SECTOR             = 4'h8;
  localparam GET_ERASE_BUSY           = 4'h9;
  localparam CHECK_ERASE_BUSY         = 4'hA;
  localparam GET_ERASE_IDLE           = 4'hB;
  localparam CHECK_ERASE_IDLE         = 4'hC;

  // The Intel on-chip flash control interface has two registers, a Status
  // Register at address 0 and a Control Register at address 1. The constants
  // defined below identify fields and values of interest in each register.
  // These are taken directly from the Max 10 Flash Memory User Guide.
  localparam STATUS_REG_ADDR              = 1'b0;
  localparam   STATUS_REG_BUSY_STATUS_MSB = 1;
  localparam   STATUS_REG_BUSY_STATUS_LSB = 0;
  localparam     STATUS_REG_IDLE          = 2'b00;
  localparam     STATUS_REG_ERASE_BUSY    = 2'b01;
  localparam     STATUS_REG_WRITE_BUSY    = 2'b10;
  localparam     STATUS_REG_READ_BUSY     = 2'b11;
  localparam   STATUS_REG_READ_STATUS     = 2;
  localparam   STATUS_REG_WRITE_STATUS    = 3;
  localparam   STATUS_REG_ERASE_STATUS    = 4;
  localparam     OPERATION_FAILED         = 0;

  localparam CONTROL_REG_ADDR        = 1'b1;
  localparam   SECTOR_ERASE_ADDR_MSB = 22;
  localparam   SECTOR_ERASE_ADDR_LSB = 20;
  // CFM0_WP_OFFSET_MSB and CFM0_WP_OFFSET_LSB are MAX10 variant dependent.
  localparam     ENABLE_WP           = MEM_INIT ? 3'b111 : 3'b100;
  localparam     DISABLE_WP          = 3'b000;

  //----------------------------------------------------------
  // State Machine
  //----------------------------------------------------------

  reg [3:0] state = IDLE;
  wire flash_no_errors_detected;

  assign flash_no_errors_detected = ~(flash_read_err | flash_write_err | flash_erase_err);

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      state <= IDLE;

      // Signals to config registers
      flash_wp_enabled <= 1'b1;
      flash_read_idle  <= 1'b1;
      flash_write_idle <= 1'b1;
      flash_erase_idle <= 1'b1;
      flash_read_err   <= 1'b0;
      flash_write_err  <= 1'b0;
      flash_erase_err  <= 1'b0;
      flash_read_data  <= 32'b0;

      // Signals to flash control interface
      csr_addr      <= 1'b0;
      csr_writedata <= {32 {1'b1}};
      csr_read      <= 1'b0;
      csr_write     <= 1'b0;

      // Signals to flash data interface
      data_addr      <= 17'b0;
      data_writedata <= 32'b0;
      data_read      <= 1'b0;
      data_write     <= 1'b0;
    end
    // Rising edge clock
    else begin
      // Default values
      csr_read      <= 1'b0;
      csr_write     <= 1'b0;
      csr_addr      <= STATUS_REG_ADDR;
      csr_writedata <= {32 {1'b1}};

      data_read  <= 1'b0;
      data_write <= 1'b0;

      // State handling
      case(state)

        // When in IDLE:
        //   * No operations are in progress and write protection is enabled.
        //   * Allowed transitions are to either read data from flash or
        //     disable write protection.
        //   * Transitions are only allowed if no error bits are asserted.
        //   * In the event both the *read_stb and *disable_wp_stb bits are
        //     asserted read operations take priority as these do not open the
        //     flash to modification.
        //   * Attempts to both enable and disable write protection
        //     simultaneously result in the state machine remaining in IDLE
        //     write protection enabled.
        IDLE: begin
          flash_wp_enabled <= 1'b1;

          if (flash_read_stb && flash_no_errors_detected) begin
            state           <= WAIT_FOR_READ_DATA_VALID;
            flash_read_idle <= 1'b0;
            data_read       <= 1'b1;
            data_addr       <= flash_addr;
          end else if (flash_disable_wp_stb && ~flash_enable_wp_stb && flash_no_errors_detected) begin
            state     <= WP_DISABLED;
            csr_write <= 1'b1;
            csr_addr  <= CONTROL_REG_ADDR;
            csr_writedata[CFM0_WP_OFFSET_MSB:CFM0_WP_OFFSET_LSB] <= DISABLE_WP;
          end
        end

        // Transition from WP_DISABLED when write protection is enabled or when
        // write/erase operations are initiated. A few things to note:
        //  * Enabling write protection takes priority, regardless of what
        //    other control bits may be asserted simultaneously, followed by
        //    writes, and lastly erases.
        //  * The user should not strobe both the *write_stb and *erase_stb
        //    bits simultaneously, but if they do the state machine returns to
        //    IDLE (thereby enabling write protection) and the *write_err and
        //    *erase_err bits are asserted.
        //  * Performing a write or erase operation is only allowed from
        //    WP_DISABLED. This allows some mitigation against data corruption
        //    as multiple steps are required to change the data in the flash.
        //    First write protection must be disabled, and only then can the
        //    flash be erased or written.
        WP_DISABLED: begin
          flash_wp_enabled <= 1'b0;

          if (flash_erase_stb && flash_write_stb) begin
            flash_erase_err <= 1'b1;
            flash_write_err <= 1'b1;
          end

          if (flash_enable_wp_stb || (flash_erase_stb && flash_write_stb)) begin
            state     <= IDLE;
            csr_write <= 1'b1;
            csr_addr  <= CONTROL_REG_ADDR;
            csr_writedata[CFM0_WP_OFFSET_MSB:CFM0_WP_OFFSET_LSB] <= ENABLE_WP;
          end else if (flash_write_stb) begin
            state            <= WAIT_FOR_WRITE_COMPLETE;
            flash_write_idle <= 1'b0;
            data_write       <= 1'b1;
            data_writedata   <= flash_write_data;
            data_addr        <= flash_addr;
          end else if (flash_erase_stb) begin
            state            <= ERASE_SECTOR;
            flash_erase_idle <= 1'b0;
            csr_write        <= 1'b1;
            csr_addr         <= CONTROL_REG_ADDR;
            csr_writedata[CFM0_WP_OFFSET_MSB:CFM0_WP_OFFSET_LSB] <= DISABLE_WP;
            csr_writedata[SECTOR_ERASE_ADDR_MSB:SECTOR_ERASE_ADDR_LSB] <= flash_sector;
          end
        end


        // Read Flash
        // --------------
        // Per Intel's Max 10 User Flash Memory User Guide, the Read bit of the
        // flash data interface should be pulsed for one clock cycle to start
        // the read process from flash. This pulse occurs upon transition from
        // IDLE to WAIT_FOR_READ_DATA_VALID. The state machine then waits in
        // WAIT_FOR_READ_DATA_VALID until the flash data interface
        // data_readdatavalid signal asserts, indicating the data is now valid.
        // Intel's documentation does not provide guidance on the expected time
        // for data_readdatavalid to assert. From simulation, however,
        // data_readdatavalid asserts four clock cycles after the Read pulse
        // ends. The data_readdatavalid signal from the flash data interface
        // pulses for one clock cycle. Only during this pulse is the data valid.
        WAIT_FOR_READ_DATA_VALID: begin
          if (data_readdatavalid) begin
            state           <= GET_READ_STATUS;
            flash_read_data <= data_readdata;
            csr_read        <= 1'b1;
          end
        end

        // The data_readdatavalid signal determines when the read operation has
        // completed in the flash data interface, but Intel's documentation
        // does not indicate the relation of this bit to the 'busy' field in
        // the flash control interface Status Register. To verify that the read
        // operation is complete, the StatusRegister is polled until the 'busy'
        // field indicates the flash is idle. This polling operation is
        // implemented with CHECK_READ_STATUS below. GET_READ_STATUS exists to
        // set the address of the flash control interface to the Status
        // Register and pulse the read bit of the flash control interface.
        // CHECK_READ_STATUS evaluates the resulting Status Register data and
        // steer the state machine accordingly. See Figure 6 in Intel's Max 10
        // User Flash Memory User Guide for a waveform on this request and
        // check mechanism. Successful read operations result in the state
        // machine returning to IDLE. Failing read operations assert the
        // flash_read_err bit before returning to IDLE. From simulation, the
        // 'busy' field returns to IDLE on the third read.
        GET_READ_STATUS: begin
          state <= CHECK_READ_STATUS;
          // csr_read set in transactions into this state
          // CSR address set as default assignment
        end
        CHECK_READ_STATUS: begin
          if (csr_readdata[STATUS_REG_BUSY_STATUS_MSB:STATUS_REG_BUSY_STATUS_LSB] == STATUS_REG_IDLE) begin
            state           <= IDLE;
            flash_read_idle <= 1'b1;
            flash_read_err  <= (csr_readdata[STATUS_REG_READ_STATUS] == OPERATION_FAILED) ? 1'b1 : 1'b0;
          end else if (csr_readdata[STATUS_REG_BUSY_STATUS_MSB:STATUS_REG_BUSY_STATUS_LSB] == STATUS_REG_READ_BUSY) begin
            state <= GET_READ_STATUS;
            csr_read <= 1'b1;
          end
        end


        // Write Flash
        //---------------
        // Per Intel's Max 10 User Flash Memory User Guide, the Write bit of
        // the flash data interface should be asserted while maintaining
        // address and data until the flash interface deasserts the
        // data_waitrequest bit. Transition from WP_DISABLED to
        // WAIT_FOR_WRITE_COMPLETE causes the write bit to assert and address
        // and data to be set. The state machine remains in this state until
        // the data_waitrequest bit deasserts. Per Intel's Max 10 User Flash
        // Memory User Guide, the data_waitrequest signal is expected to
        // deassert within 555 usec.
        WAIT_FOR_WRITE_COMPLETE: begin
          if (~data_waitrequest) begin
            state    <= GET_WRITE_STATUS;
            csr_read <= 1'b1;
          end else begin
            // Flash writes require asserting the Write bit of the flash data
            // interface until the write is complete.
            data_write <= 1'b1;
          end
        end

        // The data_waitrequest signal determines when the write operation has
        // completed in the flash data interface, but Intel's documentation does
        // not indicate the relation of this bit to the 'busy' field in the flash
        // control interface Status Register. To verify that the write operation
        // is complete the StatusRegister is polled until the 'busy' field
        // indicates the flash is idle. This polling operation is implemented with
        // GET_WRITE_STATUS and CHECK_WRITE_STATUS below, and follows the same
        // methodology as the polling operation for reads described above with the
        // following two changes:
        //   * Upon successful completion of a write operation the state
        //     machine returns to WP_DISABLED. This allows repeated writes of
        //     new data without having to disable/enable for each write.
        //   * When a failure is detected the state machine transitions to to
        //     IDLE thereby enabling write protection. Failure of a write is
        //     not expected. Write protection is enabled in the event of a
        //     failure to mitigate further data corruption.
        GET_WRITE_STATUS: begin
          state <= CHECK_WRITE_STATUS;
          // csr_read set in transactions into this state
          // CSR address set as default assignment
        end
        CHECK_WRITE_STATUS: begin
          if (csr_readdata[STATUS_REG_BUSY_STATUS_MSB:STATUS_REG_BUSY_STATUS_LSB] == STATUS_REG_IDLE) begin
            if (csr_readdata[STATUS_REG_WRITE_STATUS] == OPERATION_FAILED) begin
              state     <= IDLE;
              csr_write <= 1'b1;
              csr_addr  <= CONTROL_REG_ADDR;
              csr_writedata[CFM0_WP_OFFSET_MSB:CFM0_WP_OFFSET_LSB] <= ENABLE_WP;
            end else begin // SUCCESS
              state <= WP_DISABLED;
            end
            flash_write_idle <= 1'b1;
            flash_write_err <= (csr_readdata[STATUS_REG_WRITE_STATUS] == OPERATION_FAILED) ? 1'b1 : 1'b0;
          end else if (csr_readdata[STATUS_REG_BUSY_STATUS_MSB:STATUS_REG_BUSY_STATUS_LSB] == STATUS_REG_WRITE_BUSY) begin
            state <= GET_WRITE_STATUS;
            csr_read <= 1'b1;
          end
        end

        // Erase Flash
        //-------------
        // Erasing the primary configuration image requires a single write to
        // the flash control interface Control Register. Only one sector needs
        // to be erased to erase the entire primary configuration image.
        // Transition from WP_DISABLED to ERASE_SECTOR causes data to be
        // written to the Control Register to erase this sector and pulse the
        // flash control interface write bit.
        ERASE_SECTOR: begin
          state <= GET_ERASE_BUSY;
          csr_read <= 1'b1;
        end

        // There is some latency between writing the Control Register and the
        // 'busy' field of the flash control interface Status Register
        // indicating the erase operation is in progress. After initiating the
        // erase operation, GET_ERASE_BUSY and CHECK_ERASE_BUSY implement a
        // polling operation to determine when the erase operation has started.
        // GET_ERASE_BUSY exists to set the address of the flash control
        // interface to the Status Register and pulse the read bit of the flash
        // control interface. CHECK_ERASE_BUSY exists to evaluate the resulting
        // Status Register data and steer the state machine accordingly. The
        // polling operation continues until the 'busy' field indicates the
        // erase operation is in progress. Intel's documentation does not
        // indicate how long it takes for the Status Register to indicate the
        // erase is in progress, but simulation shows the erase is in progress
        // after the second read.
        GET_ERASE_BUSY: begin
          state <= CHECK_ERASE_BUSY;
          // csr_read set in transactions into this state
          // CSR address set as default assignment
        end
        CHECK_ERASE_BUSY: begin
          if (csr_readdata[STATUS_REG_BUSY_STATUS_MSB:STATUS_REG_BUSY_STATUS_LSB] == STATUS_REG_ERASE_BUSY) begin
            state <= GET_ERASE_IDLE;
            csr_read <= 1'b1;
          end else if (csr_readdata[STATUS_REG_BUSY_STATUS_MSB:STATUS_REG_BUSY_STATUS_LSB] == STATUS_REG_IDLE) begin
            state <= GET_ERASE_BUSY;
            csr_read <= 1'b1;
          end
        end

        // Once the erase operation is in progress a second polling operation
        // defined by GET_ERASE_IDLE and CHECK_ERASE_IDLE is implemented to
        // determine when the operation has completed. This polling operation
        // follows the same methodology as the polling operation for erase busy
        // described above. Intel's documentation indicates that erase
        // operations take a maximum of 350 msec.
        GET_ERASE_IDLE: begin
          state <= CHECK_ERASE_IDLE;
          // csr_read set in transactions into this state
          // CSR address set as default assignment
        end
        CHECK_ERASE_IDLE: begin
          if (csr_readdata[STATUS_REG_BUSY_STATUS_MSB:STATUS_REG_BUSY_STATUS_LSB] == STATUS_REG_IDLE) begin
            if (csr_readdata[STATUS_REG_ERASE_STATUS] == OPERATION_FAILED) begin
              state     <= IDLE;
              csr_write <= 1'b1;
              csr_addr  <= CONTROL_REG_ADDR;
              csr_writedata[CFM0_WP_OFFSET_MSB:CFM0_WP_OFFSET_LSB] <= ENABLE_WP;
            end else begin // SUCCESS
              state <= WP_DISABLED;
            end
            flash_erase_idle <= 1'b1;
            flash_erase_err  <= (csr_readdata[STATUS_REG_ERASE_STATUS] == OPERATION_FAILED) ? 1'b1 : 1'b0;
          end else if (csr_readdata[STATUS_REG_BUSY_STATUS_MSB:STATUS_REG_BUSY_STATUS_LSB] == STATUS_REG_ERASE_BUSY) begin
            state <= GET_ERASE_IDLE;
            csr_read <= 1'b1;
          end
        end

        // Default to IDLE in other cases
        default: begin
          state <= IDLE;
        end
      endcase

      // Reset errors
      if (clear_flash_read_err_stb) begin
        flash_read_err <= 1'b0;
      end
      if (clear_flash_write_err_stb) begin
        flash_write_err <= 1'b0;
      end
      if (clear_flash_erase_err_stb) begin
        flash_erase_err <= 1'b0;
      end
    end
  end

endmodule


`default_nettype wire


//XmlParse xml_on
//<regmap name="RECONFIG_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="RECONFIG_REGS">
//    <info>
//    These registers are used to upload and verify a new primary image to the
//    Max 10 FPGA on-chip flash when configured to support dual configuration
//    images. The steps below outline the process of verifying/preparing the
//    new image to be written, erasing the current image, writing the new
//    image, and verifying the new image was successfully written.
//      {p}{b}Prepare the data...{/b}
//            {ol}{li}{p}The Max 10 FPGA build should generate a *cfm0_auto.rpd
//                    file The *.rpd file is a "raw programming
//                    data" file holding all data related to the
//                    configuration image (CFM0). There are two
//                    important items to note regarding the addresses.
//                    First the *rpd data uses {b}byte{/b} addresses.
//                    Second, the start/end addresses defined by
//                    FLASH_PRIMARY_IMAGE_ADDR_ENUM are 32-bit word addresses{/p}{/li}
//                {li}{p}As a sanity check, verify the size of the raw
//                    programming data for CFM0 correspond to the address
//                    range of FLASH_PRIMARY_IMAGE_ADDR_ENUM. Do this by
//                    reading the values from FLASH_CFM0_START_ADDR_REG and
//                    FLASH_CFM0_END_ADDR, subtract both values, add one and
//                    multiply by four.
//                    {/p}{/li}
//                {li}{p}Having passed the sanity check the *.rpd data must
//                    now be manipulated into the form required by Altera's
//                    on-chip flash IP. Two operations must be performed.
//                    First the data must be converted from bytes to 32-bit
//                    words. Second the bit order must be reversed. This is
//                    illustrated in in the following table which shows byte
//                    address and data from the *.rpd file compared to the
//                    word address and data to be written to the on-chip
//                    flash.
//                    {table border=1}
//                      {tr}{td}.Map Addr{/td}{td}.Map Data{/td}{td}Flash Addr{/td}{td}Flash Data{/td}{/tr}
//                      {tr}{td}0x2B800{/td}{td}0x01{/td}{td rowspan=4}0xAC00{/td}{td rowspan=4}0x8040C020{/td}{/tr}
//                      {tr}{td}0x2B801{/td}{td}0x02{/td}{/tr}
//                      {tr}{td}0x2B802{/td}{td}0x03{/td}{/tr}
//                      {tr}{td}0x2B803{/td}{td}0x04{/td}{/tr}
//                      {tr}{td}0x2B804{/td}{td}0x05{/td}{td rowspan=4}0xAC01{/td}{td rowspan=4}0xA060E010{/td}{/tr}
//                      {tr}{td}0x2B805{/td}{td}0x06{/td}{/tr}
//                      {tr}{td}0x2B806{/td}{td}0x07{/td}{/tr}
//                      {tr}{td}0x2B807{/td}{td}0x08{/td}{/tr}
//                    {/table}
//                    {/p}{/li}
//                {li}{p}The resulting set of flash address data pairs should
//                    be used when writing FLASH_ADDR_REG and
//                    FLASH_WRITE_DATA_REG to update the CFM0 image.
//                    However, prior to writing the new image the old image
//                    must be erased.
//                    {/p}{/li}
//            {/ol}
//      {/p}
//      {p}{b}Erase the current primary flash image...{/b}
//            {ol}{p}{li}Read FLASH_STATUS_REG and verify no error bits are
//                    asserted and that all read, write, and erase operations
//                    are idle.{/p}{/li}
//                {p}{li}Disable write protection of the flash by strobing the
//                    FLASH_DISABLE_WP_STB bit of FLASH_CONTROL_REG.
//                    {/p}{/li}
//                {p}{li}Verify write protection is disabled and no errors are
//                    present by reading FLASH_STATUS_REG.{/p}{/li}
//                {p}{li}Initiate the erase operation by setting
//                    @.FLASH_ERASE_SECTOR and strobing FLASH_ERASE_STB of
//                    FLASH_CONTROL_REG.{/p}{/li}
//                {p}{li}Poll the FLASH_ERASE_IDLE bit of
//                    FLASH_STATUS_REG until it de-asserts indicating the
//                    erase operation is complete, then verify the operation
//                    was successful by checking that the FLASH_ERASE_ERR
//                    bit is de-asserted. Erase operations are expected to
//                    take a maximum of 350 msec. Upon completion of the erase
//                    operation write protection will remain disabled.
//                    {/p}{/li}
//                {p}{li}Erase additional sectors as required (see
//                    @.FLASH_ERASE_SECTOR for details) by restarting with first
//                    step.{/p}{/li}
//            {/ol}
//      {/p}
//      {p}{b}Write the new primary flash image...{/b}
//            {ol}{p}{li}Read FLASH_STATUS_REG and verify no error bits are
//                    asserted, all read, write, and erase operations are
//                    idle, and write protection is disabled.{/li}
//                {p}{li}Set the target address for the write to the Max 10
//                    on-chip flash by writing value from
//                    FLASH_CFM0_START_ADDR_REG to FLASH_ADDR_REG.{/li}{/p}
//                {p}{li}Set the data to be written to this address by writing
//                    the new 32-bit word of the new image to
//                    FLASH_WRITE_DATA_REG.{/li}{/p}
//                {p}{li}Initiate the write by strobing FLASH_WRITE_STB of
//                    FLASH_CONTROL_REG.{/li}{/p}
//                {p}{li}Poll the FLASH_WRITE_IDLE bit of
//                    FLASH_STATUS_REG until it de-asserts indicating the
//                    write operation is complete, then verify the operation
//                    was successful by checking that the FLASH_WRITE_ERR
//                    bit is de-asserted. Write operations are expected to
//                    take a maximum of 550 usec.{/li}{/p}
//                {p}{li}Upon completion of the write operation return to step
//                     2, incrementing the target address by one, and writing
//                     the next 32-bit word to Max10FlashWriteDatReg. If this
//                     was the last write, indicated by writing to
//                     FLASH_PRIMARY_IMAGE_END_ADDR, proceed to the next step
//                     to enable write protection.{/li}{/p}
//                {p}{li}After writing the new image enable write protection
//                     by strobing the FLASH_ENABLE_WP_STB bit of
//                     FLASH_CONTROL_REG.{/li}{/p}
//            {/ol}
//      {/p}
//      {p}{b}Verify the new primary flash image...{/b}
//            {ol}{p}{li}Read FLASH_STATUS_REG and verify no error bits are
//                    asserted and that all read, write, and erase operations
//                    are idle.{/li}{/p}
//                {p}{li}Set the target address for the read in the Max 10
//                    on-chip flash by writing value from
//                    FLASH_CFM0_START_ADDR_REG to FLASH_ADDR_REG.{/li}{/p}
//                {p}{li}Initiate the read by strobing FLASH_READ_STB of
//                    FLASH_CONTROL_REG.{/li}{/p}
//                {p}{li}Poll the FLASH_READ_IDLE bit of
//                    FLASH_STATUS_REG until it de-asserts indicating the
//                    read operation is complete, then verify the operation
//                    was successful by checking that the FLASH_READ_ERR
//                    bit is de-asserted. There is no guidance on exactly how
//                    long reads take to complete, but they are expected to be
//                    fairly quick. A very conservative timeout on this
//                    polling would be similar to that used for write
//                    operations.{/li}{/p}
//                 {p}{li}Upon completion of the read operation the resulting
//                     data returned by the on-chip flash will be available in
//                     Max10FlashReadDatReg. Read this register, compare to
//                     expected value previously written, and ensure they
//                     match.{/li}{/p}
//                 {p}{li}Return to step 2, incrementing the target
//                     address by one. If this was the last read verification
//                     is complete and no further action is required.{/li}{/p}
//            {/ol}
//      {/p}
//      {p}After the flash has been erased, programmed, and verified, a power
//         cycle is required for the new image to become active.
//      {/p}
//    </info>
//    <enumeratedtype name="FLASH_PRIMARY_IMAGE_ADDR_ENUM" showhexvalue="true">
//      <info>
//        These values are the start and end address of the CFM image flash
//        sector from Intel's On-Chip Flash IP Generator.
//        Be aware that three different values exist per each of the two
//        supported MAX10 variants: 10M04 and 10M08
//        Note that the values given in the IP generator are byte based where
//        the values of this enum are U32 based (divided by 4).
//      </info>
//      <value name="FLASH_PRIMARY_IMAGE_START_ADDR_MEM_INIT_10M04"
//        integer="4096"/>
//      <value name="FLASH_PRIMARY_IMAGE_START_ADDR_10M04"
//        integer="39936"/>
//      <value name="FLASH_PRIMARY_IMAGE_END_ADDR_10M04"
//        integer="75775"/>
//
//      <value name="FLASH_PRIMARY_IMAGE_START_ADDR_MEM_INIT_10M08"
//        integer="8192"/>
//      <value name="FLASH_PRIMARY_IMAGE_START_ADDR_10M08"
//        integer="44032"/>
//      <value name="FLASH_PRIMARY_IMAGE_END_ADDR_10M08"
//        integer="79871"/>
//    </enumeratedtype>
//    <register name="FLASH_STATUS_REG" offset="0x000" size="32"
//     attributes="Readable">
//      <bitfield name="FLASH_WP_ENABLED" range="0">
//        <info>
//        This bit is asserted when the flash is write protected and
//        de-asserted when write protection is disabled.
//        {li}Write protection must be enabled prior to performing read
//        operations.{/li}
//        {li}Write protection must be disabled prior to performing write and
//        erase operations.{/li}
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_READ_IDLE" range="4">
//        <info>
//        This bit is de-asserted when a read operation is in progress. Poll
//        this bit after strobing the FLASH_READ_STB bit of
//        FLASH_CONTROL_REG to determine when the read operation has
//        completed, then check the FLASH_READ_ERR bit to verify the
//        operation was successful.
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_READ_ERR" range="5">
//        <info>
//          This bit is asserted when a read operation fails. Clear this error
//          by strobing the CLEAR_FLASH_READ_ERROR_STB of this register. In the
//          event of a read error...
//          {li}the data in FLASH_READ_DATA_REG is invalid.{/li}
//          {li}attempts to disable write protection will be ignored.{/li}
//          {li}attempts to read/write/erase the flash will be ignored.{/li}
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_ERASE_IDLE" range="8">
//        <info>
//        This bit is de-asserted when an erase operation is in progress. Poll
//        this bit after strobing the FLASH_ERASE_STB bit of
//        FLASH_CONTROL_REG to determine when the erase operation has
//        completed, then check the FLASH_ERASE_ERR bit to verify the
//        operation was successful.
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_ERASE_ERR" range="9">
//        <info>
//          This bit is asserted when an erase operation fails. Clear this
//          error by strobing CLEAR_FLASH_ERASE_ERROR_STB of this register. In
//          the event of an erase error...
//          {li}{b}the primary configuration image may be corrupted,{/b} and
//          power cycling the board may result in unknown behavior.{/li}
//          {li}write protection of the flash will automatically be
//          re-enabled.{/li}
//         {li}attempts to disable write protection will be ignored.{/li}
//         {li}attempts to read/write/erase the flash will be ignored.{/li}
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_WRITE_IDLE" range="12">
//        <info>
//        This bit is de-asserted when a write operation is in progress. Poll
//        this bit after strobing the FLASH_WRITE_STB bit of
//        FLASH_CONTROL_REG to determine when the write operation has
//        completed, then check the FLASH_WRITE_ERR bit to verify the
//        operation was successful.
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_WRITE_ERR" range="13">
//        <info>
//          This bit is asserted when write operation fails. Clear this error
//          by strobing the CLEAR_FLASH_WRITE_ERROR_STB bit of this register. In
//          the event of a write error...
//          {li}{b}the primary configuration image may be corrupted,{/b} and
//          power cycling the board may result unknown behavior.{/li}
//          {li}write protection of the flash will automatically be
//          re-enabled.{/li}
//          {li}attempts to disable write protection will be ignored.{/li}
//          {li}attempts to read/write/erase the flash will be ignored.{/li}
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_MEM_INIT_ENABLED" range="16">
//        <info>
//        This bit is asserted when the flash can hold an image with memory
//        initialization.
//        </info>
//      </bitfield>
//    </register>
//    <register name="FLASH_CONTROL_REG" offset="0x04" size="32"
//       attributes="Writable">
//      <bitfield name="FLASH_ENABLE_WP_STB" range="0"
//       attributes="strobe">
//        <info>
//          Strobe this bit to enable write protection to the section of the
//          Max 10 on-chip flash storing the primary configuration image
//          (CFM0).
//          {li}Read the FLASH_WP_ENABLED bit of FLASH_STATUS_REG to
//          determine the current state of write protection.{/li}
//          {li}Prior to strobing this bit verify no write or erase operations
//          are in progress and no error bits are asserted by reading
//          FLASH_STATUS_REG.{/li}
//          {li}Attempts to enable write protection while erase or write
//          operations are in progress will be ignored.{/li}
//          {li}Write protection must be enabled prior to performing
//          read operations.{/li}
//          {li}Write protection should be enabled after completing
//          write or erase operations to prevent data corruption.{/li}
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_DISABLE_WP_STB" range="1"
//       attributes="strobe">
//        <info>
//          Strobe this bit to disable write protection to the section of the
//          Max 10 on-chip flash storing the primary configuration image
//          (CFM0).
//          {li}Read the FLASH_WP_ENABLED bit of FLASH_STATUS_REG to
//          determine the current state of write protection.{/li}
//          {li}Prior to strobing this bit verify no read operations are in
//          progress and no error bits are asserted by reading
//          FLASH_STATUS_REG.{/li}
//          {li}Attempts to disable write protection while a read is in
//          progress will be ignored.{/li}
//          {li}Attempts to disable write protection will be ignored if
//          this bit is strobed simultaneously with either FLASH_READ_STB
//          or FLASH_ENABLE_WP_STB.{/li}
//          {li}Write protection must be disabled prior to performing erase or
//          write operations.{/li}
//          {li}Upon completion of erase/write operations write protection
//          will remain disabled. When not actively erasing or writing a new
//          image write protection should be enabled to avoid data
//          corruption.{/li}
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_READ_STB" range="2"
//       attributes="strobe">
//        <info>
//          Strobe this bit to read data from the flash address identified in
//          FLASH_ADDR_REG.
//          {li}Prior to strobing this bit verify no read, write, or erase
//          operations are in progress, no error bits are asserted, and
//          write protection is enabled by reading FLASH_STATUS_REG.{/li}
//          {li}Attempts to read data while other operations are in progress
//          or while write protection is disabled will be ignored.{/li}
//          {li}After strobing this bit poll the FLASH_READ_IDLE and
//          FLASH_READ_ERR bits of FLASH_STATUS_REG to determine when
//          the read operation is complete and if it was successful.{/li}
//          {li}Upon successful completion the data read from flash will be
//          available in FLASH_READ_DATA_REG.{/li}
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_WRITE_STB" range="3"
//       attributes="strobe">
//        <info>
//          Strobe this bit to write the data contained in
//          FLASH_WRITE_DATA_REG to the flash address identified in
//          FLASH_ADDR_REG.
//          {li}The flash must be erased before writing new data.{/li}
//          {li}Prior to strobing this bit verify write protection is
//          disabled, no other write or erase operations are in progress, and
//          no error bits are asserted by reading FLASH_STATUS_REG.{/li}
//          {li}Attempts to write data while other write or erase operations
//          are in progress will be ignored.{/li}
//          {li}Attempts to write data with write protection enabled will be
//          ignored.{/li}
//          {li}Strobing this bit and FLASH_ERASE_STB simultaneously will
//          result in both the write and erase operation being ignored,
//          both corresponding error bits being set, and write protection
//          being re-enabled.{/li}
//          {li}After strobing this bit poll theMax10FlashWriteIdle and
//          FLASH_WRITE_ERR bits of FLASH_STATUS_REG to determine when
//          the write operation is complete and if it was successful.{/li}
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_ERASE_STB" range="4"
//       attributes="strobe">
//        <info>
//          Strobe this bit to erase the primary Max10 configuration image
//          (CFM0).
//          {li}Prior to strobing this bit verify no other write or erase
//          operations are in progress, write protection is disabled, and no
//          error bits are asserted by reading FLASH_STATUS_REG.{/li}
//          {li}Attempts to erase the primary image while other write or erase
//          operations are in progress will be ignored.
//          {li}Attempts to erase the primary image when write protection is
//          enabled will be ignored.{/li}
//          {li}Strobing this bit and FLASH_WRITE_STB simultaneously will
//          result both the erase and the write operation being ignored, both
//          corresponding error bits being set, and write protection being
//          re-enabled.{/li}
//          {li}After strobing this bit poll the FLASH_ERASE_IDLE and
//          FLASH_ERASE_ERR bits of FLASH_STATUS_REG to determine when
//          the erase operation is complete and if it was successful.{/li}
//        </info>
//      </bitfield>
//      <bitfield name="FLASH_ERASE_SECTOR" range="7..5"
//       attributes="strobe">
//        <info>
//          Defines the sector to be erased. Has to be set latest with the
//          write access which starts the erase operation by strobing
//          @.FLASH_ERASE_STB.{br}
//          With 10M04 variants, if the flash is configured to support memory
//          initialization (see @.FLASH_MEM_INIT_ENABLED flag) the sectors 2
//          to 4 have to be erased. If the flag is not asserted only sector 4
//          has to be erased.
//          With 10M08 variants, the sectors to be erased are 3 to 5 when
//          using memory initialization or only sector 5 otherwise.
//        </info>
//      </bitfield>
//      <bitfield name="CLEAR_FLASH_READ_ERROR_STB" range="8"
//       attributes="strobe">
//        <info>
//          Strobe this bit to clear a read error.
//        </info>
//      </bitfield>
//      <bitfield name="CLEAR_FLASH_WRITE_ERROR_STB" range="9"
//       attributes="strobe">
//        <info>
//          Strobe this bit to clear a write error.
//        </info>
//      </bitfield>
//      <bitfield name="CLEAR_FLASH_ERASE_ERROR_STB" range="10"
//       attributes="strobe">
//        <info>
//          Strobe this bit to clear an erase error.
//        </info>
//      </bitfield>
//    </register>
//    <register name="FLASH_ADDR_REG" offset="0x08" size="32"
//     attributes="Readable|Writable">
//      <bitfield name="FLASH_ADDR" range="16..0">
//        <info>
//          This field holds the target address for the next read or
//          write operation. Set this field prior to strobing the
//          FLASH_WRITE_STB and FLASH_READ_STB bits of
//          FLASH_CONTROL_REG. Valid addresses are defined by the
//          FLASH_PRIMARY_IMAGE_ADDR_ENUM enumeration.
//        </info>
//      </bitfield>
//    </register>
//    <register name="FLASH_WRITE_DATA_REG" offset="0x0C" size="32"
//    attributes="Writable">
//      <bitfield name="FLASH_WRITE_DATA" range="31..0">
//        <info>
//          Data in this register will be written to the flash at the address
//          identified in FLASH_ADDR_REG when a successful write operation
//          is executed.
//        </info>
//      </bitfield>
//    </register>
//    <register name="FLASH_READ_DATA_REG" offset="0x10" size="32"
//    attributes="Readable">
//      <bitfield name="FLASH_READ_DATA" range="31..0">
//        <info>
//          This register contains data read from the flash address identified
//          in FLASH_ADDR_REG after a successful read operation is executed.
//        </info>
//      </bitfield>
//    </register>
//    <register name="FLASH_CFM0_START_ADDR_REG" offset="0x14" size="32"
//    attributes="Readable">
//      <bitfield name="FLASH_CFM0_START_ADDR" range="31..0">
//        <info>
//          Start address of CFM0 image within flash memory (as defined in FLASH_PRIMARY_IMAGE_ADDR_ENUM).
//        </info>
//      </bitfield>
//    </register>
//    <register name="FLASH_CFM0_END_ADDR_REG" offset="0x18" size="32"
//    attributes="Readable">
//      <bitfield name="FLASH_CFM0_END_ADDR" range="31..0">
//        <info>
//          Last address of CFM0 image within flash memory (as defined in FLASH_PRIMARY_IMAGE_ADDR_ENUM).
//        </info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off
