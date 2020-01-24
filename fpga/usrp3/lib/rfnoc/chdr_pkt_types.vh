// CHDR Packet types
// [2:1]: Type
//   [0]: EOB
localparam [2:0] DATA_PKT     = 3'b000;
localparam [2:0] DATA_EOB_PKT = 3'b001;
localparam [2:0] FC_RESP_PKT  = 3'b010;
localparam [2:0] FC_ACK_PKT   = 3'b011;
localparam [2:0] CMD_PKT      = 3'b100;
localparam [2:0] CMD_EOB_PKT  = 3'b101;  // Unused
localparam [2:0] RESP_PKT     = 3'b110;
localparam [2:0] RESP_ERR_PKT = 3'b111;
