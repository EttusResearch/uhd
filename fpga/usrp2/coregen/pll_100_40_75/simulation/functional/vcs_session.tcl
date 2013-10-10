gui_open_window Wave
gui_sg_create pll_100_40_75_group
gui_list_add_group -id Wave.1 {pll_100_40_75_group}
gui_sg_addsignal -group pll_100_40_75_group {pll_100_40_75_tb.test_phase}
gui_set_radix -radix {ascii} -signals {pll_100_40_75_tb.test_phase}
gui_sg_addsignal -group pll_100_40_75_group {{Input_clocks}} -divider
gui_sg_addsignal -group pll_100_40_75_group {pll_100_40_75_tb.CLK_IN1}
gui_sg_addsignal -group pll_100_40_75_group {{Output_clocks}} -divider
gui_sg_addsignal -group pll_100_40_75_group {pll_100_40_75_tb.dut.clk}
gui_list_expand -id Wave.1 pll_100_40_75_tb.dut.clk
gui_sg_addsignal -group pll_100_40_75_group {{Status_control}} -divider
gui_sg_addsignal -group pll_100_40_75_group {pll_100_40_75_tb.RESET}
gui_sg_addsignal -group pll_100_40_75_group {pll_100_40_75_tb.LOCKED}
gui_sg_addsignal -group pll_100_40_75_group {{Counters}} -divider
gui_sg_addsignal -group pll_100_40_75_group {pll_100_40_75_tb.COUNT}
gui_sg_addsignal -group pll_100_40_75_group {pll_100_40_75_tb.dut.counter}
gui_list_expand -id Wave.1 pll_100_40_75_tb.dut.counter
gui_zoom -window Wave.1 -full
