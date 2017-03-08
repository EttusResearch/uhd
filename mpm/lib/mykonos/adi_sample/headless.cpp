/**
 * \file headless.c
 *
 * \brief Contains example code for user integration with their application
 *
 * All data structures required for operation have been initialized with values which reflect
 * these settings:
 *
 * Device Clock:
 * 125MHz
 *
 * Profiles:
 * Rx 100MHz, IQrate 125MSPS, Dec5
 * Tx 20/100MHz, IQrate 125MSPS, Dec5
 * ORX 100MHz, IQrate 125MSPS, Dec5
 * SRx 20MHz, IQrate 31.25MSPS, Dec5
 *
 */

#include <stdlib.h>
#include <iostream>
#include "headless.h"
#include "../adi/t_mykonos.h"
#include "../adi/mykonos.h"
#include "../adi/mykonos_gpio.h"
#include "../config/mykonos_default_config.h"
//#include "mykonos_static_config.h"

#include <functional>

// dumb function to make the error reporting reusable within this file
void call_mykonos_api(std::function<mykonosErr_t()> f)
{
    auto error = f();
    if (error != MYKONOS_ERR_OK)
    {
        // TODO: make this an exception and stop normal processing
        // in the mean time, print an error and continue happily on
        std::cout << getMykonosErrorMessage(error) << std::endl;
    }
}

int headlessinit(mykonosDevice_t *mykDevice)
{
	uint32_t initCalMask = TX_BB_FILTER | ADC_TUNER | TIA_3DB_CORNER | DC_OFFSET |
	TX_ATTENUATION_DELAY | RX_GAIN_DELAY | FLASH_CAL |
	PATH_DELAY | TX_LO_LEAKAGE_INTERNAL | TX_QEC_INIT |
	LOOPBACK_RX_LO_DELAY | LOOPBACK_RX_RX_QEC_INIT |
	RX_LO_DELAY | RX_QEC_INIT;

	uint32_t trackingCalMask = 	TRACK_RX1_QEC |
	TRACK_RX2_QEC |
	TRACK_TX1_QEC |
	TRACK_TX2_QEC |
	TRACK_ORX1_QEC|
	TRACK_ORX2_QEC;

	/*** < Action: Insert System Clock(s) Initialization Code Here          > ***/

	/*** < Action: Insert BBIC Initialization Code Here                     > ***/

	/*************************************************************************/
	/*****                Mykonos Initialization Sequence                *****/
	/*************************************************************************/

    /*** < Action: Toggle RESETB pin on Mykonos device > ***/
    call_mykonos_api(std::bind(MYKONOS_resetDevice, mykDevice));
    call_mykonos_api(std::bind(MYKONOS_initialize, mykDevice));

	/*************************************************************************/
	/*****                Mykonos CLKPLL Status Check                    *****/
	/*************************************************************************/

    // change logic to timeout/wait for PLL lock
    {
        uint8_t pllLockStatus = 0;
        call_mykonos_api(std::bind(MYKONOS_checkPllsLockStatus, mykDevice, &pllLockStatus));

        if (pllLockStatus & 0x01)
        {
            /*** < User: code here for actions once CLKPLL locked  > ***/
        }
        else
        {
            /*** < User: code here here for actions since CLKPLL not locked
             * ensure lock before proceeding - > ***/
        }
    }

    // Multichip sync was here

	/*************************************************************************/
	/*****                Mykonos Load ARM file                          *****/
	/*************************************************************************/

    // deleted check for PLL lock here, do not advance until PLL is locked

    call_mykonos_api(std::bind(MYKONOS_initArm, mykDevice));
    {
        // TODO: Add code for loading ARM binary here
        uint8_t binary[98304] = { 0 };
        uint32_t count = sizeof(binary);
        call_mykonos_api(std::bind(MYKONOS_loadArmFromBinary, mykDevice, &binary[0], count));
    }

	/*************************************************************************/
	/*****                Mykonos Set RF PLL Frequencies                 *****/
	/*************************************************************************/

    call_mykonos_api(std::bind(MYKONOS_setRfPllFrequency, mykDevice, RX_PLL, mykDevice->rx->rxPllLoFrequency_Hz));
    call_mykonos_api(std::bind(MYKONOS_setRfPllFrequency, mykDevice, TX_PLL, mykDevice->tx->txPllLoFrequency_Hz));
    call_mykonos_api(std::bind(MYKONOS_setRfPllFrequency, mykDevice, SNIFFER_PLL, mykDevice->obsRx->snifferPllLoFrequency_Hz));

	/*** < Action: wait 200ms for PLLs to lock > ***/

    // change logic to wait for rest of PLLs to lock
    {
        uint8_t pllLockStatus = 0;
        call_mykonos_api(std::bind(MYKONOS_checkPllsLockStatus, mykDevice, &pllLockStatus));

        if ((pllLockStatus & 0x0F) == 0x0F)
        {
            /*** < Info: All PLLs locked > ***/
        }
        else
        {
            /*** < Info: PLLs not locked  > ***/
            /*** < Action: Ensure lock before proceeding - User code here > ***/
        }
    }

    // GPIO Ctrl set up was here

	/*************************************************************************/
	/*****                Mykonos Set manual gains values                *****/
	/*************************************************************************/

    call_mykonos_api(std::bind(MYKONOS_setRx1ManualGain, mykDevice, 255));
    call_mykonos_api(std::bind(MYKONOS_setRx2ManualGain, mykDevice, 255));

    // setting gain of obs and sniffer channels was here

	/*************************************************************************/
	/*****                Mykonos Initialize attenuations                *****/
	/*************************************************************************/

    call_mykonos_api(std::bind(MYKONOS_setTx1Attenuation, mykDevice, 0));
    call_mykonos_api(std::bind(MYKONOS_setTx2Attenuation, mykDevice, 0));

	/*************************************************************************/
	/*****           Mykonos ARM Initialization Calibrations             *****/
	/*************************************************************************/

    call_mykonos_api(std::bind(MYKONOS_runInitCals, mykDevice, (initCalMask & ~TX_LO_LEAKAGE_EXTERNAL)));

    {
        uint8_t errorFlag = 0;
        uint8_t errorCode = 0;
        call_mykonos_api(std::bind(MYKONOS_waitInitCals, mykDevice, 60000, &errorFlag, &errorCode));

        if ((errorFlag != 0) || (errorCode != 0))
        {
            mykonosInitCalStatus_t initCalStatus = { 0 };
            call_mykonos_api(std::bind(MYKONOS_getInitCalStatus, mykDevice, &initCalStatus));

            /*** < Info: abort init cals > ***/
            uint32_t initCalsCompleted = 0;
            call_mykonos_api(std::bind(MYKONOS_abortInitCals, mykDevice, &initCalsCompleted));
            if (initCalsCompleted)
            {
                /*** < Info: which calls had completed, per the mask > ***/
            }

            uint16_t errorWord = 0;
            uint16_t statusWord = 0;
            call_mykonos_api(std::bind(MYKONOS_readArmCmdStatus, mykDevice, &errorWord, &statusWord));

            uint8_t status = 0;
            call_mykonos_api(std::bind(MYKONOS_readArmCmdStatusByte, mykDevice, 2, &status));

            if (status != 0)
            {
                /*** < Info: Arm Mailbox Status Error errorWord > ***/
                /*** < Info: Pending Flag per opcode statusWord, this follows the mask > ***/
            }
        }
        else
        {
            /*** < Info: Calibrations completed successfully  > ***/
        }
    }

	/*************************************************************************/
	/*****  Mykonos ARM Initialization External LOL Calibrations with PA *****/
	/*************************************************************************/
	/*** < Action: Please ensure PA is enabled operational at this time > ***/
	if (initCalMask & TX_LO_LEAKAGE_EXTERNAL)
	{
        call_mykonos_api(std::bind(MYKONOS_runInitCals, mykDevice, TX_LO_LEAKAGE_EXTERNAL));

        uint8_t errorFlag = 0;
        uint8_t errorCode = 0;
        call_mykonos_api(std::bind(MYKONOS_waitInitCals, mykDevice, 60000, &errorFlag, &errorCode));

		if ((errorFlag != 0) || (errorCode != 0))
		{
            mykonosInitCalStatus_t initCalStatus = { 0 };
            call_mykonos_api(std::bind(MYKONOS_getInitCalStatus, mykDevice, &initCalStatus));

			/*** < Info: abort init cals > ***/
            uint32_t initCalsCompleted = 0;
            call_mykonos_api(std::bind(MYKONOS_abortInitCals, mykDevice, &initCalsCompleted));
			if (initCalsCompleted)
			{
				/*** < Info: which calls had completed, per the mask > ***/
			}

            uint16_t errorWord = 0;
            uint16_t statusWord = 0;
            call_mykonos_api(std::bind(MYKONOS_readArmCmdStatus, mykDevice, &errorWord, &statusWord));

            uint8_t status = 0;
            call_mykonos_api(std::bind(MYKONOS_readArmCmdStatusByte, mykDevice, 2, &status));
			if (status != 0)
			{
				/*** < Info: Arm Mailbox Status Error errorWord > ***/
				/*** < Info: Pending Flag per opcode statusWord, this follows the mask > ***/
			}
		}
		else
		{
			/*** < Info: Calibrations completed successfully  > ***/
		}
	}

	/*************************************************************************/
	/*****             SYSTEM JESD bring up procedure                    *****/
	/*************************************************************************/
	/*** < Action: Make sure SYSREF is stopped/disabled > ***/
	/*** < Action: Make sure BBIC JESD is reset and ready to recieve CGS chars> ***/

    call_mykonos_api(std::bind(MYKONOS_enableSysrefToRxFramer, mykDevice, 1));

	/*** < Info: Mykonos is waiting for sysref in order to start
	 * transmitting CGS from the RxFramer> ***/

    call_mykonos_api(std::bind(MYKONOS_enableSysrefToObsRxFramer, mykDevice, 1));

	/*** < Info: Mykonos is waiting for sysref in order to start
	 * transmitting CGS from the ObsRxFramer> ***/

	/*** < User: Make sure SYSREF is stopped/disabled > ***/
    call_mykonos_api(std::bind(MYKONOS_enableSysrefToDeframer, mykDevice, 0));

    call_mykonos_api(std::bind(MYKONOS_resetDeframer, mykDevice));
    call_mykonos_api(std::bind(MYKONOS_enableSysrefToDeframer, mykDevice, 1));

	/*** < User: make sure BBIC JESD framer is actively transmitting CGS> ***/

	/*************************************************************************/
	/*****            Enable SYSREF to Mykonos and BBIC                  *****/
	/*************************************************************************/
	/*** < Action: Sends SYSREF Here > ***/

	/*** < Info: Mykonos is actively transmitting CGS from the RxFramer> ***/

	/*** < Info: Mykonos is actively transmitting CGS from the ObsRxFramer> ***/

	/*** < Action: Insert User: BBIC JESD Sync Verification Code Here > ***/

	/*************************************************************************/
	/*****               Check Mykonos Framer Status                     *****/
	/*************************************************************************/
    {
        uint8_t framerStatus = 0;
        call_mykonos_api(std::bind(MYKONOS_readRxFramerStatus, mykDevice, &framerStatus));
    }

    {
        uint8_t obsFramerStatus = 0;
        call_mykonos_api(std::bind(MYKONOS_readOrxFramerStatus, mykDevice, &obsFramerStatus));
    }

	/*************************************************************************/
	/*****               Check Mykonos Deframer Status                   *****/
	/*************************************************************************/

    {
        uint8_t deframerStatus = 0;
        call_mykonos_api(std::bind(MYKONOS_readDeframerStatus, mykDevice, &deframerStatus));
    }
	/*** < Action: When links have been verified, proceed > ***/

	/*************************************************************************/
	/*****           Mykonos enable tracking calibrations                *****/
	/*************************************************************************/

    call_mykonos_api(std::bind(MYKONOS_enableTrackingCals, mykDevice, trackingCalMask));

	/*** < Info: Allow Rx1/2 QEC tracking and Tx1/2 QEC tracking to run when in the radioOn state
	     *  Tx calibrations will only run if radioOn and the obsRx path is set to OBS_INTERNAL_CALS > ***/

	/*** < Info: Function to turn radio on, Enables transmitters and receivers
	 * that were setup during MYKONOS_initialize() > ***/

    call_mykonos_api(std::bind(MYKONOS_radioOn, mykDevice));
    /*** < Info: Allow TxQEC to run when User: is not actively using ORx receive path > ***/
    call_mykonos_api(std::bind(MYKONOS_setObsRxPathSource, mykDevice, OBS_RXOFF));
    call_mykonos_api(std::bind(MYKONOS_setObsRxPathSource, mykDevice, OBS_INTERNALCALS));

	return 0;
}
