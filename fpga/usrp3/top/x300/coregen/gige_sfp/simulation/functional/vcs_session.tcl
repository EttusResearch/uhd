gui_open_window Wave
gui_list_select -id Hier.1 { glbl demo_tb }
gui_sg_create PCS_PMA_group
gui_list_add_group -id Wave.1 {PCS_PMA_group}

gui_list_add_divider -id Wave.1 -after PCS_PMA_group { Test_semaphores }
gui_list_add_divider -id Wave.1 -after PCS_PMA_group { Rx_Stimulus }
gui_list_add_divider -id Wave.1 -after PCS_PMA_group { Tx_Monitor }
gui_list_add_divider -id Wave.1 -after PCS_PMA_group { Transceiver_Rx_Signals }
gui_list_add_divider -id Wave.1 -after PCS_PMA_group { Transceiver_Tx_Signals }
gui_list_add_divider -id Wave.1 -after PCS_PMA_group { Rx_GMII_Signals }
gui_list_add_divider -id Wave.1 -after PCS_PMA_group { Tx_GMII_Signals }
gui_list_add_divider -id Wave.1 -after PCS_PMA_group { Management_Signals }
gui_list_add_divider -id Wave.1 -after PCS_PMA_group { System_Signals }
gui_list_add -id Wave.1 -after System_Signals {{demo_tb.gtrefclk_p} {demo_tb.gtrefclk_n}}
gui_list_add -id Wave.1 -after System_Signals {demo_tb.signal_detect}
gui_list_add -id Wave.1 -after Management_Signals {{demo_tb.status_vector}}
gui_list_add -id Wave.1 -after Tx_GMII_Signals {{demo_tb.gmii_txd} {demo_tb.gmii_tx_en} {demo_tb.gmii_tx_er}}
gui_list_add -id Wave.1 -after Rx_GMII_Signals {{demo_tb.gmii_rxd} {demo_tb.gmii_rx_dv} {demo_tb.gmii_rx_er}}
gui_list_add -id Wave.1 -after Transceiver_Tx_Signals {{demo_tb.txp} {demo_tb.txn}}
gui_list_add -id Wave.1 -after Transceiver_Rx_Signals {{demo_tb.rxp} {demo_tb.rxn}}
gui_list_add -id Wave.1 -after Tx_Monitor {{demo_tb.stimulus.mon_tx_clk} {demo_tb.stimulus.tx_pdata} {demo_tb.stimulus.tx_is_k}}
gui_list_add -id Wave.1 -after Tx_Monitor {{demo_tb.stimulus.bitclock}}
gui_list_add -id Wave.1 -after Rx_Stimulus {{demo_tb.stimulus.stim_rx_clk} {demo_tb.stimulus.rx_even} {demo_tb.stimulus.rx_pdata} {demo_tb.stimulus.rx_is_k} {demo_tb.stimulus.rx_rundisp_pos}}
gui_list_add -id Wave.1 -after Test_semaphores {{demo_tb.configuration_finished} {demo_tb.tx_monitor_finished} {demo_tb.rx_monitor_finished} {demo_tb.simulation_finished}}
gui_zoom -window Wave.1 -full

