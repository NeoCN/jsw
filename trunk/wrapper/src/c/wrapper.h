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
 * Revision 1.61  2004/10/20 05:23:17  mortenson
 * Add a new property, wrapper.disable_restarts, which will completely disable
 * the Wrapper's ability to restart JVMs.
 *
 * Revision 1.60  2004/10/19 12:13:26  mortenson
 * Fix some compiler warnings on Linux.
 *
 * Revision 1.59  2004/10/19 11:48:20  mortenson
 * Rework logging so that the logfile is kept open.  Results in a 4 fold speed increase.
 *
 * Revision 1.58  2004/10/18 09:37:22  mortenson
 * Add the wrapper.cpu_output and wrapper.cpu_output.interval properties to
 * make it possible to track CPU usage of the Wrapper and JVM over time.
 *
 * Revision 1.57  2004/10/18 05:43:45  mortenson
 * Add the wrapper.memory_output and wrapper.memory_output.interval properties to
 * make it possible to track memory usage of the Wrapper and JVM over time.
 * Change the JVM process variable names to make their meaning more obvious.
 *
 * Revision 1.56  2004/09/22 11:06:28  mortenson
 * Start using nanosleep in place of usleep on UNIX platforms to work around usleep
 * problems with alarm signals on Solaris.
 *
 * Revision 1.55  2004/09/16 07:11:26  mortenson
 * Add a new wrapper.single_invocation property which will prevent multiple
 * invocations of an application from being started on Windows platforms.
 *
 * Revision 1.54  2004/09/06 07:49:16  mortenson
 * Add a new wrapper.loop_output property which makes it possible to enable high
 * resolution debug output on the progress of the main event loop.
 *
 * Revision 1.53  2004/08/31 16:36:10  mortenson
 * Rework the new 64-bit code so that it is done with only 32 bit variables.  A little
 * more complicated but it fixes compiler warnings on unix systems.
 *
 * Revision 1.52  2004/08/06 16:17:04  mortenson
 * Added a new wrapper.java.command.loglevel property which makes it possible
 * to control the log level of the generated java command.
 *
 * Revision 1.51  2004/07/05 07:43:54  mortenson
 * Fix a deadlock on solaris by being very careful that we never perform any direct
 * logging from within a signal handler.
 *
 * Revision 1.50  2004/06/16 15:56:29  mortenson
 * Added a new property, wrapper.anchorfile, which makes it possible to
 * cause the Wrapper to shutdown by deleting an anchor file.
 *
 * Revision 1.49  2004/06/14 07:20:40  mortenson
 * Add some additional output and a wrapper.timer_output property to help with
 * debugging timer issues.
 *
 * Revision 1.48  2004/06/06 15:28:18  mortenson
 * Fix a synchronization problem in the logging code which would
 * occassionally cause the Wrapper to crash with an Access Violation.
 * The problem was only encountered when the tick timer was enabled,
 * and was only seen on multi-CPU systems.  Bug #949877.
 *
 * Revision 1.47  2004/04/08 14:58:59  mortenson
 * Add a wrapper.working.dir property.
 *
 * Revision 1.46  2004/04/08 03:21:57  mortenson
 * Added an environment variable, WRAPPER_PATH_SEPARATOR, whose value is set
 * to either ':' or ';' on startup.
 *
 * Revision 1.45  2004/03/27 16:09:45  mortenson
 * Add wrapper.on_exit.<n> properties to control what happens when a exits based
 * on the exit code.  This led to a major rework of the state engine to make it possible.
 *
 * Revision 1.44  2004/03/26 03:18:00  mortenson
 * Add the wrapper.startup.delay property along with console and service
 * specific variants which make it possible to configure a delay between the
 * Wrapper being launched and the first JVM being launched.
 *
 * Revision 1.43  2004/03/20 16:55:49  mortenson
 * Add an adviser feature to help cut down on support requests from new users.
 *
 * Revision 1.42  2004/03/18 04:54:47  mortenson
 * Add a new wrapper.java.library.path.append_system_path property which will
 * cause the Wrapper to append the system path to the generated library path.
 *
 * Revision 1.41  2004/01/16 04:41:59  mortenson
 * The license was revised for this version to include a copyright omission.
 * This change is to be retroactively applied to all versions of the Java
 * Service Wrapper starting with version 3.0.0.
 *
 * Revision 1.40  2004/01/09 19:45:03  mortenson
 * Implement the tick timer on Linux.
 *
 * Revision 1.39  2004/01/09 18:31:36  mortenson
 * define the DWORD symbol so it can used.
 *
 * Revision 1.38  2004/01/09 05:15:11  mortenson
 * Implement a tick timer and convert the system time over to be compatible.
 *
 * Revision 1.37  2003/10/31 03:57:17  mortenson
 * Add a new property, wrapper.console.title, which makes it possible to set
 * the title of the console in which the Wrapper is currently running.
 *
 * Revision 1.36  2003/10/30 19:34:34  mortenson
 * Added a new wrapper.ntservice.console property so the console can be shown for
 * services.
 * Fixed a problem where requesting thread dumps on exit was failing when running
 * as a service.
 *
 * Revision 1.35  2003/10/12 18:59:06  mortenson
 * Add a new property, wrapper.native_library, which can be used to specify
 * the base name of the native library.
 *
 * Revision 1.34  2003/09/04 05:40:08  mortenson
 * Added a new wrapper.ping.interval property which lets users control the
 * frequency that the Wrapper pings the JVM.
 *
 * Revision 1.33  2003/09/03 02:33:38  mortenson
 * Requested restarts no longer reset the restart count.
 * Add new wrapper.ignore_signals property.
 *
 * Revision 1.32  2003/08/15 16:30:51  mortenson
 * Added support for the wrapper.pidfile property on the Windows platform.
 *
 * Revision 1.31  2003/07/02 04:01:52  mortenson
 * Implement the ability to specify an NT service's load order group in response
 * to feature request #764143.
 *
 * Revision 1.30  2003/06/10 14:22:00  mortenson
 * Fix bug #744801.  A Java GUI was not being displayed when the application was
 * run in either console mode or as a service with wrapper.ntservice.interactive
 * enabled on JVM versions prior to 1.4.0.
 *
 * Revision 1.29  2003/04/14 14:11:53  mortenson
 * Add support for Mac OS X.
 * (Patch from Andy Barnett)
 *
 * Revision 1.28  2003/04/03 04:05:22  mortenson
 * Fix several typos in the docs.  Thanks to Mike Castle.
 *
 * Revision 1.27  2003/04/02 10:05:53  mortenson
 * Modified the wrapper.ping.timeout property so it also controls the ping
 * timeout within the JVM.  Before the timeout on responses to the Wrapper
 * could be controlled, but the ping timeout within the JVM was hardcoded to
 * 30 seconds.
 *
 * Revision 1.26  2003/03/21 21:25:31  mortenson
 * Fix a problem where very heavy output from the JVM can cause the Wrapper to
 * give a false timeout.  The Wrapper now only ready 50 lines of input at a time
 * to guarantee that the Wrapper's event loop always gets cycles.
 *
 * Revision 1.25  2003/02/07 16:05:28  mortenson
 * Implemented feature request #676599 to enable the filtering of JVM output to
 * trigger JVM restarts or Wrapper shutdowns.
 *
 * Revision 1.24  2003/02/03 06:55:27  mortenson
 * License transfer to TanukiSoftware.org
 *
 */

#ifndef _WRAPPER_H
#define _WRAPPER_H

#ifdef WIN32
#include <winsock.h>

#else /* UNIX */
#include <time.h>
#ifndef MACOSX
#define u_short unsigned short
#endif /* MACOSX */

#endif

#ifndef DWORD
#define DWORD unsigned long
#endif

#include "property.h"

#define WRAPPER_TICK_MS 100 /* The number of ms that are represented by a single
                             *  tick.  Ticks are used as an alternative time
                             *  keeping method. See the wrapperGetTicks() and
                             *  wrapperGetTickAge() functions for more information.
                             * Some code assumes that this number can be evenly
                             *  divided into 1000. */

#define WRAPPER_TIMER_FAST_THRESHOLD 2 * 24 * 3600 * 1000 / WRAPPER_TICK_MS /* Default to 2 days. */
#define WRAPPER_TIMER_SLOW_THRESHOLD 2 * 24 * 3600 * 1000 / WRAPPER_TICK_MS /* Default to 2 days. */

#define WRAPPER_WSTATE_STARTING  51 /* Wrapper is starting.  Remains in this state
                                     *  until the JVM enters the STARTED state or
                                     *  the wrapper jumps into the STOPPING state
                                     *  in response to the JVM application asking
                                     *  to shut down. */
#define WRAPPER_WSTATE_STARTED   52 /* The JVM has entered the STARTED state.
                                     *  The wrapper will remain in this state
                                     *  until the wrapper decides to shut down.
                                     *  This is true even when the JVM process
                                     *  is being restarted. */
#define WRAPPER_WSTATE_STOPPING  53 /* The wrapper is shutting down.  Will be in
                                     *  this state until the JVM enters the DOWN
                                     *  state. */
#define WRAPPER_WSTATE_STOPPED   54 /* The wrapper enters this state just before
                                     *  it exits. */


#define WRAPPER_JSTATE_DOWN      71 /* JVM is confirmed to be down.  This is the 
                                     *  initial state and the state after the JVM
                                     *  process has gone away. */
#define WRAPPER_JSTATE_LAUNCH    72 /* Set from the DOWN state to launch a JVM.  The
                                     *  timeout will be the time to actually launch
                                     *  the JVM after any required delay. */
#define WRAPPER_JSTATE_LAUNCHING 73 /* JVM was launched, but has not yet responded.
                                     *  Must enter the LAUNCHED state before <t>
                                     *  or the JVM will be killed. */
#define WRAPPER_JSTATE_LAUNCHED  74 /* JVM was launched, and responed to a ping. */
#define WRAPPER_JSTATE_STARTING  75 /* JVM has been asked to start.  Must enter the
                                     *  STARTED state before <t> or the JVM will be
                                     *  killed. */
#define WRAPPER_JSTATE_STARTED   76 /* JVM has responded that it is running.  Must
                                     *  respond to a ping by <t> or the JVM will
                                     *  be killed. */
#define WRAPPER_JSTATE_STOPPING  77 /* JVM was sent a stop command, but has not yet
                                     *  responded.  Must enter the STOPPED state
                                     *  and exit before <t> or the JVM will be killed. */
#define WRAPPER_JSTATE_STOPPED   78 /* JVM has responed that it is stopped. */
#define WRAPPER_JSTATE_KILLING   79 /* The Wrapper is about ready to kill the JVM
                                     *  process but it must wait a few moments before
                                     *  actually doing so.  After <t> has expired, the
                                     *  JVM will be killed and we will enter the STOPPED
                                     *  state. */

#define FILTER_ACTION_NONE       90
#define FILTER_ACTION_RESTART    91
#define FILTER_ACTION_SHUTDOWN   92

#define WRAPPER_TIMEOUT_MAX      31557600 /* One Year.  Effectively never. */

/* Type definitions */
typedef struct WrapperConfig WrapperConfig;
struct WrapperConfig {
    int     configured;             /* TRUE if loadConfiguration has been called. */
    int     useSystemTime;          /* TRUE if the wrapper should use the system clock for timing, FALSE if a tick counter should be used. */
    int     timerFastThreshold;     /* If the difference between the system time based tick count and the timer tick count ever falls by more than this value then a warning will be displayed. */
    int     timerSlowThreshold;     /* If the difference between the system time based tick count and the timer tick count ever grows by more than this value then a warning will be displayed. */

    u_short port;                   /* Port number which the Wrapper is configured to be listening on */
    u_short actualPort;             /* Port number which the Wrapper is actually listening on */
    int     sock;                   /* Socket number. if open. */
    char    *configFile;            /* Name of the configuration file */
    int     commandLogLevel;        /* The log level to use when logging the java command. */
#ifdef WIN32
    char    *jvmCommand;            /* Command used to launch the JVM */
#else /* UNIX */
    char    **jvmCommand;           /* Command used to launch the JVM */
#endif
    char    key[17];                /* Key which the JVM uses to authorize connections. (16 digits + \0) */
    int     isConsole;              /* TRUE if the wrapper was launched as a console. */
    int     cpuTimeout;             /* Number of seconds without CPU before the JVM will issue a warning and extend timeouts */
    int     startupTimeout;         /* Number of seconds the wrapper will wait for a JVM to startup */
    int     pingTimeout;            /* Number of seconds the wrapper will wait for a JVM to reply to a ping */
    int     pingInterval;           /* Number of seconds between pinging the JVM */
    int     shutdownTimeout;        /* Number of seconds the wrapper will wait for a JVM to shutdown */
    int     jvmExitTimeout;         /* Number of seconds the wrapper will wait for a JVM to process to terminate */

    int     wState;                 /* The current state of the wrapper */
    int     jState;                 /* The current state of the jvm */
    DWORD   jStateTimeoutTicks;     /* Tick count until which the current jState is valid */
    int     jStateTimeoutTicksSet;  /* 1 if the current jStateTimeoutTicks is set. */
    DWORD   lastPingTicks;          /* Time that the last ping was sent */

    int     isDebugging;            /* TRUE if set in the configuration file */
    int     isAdviserEnabled;       /* TRUE if advice messages should be output. */
    char    *nativeLibrary;         /* The base name of the native library loaded by the WrapperManager. */
    int     libraryPathAppendPath;  /* TRUE if the PATH environment variable should be appended to the java library path. */
    int     isStateOutputEnabled;   /* TRUE if set in the configuration file.  Shows output on the state of the state engine. */
    int     isTimerOutputEnabled;   /* TRUE if detailed timer output should be included in debug output. */
    int     isLoopOutputEnabled;    /* TRUE if very detailed output from the main loop should be output. */
    int     isSleepOutputEnabled;   /* TRUE if detailed sleep output should be included in debug output. */
    int     isMemoryOutputEnabled;  /* TRUE if detailed memory output should be included in status output. */
    int     memoryOutputInterval;   /* Interval in seconds at which memory usage is logged. */
    DWORD   memoryOutputTimeoutTicks; /* Tick count at which memory will next be logged. */
    int     isCPUOutputEnabled;     /* TRUE if detailed CPU output should be included in status output. */
    int     cpuOutputInterval;      /* Interval in seconds at which CPU usage is logged. */
    DWORD   cpuOutputTimeoutTicks;  /* Tick count at which CPU will next be logged. */
    int     logfileInactivityTimeout; /* The number of seconds of inactivity before the logfile will be closed. */
    DWORD   logfileInactivityTimeoutTicks; /* Tick count at which the logfile will be considered inactive and closed. */
    int     isShutdownHookDisabled; /* TRUE if set in the configuration file */
    int     startupDelayConsole;    /* Delay in seconds before starting the first JVM in console mode. */
    int     startupDelayService;    /* Delay in seconds before starting the first JVM in service mode. */
    int     exitCode;               /* Code which the wrapper will exit with */
    int     exitRequested;          /* TRUE if the current JVM should be shutdown. */
    int     restartRequested;       /* TRUE if the another JVM should be launched after the current JVM is shutdown. Only set if exitRequested is set. */
    int     jvmRestarts;            /* Number of times that a JVM has been launched since the wrapper was started. */
    int     restartDelay;           /* Delay in seconds before restarting a new JVM. */
    int     isRestartDisabled;      /* TRUE if restarts should be disabled. */
    int     requestThreadDumpOnFailedJVMExit; /* TRUE if the JVM should be asked to dump its state when it fails to halt on request. */
    DWORD   jvmLaunchTicks;         /* The tick count at which the previous or current JVM was launched. */
    int     failedInvocationCount;  /* The number of times that the JVM exited in less than successfulInvocationTime in a row. */
    int     successfulInvocationTime;/* Amount of time that a new JVM must be running so that the invocation will be considered to have been a success, leading to a reset of the restart count. */
    int     maxFailedInvocations;   /* Maximum number of failed invocations in a row before the Wrapper will give up and exit. */
    int     outputFilterCount;      /* Number of registered output filters. */
    char**  outputFilters;          /* Array of output filters. */
    int*    outputFilterActions;    /* Array of output filter actions. */
    char    *pidFilename;           /* Name of file to store wrapper pid in */
    char    *javaPidFilename;       /* Name of file to store jvm pid in */
    char    *anchorFilename;        /* Name of an anchor file used to control when the Wrapper should quit. */
    int     anchorPollInterval;     /* Interval in seconds at which the existence of the anchor file is polled. */
    DWORD   anchorTimeoutTicks;     /* Tick count at which the anchor file will be checked next. */
    int     ignoreSignals;          /* True if the Wrapper should ignore any catchable system signals and inform its JVM to do the same. */

#ifdef WIN32
    char    *consoleTitle;          /* Text to set the console title to. */
    int     isSingleInvocation;     /* TRUE if only a single invocation of an application should be allowed to launch. */
    char    *ntServiceName;         /* Name of the NT Service */
    char    *ntServiceDisplayName;  /* Display name of the NT Service */
    char    *ntServiceDescription;  /* Description for service in Win2k and XP */
    char    *ntServiceLoadOrderGroup; /* Load order group name. */
    char    *ntServiceDependencies; /* List of Dependencies */
    int     ntServiceStartType;     /* Mode in which the Service is installed. 
                                     * {SERVICE_AUTO_START | SERVICE_DEMAND_START} */
    DWORD   ntServicePriorityClass; /* Priority at which the Wrapper and its JVMS will run.
                                     * {HIGH_PRIORITY_CLASS | IDLE_PRIORITY_CLASS | NORMAL_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS} */
    char    *ntServiceAccount;      /* Account name to use when running as a service.  NULL to use the LocalSystem account. */
    char    *ntServicePassword;     /* Password to use when running as a service.  NULL means no password. */
    int     ntServiceInteractive;   /* Should the service be allowed to interact with the desktop? */
    int     ntHideJVMConsole;       /* Should the JVMs Console window be hidden when run as a service.  True by default but GUIs will not be visible for JVMs prior to 1.4.0. */
    int     ntHideWrapperConsole;   /* Should the Wrapper Console window be hidden when run as a service. */
    HWND    wrapperConsoleHandle;   /* Pointer to the Wrapper Console handle if it exists.  This will only be set if the console was allocated then hidden. */
    int     ntAllocConsole;         /* True if a console should be allocated for the Service. */
#else /* UNIX */
    int     daemonize;              /* TRUE if the process  should be spawned as a daemon process on launch. */
#endif
};

#define WRAPPER_MSG_START         (char)100
#define WRAPPER_MSG_STOP          (char)101
#define WRAPPER_MSG_RESTART       (char)102
#define WRAPPER_MSG_PING          (char)103
#define WRAPPER_MSG_STOP_PENDING  (char)104
#define WRAPPER_MSG_START_PENDING (char)105
#define WRAPPER_MSG_STARTED       (char)106
#define WRAPPER_MSG_STOPPED       (char)107
#define WRAPPER_MSG_KEY           (char)110
#define WRAPPER_MSG_BADKEY        (char)111
#define WRAPPER_MSG_LOW_LOG_LEVEL (char)112
#define WRAPPER_MSG_PING_TIMEOUT  (char)113

/** Log commands are actually 116 + the LOG LEVEL. */
#define WRAPPER_MSG_LOG           (char)116

#define WRAPPER_PROCESS_DOWN      200
#define WRAPPER_PROCESS_UP        201

extern WrapperConfig *wrapperData;
extern Properties    *properties;

extern char wrapperClasspathSeparator;

/* Protocol Functions */
extern void wrapperProtocolStartServer();
extern void wrapperProtocolStopServer();
extern void wrapperProtocolOpen();
extern void wrapperProtocolClose();
extern int wrapperProtocolFunction(char function, const char *message);
extern int wrapperProtocolRead();

/******************************************************************************
 * Utility Functions
 *****************************************************************************/
void wrapperAddDefaultProperties();

#ifdef WIN32
extern char** wrapperGetSystemPath();
#endif

extern int wrapperCheckRestartTimeOK();

/**
 * command is a pointer to a pointer of an array of character strings.
 * length is the number of strings in the above array.
 */
extern void wrapperBuildJavaCommandArray(char ***strings, int *length, int addQuotes);
extern void wrapperFreeJavaCommandArray(char **strings, int length);

extern int wrapperInitializeLogging();

/**
 * Called when the Wrapper detects that the JVM process has exited.
 *  Contains code common to all platforms.
 */
extern void wrapperJVMProcessExited(int exitCode);

/**
 * Logs a single line of child output allowing any filtering
 *  to be done in a common location.
 */
extern void wrapperLogChildOutput(const char* log);

/******************************************************************************
 * Platform specific methods
 *****************************************************************************/

/**
 * Gets the error code for the last operation that failed.
 */
extern int wrapperGetLastError();

/**
 * Execute initialization code to get the wrapper set up.
 */
extern int wrapperInitialize();

/**
 * Cause the current thread to sleep for the specified number of milliseconds.
 *  Sleeps over one second are not allowed.
 */
extern void wrapperSleep(int ms);

/**
 * Reports the status of the wrapper to the service manager
 * Possible status values:
 *   WRAPPER_WSTATE_STARTING
 *   WRAPPER_WSTATE_STARTED
 *   WRAPPER_WSTATE_STOPPING
 *   WRAPPER_WSTATE_STOPPED
 */
extern void wrapperReportStatus(int useLoggerQueue, int status, int errorCode, int waitHint);

/**
 * Read and process any output from the child JVM Process.
 * Most output should be logged to the wrapper log file.
 */
extern int wrapperReadChildOutput();

/**
 * Checks on the status of the JVM Process.
 * Returns WRAPPER_PROCESS_UP or WRAPPER_PROCESS_DOWN
 */
extern int wrapperGetProcessStatus();

/**
 * Immediately kill the JVM process and set the JVM state to
 *  WRAPPER_JSTATE_DOWN.
 */
extern void wrapperKillProcessNow();

/**
 * Puts the Wrapper into a state where the JVM will be killed at the soonest
 *  possible oportunity.  It is necessary to wait a moment if a final thread
 *  dump is to be requested.  This call wll always set the JVM state to
 *  WRAPPER_JSTATE_KILLING.
 */
extern void wrapperKillProcess(int useLoggerQueue);

/**
 * Pauses before launching a new JVM if necessary.
 */
extern void wrapperPauseBeforeExecute();

/**
 * Launches a JVM process and store it internally
 */
extern void wrapperExecute();

/**
 * Returns a tick count that can be used in combination with the
 *  wrapperGetTickAge() function to perform time keeping.
 */
extern DWORD wrapperGetTicks();

/**
 * Outputs a log entry at regular intervals to track the memory usage of the
 *  Wrapper and its JVM.
 */
extern void wrapperDumpMemory();

/**
 * Outputs a log entry at regular intervals to track the CPU usage over each
 *  interval for the Wrapper and its JVM.
 */
extern void wrapperDumpCPUUsage();

/******************************************************************************
 * Wrapper inner methods.
 *****************************************************************************/
/**
 * Launch the wrapper as a console application.
 */
extern int wrapperRunConsole();

/**
 * Launch the wrapper as a service application.
 */
extern int wrapperRunService();

/**
 * Used to ask the state engine to shut down the JVM and Wrapper
 */
extern void wrapperStopProcess(int useLoggerQueue, int exitCode);

/**
 * Used to ask the state engine to shut down the JVM.
 */
extern void wrapperRestartProcess();

/**
 * The main event loop for the wrapper.  Handles all state changes and events.
 */
extern void wrapperEventLoop();

extern void wrapperBuildKey();
extern void wrapperBuildJavaCommand();
extern int  wrapperLoadConfiguration();

/**
 * Calculates a tick count using the system time.
 */
extern DWORD wrapperGetSystemTicks();

/**
 * Returns difference in seconds between the start and end ticks.  This function
 *  handles cases where the tick counter has wrapped between when the start
 *  and end tick counts were taken.  See the wrapperGetTicks() function.
 */
extern int wrapperGetTickAge(DWORD start, DWORD end);

/**
 * Returns TRUE if the specified tick timeout has expired relative to the
 *  specified tick count.
 */
extern int wrapperTickExpired(DWORD nowTicks, DWORD timeoutTicks);

/**
 * Returns a tick count that is the specified number of seconds later than
 *  the base tick count.
 */
extern DWORD wrapperAddToTicks(DWORD start, int seconds);

/**
 * Sets the working directory of the Wrapper to the specified directory.
 *  The directory can be relative or absolute.
 * If there are any problems then a non-zero value will be returned.
 */
extern int wrapperSetWorkingDir(const char* dir);

/**
 * Sets the working directory using the value of the wrapper.working.dir
 *  property.  If it is not set then the directory will not be changed.
 * If there are any problems then a non-zero value will be returned.
 */
extern int wrapperSetWorkingDirProp();

/******************************************************************************
 * Protocol callback functions
 *****************************************************************************/
extern void wrapperLogSignalled(int logLevel, char *msg);
extern void wrapperKeyRegistered(char *key);
extern void wrapperPingResponded();
extern void wrapperStopRequested(int exitCode);
extern void wrapperRestartRequested();
extern void wrapperStopPendingSignalled(int waitHint);
extern void wrapperStoppedSignalled();
extern void wrapperStartPendingSignalled(int waitHint);
extern void wrapperStartedSignalled();

#endif
