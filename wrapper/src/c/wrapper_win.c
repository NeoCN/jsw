/*
 * Copyright (c) 2001 Silver Egg Technology
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without 
 * restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following 
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// $Log$
// Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
// no message
//

#ifndef WIN32
// For some reason this is not defines sometimes when I build $%$%$@@!!
barf
#endif

#ifdef WIN32

#include <math.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <time.h>

#include "wrapper.h"
//#include "registry.h"
#include "property.h"

/*****************************************************************************
 * Win32 specific variables and procedures                                   *
 *****************************************************************************/
SERVICE_STATUS          ssStatus;       
SERVICE_STATUS_HANDLE   sshStatusHandle;
TCHAR                   szErr[1024];

// Allocate a 512 byte buffer in the exe to store a static config file name
//	when installed as a service.
static char             *staticConfigFilename = \
"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz" \
"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz" \
"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz" \
"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz" \
"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz" \
"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz" \
"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz" \
"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz";

static HANDLE wrapperProcess = NULL;
static HANDLE wrapperChildStdoutWr = NULL;
static HANDLE wrapperChildStdoutRd = NULL;
static int    wrapperChildStdoutRdLastLF = 0;

char wrapperClasspathSeparator = ';';

//*****************************************************************************
// Windows specific code
//*****************************************************************************

/**
 * Gets the error code for the last operation that failed.
 */
int wrapperGetLastError() {
    return WSAGetLastError();
}

/**
 * Create an error message from GetLastError() using the
 *  FormatMessage API Call...
 */
LPTSTR getLastErrorText(LPTSTR lpszBuf, DWORD dwSize) {
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                           FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL);

    // supplied buffer is not long enough
    if (!dwRet || ((long)dwSize < (long)dwRet+14)) {
        lpszBuf[0] = TEXT('\0');
    } else {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  //remove cr and newline character
        _stprintf( lpszBuf, TEXT("%s (0x%x)"), lpszTemp, GetLastError());
    }

    if (lpszTemp) {
        GlobalFree((HGLOBAL) lpszTemp);
    }

    return lpszBuf;
}

/**
 * Initialize the pipe which will be used to capture the output from the child
 * process.
 */
int wrapperInitChildPipe() {
    SECURITY_ATTRIBUTES saAttr;
    HANDLE childStdoutRd;

    // Set the bInheritHandle flag so pipe handles are inherited.
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT.
    if (!CreatePipe(&childStdoutRd, &wrapperChildStdoutWr, &saAttr, 0)) {
        wrapperLog(WRAPPER_SOURCE_WRAPPER, "Stdout pipe creation failed");
        return -1;
    }

    // Create a noninheritable read handle and close the inheritable read handle.
    if (!DuplicateHandle(GetCurrentProcess(), childStdoutRd, GetCurrentProcess(), &wrapperChildStdoutRd, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
        wrapperLog(WRAPPER_SOURCE_WRAPPER, "DuplicateHandle failed");
        return -1;
    }
    CloseHandle(childStdoutRd);

    return 0;
}

/**
 * Handler to take care of the case where the user hits CTRL-C when the wrapper 
 * is being run as a console.
 */
int wrapperConsoleHandler(int key) {
    int quit = FALSE;

    switch (key) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
        // The user hit CTRL-C.  Can only happen when run as a console.
        //  Always quit.
        wrapperLog(WRAPPER_SOURCE_WRAPPER, "CTRL-C trapped.  Shutting down.");
        quit = TRUE;
        break;

    case CTRL_LOGOFF_EVENT:
        // Happens when the user logs off.  We should quit when run as a
        //  console, but stay up when run as a service.
        if (wrapperData->isConsole) {
            wrapperLog(WRAPPER_SOURCE_WRAPPER, "User logged out.  Shutting down.");
            quit = TRUE;
        } else {
            wrapperLog(WRAPPER_SOURCE_WRAPPER, "User logged out.  Ignored.");
            quit = FALSE;
        }
        break;
    case CTRL_SHUTDOWN_EVENT:
        // Happens when the machine is shutdown or rebooted.  Always quit.
        wrapperLog(WRAPPER_SOURCE_WRAPPER, "Machine is shutting down.");
        quit = TRUE;
        break;
    default:
        // Unknown.  Don't quit here.
        wrapperLogI(WRAPPER_SOURCE_WRAPPER, "Trapped unexpected console signal (%d).  Ignored.", key);
        quit = FALSE;
    }

    if (quit) {
        wrapperStopProcess(0);
        // Don't actually kill the process here.  Let the application shut itself down
    }

    return TRUE; // We handled the event.
}



/******************************************************************************
 * Platform specific methods
 *****************************************************************************/

void wrapperBuildJavaCommand() {
    int commandLen;
    char **strings;
    int length, i;

    // If this is not the first time through, then dispose the old command
    if (wrapperData->jvmCommand) {
        free(wrapperData->jvmCommand);
        wrapperData->jvmCommand = NULL;
    }

    // Build the Java Command Strings
    strings = NULL;
    length = 0;
    wrapperBuildJavaCommandArray(&strings, &length, TRUE);
    /*
    for (i = 0; i < length; i++) {
        printf("%d : %s\n", i, strings[i]);
    }
    */

    // Build a single string from the array
    // Calculate the length
    commandLen = 0;
    for (i = 0; i < length; i++) {
        if (i > 0) {
            commandLen++; // Space
        }
        commandLen += strlen(strings[i]);
    }
    commandLen++; // '\0'

    // Build the actual command
    wrapperData->jvmCommand = (char *)malloc(sizeof(char) * commandLen);
    commandLen = 0;
    for (i = 0; i < length; i++) {
        if (i > 0) {
            wrapperData->jvmCommand[commandLen++] = ' ';
        }
        sprintf((char *)(&(wrapperData->jvmCommand[commandLen])), strings[i]);
        commandLen += strlen(strings[i]);
    }
    wrapperData->jvmCommand[commandLen++] = '\0';

    // Free up the temporary command array
    wrapperFreeJavaCommandArray(strings, length);
}

/**
 * Execute initialization code to get the wrapper set up.
 */
int wrapperInitialize() {
    WORD ws_version=MAKEWORD(1, 1);
    WSADATA ws_data;

    int res;

    // Set the handler to trap console signals
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)wrapperConsoleHandler, TRUE);

    // Initialize Winsock
    if ((res = WSAStartup(ws_version, &ws_data)) != 0) {
        wrapperLog(WRAPPER_SOURCE_WRAPPER, "Cannot initialize Windows socket DLLs.");
        return res;
    }

    // Initialize the pipe to capture the child process output
    if ((res = wrapperInitChildPipe()) != 0) {
        return res;
    }

    return 0;
}

/**
 * Execute clean up code in preparation for shutdown
 */
void wrapperCleanup() {
}

/**
 * Reports the status of the wrapper to the service manager
 * Possible status values:
 *   WRAPPER_WSTATE_STARTING
 *   WRAPPER_WSTATE_STARTED
 *   WRAPPER_WSTATE_STOPPING
 *   WRAPPER_WSTATE_STOPPED
 */
void wrapperReportStatus(int status, int errorCode, int waitHint) {
    int natState;
    static DWORD dwCheckPoint = 1;
    BOOL bResult = TRUE;

    switch (status) {
    case WRAPPER_WSTATE_STARTING:
        natState = SERVICE_START_PENDING;
        break;
    case WRAPPER_WSTATE_STARTED:
        natState = SERVICE_RUNNING;
        break;
    case WRAPPER_WSTATE_STOPPING:
        natState = SERVICE_STOP_PENDING;
        break;
    case WRAPPER_WSTATE_STOPPED:
        natState = SERVICE_STOPPED;
        break;
    default:
        wrapperLogI(WRAPPER_SOURCE_WRAPPER, "Unknown status: %d", status);
        return;
    }

    if (!wrapperData->isConsole) {
        if (natState == SERVICE_START_PENDING) {
            ssStatus.dwControlsAccepted = 0;
        } else {
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        }

        ssStatus.dwCurrentState = natState;
        ssStatus.dwWin32ExitCode = errorCode;
        ssStatus.dwWaitHint = waitHint;

        if ((natState == SERVICE_RUNNING ) || (natState == SERVICE_STOPPED)) {
            ssStatus.dwCheckPoint = 0;
        } else {
            ssStatus.dwCheckPoint = dwCheckPoint++;
        }

        if (!(bResult = SetServiceStatus(sshStatusHandle, &ssStatus))) {
            wrapperLog(WRAPPER_SOURCE_WRAPPER, "SetServiceStatus failed");
        }
    }
}

/**
 * Read and process any output from the child JVM Process.
 * Most output should be logged to the wrapper log file.
 */
void wrapperReadChildOutput() {
    DWORD dwRead, dwAvail;
    int lfPos;
    char chBuf[1025];
    char *c;
    int thisLF;

    for (;;) {
        // Find out how much data there is in the pipe before we try to read it.
        if (!PeekNamedPipe(wrapperChildStdoutRd, chBuf, 1024, &dwRead, &dwAvail, NULL) || dwRead == 0 || dwAvail == 0) {
            //printf("stdout avail %d.", dwAvail);
            break;
        }
        //printf("stdout avail %d.", dwAvail);

        // We only want to read in a line at a time.  We want to read in a line
        //	feed if it exists, but we want to remove it from the buffer to be logged.
        if (dwAvail > 1024) {
            dwAvail = 1024;
        }

        // Terminate the read buffer
        chBuf[dwRead] = '\0';

        lfPos = dwRead;

        thisLF = 0;

        // Look for a CR
        if ((c = strchr(chBuf, (char)0x0d)) != NULL) {
            if (c - chBuf < lfPos) {
                lfPos = c - chBuf;
                if (chBuf[lfPos + 1] == (char)0x0a) {
                    // Also read in the CR+LF
                    dwAvail = lfPos + 2;
                    //printf("CR+LF");
                    thisLF = 1;
                } else {
                    // Also read in the CR
                    dwAvail = lfPos + 1;
                    //printf("CR");
                    thisLF = 1;
                }
            }
        }

        // Look for a LF
        if ((c = strchr(chBuf, (char)0x0a)) != NULL) {
            if (c - chBuf < lfPos) {
                lfPos = c - chBuf;
                // Also read in the LF
                dwAvail = lfPos + 1;
                //printf("LF");
                thisLF = 1;
            }
        }

        if (!ReadFile(wrapperChildStdoutRd, chBuf, dwAvail, &dwRead, NULL) || dwRead == 0) {
            //printf("stdout read %d.\n", dwRead);
            break;
        }
        // Write over the lf
        chBuf[lfPos] = '\0';
        // Also set the a '\0' at the dwRead location just to be safe.
        chBuf[dwRead] = '\0';

        // Make sure that this is just another LF if the last line was missing it's LF
        if ((lfPos > 0) || (wrapperChildStdoutRdLastLF)) {
            wrapperLog(wrapperData->jvmRestarts, chBuf);
        }

        wrapperChildStdoutRdLastLF = thisLF;
    }
}

/**
 * Checks on the status of the JVM Process.
 * Returns WRAPPER_PROCESS_UP or WRAPPER_PROCESS_DOWN
 */
int wrapperGetProcessStatus() {
    int res;

    switch (WaitForSingleObject(wrapperProcess, 0)) {
    case WAIT_ABANDONED:
    case WAIT_OBJECT_0:
        res = WRAPPER_PROCESS_DOWN;
        break;

    case WAIT_TIMEOUT:
        res = WRAPPER_PROCESS_UP;
        break;

    default:
        wrapperLog(WRAPPER_SOURCE_WRAPPER, "Critical error: wait for JVM process failed");
        exit(1);
    }

    return res;
}

/**
 * Kill the JVM Process immediately and set the JVM State to
 *  WRAPPER_JSTATE_DOWN
 */
void wrapperKillProcess() {
    int ret;

    // Check to make sure that the JVM process is still running
    ret = WaitForSingleObject(wrapperProcess, 0);
    if (ret == WAIT_TIMEOUT) {
        // JVM is still up.  Kill it immediately.
        if (TerminateProcess(wrapperProcess, 0)) {
            wrapperLog(WRAPPER_SOURCE_WRAPPER, "Java Virtual Machine did not exit on request, terminated");
        } else {
            wrapperLog(WRAPPER_SOURCE_WRAPPER, "Java Virtual Machine did not exit on request.");
            wrapperLogI(WRAPPER_SOURCE_WRAPPER, "  Attempt to terminate process failed.  Error=%d", GetLastError());
        }
    }

    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->jStateTimeout = 0;
    wrapperProcess = NULL;

    // Close any open socket to the JVM
    wrapperProtocolClose();
}

/**
 * Pauses before launching a new JVM if necessary.
 */
void wrapperPauseBeforeExecute() {
    // If this is not the first time that we are launching a JVM,
    //  then pause for 5 seconds to give the previously crashed?
    //  instance of the JVM a chance to be cleaned up correctly
    //  by the system.
    if (wrapperData->jvmRestarts > 0) {
        if (wrapperData->isDebugging) {
            wrapperLog(WRAPPER_SOURCE_WRAPPER, "Pausing for 5 seconds...");
        }
        Sleep(5000);
    }
}

/**
 * Launches a JVM process and store it internally
 */
void wrapperExecute() {
    SECURITY_ATTRIBUTES process_attributes;
    STARTUPINFO startup_info; 
    PROCESS_INFORMATION process_info;
    int ret;
    // Do not show another console for the new process
    int processflags=CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS;
    //int processflags=0; //DETACHED_PROCESS;  // Use the same process group so that
    //  the new process will be protected
    //  as a service.
    // Do not show another console for the new process, but show its output in the current console.
    //int processflags=CREATE_NEW_PROCESS_GROUP;
    // Show a console for the new process
    //int processflags=CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE;
    char *commandline=NULL;
    char *environment=NULL;
    char *binparam=NULL;
    int char_block_size = 8196;
    int string_size = 0;
    int temp_int = 0;
    char szPath[512];
    char *c;

    // Increment the process ID for Log sourcing
    wrapperData->jvmRestarts++;

    // Setup the command line
    //commandline = "java -Djava.class.path=\"c:/SilverEgg/wrapper/lib/wrappertest.jar\" com.silveregg.wrapper.test.Main";
    commandline = wrapperData->jvmCommand;
    if (wrapperData->isDebugging) {
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "command: %s", commandline);
    }
                           
    // Setup environment. Use parent's for now
    environment = NULL;

    // Initialize a SECURITY_ATTRIBUTES for the process attributes of the new process.
    process_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    process_attributes.lpSecurityDescriptor = NULL;
    process_attributes.bInheritHandle = TRUE;

    // Initialize a STARTUPINFO structure to use for the new process.
    startup_info.cb=sizeof(STARTUPINFO);
    startup_info.lpReserved=NULL;
    startup_info.lpDesktop=NULL;
    startup_info.lpTitle="Wrapper Client (Do not close)";
    startup_info.dwX=0;
    startup_info.dwY=0;
    startup_info.dwXSize=0;
    startup_info.dwYSize=0;
    startup_info.dwXCountChars=0;
    startup_info.dwYCountChars=0;
    startup_info.dwFillAttribute=0;
    startup_info.dwFlags=STARTF_USESTDHANDLES;
    startup_info.wShowWindow=0;
    startup_info.cbReserved2=0;
    startup_info.lpReserved2=NULL;
    startup_info.hStdInput=NULL;
    startup_info.hStdOutput=wrapperChildStdoutWr;
    startup_info.hStdError=wrapperChildStdoutWr;

    // Initialize a PROCESS_INFORMATION structure to use for the new process */ 
    process_info.hProcess=NULL;
    process_info.hThread=NULL;
    process_info.dwProcessId=0;
    process_info.dwThreadId=0;

    // Need to directory that this program exists in.  Not the current directory.
    //	Note, the current directory when run as an NT service is the windows system directory.
    // Get the full path and filename of this program
    if (GetModuleFileName(NULL, szPath, 512) == 0){
        wrapperLogSS(WRAPPER_SOURCE_WRAPPER, "Unable to launch %s -%s",
                     wrapperData->ntServiceDisplayName, getLastErrorText(szErr, 256));
        wrapperProcess = NULL;
        return;
    }
    c = strrchr(szPath, '\\');
    if (c == NULL) {
        szPath[0] = '\0';
    } else {
        c[1] = '\0'; // terminate after the slash
    }

    // Create the new process
    ret=CreateProcess(NULL, 
                      commandline,    /* the command line to start */
                      NULL, //&process_attributes, /* process security attributes */
                      NULL, //&process_attributes, /* primary thread security attributes */
                      TRUE,           /* handles are inherited */
                      processflags,   /* we specify new process group */
                      environment,    /* use parent's environment */
                      szPath,         /* use exe's install directory */
                      &startup_info,  /* STARTUPINFO pointer */
                      &process_info); /* PROCESS_INFORMATION pointer */

    /* Check if virtual machine started */
    if (ret==FALSE) {
        int err=GetLastError();
        /* This was placed to handle the Swedish WinNT bug */
        if (err!=NO_ERROR) {
            wrapperLogSI(WRAPPER_SOURCE_WRAPPER, "can not execute \"%s\" (ERR=%d)", commandline, err);
            wrapperProcess = NULL;
            return;
        }
    }

    /* Now check if we have a process handle again for the Swedish WinNT bug */
    if (process_info.hProcess==NULL) {
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "can not execute \"%s\"", commandline);
        wrapperProcess = NULL;
        return;
    }

    if (wrapperData->isDebugging) {
        wrapperLogI(WRAPPER_SOURCE_WRAPPER, "Java Virtual Machine started (PID=%d)", process_info.dwProcessId);
    }

    wrapperProcess = process_info.hProcess;
}

/******************************************************************************
 * NT Service Methods
 *****************************************************************************/

/**
 * The service control handler is called by the service manager when there are
 *	events for the service.  registered using a call to 
 *	RegisterServiceCtrlHandler in wrapperServiceMain.
 */
VOID WINAPI wrapperServiceControlHandler(DWORD dwCtrlCode) {
    if (wrapperData->isDebugging) {
        wrapperLogI(WRAPPER_SOURCE_WRAPPER, "ServiceControlHandler(%d)", dwCtrlCode);
    }

    switch(dwCtrlCode) {
    case SERVICE_CONTROL_STOP:
        if (wrapperData->isDebugging) {
            wrapperLog(WRAPPER_SOURCE_WRAPPER, "  SERVICE_CONTROL_STOP");
        }

        // Request to stop the service. Report SERVICE_STOP_PENDING
        // to the service control manager before calling ServiceStop()
        // to avoid a "Service did not respond" error.
        wrapperReportStatus(WRAPPER_WSTATE_STOPPING, 0, 0);

        // Tell the wrapper to shutdown normally
        wrapperStopProcess(0);
        return;
        
    case SERVICE_CONTROL_INTERROGATE:
        if (wrapperData->isDebugging) {
            wrapperLog(WRAPPER_SOURCE_WRAPPER, "  SERVICE_CONTROL_INTERROGATE");
        }

        // This case MUST be processed, even though we are not
        // obligated to do anything substantial in the process.
        break;

    case SERVICE_CONTROL_SHUTDOWN:
        if (wrapperData->isDebugging) {
            wrapperLog(WRAPPER_SOURCE_WRAPPER, "  SERVICE_CONTROL_SHUTDOWN");
        }

        wrapperReportStatus(WRAPPER_WSTATE_STOPPING, 0, 0);

        // Tell the wrapper to shutdown normally
        wrapperStopProcess(0);
        break;

    default:
        // Any other cases...
        break;
    }

    // After invocation of this function, we MUST call the SetServiceStatus
    // function, which is accomplished through our ReportStatus function. We
    // must do this even if the current status has not changed.
    wrapperReportStatus(wrapperData->wState, 0, 0);
}

/**
 * The wrapperServiceMain function is the entry point for the NT service.
 *	It is called by the service manager.
 */
void WINAPI wrapperServiceMain(DWORD dwArgc, LPTSTR *lpszArgv) {
    // Call RegisterServiceCtrlHandler immediately to register a service control 
    // handler function. The returned SERVICE_STATUS_HANDLE is saved with global 
    // scope, and used as a service id in calls to SetServiceStatus.
    if (!(sshStatusHandle = RegisterServiceCtrlHandler(wrapperData->ntServiceName, wrapperServiceControlHandler))) {
        goto finally;
    }

    // The global ssStatus SERVICE_STATUS structure contains information about the
    // service, and is used throughout the program in calls made to SetStatus through
    // the ReportStatus function.
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
    ssStatus.dwServiceSpecificExitCode = 0;


    // If we could guarantee that all initialization would occur in less than one
    // second, we would not have to report our status to the service control manager.
    // For good measure, we will assign SERVICE_START_PENDING to the current service
    // state and inform the service control manager through our ReportStatus function.
    wrapperReportStatus(WRAPPER_WSTATE_STARTING, 0, 3000);

    // Now actually start the service
    wrapperRunService();

 finally:

    // Report to the service manager that the application is about to exit.
    if (sshStatusHandle) {
        wrapperReportStatus(WRAPPER_WSTATE_STOPPED, wrapperData->exitCode, 0);
        // Will not make it here if successful as the process will have been 
        //  stopped by the service manager.
    }

    // Kill the process if necessary
    exit(0);
}

/**
 * Install the Wrapper as an NT Service using the information and service
 *  name in the current configuration file.
 *
 * Creates a copy of the wrapper.exe file named ~service<ServiceName>.exe
 *  and store the current config file name in the new .exe
 *
 * This makes it possible to let the service find its config file when
 *  launched by the NT service manager.
 */
int wrapperInstall(char *appName, char *configFile) {
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    char szPath[512];
    char newPath[512];
    char *c;
    FILE *source;
    FILE *target;
    char *buffer;
    char *zbuffer;
    long int lSize, hSize, write, read;
    boolean found;
    int i, j;
    int result = 0;

    // Get the full path and filename of this program
    if (GetModuleFileName(NULL, szPath, 512) == 0){
        wrapperLogSS(WRAPPER_SOURCE_WRAPPER, "Unable to install %s -%s",
                     wrapperData->ntServiceDisplayName, getLastErrorText(szErr, 256));
        return 1;
    }

    // In order to create the NT service, we have to create a copy of the
    //	current exe named "serviceWrapper.exe" if the service to be installed is called Wrapper.
    // Find the last backslash in szPath;
    c = strrchr(szPath, '\\');
    if (c == NULL) {
        // Have not seen this case.  Assume current directory.
        sprintf(newPath, "~service%s.exe", wrapperData->ntServiceName);
    } else {
        memcpy(newPath, szPath, c - szPath);
        sprintf((char*)((int)newPath + c - szPath), "\\~service%s.exe", wrapperData->ntServiceName);
    }

    // Make sure that a file does not already exist with this name
    /*
      if ((target = fopen(newPath, "r")) != NULL) {
      // The file already exists

      // Close the file;
      fclose(target);

      wrapperLogSS(WRAPPER_SOURCE_WRAPPER, "Unable to install the service %s because the file %s already exists.",
      wrapperData->ntServiceName, newPath);
      wrapperLog("Another service may not have been uninstalled correctly.");

      return 1;
      }
    */
    // This was confusing.  Just delete the file for now.
    remove(newPath);

    // Generate the new binary for the service by making a modified copy of the original
    // Open the source
    wrapperLogS(WRAPPER_SOURCE_WRAPPER, "Generating new binary for service: %s", newPath);
    if ((source = fopen(szPath, "rb")) == NULL) {
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "Unable to open file : %s", szPath);
        return 1;
    }

    // Open the target file
    if ((target = fopen(newPath, "wb")) == NULL) {
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "Unable to create file : %s", newPath);
        fclose(source);
        return 1;
    }

    // Build the search key.  Do it this way so that it dows not exist in the binary.
    zbuffer = (char *)malloc(sizeof(char) * 512);
    memset(zbuffer, 'z', 512);

    // Loop over and read in 1024 byte blocks, keeping 2048 bytes in memory at all times to make the search easy.
    hSize = 0;
    lSize = 0;
    read = 0;
    buffer = (char *)malloc(sizeof(char) * 2048);
    found = FALSE;
    do {
        // Read a new 1024 block into the high half of the buffer
        hSize = fread(buffer + 1024, sizeof(char), sizeof(char) *1024, source);
        read += hSize;

        if (read > 171000) {
            read = read;
        }
        if (lSize > 0) {
            // If we have not yet found the search key, then look for it.

            if (!found) {
                for (i = 0; (i < 1024) && (i < lSize + hSize - 511); i++) {
                    if ((buffer + i)[0] == 'z') {
                        // Count the number of consecutive 'z's
                        j = 0;
                        while ((buffer + i)[j] == 'z') {
                            j++;
                        }

                        if (j >= 512) {
                            // Found the key;
                            found = TRUE;

                            // Clear the memory first, then write the config file path.
                            memset(buffer + i, 'y', 512);
                            sprintf(buffer + i, "%s", wrapperData->configFile);
                            break;
                        }
                    }
                }
            }

            // Write the low half of the buffer to disk
            write = fwrite(buffer, sizeof(char), lSize, target);
            if (write != lSize) {
                wrapperLogII(WRAPPER_SOURCE_WRAPPER, "Unable to write new EXE to disk.  (%0 != %1)", write, lSize);
                // Clean up
                fclose(source);
                fclose(target);
                free(buffer);
                free(zbuffer);
                return 1;
            }
        }

        // Move the high block to the low block's memory.
        if (hSize > 0) {
            memcpy(buffer, buffer + 1024, 1024);
        }
        lSize = hSize;
        hSize = 0;
    } while (lSize > 0);

    // Clean up
    fclose(source);
    fclose(target);
    free(buffer);
    free(zbuffer);

    if (!found) {
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "Unable to locate replacement key in : %s", szPath);
        remove(newPath);
        return 1;
    }



    // Next, get a handle to the service control manager
    schSCManager = OpenSCManager(
                                 NULL,                   
                                 NULL,                   
                                 SC_MANAGER_ALL_ACCESS   
                                 );
    
    if (schSCManager) {
        schService = CreateService(
                                   schSCManager,                       // SCManager database
                                   wrapperData->ntServiceName,         // name of service
                                   wrapperData->ntServiceDisplayName,  // name to display
                                   SERVICE_ALL_ACCESS,                 // desired access
                                   SERVICE_WIN32_OWN_PROCESS,          // service type
                                   wrapperData->ntServiceStartType,    // start type
                                   SERVICE_ERROR_NORMAL,               // error control type
                                   newPath,                            // service's binary
                                   NULL,                               // no load ordering group
                                   NULL,                               // no tag identifier
                                   wrapperData->ntServiceDependencies, // dependencies
                                   NULL,                               // LocalSystem account
                                   NULL);                              // no password

        if (schService){
            wrapperLogS(WRAPPER_SOURCE_WRAPPER, "%s installed.", wrapperData->ntServiceDisplayName);

            // Close the handle to this service object
            CloseServiceHandle(schService);
        } else {
            wrapperLogS(WRAPPER_SOURCE_WRAPPER, "CreateService failed - %s", getLastErrorText(szErr, 256));
            result = 1;
        }

        // Close the handle to the service control manager database
        CloseServiceHandle(schSCManager);
    } else {
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "OpenSCManager failed - %s", getLastErrorText(szErr,256));
        result = 1;
    }
    // If there were any problems then remove the new file.
    if (result != 0) {
        remove(newPath);
    }

    return result;
}

/**
 * Uninstall the service and clean up
 */
int wrapperRemove(char *appName, char *configFile) {
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    char szPath[512];
    char newPath[512];
    char *c;
    int result = 0;

    // First, get a handle to the service control manager
    schSCManager = OpenSCManager(
                                 NULL,                   
                                 NULL,                   
                                 SC_MANAGER_ALL_ACCESS   
                                 );
    if (schSCManager){

        // Next get the handle to this service...
        schService = OpenService(schSCManager, wrapperData->ntServiceName, SERVICE_ALL_ACCESS);

        if (schService){

            // Now, try to stop the service by passing a STOP code thru the control manager
            if (ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus)){
                wrapperLog(WRAPPER_SOURCE_WRAPPER, "Service is running.  Stopping it...");

                // Wait a second...
                Sleep( 1000 );

                // Poll the status of the service for SERVICE_STOP_PENDING
                while(QueryServiceStatus( schService, &ssStatus)){

                    // If the service has not stopped, wait another second
                    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING ){
                        wrapperLog(WRAPPER_SOURCE_WRAPPER, "Waiting to stop...");
                        Sleep(1000);
                    }
                    else
                        break;
                }

                if ( ssStatus.dwCurrentState == SERVICE_STOPPED ) {
                    wrapperLogS(WRAPPER_SOURCE_WRAPPER, "%s stopped.", wrapperData->ntServiceDisplayName);
                } else {
                    wrapperLogS(WRAPPER_SOURCE_WRAPPER, "%s failed to stop.", wrapperData->ntServiceDisplayName);
                    result = 1;
                }
            }

            // Now try to remove the service...
            if (DeleteService(schService)) {
                wrapperLogS(WRAPPER_SOURCE_WRAPPER, "%s removed.", wrapperData->ntServiceDisplayName);

                // Remove the service exe
                // Get the full path and filename of this program
                if (GetModuleFileName(NULL, szPath, 512) == 0){
                    wrapperLogSS(WRAPPER_SOURCE_WRAPPER, "Unable to install %s -%s",
                                 wrapperData->ntServiceDisplayName, getLastErrorText(szErr, 256));
                    return 1;
                }

                // In order to create the NT service, we have to create a copy of the
                //	current exe named "serviceWrapper.exe" if the service to be installed is called Wrapper.
                // Find the last backslash in szPath;
                c = strrchr(szPath, '\\');
                if (c == NULL) {
                    // Have not seen this case.  Assume current directory.
                    sprintf(newPath, "~service%s.exe", wrapperData->ntServiceName);
                } else {
                    memcpy(newPath, szPath, c - szPath);
                    sprintf((char*)((int)newPath + c - szPath), "\\~service%s.exe", wrapperData->ntServiceName);
                }

                // Delete the service file
                remove(newPath);
            } else {
                wrapperLogS(WRAPPER_SOURCE_WRAPPER, "DeleteService failed - %s", getLastErrorText(szErr,256));
                result = 1;
            }
            
            //Close this service object's handle to the service control manager
            CloseServiceHandle(schService);
        } else {
            wrapperLogS(WRAPPER_SOURCE_WRAPPER, "OpenService failed - %s", getLastErrorText(szErr,256));
            result = 1;
        }
        
        // Finally, close the handle to the service control manager's database
        CloseServiceHandle(schSCManager);
    } else {
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "OpenSCManager failed - %s", getLastErrorText(szErr,256));
        result = 1;
    }

    return result;
}





/******************************************************************************
 * Main function
 *****************************************************************************/

int exceptionFilterFunction(PEXCEPTION_POINTERS exceptionPointers) {
    DWORD exCode;
    char *exName;
    int i;

    wrapperLog(WRAPPER_SOURCE_WRAPPER, "encountered a fatal error in Wrapper");
    exCode = exceptionPointers->ExceptionRecord->ExceptionCode;
    switch (exCode) {
    case EXCEPTION_ACCESS_VIOLATION:
        exName = "EXCEPTION_ACCESS_VIOLATION";
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        exName = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        break;
    case EXCEPTION_BREAKPOINT:
        exName = "EXCEPTION_BREAKPOINT";
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        exName = "EXCEPTION_DATATYPE_MISALIGNMENT";
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        exName = "EXCEPTION_FLT_DENORMAL_OPERAND";
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        exName = "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        exName = "EXCEPTION_FLT_INEXACT_RESULT";
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        exName = "EXCEPTION_FLT_INVALID_OPERATION";
        break;
    case EXCEPTION_FLT_OVERFLOW:
        exName = "EXCEPTION_FLT_OVERFLOW";
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        exName = "EXCEPTION_FLT_STACK_CHECK";
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        exName = "EXCEPTION_FLT_UNDERFLOW";
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        exName = "EXCEPTION_ILLEGAL_INSTRUCTION";
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        exName = "EXCEPTION_IN_PAGE_ERROR";
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        exName = "EXCEPTION_INT_DIVIDE_BY_ZERO";
        break;
    case EXCEPTION_INT_OVERFLOW:
        exName = "EXCEPTION_INT_OVERFLOW";
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        exName = "EXCEPTION_INVALID_DISPOSITION";
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        exName = "EXCEPTION_NONCONTINUABLE_EXCEPTION";
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        exName = "EXCEPTION_PRIV_INSTRUCTION";
        break;
    case EXCEPTION_SINGLE_STEP:
        exName = "EXCEPTION_SINGLE_STEP";
        break;
    case EXCEPTION_STACK_OVERFLOW:
        exName = "EXCEPTION_STACK_OVERFLOW";
        break;
    default:
        exName = malloc(sizeof(char) * 64);  // Let this leak.  It only happens once before shutdown.
        sprintf(exName, "Unknown Exception (%ld)", exCode);
        break;
    }

    wrapperLogS(WRAPPER_SOURCE_WRAPPER, "  exceptionCode    = %s", exName);
    wrapperLogS(WRAPPER_SOURCE_WRAPPER, "  exceptionFlag    = %s", 
        (exceptionPointers->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE ? "EXCEPTION_NONCONTINUABLE" : "EXCEPTION_NONCONTINUABLE_EXCEPTION"));
    wrapperLogI(WRAPPER_SOURCE_WRAPPER, "  exceptionAddress = %p", (int)exceptionPointers->ExceptionRecord->ExceptionAddress);
    if (exCode == EXCEPTION_ACCESS_VIOLATION) {
        if (exceptionPointers->ExceptionRecord->ExceptionInformation[0] == 0) {
            wrapperLogI(WRAPPER_SOURCE_WRAPPER, "  Read access exception from %p", 
                (int)exceptionPointers->ExceptionRecord->ExceptionInformation[1]);
        } else {
            wrapperLogI(WRAPPER_SOURCE_WRAPPER, "  Write access exception to %p", 
                (int)exceptionPointers->ExceptionRecord->ExceptionInformation[1]);
        }
    } else {
        for (i = 0; i < (int)exceptionPointers->ExceptionRecord->NumberParameters; i++) {
            wrapperLogIL(WRAPPER_SOURCE_WRAPPER, "  exceptionInformation[%d] = %ld", i,
                exceptionPointers->ExceptionRecord->ExceptionInformation[i]);
        }
    }

    return TRUE;
}

/**
 * Show usage
 */
void wrapperUsage(char *appName) {
    printf("Usage:\n");
    printf("  %s [command] [config file]\n", appName);
    printf("\n");
    printf("where commands include:\n");
    printf("  -c   run as a console application\n");
    printf("  -i   install as an NT service\n");
    printf("  -r   remove as an NT service\n");
    printf("  -?   print this help message\n");
    printf("\n");
}
void _CRTAPI1 main(int argc, char **argv) {
    int result = 0;

    // The StartServiceCtrlDispatcher requires this table to specify
    // the ServiceMain function to run in the calling process. The first
    // member in this example is actually ignored, since we will install
    // our service as a SERVICE_WIN32_OWN_PROCESS service type. The NULL
    // members of the last entry are necessary to indicate the end of 
    // the table;
    SERVICE_TABLE_ENTRY serviceTable[2];
    /*
      = {
      { NULL, NULL }, // Just a placeholder
      { NULL, NULL }
      };
    */

    __try {
        // Initialize the WrapperConfig structure
        wrapperData = (WrapperConfig *)malloc(sizeof(WrapperConfig));
        wrapperData->isConsole = TRUE;
        wrapperData->wState = WRAPPER_WSTATE_STARTING;
        wrapperData->jState = WRAPPER_JSTATE_DOWN;
        wrapperData->logFile = NULL;
        wrapperData->jvmCommand = NULL;
        wrapperData->exitRequested = FALSE;
        wrapperData->exitAcknowledged = FALSE;
        wrapperData->exitCode = 0;
        wrapperData->restartRequested = FALSE;
        wrapperData->jvmRestarts = 0;

        switch(argc) {
        case 1:
            // If main is called without any arguments, it will probably be by the
            // service control manager, in which case StartServiceCtrlDispatcher
            // must be called here. A message will be printed just in case this 
            // happens from the console.

            // In this case, this must be a modified version of the EXE with the
            //	config file hard coded in.  Check this
            if (strstr(staticConfigFilename, "zzzzzzzzzz") == staticConfigFilename) {
                // This is an unmodified version of the EXE
                // The user needs to be shown the usage info.
                wrapperUsage(argv[0]);
                result = 1;
            } else {
                // The EXE has been modified, so we can continue
                properties = loadProperties(staticConfigFilename);
                if (properties == NULL) {
                    // File not found.
                    wrapperLogS(WRAPPER_SOURCE_WRAPPER, "unable to open config file. %s", staticConfigFilename);
                    result = 1;
                } else {
                    // Store the config file name
                    wrapperData->configFile = staticConfigFilename;

                    // Apply properties to the WrapperConfig structure
                    wrapperLoadConfiguration();

                    // Prepare the service table
                    serviceTable[0].lpServiceName = wrapperData->ntServiceName;
                    serviceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)wrapperServiceMain;
                    serviceTable[1].lpServiceName = NULL;
                    serviceTable[1].lpServiceProc = NULL;

                    printf("Attempting to start %s as an NT service.\n", wrapperData->ntServiceDisplayName);
                    printf("\nCalling StartServiceCtrlDispatcher...please wait.\n");

                    // Start the service control dispatcher.  This will not return
                    //  if the service is started without problems
                    if (!StartServiceCtrlDispatcher(serviceTable)){
                        printf("\nStartServiceControlDispatcher failed!\n");
                        printf("\nFor help, type\n\n%s /?\n\n", argv[0]);
                        result = 1;
                    }
                }
            }
            break;
        case 3:
            // Make sure that this is not an altered version of the EXE
            if (strstr(staticConfigFilename, "zzzzzzzzzz") != staticConfigFilename) {
                // This is a modified version of the EXE

                printf("The exe %s was built for the Service defined in the config file %s.  " \
                       "This file should not be run manually.\n\n", argv[0], staticConfigFilename);
                result = 1;
            } else {
                // The first parameter should be a command and the second should be a config file.
                if(!_stricmp(argv[1],"-i") || !_stricmp(argv[1],"/i")) {
                    properties = loadProperties(argv[2]);
                    if (properties == NULL) {
                        // File not found.
                        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "unable to open config file. %s", argv[2]);
                        result = 1;
                    } else {
                        // Store the config file name
                        wrapperData->configFile = argv[2];

                        // Apply properties to the WrapperConfig structure
                        wrapperLoadConfiguration();

                        result = wrapperInstall(argv[0], argv[2]);
                    }
                } else if(!_stricmp(argv[1],"-r") || !_stricmp(argv[1],"/r")) {
                    properties = loadProperties(argv[2]);
                    if (properties == NULL) {
                        // File not found.
                        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "unable to open config file. %s", argv[2]);
                        result = 1;
                    } else {
                        // Store the config file name
                        wrapperData->configFile = argv[2];

                        // Apply properties to the WrapperConfig structure
                        wrapperLoadConfiguration();

                        result = wrapperRemove(argv[0], argv[2]);
                    }
                } else if(!_stricmp(argv[1],"-c") || !_stricmp(argv[1],"/c")) {
                    properties = loadProperties(argv[2]);
                    if (properties == NULL) {
                        // File not found.
                        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "unable to open config file. %s", argv[2]);
                        result = 1;
                    } else {
                        // Store the config file name
                        wrapperData->configFile = argv[2];

                        // Apply properties to the WrapperConfig structure
                        wrapperLoadConfiguration();

                        //result = wrapperConsole(argv[0], argv[2]);
                        result = wrapperRunConsole();
                    }
                } else if(!_stricmp(argv[1],"-?") || !_stricmp(argv[1],"/?")) {
                    wrapperUsage(argv[0]);
                } else {
                    printf("\nUnrecognized option: %s\n", argv[1]);
                    wrapperUsage(argv[0]);
                    result = 1;
                }
            }
            break;
        default:
            // Invalid parameters were provided.
            if (strstr(staticConfigFilename, "zzzzzzzzzz") != staticConfigFilename) {
                // This is a modified version of the EXE

                printf("The exe %s was built for the Service " \
                       "defined in the config file %s.  This file should not be run manually.\n\n",
                       argv[0], staticConfigFilename);
                result = 1;
            } else {
                wrapperUsage(argv[0]);
                result = 1;
            }
            break;
        }
    } __except (exceptionFilterFunction(GetExceptionInformation())) {
        wrapperLog(WRAPPER_SOURCE_WRAPPER, "<-- Wrapper Stopping due to error");
        result = 1;
    }

    exit(result);
}

#endif // ifdef WIN32
