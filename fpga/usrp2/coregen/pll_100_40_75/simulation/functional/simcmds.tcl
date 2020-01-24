# file: simcmds.tcl

# create the simulation script
vcd dumpfile isim.vcd
vcd dumpvars -m /pll_100_40_75_tb -l 0
wave add /
run 50000ns
quit
