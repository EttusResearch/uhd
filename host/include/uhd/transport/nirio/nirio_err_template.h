//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

/*
 * NIRIO_ERR_INFO is undefined on purpose.
 * This header is designed to be included with a custom definition of NIRIO_ERR_INFO
 * To "generate" code in multiple files. This prevents duplication of code.
 */

NIRIO_ERR_INFO(NiRio_Status_Success, 0, "No errors or warnings.")
NIRIO_ERR_INFO(NiRio_Status_FifoTimeout, -50400, "The timeout expired before the FIFO operation could complete.")
NIRIO_ERR_INFO(NiRio_Status_MemoryFull, -52000, "A memory allocation failed. Try again after rebooting.")
NIRIO_ERR_INFO(NiRio_Status_SoftwareFault, -52003, "An unexpected software error occurred.")
NIRIO_ERR_INFO(NiRio_Status_InvalidParameter, -52005, "A parameter to a function was not valid. This could be a NULL pointer, a bad value, etc.")
NIRIO_ERR_INFO(NiRio_Status_ResourceNotFound, -52006, "A required resource was not found. The NiFpga.* library, the RIO resource, or some other resource may be missing.")
NIRIO_ERR_INFO(NiRio_Status_ResourceNotInitialized, -52010, "A required resource was not properly initialized. This could occur if NiFpga_Initialize was not called or a required NiFpga_IrqContext was not reserved.")
NIRIO_ERR_INFO(NiRio_Status_FpgaAlreadyRunning, -61003, "The FPGA is already running.")
NIRIO_ERR_INFO(NiRio_Status_DeviceTypeMismatch, -61024, "The bitfile was not compiled for the specified resource's device type.")
NIRIO_ERR_INFO(NiRio_Status_CommunicationTimeout, -61046, "An error was detected in the communication between the host computer and the USRP device. This could be due to a hardware failure on the bus.")
NIRIO_ERR_INFO(NiRio_Status_IrqTimeout, -61060, "The timeout expired before any of the IRQs were asserted.")
NIRIO_ERR_INFO(NiRio_Status_CorruptBitfile, -61070, "The LVBITX configuration bitstream seems to be corrupt.")
NIRIO_ERR_INFO(NiRio_Status_BadDepth, -61072, "The requested FIFO depth is invalid. It is either 0 or an amount not supported by the hardware.")
NIRIO_ERR_INFO(NiRio_Status_BadReadWriteCount, -61073, "The number of FIFO elements is invalid. Either the number is greater than the depth of the host memory DMA FIFO, or more elements were requested for release than had been acquired.")
NIRIO_ERR_INFO(NiRio_Status_ClockLostLock, -61083, "A hardware clocking error occurred.")
NIRIO_ERR_INFO(NiRio_Status_FpgaBusy, -61141, "The operation could not be performed because the FPGA is busy.")
NIRIO_ERR_INFO(NiRio_Status_FpgaBusyFpgaInterfaceCApi, -61200, "The operation could not be performed because the FPGA is busy.")
NIRIO_ERR_INFO(NiRio_Status_FpgaBusyScanInterface, -61201, "The operation could not be performed because the chassis is in Scan Interface programming mode.")
NIRIO_ERR_INFO(NiRio_Status_FpgaBusyFpgaInterface, -61202, "The operation could not be performed because the FPGA is busy operating in FPGA Interface mode. Stop all activities on the FPGA before requesting this operation.")
NIRIO_ERR_INFO(NiRio_Status_FpgaBusyInteractive, -61203, "The operation could not be performed because the FPGA is busy operating in FPGA Interactive mode. Stop all activities on the FPGA before requesting this operation.")
NIRIO_ERR_INFO(NiRio_Status_FpgaBusyEmulation, -61204, "The operation could not be performed because the FPGA is busy operating in FPGA Emulation mode. Stop all activities on the FPGA before requesting this operation.")
NIRIO_ERR_INFO(NiRio_Status_GatedClockHandshakingViolation, -61216, "A gated clock has violated the handshaking protocol.")
NIRIO_ERR_INFO(NiRio_Status_RegionsOutstandingForSession, -61217, "A session cannot be closed, reset, nor can a bitfile be downloaded while DMA FIFO region references are still outstanding for the specified session.")
NIRIO_ERR_INFO(NiRio_Status_ElementsNotPermissibleToBeAcquired, -61219, "There are currently fewer unacquired elements left in the FIFO than are being requested. Release some acquired elements before acquiring more elements.")
NIRIO_ERR_INFO(NiRio_Status_InternalError, -61499, "An unexpected internal error occurred.")
NIRIO_ERR_INFO(NiRio_Status_AccessDenied, -63033, "Access to the local or remote system was denied.")
NIRIO_ERR_INFO(NiRio_Status_RpcConnectionError, -63040, "A connection could not be established to the specified remote device manager. Ensure that the devices are on, that NI-USRPRIO software is installed, and that the USRPRIO server is running and properly configured.")
NIRIO_ERR_INFO(NiRio_Status_RpcOperationError, -63042, "A fault on the network caused the RPC operation to fail.")
NIRIO_ERR_INFO(NiRio_Status_RpcSessionError, -63043, "The RPC session to the remote device manager is invalid. Ensure that the device is connected and try restarting the server.")
NIRIO_ERR_INFO(NiRio_Status_FifoReserved, -63082, "The operation could not complete because another session is accessing the FIFO. Close the other session and retry.")
NIRIO_ERR_INFO(NiRio_Status_FifoElementsCurrentlyAcquired, -63083, "A Configure FIFO, Stop FIFO, Read FIFO, or Write FIFO function was called while the host had acquired elements of the FIFO. Release all acquired elements before configuring, stopping, reading, or writing.")
NIRIO_ERR_INFO(NiRio_Status_MisalignedAccess, -63084, "A function was called using a misaligned address. The address must be a multiple of the size of the datatype.")
NIRIO_ERR_INFO(NiRio_Status_BitfileReadError, -63101, "A valid .lvbitx bitfile is required. If you are using a valid .lvbitx bitfile, the bitfile may not be compatible with the software you are using.")
NIRIO_ERR_INFO(NiRio_Status_SignatureMismatch, -63106, "The specified signature does not match the signature of the bitfile. If the bitfile has been recompiled, regenerate the C API and rebuild the application.")
NIRIO_ERR_INFO(NiRio_Status_IncompatibleBitfile, -63107, "The bitfile you are trying to use is not compatible with the version of NI-RIO installed on the target and/or the host.")
NIRIO_ERR_INFO(NiRio_Status_InvalidResourceName, -63192, "Either the supplied resource name is invalid as a RIO resource name, or the device was not found.")
NIRIO_ERR_INFO(NiRio_Status_FeatureNotSupported, -63193, "The requested feature is not supported.")
NIRIO_ERR_INFO(NiRio_Status_VersionMismatch, -63194, "Software version mismatch.")
NIRIO_ERR_INFO(NiRio_Status_InvalidSession, -63195, "The session is invalid or has been closed.")
NIRIO_ERR_INFO(NiRio_Status_OutOfHandles, -63198, "The maximum number of open FPGA sessions has been reached. Close some open sessions.")
NIRIO_ERR_INFO(NiRio_Status_DeviceLocked, -63031, "The operation is not allowed because another session in a different process is accessing the device. Close all other sessions and retry.")


