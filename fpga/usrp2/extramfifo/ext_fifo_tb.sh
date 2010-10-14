#fuse  -prj ext_fifo_tb.prj  -t work.glbl -t work.ext_fifo_tb -L unisims_ver -L xilinxcorelib_ver -o ext_fifo_tb
iverilog -c ext_fifo_tb.cmd -o ext_fifo_tb ext_fifo_tb.v
