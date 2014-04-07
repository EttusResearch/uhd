//
// Copyright 2013-2014 Ettus Research LLC
//

#include "cyu3error.h"
#include "cyu3i2c.h"
#include "cyu3spi.h"
#include "cyu3os.h"
#include "cyu3pib.h"
#include "cyu3system.h"
#include "cyu3usb.h"
#include "cyu3utils.h"
#include "pib_regs.h"
#include "b200_vrq.h"
#include <stdint.h>

#define true CyTrue
#define false CyFalse

typedef CyBool_t bool;

/* Fast sqrt() - precision can be improved by increasing 
 * the number of iterations 
 */
float ad9361_sqrt(const float number)
{
    uint32_t i;
    float x2, y;

    x2 = number * 0.5F;
    y = number;
    i = *(uint32_t *) &y;
    i = 0x5f3759df - ( i >> 1 );
    y = *(float *) &i;
    y = y * (1.5F - (x2 * y * y));

    return number * y;
}

void ad9361_msleep(const unsigned millis)
{
    CyU3PThreadSleep(millis);
}

#define AD9361_MIN(a, b) CY_U3P_MIN(a, b)
#define AD9361_MAX(a, b) CY_U3P_MAX(a, b)

#define AD9361_CEIL_INT(a)  ((int)(a+1)) 
#define AD9361_FLOOR_INT(a) ((int)(a))

#define AD9361_CLOCKING_MODE 0

#define AD9361_RX_BAND_EDGE0 2.2e9
#define AD9361_RX_BAND_EDGE1 4e9
#define AD9361_TX_BAND_EDGE 2.5e9

#include "../ad9361/lib/ad9361_impl.c"
