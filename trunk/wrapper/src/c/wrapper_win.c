/*
 * Copyright (c) 1999, 2004 Tanuki Software
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of the Java Service Wrapper and associated
 * documentation files (the "Software"), to deal in the Software
 * without  restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sub-license,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 * 
 * Portions of the Software have been derived from source code
 * developed by Silver Egg Technology under the following license:
 * 
 * Copyright (c) 2001 Silver Egg Technology
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without 
 * restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sub-license, and/or 
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following 
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 *
 * $Log$
 * Revision 1.72  2004/05/31 08:08:22  mortenson
 * Clean up some data types.  Should have no effect on actual functionality.
 *
 * Revision 1.71  2004/05/26 06:56:18  mortenson
 * Fix a problem where CTRL-C was not being handled correctly if the console
 * was configured to be shown when running as an NT service.
 *
 * Revision 1.70  2004/04/08 14:58:59  mortenson
 * Add a wrapper.working.dir property.
 *
 * Revision 1.69  2004/04/08 03:21:57  mortenson
 * Added an environment variable, WRAPPER_PATH_SEPARATOR, whose value is set
 * to either ':' or ';' on startup.
 *
 * Revision 1.68  2004/03/27 16:09:46  mortenson
 * Add wrapper.on_exit.<n> properties to control what happens when a exits based
 * on the exit code.  This led to a major rework of the state engine to make it possible.
 *
 * Revision 1.67  2004/03/10 14:09:23  mortenson
 * Fix a potential access violation with very large system paths.
 * Fix a potential problem with the catch block executing before the logger was
 * initialized.
 *
 * Revision 1.66  2004/01/24 17:13:40  mortenson
 * Make sure that we only attempt to set the console title when it is actually going
 * to be visible.
 *
 * Revision 1.65  2004/01/16 04:42:00  mortenson
 * The license was revised for this version to include a copyright omission.
 * This change is to be retroactively applied to all versions of the Java
 * Service Wrapper starting with version 3.0.0.
 *
 * Revision 1.64  2004/01/14 09:35:14  mortenson
 * Modify so that the exit code returned by the last JVM is always used when
 * exiting the Wrapper.
 *
 * Revision 1.63  2004/01/10 15:51:32  mortenson
 * Fix a problem where a thread dump would be invoked if the request thread
 * dump on failed JVM exit was enabled and the user forced an immediate
 * shutdown by pressing CTRL-C more than once.
 *
 * Revision 1.62  2004/01/09 18:22:41  mortenson
 * The code timing the thread dump before a shutdown was still based on the system
 * time, changed over to ticks.  Also extended the time from 3 to 5 seconds.
 *
 * Revision 1.61  2004/01/09 17:49:00  mortenson
 * Rework the logging so it is now threadsafe.
 *
 * Revision 1.60  2004/01/09 05:15:11  mortenson
 * Implement a tick timer and convert the system time over to be compatible.
 *
 * Revision 1.59  2003/10/31 10:16:27  mortenson
 * Improved the algorithm of the request thread dump on failed JVM exit feature
 * so that extremely large thread dumps will not be truncated when the JVM
 * is killed.
 *
 * Revision 1.58  2003/10/31 03:57:17  mortenson
 * Add a new property, wrapper.console.title, which makes it possible to set
 * the title of the console in which the Wrapper is currently running.
 *
 * Revision 1.57  2003/10/30 19:34:34  mortenson
 * Added a new wrapper.ntservice.console property so the console can be shown for
 * services.
 * Fixed a problem where requesting thread dumps on exit was failing when running
 * as a service.
 *
 * Revision 1.56  2003/10/18 16:19:40  mortenson
 * Commit a patch by Eric Smith which corrects a misuse of the putenv function.
 * Also cleaned up some DEBUG ifdef code.
 *
 * Revision 1.55  2003/10/07 08:10:57  mortenson
 * The Windows version of the Wrapper was not correctly registering that it
 * would accept SHUTDOWN messages when running as a service.
 * Thanks to Jason Tishler for noticing this and sending in a patch.
 *
 * Revision 1.54  2003/09/09 14:18:10  mortenson
 * Fix a problem where not all properties specified on the command line worked
 * correctly when they included spaces.
 *
 * Revision 1.53  2003/09/04 06:08:17  mortenson
 * Extend the pause between requesting a thread dump on exit and killing the JVM.
 *
 * Revision 1.52  2003/09/03 02:33:38  mortenson
 * Requested restarts no longer reset the restart count.
 * Add new wrapper.ignore_signals property.
 *
 * Revision 1.51  2003/08/15 17:16:18  mortenson
 * Fix tabs.
 *
 * Revision 1.50  2003/08/15 17:13:17  mortenson
 * Added the wrapper.java.pidfile property which will cause the pid of the
 * java process to be written to a specified file.
 *
 * Revision 1.49  2003/08/15 16:30:52  mortenson
 * Added support for the wrapper.pidfile property on the Windows platform.
 *
 * Revision 1.48  2003/08/02 06:49:13  mortenson
 * Changed the way environment variables are loaded from the registry on Windows
 * platforms so users will no longer get warning messages about not being able
 * to handle very large environment variables.
 *
 * Revision 1.47  2003/07/04 03:36:05  mortenson
 * Improve the error message displayed on Windows when the configured Java
 * command can not be executed or does not exist.
 *
 * Revision 1.46  2003/07/04 03:18:36  mortenson
 * Improve the error message displayed when the NT EventLog is full in response
 * to feature request #643617.
 *
 * Revision 1.45  2003/07/02 04:01:52  mortenson
 * Implement the ability to specify an NT service's load order group in response
 * to feature request #764143.
 *
 * Revision 1.44  2003/06/10 14:22:01  mortenson
 * Fix bug #744801.  A Java GUI was not being displayed when the application was
 * run in either console mode or as a service with wrapper.ntservice.interactive
 * enabled on JVM versions prior to 1.4.0.
 *
 * Revision 1.43  2003/05/30 09:11:16  mortenson
 * Added -t and -p command line options to the Windows version of the Wrapper
 * to sTart and stoP the Wrapper as an NT service.  This can be used in place
 * of "net start" and "net stop", which do not always work correctly when a
 * service takes a long time to start up or shutdown.  See the Launch Overview
 * for more details.
 *
 * Revision 1.42  2003/05/29 10:00:10  mortenson
 * Reduce the frequency of "Waiting to stop..." messages displayed when removing
 * an NT service that is currently running.  Decreased frequency from once per
 * second to once every five seconds.
 *
 * Revision 1.41  2003/05/29 09:47:58  mortenson
 * Add some debug output to make it possible to verify that the NT ServiceManager
 * is correctly being kept up to date on the status of the service.
 *
 * Revision 1.40  2003/05/01 03:15:03  mortenson
 * Rework some code to make its flow clearer.  Should be no functional change.
 *
 * Revision 1.39  2003/04/16 04:13:11  mortenson
 * Go through and clean up the computation of the number of bytes allocated in
 * malloc statements to make sure that string sizes are always multiplied by
 * sizeof(char), etc.
 *
 * Revision 1.38  2003/04/15 23:24:22  mortenson
 * Remove casts from all malloc statements.
 *
 * Revision 1.37  2003/04/09 09:17:58  mortenson
 * Fix a problem where environment variables in the registry which had no value
 * were causing the Wrapper to crash with an access violation.
 *
 * Revision 1.36  2003/04/09 04:03:19  mortenson
 * Fix a problem where the inability to expand very large environment variables
 * was causing an access violation when run as an NT service.
 * Added a bunch of output to the DEBUG build of the Wrapper to help with tracking
 * down problems like this.
 *
 * Revision 1.35  2003/04/03 07:37:00  mortenson
 * In the last release, some work was done to avoid false timeouts caused by
 * large quantities of output.  On some heavily loaded systems, timeouts were
 * still being encountered.  Rather than reading up to 50 lines of input, the
 * code will now read for a maximum of 250ms before returning to give the main
 * event loop more cycles.
 *
 * Revision 1.34  2003/04/03 04:05:22  mortenson
 * Fix several typos in the docs.  Thanks to Mike Castle.
 *
 * Revision 1.33  2003/03/21 21:25:33  mortenson
 * Fix a problem where very heavy output from the JVM can cause the Wrapper to
 * give a false timeout.  The Wrapper now only ready 50 lines of input at a time
 * to guarantee that the Wrapper's event loop always gets cycles.
 *
 * Revision 1.32  2003/03/13 15:40:42  mortenson
 * Add the ability to set environment variables from within the configuration
 * file or from the command line.
 *
 * Revision 1.31  2003/02/08 14:35:40  mortenson
 * Modify the Win32 version of the Wrapper so that Environment Variables are
 * always read from the system registry when the Wrapper is run as a service.
 *
 * Revision 1.30  2003/02/07 16:05:28  mortenson
 * Implemented feature request #676599 to enable the filtering of JVM output to
 * trigger JVM restarts or Wrapper shutdowns.
 *
 * Revision 1.29  2003/02/07 02:48:17  mortenson
 * Fixed a problem where missing environment variables specified in classpath
 * or library path properties were not being handled correctly.
 *
 * Revision 1.28  2003/02/03 06:55:27  mortenson
 * License transfer to TanukiSoftware.org
 *
 */

/**
 * Author:
 *   Leif Mortenson <leif@tanukisoftware.com>
 *
 * Version CVS $Revision$ $Date$
 */

#ifndef WIN32
/* For some reason this is not defined sometimes when I build $%$%$@@!! */
barf
#endif

#ifdef WIN32

#include <direct.h>
#include <io.h>
#include <math.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <sys/timeb.h>
#include <conio.h>

#include "wrapper.h"
#include "property.h"
#include "logger.h"

/*****************************************************************************
 * Win32 specific variables and procedures                                   *
 *****************************************************************************/
SERVICE_STATUS          ssStatus;       
SERVICE_STATUS_HANDLE   sshStatusHandle;

#define SYSTEM_PATH_MAX_LEN 256
static char *systemPath[SYSTEM_PATH_MAX_LEN];
static HANDLE wrapperProcess = NULL;
static DWORD  wrapperProcessId = 0;
static HANDLE wrapperChildStdoutWr = NULL;
static HANDLE wrapperChildStdoutRd = NULL;
static int    wrapperChildStdoutRdLastLF = 0;

char wrapperClasspathSeparator = ';';

/* Flag which is set if this process creates a pid file. */
int ownPidFile = 0;
int ownJavaPidFile = 0;

HANDLE timerThreadHandle;
DWORD timerThreadId;
/* Initialize the timerTicks to a very high value.  This means that we will
 *  always encounter the first rollover (256 * WRAPPER_MS / 1000) seconds
 *  after the Wrapper the starts, which means the rollover will be well
 *  tested. */
DWORD timerTicks = 0xffffff00;

int exceptionFilterFunction(PEXCEPTION_POINTERS exceptionPointers);

/******************************************************************************
 * Windows specific code
 ******************************************************************************/
/**
 * Builds an array in memory of the system path.
 */
void buildSystemPath() {
    char *envBuffer;
    int len, i;
    char *c, *lc;

    /* Get the length of the PATH environment variable. */
    len = GetEnvironmentVariable("PATH", NULL, 0);
    if (len == 0) {
        /* PATH not set on this system */
        systemPath[0] = NULL;
        return;
    }

    /* Allocate the memory to hold the PATH */
    envBuffer = malloc(sizeof(char) * len);
    GetEnvironmentVariable("PATH", envBuffer, len);

#ifdef _DEBUG
    printf("Getting the system path: %s\n", envBuffer);
#endif

    /* Build an array of the path elements.  To make it easy, just
     *  assume there won't be more than 255 path elements. Verified
     *  in the loop. */
    i = 0;
    lc = envBuffer;
    /* Get the elements ending in a ';' */
    while (((c = strchr(lc, ';')) != NULL) && (i < SYSTEM_PATH_MAX_LEN - 2))
    {
        len = c - lc;
        systemPath[i] = malloc(sizeof(char) * (len + 1));
        memcpy(systemPath[i], lc, len);
        systemPath[i][len] = '\0';
#ifdef _DEBUG
        printf("PATH[%d]=%s\n", i, systemPath[i]);
#endif
        lc = c + 1;
        i++;
    }
    /* There should be one more value after the last ';' */
    len = strlen(lc);
    systemPath[i] = malloc(sizeof(char) * (len + 1));
    strcpy(systemPath[i], lc);
#ifdef _DEBUG
    printf("PATH[%d]=%s\n", i, systemPath[i]);
#endif
    i++;
    /* NULL terminate the array. */
    systemPath[i] = NULL;
#ifdef _DEBUG
    printf("PATH[%d]=<null>\n", i);
#endif
    i++;

    /* Release the environment variable memory. */
    free(envBuffer);
}
char** wrapperGetSystemPath() {
    return systemPath;
}

/**
 * exits the application after running shutdown code.
 */
void appExit(int exitCode) {
    /* Remove pid file if it was registered and created by this process. */
    if ((ownPidFile) && (wrapperData->pidFilename)) {
        unlink(wrapperData->pidFilename);
        ownPidFile = 0;
    }

    /* Do this here to unregister the syslog resources on exit.*/
    /*unregisterSyslogMessageFile(); */
    exit(exitCode);
}

/**
 * Gets the error code for the last operation that failed.
 */
int wrapperGetLastError() {
    return WSAGetLastError();
}

/**
 * Initialize the pipe which will be used to capture the output from the child
 * process.
 */
int wrapperInitChildPipe() {
    SECURITY_ATTRIBUTES saAttr;
    HANDLE childStdoutRd;

    /* Set the bInheritHandle flag so pipe handles are inherited. */
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    /* Create a pipe for the child process's STDOUT. */
    if (!CreatePipe(&childStdoutRd, &wrapperChildStdoutWr, &saAttr, 0)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Stdout pipe creation failed");
        return -1;
    }

    /* Create a noninheritable read handle and close the inheritable read handle. */
    if (!DuplicateHandle(GetCurrentProcess(), childStdoutRd, GetCurrentProcess(), &wrapperChildStdoutRd, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "DuplicateHandle failed");
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
    int halt = FALSE;

    /* Immediately register this thread with the logger. */
    logRegisterThread(WRAPPER_THREAD_SIGNAL);

    switch (key) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
        /* The user hit CTRL-C.  Can only happen when run as a console. */
        if (wrapperData->ignoreSignals) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "CTRL-C trapped, but ignored.");
        } else {
            /*  Always quit. */
            if (wrapperData->exitRequested || wrapperData->restartRequested ||
                (wrapperData->jState == WRAPPER_JSTATE_STOPPING) ||
                (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
                (wrapperData->jState == WRAPPER_JSTATE_DOWN)) {

                /* Pressed CTRL-C while we were already shutting down. */
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "CTRL-C trapped.  Forcing immediate shutdown.");
                halt = TRUE;
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "CTRL-C trapped.  Shutting down.");
            }
            quit = TRUE;
        }
        break;

    case CTRL_BREAK_EVENT:
        /* The user hit CTRL-BREAK */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "CTRL-BREAK/PAUSE trapped.  Asking the JVM to dump its state.");

        /* If the java process was launched using the same console, ie where processflags=CREATE_NEW_PROCESS_GROUP; */
        /* then the java process will also get this message, so it can be ignored here.                             */
        /*
        requestDumpJVMState();
        */

        quit = FALSE;
        break;

    case CTRL_LOGOFF_EVENT:
        /* Happens when the user logs off.  We should quit when run as a */
        /*  console, but stay up when run as a service. */
        if (wrapperData->isConsole) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "User logged out.  Shutting down.");
            quit = TRUE;
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "User logged out.  Ignored.");
            quit = FALSE;
        }
        break;
    case CTRL_SHUTDOWN_EVENT:
        /* Happens when the machine is shutdown or rebooted.  Always quit. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Machine is shutting down.");
        quit = TRUE;
        break;
    default:
        /* Unknown.  Don't quit here. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Trapped unexpected console signal (%d).  Ignored.", key);
        quit = FALSE;
    }

    if (quit) {
        if (halt) {
            /* Disable the thread dump on exit feature if it is set because it
             *  should not be displayed when the user requested the immediate exit. */
            wrapperData->requestThreadDumpOnFailedJVMExit = FALSE;
            wrapperKillProcess();
        } else {
            wrapperStopProcess(0);
        }
        /* Don't actually kill the process here.  Let the application shut itself down */

        /* To make sure that the JVM will not be restarted for any reason,
         *  start the Wrapper shutdown process as well. */
        if ((wrapperData->wState == WRAPPER_WSTATE_STOPPING) ||
            (wrapperData->wState == WRAPPER_WSTATE_STOPPED)) {
            /* Already stopping. */
        } else {
            wrapperData->wState = WRAPPER_WSTATE_STOPPING;
        }
    }

    return TRUE; /* We handled the event. */
}



/******************************************************************************
 * Platform specific methods
 *****************************************************************************/

/**
 * Send a signal to the JVM process asking it to dump its JVM state.
 */
void requestDumpJVMState() {
    if (wrapperProcess != NULL) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Dumping JVM state.");
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Sending BREAK event to process group %ld.", wrapperProcessId);
        if ( GenerateConsoleCtrlEvent( CTRL_BREAK_EVENT, wrapperProcessId ) == 0 ) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unable to send BREAK event to JVM process.  Err(%ld : %s)",
                GetLastError(), getLastErrorText());
        }
    }
}

void wrapperBuildJavaCommand() {
    int commandLen;
    char **strings;
    int length, i;

    /* If this is not the first time through, then dispose the old command */
    if (wrapperData->jvmCommand) {
#ifdef _DEBUG
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Clearing up old command line");
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Old Command Line \"%s\"", wrapperData->jvmCommand);
#endif
        free(wrapperData->jvmCommand);
        wrapperData->jvmCommand = NULL;
    }

    /* Build the Java Command Strings */
    strings = NULL;
    length = 0;
    wrapperBuildJavaCommandArray(&strings, &length, TRUE);
    
#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM Command Line Parameters");
    for (i = 0; i < length; i++) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "%d : %s", i, strings[i]);
    }
#endif

    /* Build a single string from the array */
    /* Calculate the length */
    commandLen = 0;
    for (i = 0; i < length; i++) {
        if (i > 0) {
            commandLen++; /* Space */
        }
        commandLen += strlen(strings[i]);
    }
    commandLen++; /* '\0' */

    /* Build the actual command */
    wrapperData->jvmCommand = malloc(sizeof(char) * commandLen);
    commandLen = 0;
    for (i = 0; i < length; i++) {
        if (i > 0) {
            wrapperData->jvmCommand[commandLen++] = ' ';
        }
        sprintf((char *)(&(wrapperData->jvmCommand[commandLen])), "%s", strings[i]);
        commandLen += strlen(strings[i]);
    }
    wrapperData->jvmCommand[commandLen++] = '\0';

    /* Free up the temporary command array */
    wrapperFreeJavaCommandArray(strings, length);
}

void hideConsoleWindow(HWND consoleHandle) {
    WINDOWPLACEMENT consolePlacement;
    
    if (GetWindowPlacement(consoleHandle, &consolePlacement)) {
        /* Hide the Window. */
        consolePlacement.showCmd = SW_HIDE;

        /* If we hide the window too soon after it is shown, it sometimes sticks, so wait a moment. */
        Sleep(10);

        if (!SetWindowPlacement(consoleHandle, &consolePlacement)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "Unable to set window placement information: %s", getLastErrorText());
        }
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "Unable to obtain window placement information: %s", getLastErrorText());
    }
}

HWND findAndHideConsoleWindow( char *title ) {
    HWND consoleHandle;
    int i = 0;

    /* Allow up to 2 seconds for the window to show up, but don't hang
     *  up if it doesn't */
    consoleHandle = NULL;
    while ((!consoleHandle) && (i < 200)) {
        Sleep(10);
        consoleHandle = FindWindow("ConsoleWindowClass", title);
        i++;
    }
    if (consoleHandle != NULL) {
        hideConsoleWindow(consoleHandle);
    }
    
    return consoleHandle;
}

void showConsoleWindow(HWND consoleHandle) {
    WINDOWPLACEMENT consolePlacement;
    
    if (GetWindowPlacement(consoleHandle, &consolePlacement)) {
        /* Show the Window. */
        consolePlacement.showCmd = SW_SHOW;

        if (!SetWindowPlacement(consoleHandle, &consolePlacement)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "Unable to set window placement information: %s", getLastErrorText());
        }
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "Unable to obtain window placement information: %s", getLastErrorText());
    }
}

/**
 * The main entry point for the timer thread which is started by
 *  initializeTimer().  Once started, this thread will run for the
 *  life of the process.
 *
 * This thread will only be started if we are configured NOT to
 *  use the system time as a base for the tick counter.
 */
DWORD WINAPI timerRunner(LPVOID parameter) {
    DWORD sysTicks;
    DWORD lastTickOffset = 0;
    DWORD tickOffset;
    int offsetDiff;
    int first = 1;

    /* In case there are ever any problems in this thread, enclose it in a try catch block. */
    __try {
        /* Immediately register this thread with the logger. */
        logRegisterThread(WRAPPER_THREAD_TIMER);

        while(TRUE) {
            Sleep(WRAPPER_TICK_MS);

            /* Get the tick count based on the system time. */
            sysTicks = wrapperGetSystemTicks();

            /* Advance the timer tick count. */
            timerTicks++;

            /* Calculate the offset between the two tick counts. This will always work due to overflow. */
            tickOffset = sysTicks - timerTicks;

            /* The number we really want is the difference between this tickOffset and the previous one. */
            offsetDiff = (int)(tickOffset - lastTickOffset);

            if (first) {
                first = 0;
            } else {
                if (offsetDiff > wrapperData->timerSlowThreshold) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "The timer fell behind the system clock by %dms.", (int)(offsetDiff * WRAPPER_TICK_MS));
                } else if (offsetDiff < -1 * wrapperData->timerFastThreshold) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "The system clock fell behind the timer by %dms.", (int)(-1 * offsetDiff * WRAPPER_TICK_MS));
                }
                /*
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Timer running: %u, %u, %u, %d", timerTicks, sysTicks, tickOffset, offsetDiff);
                */
            }

            /* Store this tick offset for the next time through the loop. */
            lastTickOffset = tickOffset;
        }
    } __except (exceptionFilterFunction(GetExceptionInformation())) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Fatal error in the Timer thread.");
        appExit(1);
        return 1; /* For the compiler, we will never get here. */
    }
}

/**
 * Creates a process whose job is to loop and simply increment a ticks
 *  counter.  The tick counter can then be used as a clock as an alternative
 *  to using the system clock.
 */
int initializeTimer() {
    timerThreadHandle = CreateThread(
        NULL, /* No security attributes as there will not be any child processes of the thread. */
        0,    /* Use the default stack size. */
        timerRunner,
        NULL, /* No parameters need to passed to the thread. */
        0,    /* Start the thread running immediately. */
        &timerThreadId
        );
    if (!timerThreadHandle) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
            "Unable to create a timer thread: %s", getLastErrorText());
        return 1;
    } else {
        return 0;
    }
}

/**
 * Execute initialization code to get the wrapper set up.
 */
int wrapperInitialize() {
    WORD ws_version=MAKEWORD(1, 1);
    WSADATA ws_data;
    HANDLE hStdout;
    int res;
    char titleBuffer[80];

    /* Set the process priority. */
    HANDLE process = GetCurrentProcess();
    SetPriorityClass(process, wrapperData->ntServicePriorityClass);

    /* Initialize Winsock */
    if ((res = WSAStartup(ws_version, &ws_data)) != 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Cannot initialize Windows socket DLLs.");
        return res;
    }

    /* Initialize the pipe to capture the child process output */
    if ((res = wrapperInitChildPipe()) != 0) {
        return res;
    }

    /* Initialize the Wrapper console handle to null */
    wrapperData->wrapperConsoleHandle = NULL;
    
    /* The Wrapper will not have its own console when running as a service.  We need
     *  to create one here. */
    if ((!wrapperData->isConsole) && (wrapperData->ntAllocConsole)) {
#ifdef _DEBUG
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Allocating a console for the service.");
#endif

        if (!AllocConsole()) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                "ERROR: Unable to allocate a console for the service: %s", getLastErrorText());
            return 1;
        }

        hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hStdout == INVALID_HANDLE_VALUE) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                "ERROR: Unable to get the new stdout handle: %s", getLastErrorText());
           return 1;
        }
        setConsoleStdoutHandle( hStdout );

        if (wrapperData->ntHideWrapperConsole) {
            /* A console needed to be allocated for the process but it should be hidden. */
#ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Hiding the console.");
#endif

            /* Generate a unique time for the console so we can look for it below. */
            sprintf(titleBuffer, "Wrapper Console ID %d%d (Do not close)", rand(), rand());

            SetConsoleTitle( titleBuffer );

            wrapperData->wrapperConsoleHandle = findAndHideConsoleWindow( titleBuffer );
        }
    }

    /* Attempt to set the console tilte if it exists and is accessable. */
    if (wrapperData->consoleTitle) {
        if (wrapperData->isConsole || (wrapperData->ntServiceInteractive && !wrapperData->ntHideWrapperConsole)) {
            /* The console should be visible. */
            if (!SetConsoleTitle(wrapperData->consoleTitle)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    "Attempt to set the console title failed: %s", getLastErrorText());
            }
        }
    }

    /* Set the handler to trap console signals.  This must be done after the console
     *  is created or it will not be applied to that console. */
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)wrapperConsoleHandler, TRUE);

    if (wrapperData->useSystemTime) {
        /* We are going to be using system time so there is no reason to start up a timer thread. */
        timerThreadHandle = NULL;
        timerThreadId = 0;
    } else {
        /* Create and initialize a timer thread. */
        if ((res = initializeTimer()) != 0) {
            return res;
        }
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
    char *natStateName;
    static DWORD dwCheckPoint = 1;
    BOOL bResult = TRUE;

    switch (status) {
    case WRAPPER_WSTATE_STARTING:
        natState = SERVICE_START_PENDING;
        natStateName = "SERVICE_START_PENDING";
        break;
    case WRAPPER_WSTATE_STARTED:
        natState = SERVICE_RUNNING;
        natStateName = "SERVICE_RUNNING";
        break;
    case WRAPPER_WSTATE_STOPPING:
        natState = SERVICE_STOP_PENDING;
        natStateName = "SERVICE_STOP_PENDING";
        break;
    case WRAPPER_WSTATE_STOPPED:
        natState = SERVICE_STOPPED;
        natStateName = "SERVICE_STOPPED";
        break;
    default:
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unknown status: %d", status);
        return;
    }

    if (!wrapperData->isConsole) {
        if (natState == SERVICE_START_PENDING) {
            ssStatus.dwControlsAccepted = 0;
        } else {
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
        }

        ssStatus.dwCurrentState = natState;
        if (errorCode == 0) {
            ssStatus.dwWin32ExitCode = NO_ERROR;
            ssStatus.dwServiceSpecificExitCode = 0;
        } else {
            ssStatus.dwWin32ExitCode = errorCode; //ERROR_SERVICE_SPECIFIC_ERROR;
            ssStatus.dwServiceSpecificExitCode = 0; //errorCode;
        }
        ssStatus.dwWaitHint = waitHint;

        if ((natState == SERVICE_RUNNING ) || (natState == SERVICE_STOPPED)) {
            ssStatus.dwCheckPoint = 0;
        } else {
            ssStatus.dwCheckPoint = dwCheckPoint++;
        }

        if (wrapperData->isStateOutputEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                "calling SetServiceStatus with status=%s, waitHint=%d, checkPoint=%u, errorCode=%d",
                natStateName, waitHint, dwCheckPoint, errorCode);
        }

        if (!(bResult = SetServiceStatus(sshStatusHandle, &ssStatus))) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "SetServiceStatus failed");
        }
    }
}

/**
 * Read and process any output from the child JVM Process.
 * Most output should be logged to the wrapper log file.
 *
 * This function will only be allowed to run for 250ms before returning.  This is to
 *  make sure that the main loop gets CPU.  If there is more data in the pipe, then
 *  the function returns -1, otherwise 0.  This is a hint to the mail loop not to
 *  sleep.
 */
int wrapperReadChildOutput() {
    DWORD dwRead, dwAvail;
    int lfPos;
    char chBuf[1025];
    char *c;
    int thisLF;
    struct timeb timeBuffer;
    long startTime;
    int startTimeMillis;
    long now;
    int nowMillis;
    long durr;

    ftime( &timeBuffer );
    startTime = now = timeBuffer.time;
    startTimeMillis = nowMillis = timeBuffer.millitm;

    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "now=%ld, nowMillis=%d", now, nowMillis);
    */

    /* Loop and read as much input as is available.  When a large amount of output is
     *  being piped from the JVM this can lead to the main event loop not getting any
     *  CPU for an extended period of time.  To avoid that problem, this loop is only
     *  allowed to cycle for 250ms before returning. */
    while((durr = (now - startTime) * 1000 + (nowMillis - startTimeMillis)) < 250) {
        /*
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "durr=%ld", durr);
        */

        /* Find out how much data there is in the pipe before we try to read it. */
        if (!PeekNamedPipe(wrapperChildStdoutRd, chBuf, 1024, &dwRead, &dwAvail, NULL) || dwRead == 0 || dwAvail == 0) {
            /*printf("stdout avail %d.", dwAvail); */
            break;
        }
        /*printf("stdout avail %d.", dwAvail); */

        /* We only want to read in a line at a time.  We want to read in a line */
        /*	feed if it exists, but we want to remove it from the buffer to be logged. */
        if (dwAvail > 1024) {
            dwAvail = 1024;
        }

        /* Terminate the read buffer */
        chBuf[dwRead] = '\0';

        lfPos = dwRead;

        thisLF = 0;

        /* Look for a CR */
        if ((c = strchr(chBuf, (char)0x0d)) != NULL) {
            if (c - chBuf < lfPos) {
                lfPos = c - chBuf;
                if (chBuf[lfPos + 1] == (char)0x0a) {
                    /* Also read in the CR+LF */
                    dwAvail = lfPos + 2;
                    /*printf("CR+LF"); */
                    thisLF = 1;
                } else {
                    /* Also read in the CR */
                    dwAvail = lfPos + 1;
                    /*printf("CR"); */
                    thisLF = 1;
                }
            }
        }

        /* Look for a LF */
        if ((c = strchr(chBuf, (char)0x0a)) != NULL) {
            if (c - chBuf < lfPos) {
                lfPos = c - chBuf;
                /* Also read in the LF */
                dwAvail = lfPos + 1;
                /*printf("LF"); */
                thisLF = 1;
            }
        }

        if (!ReadFile(wrapperChildStdoutRd, chBuf, dwAvail, &dwRead, NULL) || dwRead == 0) {
            /*printf("stdout read %d.\n", dwRead); */
            break;
        }
        /* Write over the lf */
        chBuf[lfPos] = '\0';
        /* Also set the a '\0' at the dwRead location just to be safe. */
        chBuf[dwRead] = '\0';

        /* Make sure that this is just another LF if the last line was missing it's LF */
        if ((lfPos > 0) || (wrapperChildStdoutRdLastLF)) {
            wrapperLogChildOutput(chBuf);
        }

        wrapperChildStdoutRdLastLF = thisLF;

        /* Get the time again */
        ftime( &timeBuffer );
        now = timeBuffer.time;
        nowMillis = timeBuffer.millitm;
    }
    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "done durr=%ld", durr);
    */
    if ((durr = (now - startTime) * 1000 + (nowMillis - startTimeMillis)) < 250) {
        return 0;
    } else {
        return 1;
    }
}

/**
 * Checks on the status of the JVM Process.
 * Returns WRAPPER_PROCESS_UP or WRAPPER_PROCESS_DOWN
 */
int wrapperGetProcessStatus() {
    int res;
    DWORD exitCode;

    switch (WaitForSingleObject(wrapperProcess, 0)) {
    case WAIT_ABANDONED:
    case WAIT_OBJECT_0:
        res = WRAPPER_PROCESS_DOWN;

        /* Get the exit code of the process. */
        if (!GetExitCodeProcess(wrapperProcess, &exitCode)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                "Critical error: unable to obtain the exit code of the JVM process: %s", getLastErrorText());
            appExit(1);
        }

        wrapperJVMProcessExited(exitCode);

        /* Remove java pid file if it was registered and created by this process. */
        if ((ownJavaPidFile) && (wrapperData->javaPidFilename)) {
            unlink(wrapperData->javaPidFilename);
            ownJavaPidFile = 0;
        }

        break;

    case WAIT_TIMEOUT:
        res = WRAPPER_PROCESS_UP;
        break;

    default:
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Critical error: wait for JVM process failed");
        appExit(1);
    }

    return res;
}

/**
 * Kill the JVM Process immediately and set the JVM State to
 *  WRAPPER_JSTATE_DOWN
 */
void wrapperKillProcess() {
    int ret;
    DWORD startTicks;
    DWORD nowTicks;

    /* Check to make sure that the JVM process is still running */
    ret = WaitForSingleObject(wrapperProcess, 0);
    if (ret == WAIT_TIMEOUT) {
        /* JVM is still up when it should have already stopped itself. */
        if (wrapperData->requestThreadDumpOnFailedJVMExit) {
            requestDumpJVMState();

            /* Loop for 5 seconds reading all available input before actually killing
             *  the JVM process.  This is to make sure that the JVM is given enough
             *  time to perform the full thread dump. */
            nowTicks = startTicks = wrapperGetTicks();
            do {
                if ( !wrapperReadChildOutput() )
                {
                    /* Sleep a moment so this loop does not eat too much CPU. */
                    Sleep(250); /* milliseconds */
                }
                nowTicks = wrapperGetTicks();
            } while ( wrapperGetTickAge( startTicks, nowTicks ) < 5 );
        }

        /* The JVM process is not responding so the only choice we have is to
         *  kill it.  The TerminateProcess funtion will kill the process, but it
         *  does not correctly notify the process's DLLs that it is shutting
         *  down.  Ideally, we would call ExitProcess, but that can only be
         *  called from within the process being killed. */
        if (TerminateProcess(wrapperProcess, 0)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM did not exit on request, terminated");
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM did not exit on request.");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "  Attempt to terminate process failed.  Error=%d", GetLastError());
        }

        /* Give the JVM a chance to be killed so that the state will be correct. */
        Sleep(500); /* 0.5 seconds in milliseconds */

        /* Set the exit code since we were forced to kill the JVM. */
        wrapperData->exitCode = 1;
    }

    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->jStateTimeoutTicks = 0;
    wrapperData->jStateTimeoutTicksSet = 0;
    wrapperProcess = NULL;

    /* Remove java pid file if it was registered and created by this process. */
    if ((ownJavaPidFile) && (wrapperData->javaPidFilename)) {
        unlink(wrapperData->javaPidFilename);
        ownJavaPidFile = 0;
    }

    /* Close any open socket to the JVM */
    wrapperProtocolClose();
}

/**
 * Launches a JVM process and store it internally
 */
void wrapperExecute() {
    SECURITY_ATTRIBUTES process_attributes;
    STARTUPINFO startup_info; 
    PROCESS_INFORMATION process_info;
    int ret;
    /* Do not show another console for the new process */
    /*int processflags=CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS; */

    /* Create a new process group as part of this console so that signals can */
    /*  be sent to the JVM. */
    DWORD processflags=CREATE_NEW_PROCESS_GROUP;

    /* Do not show another console for the new process, but show its output in the current console. */
    /*int processflags=CREATE_NEW_PROCESS_GROUP; */

    /* Show a console for the new process */
    /*int processflags=CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE; */

    char *commandline=NULL;
    char *environment=NULL;
    char *binparam=NULL;
    int char_block_size = 8196;
    int string_size = 0;
    int temp_int = 0;
    char szPath[512];
    char *c;
    char titleBuffer[80];
    int hideConsole;

    FILE *pid_fp = NULL;
    int old_umask;

    /* Reset the exit code when we launch a new JVM. */
    wrapperData->exitCode = 0;

    /* Increment the process ID for Log sourcing */
    wrapperData->jvmRestarts++;

    /* Add the priority class of the new process to the processflags */
    processflags = processflags | wrapperData->ntServicePriorityClass;

    /* Setup the command line */
    commandline = wrapperData->jvmCommand;
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "command: %s", commandline);
    }
                           
    /* Setup environment. Use parent's for now */
    environment = NULL;

    /* Initialize a SECURITY_ATTRIBUTES for the process attributes of the new process. */
    process_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    process_attributes.lpSecurityDescriptor = NULL;
    process_attributes.bInheritHandle = TRUE;

    /* Generate a unique time for the console so we can look for it below. */
    sprintf(titleBuffer, "Wrapper Controlled JVM Console ID %d%d (Do not close)", rand(), rand());

    /* Initialize a STARTUPINFO structure to use for the new process. */
    startup_info.cb=sizeof(STARTUPINFO);
    startup_info.lpReserved=NULL;
    startup_info.lpDesktop=NULL;
    startup_info.lpTitle=titleBuffer;
    startup_info.dwX=0;
    startup_info.dwY=0;
    startup_info.dwXSize=0;
    startup_info.dwYSize=0;
    startup_info.dwXCountChars=0;
    startup_info.dwYCountChars=0;
    startup_info.dwFillAttribute=0;
    
    /* Set the default flags which will not hide any windows opened by the JVM. */
    startup_info.dwFlags=STARTF_USESTDHANDLES;
    startup_info.wShowWindow=0;
    hideConsole = FALSE;
    if (wrapperData->isConsole) {
        /* We are running as a console so no special console handling needs to be done. */
    } else {
        /* Running as a service. */
        if (wrapperData->ntAllocConsole) {
            /* A console was allocated when the service was started so the JVM will not create
             *  its own. */
            if (wrapperData->wrapperConsoleHandle) {
                /* The console exists but is currently hidden. */
                if (!wrapperData->ntHideJVMConsole) {
                    /* In order to support older JVMs we need to show the console when the
                     *  JVM is launched.  We need to remember to hide it below. */
                    showConsoleWindow(wrapperData->wrapperConsoleHandle);
                    hideConsole = TRUE;
                }
            }
        } else {
            /* A console does not yet exist so the JVM will create and display one itself. */
            if (wrapperData->ntHideJVMConsole) {
                /* The console that the JVM creates should be surpressed and never shown.
                 *  JVMs of version 1.4.0 and above will still display a GUI.  But older JVMs
                 *  will not. */
                startup_info.dwFlags=STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
                startup_info.wShowWindow=SW_HIDE;
            } else {
                /* The new JVM console should be allowed to be displayed.  But we need to
                 *  remember to hide it below. */
                hideConsole = TRUE;
            }
        }
    }
    
    startup_info.cbReserved2=0;
    startup_info.lpReserved2=NULL;
    startup_info.hStdInput=NULL;
    startup_info.hStdOutput=wrapperChildStdoutWr;
    startup_info.hStdError=wrapperChildStdoutWr;

    /* Initialize a PROCESS_INFORMATION structure to use for the new process */ 
    process_info.hProcess=NULL;
    process_info.hThread=NULL;
    process_info.dwProcessId=0;
    process_info.dwThreadId=0;

    /* Need the directory that this program exists in.  Not the current directory. */
    /*	Note, the current directory when run as an NT service is the windows system directory. */
    /* Get the full path and filename of this program */
    if (GetModuleFileName(NULL, szPath, 512) == 0){
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to launch %s -%s",
                     wrapperData->ntServiceDisplayName, getLastErrorText());
        wrapperProcess = NULL;
        return;
    }
    c = strrchr(szPath, '\\');
    if (c == NULL) {
        szPath[0] = '\0';
    } else {
        c[1] = '\0'; /* terminate after the slash */
    }

    /* Create the new process */
    ret=CreateProcess(NULL, 
                      commandline,    /* the command line to start */
                      NULL,           /* process security attributes */
                      NULL,           /* primary thread security attributes */
                      TRUE,           /* handles are inherited */
                      processflags,   /* we specify new process group */
                      environment,    /* use parent's environment */
                      NULL,           /* use the Wrapper's current working directory */
                      &startup_info,  /* STARTUPINFO pointer */
                      &process_info); /* PROCESS_INFORMATION pointer */

    /* Check if virtual machine started */
    if (ret==FALSE) {
        int err=GetLastError();
        /* This was placed to handle the Swedish WinNT bug */
        if (err!=NO_ERROR) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to execute Java command.  %s", getLastErrorText());
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "    %s", commandline);
            wrapperProcess = NULL;
            return;
        }
    }

    /* Now check if we have a process handle again for the Swedish WinNT bug */
    if (process_info.hProcess==NULL) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "can not execute \"%s\"", commandline);
        wrapperProcess = NULL;
        return;
    }

    if (hideConsole) {
        /* Now that the JVM has been launched we need to hide the console that it
         *  is using. */
        if (wrapperData->wrapperConsoleHandle) {
            /* The wrapper's console needs to be hidden. */
            hideConsoleWindow(wrapperData->wrapperConsoleHandle);
        } else {
            /* We need to locate the console that was created by the JVM on launch
             *  and hide it. */
         findAndHideConsoleWindow(titleBuffer);
        }
    }

    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM started (PID=%d)", process_info.dwProcessId);
    }

    wrapperProcess = process_info.hProcess;
    wrapperProcessId = process_info.dwProcessId;

    /* If a java pid filename is specified then write the pid of the java process. */
    if (wrapperData->javaPidFilename) {
        old_umask = _umask(022);
        pid_fp = fopen(wrapperData->javaPidFilename, "w");
        _umask(old_umask);

        if (pid_fp != NULL) {
            fprintf(pid_fp, "%d\n", wrapperProcessId);
            fclose(pid_fp);

            /* Remember that we created the pid file. */
            ownJavaPidFile = 1;
        } else {
         log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "Unable to write the Java PID file: %s", wrapperData->javaPidFilename);
        }
    }
}

/**
 * Returns a tick count that can be used in combination with the
 *  wrapperGetTickAge() function to perform time keeping.
 */
DWORD wrapperGetTicks() {
    if (wrapperData->useSystemTime) {
        /* We want to return a tick count that is based on the current system time. */
        return wrapperGetSystemTicks();

    } else {
        /* Return a snapshot of the current tick count. */
        return timerTicks;
    }
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
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "ServiceControlHandler(%d)", dwCtrlCode);
    }

    /* This thread appears to always be the same as the main thread.
     *  Just to be safe reregister it. */
    logRegisterThread(WRAPPER_THREAD_MAIN);

    switch(dwCtrlCode) {
    case SERVICE_CONTROL_STOP:
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "  SERVICE_CONTROL_STOP");
        }

        /* Request to stop the service. Report SERVICE_STOP_PENDING */
        /* to the service control manager before calling ServiceStop() */
        /* to avoid a "Service did not respond" error. */
        wrapperReportStatus(WRAPPER_WSTATE_STOPPING, 0, 0);

        /* Tell the wrapper to shutdown normally */
        wrapperStopProcess(0);

        /* To make sure that the JVM will not be restarted for any reason,
         *  start the Wrapper shutdown process as well. */
        if ((wrapperData->wState == WRAPPER_WSTATE_STOPPING) ||
            (wrapperData->wState == WRAPPER_WSTATE_STOPPED)) {
            /* Already stopping. */
        } else {
            wrapperData->wState = WRAPPER_WSTATE_STOPPING;
        }

        return;
        
    case SERVICE_CONTROL_INTERROGATE:
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "  SERVICE_CONTROL_INTERROGATE");
        }

        /* This case MUST be processed, even though we are not */
        /* obligated to do anything substantial in the process. */
        break;

    case SERVICE_CONTROL_SHUTDOWN:
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "  SERVICE_CONTROL_SHUTDOWN");
        }

        wrapperReportStatus(WRAPPER_WSTATE_STOPPING, 0, 0);

        /* Tell the wrapper to shutdown normally */
        wrapperStopProcess(0);

        /* To make sure that the JVM will not be restarted for any reason,
         *  start the Wrapper shutdown process as well. */
        if ((wrapperData->wState == WRAPPER_WSTATE_STOPPING) ||
            (wrapperData->wState == WRAPPER_WSTATE_STOPPED)) {
            /* Already stopping. */
        } else {
            wrapperData->wState = WRAPPER_WSTATE_STOPPING;
        }

        break;

    default:
        /* Any other cases... */
        break;
    }

    /* After invocation of this function, we MUST call the SetServiceStatus */
    /* function, which is accomplished through our ReportStatus function. We */
    /* must do this even if the current status has not changed. */
    wrapperReportStatus(wrapperData->wState, 0, 0);
}

/**
 * The wrapperServiceMain function is the entry point for the NT service.
 *	It is called by the service manager.
 */
void WINAPI wrapperServiceMain(DWORD dwArgc, LPTSTR *lpszArgv) {
#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "wrapperServiceMain()");
#endif

    /* Immediately register this thread with the logger. */
    logRegisterThread(WRAPPER_THREAD_SRVMAIN);

    /* Call RegisterServiceCtrlHandler immediately to register a service control */
    /* handler function. The returned SERVICE_STATUS_HANDLE is saved with global */
    /* scope, and used as a service id in calls to SetServiceStatus. */
    if (!(sshStatusHandle = RegisterServiceCtrlHandler(wrapperData->ntServiceName, wrapperServiceControlHandler))) {
        goto finally;
    }

    /* The global ssStatus SERVICE_STATUS structure contains information about the */
    /* service, and is used throughout the program in calls made to SetStatus through */
    /* the ReportStatus function. */
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
    ssStatus.dwServiceSpecificExitCode = 0;


    /* If we could guarantee that all initialization would occur in less than one */
    /* second, we would not have to report our status to the service control manager. */
    /* For good measure, we will assign SERVICE_START_PENDING to the current service */
    /* state and inform the service control manager through our ReportStatus function. */
    wrapperReportStatus(WRAPPER_WSTATE_STARTING, 0, 3000);

    /* Now actually start the service */
    wrapperRunService();

 finally:

    /* The the exit code is non-zero then we want the Service Manager to detect that we
     *  are exiting in an error state.  If we tell it that it STOPPED then it appears to
     *  ignore the exit code that we have set.   We need to simply exit the process.
     * If the exit code is 0, however, then it is important that we actually report that
     *  we have stopped with an exit code of 0. */
    if (wrapperData->exitCode == 0) {
        /* Report to the service manager that the application is about to exit. */
        if (sshStatusHandle) {
            /* This will continue on an be exited normally below. */
            wrapperReportStatus(WRAPPER_WSTATE_STOPPED, wrapperData->exitCode, 1000);
        }
    }

#ifdef _DEBUG
    /* The following message will not always appear on the screen if the STOPPED
     *  status was set above.  But the code in the appExit function below always
     *  appears to be getting executed.  Looks like some kind of a timing issue. */
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Exiting service process.");
#endif

    /* Actually exit the process, returning the current exit code. */
    appExit(wrapperData->exitCode);
}

/**
 * Install the Wrapper as an NT Service using the information and service
 *  name in the current configuration file.
 *
 * Stores the parameters with the service name so that the wrapper.conf file
 *  can be located at runtime.
 */
int wrapperInstall(int argc, char **argv) {
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    DWORD       serviceType;

    char szPath[512];
    char binaryPath[4096];
    int i;
    int result = 0;
    HKEY hKey;
    char regPath[ 1024 ];

    /* Get the full path and filename of this program */
    if (GetModuleFileName(NULL, szPath, 512) == 0){
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to install %s -%s",
                     wrapperData->ntServiceDisplayName, getLastErrorText());
        return 1;
    }
    
    /* Build a new command line with all of the parameters. */
    binaryPath[0] = '\0';
    for (i = 0; i < argc; i++) {
        if (i > 0) {
            strcat(binaryPath, " ");
        }
        switch (i) {
        case 0:
            /* argv[0] is the binary name */
            /* If the szPath contains spaces, it needs to be quoted */
            if (strchr(szPath, ' ') == NULL) {
                strcat(binaryPath, szPath);
            } else {
                strcat(binaryPath, "\"");
                strcat(binaryPath, szPath);
                strcat(binaryPath, "\"");
            }
            break;
        case 1:
            /* argv[1] is '-i' option -> change to '-s' */
            strcat(binaryPath, "-s");
            break;
        default:
            /* All other argv[n] should be preserved as is. */
            /* If the argument contains spaces, it needs to be quoted */
            if (strchr(argv[i], ' ') == NULL) {
                strcat(binaryPath, argv[i]);
            } else {
                strcat(binaryPath, "\"");
                strcat(binaryPath, argv[i]);
                strcat(binaryPath, "\"");
            }
            break;
        }
    }
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Service command: %s", binaryPath);
    }

    /* Decide on the service type */
    if ( wrapperData->ntServiceInteractive ) {
        serviceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
    } else {
        serviceType = SERVICE_WIN32_OWN_PROCESS;
    }

    /* Next, get a handle to the service control manager */
    schSCManager = OpenSCManager(
                                 NULL,                   
                                 NULL,                   
                                 SC_MANAGER_ALL_ACCESS   
                                 );
    
    if (schSCManager) {
        schService = CreateService(
                                   schSCManager,                       /* SCManager database */
                                   wrapperData->ntServiceName,         /* name of service */
                                   wrapperData->ntServiceDisplayName,  /* name to display */
                                   SERVICE_ALL_ACCESS,                 /* desired access */
                                   serviceType,                        /* service type */
                                   wrapperData->ntServiceStartType,    /* start type */
                                   SERVICE_ERROR_NORMAL,               /* error control type */
                                   binaryPath,                         /* service's binary */
                                   wrapperData->ntServiceLoadOrderGroup, /* load ordering group */
                                   NULL,                               /* tag identifier not used because they are used for driver level services. */
                                   wrapperData->ntServiceDependencies, /* dependencies */
                                   wrapperData->ntServiceAccount,      /* LocalSystem account if NULL */
                                   wrapperData->ntServicePassword );   /* NULL for no password */
        
        if (schService) {
            /* Have the service, add a description to the registry. */
            sprintf(regPath, "SYSTEM\\CurrentControlSet\\Services\\%s", wrapperData->ntServiceName);
            if ((wrapperData->ntServiceDescription != NULL && strlen(wrapperData->ntServiceDescription) > 0)
                && (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, KEY_WRITE, (PHKEY) &hKey) == ERROR_SUCCESS)) {
                
                /* Set Description key in registry */
                RegSetValueEx(hKey, "Description", (DWORD) 0, (DWORD) REG_SZ,
                    (const unsigned char *)wrapperData->ntServiceDescription,
                    (strlen(wrapperData->ntServiceDescription) + 1));
                RegCloseKey(hKey);
            }
            
            /* Service was installed. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "%s installed.",
                wrapperData->ntServiceDisplayName);

            /* Close the handle to this service object */
            CloseServiceHandle(schService);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "CreateService failed - %s",
                getLastErrorText());
            result = 1;
        }

        /* Close the handle to the service control manager database */
        CloseServiceHandle(schSCManager);
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "OpenSCManager failed - %s", getLastErrorText());
        result = 1;
    }

    return result;
}

/**
 * Sets any environment variables stored in the system registry to the current
 *  environment.  The NT service environment only has access to the environment
 *  variables set when the machine was last rebooted.  This makes it possible
 *  to access the latest values in registry without a reboot.
 *
 * Note that this function is always called before the configuration file has
 *  been loaded this means that any logging that takes place will be sent to
 *  the default log file which may be difficult for the user to locate.
 */
static char **envKeys = NULL;
static int envKeysCount = 0;
int wrapperLoadEnvFromRegistry() {
    int envCount = 0;
    int result = 0;
    int ret;
    HKEY hKey;
    char regPath[1024];
    DWORD dwIndex;
    LONG err;
    CHAR name[MAX_PROPERTY_NAME_LENGTH];
    DWORD cbName;
    DWORD type;
    byte data[MAX_PROPERTY_VALUE_LENGTH];
    DWORD cbData;
    char *envVal;
    BOOL expanded;

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Loading environment variables from Registry:");
#endif

    /* Open the registry entry where the current environment variables are stored. */
    sprintf(regPath, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment\\");
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, (PHKEY) &hKey) == ERROR_SUCCESS) {
        /* Read in each of the environment variables and set them into the environment.
         *  These values will be set as is without doing any environment variable
         *  expansion.  In order for the ExpandEnvironmentStrings function to work all
         *  of the environment variables to be replaced must already be set.  To handle
         *  this, after we set the values as is from the registry, we need to go back
         *  through all the ones we set and Expand them if necessary. */
        dwIndex = 0;

        /* First time through, count all of the keys. */
        do {
            cbName = MAX_PROPERTY_NAME_LENGTH;
            cbData = MAX_PROPERTY_VALUE_LENGTH;
            err = RegEnumValue(hKey, dwIndex, name, &cbName, NULL, &type, data, &cbData); 
            dwIndex++;
        } while (err != ERROR_NO_MORE_ITEMS);

        if (dwIndex > 0) {
            /* Now allocate a buffer for the environment strings. */
            if (envKeys != NULL) {
                /* An old set of keys are already allocated.  Free them up. */
                int x = 0;
                for (x = 0; x < envKeysCount; x++) {
                    if (NULL != envKeys[x]) {
                        free(envKeys[x]);
                        envKeys[x] = NULL;
                    }
                }
                free(envKeys);
                envKeys = NULL;
            }
            envKeys = calloc(dwIndex, sizeof(char *));
            envKeysCount = dwIndex;
        }
        if (envKeys != NULL) {
            dwIndex = 0;
            do {
                cbName = MAX_PROPERTY_NAME_LENGTH;
                cbData = MAX_PROPERTY_VALUE_LENGTH;
                err = RegEnumValue(hKey, dwIndex, name, &cbName, NULL, &type, data, &cbData);
                if (err != ERROR_NO_MORE_ITEMS) {
#ifdef _DEBUG
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  Loaded var name=\"%s\", value=\"%s\"", name, data);
#endif
                    /* Found an environment variable in the registry.  Set it to the current environment. */
                    envKeys[dwIndex] = malloc(MAX_PROPERTY_NAME_LENGTH - 1 + MAX_PROPERTY_VALUE_LENGTH - 1 + 2);
                    if (envKeys[dwIndex] != NULL) {
                        sprintf(envKeys[dwIndex], "%s=%s", name, data);
                        if (putenv(envKeys[dwIndex])) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to set environment variable from the registry - %s", getLastErrorText());
                            err = ERROR_NO_MORE_ITEMS;
                            result = 1;
                        }
#ifdef _DEBUG
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  Set to local environment.");
#endif
                    }
                } else if (err == ERROR_NO_MORE_ITEMS) {
                    /* No more environment variables. */
                } else {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to read registry - %s", getLastErrorText());
                    err = ERROR_NO_MORE_ITEMS;
                    result = 1;
                }
                dwIndex++;
            } while (err != ERROR_NO_MORE_ITEMS);
        }

        if (!result) {
#ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "All environment variables loaded.  Loop back over them to evaluate any nested variables.");
#endif
            /* Go back and loop over the environment variables we just set and expand any
             *  variables which contain % characters. Loop until we make a pass which does
             *  not perform any replacements. */
            do {
                expanded = FALSE;

                dwIndex = 0;
                do {
                    cbName = MAX_PROPERTY_NAME_LENGTH;
                    cbData = MAX_PROPERTY_VALUE_LENGTH;
                    err = RegEnumValue(hKey, dwIndex, name, &cbName, NULL, NULL, NULL, NULL);
                    if (err != ERROR_NO_MORE_ITEMS) {
                        /* Found an environment variable in the registry.  Get the current value. */
#ifdef _DEBUG
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  Get the current local value of variable \"%s\"", name);
#endif
                        envVal = getenv(name);
                        if (envVal == NULL) {
#ifdef _DEBUG
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  The current local value of variable \"%s\" is null, meaning it was \"\" in the registry.  Skipping.", name);
#endif
                        } else {
#ifdef _DEBUG
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "     \"%s\"=\"%s\"", name, envVal);
#endif
                            if (strchr(envVal, '%')) {
                                /* This variable contains tokens which need to be expanded. */
                                ret = ExpandEnvironmentStrings(envVal, data, MAX_PROPERTY_VALUE_LENGTH);
                                if (ret == 0) {
                                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to expand environment variable, %s - %s", name, getLastErrorText());
                                    err = ERROR_NO_MORE_ITEMS;
                                    result = 1;
                                } else if (ret >= MAX_PROPERTY_VALUE_LENGTH) {
                                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, "Unable to expand environment variable, %s as it would be longer than the %d byte buffer size.", name, MAX_PROPERTY_VALUE_LENGTH);
                                } else if (strcmp(envVal, data) == 0) {
#ifdef _DEBUG
                                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "       Value unchanged.  Referenced environment variable not set.");
#endif
                                } else {
                                    /* Set the expanded environment variable */
                                    expanded = TRUE;
#ifdef _DEBUG
                                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  Update local environment variable.  \"%s\"=\"%s\"", name, data);
#endif
                                    /* Replace the env string. */
                                    sprintf(envKeys[ dwIndex ], "%s=%s", name, data);
                                    if (putenv(envKeys[ dwIndex ])) {
                                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to set environment variable from the registry - %s", getLastErrorText());
                                        err = ERROR_NO_MORE_ITEMS;
                                        result = 1;
                                    }
                                }
                            }
                        }
                    } else if (err == ERROR_NO_MORE_ITEMS) {
                        /* No more environment variables. */
                    } else {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to read registry - %s", getLastErrorText());
                        err = ERROR_NO_MORE_ITEMS;
                        result = 1;
                    }
                    dwIndex++;
                } while (err != ERROR_NO_MORE_ITEMS);

#ifdef _DEBUG
                if (expanded && (result == 0)) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Rescan environment variables to varify that there are no more expansions necessary.");
                }
#endif
            } while (expanded && (result == 0));

#ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  Done loading environment variables.");
#endif
        }

        /* Close the registry entry */
        RegCloseKey(hKey);
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to access registry to obtain environment variables - %s", getLastErrorText());
        result = 1;
    }

    return result;
}

char *getNTServiceStatusName(int status)
{
    char *name;
    switch(status) {
    case SERVICE_STOPPED:
        name = "STOPPED";
        break;
    case SERVICE_START_PENDING:
        name = "START_PENDING";
        break;
    case SERVICE_STOP_PENDING:
        name = "STOP_PENDING";
        break;
    case SERVICE_RUNNING:
        name = "RUNNING";
        break;
    case SERVICE_CONTINUE_PENDING:
        name = "CONTINUE_PENDING";
        break;
    case SERVICE_PAUSE_PENDING:
        name = "PAUSE_PENDING";
        break;
    case SERVICE_PAUSED:
        name = "PAUSED";
        break;
    default:
        name = "UNKNOWN";
        break;
    }
    return name;
}

/** Starts a Wrapper instance running as an NT Service. */
int wrapperStartService() {
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    SERVICE_STATUS serviceStatus;

    char *status;
    int msgCntr;
    int stopping;
    int result = 0;

    /* First, get a handle to the service control manager */
    schSCManager = OpenSCManager(
                                 NULL,                   
                                 NULL,                   
                                 SC_MANAGER_ALL_ACCESS   
                                 );
    if (schSCManager){
        /* Next get the handle to this service... */
        schService = OpenService(schSCManager, wrapperData->ntServiceName, SERVICE_ALL_ACCESS);

        if (schService){
            /* Make sure that the service is not already running. */
            if (QueryServiceStatus(schService, &serviceStatus)) {
                if (serviceStatus.dwCurrentState == SERVICE_STOPPED) {
                    /* The service is stopped, so try starting it. */
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Starting the %s service...",
                        wrapperData->ntServiceDisplayName);
                    if (StartService( schService, 0, NULL )) {
                        /* We will get here immediately if the service process was launched.
                         *  We still need to wait for it to actually start. */
                      msgCntr = 0;
                        stopping = FALSE;
                        do {
                            if ( QueryServiceStatus(schService, &serviceStatus)) {
                                if (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING) {
                                    if (!stopping) {
                                        stopping = TRUE;
                                        msgCntr = 5; /* Trigger a message */
                                    }
                                    if (msgCntr >= 5) {
                                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Stopping...");
                                        msgCntr = 0;
                                    }
                                } else {
                                    if (msgCntr >= 5) {
                                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Waiting to start...");
                                        msgCntr = 0;
                                    }
                                }
                                Sleep( 1000 );
                              msgCntr++;
                            } else {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                                    "Unable to query the status of the %s service - %s",
                                    wrapperData->ntServiceDisplayName, getLastErrorText());
                                result = 1;
                                break;
                            }
                        } while ((serviceStatus.dwCurrentState != SERVICE_STOPPED)
                            && (serviceStatus.dwCurrentState != SERVICE_RUNNING));

                        /* Was the service started? */
                        if (serviceStatus.dwCurrentState == SERVICE_RUNNING) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "%s started.", wrapperData->ntServiceDisplayName);
                        } else {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "The %s service was launched, but failed to start.",
                                wrapperData->ntServiceDisplayName);
                            result = 1;
                        }
                    } else {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unable to start the service - %s",
                            getLastErrorText());
                        result = 1;
                    }
                } else {
                    status = getNTServiceStatusName(serviceStatus.dwCurrentState);
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "The %s service is already running with status: %s",
                        wrapperData->ntServiceDisplayName, status);
                    result = 1;
                }
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to query the status of the %s service - %s",
                    wrapperData->ntServiceDisplayName, getLastErrorText());
                result = 1;
            }
            
            /* Close this service object's handle to the service control manager */
            CloseServiceHandle(schService);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "OpenService failed - %s", getLastErrorText());
            result = 1;
        }
        
        /* Finally, close the handle to the service control manager's database */
        CloseServiceHandle(schSCManager);
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "OpenSCManager failed - %s", getLastErrorText());
        result = 1;
    }

    return result;
}

/** Stops a Wrapper instance running as an NT Service. */
int wrapperStopService(int command) {
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    SERVICE_STATUS serviceStatus;

    char *status;
    int msgCntr;
    int result = 0;

    /* First, get a handle to the service control manager */
    schSCManager = OpenSCManager(
                                 NULL,                   
                                 NULL,                   
                                 SC_MANAGER_ALL_ACCESS   
                                 );
    if (schSCManager){

        /* Next get the handle to this service... */
        schService = OpenService(schSCManager, wrapperData->ntServiceName, SERVICE_ALL_ACCESS);

        if (schService){
            /* Find out what the current status of the service is so we can decide what to do. */
            if (QueryServiceStatus(schService, &serviceStatus)) {
                if (serviceStatus.dwCurrentState == SERVICE_STOPPED) {
                    if (command) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "The %s service was not running.",
                            wrapperData->ntServiceDisplayName);
                    }
                } else {
                    if (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                            "The %s service was already in the process of stopping.",
                            wrapperData->ntServiceDisplayName);
                    } else {
                        /* Stop the service. */
                        if (ControlService( schService, SERVICE_CONTROL_STOP, &serviceStatus)){
                            if ( command ) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Stopping the %s service...",
                                    wrapperData->ntServiceDisplayName);
                            } else {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Service is running.  Stopping it...");
                            }
                        } else {
                            if (serviceStatus.dwCurrentState == SERVICE_START_PENDING) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                    "The %s service was in the process of starting.  Stopping it...",
                                    wrapperData->ntServiceDisplayName);
                            } else {
                                status = getNTServiceStatusName(serviceStatus.dwCurrentState);
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Attempt to stop the %s service failed.  Status: %s",
                                    wrapperData->ntServiceDisplayName, status);
                                result = 1;
                            }
                        }
                    }
                    if (result == 0) {
                        /** Wait for the service to stop. */
                      msgCntr = 0;
                        do {
                            if ( QueryServiceStatus(schService, &serviceStatus)) {
                                if (msgCntr >= 5) {
                                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Waiting to stop...");
                                    msgCntr = 0;
                                }
                                Sleep( 1000 );
                              msgCntr++;
                            } else {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                                    "Unable to query the status of the %s service - %s",
                                    wrapperData->ntServiceDisplayName, getLastErrorText());
                                result = 1;
                                break;
                            }
                        } while (serviceStatus.dwCurrentState != SERVICE_STOPPED);

                        if ( serviceStatus.dwCurrentState == SERVICE_STOPPED ) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "%s stopped.", wrapperData->ntServiceDisplayName);
                        } else {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "%s failed to stop.", wrapperData->ntServiceDisplayName);
                            result = 1;
                        }
                    }
                }
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to query the status of the %s service - %s",
                    wrapperData->ntServiceDisplayName, getLastErrorText());
                result = 1;
            }
            
            /* Close this service object's handle to the service control manager */
            CloseServiceHandle(schService);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "OpenService failed - %s", getLastErrorText());
            result = 1;
        }
        
        /* Finally, close the handle to the service control manager's database */
        CloseServiceHandle(schSCManager);
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "OpenSCManager failed - %s", getLastErrorText());
        result = 1;
    }

    return result;
}

/**
 * Uninstall the service and clean up
 */
int wrapperRemove() {
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    int result = 0;

    /* First attempt to stop the service if it is already running. */
    result = wrapperStopService( FALSE );
    if ( result )
    {
        /* There was a problem stopping the service. */
        return result;
    }

    /* First, get a handle to the service control manager */
    schSCManager = OpenSCManager(
                                 NULL,                   
                                 NULL,                   
                                 SC_MANAGER_ALL_ACCESS   
                                 );
    if (schSCManager){

        /* Next get the handle to this service... */
        schService = OpenService(schSCManager, wrapperData->ntServiceName, SERVICE_ALL_ACCESS);

        if (schService){
            /* Now try to remove the service... */
            if (DeleteService(schService)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "%s removed.", wrapperData->ntServiceDisplayName);
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "DeleteService failed - %s", getLastErrorText());
                result = 1;
            }
            
            /* Close this service object's handle to the service control manager */
            CloseServiceHandle(schService);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "OpenService failed - %s", getLastErrorText());
            result = 1;
        }
        
        /* Finally, close the handle to the service control manager's database */
        CloseServiceHandle(schSCManager);
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "OpenSCManager failed - %s", getLastErrorText());
        result = 1;
    }

    /* Remove message file registration on service remove */
    if( result == 0 ) {
        /* Do this here to unregister the syslog on uninstall of a resource. */
        /* unregisterSyslogMessageFile( ); */
    }

    return result;
}

/**
 * Sets the working directory to that of the current executable
 */
int setWorkingDir() {
    char szPath[512];
    char* pos;
    
    /* Get the full path and filename of this program */
    if (GetModuleFileName(NULL, szPath, 512) == 0){
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to get the path-%s", getLastErrorText());
        return 1;
    }

    /* The wrapperData->isDebugging flag will never be set here, so we can't really use it. */
#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Executable Name: %s", szPath);
#endif

    /* To get the path, strip everything off after the last '\' */
    pos = strrchr(szPath, '\\');
    if (pos == NULL) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to extract path from: %s", szPath);
        return 1;
    } else {
        /* Clip the path at the position of the last backslash */
        pos[0] = (char)0;
    }

    return wrapperSetWorkingDir(szPath);
}

int writePidFile() {
    FILE *pid_fp = NULL;
    int old_umask;

    old_umask = _umask(022);
    pid_fp = fopen(wrapperData->pidFilename, "w");
    _umask(old_umask);
    
    if (pid_fp != NULL) {
        fprintf(pid_fp, "%d\n", (int)getpid());
        fclose(pid_fp);

        /* Remember that we created the pid file. */
        ownPidFile = 1;
    } else {
        return 1;
    }
    return 0;
}

/******************************************************************************
 * Main function
 *****************************************************************************/

int exceptionFilterFunction(PEXCEPTION_POINTERS exceptionPointers) {
    DWORD exCode;
    char *exName;
    int i;

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "encountered a fatal error in Wrapper");
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
        exName = malloc(sizeof(char) * 64);  /* Let this leak.  It only happens once before shutdown. */
        sprintf(exName, "Unknown Exception (%ld)", exCode);
        break;
    }

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "  exceptionCode    = %s", exName);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "  exceptionFlag    = %s", 
        (exceptionPointers->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE ? "EXCEPTION_NONCONTINUABLE" : "EXCEPTION_NONCONTINUABLE_EXCEPTION"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "  exceptionAddress = %p", (int)exceptionPointers->ExceptionRecord->ExceptionAddress);
    if (exCode == EXCEPTION_ACCESS_VIOLATION) {
        if (exceptionPointers->ExceptionRecord->ExceptionInformation[0] == 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "  Read access exception from %p", 
                (int)exceptionPointers->ExceptionRecord->ExceptionInformation[1]);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "  Write access exception to %p", 
                (int)exceptionPointers->ExceptionRecord->ExceptionInformation[1]);
        }
    } else {
        for (i = 0; i < (int)exceptionPointers->ExceptionRecord->NumberParameters; i++) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "  exceptionInformation[%d] = %ld", i,
                exceptionPointers->ExceptionRecord->ExceptionInformation[i]);
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

/**
 * Show usage
 */
void wrapperUsage(char *appName) {
    printf("Usage:\n");
    printf("  %s <command> <configuration file> [configuration properties] [...]\n", appName);
    printf("\n");
    printf("where <command> can be one of:\n");
    printf("  -c   run as a Console application\n");
    printf("  -t   starT an NT service\n");
    printf("  -p   stoP a running NT service\n");
    printf("  -i   Install as an NT service\n");
    printf("  -r   Remove as an NT service\n");
    /* Omit '-s' option from help as it is only used by the service manager. */
    /*printf("  -s   used by service manager\n"); */
    printf("  -?   print this help message\n");
    printf("\n");
    printf("<configuration file> is the wrapper.conf to use\n");
    printf("\n");
    printf("[configuration properties] are configuration name-value pairs which override values\n");
    printf("  in wrapper.conf.  For example:\n");
    printf("  wrapper.debug=true\n");
    printf("\n");
}
void _CRTAPI1 main(int argc, char **argv) {
    int result = 0;
    int i;

    /* The StartServiceCtrlDispatcher requires this table to specify
     * the ServiceMain function to run in the calling process. The first
     * member in this example is actually ignored, since we will install
     * our service as a SERVICE_WIN32_OWN_PROCESS service type. The NULL
     * members of the last entry are necessary to indicate the end of 
     * the table; */
    SERVICE_TABLE_ENTRY serviceTable[2];

    buildSystemPath();

    /* Initialize the WrapperConfig structure */
    wrapperData = malloc(sizeof(WrapperConfig));
    wrapperData->configured = FALSE;
    wrapperData->isConsole = TRUE;
    wrapperData->wState = WRAPPER_WSTATE_STARTING;
    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->jStateTimeoutTicks = 0;
    wrapperData->jStateTimeoutTicksSet = 0;
    wrapperData->lastPingTicks = wrapperGetTicks();
    wrapperData->jvmCommand = NULL;
    wrapperData->exitRequested = FALSE;
    wrapperData->restartRequested = TRUE; /* The first JVM needs to be started. */
    wrapperData->exitCode = 0;
    wrapperData->jvmRestarts = 0;
    wrapperData->jvmLaunchTicks = wrapperGetTicks();
    wrapperData->failedInvocationCount = 0;

    wrapperInitializeLogging();

    /* Immediately register this thread with the logger. */
    logRegisterThread(WRAPPER_THREAD_MAIN);

    /* Enclose the rest of the program in a try catch block so we can
     *  display and log useful information should the need arise.  This
     *  must be done after logging has been initialized as the catch
     *  block makes use of the logger. */
    __try {
#ifdef _DEBUG
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Wrapper DEBUG build!");
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Logging initialized.");
#endif

        if (setWorkingDir()) {
            appExit(1);
        }
#ifdef _DEBUG
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Working directory set.");
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Arguments:");
        for ( i = 0; i < argc; i++ ) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  argv[%d]=%s", i, argv[i]);
        }
#endif
        
        
        if (argc >= 3) {
            /* argv[1] should be the command */
            /* argv[2] should be the configuration file */

            if(!_stricmp(argv[1],"-?") || !_stricmp(argv[1],"/?")) {
                /* User asked for the usage. */
                wrapperUsage(argv[0]);
            } else {
                /* All 4 valid commands use the configuration file.  It is loaded here to
                 *  reduce duplicate code.  But before loading the parameters, in the case
                 *  of an NT service. the environment variables must first be loaded from
                 *  the registry. */
                if (!_stricmp(argv[1],"-s") || !_stricmp(argv[1],"/s")) {
                    if (wrapperLoadEnvFromRegistry())
                    {
                        result = 1;
                    }
                }

                if (!result) {
                    /* Create a Properties structure. */
                    properties = createProperties();
                    wrapperAddDefaultProperties();

                    /* All 4 valid commands accept a configuration file, followed by 0 or more
                     *  command line properties.  The command line properties need to be
                     *  loaded first, followed by the configuration file. */
                    for (i = 3; i < argc; i++) {
                        if (addPropertyPair(properties, argv[i], TRUE, TRUE)) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, 
                                "The argument '%s' is not a valid property name-value pair.", argv[i]);
                            result = 1;
                        }
                    }

                    /* Now load the configuration file. */
                    if (loadProperties(properties, argv[2])) {
                        /* File not found. */
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "unable to open configuration file. %s", argv[2]);
                        result = 1;
                    } else {
                        /* Store the configuration file name */
                        wrapperData->configFile = argv[2];

                        if (result) {
                            /* There was a problem with the arguments */
                        } else {
                            /* Display the active properties */
#ifdef _DEBUG
                            printf("Debug Configuration Properties:\n");
                            dumpProperties(properties);
#endif

                            /* Apply properties to the WrapperConfig structure */
                            if (wrapperLoadConfiguration()) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                                    "Problem loading wrapper configuration file: %s", argv[2]);
                                exit(1);
                            }

                            /* Change the working directory if configured to do so. */
                            if (wrapperSetWorkingDirProp()) {
                             appExit(1);
                            }
                        
                            /* Perform the specified command */
                            if(!_stricmp(argv[1],"-i") || !_stricmp(argv[1],"/i")) {
                                /* Install an NT service */
                                result = wrapperInstall(argc, argv);
                            } else if(!_stricmp(argv[1],"-r") || !_stricmp(argv[1],"/r")) {
                                /* Remove an NT service */
                                result = wrapperRemove();
                            } else if(!_stricmp(argv[1],"-t") || !_stricmp(argv[1],"/t")) {
                                /* Start an NT service */
                                result = wrapperStartService();
                            } else if(!_stricmp(argv[1],"-p") || !_stricmp(argv[1],"/p")) {
                                /* Stop an NT service */
                                result = wrapperStopService(TRUE);
                            } else if(!_stricmp(argv[1],"-c") || !_stricmp(argv[1],"/c")) {
                                /* Run as a console application */

                                /* Write pid file. */
                                if (wrapperData->pidFilename) {
                                    if (writePidFile()) {
                                        log_printf
                                            (WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                                             "ERROR: Could not write pid file %s: %s",
                                             wrapperData->pidFilename, getLastErrorText());
                                        exit(1);
                                    }
                                }

                                if (!result) {
                                    result = wrapperRunConsole();
                                }
                            } else if(!_stricmp(argv[1],"-s") || !_stricmp(argv[1],"/s")) {
                                /* Run as a service */

                                /* Write pid file. */
                                if (wrapperData->pidFilename) {
                                    if (writePidFile()) {
                                        log_printf
                                            (WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                                             "ERROR: Could not write pid file %s: %s",
                                             wrapperData->pidFilename, getLastErrorText());
                                        exit(1);
                                    }
                                }

                                if (!result) {
                                    /* Prepare the service table */
                                    serviceTable[0].lpServiceName = wrapperData->ntServiceName;
                                    serviceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)wrapperServiceMain;
                                    serviceTable[1].lpServiceName = NULL;
                                    serviceTable[1].lpServiceProc = NULL;
                    
                                    printf("Attempting to start %s as an NT service.\n", wrapperData->ntServiceDisplayName);
                                    printf("\nCalling StartServiceCtrlDispatcher...please wait.\n");
                    
                                    /* Start the service control dispatcher.  This will not return */
                                    /*  if the service is started without problems */
                                    if (!StartServiceCtrlDispatcher(serviceTable)){
                                        printf("\nStartServiceControlDispatcher failed!\n");
                                        printf("\nFor help, type\n\n%s /?\n\n", argv[0]);
                                        result = 1;
                                    }
                                }
                            } else {
                                printf("\nUnrecognized option: %s\n", argv[1]);
                                wrapperUsage(argv[0]);
                                result = 1;
                            }
                        }
                    }
                }
            }
        } else {
            /* Invalid parameters were provided. */
            wrapperUsage(argv[0]);
            result = 1;
        }
    } __except (exceptionFilterFunction(GetExceptionInformation())) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "<-- Wrapper Stopping due to error");
        result = 1;
    }

    appExit(result);
}

 #endif /* ifdef WIN32 */
