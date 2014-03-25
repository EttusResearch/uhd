#!/bin/sh

rm -rf simv* csrc DVEfiles AN.DB

echo "Compiling Core Simulation Models"
vlogan +v2k \
../../../gige_sfp.v \
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
../../example_design/gige_sfp_example_design.v \
../stimulus_tb.v \
../demo_tb.v

echo "Elaborating design"
vcs +vcs+lic+wait \
    -debug \
    demo_tb glbl

echo "Starting simulation"
./simv -ucli -i ucli_commands.key

dve -vpd vcdplus.vpd -session vcs_session.tcl
