/**
 * @file procinfo/procinfo.c
 *
 * Yori shell child process timer tool
 *
 * Copyright (c) 2019 Malcolm J. Smith
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <yoripch.h>
#include <yorilib.h>
#ifdef YORI_BUILTIN
#include <yoricall.h>
#endif

/**
 Help text to display to the user.
 */
const
CHAR strProcInfoHelpText[] =
        "\n"
        "Collects information about a running system process.\n"
        "\n"
        "PROCINFO [-license] [-f <fmt>] <pid>\n"
        "\n"
        "Format specifiers are:\n"
        "   $COMMIT$        Amount of Kb of memory committed by the process\n"
        "   $CPU$           Amount of CPU time used by the process\n"
        "   $CPUMS$         Amount of CPU time used by the process in ms\n"
        "   $CPUKERNEL$     Amount of kernel time used by the process\n"
        "   $CPUKERNELMS$   Amount of kernel time used by the process in ms\n"
        "   $CPUUSER$       Amount of user time used by the process\n"
        "   $CPUUSERMS$     Amount of user time used by the process in ms\n"
        "   $ELAPSED$       Amount of time the process has been in the system\n"
        "   $ELAPSEDMS$     Amount of time the process has been in the system in ms\n"
        "   $OTHERIOBYTES$  Number of bytes transferred by other IO requests in the process\n"
        "   $OTHERIOCOUNT$  Number of other IO operations in the process\n"
        "   $READCOUNT$     Number of read operations generated by the process\n"
        "   $READBYTES$     Number of bytes read by the process\n"
        "   $READCOUNT$     Number of read operations generated by the process\n"
        "   $WORKINGSET$    Amount of Kb of memory in the process working set\n"
        "   $WRITEBYTES$    Number of bytes written by the process\n"
        "   $WRITECOUNT$    Number of write operations generated by the process\n";

/**
 Display usage text to the user.
 */
BOOL
ProcInfoHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ProcInfo %i.%02i\n"), PROCINFO_VER_MAJOR, PROCINFO_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strProcInfoHelpText);
    return TRUE;
}

/**
 Output a 64 bit integer.

 @param LargeInt A large integer to output.

 @param NumberBase Specifies the numeric base to use.  Should be 10 for
        decimal or 16 for hex.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
DWORD
ProcInfoOutputLargeInteger(
    __in LARGE_INTEGER LargeInt,
    __in DWORD NumberBase,
    __inout PYORI_STRING OutputString
    )
{
    YORI_STRING String;
    TCHAR StringBuffer[32];

    YoriLibInitEmptyString(&String);
    String.StartOfString = StringBuffer;
    String.LengthAllocated = sizeof(StringBuffer)/sizeof(StringBuffer[0]);

    YoriLibNumberToString(&String, LargeInt.QuadPart, NumberBase, 0, ' ');

    if (OutputString->LengthAllocated >= String.LengthInChars) {
        memcpy(OutputString->StartOfString, String.StartOfString, String.LengthInChars * sizeof(TCHAR));
    }

    return String.LengthInChars;
}

/**
 Output a time string from 64 bit integer of milliseconds.

 @param LargeInt A large integer to output.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
DWORD
ProcInfoOutputTimestamp(
    __in LARGE_INTEGER LargeInt,
    __inout PYORI_STRING OutputString
    )
{
    YORI_STRING String;
    TCHAR StringBuffer[32];
    LARGE_INTEGER Remainder;
    DWORD Milliseconds;
    DWORD Seconds;
    DWORD Minutes;
    DWORD Hours;

    Remainder.QuadPart = LargeInt.QuadPart;
    Milliseconds = Remainder.LowPart % 1000;
    Remainder.QuadPart = Remainder.QuadPart / 1000;
    Seconds = Remainder.LowPart % 60;
    Remainder.QuadPart = Remainder.QuadPart / 60;
    Minutes = Remainder.LowPart % 60;
    Remainder.QuadPart = Remainder.QuadPart / 60;
    Hours = Remainder.LowPart;

    YoriLibInitEmptyString(&String);
    String.StartOfString = StringBuffer;
    String.LengthAllocated = sizeof(StringBuffer)/sizeof(StringBuffer[0]);
    String.LengthInChars = YoriLibSPrintf(String.StartOfString, _T("%i:%02i:%02i.%03i"), Hours, Minutes, Seconds, Milliseconds);

    if (OutputString->LengthAllocated >= String.LengthInChars) {
        memcpy(OutputString->StartOfString, String.StartOfString, String.LengthInChars * sizeof(TCHAR));
    }

    return String.LengthInChars;
}


/**
 Context containing the results of execution to pass to helper function used
 to format output.
 */
typedef struct _PROCINFO_CONTEXT {

    /**
     Amount of time in milliseconds that the process has been known to the
     system.
     */
    LARGE_INTEGER ElapsedTimeInMs;

    /**
     Amount of time in milliseconds that the process spent in kernel execution.
     */
    LARGE_INTEGER KernelTimeInMs;

    /**
     Amount of time in milliseconds that the process spent in user mode
     execution.
     */
    LARGE_INTEGER UserTimeInMs;

    /**
     Memory usage of the process.
     */
    PROCESS_VM_COUNTERS VmInfo;

    /**
     IO usage information.
     */
    YORI_IO_COUNTERS IoCounters;

} PROCINFO_CONTEXT, *PPROCINFO_CONTEXT;

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputBuffer A pointer to the output buffer to populate with data
        if a known variable is found.

 @param VariableName The variable name to expand.

 @param Context Pointer to a SYSTEMTIME structure containing the data to
        populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
DWORD
ProcInfoExpandVariables(
    __inout PYORI_STRING OutputBuffer,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    LARGE_INTEGER CpuTime;
    LARGE_INTEGER MemInKb;
    LARGE_INTEGER IoCount;
    PPROCINFO_CONTEXT ProcInfoContext = (PPROCINFO_CONTEXT)Context;

    if (YoriLibCompareStringWithLiteral(VariableName, _T("COMMIT")) == 0) {
        MemInKb.QuadPart = ProcInfoContext->VmInfo.CommitUsage / 1024;
        return ProcInfoOutputLargeInteger(MemInKb, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("CPU")) == 0) {
        CpuTime.QuadPart = ProcInfoContext->KernelTimeInMs.QuadPart + ProcInfoContext->UserTimeInMs.QuadPart;
        return ProcInfoOutputTimestamp(CpuTime, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("CPUMS")) == 0) {
        CpuTime.QuadPart = ProcInfoContext->KernelTimeInMs.QuadPart + ProcInfoContext->UserTimeInMs.QuadPart;
        return ProcInfoOutputLargeInteger(CpuTime, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("CPUKERNEL")) == 0) {
        return ProcInfoOutputTimestamp(ProcInfoContext->KernelTimeInMs, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("CPUKERNELMS")) == 0) {
        return ProcInfoOutputLargeInteger(ProcInfoContext->KernelTimeInMs, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("CPUUSER")) == 0) {
        return ProcInfoOutputTimestamp(ProcInfoContext->UserTimeInMs, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("CPUUSERMS")) == 0) {
        return ProcInfoOutputLargeInteger(ProcInfoContext->UserTimeInMs, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("ELAPSED")) == 0) {
        return ProcInfoOutputTimestamp(ProcInfoContext->ElapsedTimeInMs, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("ELAPSEDMS")) == 0) {
        return ProcInfoOutputLargeInteger(ProcInfoContext->ElapsedTimeInMs, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("OTHERIOBYTES")) == 0) {
        IoCount.QuadPart = ProcInfoContext->IoCounters.OtherBytes;
        return ProcInfoOutputLargeInteger(IoCount, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("OTHERIOCOUNT")) == 0) {
        IoCount.QuadPart = ProcInfoContext->IoCounters.OtherOperations;
        return ProcInfoOutputLargeInteger(IoCount, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("READBYTES")) == 0) {
        IoCount.QuadPart = ProcInfoContext->IoCounters.ReadBytes;
        return ProcInfoOutputLargeInteger(IoCount, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("READCOUNT")) == 0) {
        IoCount.QuadPart = ProcInfoContext->IoCounters.ReadOperations;
        return ProcInfoOutputLargeInteger(IoCount, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("WORKINGSET")) == 0) {
        MemInKb.QuadPart = ProcInfoContext->VmInfo.WorkingSetSize / 1024;
        return ProcInfoOutputLargeInteger(MemInKb, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("WRITEBYTES")) == 0) {
        IoCount.QuadPart = ProcInfoContext->IoCounters.WriteBytes;
        return ProcInfoOutputLargeInteger(IoCount, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("WRITECOUNT")) == 0) {
        IoCount.QuadPart = ProcInfoContext->IoCounters.WriteOperations;
        return ProcInfoOutputLargeInteger(IoCount, 10, OutputBuffer);
    }
    return 0;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the procinfo builtin command.
 */
#define ENTRYPOINT YoriCmd_PROCINFO
#else
/**
 The main entrypoint for the procinfo standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the procinfo cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD StartArg = 1;
    LONGLONG llTemp;
    DWORD CharsConsumed;
    DWORD Pid;
    DWORD i;
    YORI_STRING Arg;
    YORI_STRING DisplayString;
    YORI_STRING AllocatedFormatString;
    PROCINFO_CONTEXT ProcInfoContext;
    FILETIME ftCreationTime;
    FILETIME ftExitTime;
    FILETIME ftKernelTime;
    FILETIME ftUserTime;
    SYSTEMTIME stNow;
    FILETIME ftNow;
    LARGE_INTEGER liNow;
    HANDLE hProcess;

    LARGE_INTEGER liCreationTime;

    LPTSTR DefaultFormatString = 
                                 _T("Commit size:     $COMMIT$ Kb\n")
                                 _T("CPU total time:  $CPU$\n")
                                 _T("CPU kernel time: $CPUKERNEL$\n")
                                 _T("CPU user time:   $CPUUSER$\n")
                                 _T("Elapsed time:    $ELAPSED$\n")
                                 _T("Working set:     $WORKINGSET$ Kb\n");

    YoriLibInitEmptyString(&AllocatedFormatString);
    YoriLibConstantString(&AllocatedFormatString, DefaultFormatString);
    ZeroMemory(&ProcInfoContext, sizeof(ProcInfoContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ProcInfoHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                if (ArgC > i + 1) {
                    YoriLibFreeStringContents(&AllocatedFormatString);
                    YoriLibCloneString(&AllocatedFormatString, &ArgV[i + 1]);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0 || StartArg >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("procinfo: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (!YoriLibStringToNumber(&ArgV[StartArg], FALSE, &llTemp, &CharsConsumed) ||
        CharsConsumed == 0) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("procinfo: could not parse PID\n"));
        return EXIT_FAILURE;
    }

    Pid = (DWORD)llTemp;

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, Pid);
    if (hProcess == NULL) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("procinfo: open process failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return EXIT_FAILURE;
    }

    //
    //  Save off times from the process.
    //

    GetProcessTimes(hProcess, &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUserTime);
    liCreationTime.HighPart = ftCreationTime.dwHighDateTime;
    liCreationTime.LowPart = ftCreationTime.dwLowDateTime;
    ProcInfoContext.KernelTimeInMs.HighPart = ftKernelTime.dwHighDateTime;
    ProcInfoContext.KernelTimeInMs.LowPart = ftKernelTime.dwLowDateTime;
    ProcInfoContext.KernelTimeInMs.QuadPart = ProcInfoContext.KernelTimeInMs.QuadPart / (10 * 1000);
    ProcInfoContext.UserTimeInMs.HighPart = ftUserTime.dwHighDateTime;
    ProcInfoContext.UserTimeInMs.LowPart = ftUserTime.dwLowDateTime;
    ProcInfoContext.UserTimeInMs.QuadPart = ProcInfoContext.UserTimeInMs.QuadPart / (10 * 1000);

    GetSystemTime(&stNow);
    SystemTimeToFileTime(&stNow, &ftNow);
    liNow.HighPart = ftNow.dwHighDateTime;
    liNow.LowPart = ftNow.dwLowDateTime;

    ProcInfoContext.ElapsedTimeInMs.QuadPart = (liNow.QuadPart - liCreationTime.QuadPart) / (10 * 1000);

    if (DllNtDll.pNtQueryInformationProcess != NULL) {
        DWORD Status;
        DWORD dwBytesReturned;
        Status = DllNtDll.pNtQueryInformationProcess(hProcess, ProcessVmCounters, &ProcInfoContext.VmInfo, sizeof(ProcInfoContext.VmInfo), &dwBytesReturned);
        if (Status != 0) {
            CloseHandle(hProcess);
            return FALSE;
        }
    }

    if (DllKernel32.pGetProcessIoCounters != NULL) {
        DllKernel32.pGetProcessIoCounters(hProcess, &ProcInfoContext.IoCounters);
    }

    CloseHandle(hProcess);

    YoriLibInitEmptyString(&DisplayString);
    YoriLibExpandCommandVariables(&AllocatedFormatString, '$', FALSE, ProcInfoExpandVariables, &ProcInfoContext, &DisplayString);
    if (DisplayString.StartOfString != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
        YoriLibFreeStringContents(&DisplayString);
    }
    YoriLibFreeStringContents(&AllocatedFormatString);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
