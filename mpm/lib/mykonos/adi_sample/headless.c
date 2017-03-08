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
#include "headless.h"
#include "t_mykonos.h"
#include "mykonos.h"
#include "mykonos_gpio.h"
#include "mykonos_config.h"
//#include "mykonos_static_config.h"

/****< Action: Insert rest of required Includes Here >***/

int oldmain()
{
	const char* errorString;
	uint8_t mcsStatus = 0;
	uint8_t pllLockStatus = 0;
	uint8_t binary[98304] = {0};  /*** < Action: binary should contain ARM binary file as array  > ***/
	uint32_t count = sizeof(binary);
	uint8_t errorFlag = 0;
	uint8_t errorCode = 0;
	uint32_t initCalsCompleted = 0;
	uint16_t errorWord = 0;
	uint16_t statusWord = 0;
	uint8_t status = 0;
	mykonosInitCalStatus_t initCalStatus = {0};

	uint8_t deframerStatus = 0;
	uint8_t obsFramerStatus = 0;
	uint8_t framerStatus = 0;
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

	mykonosErr_t mykError = MYKONOS_ERR_OK;
	mykonosGpioErr_t mykGpioErr = MYKONOS_ERR_GPIO_OK;

	/* Allocating memory for the errorString */
	errorString = (const char*) malloc(sizeof(char) * 200);

	/*** < Action: Insert System Clock(s) Initialization Code Here          > ***/

	/*** < Action: Insert BBIC Initialization Code Here                     > ***/

	/*************************************************************************/
	/*****                Mykonos Initialization Sequence                *****/
	/*************************************************************************/

    // TODO: change to unique ptr
    mykonos_config config();
    mykonosDevice_t *mykDevice = &config.device;


	/*** < Action: Toggle RESETB pin on Mykonos device                   > ***/
	if ((mykError = MYKONOS_resetDevice(&mykDevice)) != MYKONOS_ERR_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
    }

	if ((mykError = MYKONOS_initialize(&mykDevice)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
	}

	/*************************************************************************/
	/*****                Mykonos CLKPLL Status Check                    *****/
	/*************************************************************************/
	if ((mykError = MYKONOS_checkPllsLockStatus(&mykDevice, &pllLockStatus)) != MYKONOS_ERR_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
    }

	if (pllLockStatus & 0x01)
	{
		/*** < User: code here for actions once CLKPLL locked  > ***/
	}
	else
	{
		/*** < User: code here here for actions since CLKPLL not locked
		 * ensure lock before proceeding - > ***/
	}

	/*************************************************************************/
	/*****                Mykonos Perform MultiChip Sync                 *****/
	/*************************************************************************/
	if ((mykError = MYKONOS_enableMultichipSync(&mykDevice, 1, &mcsStatus)) != MYKONOS_ERR_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
    }


	/*** < Action: minimum 3 SYSREF pulses from Clock Device has to be produced
	 * for MulticChip Sync > ***/

	/*************************************************************************/
	/*****                Mykonos Verify MultiChip Sync                 *****/
	/*************************************************************************/
	if ((mykError = MYKONOS_enableMultichipSync(&mykDevice, 0, &mcsStatus)) != MYKONOS_ERR_OK)
    {
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
    }

	if ((mcsStatus & 0x0B) == 0x0B)
	{
		/*** < Info: MCS successful  > ***/
	    /*** < Action: extra User code   > ***/
	}
	else
	{
	    /*** < Info: MCS failed  > ***/
		/*** < Action: ensure MCS before proceeding  > ***/
	}

	/*************************************************************************/
	/*****                Mykonos Load ARM file                          *****/
	/*************************************************************************/
	if (pllLockStatus & 0x01)
	{
		if ((mykError = MYKONOS_initArm(&mykDevice)) != MYKONOS_ERR_OK)
        {
            /*** < Info: errorString will contain log error string in order to debug failure > ***/
		    errorString = getMykonosErrorMessage(mykError);
        }

		/*** < Action: User must load ARM binary byte array into variable binary[98304] before calling next command > ***/
		if ((mykError = MYKONOS_loadArmFromBinary(&mykDevice, &binary[0], count)) != MYKONOS_ERR_OK)
		{
		    /*** < Info: errorString will contain log error string in order to debug why
		     *  ARM did not load properly - check binary and device settings  > ***/
		    /*** < Action: User code > ***/
		    errorString = getMykonosErrorMessage(mykError);
		}

	}
	else
	{
		/*** < Action: check settings for proper CLKPLL lock > ***/
	}

	/*************************************************************************/
	/*****                Mykonos Set RF PLL Frequencies                 *****/
	/*************************************************************************/
	if ((mykError = MYKONOS_setRfPllFrequency(&mykDevice, RX_PLL, mykDevice.rx->rxPllLoFrequency_Hz)) != MYKONOS_ERR_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
        errorString = getMykonosErrorMessage(mykError);
    }

	if ((mykError = MYKONOS_setRfPllFrequency(&mykDevice, TX_PLL, mykDevice.tx->txPllLoFrequency_Hz)) != MYKONOS_ERR_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
    }

	if ((mykError = MYKONOS_setRfPllFrequency(&mykDevice, SNIFFER_PLL, mykDevice.obsRx->snifferPllLoFrequency_Hz)) != MYKONOS_ERR_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
    }

	/*** < Action: wait 200ms for PLLs to lock > ***/

	if ((mykError = MYKONOS_checkPllsLockStatus(&mykDevice, &pllLockStatus)) != MYKONOS_ERR_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
    }

	if ((pllLockStatus & 0x0F) == 0x0F)
	{
	    /*** < Info: All PLLs locked > ***/
	}
	else
	{
	    /*** < Info: PLLs not locked  > ***/
	    /*** < Action: Ensure lock before proceeding - User code here > ***/
	}

	/*************************************************************************/
	/*****                Mykonos Set GPIOs                              *****/
	/*************************************************************************/
	if ((mykGpioErr = MYKONOS_setRx1GainCtrlPin(&mykDevice, 0, 0, 0, 0, 0)) != MYKONOS_ERR_GPIO_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getGpioMykonosErrorMessage(mykGpioErr);
    }

	if ((mykGpioErr = MYKONOS_setRx2GainCtrlPin(&mykDevice, 0, 0, 0, 0, 0)) != MYKONOS_ERR_GPIO_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getGpioMykonosErrorMessage(mykGpioErr);
    }

	if ((mykGpioErr = MYKONOS_setTx1AttenCtrlPin(&mykDevice, 0, 0, 0, 0, 0)) != MYKONOS_ERR_GPIO_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getGpioMykonosErrorMessage(mykGpioErr);
    }

	if ((mykGpioErr = MYKONOS_setTx2AttenCtrlPin(&mykDevice, 0, 0, 0, 0)) != MYKONOS_ERR_GPIO_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getGpioMykonosErrorMessage(mykGpioErr);
    }

	if ((mykGpioErr = MYKONOS_setupGpio(&mykDevice)) != MYKONOS_ERR_GPIO_OK)
    {
        /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getGpioMykonosErrorMessage(mykGpioErr);
    }

	/*************************************************************************/
	/*****                Mykonos Set manual gains values                *****/
	/*************************************************************************/
	if ((mykError = MYKONOS_setRx1ManualGain(&mykDevice, 255)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
	}

	if ((mykError = MYKONOS_setRx2ManualGain(&mykDevice, 255)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
	}

	if ((mykError = MYKONOS_setObsRxManualGain(&mykDevice, OBS_RX1_TXLO, 255)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
	}

	if ((mykError = MYKONOS_setObsRxManualGain(&mykDevice, OBS_RX2_TXLO, 255)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
	}

	if ((mykError = MYKONOS_setObsRxManualGain(&mykDevice, OBS_SNIFFER_A, 255)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
	}

	if ((mykError = MYKONOS_setObsRxManualGain(&mykDevice, OBS_SNIFFER_B, 255)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
	}
	if ((mykError = MYKONOS_setObsRxManualGain(&mykDevice, OBS_SNIFFER_C, 255)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
	}

	/*************************************************************************/
	/*****                Mykonos Initialize attenuations                *****/
	/*************************************************************************/
	if ((mykError = MYKONOS_setTx1Attenuation(&mykDevice, 0)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
	}

	if ((mykError = MYKONOS_setTx2Attenuation(&mykDevice, 0)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
	}

	/*************************************************************************/
	/*****           Mykonos ARM Initialization Calibrations             *****/
	/*************************************************************************/

    if ((mykError = MYKONOS_runInitCals(&mykDevice, (initCalMask & ~TX_LO_LEAKAGE_EXTERNAL))) != MYKONOS_ERR_OK)
	{
		/*** < Info: errorString will contain log error string in order to debug failure > ***/
		errorString = getMykonosErrorMessage(mykError);
	}

	if ((mykError = MYKONOS_waitInitCals(&mykDevice, 60000, &errorFlag, &errorCode)) != MYKONOS_ERR_OK)
	{
		/*** < Info: errorString will contain log error string in order to debug failure > ***/
		errorString = getMykonosErrorMessage(mykError);
	}

	if ((errorFlag != 0) || (errorCode != 0))
	{
		if ((mykError = MYKONOS_getInitCalStatus(&mykDevice, &initCalStatus)) != MYKONOS_ERR_OK)
		{
			/*** < Info: errorString will contain log error string in order to debug failure > ***/
			errorString = getMykonosErrorMessage(mykError);
		}

		/*** < Info: abort init cals > ***/
		if ((mykError = MYKONOS_abortInitCals(&mykDevice, &initCalsCompleted)) != MYKONOS_ERR_OK)
		{
			/*** < Info: errorString will contain log error string in order to debug failure > ***/
			errorString = getMykonosErrorMessage(mykError);
		}
		if (initCalsCompleted)
		{
			/*** < Info: which calls had completed, per the mask > ***/
		}

		if ((mykError = MYKONOS_readArmCmdStatus(&mykDevice, &errorWord, &statusWord)) != MYKONOS_ERR_OK)
		{
			/*** < Info: errorString will contain log error string in order to debug failure > ***/
			errorString = getMykonosErrorMessage(mykError);
		}

		if ((mykError = MYKONOS_readArmCmdStatusByte(&mykDevice, 2, &status)) != MYKONOS_ERR_OK)
		{
			/*** < Info: errorString will contain log error string in order to debug why  failed > ***/
			errorString = getMykonosErrorMessage(mykError);
		}
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

	/*************************************************************************/
	/*****  Mykonos ARM Initialization External LOL Calibrations with PA *****/
	/*************************************************************************/
	/*** < Action: Please ensure PA is enabled operational at this time > ***/
	if (initCalMask & TX_LO_LEAKAGE_EXTERNAL)
	{
		if ((mykError = MYKONOS_runInitCals(&mykDevice, TX_LO_LEAKAGE_EXTERNAL)) != MYKONOS_ERR_OK)
		{
			/*** < Info: errorString will contain log error string in order to debug failure > ***/
			errorString = getMykonosErrorMessage(mykError);
		}
		if ((mykError = MYKONOS_waitInitCals(&mykDevice, 60000, &errorFlag, &errorCode)) != MYKONOS_ERR_OK)
		{
			/*** < Info: errorString will contain log error string in order to debug failure > ***/
			errorString = getMykonosErrorMessage(mykError);
		}
		if ((errorFlag != 0) || (errorCode != 0))
		{
			if ((mykError = MYKONOS_getInitCalStatus(&mykDevice, &initCalStatus)) != MYKONOS_ERR_OK)
			{
				/*** < Info: errorString will contain log error string in order to debug failure > ***/
				errorString = getMykonosErrorMessage(mykError);
			}

			/*** < Info: abort init cals > ***/
			if ((mykError = MYKONOS_abortInitCals(&mykDevice, &initCalsCompleted)) != MYKONOS_ERR_OK)
			{
				/*** < Info: errorString will contain log error string in order to debug failure > ***/
				errorString = getMykonosErrorMessage(mykError);
			}
			if (initCalsCompleted)
			{
				/*** < Info: which calls had completed, per the mask > ***/
			}

			if ((mykError = MYKONOS_readArmCmdStatus(&mykDevice, &errorWord, &statusWord)) != MYKONOS_ERR_OK)
			{
				/*** < Info: errorString will contain log error string in order to debug failure > ***/
				errorString = getMykonosErrorMessage(mykError);
			}

			if ((mykError = MYKONOS_readArmCmdStatusByte(&mykDevice, 2, &status)) != MYKONOS_ERR_OK)
			{
				/*** < Info: errorString will contain log error string in order to debug failure > ***/
				errorString = getMykonosErrorMessage(mykError);
			}
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

	if ((mykError = MYKONOS_enableSysrefToRxFramer(&mykDevice, 1)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
	    errorString = getMykonosErrorMessage(mykError);
	}
	/*** < Info: Mykonos is waiting for sysref in order to start
	 * transmitting CGS from the RxFramer> ***/

	if ((mykError = MYKONOS_enableSysrefToObsRxFramer(&mykDevice, 1)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
        errorString = getMykonosErrorMessage(mykError);
	}
	/*** < Info: Mykonos is waiting for sysref in order to start
	 * transmitting CGS from the ObsRxFramer> ***/

	/*** < User: Make sure SYSREF is stopped/disabled > ***/
	if ((mykError = MYKONOS_enableSysrefToDeframer(&mykDevice, 0)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
        errorString = getMykonosErrorMessage(mykError);
	}

	if ((mykError = MYKONOS_resetDeframer(&mykDevice)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
        errorString = getMykonosErrorMessage(mykError);
	}

	/*** < User: make sure BBIC JESD framer is actively transmitting CGS> ***/
	if ((mykError = MYKONOS_enableSysrefToDeframer(&mykDevice, 1)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
        errorString = getMykonosErrorMessage(mykError);
	}

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
	if ((mykError = MYKONOS_readRxFramerStatus(&mykDevice, &framerStatus)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
        errorString = getMykonosErrorMessage(mykError);
	}
	if ((mykError = MYKONOS_readOrxFramerStatus(&mykDevice, &obsFramerStatus)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
        errorString = getMykonosErrorMessage(mykError);
	}

	/*************************************************************************/
	/*****               Check Mykonos Deframer Status                   *****/
	/*************************************************************************/
	if ((mykError = MYKONOS_readDeframerStatus(&mykDevice, &deframerStatus)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
        errorString = getMykonosErrorMessage(mykError);
	}

	/*** < Action: When links have been verified, proceed > ***/

	/*************************************************************************/
	/*****           Mykonos enable tracking calibrations                *****/
	/*************************************************************************/
	if ((mykError = MYKONOS_enableTrackingCals(&mykDevice, trackingCalMask)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug why enableTrackingCals failed > ***/
        errorString = getMykonosErrorMessage(mykError);
	}

	/*** < Info: Allow Rx1/2 QEC tracking and Tx1/2 QEC tracking to run when in the radioOn state
	     *  Tx calibrations will only run if radioOn and the obsRx path is set to OBS_INTERNAL_CALS > ***/

	/*** < Info: Function to turn radio on, Enables transmitters and receivers
	 * that were setup during MYKONOS_initialize() > ***/
	if ((mykError = MYKONOS_radioOn(&mykDevice)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
        errorString = getMykonosErrorMessage(mykError);
	}

	/*** < Info: Allow TxQEC to run when User: is not actively using ORx receive path > ***/
	if ((mykError = MYKONOS_setObsRxPathSource(&mykDevice, OBS_RXOFF)) != MYKONOS_ERR_OK)
    {
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
        errorString = getMykonosErrorMessage(mykError);
    }
	if ((mykError = MYKONOS_setObsRxPathSource(&mykDevice, OBS_INTERNALCALS)) != MYKONOS_ERR_OK)
	{
	    /*** < Info: errorString will contain log error string in order to debug failure > ***/
        errorString = getMykonosErrorMessage(mykError);
	}

	return 0;
}