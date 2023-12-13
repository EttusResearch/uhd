//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_gpio_spi
//
// Description:
//
//   This block enables control of a SPI master engine via CtrlPort
//   transactions.
//   It also enables customization on how signals from the SPI buses
//   connected to the master are mapped to the GPIO Ports.
//   This block supports configuring communication to up to 4 SPI slaves.
//
// Parameters:
//
//   NUM_SLAVES     : Number of SPI slaves to be supported. Values from 1 to 4
//                    are supported. SPI transfers can only target one slave
//                    at a time.
//   BASE_ADDRESS   : Start address for this register block.
//   SIZE_ADDRESS   : Size of the CtrlPort window to consider in this block.
//

`default_nettype none


module x4xx_gpio_spi #(
  parameter         NUM_SLAVES  = 4,
  parameter [19:0]  BASE_ADDRESS = 0,
  parameter [19:0]  SIZE_ADDRESS = 19'h20
) (
  input wire ctrlport_clk,
  input wire ctrlport_clk_2x,
  input wire ctrlport_rst,

  // Request
  input wire        s_ctrlport_req_wr,
  input wire        s_ctrlport_req_rd,
  input wire [19:0] s_ctrlport_req_addr,
  input wire [31:0] s_ctrlport_req_data,

  // Response
  output reg        s_ctrlport_resp_ack     = 1'b0,
  output reg [ 1:0] s_ctrlport_resp_status  = 2'b0,
  output reg [31:0] s_ctrlport_resp_data    = 32'b0,

  // GPIO control/status
  output wire  [31:0] gpio_out,
  output wire  [31:0] gpio_ddr,
  input  wire  [31:0] gpio_in
);

  `include "../../lib/rfnoc/core/ctrlport.vh"
  `include "regmap/dig_ifc_regmap_utils.vh"

  // Registers / wires for SPI core communication
  reg  [31:0] set_data = 0;
  reg  [ 7:0] set_addr = 0;
  reg         set_stb  = 1'b0;

  wire [31:0] readback;
  wire        readback_stb;
  wire        readback_stb_extended;

  wire       sclk;
  wire       mosi;
  wire       miso;
  // This array is set to the maximum supported SPI slaves instead of
  // the provided NUM_SLAVES to facilitate concurrent re-mapping.
  // See section(GPIO Mapping) of this file.
  wire [3:0] ss;

  // Auxiliary signals to compute which GPIO lines are outputs
  reg  [31:0] gpio_is_mosi = 32'b0;
  reg  [31:0] gpio_is_sclk = 32'b0;
  reg  [31:0] gpio_is_cs   = 32'b0;

  // SPI-to-GPIO mapping signals.
  // These arrays are set to the maximum supported SPI slaves instead of
  // the provided NUM_SLAVES to facilitate concurrent re-mapping.
  // See section(GPIO Mapping) of this file.
  reg [ SLAVE_CLK_SIZE-1:0] sclk_mapping [3:0];
  reg [SLAVE_MOSI_SIZE-1:0] mosi_mapping [3:0];
  reg [SLAVE_MISO_SIZE-1:0] miso_mapping [3:0];
  reg [  SLAVE_CS_SIZE-1:0] ss_mapping   [3:0];

  //---------------------------------------------------------------------------
  // Address calculation
  //---------------------------------------------------------------------------

  wire address_in_range =
    (s_ctrlport_req_addr >= BASE_ADDRESS) &&
    (s_ctrlport_req_addr < BASE_ADDRESS + SIZE_ADDRESS);

  // Check that address is targeting slave configuration.
  wire address_is_slave =
    (s_ctrlport_req_addr >= BASE_ADDRESS + SPI_SLAVE_CONFIG(0)) &&
    (s_ctrlport_req_addr <= BASE_ADDRESS + SPI_SLAVE_CONFIG(3));

  // Decode the slave being addressed.
  wire [1:0]slave_address = s_ctrlport_req_addr[3:2];

  //---------------------------------------------------------------------------
  // Slave configuration signals
  //---------------------------------------------------------------------------

  // These settings are registered individually for each slave
  reg [     NUM_SLAVES-1:0] data_in_edge     = {NUM_SLAVES{1'b0}};
  reg [     NUM_SLAVES-1:0] data_out_edge    = {NUM_SLAVES{1'b0}};
  reg [SPI_LENGTH_SIZE-1:0] slave_spi_length   [NUM_SLAVES-1:0];

  // One-hot encoding to indicate active slave
  reg [NUM_SLAVES-1:0] slave_select     = {NUM_SLAVES{1'b0}};

  //---------------------------------------------------------------------------
  // FSM to handle transfers
  //---------------------------------------------------------------------------

  localparam IDLE            = 3'd0;
  localparam SET_DIVIDER     = 3'd1;
  localparam WRITE_SPI       = 3'd2;
  localparam CONFIG_TRANSFER = 3'd3;
  localparam WAIT_SPI        = 3'd4;

  localparam DIVIDER_ADDRESS    = 8'd0;
  localparam CTRL_ADDRESS       = 8'd1;
  localparam DATA_ADDRESS       = 8'd2;

  reg [                 2:0] state        = IDLE;
  reg [                31:0] data_cache;
  reg [SPI_CLK_DIV_SIZE-1:0] divider;
  reg [                 1:0] cs;
  reg                        spi_go       = 1'b0;
  reg                        spi_ready    = 1'b0;

  integer slave_i;

  //---------------------------------------------------------------------------
  // CtrlPort Register endpoints
  //---------------------------------------------------------------------------

  always @ (posedge ctrlport_clk) begin

    if (ctrlport_rst) begin
      s_ctrlport_resp_ack <= 1'b0;
      spi_go              <= 1'b0;
      spi_ready           <= 1'b0;
      divider             <= {SPI_CLK_DIV_SIZE{1'b0}};
      cs                  <= 2'b0;

      // Assigned to unassigned mapping. This avoids overwriting
      // signals with those from uninitialized slaves.
      for ( slave_i = 0; slave_i < 4; slave_i = slave_i + 1 ) begin
        sclk_mapping[slave_i] <= 5'h31;
        mosi_mapping[slave_i] <= 5'h31;
        miso_mapping[slave_i] <= 5'h31;
        ss_mapping  [slave_i] <= 5'h31;
      end

      for ( slave_i = 0; slave_i < NUM_SLAVES; slave_i = slave_i + 1) begin
        slave_spi_length[slave_i] <= {SPI_LENGTH_SIZE{1'b0}};
      end
    end else begin

      // Default assignments
      s_ctrlport_resp_ack <= 1'b0;
      spi_go              <= 1'b0;

      // Requests appear
      if (s_ctrlport_req_wr) begin
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_data    <= {32{1'bx}};
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;

        // Address spi configuration writes
        if(address_is_slave) begin
          // GPIO mapping
          sclk_mapping     [slave_address] <= s_ctrlport_req_data[SLAVE_CLK_MSB:SLAVE_CLK];
          mosi_mapping     [slave_address] <= s_ctrlport_req_data[SLAVE_MOSI_MSB:SLAVE_MOSI];
          miso_mapping     [slave_address] <= s_ctrlport_req_data[SLAVE_MISO_MSB:SLAVE_MISO];
          ss_mapping       [slave_address] <= s_ctrlport_req_data[SLAVE_CS_MSB:SLAVE_CS];
          // Transfer Configuration
          slave_spi_length [slave_address] <= s_ctrlport_req_data[SPI_LENGTH_MSB:SPI_LENGTH];
          data_in_edge     [slave_address] <= s_ctrlport_req_data[MISO_EDGE];
          data_out_edge    [slave_address] <= s_ctrlport_req_data[MOSI_EDGE];
        end else begin

          case (s_ctrlport_req_addr)
            BASE_ADDRESS + SPI_TRANSACTION_CONFIG: begin
              divider <= s_ctrlport_req_data[SPI_CLK_DIV_MSB:SPI_CLK_DIV];
              cs      <= s_ctrlport_req_data[SPI_SLAVE_SELECT_MSB:SPI_SLAVE_SELECT];
            end

            BASE_ADDRESS + SPI_TRANSACTION_GO: begin
              spi_ready  <= 1'b0;
              spi_go     <= 1'b1;
            end

            // No register implementation for provided address
            default: begin
              // Acknowledge and provide error status if address is in range
              if (address_in_range) begin
                s_ctrlport_resp_status <= CTRL_STS_CMDERR;

              // No response if out of range
              end else begin
                s_ctrlport_resp_ack <= 1'b0;
              end
            end

          endcase
        end

      end else if(s_ctrlport_req_rd) begin
        // default assumption: valid request
        s_ctrlport_resp_ack     <= 1'b1;
        s_ctrlport_resp_status  <= CTRL_STS_OKAY;
        s_ctrlport_resp_data    <= {32{1'b0}};

        case (s_ctrlport_req_addr)
          BASE_ADDRESS + SPI_TRANSACTION_CONFIG: begin
            s_ctrlport_resp_data[SPI_CLK_DIV_MSB:SPI_CLK_DIV] <= divider;
          end

          BASE_ADDRESS + SPI_STATUS: begin
            s_ctrlport_resp_data[SPI_READY]                     <= spi_ready;
            s_ctrlport_resp_data[SPI_RESPONSE_MSB:SPI_RESPONSE] <= readback[SPI_RESPONSE_MSB:SPI_RESPONSE];
          end

          BASE_ADDRESS + CONTROLLER_INFO: begin
            s_ctrlport_resp_data[SLAVE_COUNT_MSB:SLAVE_COUNT] <= NUM_SLAVES;
          end

          // No register implementation for provided address
          default: begin
            // Acknowledge and provide error status if address is in range
            if (address_in_range) begin
              s_ctrlport_resp_status <= CTRL_STS_CMDERR;

            // No response if out of range
            end else begin
              s_ctrlport_resp_ack <= 1'b0;
            end
          end

        endcase
      end

      if (readback_stb_extended) begin
        spi_ready <= 1'b1;
      end
    end

  end

  //---------------------------------------------------------------------------
  // SPI Control FSM
  //---------------------------------------------------------------------------

  always @ (posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      state         <= IDLE;
      set_stb       <= 1'b0;
      data_cache    <= 32'h0;
    end else begin

      // Default Assignments
      set_stb             <= 1'b0;

      case (state)
        IDLE: begin
          // Save data and address for further steps
          data_cache    <= s_ctrlport_req_data;
          if (spi_go) begin
            state            <= CONFIG_TRANSFER;

            slave_select     <= {NUM_SLAVES{1'b0}};
            slave_select[cs] <= 1'b1;
          end
        end

        // Set slave select
        CONFIG_TRANSFER: begin
          state    <= SET_DIVIDER;

          set_stb  <= 1'b1;
          set_addr <= CTRL_ADDRESS;
          set_data <= { data_out_edge[cs],      // 1 bit
                        data_in_edge[cs],       // 1 bit
                        slave_spi_length[cs],   // 6 bits
                        {24-NUM_SLAVES{1'b0}},  // Padding for slaves
                        slave_select            // NUM_SLAVES bits
                      };
        end

        // Write divider to SPI core
        SET_DIVIDER: begin
          state    <= WRITE_SPI;

          set_stb  <= 1'b1;
          set_addr <= DIVIDER_ADDRESS;
          set_data <= {16'b0, divider};
        end

        // Write data bits to SPI core (aligned to MSB)
        WRITE_SPI: begin
          state    <= WAIT_SPI;

          set_stb  <= 1'b1;
          set_addr <= DATA_ADDRESS;
          set_data <= data_cache;
        end

        // Wait for transaction to complete and translate to ctrlport response
        WAIT_SPI: begin

          if (readback_stb_extended) begin
            state     <= IDLE;
          end
        end

        default: begin
          state <= IDLE;
        end
      endcase

      // Update Auxiliary signals
      gpio_is_mosi  <= 32'h0;
      gpio_is_sclk  <= 32'h0;
      gpio_is_cs    <= 32'h0;

      for ( slave_i = 0; slave_i < NUM_SLAVES; slave_i = slave_i + 1) begin
        gpio_is_mosi  [mosi_mapping[slave_i]] <= 1'b1;
        gpio_is_sclk  [sclk_mapping[slave_i]] <= 1'b1;
        gpio_is_cs    [  ss_mapping[slave_i]] <= 1'b1;
      end

    end
  end

  //---------------------------------------------------------------------------
  // SPI master
  //---------------------------------------------------------------------------

  `ifdef X440

    simple_spi_core #(
      .BASE     (0),
      .WIDTH    (NUM_SLAVES),
      .CLK_IDLE (0),
      .SEN_IDLE ({NUM_SLAVES{1'b1}})
    ) simple_spi_core_i (
      .clock        (ctrlport_clk),
      .reset        (ctrlport_rst),
      .set_stb      (set_stb),
      .set_addr     (set_addr),
      .set_data     (set_data),
      .readback     (readback),
      .readback_stb (readback_stb),
      .ready        (),
      .sen          (ss[NUM_SLAVES-1:0]),
      .sclk         (sclk),
      .mosi         (mosi),
      .miso         (miso),
      .debug        ()
    );

    assign readback_stb_extended = readback_stb;

  `else // X410

  // We only trigger one cycle of set_stb_2x per state change. This way the latency
  // is deterministic from the first change and aligned to the correct address,
  // without the need to pipeline the signal.
  //               ┐     ┌─────┐     ┌─────┐     ┌─────┐     ┌─────┐     ┌─────┐
  //clk          : └─────┘     └─────┘     └─────┘     └─────┘     └─────┘     └───
  //               xxxxxx/          \/          \/          \xxxxxxxxxxxxxxxxxxxxxx
  //state        : xxxxxx\   ctrl   /\   div    /\   data   /xxxxxxxxxxxxxxxxxxxxxx
  //               xxxxxxxxxxxxxxxxxx/          \/          \/          \xxxxxxxxxx
  //state_dlyd   : xxxxxxxxxxxxxxxxxx\   ctrl   /\   div    /\   data   /xxxxxxxxxx
  //               ┐                 ┌───────────────────────────────────┐
  //set_stb      : └─────────────────┘                                   └─────────
  //               xxxxxxxxxxxxxxxxxx/          \/          \/          \xxxxxxxxxx
  //set_addr     : xxxxxxxxxxxxxxxxxx\   0x1    /\   0x0    /\   0x2    /xxxxxxxxxx
  //                ──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐
  //clk_2x       :    └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └
  //               xxxxxxxxxxxxxxxxxxxxxxxx/          \/          \/          \xxxx
  //state_dlyd_2x: xxxxxxxxxxxxxxxxxxxxxxxx\   ctrl   /\   div    /\   data   /xxxx
  //                                       ┌─────┐     ┌─────┐     ┌─────┐
  //set_stb_2x   : ────────────────────────┘     └─────┘     └─────┘     └─────────
  //                                                                     ┌─────┐
  //trigger_spi  : ──────────────────────────────────────────────────────┘     └───
  //

    reg set_stb_2x = 1'b0;
    reg [2:0] state_dlyd, state_dlyd_2x = IDLE;

    always @ (posedge ctrlport_clk) begin
      if (ctrlport_rst) begin
        state_dlyd <= IDLE;
      end else begin
        state_dlyd <= state;
      end
    end

    always @ (posedge ctrlport_clk_2x) begin
      if (ctrlport_rst) begin
        state_dlyd_2x <= IDLE;
        set_stb_2x <= 1'b0;
      end else begin
        state_dlyd_2x <= state_dlyd;
        set_stb_2x <= set_stb && (state_dlyd_2x != state_dlyd);
      end
    end

    simple_spi_core #(
      .BASE     (0),
      .WIDTH    (NUM_SLAVES),
      .CLK_IDLE (0),
      .SEN_IDLE ({NUM_SLAVES{1'b1}})
    ) simple_spi_core_i (
      .clock        (ctrlport_clk_2x),
      .reset        (ctrlport_rst),
      .set_stb      (set_stb_2x),
      .set_addr     (set_addr),
      .set_data     (set_data),
      .readback     (readback),
      .readback_stb (readback_stb),
      .ready        (),
      .sen          (ss[NUM_SLAVES-1:0]),
      .sclk         (sclk),
      .mosi         (mosi),
      .miso         (miso),
      .debug        ()
    );

    // Delay and extend signal for use in 1x domain.
    reg readback_stb_dly = 1'b0;

    always @ (posedge ctrlport_clk_2x) begin
      readback_stb_dly <= readback_stb;
    end

    assign readback_stb_extended = readback_stb_dly | readback_stb;

  `endif

  //---------------------------------------------------------------------------
  // GPIO Mapping
  //---------------------------------------------------------------------------

  wire [31:0] mosi_mux_out;
  reg  [31:0] mosi_mux_out_dlyd;
  wire [31:0] ss_mux_out;
  wire [31:0] gated_sclk;
  reg  [31:0] gated_sclk_dlyd;
  reg  [31:0] gpio_is_sclk_dlyd;

  genvar i;
  generate

    for (i = 0; i < 32; i = i + 1) begin: dio_output_gen

      // Indicate which GPIO lines are outputs
      assign gpio_ddr[i] = gpio_is_mosi[i] | gpio_is_sclk[i] | gpio_is_cs[i];

      // CS re-mapping
      assign ss_mux_out[i] =  ( i == ss_mapping[0] ) ? ss[0] :
                              ( i == ss_mapping[1] ) ? ss[1] :
                              ( i == ss_mapping[2] ) ? ss[2] :
                              ( i == ss_mapping[3] ) ? ss[3] :
                              1'b0;

      assign mosi_mux_out[i] = gpio_is_mosi[i] ? mosi : ss_mux_out[i];
      assign gated_sclk[i]   = gpio_is_sclk[i] ? sclk : 1'b0;

      // register signals once remapping logic is resolved
      `ifdef X440
        always @ (posedge ctrlport_clk) begin
          mosi_mux_out_dlyd[i] <= mosi_mux_out[i];
          gpio_is_sclk_dlyd[i] <= gpio_is_sclk[i];
          gated_sclk_dlyd[i]   <= gated_sclk[i];
        end
      `else // X410
        always @ (posedge ctrlport_clk_2x) begin
          mosi_mux_out_dlyd[i] <= mosi_mux_out[i];
          gpio_is_sclk_dlyd[i] <= gpio_is_sclk[i];
          gated_sclk_dlyd[i]   <= gated_sclk[i];
        end
      `endif

      // Choose between SCLK and MOSI/SS mux.
      glitch_free_mux glitch_free_gpio_out (
        .select       (gpio_is_sclk_dlyd[i]),
        .signal0      (mosi_mux_out_dlyd[i]),
        .signal1      (gated_sclk_dlyd[i]),
        .muxed_signal (gpio_out[i])
      );

    end
  endgenerate

  assign miso = ( ~ss[0] ) ? gpio_in[miso_mapping[0]] :
                ( ~ss[1] ) ? gpio_in[miso_mapping[1]] :
                ( ~ss[2] ) ? gpio_in[miso_mapping[2]] :
                ( ~ss[3] ) ? gpio_in[miso_mapping[3]] :
                1'b0;


endmodule


`default_nettype wire

//XmlParse xml_on
//<regmap name="DIG_IFC_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <group name="SPI_OVER_GPIO_REGS">
//    <info>
//    </info>
//    <regtype name="SPI_SETUP" size="32">
//      <info>
//        Controls SPI Transaction
//      </info>
//      <bitfield name="SLAVE_CLK"  range="0..4"   initialvalue="0">
//        <info>
//          Indicates which GPIO line to use for the SCLK signal.</br>
//            0-11 : Port A GPIO</br>
//            16-27: Port B GPIO
//        </info>
//      </bitfield>
//      <bitfield name="SLAVE_MOSI" range="5..9"   initialvalue="0">
//        <info>
//          Indicates which GPIO line to use for the MOSI signal.</br>
//            0-11 : Port A GPIO</br>
//            16-27: Port B GPIO
//        </info>
//      </bitfield>
//      <bitfield name="SLAVE_MISO" range="10..14" initialvalue="0">
//        <info>
//          Indicates which GPIO line to use for the MISO signal.</br>
//            0-11 : Port A GPIO</br>
//            16-27: Port B GPIO
//        </info>
//      </bitfield>
//      <bitfield name="SLAVE_CS"   range="15..19" initialvalue="0">
//        <info>
//          Indicates which GPIO line to use for the CS signal.</br>
//            0-11 : Port A GPIO</br>
//            16-27: Port B GPIO
//        </info>
//      </bitfield>
//      <bitfield name="SPI_LENGTH" range="20..25" initialvalue="0">
//        <info>
//          Indicates the length of SPI transactions to this slave.
//        </info>
//      </bitfield>
//      <bitfield name="MISO_EDGE"  range="26"     initialvalue="0">
//        <info>
//          Controls the edge in which the MISO line is latched.</br>
//            0 =  falling edge of SCLK.</br>
//            1 =  rising edge of SCLK.
//        </info>
//      </bitfield>
//      <bitfield name="MOSI_EDGE"  range="27"     initialvalue="0">
//        <info>
//          Controls the edge in which the MOSI line is updated.</br>
//            0 =  falling edge of SCLK.</br>
//            1 =  rising edge of SCLK.
//        </info>
//      </bitfield>
//    </regtype>
//    <register name="SPI_SLAVE_CONFIG" typename="SPI_SETUP"  offset="0x00" count="4" options="--step 4">
//      <info> Set of configuration registers for the supported slaves. </info>
//    </register>
//    <register name="SPI_TRANSACTION_CONFIG" offset="0x10" size="32">
//      <info>
//        Controls clock rate and target for subsequent SPI transactions.
//      </info>
//      <bitfield name="SPI_CLK_DIV"      range="0..15"  initialvalue="0">
//        <info> Controls the rate for subsequent SPI transactions. SCLK = DataClk/[(SPI_CLK_DIV+1)]</info>
//      </bitfield>
//      <bitfield name="SPI_SLAVE_SELECT" range="16..17" initialvalue="0"/>
//    </register>
//    <register name="SPI_TRANSACTION_GO" offset="0x14" size="32" readable="false">
//      <info>
//        Starts a SPI transaction
//      </info>
//      <bitfield name="SPI_DATA"         range="0..31"  initialvalue="0">
//        <info> Payload to be sent for the SPI transaction. If the payload is shorter than 32 bits,
//               it must be aligned to the MSbs in this field. LSbs are ignored in this scenario.</info>
//      </bitfield>
//    </register>
//    <register name="SPI_STATUS" offset="0x18" size="32" writable="false">
//      <info>
//        Contains the status of the SPI engine.
//      </info>
//      <bitfield name="SPI_READY"        range="24"     initialvalue="0">
//        <info> Indicates the SPI engine is ready to start a new SPI transaction. </info>
//      </bitfield>
//      <bitfield name="SPI_RESPONSE"     range="0..23"  initialvalue="0">
//        <info> Records the response of the last completed SPI transaction. </info>
//      </bitfield>
//    </register>
//    <register name="CONTROLLER_INFO" offset="0x1C" size="32" writable="false">
//      <info>
//        Contains information pertaining this SPI controller block.
//      </info>
//      <bitfield name="SLAVE_COUNT"     range="3..0">
//        <info> Indicates the number SPI slaves configurable by the controller. </info>
//      </bitfield>
//    </register>
//  </group>
//</regmap>
//XmlParse xml_off
