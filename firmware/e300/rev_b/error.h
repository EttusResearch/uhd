/*
 * error.h
 *
 * Copyright 2015 National Instruments Corp
 */ 


#ifndef ERROR_H_
#define ERROR_H_

enum ErrorBlinkCount	// Lower number = higher priority
{
	BlinkError_None,
	// Low power/battery
	BlinkError_LowVoltage,
	BlinkError_LTC3675_UnderVoltage = BlinkError_LowVoltage,
	BlinkError_LTC4155_UnderVoltage = BlinkError_LowVoltage,	// FIXME: This does not work when checking status
	// Should match power boot steps
	BlinkError_FPGA_Power,
	BlinkError_DRAM_Power,
	BlinkError_1_8V_Peripherals_Power,
	BlinkError_3_3V_Peripherals_Power,
	BlinkError_TX_Power,
	// LTC3675
	BlinkError_LTC3675_OverTemperature,
	// LTC4155
	BlinkError_LTC4155_BadCell
};

#endif /* ERROR_H_ */
