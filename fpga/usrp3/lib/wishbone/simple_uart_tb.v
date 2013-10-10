module simple_uart_tb();
   
   localparam SUART_CLKDIV = 0;
   localparam SUART_TXLEVEL = 1;
   localparam SUART_RXLEVEL = 2;
   localparam SUART_TXCHAR = 3;
   localparam SUART_RXCHAR = 4;
   
   reg           clk;
   reg 		 rst;

   reg 		 we_i;
   reg 		 stb_i;
   reg 		 cyc_i;
   wire 	 ack_o;
   reg [2:0] 	 adr_i;
   reg [31:0] 	 dat_i;
   wire [31:0] 	 dat_o;
   wire 	 rx_int_o;
   wire 	 tx_int_o;
   wire 	 tx_o;
   reg 		 rx_i;
   wire 	 baud_o;

   reg [31:0] 	 read_data;
   

   initial 
     clk = 0;

   // 200MHz clock
   always
     #2.5 clk = ~clk;

   initial begin
      rst <= 0;
      we_i <= 0;
      stb_i <= 0;
      cyc_i <= 0;
      adr_i <= 0;
      dat_i <= 0;
      rx_i <= 0;
   end
   
   
   task write_wb;
      input [31:0] data_in;
      input [2:0]  addr_in;

      begin
	 @(negedge clk);	 
	 dat_i <= data_in;
	 adr_i <= addr_in;
	 we_i <= 1;
	 stb_i <= 1;
	 cyc_i <= 1;
	 @(negedge clk);
	 while (ack_o == 0) begin
	    @(negedge clk);
	 end
	 dat_i <= 0;
	 adr_i <= 0;
	 we_i <= 0;
	 stb_i <= 0;
	 cyc_i <= 0;
      end
   endtask // write_wb

   
   task read_wb;
      output [31:0] data_out;
      input [2:0]  addr_in;

      begin
	 @(negedge clk);	 
	 adr_i <= addr_in;
	 we_i <= 0;
	 stb_i <= 1;
	 cyc_i <= 1;
	 @(negedge clk);
	 while (ack_o == 0) begin
	    @(negedge clk);
	 end
	 data_out <= dat_o;
	 adr_i <= 0;
	 stb_i <= 0;
	 cyc_i <= 0;
      end
   endtask // write_wb

   initial begin
      @(negedge clk);    
      rst <= 1;
      repeat(10) @(negedge clk);
      rst <= 0;
      repeat(10) @(negedge clk);
      write_wb(4'h0620,SUART_CLKDIV);
      repeat(10) @(negedge clk);
      read_wb(read_data,SUART_TXLEVEL);
      repeat(10) @(negedge clk);
   end // initial begin
   
   
   
   simple_uart
     #(.CLKDIV_DEFAULT(16'd0))
   simple_uart_i
     (
      .clk_i(clk),
      .rst_i(rst),
      .we_i(we_i),
      .stb_i(stb_i),
      .cyc_i(cyc_i),
      .ack_o(ack_o),
      .adr_i(adr_i),
      .dat_i(dat_i),
      .dat_o(dat_o),
      .rx_int_o(rx_int_o),
      .tx_int_o(tx_int_o), 
      .tx_o(tx_o), 
      .rx_i(rx_i),
      .baud_o(baud_o)
      );

   

endmodule // simple_uart_tb
