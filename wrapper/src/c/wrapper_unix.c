/*
 * Copyright (c) 1999, 2005 Tanuki Software Inc.
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
 * Revision 1.106  2005/09/29 01:49:27  mortenson
 * Fix a problem on UNIX where the child JVM was sometimes leaving around
 * zombie processes after a restart.
 *
 * Revision 1.105  2005/08/21 14:22:14  mortenson
 * Modify the usage output of the Wrapper on all platforms so the Wrapper's
 * version is now included.  It was not previously possible to get the version
 * of the Wrapper being used without launching a JVM.
 *
 * Revision 1.104  2005/05/23 02:37:55  mortenson
 * Update the copyright information.
 *
 * Revision 1.103  2005/05/08 10:33:57  mortenson
 * Fix some compiler problems.
 *
 * Revision 1.102  2005/05/08 10:11:15  mortenson
 * Fix some unix linking problems.
 *
 * Revision 1.101  2005/05/08 09:43:33  mortenson
 * Add a new wrapper.java.idfile property which can be used by external
 * applications to monitor the internal state of the JVM at any given time.
 *
 * Revision 1.100  2005/05/05 16:05:45  mortenson
 * Add new wrapper.statusfile and wrapper.java.statusfile properties which can
 *  be used by external applications to monitor the internal state of the Wrapper
 *  or JVM at any given time.
 *
 * Revision 1.99  2005/03/24 06:09:14  mortenson
 * Fix some compiler warnings on A64 linux platforms.
 *
 * Revision 1.98  2004/12/08 03:31:21  mortenson
 * Simplify the makefiles by making the use of nanosleep the default in the c source.
 *
 * Revision 1.97  2004/12/08 02:55:23  mortenson
 * Modify the UNIX version so it always sets the working dir to the location
 * of the wrapper binary on startup.  This is to make it work the same way
 * as the Windows version.
 *
 * Revision 1.96  2004/12/06 08:18:07  mortenson
 * Make it possible to reload the Wrapper configuration just before a JVM restart.
 *
 * Revision 1.95  2004/11/15 08:15:48  mortenson
 * Make it possible for users to access the Wrapper and JVM PIDs from within the JVM.
 *
 * Revision 1.94  2004/10/20 07:55:35  mortenson
 * Make sure that the logfile is flushed in a timely manner rather than leaving
 * it entirely up to the OS.
 *
 * Revision 1.93  2004/10/19 11:48:20  mortenson
 * Rework logging so that the logfile is kept open.  Results in a 4 fold speed increase.
 *
 * Revision 1.92  2004/10/18 09:37:22  mortenson
 * Add the wrapper.cpu_output and wrapper.cpu_output.interval properties to
 * make it possible to track CPU usage of the Wrapper and JVM over time.
 *
 * Revision 1.91  2004/10/18 05:43:45  mortenson
 * Add the wrapper.memory_output and wrapper.memory_output.interval properties to
 * make it possible to track memory usage of the Wrapper and JVM over time.
 * Change the JVM process variable names to make their meaning more obvious.
 *
 * Revision 1.90  2004/09/22 11:06:28  mortenson
 * Start using nanosleep in place of usleep on UNIX platforms to work around usleep
 * problems with alarm signals on Solaris.
 *
 * Revision 1.89  2004/09/17 01:27:46  mortenson
 * Fix an access violation on shutdown when the Wrapper was started without any
 * arguments.  Caused by uninitialized pointers.
 *
 * Revision 1.88  2004/09/16 04:07:20  mortenson
 * Centralize shutdown code on UNIX version in an appExit method as was already
 * being done for Windows versions.
 *
 * Revision 1.87  2004/08/06 16:17:05  mortenson
 * Added a new wrapper.java.command.loglevel property which makes it possible
 * to control the log level of the generated java command.
 *
 * Revision 1.86  2004/08/06 07:27:06  mortenson
 * Make it possible to display timer output without having to enable all debug output.
 *
 * Revision 1.85  2004/07/15 14:30:49  mortenson
 * Remove an unused variable.
 *
 * Revision 1.84  2004/07/15 10:09:55  mortenson
 * Correct the FREEBSD ifdef statement.
 *
 * Revision 1.83  2004/07/15 03:26:36  mortenson
 * Now that the FreeBSD O_NONBLOCK bug is better understood, apply a better
 * patch to fix it.
 *
 * Revision 1.82  2004/07/11 14:01:34  mortenson
 * Work around a problem where the non-blocking bit on the JVM pipe is reset
 * on some FreeBSD systems.
 * Reenable sigaction for FreeBSD platforms.
 *
 * Revision 1.81  2004/07/05 09:56:59  mortenson
 * Long day!  On further testing, I realized that the duplicate thread dump problem
 * also exists when using the signal function.  So the only problem with sigaction is
 * on FreeBSD.  Go back to only disabling it on that platform.
 *
 * Revision 1.80  2004/07/05 09:05:37  mortenson
 * Go back to using signal on all UNIX platforms by default.  Encountered another
 * problem where it is not immediately possible to prevent duplicate thread dumps
 * when ctrl+\ is pressed in the console.   Disable this for now and maybe get it
 * into a future release...
 *
 * Revision 1.79  2004/07/05 08:41:03  mortenson
 * Queue some additional signal output.
 *
 * Revision 1.78  2004/07/05 07:43:54  mortenson
 * Fix a deadlock on solaris by being very careful that we never perform any direct
 * logging from within a signal handler.
 *
 * Revision 1.77  2004/07/05 03:15:55  mortenson
 * Partially back out the switch from signal to sigaction as it had problems on some
 * platforms.  It can still be enabled by uncommenting the WRAPPER_USE_SIGACTION
 * define.
 *
 * Revision 1.76  2004/07/01 03:33:31  mortenson
 * Add some additional error checks after calls to control the pipe between
 * the JVM and Wrapper.
 *
 * Revision 1.75  2004/06/16 15:56:29  mortenson
 * Added a new property, wrapper.anchorfile, which makes it possible to
 * cause the Wrapper to shutdown by deleting an anchor file.
 *
 * Revision 1.74  2004/06/15 03:16:36  mortenson
 * Debug output from the forked child process just before the JVM was launched
 * was causing corruption of debug output on Solaris.
 *
 * Revision 1.73  2004/06/15 02:14:19  mortenson
 * Get the new sigaction code compiling on Solaris.
 *
 * Revision 1.72  2004/06/14 16:59:20  mortenson
 * Replace the signal function with sigaction so that we can provide useful
 * debug information about where the signal came from.
 *
 * Revision 1.71  2004/06/14 08:24:42  mortenson
 * Catch and ignore SIGALRM signals that we are being hit with on Solaris when the
 * Tick timer is being used.
 *
 * Revision 1.70  2004/06/14 07:20:40  mortenson
 * Add some additional output and a wrapper.timer_output property to help with
 * debugging timer issues.
 *
 * Revision 1.69  2004/06/06 15:28:18  mortenson
 * Fix a synchronization problem in the logging code which would
 * occassionally cause the Wrapper to crash with an Access Violation.
 * The problem was only encountered when the tick timer was enabled,
 * and was only seen on multi-CPU systems.  Bug #949877.
 *
 * Revision 1.68  2004/06/04 06:16:40  mortenson
 * Fix a problem where signals fired at UNIX versions of the wrapper were
 * not being handled correctly when the tick timer was being used.
 *
 * Revision 1.67  2004/06/02 09:15:55  mortenson
 * Fix some indentation and a few comment typos.
 *
 * Revision 1.66  2004/04/08 15:09:12  mortenson
 * Fix a compiler error.
 *
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
#include <pwd.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "wrapper.h"
#include "wrapperinfo.h"
#include "property.h"
#include "logger.h"

#include <sys/resource.h>
#include <sys/time.h>

#ifndef USE_USLEEP
#include <time.h>
#endif

/* Moved from using the signal call to using sigaction.  Until this has
 *  been well tested on all platforms, make it easy to go back by commenting
 *  out the WRAPPER_USE_SIGACTION definition. */
#define WRAPPER_USE_SIGACTION

#ifdef WRAPPER_USE_SIGACTION
#ifndef getsid
/* getpid links ok on Linux, but is not defined correctly. */
pid_t getsid(pid_t pid);
#endif
#endif

#define max(x,y) (((x) > (y)) ? (x) : (y)) 
#define min(x,y) (((x) < (y)) ? (x) : (y)) 

static pid_t jvmPid = -1;
int          jvmOut = -1;

/* Define a global pipe descriptor so that we don't have to keep allocating
 *  a new pipe each time a JVM is launched. */
int pipedes[2];
int pipeInitialized = 0;  

char wrapperClasspathSeparator = ':';

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
 * exits the application after running shutdown code.
 */
void appExit(int exitCode) {
    /* Remove pid file.  It may no longer exist. */
    if (wrapperData->pidFilename) {
        unlink(wrapperData->pidFilename);
    }
    
    /* Remove status file.  It may no longer exist. */
    if (wrapperData->statusFilename) {
        unlink(wrapperData->statusFilename);
    }
    
    /* Remove java status file if it was registered and created by this process. */
    if (wrapperData->javaStatusFilename) {
        unlink(wrapperData->javaStatusFilename);
    }

    /* Remove java id file if it was registered and created by this process. */
    if (wrapperData->javaIdFilename) {
        unlink(wrapperData->javaIdFilename);
    }

    /* Remove anchor file.  It may no longer exist. */
    if (wrapperData->anchorFilename) {
        unlink(wrapperData->anchorFilename);
    }
    
    /* Clean up the logging system. */
    disposeLogging();

    exit(exitCode);
}

/**
 * Gets the error code for the last operation that failed.
 */
int wrapperGetLastError() {
    return errno;
}

/**
 * Writes a PID to disk.
 *
 * filename: File to write to.
 * pid: pid to write in the file.
 */
int writePidFile(const char *filename, DWORD pid) {
    FILE *pid_fp = NULL;
    int old_umask;

    old_umask = umask(022);
    pid_fp = fopen(filename, "w");
    umask(old_umask);
    
    if (pid_fp != NULL) {
        fprintf(pid_fp, "%d\n", (int)pid);
        fclose(pid_fp);
    } else {
        return 1;
    }
    return 0;
}

/**
 * Send a signal to the JVM process asking it to dump its JVM state.
 */
void wrapperRequestDumpJVMState(int useLoggerQueue) {
    log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
        "Dumping JVM state.");
    if (kill(jvmPid, SIGQUIT) < 0) {
        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                   "Could not dump JVM state: %s", getLastErrorText());
    }
}

#ifdef WRAPPER_USE_SIGACTION
const char* getSignalCodeDesc(int code) {
    switch (code) {
    case SI_USER:
        return "kill, sigsend or raise";

#ifdef SI_KERNEL
    case SI_KERNEL:
        return "the kernel";
#endif

    case SI_QUEUE:
        return "sigqueue";

    case SI_TIMER:
        return "timer expired";

    case SI_MESGQ:
        return "mesq state changed";

    case SI_ASYNCIO:
        return "AIO completed";

#ifdef SI_SIGIO
    case SI_SIGIO:
        return "queued SIGIO";
#endif

    default:
        return "unknown";
    }
}

void descSignal(siginfo_t *sigInfo) {
    struct passwd *pw;
    char *uName;

    /* Not supported on all platforms */
    if (sigInfo == NULL) {
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            "Signal trapped.  No details available.");
        return;
    }

    if (wrapperData->isDebugging) {
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            "Signal trapped.  Details:");

        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            "  signal number=%d, source=\"%s\"",
            sigInfo->si_signo,
            getSignalCodeDesc(sigInfo->si_code));

        if (sigInfo->si_errno != 0) {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "  signal err=%d, \"%s\"",
                sigInfo->si_errno,
                strerror(sigInfo->si_errno));
        }

        if (sigInfo->si_code == SI_USER) {
            pw = getpwuid(sigInfo->si_uid);
            if (pw == NULL) {
                uName = "<unknown>";
            } else {
                uName = pw->pw_name;
            }

            /* It appears that the getsid function was added in version 1.3.44 of the linux kernel. */
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "  signal generated by PID: %d (Session PID: %d), UID: %d (%s)",
                sigInfo->si_pid, getsid(sigInfo->si_pid), sigInfo->si_uid, uName);
        }
    }
}
#endif

#ifdef WRAPPER_USE_SIGACTION
void sigActionCommon(const char *sigName, siginfo_t *sigInfo) {
#else
void handleCommon(const char* sigName) {
#endif
    pthread_t threadId;
    threadId = pthread_self();

    /* All threads will receive a signal.  We want to ignore any signal sent to the timer thread. */
    if (threadId == timerThreadId) {
        if (wrapperData->isDebugging) {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "%s trapped, but signals for timer thread are ignored.", sigName);
        }
    } else {
        if (wrapperData->ignoreSignals) {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                "%s trapped, but ignored.", sigName);
        } else {
            if (wrapperData->exitRequested || wrapperData->restartRequested ||
                (wrapperData->jState == WRAPPER_JSTATE_STOPPING) ||
                (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
                (wrapperData->jState == WRAPPER_JSTATE_KILLING) ||
                (wrapperData->jState == WRAPPER_JSTATE_DOWN)) {

                /* Signalled while we were already shutting down. */
                log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    "%s trapped.  Forcing immediate shutdown.", sigName);

                /* Disable the thread dump on exit feature if it is set because it
                 *  should not be displayed when the user requested the immediate exit. */
                wrapperData->requestThreadDumpOnFailedJVMExit = FALSE;
                wrapperKillProcess(TRUE);
            } else {
                log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    "%s trapped.  Shutting down.", sigName);
                wrapperStopProcess(TRUE, 0);
            }
            /* Don't actually kill the process here.  Let the application shut itself down */

            /* To make sure that the JVM will not be restarted for any reason,
             *  start the Wrapper shutdown process as well. */
            if ((wrapperData->wState == WRAPPER_WSTATE_STOPPING) ||
                (wrapperData->wState == WRAPPER_WSTATE_STOPPED)) {
                /* Already stopping. */
            } else {
                wrapperSetWrapperState(WRAPPER_WSTATE_STOPPING);
            }
        }
    }
}

#ifdef WRAPPER_USE_SIGACTION
/**
 * Handle alarm signals.  We are getting them on solaris when running with
 *  the tick timer.  Not yet sure where they are coming from.
 */
void sigActionAlarm(int sigNum, siginfo_t *sigInfo, void *na) {
    pthread_t threadId;

    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    descSignal(sigInfo);

    threadId = pthread_self();

    if (wrapperData->isDebugging) {
        if (threadId == timerThreadId) {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "Timer thread received an Alarm signal.  Ignoring.");
        } else {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "Received an Alarm signal.  Ignoring.");
        }
    }
}

/**
 * Handle interrupt signals (i.e. Crtl-C).
 */
void sigActionInterrupt(int sigNum, siginfo_t *sigInfo, void *na) {
    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    descSignal(sigInfo);

    sigActionCommon("INT", sigInfo);
}

/**
 * Handle quit signals (i.e. Crtl-\).
 */
void sigActionQuit(int sigNum, siginfo_t *sigInfo, void *na) {
    pthread_t threadId;

    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    descSignal(sigInfo);

    threadId = pthread_self();

    if (threadId == timerThreadId) {
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            "Timer thread received an Quit signal.  Ignoring.");
    } else {
        wrapperRequestDumpJVMState(TRUE);
    }
}

/**
 * Handle termination signals (i.e. machine is shutting down).
 */
void sigActionChildDeath(int sigNum, siginfo_t *sigInfo, void *na) {
    /* On UNIX, when a Child process changes state, a SIGCHLD signal is sent to the parent.
     *  The parent should do a wait to make sure the child is cleaned up and doesn't become
     *  a zombie process. */

    descSignal(sigInfo);

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Received SIGCHLD, calling wait().");
    wait(NULL);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "wait() returned, zombie should be gone.");
}

/**
 * Handle termination signals (i.e. machine is shutting down).
 */
void sigActionTermination(int sigNum, siginfo_t *sigInfo, void *na) {
    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    descSignal(sigInfo);

    sigActionCommon("TERM", sigInfo);
}

/**
 * Registers a single signal handler.
 */
int registerSigAction(int sigNum, void (*sigAction)(int, siginfo_t *, void *)) {
    struct sigaction newAct;

    newAct.sa_sigaction = sigAction;
    sigemptyset(&newAct.sa_mask);
    newAct.sa_flags = SA_SIGINFO;

    if (sigaction(sigNum, &newAct, NULL)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
            "Unable to register signal handler for signal %d.  %s", sigNum, getLastErrorText());
        return 1;
    }
    return 0;
}
#else
/**
 * Handle alarm signals.  We are getting them on solaris when running with
 *  the tick timer.  Not yet sure where they are coming from.
 */
void handleAlarm(int sig_num) {
    pthread_t threadId;

    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    /* Ignore any other signals while in this handler. */
    signal(SIGALRM, SIG_IGN);

    threadId = pthread_self();

    if (wrapperData->isDebugging) {
        if (threadId == timerThreadId) {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "Timer thread received an Alarm signal.  Ignoring.");
        } else {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "Received an Alarm signal.  Ignoring.");
        }
    }    

    signal(SIGALRM, handleAlarm);
}

/**
 * Handle interrupt signals (i.e. Crtl-C).
 */
void handleInterrupt(int sig_num) {
    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    /* Ignore any other signals while in this handler. */
    signal(SIGINT, SIG_IGN);

    handleCommon("INT");

    signal(SIGINT, handleInterrupt);
}

/**
 * Handle quit signals (i.e. Crtl-\).
 */
void handleQuit(int sig_num) {
    pthread_t threadId;

    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    threadId = pthread_self();

    /* Ignore any other signals while in this handler. */
    signal(SIGQUIT, SIG_IGN);

    if (threadId == timerThreadId) {
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            "Timer thread received an Quit signal.  Ignoring.");
    } else {
        wrapperRequestDumpJVMState(TRUE);
    }

    signal(SIGQUIT, handleQuit); 
}

/**
 * Handle child death
 */
void handleChildDeath(int sig_num) {
    /* On UNIX, when a Child process changes state, a SIGCHLD signal is sent to the parent.
     *  The parent should do a wait to make sure the child is cleaned up and doesn't become
     *  a zombie process. */

    /* Ignore any other signals while in this handler. */
    signal(SIGTERM, SIG_IGN);
    
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Received SIGCHLD, calling wait().");
    wait(NULL);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "wait() returned, zombie should be gone.");
    
    signal(SIGCHLD, handleChildDeath);
}

/**
 * Handle termination signals (i.e. machine is shutting down).
 */
void handleTermination(int sig_num) {
    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    /* Ignore any other signals while in this handler. */
    signal(SIGTERM, SIG_IGN);

    handleCommon("TERM");

    signal(SIGTERM, handleTermination); 
}
#endif

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

    if (wrapperData->isTimerOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Timer thread started.");
    }

    while (TRUE) {
        wrapperSleep(WRAPPER_TICK_MS);

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

            if (wrapperData->isTimerOutputEnabled) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    "    Timer: ticks=%lu, system ticks=%lu, offset=%lu, offsetDiff=%ld",
                    timerTicks, sysTicks, tickOffset, offsetDiff);
            }
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

    if (wrapperData->isTimerOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Launching Timer thread.");
    }

    res = pthread_create(
        &timerThreadId,
        NULL, /* No attributes. */
        timerRunner,
        NULL); /* No parameters need to be passed to the thread. */
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

#ifdef WRAPPER_USE_SIGACTION
    /* Register any signal actions we are concerned with. */
    if (registerSigAction(SIGALRM, sigActionAlarm) ||
        registerSigAction(SIGINT,  sigActionInterrupt) ||
        registerSigAction(SIGQUIT, sigActionQuit) ||
        registerSigAction(SIGCHLD, sigActionChildDeath) ||
        registerSigAction(SIGTERM, sigActionTermination)) {
        retval = -1;
    }
#else
    /* Register any signals we are concerned with. */
    if (signal(SIGALRM, handleAlarm)       == SIG_ERR ||
        signal(SIGINT,  handleInterrupt)   == SIG_ERR ||
        signal(SIGQUIT, handleQuit)        == SIG_ERR ||
        signal(SIGCHLD, handleChildDeath)  == SIG_ERR ||
        signal(SIGTERM, handleTermination) == SIG_ERR) {
        retval = -1;
    }
#endif

    if (wrapperData->useSystemTime) {
        /* We are going to be using system time so there is no reason to start up a timer thread. */
        timerThreadId = 0;
    } else {
        /* Create and initialize a timer thread. */
        if ((res = initializeTimer()) != 0 ) {
            return res;
        } 
    }

    return retval;
}

/**
 * Cause the current thread to sleep for the specified number of milliseconds.
 *  Sleeps over one second are not allowed.
 */
void wrapperSleep(int ms) {
    /* We want to use nanosleep if it is available, but make it possible for the
       user to build a version that uses usleep if they want.
       usleep does not behave nicely with signals thrown while sleeping.  This
       was the believed cause of a hang experienced on one Solaris system. */
#ifdef USE_USLEEP
    if (wrapperData->isSleepOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Sleep: usleep %dms", ms);
    }
    usleep(ms * 1000); /* microseconds */
#else
    struct timespec ts;
    
    ts.tv_sec = 0;
    ts.tv_nsec = ms * 1000000; /* nanoseconds */
    
    if (wrapperData->isSleepOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Sleep: nanosleep %dms", ms);
    }
    nanosleep(&ts, NULL);
#endif
    
    if (wrapperData->isSleepOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Sleep: awake");
    }
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
    
    if (wrapperData->commandLogLevel != LEVEL_NONE) {
        for (i = 0; i < length; i++) {
            log_printf(WRAPPER_SOURCE_WRAPPER, wrapperData->commandLogLevel,
                "Command[%d] : %s", i, strings[i]);
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
    
    /* Make sure the log file is closed before the Java process is created.  Failure to do
     *  so will give the Java process a copy of the open file.  This means that this process
     *  will not be able to rename the file even after closing it because it will still be
     *  open in the Java process.  Also set the auto close flag to make sure that other
     *  threads do not reopen the log file as the new process is being created. */
    setLogfileAutoClose(TRUE);
    closeLogfile();
    
    /* Fork off the child. */
    proc = fork();
    
    if (proc == -1) {
        /* Fork failed. */
        
        /* Restore the auto close flag. */
        setLogfileAutoClose(wrapperData->logfileInactivityTimeout <= 0);

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
            
            /* The logging code causes some log corruption if logging is called from the
             *  child of a fork.  Not sure exactly why but most likely because the forked
             *  child receives a copy of the mutex and thus synchronization is not working.
             * It is ok to log errors in here, but avoid output otherwise. */
            
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
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                "Unable to start JVM: %s (%d)", getLastErrorText(), errno);
        
        } else {
            /* We are the parent side. */
            jvmPid = proc;
            jvmOut = pipedes[STDIN_FILENO];
            
            /* Restore the auto close flag. */
            setLogfileAutoClose(wrapperData->logfileInactivityTimeout <= 0);

            /* The pipedes array is global so do not close the pipes. */
            
            /* Mark our side of the pipe so that it won't block
             * and will close on exec, so new children won't see it. */
            if (fcntl(jvmOut, F_SETFL, O_NONBLOCK) < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                    "Failed to set jvm output handle to non blocking mode: %s (%d)",
                    getLastErrorText(), errno);
            }
            if (fcntl(jvmOut, F_SETFD, FD_CLOEXEC) < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                    "Failed to set jvm output handle to close on JVM exit: %s (%d)",
                    getLastErrorText(), errno);
            }

            /* If a java pid filename is specified then write the pid of the java process. */
            if (wrapperData->javaPidFilename) {
                if (writePidFile(wrapperData->javaPidFilename, getpid())) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                        "Unable to write the Java PID file: %s", wrapperData->javaPidFilename);
                }
            }

            /* If a java id filename is specified then write the pid of the java process. */
            if (wrapperData->javaIdFilename) {
                if (writePidFile(wrapperData->javaIdFilename, wrapperData->jvmRestarts)) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                        "Unable to write the Java ID file: %s", wrapperData->javaIdFilename);
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
 * Returns the PID of the Wrapper process.
 */
int wrapperGetPID() {
    return (int)getpid();
}

/**
 * Outputs a log entry at regular intervals to track the memory usage of the
 *  Wrapper and its JVM.
 */
void wrapperDumpMemory() {
    /* Not yet implemented on UNIX platforms. */
}

/**
 * Outputs a log entry at regular intervals to track the CPU usage over each
 *  interval for the Wrapper and its JVM.
 */
void wrapperDumpCPUUsage() {
    struct rusage wUsage;
    struct rusage jUsage;

    if (getrusage(RUSAGE_SELF, &wUsage)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            "Call to getrusage failed for Wrapper process: %s", getLastErrorText());
        return;
    }
    if (getrusage(RUSAGE_CHILDREN, &jUsage)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            "Call to getrusage failed for Java process: %s", getLastErrorText());
        return;
    }

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
        "Wrapper CPU: system %ld.%03ld, user %ld.%03ld  Java CPU: system %ld.%03ld, user %ld.%03ld",
        wUsage.ru_stime.tv_sec, wUsage.ru_stime.tv_usec / 1000,
        wUsage.ru_utime.tv_sec, wUsage.ru_utime.tv_usec / 1000,
        jUsage.ru_stime.tv_sec, jUsage.ru_stime.tv_usec / 1000,
        jUsage.ru_utime.tv_sec, jUsage.ru_utime.tv_usec / 1000);
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
        appExit(1);
        
        /* For compiler, won't get here. */
        res = WRAPPER_PROCESS_DOWN;
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
        if (wrapperData->javaPidFilename) {
            unlink(wrapperData->javaPidFilename);
        }
    } else {
        res = WRAPPER_PROCESS_UP;
    }
    
    return res;
}

/**
 * This function does nothing on Unix machines.
 */
void wrapperReportStatus(int useLoggerQueue, int status, int errorCode, int waitHint) {
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

#if defined OPENBSD || defined FREEBSD
            /* Work around FreeBSD Bug #kern/64313
             *  http://www.freebsd.org/cgi/query-pr.cgi?pr=kern/64313
             *
             * When linked with the pthreads library the O_NONBLOCK flag is being reset
             *  on the jvmOut handle.  Not sure yet of the exact event that is causing
             *  this, but once it happens reads will start to block even though calls
             *  to fcntl(jvmOut, F_GETFL) say that the O_NONBLOCK flag is set.
             * Calling fcntl(jvmOut, F_SETFL, O_NONBLOCK) again will set the flag back
             *  again and cause it to start working correctly.  This may only need to
             *  be done once, however, because F_GETFL does not return the accurate
             *  state there is no reliable way to check.  Be safe and always set the
             *  flag. */
            if (fcntl(jvmOut, F_SETFL, O_NONBLOCK) < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                    "Failed to set jvm output handle to non blocking mode: %s (%d)",
                    getLastErrorText(), errno);
            }
#endif

            /* Fill read buffer. */
            bytesRead = read(jvmOut, readBuf, readSize);

            if (bytesRead <= 0) {
                /* No more bytes available, return for now.  But make sure that this was not an error. */
                if ( errno == EAGAIN ) {
                    /* Normal, the call would have blocked as there is no data available. */
                } else {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                        "Failed to read console output from the JVM: %s (%d)",
                        getLastErrorText(), errno);
                }

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
void wrapperKillProcessNow() {
    /* Check to make sure that the JVM process is still running */
    if (waitpid(jvmPid, NULL, WNOHANG) == 0) {
        /* JVM is still up when it should have already stopped itself. */

        /* The JVM process is not responding so the only choice we have is to
         *  kill it. */
        if (kill(jvmPid, SIGKILL)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM did not exit on request.");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                "  Attempt to terminate process failed: %s", getLastErrorText());
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM did not exit on request, terminated");
        }

        /* Give the JVM a chance to be killed so that the state will be correct. */
        wrapperSleep(500); /* 0.5 seconds */

        /* Set the exit code since we were forced to kill the JVM. */
        wrapperData->exitCode = 1;
    }

    wrapperSetJavaState(WRAPPER_JSTATE_DOWN, -1, -1);
    jvmPid = -1;

    /* Remove java pid file if it was registered and created by this process. */
    if (wrapperData->javaPidFilename) {
        unlink(wrapperData->javaPidFilename);
    }

    /* Close any open socket to the JVM */
    wrapperProtocolClose();
}

/**
 * Puts the Wrapper into a state where the JVM will be killed at the soonest
 *  possible oportunity.  It is necessary to wait a moment if a final thread
 *  dump is to be requested.  This call wll always set the JVM state to
 *  WRAPPER_JSTATE_KILLING.
 */
void wrapperKillProcess(int useLoggerQueue) {
    int delay = 0;

    /* Check to make sure that the JVM process is still running */
    if (waitpid(jvmPid, NULL, WNOHANG) == 0) {
        /* JVM is still up when it should have already stopped itself. */
        if (wrapperData->requestThreadDumpOnFailedJVMExit) {
            wrapperRequestDumpJVMState(useLoggerQueue);
            
            delay = 5;
        }
    }

    wrapperSetJavaState(WRAPPER_JSTATE_KILLING, wrapperGetTicks(), delay);
}

/**
 * Show usage.
 */
void wrapperUsage(char *appName) {
    printf("Wrapper (Version %s) http://wrapper.tanukisoftware.org\n", wrapperVersion);
    printf("\n");
    printf("Usage: %s <file> [configuration properties] [...]\n", appName);
    printf("<file> is the application configuration file.\n");
    printf("\n");
    printf("[configuration properties] are configuration name-value pairs which\n");
    printf("  override values in wrapper.conf.  For example:\n");
    printf("  wrapper.debug=true\n");
    printf("\n");
    printf("Options:  --help\n");
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
    
    /* Set the auto close flag and close the logfile before doing any forking to avoid
     *  duplicate open files. */
    setLogfileAutoClose(TRUE);
    closeLogfile();
    
    /* first fork */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Spawning intermediate process...");
    }	
    if ((pid = fork()) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Could not spawn daemon process: %s",
            getLastErrorText());
        appExit(1);
    } else if (pid != 0) {
        /* Intermediate process is now running.  This is the original process, so exit. */
        
        /* If the main process was not launched in the background, then we want to make
         * the console output look nice by making sure that all output from the
         * intermediate and daemon threads are complete before this thread exits.
         * Sleep for 0.5 seconds. */
        wrapperSleep(500);
        
        /* Call exit rather than appExit as we are only exiting this process. */
        exit(0);
    }
    
    /* become session leader */
    if (setsid() == -1) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "setsid() failed: %s",
           getLastErrorText());
        appExit(1);
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
        appExit(1);
    } else if (pid != 0) {
        /* Daemon process is now running.  This is the intermediate process, so exit. */
        /* Call exit rather than appExit as we are only exiting this process. */
        exit(0);
    }
    
    /* Restore the auto close flag in the daemonized process. */
    setLogfileAutoClose(wrapperData->logfileInactivityTimeout <= 0);
} 


/**
 * Sets the working directory to that of the current executable
 */
int setWorkingDir(char *app) {
    char szPath[PATH_MAX];
    char* pos;
    
    /* Get the full path and filename of this program */
    if (realpath(app, szPath) == NULL) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to get the path-%s", getLastErrorText());
        return 1;
    }

    /* The wrapperData->isDebugging flag will never be set here, so we can't really use it. */
#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Executable Name: %s", szPath);
#endif

    /* To get the path, strip everything off after the last '\' */
    pos = strrchr(szPath, '/');
    if (pos == NULL) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to extract path from: %s", szPath);
        return 1;
    } else {
        /* Clip the path at the position of the last backslash */
        pos[0] = (char)0;
    }

    if (chdir(szPath)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
            "Unable to set working dir to %s : %s", szPath, getLastErrorText());
        return 1;
    }

    return 0;
}

/*******************************************************************************
 * Main function                                                               *
 *******************************************************************************/

int main(int argc, char **argv) {
    int exitStatus;
#ifdef _DEBUG
    int i;
#endif

    /* Initialize the properties variable. */
    properties = NULL;

    /* Make sure all values are reliably set to 0. All required values should also be
     *  set below, but this extra step will protect against future changes.  Some
     *  platforms appear to initialize maloc'd memory to 0 while others do not. */
    wrapperData = malloc(sizeof(WrapperConfig));
    memset(wrapperData, 0, sizeof(WrapperConfig));
    /* Setup the initial values of required properties. */
    wrapperData->configured = FALSE;
    wrapperData->isConsole = TRUE;
    wrapperSetWrapperState(WRAPPER_WSTATE_STARTING);
    wrapperSetJavaState(WRAPPER_JSTATE_DOWN, 0, -1);
    wrapperData->lastPingTicks = wrapperGetTicks();
    wrapperData->jvmCommand = NULL;
    wrapperData->exitRequested = FALSE;
    wrapperData->restartRequested = TRUE; /* The first JVM needs to be started. */
    wrapperData->exitCode = 0;
    wrapperData->jvmRestarts = 0;
    wrapperData->jvmLaunchTicks = wrapperGetTicks();
    wrapperData->failedInvocationCount = 0;
        
    if (wrapperInitializeLogging()) {
        appExit(1);
        return 1; /* For compiler. */
    }

    /* Immediately register this thread with the logger. */
    logRegisterThread(WRAPPER_THREAD_MAIN);

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Wrapper DEBUG build!");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Logging initialized.");
#endif

    if (setWorkingDir(argv[0])) {
        appExit(1);
        return 1; /* For compiler. */
    }
#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Working directory set.");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Arguments:");
    for ( i = 0; i < argc; i++ ) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  argv[%d]=%s", i, argv[i]);
    }
#endif
    
    if (argc < 2) {
        wrapperUsage(argv[0]);
        appExit(1);
        return 1; /* For compiler. */
        
    } else if (strcmp(argv[1],"--help") == 0) {
        wrapperUsage(argv[0]);
        appExit(0);
        return 0; /* For compiler. */
        
    } else {
        /* Store information about the arguments. */
        wrapperData->argBase = 2;
        wrapperData->argCount = argc;
        wrapperData->argValues = argv;
        
        /* Load the properties. */
        if (wrapperLoadConfigurationProperties()) {
            /* Unable to load the configuration.  Any errors will have already
             *  been reported. */
            appExit(1);
            return 1; /* For compiler. */
            
        } else {
            /* Change the working directory if configured to do so. */
            if (wrapperSetWorkingDirProp()) {
                appExit(1);
                return 1; /* For compiler. */
            }

            /* fork to a Daemonized process if configured to do so. */
            if (wrapperData->daemonize) {
                daemonize();
            }

            /* Write pid and anchor files as requested.  If they are the same file the file is
             *  simply overwritten. */
            if (wrapperData->anchorFilename) {
                if (writePidFile(wrapperData->anchorFilename, (int)getpid())) {
                    log_printf
                        (WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                         "ERROR: Could not write anchor file %s: %s",
                         wrapperData->anchorFilename, getLastErrorText());
                    appExit(1);
                    return 1; /* For compiler. */
                }
            }
            if (wrapperData->pidFilename) {
                if (writePidFile(wrapperData->pidFilename, (int)getpid())) {
                    log_printf
                        (WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                         "ERROR: Could not write pid file %s: %s",
                         wrapperData->pidFilename, getLastErrorText());
                    appExit(1);
                    return 1; /* For compiler. */
                }
            }

            exitStatus = wrapperRunConsole();
            
            appExit(exitStatus);
            return exitStatus; /* For compiler. */
        }
    }
}

#endif /* ifndef WIN32 */
