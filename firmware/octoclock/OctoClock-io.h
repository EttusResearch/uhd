/*
 * Copyright 2009 Ettus Research LLC
 */

#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stdbool.h>


typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;


typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;


typedef float               f32;
typedef double              f64;
typedef long double         f128;

typedef int                 error_indicator; /* A type for functions, usually... */
typedef unsigned int        boolean;         /* "natural" type */

typedef char                c8;              /* 8-bit character */
typedef unsigned char       uc8;             /* 8-bit character, unsigned*/
/* it is sometimes useful to*/
/* make the distinction*/

#ifdef FALSE
#undef FALSE
#endif
#define FALSE (0)

#ifdef TRUE
#undef TRUE
#endif
#define TRUE (!FALSE)

#define is    :{
#define esac  break;}


// This other crap can be deleted, below, between the #if 0 and #endif inclusive

#if 0

#define IO_PX(port, pin) ((uint8_t)(((port - 'A') << 4) + pin))
#define IO_PA(pin) IO_PX('A', pin)
#define IO_PB(pin) IO_PX('B', pin)
#define IO_PC(pin) IO_PX('C', pin)
#define IO_PD(pin) IO_PX('D', pin)

typedef const uint8_t io_pin_t;

void io_output_pin(io_pin_t pin);
void io_input_pin(io_pin_t pin);
void io_set_pin(io_pin_t pin);
void io_clear_pin(io_pin_t pin);
bool io_test_pin(io_pin_t pin);

#endif  //if 0

#endif /* IO_H */
