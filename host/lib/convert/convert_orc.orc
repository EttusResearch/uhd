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

.function _convert_fc32_1_to_item32_1_bswap_orc
.source 8 src
.dest 4 dst
.floatparam 4 scalar
.temp 8 scaled
.temp 8 converted
.temp 4 short
x2 mulf scaled, src, scalar
x2 convfl converted, scaled
x2 convlw short, converted
x2 swapw dst, short

.function _convert_item32_1_to_fc32_1_nswap_orc
.source 4 src
.dest 8 dst
.floatparam 4 scalar
.temp 4 tmp1
.temp 8 tmp2
x2 swapw tmp1, src
swapl tmp1, tmp1
x2 convswl tmp2, tmp1
x2 convlf tmp2, tmp2
x2 mulf dst, tmp2, scalar

.function _convert_item32_1_to_fc32_1_bswap_orc
.source 4 src
.dest 8 dst
.floatparam 4 scalar
.temp 4 tmp1
.temp 8 tmp2
x2 swapw tmp1, src
x2 convswl tmp2, tmp1
x2 convlf tmp2, tmp2
x2 mulf dst, tmp2, scalar

.function _convert_sc16_1_to_item32_1_nswap_orc
.source 4 src
.dest 4 dst
.floatparam 4 scalar
swapl dst, src
x2 swapw dst, dst

.function _convert_item32_1_to_sc16_1_nswap_orc
.source 4 src
.dest 4 dst
.floatparam 4 scalar
x2 swapw dst, src
swapl dst, dst
