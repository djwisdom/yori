/**
 * @file lib/yoricall.h
 *
 * Yori exported API for modules to call
 *
 * Copyright (c) 2017-2025 Malcolm J. Smith
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

BOOL
YoriCallAddAlias(
    __in PYORI_STRING Alias,
    __in PYORI_STRING Value
    );

BOOL
YoriCallAddHistoryString(
    __in PYORI_STRING NewCmd
    );

BOOL
YoriCallAddSystemAlias(
    __in PYORI_STRING Alias,
    __in PYORI_STRING Value
    );

BOOL
YoriCallBuiltinRegister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    );

BOOL
YoriCallBuiltinUnregister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    );

BOOL
YoriCallClearHistoryStrings(VOID);

BOOL
YoriCallDecrementPromptRecursionDepth(VOID);

BOOL
YoriCallDeleteAlias(
    __in PYORI_STRING Alias
    );

BOOL
YoriCallExecuteBuiltin(
    __in PYORI_STRING Expression
    );

BOOL
YoriCallExecuteExpression(
    __in PYORI_STRING Expression
    );

VOID
YoriCallExitProcess(
    __in DWORD ExitCode
    );

BOOL
YoriCallExpandAlias(
    __in PYORI_STRING CommandString,
    __in PYORI_STRING ExpandedString
    );

VOID
YoriCallFreeYoriString(
    __in PYORI_STRING String
    );

__success(return)
BOOL
YoriCallGetAliasStrings(
    __out PYORI_STRING AliasStrings
    );

__success(return)
BOOL
YoriCallGetEnvironmentVariable(
    __in PYORI_STRING VariableName,
    __out PYORI_STRING Value
    );

DWORD
YoriCallGetErrorLevel(VOID);

__success(return)
BOOL
YoriCallGetEscapedArguments(
    __out PDWORD ArgC,
    __out PYORI_STRING * ArgV
    );

__success(return)
BOOL
YoriCallGetEscapedArgumentsEx(
    __out PDWORD ArgC,
    __out PYORI_STRING * ArgV,
    __out PBOOLEAN * ArgContainsQuotes
    );

__success(return)
BOOL
YoriCallGetHistoryStrings(
    __in DWORD MaximumNumber,
    __out PYORI_STRING HistoryStrings
    );

__success(return)
BOOL
YoriCallGetJobInformation(
    __in DWORD JobId,
    __out PBOOL HasCompleted,
    __out PBOOL HasOutput,
    __out PDWORD ExitCode,
    __inout PYORI_STRING Command
    );

__success(return)
BOOL
YoriCallGetJobOutput(
    __in DWORD JobId,
    __out PYORI_STRING Output,
    __out PYORI_STRING Errors
    );

DWORD
YoriCallGetNextJobId(
    __in DWORD PreviousJobId
    );

__success(return)
BOOL
YoriCallGetSystemAliasStrings(
    __out PYORI_STRING AliasStrings
    );

__success(return)
BOOL
YoriCallGetYoriVersion(
    __out PDWORD MajorVersion,
    __out PDWORD MinorVersion
    );

BOOL
YoriCallIncrementPromptRecursionDepth(VOID);

BOOL
YoriCallIsProcessExiting(VOID);

BOOL
YoriCallPipeJobOutput(
    __in DWORD JobId,
    __in_opt HANDLE hPipeOutput,
    __in_opt HANDLE hPipeErrors
    );

__success(return)
BOOL
YoriCallSetCurrentDirectory(
    __in PYORI_STRING NewCurrentDirectory
    );

BOOL
YoriCallSetDefaultColor(
    __in WORD NewDefaultColor
    );

BOOL
YoriCallSetEnvironmentVariable(
    __in PYORI_STRING VariableName,
    __in_opt PYORI_STRING Value
    );

BOOL
YoriCallSetJobPriority(
    __in DWORD JobId,
    __in DWORD PriorityClass
    );

BOOL
YoriCallSetNextCommand(
    __in PYORI_STRING NextCommand
    );

BOOL
YoriCallSetUnloadRoutine(
    __in PYORI_BUILTIN_UNLOAD_NOTIFY UnloadNotify
    );

BOOL
YoriCallTerminateJob(
    __in DWORD JobId
    );

VOID
YoriCallWaitForJob(
    __in DWORD JobId
    );
