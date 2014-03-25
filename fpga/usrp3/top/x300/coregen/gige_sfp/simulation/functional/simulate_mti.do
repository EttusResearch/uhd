vlib work
vmap work work

echo "Compiling Core Simulation Models"
vlog -work work ../../../gige_sfp.v

echo "Compiling Example Design"
vlog -work work \
../../example_design/gige_sfp_sync_block.v \
../../example_design/gige_sfp_reset_sync.v \
../../example_design/transceiver/gige_sfp_gtwizard_gt.v \
../../example_design/transceiver/gige_sfp_gtwizard.v \
../../example_design/transceiver/gige_sfp_tx_startup_fsm.v \
../../example_design/transceiver/gige_sfp_rx_startup_fsm.v \
../../example_design/transceiver/gige_sfp_gtwizard_init.v \
../../example_design/transceiver/gige_sfp_transceiver.v \
../../example_design/gige_sfp_tx_elastic_buffer.v \
../../example_design/gige_sfp_block.v \
../../example_design/gige_sfp_example_design.v

echo "Compiling Test Bench"
vlog -work work -novopt ../stimulus_tb.v ../demo_tb.v

echo "Starting simulation"
vsim -voptargs="+acc" -L unisims_ver -L secureip -t ps work.demo_tb work.glbl
do wave_mti.do
run -all

