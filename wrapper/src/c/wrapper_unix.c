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
 * Revision 1.65  2004/04/08 14:58:59  mortenson
 * Add a wrapper.working.dir property.
 *
 * Revision 1.64  2004/04/08 03:21:57  mortenson
 * Added an environment variable, WRAPPER_PATH_SEPARATOR, whose value is set
 * to either ':' or ';' on startup.
 *
 * Revision 1.63  2004/03/27 16:09:45  mortenson
 * Add wrapper.on_exit.<n> properties to control what happens when a exits based
 * on the exit code.  This led to a major rework of the state engine to make it possible.
 *
 * Revision 1.62  2004/03/14 14:02:37  mortenson
 * Modify the way the Wrapper forcibly kills a frozen JVM on UNIX platforms so
 * that it now sends a SIGTERM, waits up to 5 seconds, then sends a SIGKILL.
 *
 * Revision 1.61  2004/01/16 04:41:59  mortenson
 * The license was revised for this version to include a copyright omission.
 * This change is to be retroactively applied to all versions of the Java
 * Service Wrapper starting with version 3.0.0.
 *
 * Revision 1.60  2004/01/14 09:34:29  mortenson
 * Set the exit code when a jvm needs to be killed to get it to exit, also reset the
 * exit code whenever a new JVM is launched.
 *
 * Revision 1.59  2004/01/10 15:51:31  mortenson
 * Fix a problem where a thread dump would be invoked if the request thread
 * dump on failed JVM exit was enabled and the user forced an immediate
 * shutdown by pressing CTRL-C more than once.
 *
 * Revision 1.58  2004/01/09 19:45:03  mortenson
 * Implement the tick timer on Linux.
 *
 * Revision 1.57  2004/01/09 18:32:48  mortenson
 * Fix some code that had not yet been converted to using ticks.
 *
 * Revision 1.56  2004/01/09 18:22:41  mortenson
 * The code timing the thread dump before a shutdown was still based on the system
 * time, changed over to ticks.  Also extended the time from 3 to 5 seconds.
 *
 * Revision 1.55  2004/01/09 17:49:00  mortenson
 * Rework the logging so it is now threadsafe.
 *
 * Revision 1.54  2004/01/09 05:15:11  mortenson
 * Implement a tick timer and convert the system time over to be compatible.
 *
 * Revision 1.53  2003/10/31 10:16:27  mortenson
 * Improved the algorithm of the request thread dump on failed JVM exit feature
 * so that extremely large thread dumps will not be truncated when the JVM
 * is killed.
 *
 * Revision 1.52  2003/10/30 19:29:45  mortenson
 * Fix some Java style comments.
 *
 * Revision 1.51  2003/09/11 07:37:27  mortenson
 * Fix a compiler warning on Solaris.
 *
 * Revision 1.50  2003/09/09 14:18:10  mortenson
 * Fix a problem where not all properties specified on the command line worked
 * correctly when they included spaces.
 *
 * Revision 1.49  2003/09/04 06:08:17  mortenson
 * Extend the pause between requesting a thread dump on exit and killing the JVM.
 *
 * Revision 1.48  2003/09/03 02:33:38  mortenson
 * Requested restarts no longer reset the restart count.
 * Add new wrapper.ignore_signals property.
 *
 * Revision 1.47  2003/08/17 08:51:35  mortenson
 * Save a little CPU by disabling console log output when the wrapper is
 * daemonized.
 *
 * Revision 1.46  2003/08/15 17:40:26  mortenson
 * Stop clearing the file creation mask when the Unix version of the Wrapper is
 * run as a daemon process.
 *
 * Revision 1.45  2003/08/15 17:16:18  mortenson
 * Fix tabs.
 *
 * Revision 1.44  2003/08/15 17:13:17  mortenson
 * Added the wrapper.java.pidfile property which will cause the pid of the
 * java process to be written to a specified file.
 *
 * Revision 1.43  2003/08/02 16:22:29  mortenson
 * Replace direct calls to strerror with getLastErrorText() to make things
 * consistent with the windows code.
 *
 * Revision 1.42  2003/08/02 16:12:26  mortenson
 * Add a patch by Mike Castle to fix a problem where started wrapper processes
 * would cause the shell from which they were started to hang on exit.
 *
 * Revision 1.41  2003/07/09 05:59:47  mortenson
 * Fix a problem where the Wrapper was leaving a pipe unclosed each time the JVM
 * was restarted on all UNIX platforms.
 *
 * Revision 1.40  2003/05/18 04:08:45  mortenson
 * Fix a problem on UNIX systems where requesting a second thread dump any time
 * during the life of a single Wrapper process would cause the Wrapper and JVM
 * to shutdown rather than perform the thread dump.
 *
 * Revision 1.39  2003/04/16 04:13:11  mortenson
 * Go through and clean up the computation of the number of bytes allocated in
 * malloc statements to make sure that string sizes are always multiplied by
 * sizeof(char), etc.
 *
 * Revision 1.38  2003/04/16 03:58:34  mortenson
 * Fix a potential error if any element of a command string contains % characters.
 *
 * Revision 1.37  2003/04/16 03:39:01  mortenson
 * Make the Wrapper dump the configured properties if _DEBUG is set.
 *
 * Revision 1.36  2003/04/16 03:12:44  mortenson
 * Remove a variable definition that is no longer being used.
 *
 * Revision 1.35  2003/04/15 23:56:27  mortenson
 * Be more careful about writing to the childOutputBuffer to make sure there is
 * always enough space.
 *
 * Revision 1.34  2003/04/15 23:24:22  mortenson
 * Remove casts from all malloc statements.
 *
 * Revision 1.33  2003/04/03 08:10:19  mortenson
 * Fix the imports so the file compiles.  Had edited on a win32 machine.
 *
 * Revision 1.32  2003/04/03 07:39:56  mortenson
 * Convert tabs to spaces.  No other changes.
 *
 * Revision 1.31  2003/04/03 07:37:00  mortenson
 * In the last release, some work was done to avoid false timeouts caused by
 * large quantities of output.  On some heavily loaded systems, timeouts were
 * still being encountered.  Rather than reading up to 50 lines of input, the
 * code will now read for a maximum of 250ms before returning to give the main
 * event loop more cycles.
 *
 * Revision 1.30  2003/04/03 04:05:22  mortenson
 * Fix several typos in the docs.  Thanks to Mike Castle.
 *
 * Revision 1.29  2003/03/26 04:01:40  mortenson
 * Fix an unreleased problem where extremely long lines of output from the
 * JVM were causing a segmentation fault on Unix systems.
 *
 * Revision 1.28  2003/03/21 22:09:26  mortenson
 * Fix a problem on UNIX versions where extra line breaks would sometimes be
 * added to the logged output when there was large amounts of output being
 * sent from the JVM.
 *
 * Revision 1.27  2003/03/21 21:32:00  mortenson
 * Remove tabs.
 *
 * Revision 1.26  2003/03/21 21:25:32  mortenson
 * Fix a problem where very heavy output from the JVM can cause the Wrapper to
 * give a false timeout.  The Wrapper now only ready 50 lines of input at a time
 * to guarantee that the Wrapper's event loop always gets cycles.
 *
 * Revision 1.25  2003/03/13 15:40:41  mortenson
 * Add the ability to set environment variables from within the configuration
 * file or from the command line.
 *
 * Revision 1.24  2003/02/07 16:05:28  mortenson
 * Implemented feature request #676599 to enable the filtering of JVM output to
 * trigger JVM restarts or Wrapper shutdowns.
 *
 * Revision 1.23  2003/02/03 06:58:35  mortenson
 * License transfer to TanukiSoftware.org
 *
 */

/**
 * Author:
 *   Leif Mortenson <leif@tanukisoftware.com>
 *   Ryan Shaw
 *
 * Version CVS $Revision$ $Date$
 */

#ifndef WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "wrapper.h"
#include "property.h"
#include "logger.h"

#define max(x,y) (((x) > (y)) ? (x) : (y)) 
#define min(x,y) (((x) < (y)) ? (x) : (y)) 

static pid_t jvmPid = -1;
int          jvmOut = -1;

/* Define a global pipe descriptor so that we don't have to keep allocating
 *  a new pipe each time a JVM is launched. */
int pipedes[2];
int pipeInitialized = 0;  

char wrapperClasspathSeparator = ':';

/* Flag which is set if this process creates a pid file. */
int ownJavaPidFile = 0;

pthread_t timerThreadId;
/* Initialize the timerTicks to a very high value.  This means that we will
 *  always encounter the first rollover (256 * WRAPPER_MS / 1000) seconds
 *  after the Wrapper the starts, which means the rollover will be well
 *  tested. */
DWORD timerTicks = 0xffffff00;

/******************************************************************************
 * Platform specific methods
 *****************************************************************************/

/**
 * Gets the error code for the last operation that failed.
 */
int wrapperGetLastError() {
    return errno;
}

/**
 * Send a signal to the JVM process asking it to dump its JVM state.
 */
void requestDumpJVMState() {
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Dumping JVM state.");
    if (kill(jvmPid, SIGQUIT) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                   "Could not dump JVM state: %s", getLastErrorText());
    }
}

void handleCommon(const char* signal) {
    if (wrapperData->ignoreSignals) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "%s trapped, but ignored.", signal);
    } else {
        if (wrapperData->exitRequested || wrapperData->restartRequested ||
            (wrapperData->jState == WRAPPER_JSTATE_STOPPING) ||
            (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
            (wrapperData->jState == WRAPPER_JSTATE_DOWN)) {

            /* Signalled while we were already shutting down. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "%s trapped.  Forcing immediate shutdown.", signal);

            /* Disable the thread dump on exit feature if it is set because it
             *  should not be displayed when the user requested the immediate exit. */
            wrapperData->requestThreadDumpOnFailedJVMExit = FALSE;
            wrapperKillProcess();
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "%s trapped.  Shutting down.", signal);
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
}

/**
 * Handle interrupt signals (i.e. Crtl-C).
 */
void handleInterrupt(int sig_num) {
    /* Immediately register this thread with the logger. */
    logRegisterThread(WRAPPER_THREAD_SIGNAL);

    signal(SIGINT, handleInterrupt);

    handleCommon("INT");
}

/**
 * Handle quit signals (i.e. Crtl-\).
 */
void handleQuit(int sig_num) {
    /* Immediately register this thread with the logger. */
    logRegisterThread(WRAPPER_THREAD_SIGNAL);

    signal(SIGQUIT, handleQuit); 
    requestDumpJVMState();
}

/**
 * Handle termination signals (i.e. machine is shutting down).
 */
void handleTermination(int sig_num) {
    /* Immediately register this thread with the logger. */
    logRegisterThread(WRAPPER_THREAD_SIGNAL);

    signal(SIGTERM, handleTermination); 

    handleCommon("TERM");
}

/**
 * The main entry point for the timer thread which is started by
 *  initializeTimer().  Once started, this thread will run for the
 *  life of the process.
 *
 * This thread will only be started if we are configured NOT to
 *  use the system time as a base for the tick counter.
 */
void *timerRunner(void *arg) {
    DWORD sysTicks;
    DWORD lastTickOffset = 0;
    DWORD tickOffset;
    long int offsetDiff;
    int first = 1;

    /* Immediately register this thread with the logger. */
   logRegisterThread(WRAPPER_THREAD_TIMER);

   while (TRUE) {
       usleep(WRAPPER_TICK_MS * 1000);

       /* Get the tick count based on the system time. */
       sysTicks = wrapperGetSystemTicks();

       /* Advance the timer tick count. */
       timerTicks++;

       /* Calculate the offset between the two tick counts. This will always work due to overflow. */
       tickOffset = sysTicks - timerTicks;

       /* The number we really want is the difference between this tickOffset and the previous one. */
       offsetDiff = tickOffset - lastTickOffset;

       if (first) {
           first = 0;
       } else {
           if (offsetDiff > wrapperData->timerSlowThreshold) {
               log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "The timer fell behind the system clock by %ldms.", offsetDiff * WRAPPER_TICK_MS);
           } else if (offsetDiff < -1 * wrapperData->timerFastThreshold) {
               log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "The system clock fell behind the timer by %ldms.", -1 * offsetDiff * WRAPPER_TICK_MS);
       }
           /*
           log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Timer running: %lu, %lu, %lu, %ld", timerTicks, sysTicks, tickOffset, offsetDiff);
           */
       }

       /* Store this tick offset for the next time through the loop. */
       lastTickOffset = tickOffset;
   }

   return NULL;
}

/**
 * Creates a process whose job is to loop and simply increment a ticks
 *  counter.  The tick counter can then be used as a clock as an alternative
 *  to using the system clock.
 */
int initializeTimer() {
    int res;

    res = pthread_create(
        &timerThreadId,
        NULL, /* No attributes. */
        timerRunner,
        NULL); /* No parameters need to passed to the thread. */
    if (res) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
            "Unable to create a timer thread: %d, %s", res, getLastErrorText());
        return 1;
    } else {
        return 0;
    }
}

/**
 * Execute initialization code to get the wrapper set up.
 */
int wrapperInitialize() {
    int retval = 0;
    int res;

    /* Set handlers for signals */
    if (signal(SIGINT,  handleInterrupt)   == SIG_ERR ||
        signal(SIGQUIT, handleQuit)        == SIG_ERR ||
        signal(SIGTERM, handleTermination) == SIG_ERR) {
        retval = -1;
    }

    if (wrapperData->useSystemTime) {
        /* We are going to be using system time so there is no reason to start up a timer thread. */
        timerThreadId = 0;
    } else {
        /* Create an initialize a timer thread. */
        if ((res = initializeTimer()) != 0 ) {
            return res;
        } 
    }

    return retval;
}

void wrapperBuildJavaCommand() {
    char **strings;
    int length, i;

    /* If this is not the first time through, then dispose the old command array */
    if (wrapperData->jvmCommand) {
        i = 0;
        while(wrapperData->jvmCommand[i] != NULL) {
            free(wrapperData->jvmCommand[i]);
            wrapperData->jvmCommand[i] = NULL;
            i++;
        }

        free(wrapperData->jvmCommand);
        wrapperData->jvmCommand = NULL;
    }

    /* Build the Java Command Strings */
    strings = NULL;
    length = 0;
    wrapperBuildJavaCommandArray(&strings, &length, FALSE);
    
    if (wrapperData->isDebugging) {
        for (i = 0; i < length; i++) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Command[%d] : %s", i, strings[i]);
        }
    }

    /* Allocate memory to hold array of command strings */
    wrapperData->jvmCommand = malloc(sizeof(char *) * (length + 1)); 
    /*                        number of arguments + 1 for a NULL pointer at the end */
    for (i = 0; i <= length; i++) {
        if (i < length) {
            wrapperData->jvmCommand[i] = malloc(sizeof(char) * (strlen(strings[i]) + 1));
            strcpy(wrapperData->jvmCommand[i], strings[i]);
        } else {
            wrapperData->jvmCommand[i] = NULL;
        }
    }
    
    /* Free up the temporary command array */
    wrapperFreeJavaCommandArray(strings, length);
}

/**
 * Launches a JVM process and stores it internally.
 */
void wrapperExecute() {
    pid_t proc;

    FILE *pid_fp = NULL;
    mode_t old_umask;

    /* Only allocate a pipe if we have not already done so. */
    if (!pipeInitialized) {
        /* Create the pipe. */
        if (pipe (pipedes) < 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                       "Could not init pipe: %s", getLastErrorText());
            return;
        }
        pipeInitialized = 1;
    }
    
    /* Fork off the child. */
    proc = fork();
    
    if (proc == -1) {
        /* Fork failed. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                   "Could not spawn JVM process: %s", getLastErrorText());
        
        /* The pipedes array is global so do not close the pipes. */

    } else {
        /* Reset the exit code when we launch a new JVM. */
        wrapperData->exitCode = 0;
        
        /* Fork succeeded: increment the process ID for logging. */
        wrapperData->jvmRestarts++;

        if (proc == 0) {
            /* We are the child side. */
            
            /* Send output to the pipe by dupicating the pipe fd and setting the copy as the stdout fd. */
            if (dup2(pipedes[STDOUT_FILENO], STDOUT_FILENO) < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "Unable to set JVM's stdout: %s", getLastErrorText());
                return;
            }
        
            /* Send errors to the pipe by dupicating the pipe fd and setting the copy as the stderr fd. */
            if (dup2(pipedes[STDOUT_FILENO], STDERR_FILENO) < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "Unable to set JVM's stderr: %s", getLastErrorText());
                return;
            }

            /* The pipedes array is global so do not close the pipes. */
            
            /* Child process: execute the JVM. */
            execvp(wrapperData->jvmCommand[0], wrapperData->jvmCommand);
            
            /* We reached this point...meaning we were unable to start. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unable to start JVM: %s (%d)", getLastErrorText(), errno);
        
        } else {
            /* We are the parent side. */
            jvmPid = proc;
            jvmOut = pipedes[STDIN_FILENO];

            /* The pipedes array is global so do not close the pipes. */
            
            /* Mark our side of the pipe so that it won't block
             * and will close on exec, so new children won't see it. */
            (void)fcntl(jvmOut, F_SETFL, O_NONBLOCK);
            (void)fcntl(jvmOut, F_SETFD, FD_CLOEXEC);

            /* If a java pid filename is specified then write the pid of the java process. */
            if (wrapperData->javaPidFilename) {
                old_umask = umask(022);
                pid_fp = fopen(wrapperData->javaPidFilename, "w");
                umask(old_umask);

                if (pid_fp != NULL) {
                    fprintf(pid_fp, "%d\n", (int)jvmPid);
                    fclose(pid_fp);

                    /* Remember that we created the pid file. */
                    ownJavaPidFile = 1;
                } else {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                        "Unable to write the Java PID file: %s", wrapperData->javaPidFilename);
                }
            }
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

/**
 * Checks on the status of the JVM Process.
 * Returns WRAPPER_PROCESS_UP or WRAPPER_PROCESS_DOWN
 */
int wrapperGetProcessStatus() {
    int retval;
    int status;
    int exitCode;
    int res;

    retval = waitpid(jvmPid, &status, WNOHANG);

    if (retval < 0) {
        /* Wait failed. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                   "Critical error: wait for JVM process failed (%s)", getLastErrorText());
        exit(1);

    } else if (retval > 0) {
        /* JVM has exited. */
        res = WRAPPER_PROCESS_DOWN;

        /* Get the exit code of the process. */
        if (WIFEXITED(status)) {
            exitCode = WEXITSTATUS(status);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                       "WIFEXITED indicates that the JVM exited abnormally.");
            exitCode = 1;
        }

        wrapperJVMProcessExited(exitCode);

        /* Remove java pid file if it was registered and created by this process. */
        if ((ownJavaPidFile) && (wrapperData->javaPidFilename)) {
            unlink(wrapperData->javaPidFilename);
            ownJavaPidFile = 0;
        }
    } else {
        res = WRAPPER_PROCESS_UP;
    }
    
    return res;
}

/**
 * This function does nothing on Unix machines.
 */
void wrapperReportStatus(int status, int errorCode, int waitHint) {
    return;
}

char *childOutputBuffer = NULL;
int childOutputBufferSize = 0;
/**
 * Make sure there is enough space in the outputBuffer.
 */
void ensureSpaceInChildOutputBuffer(int childOutputBufferPos, int requiredSpace) {
    char *tempBuf;
    
    if ( childOutputBufferPos >= childOutputBufferSize - requiredSpace ) {
        tempBuf = malloc(sizeof(char) * (childOutputBufferSize + 1024));
        if (childOutputBuffer != NULL) {
            /* Copy over the old data */
            memcpy(tempBuf, childOutputBuffer, childOutputBufferSize);
            tempBuf[childOutputBufferSize - 1] = '\0';
            free(childOutputBuffer);
            childOutputBuffer = NULL;
        } 
        childOutputBuffer = tempBuf;
        childOutputBufferSize += 1024;
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
    int readSize;
    ssize_t bytesRead;
    char readBuf [1025];
    int readBufPos, childOutputBufferPos;
    struct timeb timeBuffer;
    long startTime;
    int startTimeMillis;
    long now;
    int nowMillis;
    long durr;
    
    if (jvmOut != -1) {
        ftime( &timeBuffer );
        startTime = now = timeBuffer.time;
        startTimeMillis = nowMillis = timeBuffer.millitm;

        /*
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "now=%ld, nowMillis=%d", now, nowMillis);
        */

        /* Loop and read as much input as is available.  When a large amount of output is
         *  being piped from the JVM this can lead to the main event loop not getting any
         *  CPU for an extended period of time.  To avoid that problem, this loop is only
         *  allowed to cycle for 250ms before returning.  After 250ms, switch to a less
         *  efficient method of reading data because we need to make sure that we have
         *  not read past a line break before returning. */
        childOutputBufferPos = 0;
        while(1) {
            /*
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "durr=%ld", durr);
            */

            if ((durr = (now - startTime) * 1000 + (nowMillis - startTimeMillis)) < 250) {
                readSize = 1024;
            } else {
                readSize = 1;
            }

            /* Fill read buffer. */
            bytesRead = read(jvmOut, readBuf, readSize);

            if (bytesRead <= 0) {
                /* No more bytes available, return for now. */

                if (childOutputBufferPos > 0) {
                    /* We have a partial line, write it out so it is not lost. */
                    ensureSpaceInChildOutputBuffer( childOutputBufferPos, 1 );
                    childOutputBuffer[childOutputBufferPos] = '\0';
                    wrapperLogChildOutput(childOutputBuffer);
                    childOutputBufferPos = 0;
                } 

                break;
            }

            /* Terminate the read buffer. */
            readBuf[bytesRead] = '\0';
        
            /* Step through chars in read buffer. */
            for (readBufPos = 0; readBufPos < bytesRead; readBufPos++) {
                if (readBuf[readBufPos] == (char)0x0a) {
                    /* Line feed; write out buffer and reset it. */
                    ensureSpaceInChildOutputBuffer( childOutputBufferPos, 1 );
                    childOutputBuffer[childOutputBufferPos] = '\0';
                    wrapperLogChildOutput(childOutputBuffer);
                    childOutputBufferPos = 0;

                    if ( readSize == 1 ) {
                        /* This last line was read byte by byte, now exit. */
                        return -1;
                    }
                } else {
                    ensureSpaceInChildOutputBuffer( childOutputBufferPos, 2 );

                    /* Add character to write buffer. */
                    childOutputBuffer[childOutputBufferPos++] = readBuf[readBufPos];
                }
            }

            /* Get the time again */
            ftime( &timeBuffer );
            now = timeBuffer.time;
            nowMillis = timeBuffer.millitm;
        }
    }
    
    return 0;
}

/**
 * Kill the JVM Process immediately and set the JVM State to WRAPPER_JSTATE_DOWN
 */
void wrapperKillProcess() {
    DWORD startTicks;
    DWORD nowTicks;

    /* Check to make sure that the JVM process is still running */
    if (waitpid(jvmPid, NULL, WNOHANG) == 0) {
        /* JVM is still up when it should have already stopped itself. */
        if (wrapperData->requestThreadDumpOnFailedJVMExit) {
            requestDumpJVMState();

            /* Loop for 5 seconds reading all available input before actually killing
             *  the JVM process.  This is to make sure that the JVM is given enough
             *  time to perform the full thread dump. */
            nowTicks = startTicks = wrapperGetTicks();
            do {
                if (!wrapperReadChildOutput())
                {
                    /* Sleep a moment so this loop does not eat too much CPU. */
                    usleep(250000); /* microseconds */
                }
                nowTicks = wrapperGetTicks();
            } while (wrapperGetTickAge(startTicks, nowTicks) < 5);
        }

        /* Give the JVM one last chance to shutdown on its own by sending it a SIGTERM */
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Sending the JVM process a SIGTERM");
        }
        kill(jvmPid, SIGTERM);

        /* Wait up to 3 seconds for the JVM to respond to the SIGTERM.  Handle any JVM output. */
        nowTicks = startTicks = wrapperGetTicks();
        do {
            if ( !wrapperReadChildOutput() )
            {
                /* Sleep a moment so this loop does not eat too much CPU. */
                usleep(250000); /* microseconds */
            }
            nowTicks = wrapperGetTicks();
        } while ((wrapperGetTickAge( startTicks, nowTicks ) < 5 ) && (waitpid(jvmPid, NULL, WNOHANG) == 0));

        /* Is the JVM still alive? */
        if (waitpid(jvmPid, NULL, WNOHANG) == 0) {
            /* Kill it immediately. */
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM did not exit with a SIGTERM, send a SIGKILL");
            }
            kill(jvmPid, SIGKILL);
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM did not exit on request, terminated with SIGKILL");

            /* Give the JVM a chance to be killed so that the state will be correct. */
            usleep(500000); /* 0.5 seconds in microseconds */
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM did not exit on request, terminated with SIGTERM.");
        }

        /* Set the exit code since we were forced to kill the JVM. */
        wrapperData->exitCode = 1;
    }

    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->jStateTimeoutTicks = 0;
    wrapperData->jStateTimeoutTicksSet = 0;
    jvmPid = -1;

    /* Remove java pid file if it was registered and created by this process. */
    if ((ownJavaPidFile) && (wrapperData->javaPidFilename)) {
        unlink(wrapperData->javaPidFilename);
        ownJavaPidFile = 0;
    }

    /* Close any open socket to the JVM */
    wrapperProtocolClose();
}

/**
 * Show usage.
 */
void wrapperUsage(char *appName) {
    printf("Usage: %s <file> [configuration properties] [...]\n", appName);
    printf("<file> is the application configuration file.\n");
    printf("\n");
    printf("[configuration properties] are configuration name-value pairs which\n");
    printf("  override values in wrapper.conf.  For example:\n");
    printf("  wrapper.debug=true\n");
    printf("\n");
    printf("Options:  --help\n");
}

int writePidFile() {
    FILE *pid_fp = NULL;
    mode_t old_umask;

    /*enter_suid(); */
    old_umask = umask(022);
    pid_fp = fopen(wrapperData->pidFilename, "w");
    umask(old_umask);
    /*leave_suid(); */
    
    if (pid_fp != NULL) {
        fprintf(pid_fp, "%d\n", (int)getpid());
        fclose(pid_fp);
    } else {
        return 1;
    }
    return 0;
}

/**
 * Transform a program into a daemon.
 * Inspired by code from GNU monit, which in turn, was
 * inspired by code from Stephen A. Rago's book,
 * Unix System V Network Programming.
 *
 * The idea is to first fork, then make the child a session leader,
 * and then fork again, so that it, (the session group leader), can
 * exit. This means that we, the grandchild, as a non-session group
 * leader, can never regain a controlling terminal.
 */
void daemonize() {
    pid_t pid;
    int fd;
    
    /* first fork */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Spawning intermediate process...");
    }	
    if ((pid = fork()) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Could not spawn daemon process: %s",
            getLastErrorText());
        exit(1);
    } else if (pid != 0) {
        /* Intermediate process is now running.  This is the original process, so exit. */
        
        /* If the main process was not launched in the background, then we want to make
         * the console output look nice by making sure that all output from the
         * intermediate and daemon threads are complete before this thread exits.
         * Sleep for 0.5 seconds. */
        usleep(500000);
        
        exit(0);
    }
    
    /* become session leader */
    if (setsid() == -1) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "setsid() failed: %s",
           getLastErrorText());
        exit(1);
    }
    
    signal(SIGHUP, SIG_IGN); /* don't let future opens allocate controlling terminals */
    
    /* Redirect stdin, stdout and stderr before closing to prevent the shell which launched
     *  the Wrapper from hanging when it exits. */
    fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1) {
        close(STDIN_FILENO);
        dup2(fd, STDIN_FILENO);
        close(STDOUT_FILENO);
        dup2(fd, STDOUT_FILENO);
        close(STDERR_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd != STDIN_FILENO &&
            fd != STDOUT_FILENO &&
            fd != STDERR_FILENO) {
            close(fd);
        }
    }
    /* Console output was disabled above, so make sure the console log output is disabled
     *  so we don't waste any CPU formatting and sending output to '/dev/null'/ */
    setConsoleLogLevelInt(LEVEL_NONE);
    
    /* second fork */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Spawning daemon process...");
    }	
    if ((pid = fork()) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Could not spawn daemon process: %s",
            getLastErrorText());
        exit(1);
    } else if (pid != 0) {
        /* Daemon process is now running.  This is the intermediate process, so exit. */
        exit(0);
    }
} 


/*******************************************************************************
 * Main function                                                               *
 *******************************************************************************/

int main(int argc, char **argv) {
    int exitStatus;
    int i;

    /* Initialize the WrapperConfig structure. */
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
    
    if (argc < 2) {
        wrapperUsage(argv[0]);
        exit(1);
        
    } else if (strcmp(argv[1],"--help") == 0) {
        wrapperUsage(argv[0]);
        exit(0);
        
    } else {
        /* Create a Properties structure. */
        properties = createProperties();
        wrapperAddDefaultProperties();

        /* The first argument is the configuration file, followed by 0 or more
         *  command line properties.  The command line properties need to be
         *  loaded first, followed by the configuration file. */
        for (i = 2; i < argc; i++) {
            if (addPropertyPair(properties, argv[i], TRUE, TRUE)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, 
                    "The argument '%s' is not a valid property name-value pair.", argv[i]);
                exit(1);
            }
        }

        /* Now load the configuration file. */
        if (loadProperties(properties, argv[1])) {
            /* File not found. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                "Unable to open wrapper configuration file: %s", argv[1]);
            exit(1);
        
        } else {
            /* Store the configuration file name. */
            wrapperData->configFile = argv[1];
            
            /* Display the active properties */
#ifdef _DEBUG
            printf("Debug Configuration Properties:\n");
            dumpProperties(properties);
#endif

            /* Apply properties to the WrapperConfig structure. */
            if (wrapperLoadConfiguration()) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    "Problem loading wrapper configuration file: %s", argv[1]);
                exit(1);
            }

            /* Change the working directory if configured to do so. */
            if (wrapperSetWorkingDirProp()) {
                appExit(1);
            }

            /* fork to a Daemonized process if configured to do so. */
            if (wrapperData->daemonize) {
                daemonize();
            }

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

            exitStatus = wrapperRunConsole();
            
            /* Remove pid file. */
            if (wrapperData->pidFilename) {
                unlink(wrapperData->pidFilename);
            }

            exit(exitStatus);
        }
    }
}

#endif /* ifndef WIN32 */
