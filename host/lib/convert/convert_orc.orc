.function _convert_fc32_1_to_item32_1_nswap_orc
.source 8 src
.dest 4 dst
.floatparam 4 scalar
.temp 8 scaled
.temp 4 converted

x2 mulf scaled, src, scalar
x2 convfw converted, scaled
swapl converted, converted
x2 swapw dst, converted
