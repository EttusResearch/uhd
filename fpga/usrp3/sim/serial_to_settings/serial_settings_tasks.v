
   task serial_settings_transaction;
      input [7:0] address;
      input [31:0] data;

      integer x;
      
      begin
	 scl_r <= 1'b1;
	 sda_r <= 1'b1;
	 @(negedge clk);
	 @(negedge clk);
	 // Drive SDA low whilst SCL high to signal START
	 sda_r <= 1'b0;
	 @(negedge clk);
	 @(negedge clk);
	 // Send 8 Address bits MSB first on falling edge of SCL clocks
	 for (x = 7; x >= 0; x = x - 1)
	   serial_settings_bit(address[x]);
	 // Send 32 Data bits MSB first on falling edge of SCL clocks
	 for (x = 31; x >= 0; x = x - 1)
	   serial_settings_bit(data[x]);
	 // Send STOP.
	 scl_r <= 1'b0;
	 sda_r <= 1'b0;
	  @(negedge clk);
	 @(negedge clk);
	 @(negedge clk);
	 @(negedge clk);
	 scl_r <= 1'b1;
	 @(negedge clk);	 
	 @(negedge clk);
	 @(negedge clk);	
	 @(negedge clk);
	 sda_r <= 1'b1;
	 @(negedge clk);	 
	 @(negedge clk);
	 @(negedge clk);	
	 @(negedge clk);
      end
   endtask // serial_settings_transaction

   task serial_settings_bit;
      input one_bit;

      begin
	 scl_r <= 1'b0;
	 sda_r <= one_bit;
	 @(negedge clk);
	 @(negedge clk);
	 @(negedge clk);
	 @(negedge clk);
	 scl_r <= 1'b1;
	 @(negedge clk);	 
	 @(negedge clk);
	 @(negedge clk);	
	 @(negedge clk);
      end
   endtask // send_settings_bit
