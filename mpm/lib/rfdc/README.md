The `xrfdc*.c` files here (and the files `mpm/include/mpm/rfdc/xrfdc*.h`) are derived from files in the `xilinx-v2019.1` tag of [embeddedsw](https://github.com/Xilinx/embeddedsw/), with the following changes:
* Include paths
* `Xil_Assert` macros in `xrfdc.h` were replaced with custom functionality.
 * See `patches/xrfdc.h.patch` for the patch applied to that file.
* Call `closedir` in `xrfdc_sinit.c`
 * See `patches/xrfdc_sinic.c.patch` for the patch
