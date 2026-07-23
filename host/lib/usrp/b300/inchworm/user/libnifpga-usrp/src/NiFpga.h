/*
 * FPGA Interface C API header file.
 *
 * NOTE: All NiFpgaEx_* symbols below are extensions that are not part of the
 * standard FPGA Interface C API.
 *
 * Copyright (c) 2014 National Instruments
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#pragma once

/*
 * Determine platform details.
 */
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#    define NiFpga_Linux 1
#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WINDOWS__)
#    define NiFpga_Windows 1
#else
#error Unsupported platform. This version of the FPGA Interface C API \
          supports Linux and Windows.
#endif

/*
 * Determine compliance with different C/C++ language standards.
 */
#if defined(__cplusplus)
#    define NiFpga_Cpp 1
#    if __cplusplus >= 199707L
#        define NiFpga_Cpp98 1
#        if __cplusplus >= 201103L
#            define NiFpga_Cpp11 1
#        endif
#    endif
#endif
#if defined(__STDC__)
#    define NiFpga_C89 1
#    if defined(__STDC_VERSION__)
#        define NiFpga_C90 1
#        if __STDC_VERSION__ >= 199409L
#            define NiFpga_C94 1
#            if __STDC_VERSION__ >= 199901L
#                define NiFpga_C99 1
#                if __STDC_VERSION__ >= 201112L
#                    define NiFpga_C11 1
#                endif
#            endif
#        endif
#    endif
#endif

/*
 * Determine compiler.
 */
#if defined(__GNUC__) && __GNUC__ >= 4
/* Allows for "#if NiFpga_Gcc > 40302" to test for version 4.3.2. */
#    define NiFpga_Gcc (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ * 1)
/* This version of GCC supports the inline keyword. */
#    define NiFpga_Inline inline
#elif defined(_MSC_VER)
/* Microsoft Visual C++ compiler */
#    define NiFpga_MSVC   _MSC_VER
#    define NiFpga_Inline __inline
#elif defined(__GNUC__)
#error Unsupported GCC version. This version of the FPGA Interface C API \
          requires GCC 4.0 and greater.
#else
/* Unknown compiler, assume inline support */
#    define NiFpga_Inline inline
#endif

/* Included for definition of size_t. */
#include <stddef.h>

/* Included for definition of exact-width integer types. */
#include <stdint.h>

/* Make the following symbols public. */
#if defined(__GNUC__)
#    pragma GCC visibility push(default)
#elif defined(_MSC_VER)
/* MSVC uses __declspec(dllexport/dllimport) for symbol visibility */
/* For static linking, no special visibility needed */
#endif

#if defined(NiFpga_Cpp)
extern "C" {
#endif

/**
 * A boolean value; either NiFpga_False or NiFpga_True.
 */
typedef uint8_t NiFpga_Bool;

/**
 * Represents a false condition.
 */
static const NiFpga_Bool NiFpga_False = 0;

/**
 * Represents a true condition.
 */
static const NiFpga_Bool NiFpga_True = 1;

/**
 * Represents the resulting status of a function call through its return value.
 * 0 is success, negative values are errors, and positive values are warnings.
 */
typedef int32_t NiFpga_Status;

/**
 * No errors or warnings.
 */
static const NiFpga_Status NiFpga_Status_Success = 0;

/**
 * The timeout expired before the FIFO operation could complete.
 */
static const NiFpga_Status NiFpga_Status_FifoTimeout = -50400;

/**
 * No transfer is in progress because the transfer was aborted by the client.
 */
static const NiFpga_Status NiFpga_Status_TransferAborted = -50405;

/**
 * A memory allocation failed. Try again after rebooting.
 */
static const NiFpga_Status NiFpga_Status_MemoryFull = -52000;

/**
 * An unexpected software error occurred.
 */
static const NiFpga_Status NiFpga_Status_SoftwareFault = -52003;

/**
 * A parameter to a function was not valid. This could be a NULL pointer, a bad
 * value, etc.
 */
static const NiFpga_Status NiFpga_Status_InvalidParameter = -52005;

/**
 * A required resource was not found. The NiFpga.* library, the RIO resource, or
 * some other resource may be missing.
 */
static const NiFpga_Status NiFpga_Status_ResourceNotFound = -52006;

/**
 * A required resource was not properly initialized. This could occur if a
 * required NiFpga_IrqContext was not reserved.
 */
static const NiFpga_Status NiFpga_Status_ResourceNotInitialized = -52010;

/**
 * A hardware failure has occurred. The operation could not be completed as
 * specified.
 */
static const NiFpga_Status NiFpga_Status_HardwareFault = -52018;

/**
 * The FPGA is already running.
 */
static const NiFpga_Status NiFpga_Status_FpgaAlreadyRunning = -61003;

/**
 * An error occurred downloading the VI to the FPGA device. Verify that
 * the target is connected and powered and that the resource of the target
 * is properly configured.
 */
static const NiFpga_Status NiFpga_Status_DownloadError = -61018;

/**
 * The bitfile was not compiled for the specified resource's device type.
 */
static const NiFpga_Status NiFpga_Status_DeviceTypeMismatch = -61024;

/**
 * An error was detected in the communication between the host computer and the
 * FPGA target.
 */
static const NiFpga_Status NiFpga_Status_CommunicationTimeout = -61046;

/**
 * The timeout expired before any of the IRQs were asserted.
 */
static const NiFpga_Status NiFpga_Status_IrqTimeout = -61060;

/**
 * The specified bitfile is invalid or corrupt.
 */
static const NiFpga_Status NiFpga_Status_CorruptBitfile = -61070;

/**
 * The requested FIFO depth is invalid. It is either 0 or an amount not
 * supported by the hardware.
 */
static const NiFpga_Status NiFpga_Status_BadDepth = -61072;

/**
 * The number of FIFO elements is invalid. Either the number is greater than the
 * depth of the host memory DMA FIFO, or more elements were requested for
 * release than had been acquired.
 */
static const NiFpga_Status NiFpga_Status_BadReadWriteCount = -61073;

/**
 * A hardware clocking error occurred. A derived clock lost lock with its base
 * clock during the execution of the LabVIEW FPGA VI. If any base clocks with
 * derived clocks are referencing an external source, make sure that the
 * external source is connected and within the supported frequency, jitter,
 * accuracy, duty cycle, and voltage specifications. Also verify that the
 * characteristics of the base clock match the configuration specified in the
 * FPGA Base Clock Properties. If all base clocks with derived clocks are
 * generated from free-running, on-board sources, please contact National
 * Instruments technical support at ni.com/support.
 */
static const NiFpga_Status NiFpga_Status_ClockLostLock = -61083;

/**
 * The operation could not be performed because the FPGA is busy. Stop all
 * activities on the FPGA before requesting this operation. If the target is in
 * Scan Interface programming mode, put it in FPGA Interface programming mode.
 */
static const NiFpga_Status NiFpga_Status_FpgaBusy = -61141;

/**
 * The operation could not be performed because the FPGA is busy operating in
 * FPGA Interface C API mode. Stop all activities on the FPGA before requesting
 * this operation.
 */
static const NiFpga_Status NiFpga_Status_FpgaBusyFpgaInterfaceCApi = -61200;

/**
 * The chassis is in Scan Interface programming mode. In order to run FPGA VIs,
 * you must go to the chassis properties page, select FPGA programming mode, and
 * deploy settings.
 */
static const NiFpga_Status NiFpga_Status_FpgaBusyScanInterface = -61201;

/**
 * The operation could not be performed because the FPGA is busy operating in
 * FPGA Interface mode. Stop all activities on the FPGA before requesting this
 * operation.
 */
static const NiFpga_Status NiFpga_Status_FpgaBusyFpgaInterface = -61202;

/**
 * The operation could not be performed because the FPGA is busy operating in
 * Interactive mode. Stop all activities on the FPGA before requesting this
 * operation.
 */
static const NiFpga_Status NiFpga_Status_FpgaBusyInteractive = -61203;

/**
 * The operation could not be performed because the FPGA is busy operating in
 * Emulation mode. Stop all activities on the FPGA before requesting this
 * operation.
 */
static const NiFpga_Status NiFpga_Status_FpgaBusyEmulation = -61204;

/**
 * LabVIEW FPGA does not support the Reset method for bitfiles that allow
 * removal of implicit enable signals in single-cycle Timed Loops.
 */
static const NiFpga_Status NiFpga_Status_ResetCalledWithImplicitEnableRemoval = -61211;

/**
 * LabVIEW FPGA does not support the Abort method for bitfiles that allow
 * removal of implicit enable signals in single-cycle Timed Loops.
 */
static const NiFpga_Status NiFpga_Status_AbortCalledWithImplicitEnableRemoval = -61212;

/**
 * LabVIEW FPGA does not support Close and Reset if Last Reference for bitfiles
 * that allow removal of implicit enable signals in single-cycle Timed Loops.
 * Pass the NiFpga_CloseAttribute_NoResetIfLastSession attribute to NiFpga_Close
 * instead of 0.
 */
static const NiFpga_Status NiFpga_Status_CloseAndResetCalledWithImplicitEnableRemoval =
    -61213;

/**
 * For bitfiles that allow removal of implicit enable signals in single-cycle
 * Timed Loops, LabVIEW FPGA does not support this method prior to running the
 * bitfile.
 */
static const NiFpga_Status NiFpga_Status_ImplicitEnableRemovalButNotYetRun = -61214;

/**
 * Bitfiles that allow removal of implicit enable signals in single-cycle Timed
 * Loops can run only once. Download the bitfile again before re-running the VI.
 */
static const NiFpga_Status NiFpga_Status_RunAfterStoppedCalledWithImplicitEnableRemoval =
    -61215;

/**
 * A gated clock has violated the handshaking protocol. If you are using
 * external gated clocks, ensure that they follow the required clock gating
 * protocol. If you are generating your clocks internally, please contact
 * National Instruments Technical Support.
 */
static const NiFpga_Status NiFpga_Status_GatedClockHandshakingViolation = -61216;

/**
 * A session cannot be closed, reset, nor can a bitfile be downloaded while
 * DMA FIFO region references are still outstanding for the specified session.
 * Using Delete Data Value Reference, delete any regions acquired from Acquire
 * Read Region or Acquire Write Region first before taking any of these actions.
 */
static const NiFpga_Status NiFpga_Status_RegionsOutstandingForSession = -61217;

/**
 * The number of elements requested must be less than or equal to the number of
 * unacquired elements left in the host memory DMA FIFO. There are currently
 * fewer unacquired elements left in the FIFO than are being requested. Release
 * some acquired elements before acquiring more elements.
 */
static const NiFpga_Status NiFpga_Status_ElementsNotPermissibleToBeAcquired = -61219;

/**
 * An unexpected internal error occurred.
 */
static const NiFpga_Status NiFpga_Status_InternalError = -61499;

/**
 * The combined depth of all DMA FIFOs exceeds the maximum depth for the
 * controller. You may be able to reconfigure the controller with a greater
 * maximum FIFO depth. For more information, refer to the NI
 * KnowledgeBase article 65OF2ERQ.
 */
static const NiFpga_Status NiFpga_Status_TotalDmaFifoDepthExceeded = -63003;

/**
 * Access was denied. For remote systems, use MAX to check the Remote Device
 * Access settings under Software>>NI-RIO>>NI-RIO Settings on the remote system.
 * Otherwise, you may need elevated permissions for this operation.
 */
static const NiFpga_Status NiFpga_Status_AccessDenied = -63033;

/**
 * The current session is invalid because the FPGA protection mechanism has been
 * activated to prevent it from exceeding its temperature specifications. The
 * current operation cannot complete. Try again.
 */
static const NiFpga_Status NiFpga_Status_ProtectionActivated = -63037;

/**
 * The NI-RIO software on the target system is not compatible with this
 * software. Upgrade the NI-RIO software on the host in order to connect to
 * this device.
 */
static const NiFpga_Status NiFpga_Status_DriverUpgradeRequired = -63038;

/**
 * A connection could not be established to the specified remote device. Ensure
 * that the device is on and accessible over the network, that NI-RIO software
 * is installed, and that the RIO server is running and properly configured.
 */
static const NiFpga_Status NiFpga_Status_RpcConnectionError = -63040;

/**
 * The RPC session is invalid. The target may have reset or been rebooted. Check
 * the network connection and retry the operation.
 */
static const NiFpga_Status NiFpga_Status_RpcSessionError = -63043;

/**
 * The allocated buffer is too small.
 */
static const NiFpga_Status NiFpga_Status_BufferInvalidSize = -63080;

/**
 * The operation could not complete because another session is accessing the
 * FIFO. Close the other session and retry.
 */
static const NiFpga_Status NiFpga_Status_FifoReserved = -63082;

/**
 * A Configure FIFO, Stop FIFO, Read FIFO, or Write FIFO function was called
 * while the host had acquired elements of the FIFO. Release all acquired
 * elements before configuring, stopping, reading, or writing.
 */
static const NiFpga_Status NiFpga_Status_FifoElementsCurrentlyAcquired = -63083;

/**
 * A function was called using a misaligned address. The address must be a
 * multiple of the size of the datatype.
 */
static const NiFpga_Status NiFpga_Status_MisalignedAccess = -63084;

/**
 * A valid .lvbitx bitfile is required. If you are using a valid .lvbitx
 * bitfile, the bitfile may not be compatible with the software you are using.
 * Determine which version of LabVIEW was used to make the bitfile, update your
 * software to that version or later, and try again.
 */
static const NiFpga_Status NiFpga_Status_BitfileReadError = -63101;

/**
 * The specified signature does not match the signature of the bitfile. If the
 * bitfile has been recompiled, regenerate the C API and rebuild the
 * application.
 */
static const NiFpga_Status NiFpga_Status_SignatureMismatch = -63106;

/**
 * The bitfile you are trying to use is incompatible with the version
 * of NI-RIO installed on the target and/or host. Update the version
 * of NI-RIO on the target and/or host to the same version (or later)
 * used to compile the bitfile. Alternatively, recompile the bitfile
 * with the same version of NI-RIO that is currently installed on the
 * target and/or host.
 */
static const NiFpga_Status NiFpga_Status_IncompatibleBitfile = -63107;

/**
 * The supplied resource name is invalid, not found, or could not be resolved
 * due to multiple matches.
 */
static const NiFpga_Status NiFpga_Status_InvalidResourceName = -63192;

/**
 * The requested feature is not supported.
 */
static const NiFpga_Status NiFpga_Status_FeatureNotSupported = -63193;

/**
 * The NI-RIO software on the target system is not compatible with this
 * software. Upgrade the NI-RIO software on the target system.
 */
static const NiFpga_Status NiFpga_Status_VersionMismatch = -63194;

/**
 * The session is invalid or has been closed.
 */
static const NiFpga_Status NiFpga_Status_InvalidSession = -63195;

/**
 * The maximum number of open FPGA sessions has been reached. Close some open
 * sessions.
 */
static const NiFpga_Status NiFpga_Status_OutOfHandles = -63198;

/**
 * Tests whether a status is an error.
 *
 * @param status status to check for an error
 * @return whether the status was an error
 */
static NiFpga_Inline NiFpga_Bool NiFpga_IsError(const NiFpga_Status status)
{
    return status < NiFpga_Status_Success;
}

/**
 * Tests whether a status is not an error. Success and warnings are not errors.
 *
 * @param status status to check for an error
 * @return whether the status was a success or warning
 */
static NiFpga_Inline NiFpga_Bool NiFpga_IsNotError(const NiFpga_Status status)
{
    return status >= NiFpga_Status_Success;
}

/**
 * Conditionally sets the status to a new value. The previous status is
 * preserved unless the new status is more of an error, which means that
 * warnings and errors overwrite successes, and errors overwrite warnings. New
 * errors do not overwrite older errors, and new warnings do not overwrite
 * older warnings.
 *
 * @param status status to conditionally set
 * @param newStatus new status value that may be set
 * @return the resulting status
 */
static NiFpga_Inline NiFpga_Status NiFpga_MergeStatus(
    NiFpga_Status* const status, const NiFpga_Status newStatus)
{
    if (!status)
        return NiFpga_Status_InvalidParameter;
    if (NiFpga_IsNotError(*status)
        && (*status == NiFpga_Status_Success || NiFpga_IsError(newStatus)))
        *status = newStatus;
    return *status;
}

/**
 * This macro evaluates the expression only if the status is not an error. The
 * expression must evaluate to an NiFpga_Status, such as a call to any NiFpga_*
 * function, because the status will be set to the returned status if the
 * expression is evaluated.
 *
 * You can use this macro to mimic status chaining in LabVIEW, where the status
 * does not have to be explicitly checked after each call. Such code may look
 * like the following example.
 *
 *    NiFpga_Status status = NiFpga_Status_Success;
 *    NiFpga_IfIsNotError(status, NiFpga_WriteU32(...));
 *    NiFpga_IfIsNotError(status, NiFpga_WriteU32(...));
 *    NiFpga_IfIsNotError(status, NiFpga_WriteU32(...));
 *
 * @param status status to check for an error
 * @param expression expression to call if the incoming status is not an error
 */
#define NiFpga_IfIsNotError(status, expression) \
    if (NiFpga_IsNotError(status))              \
        NiFpga_MergeStatus(&status, (expression));

/**
 * Deprecated. Neither NiFpga_Initialize nor NiFpga_Finalize are required
 * with this version of the FPGA Interface C API and can be removed.
 *
 * @return result of the call
 */
#if defined(__GNUC__)
static NiFpga_Status NiFpga_Initialize(void) __attribute__((deprecated));
#elif defined(_MSC_VER)
__declspec(deprecated) static NiFpga_Status NiFpga_Initialize(void);
#else
static NiFpga_Status NiFpga_Initialize(void);
#endif

static NiFpga_Inline NiFpga_Status NiFpga_Initialize(void)
{
    return NiFpga_Status_Success;
}

/**
 * Deprecated. Neither NiFpga_Initialize nor NiFpga_Finalize are required
 * with this version of the FPGA Interface C API and can be removed.
 *
 * @return result of the call
 */
#if defined(__GNUC__)
static NiFpga_Status NiFpga_Finalize(void) __attribute__((deprecated));
#elif defined(_MSC_VER)
__declspec(deprecated) static NiFpga_Status NiFpga_Finalize(void);
#else
static NiFpga_Status NiFpga_Finalize(void);
#endif

static NiFpga_Inline NiFpga_Status NiFpga_Finalize(void)
{
    return NiFpga_Status_Success;
}

/**
 * A handle to an FPGA session.
 */
typedef uint32_t NiFpga_Session;

/**
 * Attributes that NiFpga_Open accepts.
 */
typedef enum {
    NiFpga_OpenAttribute_NoRun            = 1,
    NiFpga_OpenAttribute_NoSignatureCheck = 1U << 31,
} NiFpga_OpenAttribute;

/**
 * Opens a session to the FPGA. This call ensures that the contents of the
 * bitfile are programmed to the FPGA. The FPGA runs unless the
 * NiFpga_OpenAttribute_NoRun attribute is used.
 *
 * Because different operating systems have different default current working
 * directories for applications, you must pass an absolute path for the bitfile
 * parameter. If you pass only the filename instead of an absolute path, the
 * operating system may not be able to locate the bitfile. For example, the
 * default current working directories are C:\ni-rt\system\ for Phar Lap ETS and
 * /c/ for VxWorks. Because the generated *_Bitfile constant is a #define to a
 * string literal, you can use C/C++ string-literal concatenation to form an
 * absolute path. For example, if the bitfile is in the root directory of a
 * Phar Lap ETS system, pass the following for the bitfile parameter.
 *
 *    "C:\\" NiFpga_MyApplication_Bitfile
 *
 * @param bitfile path to the bitfile
 * @param signature signature of the bitfile, or NULL to not verify signature
 *                  for use when only using NiFpgaEx_FindResource instead of a
 *                  header generated with the FPGA Interface C API Generator
 * @param resource RIO resource string to open ("RIO0")
 * @param attribute bitwise OR of any NiFpga_OpenAttributes, or 0
 * @param session outputs the session handle, which must be closed when no
 *                longer needed
 * @return result of the call
 */
NiFpga_Status NiFpga_Open(const char* resource, NiFpga_Session* session);

/**
 * Attributes that NiFpga_Close accepts.
 */
typedef enum { NiFpga_CloseAttribute_NoResetIfLastSession = 1 } NiFpga_CloseAttribute;

/**
 * Closes the session to the FPGA. The FPGA resets unless either another session
 * is still open or you use the NiFpga_CloseAttribute_NoResetIfLastSession
 * attribute.
 *
 * @param session handle to a currently open session
 * @param attribute bitwise OR of any NiFpga_CloseAttributes, or 0
 * @return result of the call
 */
NiFpga_Status NiFpga_Close(NiFpga_Session session, uint32_t attribute);

/**
 * Attributes that NiFpga_Run accepts.
 */
typedef enum { NiFpga_RunAttribute_WaitUntilDone = 1 } NiFpga_RunAttribute;

/**
 * Runs the FPGA VI on the target. If you use NiFpga_RunAttribute_WaitUntilDone,
 * NiFpga_Run blocks the thread until the FPGA finishes running.
 *
 * @param session handle to a currently open session
 * @param attribute bitwise OR of any NiFpga_RunAttributes, or 0
 * @return result of the call
 */
NiFpga_Status NiFpga_Run(NiFpga_Session session, uint32_t attribute);

/**
 * Aborts the FPGA VI.
 *
 * @param session handle to a currently open session
 * @return result of the call
 */
NiFpga_Status NiFpga_Abort(NiFpga_Session session);

/**
 * Resets the FPGA VI.
 *
 * @param session handle to a currently open session
 * @return result of the call
 */
NiFpga_Status NiFpga_Reset(NiFpga_Session session);

/**
 * Re-downloads the FPGA bitstream to the target.
 *
 * @param session handle to a currently open session
 * @return result of the call
 */
NiFpga_Status NiFpga_Download(NiFpga_Session session);

/** Any indicator, control, or FIFO resource. */
typedef uint32_t NiFpgaEx_Resource;

/** Any indicator or control resource. */
typedef NiFpgaEx_Resource NiFpgaEx_Register;
typedef NiFpgaEx_Register NiFpgaEx_RegisterBool;
typedef NiFpgaEx_Register NiFpgaEx_RegisterI8;
typedef NiFpgaEx_Register NiFpgaEx_RegisterU8;
typedef NiFpgaEx_Register NiFpgaEx_RegisterI16;
typedef NiFpgaEx_Register NiFpgaEx_RegisterU16;
typedef NiFpgaEx_Register NiFpgaEx_RegisterI32;
typedef NiFpgaEx_Register NiFpgaEx_RegisterU32;
typedef NiFpgaEx_Register NiFpgaEx_RegisterI64;
typedef NiFpgaEx_Register NiFpgaEx_RegisterU64;
typedef NiFpgaEx_Register NiFpgaEx_RegisterSgl;
typedef NiFpgaEx_Register NiFpgaEx_RegisterDbl;

/** Any array indicator or control resource. */
typedef NiFpgaEx_Register NiFpgaEx_RegisterArray;
typedef NiFpgaEx_RegisterArray NiFpgaEx_RegisterArrayBool;
typedef NiFpgaEx_RegisterArray NiFpgaEx_RegisterArrayI8;
typedef NiFpgaEx_RegisterArray NiFpgaEx_RegisterArrayU8;
typedef NiFpgaEx_RegisterArray NiFpgaEx_RegisterArrayI16;
typedef NiFpgaEx_RegisterArray NiFpgaEx_RegisterArrayU16;
typedef NiFpgaEx_RegisterArray NiFpgaEx_RegisterArrayI32;
typedef NiFpgaEx_RegisterArray NiFpgaEx_RegisterArrayU32;
typedef NiFpgaEx_RegisterArray NiFpgaEx_RegisterArrayI64;
typedef NiFpgaEx_RegisterArray NiFpgaEx_RegisterArrayU64;
typedef NiFpgaEx_RegisterArray NiFpgaEx_RegisterArraySgl;
typedef NiFpgaEx_RegisterArray NiFpgaEx_RegisterArrayDbl;

/** Any target-to-host or host-to-target DMA FIFO resource. */
typedef NiFpgaEx_Resource NiFpgaEx_DmaFifo;

/** Any target-to-host DMA FIFO resource. */
typedef NiFpgaEx_DmaFifo NiFpgaEx_TargetToHostFifo;
typedef NiFpgaEx_TargetToHostFifo NiFpgaEx_TargetToHostFifoBool;
typedef NiFpgaEx_TargetToHostFifo NiFpgaEx_TargetToHostFifoI8;
typedef NiFpgaEx_TargetToHostFifo NiFpgaEx_TargetToHostFifoU8;
typedef NiFpgaEx_TargetToHostFifo NiFpgaEx_TargetToHostFifoI16;
typedef NiFpgaEx_TargetToHostFifo NiFpgaEx_TargetToHostFifoU16;
typedef NiFpgaEx_TargetToHostFifo NiFpgaEx_TargetToHostFifoI32;
typedef NiFpgaEx_TargetToHostFifo NiFpgaEx_TargetToHostFifoU32;
typedef NiFpgaEx_TargetToHostFifo NiFpgaEx_TargetToHostFifoI64;
typedef NiFpgaEx_TargetToHostFifo NiFpgaEx_TargetToHostFifoU64;
typedef NiFpgaEx_TargetToHostFifo NiFpgaEx_TargetToHostFifoSgl;
typedef NiFpgaEx_TargetToHostFifo NiFpgaEx_TargetToHostFifoDbl;

/** Any host-to-target DMA FIFO resource. */
typedef NiFpgaEx_DmaFifo NiFpgaEx_HostToTargetFifo;
typedef NiFpgaEx_HostToTargetFifo NiFpgaEx_HostToTargetFifoBool;
typedef NiFpgaEx_HostToTargetFifo NiFpgaEx_HostToTargetFifoI8;
typedef NiFpgaEx_HostToTargetFifo NiFpgaEx_HostToTargetFifoU8;
typedef NiFpgaEx_HostToTargetFifo NiFpgaEx_HostToTargetFifoI16;
typedef NiFpgaEx_HostToTargetFifo NiFpgaEx_HostToTargetFifoU16;
typedef NiFpgaEx_HostToTargetFifo NiFpgaEx_HostToTargetFifoI32;
typedef NiFpgaEx_HostToTargetFifo NiFpgaEx_HostToTargetFifoU32;
typedef NiFpgaEx_HostToTargetFifo NiFpgaEx_HostToTargetFifoI64;
typedef NiFpgaEx_HostToTargetFifo NiFpgaEx_HostToTargetFifoU64;
typedef NiFpgaEx_HostToTargetFifo NiFpgaEx_HostToTargetFifoSgl;
typedef NiFpgaEx_HostToTargetFifo NiFpgaEx_HostToTargetFifoDbl;

/** Any peer-to-peer FIFO resource. */
typedef NiFpgaEx_Resource NiFpgaEx_PeerToPeerFifo;

/**
 * All possible FPGA resource types that can be found with
 * NiFpgaEx_FindResource.
 */
typedef enum {
    NiFpgaEx_ResourceType_IndicatorBool        = 0,
    NiFpgaEx_ResourceType_IndicatorI8          = 1,
    NiFpgaEx_ResourceType_IndicatorU8          = 2,
    NiFpgaEx_ResourceType_IndicatorI16         = 3,
    NiFpgaEx_ResourceType_IndicatorU16         = 4,
    NiFpgaEx_ResourceType_IndicatorI32         = 5,
    NiFpgaEx_ResourceType_IndicatorU32         = 6,
    NiFpgaEx_ResourceType_IndicatorI64         = 7,
    NiFpgaEx_ResourceType_IndicatorU64         = 8,
    NiFpgaEx_ResourceType_ControlBool          = 9,
    NiFpgaEx_ResourceType_ControlI8            = 10,
    NiFpgaEx_ResourceType_ControlU8            = 11,
    NiFpgaEx_ResourceType_ControlI16           = 12,
    NiFpgaEx_ResourceType_ControlU16           = 13,
    NiFpgaEx_ResourceType_ControlI32           = 14,
    NiFpgaEx_ResourceType_ControlU32           = 15,
    NiFpgaEx_ResourceType_ControlI64           = 16,
    NiFpgaEx_ResourceType_ControlU64           = 17,
    NiFpgaEx_ResourceType_IndicatorArrayBool   = 18,
    NiFpgaEx_ResourceType_IndicatorArrayI8     = 19,
    NiFpgaEx_ResourceType_IndicatorArrayU8     = 20,
    NiFpgaEx_ResourceType_IndicatorArrayI16    = 21,
    NiFpgaEx_ResourceType_IndicatorArrayU16    = 22,
    NiFpgaEx_ResourceType_IndicatorArrayI32    = 23,
    NiFpgaEx_ResourceType_IndicatorArrayU32    = 24,
    NiFpgaEx_ResourceType_IndicatorArrayI64    = 25,
    NiFpgaEx_ResourceType_IndicatorArrayU64    = 26,
    NiFpgaEx_ResourceType_ControlArrayBool     = 27,
    NiFpgaEx_ResourceType_ControlArrayI8       = 28,
    NiFpgaEx_ResourceType_ControlArrayU8       = 29,
    NiFpgaEx_ResourceType_ControlArrayI16      = 30,
    NiFpgaEx_ResourceType_ControlArrayU16      = 31,
    NiFpgaEx_ResourceType_ControlArrayI32      = 32,
    NiFpgaEx_ResourceType_ControlArrayU32      = 33,
    NiFpgaEx_ResourceType_ControlArrayI64      = 34,
    NiFpgaEx_ResourceType_ControlArrayU64      = 35,
    NiFpgaEx_ResourceType_TargetToHostFifoBool = 36,
    NiFpgaEx_ResourceType_TargetToHostFifoI8   = 37,
    NiFpgaEx_ResourceType_TargetToHostFifoU8   = 38,
    NiFpgaEx_ResourceType_TargetToHostFifoI16  = 39,
    NiFpgaEx_ResourceType_TargetToHostFifoU16  = 40,
    NiFpgaEx_ResourceType_TargetToHostFifoI32  = 41,
    NiFpgaEx_ResourceType_TargetToHostFifoU32  = 42,
    NiFpgaEx_ResourceType_TargetToHostFifoI64  = 43,
    NiFpgaEx_ResourceType_TargetToHostFifoU64  = 44,
    NiFpgaEx_ResourceType_HostToTargetFifoBool = 45,
    NiFpgaEx_ResourceType_HostToTargetFifoI8   = 46,
    NiFpgaEx_ResourceType_HostToTargetFifoU8   = 47,
    NiFpgaEx_ResourceType_HostToTargetFifoI16  = 48,
    NiFpgaEx_ResourceType_HostToTargetFifoU16  = 49,
    NiFpgaEx_ResourceType_HostToTargetFifoI32  = 50,
    NiFpgaEx_ResourceType_HostToTargetFifoU32  = 51,
    NiFpgaEx_ResourceType_HostToTargetFifoI64  = 52,
    NiFpgaEx_ResourceType_HostToTargetFifoU64  = 53,
    NiFpgaEx_ResourceType_IndicatorSgl         = 54,
    NiFpgaEx_ResourceType_ControlSgl           = 55,
    NiFpgaEx_ResourceType_IndicatorArraySgl    = 56,
    NiFpgaEx_ResourceType_ControlArraySgl      = 57,
    NiFpgaEx_ResourceType_TargetToHostFifoSgl  = 58,
    NiFpgaEx_ResourceType_HostToTargetFifoSgl  = 59,
    NiFpgaEx_ResourceType_IndicatorDbl         = 60,
    NiFpgaEx_ResourceType_ControlDbl           = 61,
    NiFpgaEx_ResourceType_IndicatorArrayDbl    = 62,
    NiFpgaEx_ResourceType_ControlArrayDbl      = 63,
    NiFpgaEx_ResourceType_TargetToHostFifoDbl  = 64,
    NiFpgaEx_ResourceType_HostToTargetFifoDbl  = 65,
    NiFpgaEx_ResourceType_Any                  = 0xFFFFFFFF
} NiFpgaEx_ResourceType;

/**
 * Finds a given resource by its name and type. If there is an ambiguity from
 * multiple matching resources, NiFpga_Status_InvalidResourceName will be
 * returned.
 *
 * @param session handle to a currently open session
 * @param name name of the resource
 * @param type type of the resource
 * @param resource outputs the resource if it was found
 * @return result of the call
 */
NiFpga_Status NiFpgaEx_FindResource(NiFpga_Session session,
    const char* name,
    NiFpgaEx_ResourceType type,
    NiFpgaEx_Resource* resource);

/**
 * Reads a boolean value from a given indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg indicator or control from which to read
 * @param value outputs the value that was read
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadBool(
    NiFpga_Session session, NiFpgaEx_RegisterBool reg, NiFpga_Bool* value);

/**
 * Reads a signed 8-bit integer value from a given indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg indicator or control from which to read
 * @param value outputs the value that was read
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadI8(
    NiFpga_Session session, NiFpgaEx_RegisterI8 reg, int8_t* value);

/**
 * Reads an unsigned 8-bit integer value from a given indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg indicator or control from which to read
 * @param value outputs the value that was read
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadU8(
    NiFpga_Session session, NiFpgaEx_RegisterU8 reg, uint8_t* value);

/**
 * Reads a signed 16-bit integer value from a given indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg indicator or control from which to read
 * @param value outputs the value that was read
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadI16(
    NiFpga_Session session, NiFpgaEx_RegisterI16 reg, int16_t* value);

/**
 * Reads an unsigned 16-bit integer value from a given indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg indicator or control from which to read
 * @param value outputs the value that was read
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadU16(
    NiFpga_Session session, NiFpgaEx_RegisterU16 reg, uint16_t* value);

/**
 * Reads a signed 32-bit integer value from a given indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg indicator or control from which to read
 * @param value outputs the value that was read
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadI32(
    NiFpga_Session session, NiFpgaEx_RegisterI32 reg, int32_t* value);

/**
 * Reads an unsigned 32-bit integer value from a given indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg indicator or control from which to read
 * @param value outputs the value that was read
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadU32(
    NiFpga_Session session, NiFpgaEx_RegisterU32 reg, uint32_t* value);

/**
 * Reads a signed 64-bit integer value from a given indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg indicator or control from which to read
 * @param value outputs the value that was read
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadI64(
    NiFpga_Session session, NiFpgaEx_RegisterI64 reg, int64_t* value);

/**
 * Reads an unsigned 64-bit integer value from a given indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg indicator or control from which to read
 * @param value outputs the value that was read
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadU64(
    NiFpga_Session session, NiFpgaEx_RegisterU64 reg, uint64_t* value);

/**
 * Reads an 32-bit float value from a given indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg indicator or control from which to read
 * @param value outputs the value that was read
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadSgl(
    NiFpga_Session session, NiFpgaEx_RegisterSgl reg, float* value);

/**
 * Reads a 64-bit double value from a given indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg indicator or control from which to read
 * @param value outputs the value that was read
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadDbl(
    NiFpga_Session session, NiFpgaEx_RegisterDbl reg, double* value);

/**
 * Writes a boolean value to a given control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg control or indicator to which to write
 * @param value value to write
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteBool(
    NiFpga_Session session, NiFpgaEx_RegisterBool reg, NiFpga_Bool value);

/**
 * Writes a signed 8-bit integer value to a given control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg control or indicator to which to write
 * @param value value to write
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteI8(
    NiFpga_Session session, NiFpgaEx_RegisterI8 reg, int8_t value);

/**
 * Writes an unsigned 8-bit integer value to a given control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg control or indicator to which to write
 * @param value value to write
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteU8(
    NiFpga_Session session, NiFpgaEx_RegisterU8 reg, uint8_t value);

/**
 * Writes a signed 16-bit integer value to a given control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg control or indicator to which to write
 * @param value value to write
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteI16(
    NiFpga_Session session, NiFpgaEx_RegisterI16 reg, int16_t value);

/**
 * Writes an unsigned 16-bit integer value to a given control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg control or indicator to which to write
 * @param value value to write
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteU16(
    NiFpga_Session session, NiFpgaEx_RegisterU16 reg, uint16_t value);

/**
 * Writes a signed 32-bit integer value to a given control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg control or indicator to which to write
 * @param value value to write
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteI32(
    NiFpga_Session session, NiFpgaEx_RegisterI32 reg, int32_t value);

/**
 * Writes an unsigned 32-bit integer value to a given control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg control or indicator to which to write
 * @param value value to write
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteU32(
    NiFpga_Session session, NiFpgaEx_RegisterU32 reg, uint32_t value);

/**
 * Writes a signed 64-bit integer value to a given control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg control or indicator to which to write
 * @param value value to write
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteI64(
    NiFpga_Session session, NiFpgaEx_RegisterI64 reg, int64_t value);

/**
 * Writes an unsigned 64-bit integer value to a given control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg control or indicator to which to write
 * @param value value to write
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteU64(
    NiFpga_Session session, NiFpgaEx_RegisterU64 reg, uint64_t value);

/**
 * Writes 32-bit float value to a given control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg control or indicator to which to write
 * @param value value to write
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteSgl(
    NiFpga_Session session, NiFpgaEx_RegisterSgl reg, float value);

/**
 * Writes 64-bit double value to a given control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg control or indicator to which to write
 * @param value value to write
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteDbl(
    NiFpga_Session session, NiFpgaEx_RegisterDbl reg, double value);

/**
 * Reads an entire array of boolean values from a given array indicator or
 * control.
 *
 * @warning The size passed must be the exact number of elements in the
 *          indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg array indicator or control from which to read
 * @param array outputs the entire array that was read
 * @param size exact number of elements in the indicator or control
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadArrayBool(NiFpga_Session session,
    NiFpgaEx_RegisterArrayBool reg,
    NiFpga_Bool* array,
    size_t size);

/**
 * Reads an entire array of signed 8-bit integer values from a given array
 * indicator or control.
 *
 * @warning The size passed must be the exact number of elements in the
 *          indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg array indicator or control from which to read
 * @param array outputs the entire array that was read
 * @param size exact number of elements in the indicator or control
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadArrayI8(
    NiFpga_Session session, NiFpgaEx_RegisterArrayI8 reg, int8_t* array, size_t size);

/**
 * Reads an entire array of unsigned 8-bit integer values from a given array
 * indicator or control.
 *
 * @warning The size passed must be the exact number of elements in the
 *          indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg array indicator or control from which to read
 * @param array outputs the entire array that was read
 * @param size exact number of elements in the indicator or control
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadArrayU8(
    NiFpga_Session session, NiFpgaEx_RegisterArrayU8 reg, uint8_t* array, size_t size);

/**
 * Reads an entire array of signed 16-bit integer values from a given array
 * indicator or control.
 *
 * @warning The size passed must be the exact number of elements in the
 *          indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg array indicator or control from which to read
 * @param array outputs the entire array that was read
 * @param size exact number of elements in the indicator or control
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadArrayI16(
    NiFpga_Session session, NiFpgaEx_RegisterArrayI16 reg, int16_t* array, size_t size);

/**
 * Reads an entire array of unsigned 16-bit integer values from a given array
 * indicator or control.
 *
 * @warning The size passed must be the exact number of elements in the
 *          indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg array indicator or control from which to read
 * @param array outputs the entire array that was read
 * @param size exact number of elements in the indicator or control
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadArrayU16(
    NiFpga_Session session, NiFpgaEx_RegisterArrayU16 reg, uint16_t* array, size_t size);

/**
 * Reads an entire array of signed 32-bit integer values from a given array
 * indicator or control.
 *
 * @warning The size passed must be the exact number of elements in the
 *          indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg array indicator or control from which to read
 * @param array outputs the entire array that was read
 * @param size exact number of elements in the indicator or control
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadArrayI32(
    NiFpga_Session session, NiFpgaEx_RegisterArrayI32 reg, int32_t* array, size_t size);

/**
 * Reads an entire array of unsigned 32-bit integer values from a given array
 * indicator or control.
 *
 * @warning The size passed must be the exact number of elements in the
 *          indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg array indicator or control from which to read
 * @param array outputs the entire array that was read
 * @param size exact number of elements in the indicator or control
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadArrayU32(
    NiFpga_Session session, NiFpgaEx_RegisterArrayU32 reg, uint32_t* array, size_t size);

/**
 * Reads an entire array of signed 64-bit integer values from a given array
 * indicator or control.
 *
 * @warning The size passed must be the exact number of elements in the
 *          indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg array indicator or control from which to read
 * @param array outputs the entire array that was read
 * @param size exact number of elements in the indicator or control
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadArrayI64(
    NiFpga_Session session, NiFpgaEx_RegisterArrayI64 reg, int64_t* array, size_t size);

/**
 * Reads an entire array of unsigned 64-bit integer values from a given array
 * indicator or control.
 *
 * @warning The size passed must be the exact number of elements in the
 *          indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg array indicator or control from which to read
 * @param array outputs the entire array that was read
 * @param size exact number of elements in the indicator or control
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadArrayU64(
    NiFpga_Session session, NiFpgaEx_RegisterArrayU64 reg, uint64_t* array, size_t size);

/**
 * Reads an entire array of 32-bit float values from a given array
 * indicator or control.
 *
 * @warning The size passed must be the exact number of elements in the
 *          indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg array indicator or control from which to read
 * @param array outputs the entire array that was read
 * @param size exact number of elements in the indicator or control
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadArraySgl(
    NiFpga_Session session, NiFpgaEx_RegisterArraySgl reg, float* array, size_t size);

/**
 * Reads an entire array of 64-bit double values from a given array
 * indicator or control.
 *
 * @warning The size passed must be the exact number of elements in the
 *          indicator or control.
 *
 * @param session handle to a currently open session
 * @param reg array indicator or control from which to read
 * @param array outputs the entire array that was read
 * @param size exact number of elements in the indicator or control
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadArrayDbl(
    NiFpga_Session session, NiFpgaEx_RegisterArrayDbl reg, double* array, size_t size);

/**
 * Writes an entire array of boolean values to a given array control or
 * indicator.
 *
 * @warning The size passed must be the exact number of elements in the
 *          control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg array control or indicator to which to write
 * @param array entire array to write
 * @param size exact number of elements in the control or indicator
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteArrayBool(NiFpga_Session session,
    NiFpgaEx_RegisterArrayBool reg,
    const NiFpga_Bool* array,
    size_t size);

/**
 * Writes an entire array of signed 8-bit integer values to a given array
 * control or indicator.
 *
 * @warning The size passed must be the exact number of elements in the
 *          control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg array control or indicator to which to write
 * @param array entire array to write
 * @param size exact number of elements in the control or indicator
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteArrayI8(NiFpga_Session session,
    NiFpgaEx_RegisterArrayI8 reg,
    const int8_t* array,
    size_t size);

/**
 * Writes an entire array of unsigned 8-bit integer values to a given array
 * control or indicator.
 *
 * @warning The size passed must be the exact number of elements in the
 *          control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg array control or indicator to which to write
 * @param array entire array to write
 * @param size exact number of elements in the control or indicator
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteArrayU8(NiFpga_Session session,
    NiFpgaEx_RegisterArrayU8 reg,
    const uint8_t* array,
    size_t size);

/**
 * Writes an entire array of signed 16-bit integer values to a given array
 * control or indicator.
 *
 * @warning The size passed must be the exact number of elements in the
 *          control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg array control or indicator to which to write
 * @param array entire array to write
 * @param size exact number of elements in the control or indicator
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteArrayI16(NiFpga_Session session,
    NiFpgaEx_RegisterArrayI16 reg,
    const int16_t* array,
    size_t size);

/**
 * Writes an entire array of unsigned 16-bit integer values to a given array
 * control or indicator.
 *
 * @warning The size passed must be the exact number of elements in the
 *          control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg array control or indicator to which to write
 * @param array entire array to write
 * @param size exact number of elements in the control or indicator
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteArrayU16(NiFpga_Session session,
    NiFpgaEx_RegisterArrayU16 reg,
    const uint16_t* array,
    size_t size);

/**
 * Writes an entire array of signed 32-bit integer values to a given array
 * control or indicator.
 *
 * @warning The size passed must be the exact number of elements in the
 *          control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg array control or indicator to which to write
 * @param array entire array to write
 * @param size exact number of elements in the control or indicator
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteArrayI32(NiFpga_Session session,
    NiFpgaEx_RegisterArrayI32 reg,
    const int32_t* array,
    size_t size);

/**
 * Writes an entire array of unsigned 32-bit integer values to a given array
 * control or indicator.
 *
 * @warning The size passed must be the exact number of elements in the
 *          control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg array control or indicator to which to write
 * @param array entire array to write
 * @param size exact number of elements in the control or indicator
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteArrayU32(NiFpga_Session session,
    NiFpgaEx_RegisterArrayU32 reg,
    const uint32_t* array,
    size_t size);

/**
 * Writes an entire array of signed 64-bit integer values to a given array
 * control or indicator.
 *
 * @warning The size passed must be the exact number of elements in the
 *          control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg array control or indicator to which to write
 * @param array entire array to write
 * @param size exact number of elements in the control or indicator
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteArrayI64(NiFpga_Session session,
    NiFpgaEx_RegisterArrayI64 reg,
    const int64_t* array,
    size_t size);

/**
 * Writes an entire array of unsigned 64-bit integer values to a given array
 * control or indicator.
 *
 * @warning The size passed must be the exact number of elements in the
 *          control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg array control or indicator to which to write
 * @param array entire array to write
 * @param size exact number of elements in the control or indicator
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteArrayU64(NiFpga_Session session,
    NiFpgaEx_RegisterArrayU64 reg,
    const uint64_t* array,
    size_t size);

/**
 * Writes an entire array of 32-bit float values to a given array
 * control or indicator.
 *
 * @warning The size passed must be the exact number of elements in the
 *          control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg array control or indicator to which to write
 * @param array entire array to write
 * @param size exact number of elements in the control or indicator
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteArraySgl(NiFpga_Session session,
    NiFpgaEx_RegisterArraySgl reg,
    const float* array,
    size_t size);

/**
 * Writes an entire array of 64-bit double values to a given array
 * control or indicator.
 *
 * @warning The size passed must be the exact number of elements in the
 *          control or indicator.
 *
 * @param session handle to a currently open session
 * @param reg array control or indicator to which to write
 * @param array entire array to write
 * @param size exact number of elements in the control or indicator
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteArrayDbl(NiFpga_Session session,
    NiFpgaEx_RegisterArrayDbl reg,
    const double* array,
    size_t size);

/**
 * Enumeration of all 32 possible IRQs. Multiple IRQs can be bitwise ORed
 * together like this:
 *
 *    NiFpga_Irq_3 | NiFpga_Irq_23
 */
typedef enum {
    NiFpga_Irq_0  = 1 << 0,
    NiFpga_Irq_1  = 1 << 1,
    NiFpga_Irq_2  = 1 << 2,
    NiFpga_Irq_3  = 1 << 3,
    NiFpga_Irq_4  = 1 << 4,
    NiFpga_Irq_5  = 1 << 5,
    NiFpga_Irq_6  = 1 << 6,
    NiFpga_Irq_7  = 1 << 7,
    NiFpga_Irq_8  = 1 << 8,
    NiFpga_Irq_9  = 1 << 9,
    NiFpga_Irq_10 = 1 << 10,
    NiFpga_Irq_11 = 1 << 11,
    NiFpga_Irq_12 = 1 << 12,
    NiFpga_Irq_13 = 1 << 13,
    NiFpga_Irq_14 = 1 << 14,
    NiFpga_Irq_15 = 1 << 15,
    NiFpga_Irq_16 = 1 << 16,
    NiFpga_Irq_17 = 1 << 17,
    NiFpga_Irq_18 = 1 << 18,
    NiFpga_Irq_19 = 1 << 19,
    NiFpga_Irq_20 = 1 << 20,
    NiFpga_Irq_21 = 1 << 21,
    NiFpga_Irq_22 = 1 << 22,
    NiFpga_Irq_23 = 1 << 23,
    NiFpga_Irq_24 = 1 << 24,
    NiFpga_Irq_25 = 1 << 25,
    NiFpga_Irq_26 = 1 << 26,
    NiFpga_Irq_27 = 1 << 27,
    NiFpga_Irq_28 = 1 << 28,
    NiFpga_Irq_29 = 1 << 29,
    NiFpga_Irq_30 = 1 << 30,
    NiFpga_Irq_31 = 1U << 31
} NiFpga_Irq;

/**
 * Represents an infinite timeout.
 */
static const uint32_t NiFpga_InfiniteTimeout = 0xFFFFFFFF;

/**
 * See NiFpga_ReserveIrqContext for more information.
 */
typedef void* NiFpga_IrqContext;

/**
 * IRQ contexts are single-threaded; only one thread can wait with a particular
 * context at any given time. Clients must reserve as many contexts as the
 * application requires.
 *
 * If a context is successfully reserved (the returned status is not an error),
 * it must be unreserved later. Otherwise a memory leak will occur.
 *
 * @param session handle to a currently open session
 * @param context outputs the IRQ context
 * @return result of the call
 */
NiFpga_Status NiFpga_ReserveIrqContext(
    NiFpga_Session session, NiFpga_IrqContext* context);

/**
 * Unreserves an IRQ context obtained from NiFpga_ReserveIrqContext.
 *
 * @param session handle to a currently open session
 * @param context IRQ context to unreserve
 * @return result of the call
 */
NiFpga_Status NiFpga_UnreserveIrqContext(
    NiFpga_Session session, NiFpga_IrqContext context);

/**
 * This is a blocking function that stops the calling thread until the FPGA
 * asserts any IRQ in the irqs parameter, or until the function call times out.
 * Before calling this function, you must use NiFpga_ReserveIrqContext to
 * reserve an IRQ context. No other threads can use the same context when this
 * function is called.
 *
 * You can use the irqsAsserted parameter to determine which IRQs were asserted
 * for each function call.
 *
 * @param session handle to a currently open session
 * @param context IRQ context with which to wait
 * @param irqs bitwise OR of NiFpga_Irqs
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param irqsAsserted if non-NULL, outputs bitwise OR of IRQs that were
 *                     asserted
 * @param timedOut if non-NULL, outputs whether the timeout expired
 * @return result of the call
 */
NiFpga_Status NiFpga_WaitOnIrqs(NiFpga_Session session,
    NiFpga_IrqContext context,
    uint32_t irqs,
    uint32_t timeout,
    uint32_t* irqsAsserted,
    NiFpga_Bool* timedOut);

/**
 * Acknowledges an IRQ or set of IRQs.
 *
 * @param session handle to a currently open session
 * @param irqs bitwise OR of NiFpga_Irqs
 * @return result of the call
 */
NiFpga_Status NiFpga_AcknowledgeIrqs(NiFpga_Session session, uint32_t irqs);

/**
 * Specifies the depth of the host memory part of the DMA FIFO. This method is
 * optional. In order to see the actual depth configured, use
 * NiFpga_ConfigureFifo2.
 *
 * @param session handle to a currently open session
 * @param fifo FIFO to configure
 * @param depth requested number of elements in the host memory part of the
 *              DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ConfigureFifo(
    NiFpga_Session session, NiFpgaEx_DmaFifo fifo, size_t depth);

/**
 * Specifies the depth of the host memory part of the DMA FIFO. This method is
 * optional.
 *
 * @param session handle to a currently open session
 * @param fifo FIFO to configure
 * @param requestedDepth requested number of elements in the host memory part
 *                       of the DMA FIFO
 * @param actualDepth if non-NULL, outputs the actual number of elements in the
 *                    host memory part of the DMA FIFO, which may be more than
 *                    the requested number
 * @return result of the call
 */
NiFpga_Status NiFpga_ConfigureFifo2(NiFpga_Session session,
    NiFpgaEx_DmaFifo fifo,
    size_t requestedDepth,
    size_t* actualDepth);

/**
 * Starts a FIFO. This method is optional.
 *
 * @param session handle to a currently open session
 * @param fifo FIFO to start
 * @return result of the call
 */
NiFpga_Status NiFpga_StartFifo(NiFpga_Session session, NiFpgaEx_DmaFifo fifo);

/**
 * Stops a FIFO. This method is optional.
 *
 * @param session handle to a currently open session
 * @param fifo FIFO to stop
 * @return result of the call
 */
NiFpga_Status NiFpga_StopFifo(NiFpga_Session session, NiFpgaEx_DmaFifo fifo);

/**
 * Reads from a target-to-host FIFO of booleans.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param data outputs the data that was read
 * @param numberOfElements number of elements to read
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadFifoBool(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoBool fifo,
    NiFpga_Bool* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* elementsRemaining);

/**
 * Reads from a target-to-host FIFO of signed 8-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param data outputs the data that was read
 * @param numberOfElements number of elements to read
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadFifoI8(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoI8 fifo,
    int8_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* elementsRemaining);

/**
 * Reads from a target-to-host FIFO of unsigned 8-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param data outputs the data that was read
 * @param numberOfElements number of elements to read
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadFifoU8(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoU8 fifo,
    uint8_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* elementsRemaining);

/**
 * Reads from a target-to-host FIFO of signed 16-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param data outputs the data that was read
 * @param numberOfElements number of elements to read
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadFifoI16(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoI16 fifo,
    int16_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* elementsRemaining);

/**
 * Reads from a target-to-host FIFO of unsigned 16-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param data outputs the data that was read
 * @param numberOfElements number of elements to read
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadFifoU16(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoU16 fifo,
    uint16_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* elementsRemaining);

/**
 * Reads from a target-to-host FIFO of signed 32-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param data outputs the data that was read
 * @param numberOfElements number of elements to read
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadFifoI32(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoI32 fifo,
    int32_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* elementsRemaining);

/**
 * Reads from a target-to-host FIFO of unsigned 32-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param data outputs the data that was read
 * @param numberOfElements number of elements to read
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadFifoU32(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoU32 fifo,
    uint32_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* elementsRemaining);

/**
 * Reads from a target-to-host FIFO of signed 64-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param data outputs the data that was read
 * @param numberOfElements number of elements to read
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadFifoI64(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoI64 fifo,
    int64_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* elementsRemaining);

/**
 * Reads from a target-to-host FIFO of unsigned 64-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param data outputs the data that was read
 * @param numberOfElements number of elements to read
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadFifoU64(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoU64 fifo,
    uint64_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* elementsRemaining);

/**
 * Reads from a target-to-host FIFO of 32-bit floats.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param data outputs the data that was read
 * @param numberOfElements number of elements to read
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadFifoSgl(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoSgl fifo,
    float* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* elementsRemaining);
/**
 * Reads from a target-to-host FIFO of 64-bit doubles.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param data outputs the data that was read
 * @param numberOfElements number of elements to read
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_ReadFifoDbl(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoDbl fifo,
    double* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* elementsRemaining);

/**
 * Writes to a host-to-target FIFO of booleans.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param data data to write
 * @param numberOfElements number of elements to write
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param emptyElementsRemaining if non-NULL, outputs the number of empty
 *                               elements remaining in the host memory part of
 *                               the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteFifoBool(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoBool fifo,
    const NiFpga_Bool* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* emptyElementsRemaining);

/**
 * Writes to a host-to-target FIFO of signed 8-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param data data to write
 * @param numberOfElements number of elements to write
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param emptyElementsRemaining if non-NULL, outputs the number of empty
 *                               elements remaining in the host memory part of
 *                               the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteFifoI8(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoI8 fifo,
    const int8_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* emptyElementsRemaining);

/**
 * Writes to a host-to-target FIFO of unsigned 8-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param data data to write
 * @param numberOfElements number of elements to write
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param emptyElementsRemaining if non-NULL, outputs the number of empty
 *                               elements remaining in the host memory part of
 *                               the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteFifoU8(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoU8 fifo,
    const uint8_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* emptyElementsRemaining);

/**
 * Writes to a host-to-target FIFO of signed 16-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param data data to write
 * @param numberOfElements number of elements to write
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param emptyElementsRemaining if non-NULL, outputs the number of empty
 *                               elements remaining in the host memory part of
 *                               the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteFifoI16(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoI16 fifo,
    const int16_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* emptyElementsRemaining);

/**
 * Writes to a host-to-target FIFO of unsigned 16-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param data data to write
 * @param numberOfElements number of elements to write
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param emptyElementsRemaining if non-NULL, outputs the number of empty
 *                               elements remaining in the host memory part of
 *                               the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteFifoU16(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoU16 fifo,
    const uint16_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* emptyElementsRemaining);

/**
 * Writes to a host-to-target FIFO of signed 32-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param data data to write
 * @param numberOfElements number of elements to write
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param emptyElementsRemaining if non-NULL, outputs the number of empty
 *                               elements remaining in the host memory part of
 *                               the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteFifoI32(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoI32 fifo,
    const int32_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* emptyElementsRemaining);

/**
 * Writes to a host-to-target FIFO of unsigned 32-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param data data to write
 * @param numberOfElements number of elements to write
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param emptyElementsRemaining if non-NULL, outputs the number of empty
 *                               elements remaining in the host memory part of
 *                               the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteFifoU32(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoU32 fifo,
    const uint32_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* emptyElementsRemaining);

/**
 * Writes to a host-to-target FIFO of signed 64-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param data data to write
 * @param numberOfElements number of elements to write
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param emptyElementsRemaining if non-NULL, outputs the number of empty
 *                               elements remaining in the host memory part of
 *                               the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteFifoI64(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoI64 fifo,
    const int64_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* emptyElementsRemaining);

/**
 * Writes to a host-to-target FIFO of unsigned 64-bit integers.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param data data to write
 * @param numberOfElements number of elements to write
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param emptyElementsRemaining if non-NULL, outputs the number of empty
 *                               elements remaining in the host memory part of
 *                               the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteFifoU64(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoU64 fifo,
    const uint64_t* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* emptyElementsRemaining);

/**
 * Writes to a host-to-target FIFO of 32-bit floats.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param data data to write
 * @param numberOfElements number of elements to write
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param emptyElementsRemaining if non-NULL, outputs the number of empty
 *                               elements remaining in the host memory part of
 *                               the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteFifoSgl(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoSgl fifo,
    const float* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* emptyElementsRemaining);

/**
 * Writes to a host-to-target FIFO of 64-bit doubles.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param data data to write
 * @param numberOfElements number of elements to write
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param emptyElementsRemaining if non-NULL, outputs the number of empty
 *                               elements remaining in the host memory part of
 *                               the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_WriteFifoDbl(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoDbl fifo,
    const double* data,
    size_t numberOfElements,
    uint32_t timeout,
    size_t* emptyElementsRemaining);

/**
 * Acquires elements for reading from a target-to-host FIFO of booleans.
 *
 * Acquiring, reading, and releasing FIFO elements prevents the need to copy
 * the contents of elements from the host memory buffer to a separate
 * user-allocated buffer before reading. The FPGA target cannot write to
 * elements acquired by the host. Therefore, the host must release elements
 * after reading them. The number of elements acquired may differ from the
 * number of elements requested if, for example, the number of elements
 * requested reaches the end of the host memory buffer. Always release all
 * acquired elements before closing the session. Do not attempt to access FIFO
 * elements after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoReadElementsBool(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoBool fifo,
    NiFpga_Bool** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for reading from a target-to-host FIFO of signed 8-bit
 * integers.
 *
 * Acquiring, reading, and releasing FIFO elements prevents the need to copy
 * the contents of elements from the host memory buffer to a separate
 * user-allocated buffer before reading. The FPGA target cannot write to
 * elements acquired by the host. Therefore, the host must release elements
 * after reading them. The number of elements acquired may differ from the
 * number of elements requested if, for example, the number of elements
 * requested reaches the end of the host memory buffer. Always release all
 * acquired elements before closing the session. Do not attempt to access FIFO
 * elements after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoReadElementsI8(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoI8 fifo,
    int8_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for reading from a target-to-host FIFO of unsigned 8-bit
 * integers.
 *
 * Acquiring, reading, and releasing FIFO elements prevents the need to copy
 * the contents of elements from the host memory buffer to a separate
 * user-allocated buffer before reading. The FPGA target cannot write to
 * elements acquired by the host. Therefore, the host must release elements
 * after reading them. The number of elements acquired may differ from the
 * number of elements requested if, for example, the number of elements
 * requested reaches the end of the host memory buffer. Always release all
 * acquired elements before closing the session. Do not attempt to access FIFO
 * elements after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoReadElementsU8(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoU8 fifo,
    uint8_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for reading from a target-to-host FIFO of signed 16-bit
 * integers.
 *
 * Acquiring, reading, and releasing FIFO elements prevents the need to copy
 * the contents of elements from the host memory buffer to a separate
 * user-allocated buffer before reading. The FPGA target cannot write to
 * elements acquired by the host. Therefore, the host must release elements
 * after reading them. The number of elements acquired may differ from the
 * number of elements requested if, for example, the number of elements
 * requested reaches the end of the host memory buffer. Always release all
 * acquired elements before closing the session. Do not attempt to access FIFO
 * elements after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoReadElementsI16(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoI16 fifo,
    int16_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for reading from a target-to-host FIFO of unsigned 16-bit
 * integers.
 *
 * Acquiring, reading, and releasing FIFO elements prevents the need to copy
 * the contents of elements from the host memory buffer to a separate
 * user-allocated buffer before reading. The FPGA target cannot write to
 * elements acquired by the host. Therefore, the host must release elements
 * after reading them. The number of elements acquired may differ from the
 * number of elements requested if, for example, the number of elements
 * requested reaches the end of the host memory buffer. Always release all
 * acquired elements before closing the session. Do not attempt to access FIFO
 * elements after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoReadElementsU16(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoU16 fifo,
    uint16_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for reading from a target-to-host FIFO of signed 32-bit
 * integers.
 *
 * Acquiring, reading, and releasing FIFO elements prevents the need to copy
 * the contents of elements from the host memory buffer to a separate
 * user-allocated buffer before reading. The FPGA target cannot write to
 * elements acquired by the host. Therefore, the host must release elements
 * after reading them. The number of elements acquired may differ from the
 * number of elements requested if, for example, the number of elements
 * requested reaches the end of the host memory buffer. Always release all
 * acquired elements before closing the session. Do not attempt to access FIFO
 * elements after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoReadElementsI32(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoI32 fifo,
    int32_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for reading from a target-to-host FIFO of unsigned 32-bit
 * integers.
 *
 * Acquiring, reading, and releasing FIFO elements prevents the need to copy
 * the contents of elements from the host memory buffer to a separate
 * user-allocated buffer before reading. The FPGA target cannot write to
 * elements acquired by the host. Therefore, the host must release elements
 * after reading them. The number of elements acquired may differ from the
 * number of elements requested if, for example, the number of elements
 * requested reaches the end of the host memory buffer. Always release all
 * acquired elements before closing the session. Do not attempt to access FIFO
 * elements after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoReadElementsU32(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoU32 fifo,
    uint32_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for reading from a target-to-host FIFO of signed 64-bit
 * integers.
 *
 * Acquiring, reading, and releasing FIFO elements prevents the need to copy
 * the contents of elements from the host memory buffer to a separate
 * user-allocated buffer before reading. The FPGA target cannot write to
 * elements acquired by the host. Therefore, the host must release elements
 * after reading them. The number of elements acquired may differ from the
 * number of elements requested if, for example, the number of elements
 * requested reaches the end of the host memory buffer. Always release all
 * acquired elements before closing the session. Do not attempt to access FIFO
 * elements after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoReadElementsI64(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoI64 fifo,
    int64_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for reading from a target-to-host FIFO of unsigned 64-bit
 * integers.
 *
 * Acquiring, reading, and releasing FIFO elements prevents the need to copy
 * the contents of elements from the host memory buffer to a separate
 * user-allocated buffer before reading. The FPGA target cannot write to
 * elements acquired by the host. Therefore, the host must release elements
 * after reading them. The number of elements acquired may differ from the
 * number of elements requested if, for example, the number of elements
 * requested reaches the end of the host memory buffer. Always release all
 * acquired elements before closing the session. Do not attempt to access FIFO
 * elements after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoReadElementsU64(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoU64 fifo,
    uint64_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for reading from a target-to-host FIFO of 32-bit
 * floats.
 *
 * Acquiring, reading, and releasing FIFO elements prevents the need to copy
 * the contents of elements from the host memory buffer to a separate
 * user-allocated buffer before reading. The FPGA target cannot write to
 * elements acquired by the host. Therefore, the host must release elements
 * after reading them. The number of elements acquired may differ from the
 * number of elements requested if, for example, the number of elements
 * requested reaches the end of the host memory buffer. Always release all
 * acquired elements before closing the session. Do not attempt to access FIFO
 * elements after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoReadElementsSgl(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoSgl fifo,
    float** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for reading from a target-to-host FIFO of 64-bit
 * doubles.
 *
 * Acquiring, reading, and releasing FIFO elements prevents the need to copy
 * the contents of elements from the host memory buffer to a separate
 * user-allocated buffer before reading. The FPGA target cannot write to
 * elements acquired by the host. Therefore, the host must release elements
 * after reading them. The number of elements acquired may differ from the
 * number of elements requested if, for example, the number of elements
 * requested reaches the end of the host memory buffer. Always release all
 * acquired elements before closing the session. Do not attempt to access FIFO
 * elements after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo target-to-host FIFO from which to read
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoReadElementsDbl(NiFpga_Session session,
    NiFpgaEx_TargetToHostFifoDbl fifo,
    double** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for writing to a host-to-target FIFO of booleans.
 *
 * Acquiring, writing, and releasing FIFO elements prevents the need to write
 * first into a separate user-allocated buffer and then copy the contents of
 * elements to the host memory buffer. The FPGA target cannot read elements
 * acquired by the host. Therefore, the host must release elements after
 * writing to them. The number of elements acquired may differ from the number
 * of elements requested if, for example, the number of elements requested
 * reaches the end of the host memory buffer. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoWriteElementsBool(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoBool fifo,
    NiFpga_Bool** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for writing to a host-to-target FIFO of signed 8-bit
 * integers.
 *
 * Acquiring, writing, and releasing FIFO elements prevents the need to write
 * first into a separate user-allocated buffer and then copy the contents of
 * elements to the host memory buffer. The FPGA target cannot read elements
 * acquired by the host. Therefore, the host must release elements after
 * writing to them. The number of elements acquired may differ from the number
 * of elements requested if, for example, the number of elements requested
 * reaches the end of the host memory buffer. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoWriteElementsI8(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoI8 fifo,
    int8_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for writing to a host-to-target FIFO of unsigned 8-bit
 * integers.
 *
 * Acquiring, writing, and releasing FIFO elements prevents the need to write
 * first into a separate user-allocated buffer and then copy the contents of
 * elements to the host memory buffer. The FPGA target cannot read elements
 * acquired by the host. Therefore, the host must release elements after
 * writing to them. The number of elements acquired may differ from the number
 * of elements requested if, for example, the number of elements requested
 * reaches the end of the host memory buffer. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoWriteElementsU8(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoU8 fifo,
    uint8_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for writing to a host-to-target FIFO of signed 16-bit
 * integers.
 *
 * Acquiring, writing, and releasing FIFO elements prevents the need to write
 * first into a separate user-allocated buffer and then copy the contents of
 * elements to the host memory buffer. The FPGA target cannot read elements
 * acquired by the host. Therefore, the host must release elements after
 * writing to them. The number of elements acquired may differ from the number
 * of elements requested if, for example, the number of elements requested
 * reaches the end of the host memory buffer. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoWriteElementsI16(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoI16 fifo,
    int16_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for writing to a host-to-target FIFO of unsigned 16-bit
 * integers.
 *
 * Acquiring, writing, and releasing FIFO elements prevents the need to write
 * first into a separate user-allocated buffer and then copy the contents of
 * elements to the host memory buffer. The FPGA target cannot read elements
 * acquired by the host. Therefore, the host must release elements after
 * writing to them. The number of elements acquired may differ from the number
 * of elements requested if, for example, the number of elements requested
 * reaches the end of the host memory buffer. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoWriteElementsU16(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoU16 fifo,
    uint16_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for writing to a host-to-target FIFO of signed 32-bit
 * integers.
 *
 * Acquiring, writing, and releasing FIFO elements prevents the need to write
 * first into a separate user-allocated buffer and then copy the contents of
 * elements to the host memory buffer. The FPGA target cannot read elements
 * acquired by the host. Therefore, the host must release elements after
 * writing to them. The number of elements acquired may differ from the number
 * of elements requested if, for example, the number of elements requested
 * reaches the end of the host memory buffer. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoWriteElementsI32(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoI32 fifo,
    int32_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for writing to a host-to-target FIFO of unsigned 32-bit
 * integers.
 *
 * Acquiring, writing, and releasing FIFO elements prevents the need to write
 * first into a separate user-allocated buffer and then copy the contents of
 * elements to the host memory buffer. The FPGA target cannot read elements
 * acquired by the host. Therefore, the host must release elements after
 * writing to them. The number of elements acquired may differ from the number
 * of elements requested if, for example, the number of elements requested
 * reaches the end of the host memory buffer. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoWriteElementsU32(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoU32 fifo,
    uint32_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for writing to a host-to-target FIFO of signed 64-bit
 * integers.
 *
 * Acquiring, writing, and releasing FIFO elements prevents the need to write
 * first into a separate user-allocated buffer and then copy the contents of
 * elements to the host memory buffer. The FPGA target cannot read elements
 * acquired by the host. Therefore, the host must release elements after
 * writing to them. The number of elements acquired may differ from the number
 * of elements requested if, for example, the number of elements requested
 * reaches the end of the host memory buffer. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoWriteElementsI64(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoI64 fifo,
    int64_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for writing to a host-to-target FIFO of unsigned 64-bit
 * integers.
 *
 * Acquiring, writing, and releasing FIFO elements prevents the need to write
 * first into a separate user-allocated buffer and then copy the contents of
 * elements to the host memory buffer. The FPGA target cannot read elements
 * acquired by the host. Therefore, the host must release elements after
 * writing to them. The number of elements acquired may differ from the number
 * of elements requested if, for example, the number of elements requested
 * reaches the end of the host memory buffer. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoWriteElementsU64(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoU64 fifo,
    uint64_t** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for writing to a host-to-target FIFO of 32-bit
 * floats.
 *
 * Acquiring, writing, and releasing FIFO elements prevents the need to write
 * first into a separate user-allocated buffer and then copy the contents of
 * elements to the host memory buffer. The FPGA target cannot read elements
 * acquired by the host. Therefore, the host must release elements after
 * writing to them. The number of elements acquired may differ from the number
 * of elements requested if, for example, the number of elements requested
 * reaches the end of the host memory buffer. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoWriteElementsSgl(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoSgl fifo,
    float** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Acquires elements for writing to a host-to-target FIFO of 64-bit
 * doubles.
 *
 * Acquiring, writing, and releasing FIFO elements prevents the need to write
 * first into a separate user-allocated buffer and then copy the contents of
 * elements to the host memory buffer. The FPGA target cannot read elements
 * acquired by the host. Therefore, the host must release elements after
 * writing to them. The number of elements acquired may differ from the number
 * of elements requested if, for example, the number of elements requested
 * reaches the end of the host memory buffer. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo host-to-target FIFO to which to write
 * @param elements outputs a pointer to the elements acquired
 * @param elementsRequested requested number of elements
 * @param timeout timeout in milliseconds, or NiFpga_InfiniteTimeout
 * @param elementsAcquired actual number of elements acquired, which may be
 *                         less than the requested number
 * @param elementsRemaining if non-NULL, outputs the number of elements
 *                          remaining in the host memory part of the DMA FIFO
 * @return result of the call
 */
NiFpga_Status NiFpga_AcquireFifoWriteElementsDbl(NiFpga_Session session,
    NiFpgaEx_HostToTargetFifoDbl fifo,
    double** elements,
    size_t elementsRequested,
    uint32_t timeout,
    size_t* elementsAcquired,
    size_t* elementsRemaining);

/**
 * Releases previously acquired FIFO elements.
 *
 * The FPGA target cannot read elements acquired by the host. Therefore, the
 * host must release elements after acquiring them. Always release all acquired
 * elements before closing the session. Do not attempt to access FIFO elements
 * after the elements are released or the session is closed.
 *
 * @param session handle to a currently open session
 * @param fifo FIFO from which to release elements
 * @param elements number of elements to release
 * @return result of the call
 */
NiFpga_Status NiFpga_ReleaseFifoElements(
    NiFpga_Session session, NiFpgaEx_DmaFifo fifo, size_t elements);

#if defined(NiFpga_Cpp)
}
#endif

/* Any symbols following this include will not be affected. */
#if defined(__GNUC__)
#    pragma GCC visibility pop
#endif
