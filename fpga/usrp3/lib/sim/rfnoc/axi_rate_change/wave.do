onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate /axi_rate_change_tb/axi_rate_change/clk
add wave -noupdate /axi_rate_change_tb/axi_rate_change/reset
add wave -noupdate /axi_rate_change_tb/axi_rate_change/clear
add wave -noupdate /axi_rate_change_tb/axi_rate_change/clear_user
add wave -noupdate /axi_rate_change_tb/axi_rate_change/active
add wave -noupdate /axi_rate_change_tb/axi_rate_change/i_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/i_tlast
add wave -noupdate /axi_rate_change_tb/axi_rate_change/i_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/i_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/i_tuser
add wave -noupdate /axi_rate_change_tb/axi_rate_change/word_cnt_div_n
add wave -noupdate /axi_rate_change_tb/axi_rate_change/word_cnt_div_n_frac
add wave -noupdate /axi_rate_change_tb/axi_rate_change/word_cnt_div_n_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/word_cnt_div_n_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/word_cnt_div_n_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/in_pkt_cnt
add wave -noupdate /axi_rate_change_tb/axi_rate_change/i_reg_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/i_reg_tlast
add wave -noupdate /axi_rate_change_tb/axi_rate_change/i_reg_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/i_reg_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/i_reg_tuser
add wave -noupdate /axi_rate_change_tb/axi_rate_change/m_axis_data_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/m_axis_data_tlast
add wave -noupdate /axi_rate_change_tb/axi_rate_change/m_axis_data_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/m_axis_data_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/s_axis_data_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/s_axis_data_tlast
add wave -noupdate /axi_rate_change_tb/axi_rate_change/s_axis_data_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/s_axis_data_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/out_payload_cnt
add wave -noupdate /axi_rate_change_tb/axi_rate_change/payload_length_out
add wave -noupdate /axi_rate_change_tb/axi_rate_change/word_cnt_div_m
add wave -noupdate /axi_rate_change_tb/axi_rate_change/word_cnt_div_n_fifo_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/word_cnt_div_n_fifo_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/word_cnt_div_n_fifo_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/first_pkt_out
add wave -noupdate /axi_rate_change_tb/axi_rate_change/vita_time_out
add wave -noupdate /axi_rate_change_tb/axi_rate_change/vita_time_accum
add wave -noupdate /axi_rate_change_tb/axi_rate_change/vita_time_reg
add wave -noupdate /axi_rate_change_tb/axi_rate_change/header_fifo_out_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/header_fifo_out_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/o_reg_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/o_reg_tlast
add wave -noupdate /axi_rate_change_tb/axi_rate_change/o_reg_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/o_reg_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/o_reg_tuser
add wave -noupdate /axi_rate_change_tb/axi_rate_change/o_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/o_tlast
add wave -noupdate /axi_rate_change_tb/axi_rate_change/o_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/o_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/o_tuser
add wave -noupdate -divider {AXI Drop Partial Packet}
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/i_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/i_terror
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/i_tlast
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/i_tlast_int
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/i_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/i_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/in_cnt
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/large_pkt
add wave -noupdate {/axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/o_tdata[32]}
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/o_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/o_tlast
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/o_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/o_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/small_pkt
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/flush
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/hold_last_sample
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/in_pkt_cnt
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/out_pkt_cnt
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/release_due_to_error
add wave -noupdate -divider {AXI Drop Packet}
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/hold
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/i_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/i_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/i_terror
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/i_tlast
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/i_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/empty
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/full
add wave -noupdate -radix unsigned /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/rd_addr
add wave -noupdate -radix unsigned /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/wr_addr
add wave -noupdate -radix unsigned /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/in_pkt_cnt
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/int_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/int_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/int_tlast
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/int_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/mem
add wave -noupdate -radix hexadecimal /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/o_tdata
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/o_tvalid
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/o_tready
add wave -noupdate /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/o_tlast
add wave -noupdate -radix unsigned /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/out_pkt_cnt
add wave -noupdate -radix unsigned /axi_rate_change_tb/axi_rate_change/axi_drop_partial_packet/axi_drop_packet/prev_wr_addr
add wave -noupdate /axi_rate_change_tb/axi_rate_change/n
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {1112517 ps} 0} {{Cursor 2} {1974349848 ps} 0}
quietly wave cursor active 2
configure wave -namecolwidth 633
configure wave -valuecolwidth 184
configure wave -justifyvalue left
configure wave -signalnamewidth 0
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 0
configure wave -gridperiod 1
configure wave -griddelta 40
configure wave -timeline 0
configure wave -timelineunits ns
update
WaveRestoreZoom {1974303474 ps} {1974374671 ps}
