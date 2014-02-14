# SimVision Command Script

#
# groups
#

if {[catch {group new -name {System Signals} -overlay 0}] != ""} {
    group using {System Signals}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    demo_tb.reset \
    demo_tb.gtrefclk_p \
    demo_tb.gtrefclk_n
    demo_tb.signal_detect \

if {[catch {group new -name {Management I/F} -overlay 0}] != ""} {
    group using {Management I/F}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    demo_tb.status_vector

if {[catch {group new -name {Tx GMII} -overlay 0}] != ""} {
    group using {Tx GMII}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    {demo_tb.gmii_txd[7:0]} \
    demo_tb.gmii_tx_en \
    demo_tb.gmii_tx_er

if {[catch {group new -name {Rx GMII} -overlay 0}] != ""} {
    group using {Rx GMII}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    {demo_tb.gmii_rxd[7:0]} \
    demo_tb.gmii_rx_dv \
    demo_tb.gmii_rx_er

if {[catch {group new -name {Transceiver Tx} -overlay 0}] != ""} {
    group using {Transceiver Tx}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    demo_tb.txp \
    demo_tb.txn

if {[catch {group new -name {Transceiver Rx} -overlay 0}] != ""} {
    group using {Transceiver Rx}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    demo_tb.rxp \
    demo_tb.rxn

if {[catch {group new -name {Tx Monitor} -overlay 0}] != ""} {
    group using {Tx Monitor}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    demo_tb.stimulus.mon_tx_clk \
    {demo_tb.stimulus.tx_pdata[7:0]} \
    demo_tb.stimulus.tx_is_k \
    demo_tb.stimulus.bitclock
if {[catch {group new -name {Rx Stimulus} -overlay 0}] != ""} {
    group using {Rx Stimulus}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    demo_tb.stimulus.stim_rx_clk \
    demo_tb.stimulus.rx_even \
    {demo_tb.stimulus.rx_pdata[7:0]} \
    demo_tb.stimulus.rx_is_k \
    demo_tb.stimulus.rx_rundisp_pos

if {[catch {group new -name {Test semaphores} -overlay 0}] != ""} {
    group using {Test semaphores}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    demo_tb.configuration_finished \
    demo_tb.tx_monitor_finished \
    demo_tb.rx_monitor_finished \
    demo_tb.simulation_finished

#
# Waveform windows
#
if {[window find -match exact -name "Waveform 1"] == {}} {
    window new WaveWindow -name "Waveform 1" -geometry 906x585+25+55
} else {
    window geometry "Waveform 1" 906x585+25+55
}
window target "Waveform 1" on
waveform using {Waveform 1}
waveform sidebar visibility partial
waveform set \
    -primarycursor TimeA \
    -signalnames name \
    -signalwidth 175 \
    -units fs \
    -valuewidth 75
cursor set -using TimeA -time 50,000,000,000fs
cursor set -using TimeA -marching 1
waveform baseline set -time 0

set groupId [waveform add -groups {{System Signals}}]

set groupId [waveform add -groups {{Management I/F}}]

set groupId [waveform add -groups {{Tx GMII}}]

set groupId [waveform add -groups {{Rx GMII}}]

set groupId [waveform add -groups {{Transceiver Tx}}]

set groupId [waveform add -groups {{Transceiver Rx}}]


set groupId [waveform add -groups {{Tx Monitor}}]

set groupId [waveform add -groups {{Rx Stimulus}}]

set groupId [waveform add -groups {{Test semaphores}}]

waveform xview limits 0fs 10us

simcontrol run -time 200us
