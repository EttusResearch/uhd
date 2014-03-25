#!/bin/sh
mkdir work

echo "Compiling Core Simulation Models"
ncvlog -work work ../../../gige_sfp_mdio.v

echo "Compiling Example Design"
ncvlog -work work \
../../example_design/gige_sfp_mdio_sync_block.v \
../../example_design/gige_sfp_mdio_reset_sync.v \
../../example_design/transceiver/gige_sfp_mdio_gtwizard_gt.v \
../../example_design/transceiver/gige_sfp_mdio_gtwizard.v \
../../example_design/transceiver/gige_sfp_mdio_tx_startup_fsm.v \
../../example_design/transceiver/gige_sfp_mdio_rx_startup_fsm.v \
../../example_design/transceiver/gige_sfp_mdio_recclk_monitor.v \
../../example_design/transceiver/gige_sfp_mdio_gtwizard_init.v \
../../example_design/transceiver/gige_sfp_mdio_transceiver.v \
../../example_design/gige_sfp_mdio_tx_elastic_buffer.v \
../../example_design/gige_sfp_mdio_block.v \
../../example_design/gige_sfp_mdio_example_design.v

echo "Compiling Test Bench"
ncvlog -work work ../stimulus_tb.v ../demo_tb.v

echo "Elaborating design"
ncelab -access +rw work.demo_tb glbl

echo "Starting simulation"
ncsim -gui work.demo_tb -input @"simvision -input wave_ncsim.sv"
