/**
 * @file fileuser/fileuser.c
 *
 * Yori determine which processes are keeping files open.
 *
 * Copyright (c) 2018 Malcolm J. Smith
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

/**
 Help text to display to the user.
 */
const
CHAR strHelpText[] =
        "\n"
        "Determine which processes are keeping files open.\n"
        "\n"
        "FILEUSER [-license] <file>...\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -s             Process files from all subdirectories\n";

/**
 Display usage text to the user.
 */
BOOL
FileUserHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("FileUser %i.%i\n"), FILEUSER_VER_MAJOR, FILEUSER_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _FILEUSER_CONTEXT {

    /**
     Counts the number of files processed in an enumerate.  If this is zero,
     the program assumes the request is to create a new file.
     */
    DWORD FilesFoundThisArg;
    
    /**
     The number of bytes in the Buffer below.
     */
    DWORD BufferLength;

    /**
     A buffer that is populated with the array of process IDs using a given
     file.
     */
    PFILE_PROCESS_IDS_USING_FILE_INFORMATION Buffer;

} FILEUSER_CONTEXT, *PFILEUSER_CONTEXT;

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.  Note in this application this can
        be NULL when it is operating on files that do not yet exist.

 @param Depth Specifies the recursion depth.  Ignored in this application.

 @param Context Pointer to the fileuser context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
FileUserFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    HANDLE FileHandle;
    PFILEUSER_CONTEXT FileUserContext = (PFILEUSER_CONTEXT)Context;
    IO_STATUS_BLOCK IoStatus;
    DWORD Index;
    LONG Status;

    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(FileInfo);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    FileHandle = CreateFile(FilePath->StartOfString,
                            FILE_READ_ATTRIBUTES,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("fileuser: open of %y failed: %s"), FilePath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return TRUE;
    }

    FileUserContext->FilesFoundThisArg++;

    Status = DllNtDll.pNtQueryInformationFile(FileHandle, &IoStatus, FileUserContext->Buffer, FileUserContext->BufferLength, FileProcessIdsUsingFileInformation);
    if (Status == 0) {
        for (Index = 0; Index < FileUserContext->Buffer->NumberOfProcesses; Index++) {
            HANDLE ProcessHandle;
            TCHAR ProcessName[300];
            DWORD ProcessNameSize;

            ProcessName[0] = '\0';
            ProcessNameSize = sizeof(ProcessName)/sizeof(ProcessName[0]);
            ProcessHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)FileUserContext->Buffer->ProcessIds[Index]);
            if (ProcessHandle != NULL) {
                DllKernel32.pQueryFullProcessImageNameW(ProcessHandle, 0, ProcessName, &ProcessNameSize);
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%10i %s\n"), FileUserContext->Buffer->ProcessIds[Index], ProcessName);
        }
    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("fileuser: query of %y failed: %08x"), FilePath, Status);
    }

    CloseHandle(FileHandle);
    return TRUE;
}

/**
 The main entrypoint for the fileuser cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    DWORD MatchFlags;
    BOOL Recursive = FALSE;
    BOOL BasicEnumeration = FALSE;
    FILEUSER_CONTEXT FileUserContext;
    YORI_STRING Arg;

    ZeroMemory(&FileUserContext, sizeof(FileUserContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                FileUserHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
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

    if (DllNtDll.pNtQueryInformationFile == NULL ||
        DllKernel32.pQueryFullProcessImageNameW == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("fileuser: OS support not present\n"));
        return EXIT_FAILURE;
    }

    FileUserContext.BufferLength = 16 * 1024;
    FileUserContext.Buffer = YoriLibMalloc(FileUserContext.BufferLength);
    if (FileUserContext.Buffer == NULL) {
        return EXIT_FAILURE;
    }

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("fileuser: missing argument\n"));
        return EXIT_FAILURE;
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES;
        if (Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }

        for (i = StartArg; i < ArgC; i++) {

            FileUserContext.FilesFoundThisArg = 0;
            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, FileUserFileFoundCallback, &FileUserContext);
            if (FileUserContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
                    FileUserFileFoundCallback(&FullPath, NULL, 0, &FileUserContext);
                    YoriLibFreeStringContents(&FullPath);
                }
            }
        }
    }

    YoriLibFree(FileUserContext.Buffer);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
