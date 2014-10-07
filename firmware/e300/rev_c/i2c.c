#include "config.h"
#include "i2c.h"

#include <util/delay.h>

#include "io.h"
#include "debug.h"

/*
	- Reset bus on failure (lack of ACK, etc)
	- Clock stretching
	- In pull-up mode, much code was commented out to ever avoid driving the bus (for a fleeting moment) as this was visible on the scope as short peaks (instead the line will briefly go Hi Z).
*/

volatile bool _i2c_disable_ack_check = false;

// FIXME: Follow magic numbers should be in a struct that is passed into each function

#define I2C_DEFAULT_RETRY_DELAY     1   // us MAGIC
#define I2C_DEFAULT_MAX_ACK_RETRIES 10  // * I2C_DEFAULT_RETRY_DELAY us

#define I2C_DEFAULT_BUS_WAIT		10	// us MAGIC
#define I2C_DEFAULT_MAX_BUS_RETRIES	10

#define I2C_DEFAULT_SCL_LOW_PERIOD  2   // 1.3 us
#define I2C_DEFAULT_SCL_HIGH_PERIOD 1   // 0.6 us
#define I2C_DEFAULT_BUS_FREE_TIME   2   // 1.3 us
#define I2C_DEFAULT_STOP_TIME       1   // 0.6 us

#define I2C_DELAY	_delay_us	// _delay_ms

static bool _i2c_start_ex(io_pin_t sda, io_pin_t scl, bool pull_up)
{
	// Assumes: SDA/SCL are both inputs

	uint8_t retries = I2C_DEFAULT_MAX_BUS_RETRIES;
	while ((io_test_pin(sda) == false) || (io_test_pin(scl) == false))
	{
		I2C_DELAY(I2C_DEFAULT_BUS_WAIT);
		if (retries-- == 0)
		{
debug_log("I2C:S1");
			return false;
		}			
	}
	
	// START condition
//	if (pull_up == false)
		io_clear_pin(sda);	// Set LOW before switching to output
	io_output_pin(sda);
//	if (pull_up)
//		io_clear_pin(sda);
	I2C_DELAY(I2C_DEFAULT_SCL_LOW_PERIOD);  // Thd, sta
	
	retries = I2C_DEFAULT_MAX_BUS_RETRIES;
	while (io_test_pin(scl) == false)	// SCL should remain high
	{
		I2C_DELAY(I2C_DEFAULT_BUS_WAIT);
		if (retries-- == 0)
		{
			io_input_pin(sda);
debug_log_ex("I2C:S2", false);
debug_log_hex(scl);
			return false;
		}			
	}

//	if (pull_up == false)
		io_clear_pin(scl);
	io_output_pin(scl);
//	if (pull_up)
//		io_clear_pin(scl);
	I2C_DELAY(I2C_DEFAULT_SCL_LOW_PERIOD / 2);   // MAGIC

	return true;
}

static bool _i2c_stop_ex(io_pin_t sda, io_pin_t scl, bool pull_up)
{
	// Assumes:
	//	SCL is output & LOW
	//	SDA is input (Hi-Z, or pull-up enabled)
	
	// Assuming pull-up already enabled
	//if (pull_up)
	//	io_set_pin(sda);
	
	bool result = true;
	
	// SDA should be HIGH after ACK has been clocked away
//	bool skip_drive = false;
	uint8_t retries = 0;
	while (io_test_pin(sda) == false)
	{
		if (retries == I2C_DEFAULT_MAX_ACK_RETRIES)
		{
			debug_log_ex("I2C:STP ", false);
			debug_log_hex(sda);
			debug_blink_rev(4);
			
//			skip_drive = true;
			result = false;
			break;	// SDA is being held low?!
		}

		++retries;
		I2C_DELAY(I2C_DEFAULT_RETRY_DELAY);
	}
	
	// STOP condition
//	if ((pull_up == false) || (skip_drive))
		io_clear_pin(sda);	// Don't tri-state if internal pull-up is used
//	//else
//	// Pin will now be driven, but having checked SDA is HIGH above means slave's SDA should be Open Collector (i.e. it won't blow up)
	io_output_pin(sda);	// Drive LOW
//	if (pull_up)
//		io_clear_pin(sda);

	///////////////////////////////////
	
//	if (pull_up)
//		io_set_pin(scl);	// Don't tri-state if internal pull-up is used. Line will be driven, but assuming this is the only master on the clock line (i.e. no one else will pull it low).
	io_input_pin(scl);
	if (pull_up)
		io_set_pin(scl);
	I2C_DELAY(I2C_DEFAULT_STOP_TIME);
	
	///////////////////////////////////

//	if ((pull_up) && (skip_drive == false))
//		io_set_pin(sda);	// Don't tri-state if internal pull-up is used
	io_input_pin(sda);
//	if ((pull_up) && (skip_drive))
		io_set_pin(sda);
	I2C_DELAY(I2C_DEFAULT_BUS_FREE_TIME);
	
	return result;
}
/*
static void _i2c_stop(io_pin_t sda, io_pin_t scl)
{
	_i2c_stop_ex(sda, scl, false);
}
*//*
static void _i2c_abort_safe_ex(io_pin_t pin, bool pull_up)
{
	if (io_is_output(pin))
	{
		if (io_is_pin_set(pin))	// This is bad - hope no slave is pulling down the line
		{
			io_input_pin(pin);	// Pull-up already enabled
			
			if (pull_up == false)
				io_clear_pin(pin);	// Doing this after changing direction ensures the line is not brought down
		}
		else	// Currently pulling line down
		{
			io_input_pin(pin);	// Hi-Z
			
			if (pull_up)	// There will be a moment where the line will float (better than driving the line though...)
			{
				io_set_pin(pin);
			}
		}
	}
	else	// Already an input
	{
		if (pull_up)
		{
			io_set_pin(pin);	// Enable pull-ups
		}
		else
		{
			io_clear_pin(pin);	// Disable pull-ups
		}
	}
	
	// Normally: pin will be Hi-Z input
	// With internal pull-up: pin will be input with pull-up enabled
}
*/
static void _i2c_abort_safe(io_pin_t pin, bool pull_up)
{
	if (pull_up == false)
		io_clear_pin(pin);	// Should never be output/HIGH, could be input/<was outputting HIGH> so disable pull-ups
	
	io_input_pin(pin);
	
	if (pull_up)
		io_set_pin(pin);	// Enable pull-up
}

static void _i2c_abort_ex(io_pin_t sda, io_pin_t scl, bool pull_up)
{
/*	if (pull_up == false)
	{
		io_clear_pin(sda);
		io_clear_pin(scl);
	}
	
	io_input_pin(scl);
	io_input_pin(sda);
	
	if (pull_up)
	{
		io_set_pin(sda);
		io_set_pin(scl);
	}
*/
	_i2c_abort_safe(scl, pull_up);
	_i2c_abort_safe(sda, pull_up);

	//_i2c_abort_safe_ex(scl, pull_up);
	//_i2c_abort_safe_ex(sda, pull_up);
}
/*
static void _i2c_abort(io_pin_t sda, io_pin_t scl)
{
	_i2c_abort_ex(sda, scl, false);
}
*/
static bool _i2c_write_byte_ex(io_pin_t sda, io_pin_t scl, uint8_t value, bool pull_up)
{
    // Assumes:
    //  SDA output is LOW
    //  SCL output is LOW

    for (uint8_t i = 0; i < 8; ++i)
    {
		bool b = ((value & (0x01 << (7 - i))) != 0x00);	// MSB first
		
		if (b)
		{
			if (pull_up)
			{
//				io_set_pin(sda);	// This is bad (will drive line for a moment), but more stable than letting line float
				io_input_pin(sda);
				io_set_pin(sda);
			}				
			else
				io_input_pin(sda);	// Release HIGH
			
			if (io_test_pin(sda) == false)
			{
				debug_log("I2C:WR ");
				debug_log_hex(sda);
				debug_blink_rev(1);
				return false;
			}			
		}			
		else
		{
			if (pull_up)
			{
//				if (io_is_output(sda))
					io_clear_pin(sda);
//				else
//				{
					io_output_pin(sda);	// [This is bad (will drive line for a moment), but more stable than letting line float]
//					io_clear_pin(sda);
//				}
			}
			else
			{
				io_enable_pin(sda, false);
				io_output_pin(sda);	// Drive LOW
			}				
		}
		
		///////////////////////////////

        io_input_pin(scl);	// Release HIGH
		if (pull_up)
			io_set_pin(scl);
        I2C_DELAY(I2C_DEFAULT_SCL_HIGH_PERIOD);
#ifdef I2C_ALLOW_CLOCK_STRETCH
		uint8_t retries = I2C_DEFAULT_MAX_BUS_RETRIES;
		while (io_test_pin(scl) == false)	// Clock stretch requested?
		{
			I2C_DELAY(I2C_DEFAULT_BUS_WAIT);
			if (--retries == 0)
			{
				io_input_pin(sda);	// Release HIGH
				if (pull_up)
					io_set_pin(sda);
				
				debug_log_ex("I2C:STRTCH ", false);
				debug_log_hex(scl);
				debug_blink_rev(2);
				return false;
			}
		}
#endif // I2C_ALLOW_CLOCK_STRETCH
		if (pull_up)
			io_clear_pin(scl);
        io_output_pin(scl);	// Drive LOW
        I2C_DELAY(I2C_DEFAULT_SCL_LOW_PERIOD);
    }

    io_input_pin(sda);	// Release HIGH
	if (pull_up)
		io_set_pin(sda);	// Assuming letting line float won't confuse slave when pulling line LOW for ACK
    I2C_DELAY(I2C_DEFAULT_SCL_HIGH_PERIOD);

    uint8_t retries = 0;
    while ((_i2c_disable_ack_check == false) && (io_test_pin(sda)))
    {
        if (retries == I2C_DEFAULT_MAX_ACK_RETRIES)
		{
			debug_log_ex("I2C:ACK ", false);
			debug_log_hex_ex(sda, false);
			debug_log_hex(value);
			debug_blink_rev(3);
            return false;	// Will abort and not release bus - done by caller
		}

        ++retries;
        I2C_DELAY(I2C_DEFAULT_RETRY_DELAY);
    }

    // Clock away acknowledge
//	if (pull_up)
//		io_set_pin(scl);
    io_input_pin(scl);	// Release HIGH
	if (pull_up)
		io_set_pin(scl);
    I2C_DELAY(I2C_DEFAULT_SCL_HIGH_PERIOD);

	if (pull_up)
		io_clear_pin(scl);
    io_output_pin(scl);	// Drive LOW
//	if (pull_up)
//		io_clear_pin(scl);
    I2C_DELAY(I2C_DEFAULT_SCL_LOW_PERIOD);

    return true;
}

static bool _i2c_read_byte_ex(io_pin_t sda, io_pin_t scl, uint8_t* value, bool pull_up)
{
    // Assumes:
    //  SDA output is LOW
    //  SCL output is LOW
	
	io_input_pin(sda);
	if (pull_up)
		io_set_pin(sda);	// OK to leave line floating for a moment (better not to drive as slave will be pulling it to ground)

    (*value) = 0x00;

    for (uint8_t i = 0; i < 8; ++i)
    {
//		if (pull_up)
//			io_set_pin(scl);	// [Not ideal with pull-up]
        io_input_pin(scl);	// Release HIGH
		if (pull_up)
			io_set_pin(scl);
        I2C_DELAY(I2C_DEFAULT_SCL_HIGH_PERIOD);
#ifdef I2C_ALLOW_CLOCK_STRETCH
		uint8_t retries = I2C_DEFAULT_MAX_BUS_RETRIES;
		while (io_test_pin(scl) == false)	// Clock stretch requested?
		{
			I2C_DELAY(I2C_DEFAULT_BUS_WAIT);
			if (--retries == 0)
			{
				debug_log_ex("I2C:R ");
				debug_log_hex(scl);
				debug_blink_rev(5);
				return false;
			}				
		}
#endif // I2C_ALLOW_CLOCK_STRETCH
        (*value) |= ((io_test_pin(sda) ? 0x1 : 0x0) << (7 - i));   // MSB first

		if (pull_up)
			io_clear_pin(scl);
        io_output_pin(scl);	// Drive LOW (not ideal with pull-up)
//		if (pull_up)
//			io_clear_pin(scl);
        I2C_DELAY(I2C_DEFAULT_SCL_LOW_PERIOD);
    }

    // Not necessary to ACK since it's only this one byte

    return true;
}

bool i2c_read2_ex(io_pin_t sda, io_pin_t scl, uint8_t addr, uint8_t subaddr, uint8_t* value, bool pull_up)
{
	if (_i2c_start_ex(sda, scl, pull_up) == false)
		return false;

	if (_i2c_write_byte_ex(sda, scl, addr & ~0x01, pull_up) == false)
	{
#ifdef I2C_EXTRA_DEBUGGING
		//debug_log_ex("R21:", false);
		debug_log("R21");
		//debug_log_hex(addr);
#endif // I2C_EXTRA_DEBUGGING
		goto i2c_read2_fail;
	}

	if (_i2c_write_byte_ex(sda, scl, subaddr, pull_up) == false)
	{
#ifdef I2C_EXTRA_DEBUGGING
		//debug_log_ex("R22:", false);
		debug_log("R22");
		//debug_log_hex(subaddr);
#endif // I2C_EXTRA_DEBUGGING
		goto i2c_read2_fail;
	}
	
	io_input_pin(scl);
	if (pull_up)
		io_set_pin(scl);
	I2C_DELAY(I2C_DEFAULT_BUS_WAIT);
	
	if (_i2c_start_ex(sda, scl, pull_up) == false)
	{
		return false;
	}
	
	if (_i2c_write_byte_ex(sda, scl, addr | 0x01, pull_up) == false)
	{
#ifdef I2C_EXTRA_DEBUGGING
		//debug_log_ex("R23:", false);
		debug_log("R23");
		//debug_log_hex(addr);
#endif // I2C_EXTRA_DEBUGGING
		goto i2c_read2_fail;
	}

	if (_i2c_read_byte_ex(sda, scl, value, pull_up) == false)
	{
#ifdef I2C_EXTRA_DEBUGGING
		//debug_log_ex("R24:", false);
		debug_log("R24");
		//debug_log_hex(*value);
#endif // I2C_EXTRA_DEBUGGING
		goto i2c_read2_fail;
	}
	
	if (_i2c_stop_ex(sda, scl, pull_up) == false)
	{
#ifdef I2C_EXTRA_DEBUGGING
		debug_log("R25");
#endif // I2C_EXTRA_DEBUGGING
	}

	return true;
i2c_read2_fail:
	_i2c_abort_ex(sda, scl, pull_up);
	return false;
}

bool i2c_write_ex(io_pin_t sda, io_pin_t scl, uint8_t addr, uint8_t subaddr, uint8_t value, bool pull_up)
{
	if (_i2c_start_ex(sda, scl, pull_up) == false)
		return false;

    if (_i2c_write_byte_ex(sda, scl, addr, pull_up) == false)
        goto i2c_write_fail;

    if (_i2c_write_byte_ex(sda, scl, subaddr, pull_up) == false)
        goto i2c_write_fail;

    if (_i2c_write_byte_ex(sda, scl, value, pull_up) == false)
        goto i2c_write_fail;

    _i2c_stop_ex(sda, scl, pull_up);

    return true;
i2c_write_fail:
	_i2c_abort_ex(sda, scl, pull_up);
	return false;
}

bool i2c_write(io_pin_t sda, io_pin_t scl, uint8_t addr, uint8_t subaddr, uint8_t value)
{
	return i2c_write_ex(sda, scl, addr, subaddr, value, false);
}

bool i2c_read_ex(io_pin_t sda, io_pin_t scl, uint8_t addr, uint8_t subaddr, uint8_t* value, bool pull_up)
{
    if (_i2c_start_ex(sda, scl, pull_up) == false)
		return false;

    if (_i2c_write_byte_ex(sda, scl, addr, pull_up) == false)
        goto i2c_read_fail;

    if (_i2c_write_byte_ex(sda, scl, subaddr, pull_up) == false)
        goto i2c_read_fail;

    if (_i2c_read_byte_ex(sda, scl, value, pull_up) == false)
        goto i2c_read_fail;

    _i2c_stop_ex(sda, scl, pull_up);

    return true;
i2c_read_fail:
	_i2c_abort_ex(sda, scl, pull_up);
	return false;
}

bool i2c_read(io_pin_t sda, io_pin_t scl, uint8_t addr, uint8_t subaddr, uint8_t* value)
{
	return i2c_read_ex(sda, scl, addr, subaddr, value, false);
}

void i2c_init_ex(io_pin_t sda, io_pin_t scl, bool pull_up)
{
	_i2c_abort_ex(sda, scl, pull_up);
}

void i2c_init(io_pin_t sda, io_pin_t scl)
{
	i2c_init_ex(sda, scl, false);
}
