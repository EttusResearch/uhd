/*
 * debug.h
 */ 

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdint.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

#include "io.h"

#ifdef DEBUG
#define DEBUG_INLINE
#define DEBUG_NOOP	;
#define LED_ON		false
#define LED_OFF		true
#else
#define DEBUG_INLINE inline
#define DEBUG_NOOP	{}
#define LED_ON		true
#define LED_OFF		false
#endif // DEBUG

//#define DEBUG_VOID
#define DEBUG_SAFETY

#ifdef DEBUG_VOID

//#define debug_init	(void)
#define debug_set	(void)
#define debug_blink	(void)
#define debug_blink_rev	(void)
#define debug_blink2 (void)
#define debug_wait	(void)

#else

//DEBUG_INLINE void debug_init(void) DEBUG_NOOP
DEBUG_INLINE void debug_set(io_pin_t pin, bool enable) DEBUG_NOOP
DEBUG_INLINE void debug_blink(uint8_t count) DEBUG_NOOP
//DEBUG_INLINE void debug_blink_rev(uint8_t count) DEBUG_NOOP
void debug_blink_rev(uint8_t count);
DEBUG_INLINE void debug_blink2(uint8_t count) DEBUG_NOOP
DEBUG_INLINE void debug_wait(void) DEBUG_NOOP

#endif // DEBUG_VOID

#if defined(DEBUG) && !defined(ENABLE_SERIAL)
#define ENABLE_SERIAL
#endif // DEBUG && !ENABLE_SERIAL

/*DEBUG_INLINE */void debug_init(void)/* DEBUG_NOOP*/;

#ifdef ENABLE_SERIAL

void debug_log_ex_P(const char* message, bool new_line);
void debug_log_hex_ex(uint8_t n, bool new_line);
void debug_log_byte_ex(uint8_t n, bool new_line);
void _debug_log_ex(const char* message, bool new_line);

// Prototypes to silence avr-gcc
inline void debug_log_P(const char* message);
inline void debug_log_hex(uint8_t n);
inline void debug_log_byte(uint8_t n);
inline void _debug_log(const char* message);

inline void debug_log_P(const char* message) { debug_log_ex_P(message, true); }
inline void debug_log_hex(uint8_t n) { debug_log_hex_ex(n, true); }
inline void debug_log_byte(uint8_t n) { debug_log_byte_ex(n, true); }
inline void _debug_log(const char* message) { _debug_log_ex(message, true); }

#else

inline void debug_log_ex_P		(const char* message, bool new_line) {};
inline void debug_log_hex_ex	(uint8_t n, bool new_line) {};
inline void debug_log_byte_ex	(uint8_t n, bool new_line) {};
inline void _debug_log_ex		(const char* message, bool new_line) {};

#define debug_log_P			(void)
#define debug_log_hex		(void)
#define debug_log_byte		(void)
#define _debug_log			(void)
#endif // ENABLE_SERIAL

#define debug_log(x)		debug_log_P(PSTR(x))
#define debug_log_ex(x,nl)	debug_log_ex_P(PSTR(x), nl)

#endif /* DEBUG_H_ */
