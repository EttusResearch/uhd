module float_to_iq_tb();

reg clk, reset;

integer x,file;
reg [31:0]  in;
wire [15:0] out;

initial clk = 0;

always #10 clk = ~clk;

initial $dumpfile("float_to_iq_tb.vcd");
initial $dumpvars(0,float_to_iq_tb);

initial 
   begin

      x <= 0;
      reset <= 1;
      in <= 0;
      file = $fopen("float_to_iq_VER.txt");

      repeat(65536) @(posedge clk);
      reset <=0;
      repeat(65536) @(posedge clk)
         begin
            in <= data[x];
            x <= x+1;
            $fdisplayh(file,out);
         end
         $fclose(file);
         repeat(65536) @(posedge clk);
         $finish;
      end

      float_to_iq #(.BITS_IN(32),.BITS_OUT(16))
      dut 
         (
            .in(in), .out(out), .clk(clk), .reset(reset)
         );
//input
         reg    [31:0] data [0:65535];
         initial $readmemh("iq_to_float_output.txt",data);
//golden output
//
/*
         reg    [15:0] out_array [0:65535];
         initial $readmemh("my_data.txt",out_array);
         reg fail;
         initial 
            fail <= 0;
//compare golden output with your output

         always @(posedge clk) begin
            if (out != out_array[index]) begin
               $display("Line %d : Expected %x, got %x",index,out_array[index],out);
            fail <= 1;
         end
      end
*/
         end





         endmodule

