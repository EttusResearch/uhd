.function _convert_fc32_1_to_item32_1_nswap_orc
.source 8 src
.dest 4 dst
.floatparam 4 scalar
.temp 8 scaled
.temp 8 converted
.temp 4 short

x2 mulf scaled, src, scalar
x2 convfl converted, scaled
x2 convlw short, converted
swapl short, short
x2 swapw dst, short
