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
 *
 *
 * $Log$
 * Revision 1.11  2002/05/16 04:51:18  mortenson
 * Add a debug message stating which thread lead to System.exit being called
 *   via a call to shutdown.
 *
 * Revision 1.10  2002/05/07 05:46:36  mortenson
 * Add the ability to set the priority at which the wrapper is run under NT systems.
 *
 * Revision 1.9  2002/03/07 10:05:48  rybesh
 * fixed some miscellaneous unix compile errors
 *
 * Revision 1.8  2002/03/07 09:23:25  mortenson
 * Go through and change the style of comments that we use so that they will not
 * cause compiler errors on older unix compilers.
 *
 * Revision 1.7  2002/03/07 08:10:13  mortenson
 * Add support for Thread Dumping
 * Fix a problem locating java on the path.
 *
 * Revision 1.6  2002/01/27 19:33:55  spocke
 * Added support for the service description property.
 *
 * Revision 1.5  2002/01/24 09:43:56  mortenson
 * Added new Logger code which allows log levels.
 *
 * Revision 1.4  2002/01/24 05:48:48  mortenson
 * Remove old commented code.
 *
 * Revision 1.3  2001/12/11 05:19:39  mortenson
 * Added the ablility to format and/or disable file logging and output to
 * the console.
 *
 * Revision 1.2  2001/12/06 09:36:24  mortenson
 * Docs changes, Added sample apps, Fixed some problems with
 * relative paths  (See revisions.txt)
 *
 * Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
 * no message
 *
 */

#ifndef _WRAPPER_H
#define _WRAPPER_H

#ifdef WIN32
#include <winsock.h>
#else
#include <time.h>
#define u_short unsigned short
#endif

#include "property.h"

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
#define WRAPPER_JSTATE_LAUNCHING 72 /* JVM was launched, but has not yet responded.
                                     *  Must enter the LAUNCHED state before <t>
                                     *  or the JVM will be killed. */
#define WRAPPER_JSTATE_LAUNCHED  73 /* JVM was launched, and responed to a ping. */
#define WRAPPER_JSTATE_STARTING  74 /* JVM has been asked to start.  Must enter the
                                     *  STARTED state before <t> or the JVM will be
                                     *  killed. */
#define WRAPPER_JSTATE_STARTED   75 /* JVM has responded that it is running.  Must
                                     *  respond to a ping by <t> or the JVM will
                                     *  be killed. */
#define WRAPPER_JSTATE_STOPPING  76 /* JVM was sent a stop command, but has not yet
                                     *  responded.  Must enter the STOPPED state
                                     *  and exit before <t> or the JVM will be killed. */
#define WRAPPER_JSTATE_STOPPED   77 /* JVM has responed that it is stopped. */



/* Type definitions */
typedef struct WrapperConfig WrapperConfig;
struct WrapperConfig {
    u_short port;                   /* Port number which the Wrapper is configured to be listening on */
    u_short actualPort;             /* Port number which the Wrapper is actually listening on */
    int     sock;                   /* Socket number. if open. */
    char    *configFile;            /* Name of the config file */
#ifdef WIN32
    char    *jvmCommand;            /* Command used to launch the JVM */
#else /* UNIX */
    char    **jvmCommand;           /* Command used to launch the JVM */
#endif
    char    key[17];                /* Key which the JVM uses to authorize connections. (16 digits + \0) */
    int     isConsole;              /* TRUE if the wrapper was launched as a console. */
    int     startupTimeout;         /* Number of seconds the wrapper will wait for a JVM to startup */
    int     pingTimeout;            /* Number of seconds the wrapper will wait for a JVM to reply to a ping */
    int     wState;                 /* The current state of the wrapper */
    int     jState;                 /* The current state of the jvm */
    time_t  jStateTimeout;          /* Time until which the current jState is valid */
    time_t  lastPingTime;           /* Time that the last ping was sent */
    int     isDebugging;            /* TRUE if set in the config file */
    int     isShutdownHookDisabled; /* TRUE if set in the config file */
    int     exitCode;               /* Code which the wrapper will exit with */
    int     exitRequested;          /* Non-zero if another thread has requested that the wrapper and JVM be shutdown */
    int     exitAcknowledged;       /* Non-zero if the main thread has acknowledged the exit request */
    int     restartRequested;       /* Non-zero if another thread has requested that the JVM be restarted */
    int     jvmRestarts;            /* Number of times that a JVM has been launched since the wrapper was started. */
	int     requestThreadDumpOnFailedJVMExit; /* TRUE if the JVM should be asked to dump its state when it fails to halt on request. */

#ifdef WIN32
    char    *ntServiceName;         /* Name of the NT Service */
    char    *ntServiceDisplayName;  /* Display name of the NT Service */
    char    *ntServiceDescription;  /* Description for service in Win2k and XP */
    char    *ntServiceDependencies; /* List of Dependencies */
    int     ntServiceStartType;     /* Mode in which the Service is installed. 
                                     * {SERVICE_AUTO_START | SERVICE_DEMAND_START} */
	DWORD   ntServicePriorityClass; /* Priority at which the Wrapper and its JVMS will run.
									 * {HIGH_PRIORITY_CLASS | IDLE_PRIORITY_CLASS | NORMAL_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS} */
#endif

#ifdef SOLARIS
    char    *pidFilename;           /* Name of file to store wrapper pid in */
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
 * Execute clean up code in preparation for shutdown
 */
extern void wrapperCleanup();

/**
 * Reports the status of the wrapper to the service manager
 * Possible status values:
 *   WRAPPER_WSTATE_STARTING
 *   WRAPPER_WSTATE_STARTED
 *   WRAPPER_WSTATE_STOPPING
 *   WRAPPER_WSTATE_STOPPED
 */
extern void wrapperReportStatus(int status, int errorCode, int waitHint);

/**
 * Read and process any output from the child JVM Process.
 * Most output should be logged to the wrapper log file.
 */
extern void wrapperReadChildOutput();

/**
 * Checks on the status of the JVM Process.
 * Returns WRAPPER_PROCESS_UP or WRAPPER_PROCESS_DOWN
 */
extern int wrapperGetProcessStatus();

/**
 * Kill the JVM Process immediately and set the JVM State to
 *  WRAPPER_JSTATE_DOWN
 */
extern void wrapperKillProcess();

/**
 * Pauses before launching a new JVM if necessary.
 */
extern void wrapperPauseBeforeExecute();

/**
 * Launches a JVM process and store it internally
 */
extern void wrapperExecute();

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
extern void wrapperStopProcess(int exitCode);

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

/******************************************************************************
 * Protocol callback functions
 *****************************************************************************/
extern void wrapperKeyRegistered(char *key);
extern void wrapperPingResponded();
extern void wrapperStopRequested(int exitCode);
extern void wrapperRestartRequested();
extern void wrapperStopPendingSignalled(int waitHint);
extern void wrapperStoppedSignalled();
extern void wrapperStartPendingSignalled(int waitHint);
extern void wrapperStartedSignalled();

#endif
