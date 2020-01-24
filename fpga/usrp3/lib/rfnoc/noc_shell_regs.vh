  // Registers 0 - 127 for NoC Shell
  localparam [7:0] SR_FLOW_CTRL_BYTES_PER_ACK     = 1;
  localparam [7:0] SR_FLOW_CTRL_WINDOW_SIZE       = 2;
  localparam [7:0] SR_FLOW_CTRL_EN                = 3;
  localparam [7:0] SR_ERROR_POLICY                = 4;
  localparam [7:0] SR_SRC_SID                     = 5;
  localparam [7:0] SR_NEXT_DST_SID                = 6;
  localparam [7:0] SR_RESP_IN_DST_SID             = 7;
  localparam [7:0] SR_RESP_OUT_DST_SID            = 8;
  localparam [7:0] SR_FLOW_CTRL_PKT_LIMIT         = 9;
  localparam [7:0] SR_RB_ADDR_USER                = 124;
  localparam [7:0] SR_CLEAR_RX_FC                 = 125;
  localparam [7:0] SR_CLEAR_TX_FC                 = 126;
  localparam [7:0] SR_RB_ADDR                     = 127;
  // Registers 128-255 for users
  localparam [7:0] SR_USER_REG_BASE               = 128;

  // NoC Shell readback registers
  localparam [7:0] RB_NOC_ID                      = 0;
  localparam [7:0] RB_GLOBAL_PARAMS               = 1;
  localparam [7:0] RB_FIFOSIZE                    = 2;
  localparam [7:0] RB_MTU                         = 3;
  localparam [7:0] RB_BLOCK_PORT_SIDS             = 4;
  localparam [7:0] RB_USER_RB_DATA                = 5;
  localparam [7:0] RB_NOC_SHELL_COMPAT_NUM        = 6;
