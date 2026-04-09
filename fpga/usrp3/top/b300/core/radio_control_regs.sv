//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: radio_control_regs
//
// Description:
//   Implements radio block ctrlport endpoints for B310 FPGA
//

`default_nettype none

module radio_control_regs #(
  parameter logic [19:0]  BASE_ADDRESS    = 0,
  parameter logic [19:0]  SIZE_ADDRESS    = 0
) (
  ctrlport_if.slave s_ctrlport,

  // Radio Misc. Control/Status
  output logic        radio_adrv_reset,
  output logic        radio_test_enable,

  // Radio LED Controls
  output logic [1:0]  radio_led_tx_green,
  output logic [1:0]  radio_led_tx_red,
  output logic [1:0]  radio_led_rx_green,

  // Radio FP GPIO Controls
  output logic [9:0]  radio_ch0_fp_gpio_out,
  output logic [9:0]  radio_ch1_fp_gpio_out,
  input  wire  [9:0]  radio_fp_gpio_in,
  output logic [9:0]  radio_ch0_fp_gpio_dir,
  output logic [9:0]  radio_ch1_fp_gpio_dir,

  // Radio ADRV GPIO Controls
  output logic [3:0]  radio_ch0_adrv_gpio_out,
  output logic [3:0]  radio_ch1_adrv_gpio_out,
  input  wire  [3:0]  radio_ch0_adrv_gpio_in,
  input  wire  [3:0]  radio_ch1_adrv_gpio_in,
  output logic [3:0]  radio_ch0_adrv_gpio_dir,
  output logic [3:0]  radio_ch1_adrv_gpio_dir,
  output logic [3:0]  radio_ch0_adrv_trx_out,
  output logic [3:0]  radio_ch1_adrv_trx_out,
  input  wire         radio_ch0_adrv_int,
  input  wire         radio_ch1_adrv_int,

  // Radio path control signals
  output logic [1:0] radio_enable_tdr,
  output logic [1:0] radio_rx_bypass,

  // Radio ATR state
  input wire [1:0] rx_running,
  input wire [1:0] tx_running
);

  import ctrlport_pkg::*;
  `include "../regmap/radio_control_regmap_utils.vh"

  //vhook_sigstart
  //vhook_sigend

  // ATR readback signals
  logic [7:0] ch0_led_atr_rb;
  logic [7:0] ch1_led_atr_rb;
  logic [9:0] ch0_fp_gpio_atr_rb;
  logic [9:0] ch1_fp_gpio_atr_rb;
  logic [7:0] ch0_adrv_gpio_atr_rb;
  logic [7:0] ch1_adrv_gpio_atr_rb;
  logic [1:0] ch0_path_ctrl_rb;
  logic [1:0] ch1_path_ctrl_rb;

  // Split incoming ctrlport into two interfaces:
  // - One for the registers implemented in this module
  // - One for the ATR registers
  localparam int NUM_REG_BLOCKS = 2; // Number of core blocks in the B310 core

  ctrlport_if m_radio_regs_ctrlport_array [NUM_REG_BLOCKS] (.clk(s_ctrlport.clk), .rst(s_ctrlport.rst));

  //vhook_e ctrlport_if_splitter
  //vhook_a NUM_SLAVES   NUM_REG_BLOCKS
  //vhook_a m_ctrlport   m_radio_regs_ctrlport_array
  ctrlport_if_splitter #(
    .NUM_SLAVES(NUM_REG_BLOCKS)  //int:=2
  ) ctrlport_if_splitterx (
    .s_ctrlport(s_ctrlport),                  // ctrlport_if.slave
    .m_ctrlport(m_radio_regs_ctrlport_array)  // ctrlport_if.master[(NUM_SLAVES-1):0]
  );

  ctrlport_if m_radio_regs_ctrlport_lcl(.clk(s_ctrlport.clk), .rst(s_ctrlport.rst));
  ctrlport_if m_radio_sb_ctrlport(.clk(s_ctrlport.clk), .rst(s_ctrlport.rst));

  assign m_radio_regs_ctrlport_lcl.req = m_radio_regs_ctrlport_array[0].req;
  assign m_radio_regs_ctrlport_array[0].resp = m_radio_regs_ctrlport_lcl.resp;
  assign m_radio_sb_ctrlport.req = m_radio_regs_ctrlport_array[1].req;
  assign m_radio_regs_ctrlport_array[1].resp = m_radio_sb_ctrlport.resp;

  //----------------------------------------------------------
  // Handling of CtrlPort
  //----------------------------------------------------------
  wire address_in_range = (m_radio_regs_ctrlport_lcl.req.addr >= BASE_ADDRESS) &&
                          (m_radio_regs_ctrlport_lcl.req.addr < BASE_ADDRESS + SIZE_ADDRESS);
  // Indicate the transaction is targeting the ATR registers. The upper boundary considers all
  // the addresses in the last ATR table.
  wire address_is_atr_wr = (m_radio_regs_ctrlport_lcl.req.addr >= BASE_ADDRESS + CH0_LED_ATR) &&
                           (m_radio_regs_ctrlport_lcl.req.addr < BASE_ADDRESS + CH1_PATH_CTRL_ATR + 8);

  always_ff @(posedge m_radio_regs_ctrlport_lcl.clk) begin
    // reset internal registers and responses
    if (m_radio_regs_ctrlport_lcl.rst) begin
      radio_adrv_reset           <= '0;
      radio_test_enable          <= '0;

      m_radio_regs_ctrlport_lcl.resp.ack     <= 1'b0;
      m_radio_regs_ctrlport_lcl.resp.data    <= 'x;
      m_radio_regs_ctrlport_lcl.resp.status  <= STS_OKAY;

    end else begin

      // write requests
      if (m_radio_regs_ctrlport_lcl.req.wr) begin
        // always issue an ack and no data
        m_radio_regs_ctrlport_lcl.resp.ack     <= 1'b1;
        m_radio_regs_ctrlport_lcl.resp.data    <= 'x;
        m_radio_regs_ctrlport_lcl.resp.status  <= STS_OKAY;

        case (m_radio_regs_ctrlport_lcl.req.addr)
          BASE_ADDRESS + MISC_OUTPUTS: begin
            radio_adrv_reset <= m_radio_regs_ctrlport_lcl.req.data[ADRV9032_RESET];
            radio_test_enable <= m_radio_regs_ctrlport_lcl.req.data[ADRV9032_TEST_EN];
          end

          // error on undefined address
          default: begin
            if (address_in_range) begin
              if (!address_is_atr_wr) begin
                m_radio_regs_ctrlport_lcl.resp.status  <= STS_CMDERR;
              end

            // no response if out of range
            end else begin
              m_radio_regs_ctrlport_lcl.resp.ack     <= 1'b0;
            end
          end
        endcase

      // read requests
      end else if (m_radio_regs_ctrlport_lcl.req.rd) begin
        // default assumption: valid request
        m_radio_regs_ctrlport_lcl.resp.ack     <= 1'b1;
        m_radio_regs_ctrlport_lcl.resp.status  <= STS_OKAY;
        m_radio_regs_ctrlport_lcl.resp.data    <= '0;

        case (m_radio_regs_ctrlport_lcl.req.addr)
          BASE_ADDRESS + MISC_OUTPUTS: begin
            m_radio_regs_ctrlport_lcl.resp.data[ADRV9032_RESET]
              <= radio_adrv_reset;
            m_radio_regs_ctrlport_lcl.resp.data[ADRV9032_TEST_EN]
              <= radio_test_enable;
          end

          BASE_ADDRESS + MISC_INPUTS: begin
            m_radio_regs_ctrlport_lcl.resp.data[ADRV_INTERRUPT_0]
              <= radio_ch0_adrv_int;
            m_radio_regs_ctrlport_lcl.resp.data[ADRV_INTERRUPT_1]
              <= radio_ch1_adrv_int;
          end

          BASE_ADDRESS + CH0_LED_STATUS: begin
            m_radio_regs_ctrlport_lcl.resp.data[LED_TX_GREEN]
              <= ch0_led_atr_rb[LED_TX_GREEN];
            m_radio_regs_ctrlport_lcl.resp.data[LED_TX_RED]
              <= ch0_led_atr_rb[LED_TX_RED];
            m_radio_regs_ctrlport_lcl.resp.data[LED_RX_GREEN]
              <= ch0_led_atr_rb[LED_RX_GREEN];
          end

          BASE_ADDRESS + CH1_LED_STATUS: begin
            m_radio_regs_ctrlport_lcl.resp.data[LED_TX_GREEN]
              <= ch1_led_atr_rb[LED_TX_GREEN];
            m_radio_regs_ctrlport_lcl.resp.data[LED_TX_RED]
              <= ch1_led_atr_rb[LED_TX_RED];
            m_radio_regs_ctrlport_lcl.resp.data[LED_RX_GREEN]
              <= ch1_led_atr_rb[LED_RX_GREEN];
          end

          BASE_ADDRESS + CH0_FP_GPIO_STATUS: begin
            m_radio_regs_ctrlport_lcl.resp.data[FP_DRIVE_VALUES_MSB : FP_DRIVE_VALUES]
              <= ch0_fp_gpio_atr_rb[FP_DRIVE_VALUES_MSB : FP_DRIVE_VALUES];
          end

          BASE_ADDRESS + CH1_FP_GPIO_STATUS: begin
            m_radio_regs_ctrlport_lcl.resp.data[FP_DRIVE_VALUES_MSB : FP_DRIVE_VALUES]
              <= ch1_fp_gpio_atr_rb[FP_DRIVE_VALUES_MSB : FP_DRIVE_VALUES];
          end

          BASE_ADDRESS + CH0_ADRV_GPIO_STATUS: begin
            m_radio_regs_ctrlport_lcl.resp.data[TRX_CTRL_3 : TRX_CTRL_0]
              <= ch0_adrv_gpio_atr_rb[TRX_CTRL_3 : TRX_CTRL_0];
            m_radio_regs_ctrlport_lcl.resp.data[ADRV_GPIO_3 : ADRV_GPIO_0]
              <= ch0_adrv_gpio_atr_rb[ADRV_GPIO_3 : ADRV_GPIO_0];
          end

          BASE_ADDRESS + CH1_ADRV_GPIO_STATUS: begin
            m_radio_regs_ctrlport_lcl.resp.data[TRX_CTRL_3 : TRX_CTRL_0]
              <= ch1_adrv_gpio_atr_rb[TRX_CTRL_3 : TRX_CTRL_0];
            m_radio_regs_ctrlport_lcl.resp.data[ADRV_GPIO_3 : ADRV_GPIO_0]
              <= ch1_adrv_gpio_atr_rb[ADRV_GPIO_3 : ADRV_GPIO_0];
          end

          BASE_ADDRESS + CH0_PATH_CTRL_STATUS: begin
            m_radio_regs_ctrlport_lcl.resp.data[CH_ENABLE_TXRX_TDR]
              <= ch0_path_ctrl_rb[CH_ENABLE_TXRX_TDR];
            m_radio_regs_ctrlport_lcl.resp.data[CH_BYPASS_RX]
              <= ch0_path_ctrl_rb[CH_BYPASS_RX];
          end

          BASE_ADDRESS + CH1_PATH_CTRL_STATUS: begin
            m_radio_regs_ctrlport_lcl.resp.data[CH_ENABLE_TXRX_TDR]
              <= ch1_path_ctrl_rb[CH_ENABLE_TXRX_TDR];
            m_radio_regs_ctrlport_lcl.resp.data[CH_BYPASS_RX]
              <= ch1_path_ctrl_rb[CH_BYPASS_RX];
          end

          // error on undefined address
          default: begin
            m_radio_regs_ctrlport_lcl.resp.data <= '0;
            if (address_in_range) begin
              m_radio_regs_ctrlport_lcl.resp.status <= STS_CMDERR;

            // no response if out of range
            end else begin
              m_radio_regs_ctrlport_lcl.resp.ack <= 1'b0;
            end
          end
        endcase

      // no request
      end else begin
        m_radio_regs_ctrlport_lcl.resp.ack <= 1'b0;
      end
    end
  end

  // ATR register conditioning.
  // The ATR registers requires a settings bus interface, which is translated
  // to in this section.
  // To achieve this, we will filter the ctrlport interface within the register
  // address space containing ATR writes. Read operations in this address space
  // only monitor status signals and can be serviced with ctrlport.
  ctrlport_if radio_atr_if (
    .clk(s_ctrlport.clk),
    .rst(s_ctrlport.rst)
  );

  // Create a window for the ctrlport interface to the ATR registers.

  //vhook_e ctrlport_if_window radio_atr_reg_window
  //vhook_a BASE_ADDRESS BASE_ADDRESS+CH0_LED_ATR
  //vhook_a WINDOW_SIZE  'h80
  //vhook_a s_ctrlport m_radio_sb_ctrlport
  //vhook_a m_ctrlport radio_atr_if
  ctrlport_if_window #(
    .BASE_ADDRESS(BASE_ADDRESS+CH0_LED_ATR),  //int:=0
    .WINDOW_SIZE ('h80)                       //int:=32
  ) radio_atr_reg_window (
    .s_ctrlport(m_radio_sb_ctrlport),  // ctrlport_if.slave
    .m_ctrlport(radio_atr_if)          // ctrlport_if.master
  );

  wire        radio_sb_stb  = radio_atr_if.req.wr;
  wire [7:0]  radio_sb_addr = radio_atr_if.req.addr[7:0];
  wire [31:0] radio_sb_data = radio_atr_if.req.data;

  // Reads are handled by the status registers
  assign radio_atr_if.resp.data   = '0;
  assign radio_atr_if.resp.status = STS_OKAY;

  // ATR Register Implementation
  //----------------------------------------------------------------

  // CH0 LED ATR
  logic [7:0] ch0_led_atr_out;

  //vhook_e gpio_atr ch0_led_atr_i
  //vhook_a BASE BASE_ADDRESS+CH0_LED_ATR
  //vhook_a WIDTH 8
  //vhook_a FAB_CTRL_EN '0
  //vhook_a DEFAULT_DDR '1
  //vhook_a DEFAULT_IDLE '0
  //vhook_a clk s_ctrlport.clk
  //vhook_a reset s_ctrlport.rst
  //vhook_a set_stb radio_sb_stb
  //vhook_a set_addr radio_sb_addr
  //vhook_a set_data radio_sb_data
  //vhook_a rx rx_running[0]
  //vhook_a tx tx_running[0]
  //vhook_a gpio_in '0
  //vhook_a gpio_out ch0_led_atr_out
  //vhook_a gpio_ddr {/*unused, assumed output only*/}
  //vhook_a gpio_out_fab '0
  //vhook_a gpio_sw_rb ch0_led_atr_rb
  gpio_atr #(
    .BASE        (BASE_ADDRESS+CH0_LED_ATR),  //integer:=0
    .WIDTH       (8),                         //integer:=32
    .FAB_CTRL_EN ('0),                        //integer:=0
    .DEFAULT_DDR ('1),                        //integer:=0
    .DEFAULT_IDLE('0)                         //integer:=0
  ) ch0_led_atr_i (
    .clk         (s_ctrlport.clk),                     //input wire
    .reset       (s_ctrlport.rst),                     //input wire
    .set_stb     (radio_sb_stb),                       //input wire
    .set_addr    (radio_sb_addr),                      //input wire[7:0]
    .set_data    (radio_sb_data),                      //input wire[31:0]
    .rx          (rx_running[0]),                      //input wire
    .tx          (tx_running[0]),                      //input wire
    .gpio_in     ('0),                                 //input wire[(WIDTH-1):0]
    .gpio_out    (ch0_led_atr_out),                    //output reg[(WIDTH-1):0]
    .gpio_ddr    ({/*unused, assumed output only*/}),  //output reg[(WIDTH-1):0]
    .gpio_out_fab('0),                                 //input wire[(WIDTH-1):0]
    .gpio_sw_rb  (ch0_led_atr_rb)                      //output reg[(WIDTH-1):0]
  );

  assign radio_led_tx_green[0] = ch0_led_atr_out[LED_TX_GREEN];
  assign radio_led_tx_red[0]   = ch0_led_atr_out[LED_TX_RED];
  assign radio_led_rx_green[0] = ch0_led_atr_out[LED_RX_GREEN];

  //CH1 LED ATR
  logic [7:0] ch1_led_atr_out;

  //vhook_e gpio_atr ch1_led_atr_i
  //vhook_a BASE BASE_ADDRESS+CH1_LED_ATR
  //vhook_a WIDTH 8
  //vhook_a FAB_CTRL_EN '0
  //vhook_a DEFAULT_DDR '1
  //vhook_a DEFAULT_IDLE '0
  //vhook_a clk s_ctrlport.clk
  //vhook_a reset s_ctrlport.rst
  //vhook_a set_stb radio_sb_stb
  //vhook_a set_addr radio_sb_addr
  //vhook_a set_data radio_sb_data
  //vhook_a rx rx_running[1]
  //vhook_a tx tx_running[1]
  //vhook_a gpio_in '0
  //vhook_a gpio_out ch1_led_atr_out
  //vhook_a gpio_ddr {/*unused, assumed output only*/}
  //vhook_a gpio_out_fab '0
  //vhook_a gpio_sw_rb ch1_led_atr_rb
  gpio_atr #(
    .BASE        (BASE_ADDRESS+CH1_LED_ATR),  //integer:=0
    .WIDTH       (8),                         //integer:=32
    .FAB_CTRL_EN ('0),                        //integer:=0
    .DEFAULT_DDR ('1),                        //integer:=0
    .DEFAULT_IDLE('0)                         //integer:=0
  ) ch1_led_atr_i (
    .clk         (s_ctrlport.clk),                     //input wire
    .reset       (s_ctrlport.rst),                     //input wire
    .set_stb     (radio_sb_stb),                       //input wire
    .set_addr    (radio_sb_addr),                      //input wire[7:0]
    .set_data    (radio_sb_data),                      //input wire[31:0]
    .rx          (rx_running[1]),                      //input wire
    .tx          (tx_running[1]),                      //input wire
    .gpio_in     ('0),                                 //input wire[(WIDTH-1):0]
    .gpio_out    (ch1_led_atr_out),                    //output reg[(WIDTH-1):0]
    .gpio_ddr    ({/*unused, assumed output only*/}),  //output reg[(WIDTH-1):0]
    .gpio_out_fab('0),                                 //input wire[(WIDTH-1):0]
    .gpio_sw_rb  (ch1_led_atr_rb)                      //output reg[(WIDTH-1):0]
  );

  assign radio_led_tx_green[1] = ch1_led_atr_out[LED_TX_GREEN];
  assign radio_led_tx_red[1]   = ch1_led_atr_out[LED_TX_RED];
  assign radio_led_rx_green[1] = ch1_led_atr_out[LED_RX_GREEN];

  // CH0 FP GPIO ATR

  //vhook_e gpio_atr ch0_fp_gpio_atr_i
  //vhook_a BASE BASE_ADDRESS+CH0_FP_GPIO_ATR
  //vhook_a WIDTH 10
  //vhook_a FAB_CTRL_EN '0
  //vhook_a DEFAULT_DDR '1
  //vhook_a DEFAULT_IDLE '0
  //vhook_a clk s_ctrlport.clk
  //vhook_a reset s_ctrlport.rst
  //vhook_a set_stb radio_sb_stb
  //vhook_a set_addr radio_sb_addr
  //vhook_a set_data radio_sb_data
  //vhook_a rx rx_running[0]
  //vhook_a tx tx_running[0]
  //vhook_a gpio_in radio_fp_gpio_in
  //vhook_a gpio_out radio_ch0_fp_gpio_out
  //vhook_a gpio_ddr radio_ch0_fp_gpio_dir
  //vhook_a gpio_out_fab '0
  //vhook_a gpio_sw_rb ch0_fp_gpio_atr_rb
  gpio_atr #(
    .BASE        (BASE_ADDRESS+CH0_FP_GPIO_ATR),  //integer:=0
    .WIDTH       (10),                            //integer:=32
    .FAB_CTRL_EN ('0),                            //integer:=0
    .DEFAULT_DDR ('1),                            //integer:=0
    .DEFAULT_IDLE('0)                             //integer:=0
  ) ch0_fp_gpio_atr_i (
    .clk         (s_ctrlport.clk),         //input wire
    .reset       (s_ctrlport.rst),         //input wire
    .set_stb     (radio_sb_stb),           //input wire
    .set_addr    (radio_sb_addr),          //input wire[7:0]
    .set_data    (radio_sb_data),          //input wire[31:0]
    .rx          (rx_running[0]),          //input wire
    .tx          (tx_running[0]),          //input wire
    .gpio_in     (radio_fp_gpio_in),       //input wire[(WIDTH-1):0]
    .gpio_out    (radio_ch0_fp_gpio_out),  //output reg[(WIDTH-1):0]
    .gpio_ddr    (radio_ch0_fp_gpio_dir),  //output reg[(WIDTH-1):0]
    .gpio_out_fab('0),                     //input wire[(WIDTH-1):0]
    .gpio_sw_rb  (ch0_fp_gpio_atr_rb)      //output reg[(WIDTH-1):0]
  );

  // CH1 FP GPIO ATR

  //vhook_e gpio_atr ch1_fp_gpio_atr_i
  //vhook_a BASE BASE_ADDRESS+CH1_FP_GPIO_ATR
  //vhook_a WIDTH 10
  //vhook_a FAB_CTRL_EN '0
  //vhook_a DEFAULT_DDR '1
  //vhook_a DEFAULT_IDLE '0
  //vhook_a clk s_ctrlport.clk
  //vhook_a reset s_ctrlport.rst
  //vhook_a set_stb radio_sb_stb
  //vhook_a set_addr radio_sb_addr
  //vhook_a set_data radio_sb_data
  //vhook_a rx rx_running[1]
  //vhook_a tx tx_running[1]
  //vhook_a gpio_in radio_fp_gpio_in
  //vhook_a gpio_out radio_ch1_fp_gpio_out
  //vhook_a gpio_ddr radio_ch1_fp_gpio_dir
  //vhook_a gpio_out_fab '0
  //vhook_a gpio_sw_rb ch1_fp_gpio_atr_rb
  gpio_atr #(
    .BASE        (BASE_ADDRESS+CH1_FP_GPIO_ATR),  //integer:=0
    .WIDTH       (10),                            //integer:=32
    .FAB_CTRL_EN ('0),                            //integer:=0
    .DEFAULT_DDR ('1),                            //integer:=0
    .DEFAULT_IDLE('0)                             //integer:=0
  ) ch1_fp_gpio_atr_i (
    .clk         (s_ctrlport.clk),         //input wire
    .reset       (s_ctrlport.rst),         //input wire
    .set_stb     (radio_sb_stb),           //input wire
    .set_addr    (radio_sb_addr),          //input wire[7:0]
    .set_data    (radio_sb_data),          //input wire[31:0]
    .rx          (rx_running[1]),          //input wire
    .tx          (tx_running[1]),          //input wire
    .gpio_in     (radio_fp_gpio_in),       //input wire[(WIDTH-1):0]
    .gpio_out    (radio_ch1_fp_gpio_out),  //output reg[(WIDTH-1):0]
    .gpio_ddr    (radio_ch1_fp_gpio_dir),  //output reg[(WIDTH-1):0]
    .gpio_out_fab('0),                     //input wire[(WIDTH-1):0]
    .gpio_sw_rb  (ch1_fp_gpio_atr_rb)      //output reg[(WIDTH-1):0]
  );

  //CH0 ADRV GPIO ATR
  logic [7:0] ch0_adrv_gpio_atr_ddr;

  //vhook_e gpio_atr ch0_adrv_gpio_atr_i
  //vhook_a BASE BASE_ADDRESS+CH0_ADRV_GPIO_ATR
  //vhook_a WIDTH 8
  //vhook_a FAB_CTRL_EN '0
  //vhook_a DEFAULT_DDR '1
  //vhook_a DEFAULT_IDLE '0
  //vhook_a clk s_ctrlport.clk
  //vhook_a reset s_ctrlport.rst
  //vhook_a set_stb radio_sb_stb
  //vhook_a set_addr radio_sb_addr
  //vhook_a set_data radio_sb_data
  //vhook_a rx rx_running[0]
  //vhook_a tx tx_running[0]
  //vhook_a gpio_in {radio_ch0_adrv_gpio_in[3:0], ch0_adrv_gpio_atr_rb[3:0]}
  //vhook_a gpio_out {radio_ch0_adrv_gpio_out[3:0], radio_ch0_adrv_trx_out[3:0]}
  //vhook_a gpio_ddr ch0_adrv_gpio_atr_ddr
  //vhook_a gpio_out_fab '0
  //vhook_a gpio_sw_rb ch0_adrv_gpio_atr_rb
  gpio_atr #(
    .BASE        (BASE_ADDRESS+CH0_ADRV_GPIO_ATR),  //integer:=0
    .WIDTH       (8),                               //integer:=32
    .FAB_CTRL_EN ('0),                              //integer:=0
    .DEFAULT_DDR ('1),                              //integer:=0
    .DEFAULT_IDLE('0)                               //integer:=0
  ) ch0_adrv_gpio_atr_i (
    .clk         (s_ctrlport.clk),                                               //input wire
    .reset       (s_ctrlport.rst),                                               //input wire
    .set_stb     (radio_sb_stb),                                                 //input wire
    .set_addr    (radio_sb_addr),                                                //input wire[7:0]
    .set_data    (radio_sb_data),                                                //input wire[31:0]
    .rx          (rx_running[0]),                                                //input wire
    .tx          (tx_running[0]),                                                //input wire
    .gpio_in     ({radio_ch0_adrv_gpio_in[3:0], ch0_adrv_gpio_atr_rb[3:0]}),     //input wire[(WIDTH-1):0]
    .gpio_out    ({radio_ch0_adrv_gpio_out[3:0], radio_ch0_adrv_trx_out[3:0]}),  //output reg[(WIDTH-1):0]
    .gpio_ddr    (ch0_adrv_gpio_atr_ddr),                                        //output reg[(WIDTH-1):0]
    .gpio_out_fab('0),                                                           //input wire[(WIDTH-1):0]
    .gpio_sw_rb  (ch0_adrv_gpio_atr_rb)                                          //output reg[(WIDTH-1):0]
  );

  assign radio_ch0_adrv_gpio_dir = ch0_adrv_gpio_atr_ddr[ADRV_GPIO_3:ADRV_GPIO_0];


  //CH1 ADRV GPIO ATR
  logic [7:0] ch1_adrv_gpio_atr_ddr;

  //vhook_e gpio_atr ch1_adrv_gpio_atr_i
  //vhook_a BASE BASE_ADDRESS+CH1_ADRV_GPIO_ATR
  //vhook_a WIDTH 8
  //vhook_a FAB_CTRL_EN '0
  //vhook_a DEFAULT_DDR '1
  //vhook_a DEFAULT_IDLE '0
  //vhook_a clk s_ctrlport.clk
  //vhook_a reset s_ctrlport.rst
  //vhook_a set_stb radio_sb_stb
  //vhook_a set_addr radio_sb_addr
  //vhook_a set_data radio_sb_data
  //vhook_a rx rx_running[1]
  //vhook_a tx tx_running[1]
  //vhook_a gpio_in {radio_ch1_adrv_gpio_in[3:0], ch1_adrv_gpio_atr_rb[3:0]}
  //vhook_a gpio_out {radio_ch1_adrv_gpio_out[3:0], radio_ch1_adrv_trx_out[3:0]}
  //vhook_a gpio_ddr ch1_adrv_gpio_atr_ddr
  //vhook_a gpio_out_fab '0
  //vhook_a gpio_sw_rb ch1_adrv_gpio_atr_rb
  gpio_atr #(
    .BASE        (BASE_ADDRESS+CH1_ADRV_GPIO_ATR),  //integer:=0
    .WIDTH       (8),                               //integer:=32
    .FAB_CTRL_EN ('0),                              //integer:=0
    .DEFAULT_DDR ('1),                              //integer:=0
    .DEFAULT_IDLE('0)                               //integer:=0
  ) ch1_adrv_gpio_atr_i (
    .clk         (s_ctrlport.clk),                                               //input wire
    .reset       (s_ctrlport.rst),                                               //input wire
    .set_stb     (radio_sb_stb),                                                 //input wire
    .set_addr    (radio_sb_addr),                                                //input wire[7:0]
    .set_data    (radio_sb_data),                                                //input wire[31:0]
    .rx          (rx_running[1]),                                                //input wire
    .tx          (tx_running[1]),                                                //input wire
    .gpio_in     ({radio_ch1_adrv_gpio_in[3:0], ch1_adrv_gpio_atr_rb[3:0]}),     //input wire[(WIDTH-1):0]
    .gpio_out    ({radio_ch1_adrv_gpio_out[3:0], radio_ch1_adrv_trx_out[3:0]}),  //output reg[(WIDTH-1):0]
    .gpio_ddr    (ch1_adrv_gpio_atr_ddr),                                        //output reg[(WIDTH-1):0]
    .gpio_out_fab('0),                                                           //input wire[(WIDTH-1):0]
    .gpio_sw_rb  (ch1_adrv_gpio_atr_rb)                                          //output reg[(WIDTH-1):0]
  );

  assign radio_ch1_adrv_gpio_dir = ch1_adrv_gpio_atr_ddr[ADRV_GPIO_3:ADRV_GPIO_0];


  //CH0 Path Control ATR

  //vhook_e gpio_atr ch0_path_ctrl_i
  //vhook_a BASE BASE_ADDRESS+CH0_PATH_CTRL_ATR
  //vhook_a WIDTH 2
  //vhook_a FAB_CTRL_EN '0
  //vhook_a DEFAULT_DDR '1
  //vhook_a DEFAULT_IDLE '0
  //vhook_a clk s_ctrlport.clk
  //vhook_a reset s_ctrlport.rst
  //vhook_a set_stb radio_sb_stb
  //vhook_a set_addr radio_sb_addr
  //vhook_a set_data radio_sb_data
  //vhook_a rx rx_running[0]
  //vhook_a tx tx_running[0]
  //vhook_a gpio_in '0
  //vhook_a gpio_out {radio_rx_bypass[0], radio_enable_tdr[0]}
  //vhook_a gpio_ddr {/*unused, assumed output only*/}
  //vhook_a gpio_out_fab '0
  //vhook_a gpio_sw_rb ch0_path_ctrl_rb
  gpio_atr #(
    .BASE        (BASE_ADDRESS+CH0_PATH_CTRL_ATR),  //integer:=0
    .WIDTH       (2),                               //integer:=32
    .FAB_CTRL_EN ('0),                              //integer:=0
    .DEFAULT_DDR ('1),                              //integer:=0
    .DEFAULT_IDLE('0)                               //integer:=0
  ) ch0_path_ctrl_i (
    .clk         (s_ctrlport.clk),                             //input wire
    .reset       (s_ctrlport.rst),                             //input wire
    .set_stb     (radio_sb_stb),                               //input wire
    .set_addr    (radio_sb_addr),                              //input wire[7:0]
    .set_data    (radio_sb_data),                              //input wire[31:0]
    .rx          (rx_running[0]),                              //input wire
    .tx          (tx_running[0]),                              //input wire
    .gpio_in     ('0),                                         //input wire[(WIDTH-1):0]
    .gpio_out    ({radio_rx_bypass[0], radio_enable_tdr[0]}),  //output reg[(WIDTH-1):0]
    .gpio_ddr    ({/*unused, assumed output only*/}),          //output reg[(WIDTH-1):0]
    .gpio_out_fab('0),                                         //input wire[(WIDTH-1):0]
    .gpio_sw_rb  (ch0_path_ctrl_rb)                            //output reg[(WIDTH-1):0]
  );

  //CH1 Path Control ATR

  //vhook_e gpio_atr ch1_path_ctrl_i
  //vhook_a BASE BASE_ADDRESS+CH1_PATH_CTRL_ATR
  //vhook_a WIDTH 2
  //vhook_a FAB_CTRL_EN '0
  //vhook_a DEFAULT_DDR '1
  //vhook_a DEFAULT_IDLE '0
  //vhook_a clk s_ctrlport.clk
  //vhook_a reset s_ctrlport.rst
  //vhook_a set_stb radio_sb_stb
  //vhook_a set_addr radio_sb_addr
  //vhook_a set_data radio_sb_data
  //vhook_a rx rx_running[1]
  //vhook_a tx tx_running[1]
  //vhook_a gpio_in '0
  //vhook_a gpio_out {radio_rx_bypass[1], radio_enable_tdr[1]}
  //vhook_a gpio_ddr {/*unused, assumed output only*/}
  //vhook_a gpio_out_fab '0
  //vhook_a gpio_sw_rb ch1_path_ctrl_rb
  gpio_atr #(
    .BASE        (BASE_ADDRESS+CH1_PATH_CTRL_ATR),  //integer:=0
    .WIDTH       (2),                               //integer:=32
    .FAB_CTRL_EN ('0),                              //integer:=0
    .DEFAULT_DDR ('1),                              //integer:=0
    .DEFAULT_IDLE('0)                               //integer:=0
  ) ch1_path_ctrl_i (
    .clk         (s_ctrlport.clk),                             //input wire
    .reset       (s_ctrlport.rst),                             //input wire
    .set_stb     (radio_sb_stb),                               //input wire
    .set_addr    (radio_sb_addr),                              //input wire[7:0]
    .set_data    (radio_sb_data),                              //input wire[31:0]
    .rx          (rx_running[1]),                              //input wire
    .tx          (tx_running[1]),                              //input wire
    .gpio_in     ('0),                                         //input wire[(WIDTH-1):0]
    .gpio_out    ({radio_rx_bypass[1], radio_enable_tdr[1]}),  //output reg[(WIDTH-1):0]
    .gpio_ddr    ({/*unused, assumed output only*/}),          //output reg[(WIDTH-1):0]
    .gpio_out_fab('0),                                         //input wire[(WIDTH-1):0]
    .gpio_sw_rb  (ch1_path_ctrl_rb)                            //output reg[(WIDTH-1):0]
  );


endmodule : radio_control_regs

`default_nettype wire

//XmlParse xml_on
//
//<regmap name="RADIO_CONTROL_REGMAP" readablestrobes="false" generateverilog="true" generatesv="false" ettusguidelines="true">
//  <group name="RADIO_CONTROL_REGS">
//    <register name="MISC_OUTPUTS" size="32" offset="0x0" attributes="Readable|Writable">
//      <info>
//        This register contains the miscellaneous control for the analog path.
//      </info>
//      <bitfield name="ADRV9032_RESET" range="0">
//        <info>
//          Controls reset line to ADRV9032 chip.
//          0 = reset, 1 = normal operation
//        </info>
//      </bitfield>
//      <bitfield name="ADRV9032_TEST_EN" range="1">
//        <info>
//          Controls test mode for ADRV9032 chip.
//          0 = normal operation, 1 = test mode
//        </info>
//      </bitfield>
//    </register>
//    <register name="MISC_INPUTS" size="32" offset="0x4" attributes="Readable">
//      <info>
//        Status register for miscellaneous inputs.
//      </info>
//      <bitfield name="ADRV_INTERRUPT_0" range="0">
//        <info>
//          Status of ADRV9032 interrupt line 0.
//        </info>
//      </bitfield>
//      <bitfield name="ADRV_INTERRUPT_1" range="1">
//        <info>
//          Status of ADRV9032 interrupt line 1.
//        </info>
//      </bitfield>
//    </register>
//   <regtype name="CH_LED_ATR" size="32">
//     <info>
//       Controls one channels LED values given the channel's ATR state. This is the base offset of
//       6 different registers:
//       BASE+0 DEFAULT_IDLE
//       BASE+1 RX_ACTIVE
//       BASE+2 TX_ACTIVE
//       BASE+3 FDX
//       BASE+4 DDR
//       BASE+5 ATR_DISABLED
//     </info>
//     <bitfield name="LED_TX_GREEN" range="0">
//       <info>
//         Controls the TX green LED state.
//         0 = off, 1 = on
//       </info>
//     </bitfield>
//     <bitfield name="LED_TX_RED" range="1">
//       <info>
//         Controls the TX red LED state.
//         0 = off, 1 = on
//       </info>
//     </bitfield>
//     <bitfield name="LED_RX_GREEN" range="4">
//       <info>
//         Controls the RX green LED state.
//         0 = off, 1 = on
//       </info>
//     </bitfield>
//   </regtype>
//    <register name="CH0_LED_ATR" typename="CH_LED_ATR" offset="0x10" attributes="Writable">
//     <info>
//       Controls the LEDs state for channel 0 given the channel's ATR state.
//     </info>
//   </register>
//   <register name="CH0_LED_STATUS" typename="CH_LED_ATR" offset="0x18" attributes="Readable">
//     <info>
//       Reads the LEDs state for channel 0.
//     </info>
//   </register>
//   <register name="CH1_LED_ATR" typename="CH_LED_ATR" offset="0x20" attributes="Writable">
//     <info>
//       Controls the LEDs state for channel 1 given the channel's ATR state.
//     </info>
//   </register>
//   <register name="CH1_LED_STATUS" typename="CH_LED_ATR" offset="0x28" attributes="Readable">
//     <info>
//       Reads the LEDs state for channel 1.
//     </info>
//   </register>
//   <regtype name="CH_FP_GPIO_ATR" size="32">
//     <info>
//       Controls one channel's HDMI GPIO drive values based on the channel's ATR state.
//       This is the base offset of 6 different registers:
//       BASE+0 DEFAULT_IDLE
//       BASE+1 RX_ACTIVE
//       BASE+2 TX_ACTIVE
//       BASE+3 FDX
//       BASE+4 DDR
//       BASE+5 ATR_DISABLED
//     </info>
//     <bitfield name="FP_DRIVE_VALUES" range="9..0">
//       <info>
//         Controls the HDMI GPIO drive values for the channel.
//       </info>
//     </bitfield>
//   </regtype>
//   <register name="CH0_FP_GPIO_ATR" typename="CH_FP_GPIO_ATR" offset="0x30" attributes="Writable">
//     <info>
//       Controls the HDMI GPIO drive values for channel 0 based on the channel's ATR state
//     </info>
//   </register>
//   <register name="CH0_FP_GPIO_STATUS" typename="CH_FP_GPIO_ATR" offset="0x38" attributes="Readable">
//     <info>
//       Reads the HDMI GPIO drive values for channel 0.
//     </info>
//   </register>
//   <register name="CH1_FP_GPIO_ATR" typename="CH_FP_GPIO_ATR" offset="0x40" attributes="Writable">
//     <info>
//       Controls the HDMI GPIO drive values for channel 1 based on the channel's ATR state
//     </info>
//   </register>
//   <register name="CH1_FP_GPIO_STATUS" typename="CH_FP_GPIO_ATR" offset="0x48" attributes="Readable">
//     <info>
//       Reads the HDMI GPIO drive values for channel 1.
//     </info>
//   </register>
//   <regtype name="CH_ADRV_GPIO_ATR" size="32">
//     <info>
//       Controls one channel's ADRV TRX drive values based on the channel's ATR state.
//       This is the base offset of 6 different registers:
//       BASE+0 DEFAULT_IDLE
//       BASE+1 RX_ACTIVE
//       BASE+2 TX_ACTIVE
//       BASE+3 FDX
//       BASE+4 DDR
//       BASE+5 ATR_DISABLED
//     </info>
//     <bitfield name="TRX_CTRL_0" range="0">
//       <info>
//         Controls first ADRV TRX line for the channel.
//       </info>
//     </bitfield>
//     <bitfield name="TRX_CTRL_1" range="1">
//       <info>
//         Controls second ADRV TRX line for the channel.
//       </info>
//     </bitfield>
//     <bitfield name="TRX_CTRL_2" range="2">
//       <info>
//         Controls third ADRV TRX line for the channel.
//       </info>
//     </bitfield>
//     <bitfield name="TRX_CTRL_3" range="3">
//       <info>
//         Controls fourth ADRV TRX line for the channel.
//       </info>
//     </bitfield>
//     <bitfield name="ADRV_GPIO_0" range="4">
//       <info>
//         Controls first ADRV GPIO line for the channel.
//       </info>
//     </bitfield>
//     <bitfield name="ADRV_GPIO_1" range="5">
//       <info>
//         Controls second ADRV GPIO line for the channel.
//       </info>
//     </bitfield>
//     <bitfield name="ADRV_GPIO_2" range="6">
//       <info>
//         Controls third ADRV GPIO line for the channel.
//       </info>
//     </bitfield>
//     <bitfield name="ADRV_GPIO_3" range="7">
//       <info>
//         Controls fourth ADRV GPIO line for the channel.
//       </info>
//     </bitfield>
//   </regtype>
//   <register name="CH0_ADRV_GPIO_ATR" typename="CH_ADRV_GPIO_ATR" offset="0x50" attributes="Writable">
//     <info>
//       Controls the ADRV TRX GPIO drive values for channel 0 based on the channel
//       ATR state.
//     </info>
//   </register>
//   <register name="CH0_ADRV_GPIO_STATUS" typename="CH_ADRV_GPIO_ATR" offset="0x58" attributes="Readable">
//     <info>
//       Reads the ADRV TRX GPIO drive values for channel 0.
//     </info>
//   </register>
//   <register name="CH1_ADRV_GPIO_ATR" typename="CH_ADRV_GPIO_ATR" offset="0x60" attributes="Writable">
//     <info>
//       Controls the ADRV TRX GPIO drive values for channel 1 based on the channel
//       ATR state.
//     </info>
//   </register>
//   <register name="CH1_ADRV_GPIO_STATUS" typename="CH_ADRV_GPIO_ATR" offset="0x68" attributes="Readable">
//     <info>
//       Reads the ADRV TRX GPIO drive values for channel 1.
//     </info>
//   </register>
//   <regtype name="CH_PATH_CTRL_ATR" size="32">
//     <info>
//       Controls one channel's path control signals.
//       This is the base offset of 6 different registers:
//       BASE+0 DEFAULT_IDLE
//       BASE+1 RX_ACTIVE
//       BASE+2 TX_ACTIVE
//       BASE+3 FDX
//       BASE+4 DDR
//       BASE+5 ATR_DISABLED
//     </info>
//     <bitfield name="CH_ENABLE_TXRX_TDR" range="0">
//       <info>
//         Controls FE TDR switch.
//       </info>
//     </bitfield>
//     <bitfield name="CH_BYPASS_RX" range="1">
//       <info>
//         Controls RX bypass switch.
//         1 = bypass, 0 = RX enabled
//       </info>
//     </bitfield>
//   </regtype>
//   <register name="CH0_PATH_CTRL_ATR" typename="CH_PATH_CTRL_ATR" offset="0x70" attributes="Writable">
//     <info>
//       Controls the path control signals for channel 0 based on the channel's ATR state.
//     </info>
//   </register>
//   <register name="CH0_PATH_CTRL_STATUS" typename="CH_PATH_CTRL_ATR" offset="0x78" attributes="Readable">
//     <info>
//       Reads the path control signals for channel 0.
//     </info>
//   </register>
//   <register name="CH1_PATH_CTRL_ATR" typename="CH_PATH_CTRL_ATR" offset="0x80" attributes="Writable">
//     <info>
//       Controls the path control signals for channel 1 based on the channel's ATR state.
//     </info>
//   </register>
//   <register name="CH1_PATH_CTRL_STATUS" typename="CH_PATH_CTRL_ATR" offset="0x88" attributes="Readable">
//     <info>
//       Reads the path control signals for channel 1.
//     </info>
//   </register>
// </group>
// </regmap>
//
//XmlParse xml_off
