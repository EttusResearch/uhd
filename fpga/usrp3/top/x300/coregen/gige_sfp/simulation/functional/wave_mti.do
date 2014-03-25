view structure
view signals
view wave

onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate -divider {System Signals}
add wave -noupdate -format logic /demo_tb/reset
add wave -noupdate -format logic /demo_tb/gtrefclk_p
add wave -noupdate -format logic /demo_tb/gtrefclk_n
add wave -noupdate -format logic /demo_tb/signal_detect
add wave -noupdate -divider {Management I/F}
add wave -noupdate -format logic -binary /demo_tb/status_vector
add wave -noupdate -divider {Tx GMII}
add wave -noupdate -format logic -hex /demo_tb/gmii_txd
add wave -noupdate -format logic /demo_tb/gmii_tx_en
add wave -noupdate -format logic /demo_tb/gmii_tx_er
add wave -noupdate -divider {Rx GMII}
add wave -noupdate -format logic -hex /demo_tb/gmii_rxd
add wave -noupdate -format logic /demo_tb/gmii_rx_dv
add wave -noupdate -format logic /demo_tb/gmii_rx_er
add wave -noupdate -divider {Transceiver Tx}
add wave -noupdate -format logic /demo_tb/txp
add wave -noupdate -format logic /demo_tb/txn
add wave -noupdate -divider {Transceiver Rx}
add wave -noupdate -format logic /demo_tb/rxp
add wave -noupdate -format logic /demo_tb/rxn
add wave -noupdate -divider {Tx Monitor}
add wave -noupdate -format logic /demo_tb/stimulus/mon_tx_clk
add wave -noupdate -format logic -hex /demo_tb/stimulus/tx_pdata
add wave -noupdate -format logic /demo_tb/stimulus/tx_is_k
add wave -noupdate -format logic /demo_tb/stimulus/bitclock
add wave -noupdate -divider {Rx Stimulus}
add wave -noupdate -format logic /demo_tb/stimulus/stim_rx_clk
add wave -noupdate -format logic /demo_tb/stimulus/rx_even
add wave -noupdate -format logic -hex /demo_tb/stimulus/rx_pdata
add wave -noupdate -format logic /demo_tb/stimulus/rx_is_k
add wave -noupdate -format logic /demo_tb/stimulus/rx_rundisp_pos
add wave -noupdate -divider {Test semaphores}
add wave -noupdate -format logic /demo_tb/configuration_finished
add wave -noupdate -format logic /demo_tb/tx_monitor_finished
add wave -noupdate -format logic /demo_tb/rx_monitor_finished
add wave -noupdate -format logic /demo_tb/simulation_finished
TreeUpdate [SetDefaultTree]
