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
 * Revision 1.13  2004/10/18 05:43:45  mortenson
 * Add the wrapper.memory_output and wrapper.memory_output.interval properties to
 * make it possible to track memory usage of the Wrapper and JVM over time.
 * Change the JVM process variable names to make their meaning more obvious.
 *
 * Revision 1.12  2004/09/22 11:06:28  mortenson
 * Start using nanosleep in place of usleep on UNIX platforms to work around usleep
 * problems with alarm signals on Solaris.
 *
 * Revision 1.11  2004/09/09 13:20:28  mortenson
 * Add more low level loop debug output.
 *
 * Revision 1.10  2004/09/06 07:49:16  mortenson
 * Add a new wrapper.loop_output property which makes it possible to enable high
 * resolution debug output on the progress of the main event loop.
 *
 * Revision 1.9  2004/08/31 16:36:10  mortenson
 * Rework the new 64-bit code so that it is done with only 32 bit variables.  A little
 * more complicated but it fixes compiler warnings on unix systems.
 *
 * Revision 1.8  2004/08/31 15:58:33  mortenson
 * Add 64 bit UNIX code
 *
 * Revision 1.7  2004/08/31 15:13:48  mortenson
 * Add additional log output when timer output is enabled to show when tick
 * overflows will take place.
 *
 * Revision 1.6  2004/07/05 08:42:38  mortenson
 * Call maintainLogger a second time to make sure that any queued messages are
 * displayed before any state changes to avoid confusing output.
 *
 * Revision 1.5  2004/07/05 07:43:54  mortenson
 * Fix a deadlock on solaris by being very careful that we never perform any direct
 * logging from within a signal handler.
 *
 * Revision 1.4  2004/06/22 02:25:22  mortenson
 * Modify the timing of a low level debug message related to the anchor file.
 *
 * Revision 1.3  2004/06/16 15:56:29  mortenson
 * Added a new property, wrapper.anchorfile, which makes it possible to
 * cause the Wrapper to shutdown by deleting an anchor file.
 *
 * Revision 1.2  2004/03/27 16:50:15  mortenson
 * Get the latest changes to compile on UNIX platforms.
 *
 * Revision 1.1  2004/03/27 16:09:48  mortenson
 * Add wrapper.on_exit.<n> properties to control what happens when a exits based
 * on the exit code.  This led to a major rework of the state engine to make it possible.
 *
 */

/**
 * This file contains the main event loop and state engine for
 *  the Java Service Wrapper.
 *
 * Author:
 *   Leif Mortenson <leif@tanukisoftware.com>
 *   Ryan Shaw
 *
 * Version CVS $Revision$ $Date$
 */

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>

#ifdef WIN32
#else /* UNIX */
#include <unistd.h>
#endif

#include "wrapper.h"
#include "logger.h"

const char *wrapperGetWState(int wState) {
    const char *name;
    switch(wState) {
    case WRAPPER_WSTATE_STARTING:
        name = "STARTING";
        break;
    case WRAPPER_WSTATE_STARTED:
        name = "STARTED";
        break;
    case WRAPPER_WSTATE_STOPPING:
        name = "STOPPING";
        break;
    case WRAPPER_WSTATE_STOPPED:
        name = "STOPPED";
        break;
    default:
        name = "UNKNOWN";
        break;
    }
    return name;
}

const char *wrapperGetJState(int jState) {
    const char *name;
    switch(jState) {
    case WRAPPER_JSTATE_DOWN:
        name = "DOWN";
        break;
    case WRAPPER_JSTATE_LAUNCH:
        name = "LAUNCH(DELAY)";
        break;
    case WRAPPER_JSTATE_LAUNCHING:
        name = "LAUNCHING";
        break;
    case WRAPPER_JSTATE_LAUNCHED:
        name = "LAUNCHED";
        break;
    case WRAPPER_JSTATE_STARTING:
        name = "STARTING";
        break;
    case WRAPPER_JSTATE_STARTED:
        name = "STARTED";
        break;
    case WRAPPER_JSTATE_STOPPING:
        name = "STOPPING";
        break;
    case WRAPPER_JSTATE_STOPPED:
        name = "STOPPED";
        break;
    case WRAPPER_JSTATE_KILLING:
        name = "KILLING";
        break;
    default:
        name = "UNKNOWN";
        break;
    }
    return name;
}

void displayLaunchingTimeoutMessage() {
    const char *mainClass;

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
        "Startup failed: Timed out waiting for a signal from the JVM.");

    mainClass = getStringProperty(properties, "wrapper.java.mainclass", "Main");

    if ((strstr(mainClass, "org.tanukisoftware.wrapper.WrapperSimpleApp") != NULL)
        || (strstr(mainClass, "org.tanukisoftware.wrapper.WrapperStartStopApp") != NULL)
        || (strstr(mainClass, "com.silveregg.wrapper.WrapperSimpleApp") != NULL)
        || (strstr(mainClass, "com.silveregg.wrapper.WrapperStartStopApp") != NULL)) {

        /* The user appears to be using a valid main class, so no advice available. */
    } else {
        if (wrapperData->isAdviserEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, "" );
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "------------------------------------------------------------------------" );
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "Advice:" );
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "The Wrapper consists of a native component as well as a set of classes");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "which run within the JVM that it launches.  The Java component of the");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "Wrapper must be initialized promptly after the JVM is launched or the");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "Wrapper will timeout, as just happened.  Most likely the main class");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "specified in the Wrapper configuration file is not correctly initializing");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "the Wrapper classes:");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "    %s", mainClass);
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "While it is possible to do so manually, the Wrapper ships with helper");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "classes to make this initialization processes automatic.");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "Please review the integration section of the Wrapper's documentation");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "for the various methods which can be employed to launch an application");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "within the Wrapper:");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "    http://wrapper.tanukisoftware.org/doc/english/integrate.html");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                "------------------------------------------------------------------------" );
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, "" );
        }
    }
}

/**
 * Tests for the existence of the anchor file.  If it does not exist then
 *  the Wrapper will begin its shutdown process.
 *
 * nowTicks: The tick counter value this time through the event loop.
 */
void anchorPoll(DWORD nowTicks) {
    struct stat fileStat;
    int result;

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, 
        "Anchor timeout=%d, now=%d", wrapperData->anchorTimeoutTicks, nowTicks);
#endif

    if (wrapperData->anchorFilename) {
        if (wrapperData->isLoopOutputEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Loop: check anchor file");
        }
        
        if (wrapperTickExpired(nowTicks, wrapperData->anchorTimeoutTicks)) {
            result = stat(wrapperData->anchorFilename, &fileStat);
            if (result == 0) {
                /* Anchor file exists.  Do nothing. */
#ifdef _DEBUG
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO,
                    "The anchor file %s exists.", wrapperData->anchorFilename);
#endif
            } else {
                /* Anchor file is gone. */
#ifdef _DEBUG
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO,
                    "The anchor file %s was deleted.", wrapperData->anchorFilename);
#endif

                /* Unless we are already doing so, start the shudown process. */
                if (wrapperData->exitRequested || wrapperData->restartRequested ||
                    (wrapperData->jState == WRAPPER_JSTATE_STOPPING) ||
                    (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
                    (wrapperData->jState == WRAPPER_JSTATE_KILLING) ||
                    (wrapperData->jState == WRAPPER_JSTATE_DOWN)) {
                    /* Already shutting down, so nothing more to do. */
                } else {
                    /* Start the shutdown process. */
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Anchor file deleted.  Shutting down.");

                    wrapperStopProcess(FALSE, 0);

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

            wrapperData->anchorTimeoutTicks = wrapperAddToTicks(nowTicks, wrapperData->anchorPollInterval);
        }
    }
}

/********************************************************************
 * Wrapper States
 *******************************************************************/
/**
 * WRAPPER_WSTATE_STARTING
 * The Wrapper process is being started.  It will remain in this state
 *  until a JVM and its application has been successfully started.
 *
 * nowTicks: The tick counter value this time through the event loop.
 */
void wStateStarting(DWORD nowTicks) {
    /* While the wrapper is starting up, we need to ping the service  */
    /*  manager to reasure it that we are still alive. */

    /* Tell the service manager that we are starting */
    wrapperReportStatus(FALSE, WRAPPER_WSTATE_STARTING, 0, 1000);
    
    /* If the JVM state is now STARTED, then change the wrapper state */
    /*  to be STARTED as well. */
    if (wrapperData->jState == WRAPPER_JSTATE_STARTED) {
        wrapperData->wState = WRAPPER_WSTATE_STARTED;
        
        /* Tell the service manager that we started */
        wrapperReportStatus(FALSE, WRAPPER_WSTATE_STARTED, 0, 0);
    }
}

/**
 * WRAPPER_WSTATE_STARTED
 * The Wrapper process is started.  It will remain in this state until
 *  the Wrapper is ready to start shutting down.  The JVM process may
 *  be restarted one or more times while the Wrapper is in this state.
 *
 * nowTicks: The tick counter value this time through the event loop.
 */
void wStateStarted(DWORD nowTicks) {
    /* Just keep running.  Nothing to do here. */
}

/**
 * WRAPPER_WSTATE_STOPPING
 * The Wrapper process has started its shutdown process.  It will
 *  remain in this state until it is confirmed that the JVM has been
 *  stopped.
 *
 * nowTicks: The tick counter value this time through the event loop.
 */
void wStateStopping(DWORD nowTicks) {
    /* The wrapper is stopping, we need to ping the service manager */
    /*  to reasure it that we are still alive. */
    
    /* Tell the service manager that we are stopping */
    wrapperReportStatus(FALSE, WRAPPER_WSTATE_STOPPING, wrapperData->exitCode, 1000);
    
    /* If the JVM state is now DOWN, then change the wrapper state */
    /*  to be STOPPED as well. */
    if (wrapperData->jState == WRAPPER_JSTATE_DOWN) {
        wrapperData->wState = WRAPPER_WSTATE_STOPPED;
        
        /* Don't tell the service manager that we stopped here.  That */
        /*	will be done when the application actually quits. */
    }
}

/**
 * WRAPPER_WSTATE_STOPPED
 * The Wrapper process is now ready to exit.  The event loop will complete
 *  and the Wrapper process will exit.
 *
 * nowTicks: The tick counter value this time through the event loop.
 */
void wStateStopped(DWORD nowTicks) {
    /* The wrapper is ready to stop.  Nothing to be done here.  This */
    /*  state will exit the event loop below. */
}

/********************************************************************
 * JVM States
 *******************************************************************/

/**
 * WRAPPER_JSTATE_DOWN
 * The JVM process currently does not exist.  Depending on the Wrapper
 *  state and other factors, we will either stay in this state or switch
 *  to the LAUNCH state causing a JVM to be launched after a delay set
 *  in this function.
 *
 * nowTicks: The tick counter value this time through the event loop.
 * nextSleep: Flag which is used to determine whether or not the state engine
 *            will be sleeping before then next time through the loop.  It
 *            may make sense to avoid certain actions if it is known that the
 *            function will be called again immediately.
 */
void jStateDown(DWORD nowTicks, int nextSleep) {
    char onExitParamBuffer[16 + 10 + 1];
    int startupDelay;

    /* The JVM can be down for one of 3 reasons.  The first is that the
     *  wrapper is just starting.  The second is that the JVM is being
     *  restarted for some reason, and the 3rd is that the wrapper is
     *  trying to shut down. */
    if ((wrapperData->wState == WRAPPER_WSTATE_STARTING) ||
        (wrapperData->wState == WRAPPER_WSTATE_STARTED)) {

        if (wrapperData->restartRequested) {
            /* A JVM needs to be launched. */
            wrapperData->restartRequested = FALSE;

            /* Depending on the number of restarts to date, decide how to handle the (re)start. */
            if (wrapperData->jvmRestarts > 0) {
                /* This is not the first JVM, so make sure that we still want to launch. */
                if (wrapperGetTickAge(wrapperData->jvmLaunchTicks, nowTicks) >= wrapperData->successfulInvocationTime) {
                    /* The previous JVM invocation was running long enough that its invocation */
                    /*   should be considered a success.  Reset the failedInvocationStart to   */
                    /*   start the count fresh.                                                */
                    wrapperData->failedInvocationCount = 0;

                    /* Set the state to launch after the restart delay. */
                    wrapperData->jState = WRAPPER_JSTATE_LAUNCH;
                    wrapperData->jStateTimeoutTicks = wrapperAddToTicks(nowTicks, wrapperData->restartDelay);
                    wrapperData->jStateTimeoutTicksSet = 1;

                    if (wrapperData->restartDelay > 0) {
                        if (wrapperData->isDebugging) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, 
                                "Waiting %d seconds before launching another JVM.", wrapperData->restartDelay);
                        }
                    }
                } else {
                    /* The last JVM invocation died quickly and was considered to have */
                    /*  been a faulty launch.  Increase the failed count.              */
                    wrapperData->failedInvocationCount++;

                    if (wrapperData->isDebugging) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, 
                            "JVM was only running for %d seconds leading to a failed restart count of %d.",
                            wrapperGetTickAge(wrapperData->jvmLaunchTicks, nowTicks), wrapperData->failedInvocationCount);
                    }

                    /* See if we are allowed to try restarting the JVM again. */
                    if (wrapperData->failedInvocationCount < wrapperData->maxFailedInvocations) {
                        /* Try reslaunching the JVM */

                        /* Set the state to launch after the restart delay. */
                        wrapperData->jState = WRAPPER_JSTATE_LAUNCH;
                        wrapperData->jStateTimeoutTicks = wrapperAddToTicks(nowTicks, wrapperData->restartDelay);
                        wrapperData->jStateTimeoutTicksSet = 1;

                        if (wrapperData->restartDelay > 0) {
                            if (wrapperData->isDebugging) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, 
                                    "Waiting %d seconds before launching another JVM.", wrapperData->restartDelay);
                            }
                        }
                    } else {
                        /* Unable to launch another JVM. */
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                                   "There were %d failed launches in a row, each lasting less than %d seconds.  Giving up.",
                                   wrapperData->failedInvocationCount, wrapperData->successfulInvocationTime);
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                                   "  There may be a configuration problem: please check the logs.");
                        wrapperData->wState = WRAPPER_WSTATE_STOPPING;
                    }
                }
            } else {
                /* This will be the first invocation. */
                wrapperData->failedInvocationCount = 0;

                /* Set the state to launch after the startup delay. */
                wrapperData->jState = WRAPPER_JSTATE_LAUNCH;
                if (wrapperData->isConsole) {
                    startupDelay = wrapperData->startupDelayConsole;
                } else {
                    startupDelay = wrapperData->startupDelayService;
                }
                wrapperData->jStateTimeoutTicks = wrapperAddToTicks(nowTicks, startupDelay);
                wrapperData->jStateTimeoutTicksSet = 1;

                if (startupDelay > 0) {
                    if (wrapperData->isDebugging) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, 
                            "Waiting %d seconds before launching the first JVM.", startupDelay);
                    }
                }
            }
        } else {
            /* The JVM is down, but a restart has not yet been requested.
             *   See if the user has registered any events for the exit code. */
            sprintf(onExitParamBuffer, "wrapper.on_exit.%d", wrapperData->exitCode);
            if (checkPropertyEqual(properties, onExitParamBuffer, getStringProperty(properties, "wrapper.on_exit.default", "shutdown"), "restart")) {
                /* We want to restart the JVM. */
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    "on_exit trigger matched.  Restarting the JVM.  (Exit code: %d)", wrapperData->exitCode);

                wrapperData->restartRequested = TRUE;

                /* Fall through, the restart will take place on the next loop. */
            } else {
                /* We want to stop the Wrapper. */
                wrapperData->wState = WRAPPER_WSTATE_STOPPING;
            }
        }
    } else {
        /* The wrapper is shutting down.  Do nothing. */
    }

    /* Reset the last ping time */
    wrapperData->lastPingTicks = nowTicks;
}

/**
 * WRAPPER_JSTATE_LAUNCH
 * Waiting to launch a JVM.  When the state timeout has expired, a JVM
 *  will be launched.
 *
 * nowTicks: The tick counter value this time through the event loop.
 * nextSleep: Flag which is used to determine whether or not the state engine
 *            will be sleeping before then next time through the loop.  It
 *            may make sense to avoid certain actions if it is known that the
 *            function will be called again immediately.
 */
void jStateLaunch(DWORD nowTicks, int nextSleep) {
    /* The Waiting state is set from the DOWN state if a JVM had
     *  previously been launched the Wrapper will wait in this state
     *  until the restart delay has expired.  If this was the first
     *  invocation, then the state timeout will be set to the current
     *  time causing the new JVM to be launced immediately. */
    if ((wrapperData->wState == WRAPPER_WSTATE_STARTING) ||
        (wrapperData->wState == WRAPPER_WSTATE_STARTED)) {

        /* Is it time to proceed? */
        if (wrapperGetTickAge(wrapperData->jStateTimeoutTicks, nowTicks) >= 0) {
            /* Launch the new JVM */

            /* Set the launch time to the curent time */
            wrapperData->jvmLaunchTicks = wrapperGetTicks();

            /* Generate a unique key to use when communicating with the JVM */
            wrapperBuildKey();
        
            /* Generate the command used to launch the Java process */
            wrapperBuildJavaCommand();
        
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Launching a JVM...");
            wrapperExecute();
        
            /* Check if the start was successful. */
            if (nextSleep && (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN)) {
                /* Failed to start the JVM.  Tell the wrapper to shutdown. */
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unable to start a JVM");
                wrapperData->wState = WRAPPER_WSTATE_STOPPING;
            } else {
                /* The JVM was launched.  We still do not know whether the
                 *  launch will be successful.  Allow <startupTimeout> seconds before giving up.
                 *  This can take quite a while if the system is heavily loaded.
                 *  (At startup for example) */
                wrapperData->jState = WRAPPER_JSTATE_LAUNCHING;
                wrapperData->jStateTimeoutTicks = wrapperAddToTicks(nowTicks, wrapperData->startupTimeout);
                wrapperData->jStateTimeoutTicksSet = 1;
            }
        }
    } else {
        /* The wrapper is shutting down.  Switch to the down state. */
        wrapperData->jState = WRAPPER_JSTATE_DOWN;
    }
}

/**
 * WRAPPER_JSTATE_LAUNCH
 * The JVM process has been launched, but there has been no confirmation that
 *  the JVM and its application have started.  We remain in this state until
 *  the state times out or the WrapperManager class in the JVM has sent a
 *  message that it is initialized.
 *
 * nowTicks: The tick counter value this time through the event loop.
 * nextSleep: Flag which is used to determine whether or not the state engine
 *            will be sleeping before then next time through the loop.  It
 *            may make sense to avoid certain actions if it is known that the
 *            function will be called again immediately.
 */
void jStateLaunching(DWORD nowTicks, int nextSleep) {
    /* Make sure that the JVM process is still up and running */
    if (nextSleep && (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN)) {
        /* The process is gone.  Restart it. */
        wrapperData->jState = WRAPPER_JSTATE_DOWN;
        wrapperData->jStateTimeoutTicks = 0;
        wrapperData->jStateTimeoutTicksSet = 0;
        wrapperData->restartRequested = TRUE;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                   "JVM exited while loading the application.");
        wrapperProtocolClose();
    } else {
        /* The process is up and running.
         * We are waiting in this state until we receive a KEY packet
         *  from the JVM attempting to register.
         * Have we waited too long already */
        if (wrapperGetTickAge(wrapperData->jStateTimeoutTicks, nowTicks) >= 0) {
            displayLaunchingTimeoutMessage();

            /* Give up on the JVM and start trying to kill it. */
            wrapperKillProcess(FALSE);

            /* Restart the JVM. */
            wrapperData->restartRequested = TRUE;
        }
    }
}

/**
 * WRAPPER_JSTATE_LAUNCHED
 * The WrapperManager class in the JVM has been initialized.  We are now
 *  ready to request that the application in the JVM be started.
 *
 * nowTicks: The tick counter value this time through the event loop.
 * nextSleep: Flag which is used to determine whether or not the state engine
 *            will be sleeping before then next time through the loop.  It
 *            may make sense to avoid certain actions if it is known that the
 *            function will be called again immediately.
 */
void jStateLaunched(DWORD nowTicks, int nextSleep) {
    int ret;

    /* The Java side of the wrapper code has responded to a ping.
     *  Tell the Java wrapper to start the Java application. */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Start Application.");
    }
    ret = wrapperProtocolFunction(WRAPPER_MSG_START, "start");
    if (ret < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unable to send the start command to the JVM.");

        /* Give up on the JVM and start trying to kill it. */
        wrapperKillProcess(FALSE);

        /* Restart the JVM. */
        wrapperData->restartRequested = TRUE;
    } else {
        /* Start command send.  Start waiting for the app to signal
         *  that it has started.  Allow <startupTimeout> seconds before 
         *  giving up.  A good application will send starting signals back
         *  much sooner than this as a way to extend this time if necessary. */
        wrapperData->jState = WRAPPER_JSTATE_STARTING;
        wrapperData->jStateTimeoutTicks = wrapperAddToTicks(nowTicks, wrapperData->startupTimeout);
        wrapperData->jStateTimeoutTicksSet = 1;
    }
}

/**
 * WRAPPER_JSTATE_STARTING
 * The JVM is up and the application has been asked to start.  We
 *  stay in this state until we receive confirmation that the
 *  application has been started or the state times out.
 *
 * nowTicks: The tick counter value this time through the event loop.
 * nextSleep: Flag which is used to determine whether or not the state engine
 *            will be sleeping before then next time through the loop.  It
 *            may make sense to avoid certain actions if it is known that the
 *            function will be called again immediately.
 */
void jStateStarting(DWORD nowTicks, int nextSleep) {
    /* Make sure that the JVM process is still up and running */
    if (nextSleep && (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN)) {
        /* The process is gone.  Restart it. */
        wrapperData->jState = WRAPPER_JSTATE_DOWN;
        wrapperData->jStateTimeoutTicks = 0;
        wrapperData->jStateTimeoutTicksSet = 0;
        wrapperData->restartRequested = TRUE;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                   "JVM exited while starting the application.");
        wrapperProtocolClose();
    } else {
        /* Have we waited too long already */
        if (wrapperGetTickAge(wrapperData->jStateTimeoutTicks, nowTicks) >= 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                       "Startup failed: Timed out waiting for signal from JVM.");

            /* Give up on the JVM and start trying to kill it. */
            wrapperKillProcess(FALSE);

            /* Restart the JVM. */
            wrapperData->restartRequested = TRUE;
        } else {
            /* Keep waiting. */
        }
    }
}

/**
 * WRAPPER_JSTATE_STARTED
 * The application in the JVM has confirmed that it is started.  We will
 *  stay in this state, sending pings to the JVM at regular intervals,
 *  until the JVM fails to respond to a ping, or the JVM is ready to be
 *  shutdown.
 * The pings are sent to make sure that the JVM does not die or hang.
 *
 * nowTicks: The tick counter value this time through the event loop.
 * nextSleep: Flag which is used to determine whether or not the state engine
 *            will be sleeping before then next time through the loop.  It
 *            may make sense to avoid certain actions if it is known that the
 *            function will be called again immediately.
 */
void jStateStarted(DWORD nowTicks, int nextSleep) {
    int ret;

    /* Make sure that the JVM process is still up and running */
    if (nextSleep && (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN)) {
        /* The process is gone.  Restart it. */
        wrapperData->jState = WRAPPER_JSTATE_DOWN;
        wrapperData->jStateTimeoutTicks = 0;
        wrapperData->jStateTimeoutTicksSet = 0;
        wrapperData->restartRequested = TRUE;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                   "JVM exited unexpectedly.");
        wrapperProtocolClose();
    } else {
        /* Have we waited too long already.  The jStateTimeoutTicks is reset each time a ping
         *  response is received from the JVM. */
        if (wrapperGetTickAge(wrapperData->jStateTimeoutTicks, nowTicks) >= 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                       "JVM appears hung: Timed out waiting for signal from JVM.");

            /* Give up on the JVM and start trying to kill it. */
            wrapperKillProcess(FALSE);

            /* Restart the JVM. */
            wrapperData->restartRequested = TRUE;
        } else if (wrapperGetTickAge(wrapperAddToTicks(wrapperData->lastPingTicks, wrapperData->pingInterval), nowTicks) >= 0) {
            /* It is time to send another ping to the JVM */
            if (wrapperData->isLoopOutputEnabled) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Temp: Sending a ping packet.");
            }
            ret = wrapperProtocolFunction(WRAPPER_MSG_PING, "ping");
            if (ret < 0) {
                /* Failed to send the ping. */
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM Ping Failed.");
                }
            }
            if (wrapperData->isLoopOutputEnabled) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Temp: Sent a ping packet.");
            }
            wrapperData->lastPingTicks = nowTicks;
        } else {
            /* Do nothing.  Keep waiting. */
        }
    }
}

/**
 * WRAPPER_JSTATE_STOPPING
 * The application in the JVM has been asked to stop but we are still
 *  waiting for a signal that it is stopped.
 *
 * nowTicks: The tick counter value this time through the event loop.
 * nextSleep: Flag which is used to determine whether or not the state engine
 *            will be sleeping before then next time through the loop.  It
 *            may make sense to avoid certain actions if it is known that the
 *            function will be called again immediately.
 */
void jStateStopping(DWORD nowTicks, int nextSleep) {
    /* Make sure that the JVM process is still up and running */
    if (nextSleep && (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN)) {
        /* The process is gone. */
        wrapperData->jState = WRAPPER_JSTATE_DOWN;
        wrapperData->jStateTimeoutTicks = 0;
        wrapperData->jStateTimeoutTicksSet = 0;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                   "JVM exited unexpectedly while stopping the application.");
        wrapperProtocolClose();
    } else {
        /* Have we waited too long already */
        if (wrapperGetTickAge(wrapperData->jStateTimeoutTicks, nowTicks) >= 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                       "Shutdown failed: Timed out waiting for signal from JVM.");

            /* Give up on the JVM and start trying to kill it. */
            wrapperKillProcess(FALSE);
        } else {
            /* Keep waiting. */
        }
    }
}

/**
 * WRAPPER_JSTATE_STOPPING
 * The application in the JVM has signalled that it has stopped.  We are now
 *  waiting for the JVM process to exit.  A good application will do this on
 *  its own, but if it fails to exit in a timely manner then the JVM will be
 *  killed.
 * Once the JVM process is gone we go back to the DOWN state.
 *
 * nowTicks: The tick counter value this time through the event loop.
 * nextSleep: Flag which is used to determine whether or not the state engine
 *            will be sleeping before then next time through the loop.  It
 *            may make sense to avoid certain actions if it is known that the
 *            function will be called again immediately.
 */
void jStateStopped(DWORD nowTicks, int nextSleep) {
    if (nextSleep && (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN)) {
        /* The process is gone. */
        wrapperData->jState = WRAPPER_JSTATE_DOWN;
        wrapperData->jStateTimeoutTicks = 0;
        wrapperData->jStateTimeoutTicksSet = 0;
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM exited normally.");
        }
        wrapperProtocolClose();
    } else {
        /* Have we waited too long already */
        if (wrapperGetTickAge(wrapperData->jStateTimeoutTicks, nowTicks) >= 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                       "Shutdown failed: Timed out waiting for the JVM to terminate.");

            /* Give up on the JVM and start trying to kill it. */
            wrapperKillProcess(FALSE);
        } else {
            /* Keep waiting. */
        }
    }
}

/**
 * WRAPPER_JSTATE_KILLING
 * The Wrapper is about to kill the JVM.  If thread dumps on exit is enabled
 *  then the Wrapper must wait a few moments between telling the JVM to do
 *  a thread dump and actually killing it.  The Wrapper will sit in this state
 *  while it is waiting.
 *
 * nowTicks: The tick counter value this time through the event loop.
 * nextSleep: Flag which is used to determine whether or not the state engine
 *            will be sleeping before then next time through the loop.  It
 *            may make sense to avoid certain actions if it is known that the
 *            function will be called again immediately.
 */
void jStateKilling(DWORD nowTicks, int nextSleep) {
    /* Make sure that the JVM process is still up and running */
    if (nextSleep && (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN)) {
        /* The process is gone. */
        wrapperData->jState = WRAPPER_JSTATE_DOWN;
        wrapperData->jStateTimeoutTicks = 0;
        wrapperData->jStateTimeoutTicksSet = 0;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO,
                   "JVM exited on its own while waiting to kill the application.");
        wrapperProtocolClose();
    } else {
        /* Have we waited long enough */
        if (wrapperGetTickAge(wrapperData->jStateTimeoutTicks, nowTicks) >= 0) {
            /* It is time to actually kill the JVM. */
            wrapperKillProcessNow();
        } else {
            /* Keep waiting. */
        }
    }
}

/********************************************************************
 * Event Loop / State Engine
 *******************************************************************/

void logTimerStats() {
    char buffer[30];
    struct tm when;
    time_t now, overflowTime;

    DWORD sysTicks;
    DWORD ticks;

    time(&now);

    sysTicks = wrapperGetSystemTicks();

    overflowTime = (time_t)(now - (sysTicks / (1000 / WRAPPER_TICK_MS)));
    when = *localtime(&overflowTime);
    sprintf(buffer, "%s", asctime(&when));
    buffer[strlen(buffer) - 1] = '\0'; /* Remove the line feed. */
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Last system time tick overflow at: %s", buffer);

    overflowTime = (time_t)(now + ((0xffffffffUL - sysTicks) / (1000 / WRAPPER_TICK_MS)));
    when = *localtime(&overflowTime);
    sprintf(buffer, "%s", asctime(&when));
    buffer[strlen(buffer) - 1] = '\0'; /* Remove the line feed. */
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Next system time tick overflow at: %s", buffer);

    if (!wrapperData->useSystemTime) {
        ticks = wrapperGetTicks(); 

        overflowTime = (time_t)(now - (ticks / (1000 / WRAPPER_TICK_MS)));
        when = *localtime(&overflowTime);
        sprintf(buffer, "%s", asctime(&when));
        buffer[strlen(buffer) - 1] = '\0'; /* Remove the line feed. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Last tick overflow at: %s", buffer);

        overflowTime = (time_t)(now + ((0xffffffffUL - ticks) / (1000 / WRAPPER_TICK_MS)));
        when = *localtime(&overflowTime);
        sprintf(buffer, "%s", asctime(&when));
        buffer[strlen(buffer) - 1] = '\0'; /* Remove the line feed. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Next tick overflow at: %s", buffer);
    }
}

/**
 * The main event loop for the wrapper.  Handles all state changes and events.
 */
void wrapperEventLoop() {
    DWORD nowTicks;
    DWORD lastCycleTicks = wrapperGetTicks();
    int nextSleep;

    wrapperData->anchorTimeoutTicks = lastCycleTicks;
    wrapperData->memoryOutputTimeoutTicks = lastCycleTicks;

    if (wrapperData->isTimerOutputEnabled) {
        logTimerStats();
    }

    if (wrapperData->isLoopOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Event loop started.");
    }

    nextSleep = TRUE;
    do {
        if (wrapperData->isLoopOutputEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Loop: %ssleep", (nextSleep ? "" : "no "));
        }
        if (nextSleep) {
            /* Sleep for a tenth of a second. */
            wrapperSleep(100);
        }
        nextSleep = TRUE;

        /* Before doing anything else, always maintain the logger to make sure
         *  that any queued messages are logged before doing anything else.
         *  Called a second time after socket and child output to make sure
         *  that all messages appropriate for the state changes have been
         *  logged.  Failure to do so can result in a confusing sequence of
         *  output. */
        if (wrapperData->isLoopOutputEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Loop: maintain logger");
        }
        maintainLogger();

        /* Check the stout pipe of the child process. */
        if (wrapperData->isLoopOutputEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Loop: process jvm output");
        }
        if ( wrapperReadChildOutput() )
        {
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                    "Pause reading child output to share cycles.");
            }
            nextSleep = FALSE;
        }
        
        /* Check for incoming data packets. */
        if (wrapperData->isLoopOutputEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Loop: process socket");
        }
        if ( wrapperProtocolRead() )
        {
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                    "Pause reading socket data to share cycles.");
            }
            nextSleep = FALSE;
        }
        
        /* See comment for first call above. */
        if (wrapperData->isLoopOutputEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Loop: maintain logger(2)");
        }
        maintainLogger();

        /* Get the current time for use in this cycle. */
        nowTicks = wrapperGetTicks();
        
        /* Log memory usage. */
        if (wrapperData->isMemoryOutputEnabled) {
            if (wrapperTickExpired(nowTicks, wrapperData->memoryOutputTimeoutTicks)) {
                wrapperDumpMemory();
                wrapperData->memoryOutputTimeoutTicks = wrapperAddToTicks(nowTicks, wrapperData->memoryOutputInterval);
            }
        }

        /* Has the process been getting CPU? This check will only detect a lag
         * if the useSystemTime flag is set. */
        if (wrapperGetTickAge(lastCycleTicks, nowTicks) > wrapperData->cpuTimeout) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO,
                "Wrapper Process has not received any CPU time for %d seconds.  Extending timeouts.",
                wrapperGetTickAge(lastCycleTicks, nowTicks));

            if (wrapperData->jStateTimeoutTicksSet) {
                wrapperData->jStateTimeoutTicks =
                    wrapperAddToTicks(wrapperData->jStateTimeoutTicks, wrapperGetTickAge(lastCycleTicks, nowTicks));
            }
        }
        lastCycleTicks = nowTicks;

        /* Useful for development debugging, but not runtime debugging */
        if (wrapperData->isStateOutputEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                       "    Ticks=%08lx, WrapperState=%s, JVMState=%s JVMStateTimeoutTicks=%08lx (%ds), Exit=%s, Restart=%s",
                       nowTicks,
                       wrapperGetWState(wrapperData->wState),
                       wrapperGetJState(wrapperData->jState),
                       wrapperData->jStateTimeoutTicks,
                       (wrapperData->jStateTimeoutTicksSet ? wrapperGetTickAge(nowTicks, wrapperData->jStateTimeoutTicks) : 0),
                       (wrapperData->exitRequested ? "true" : "false"),
                       (wrapperData->restartRequested ? "true" : "false"));
        }

        /* If we are configured to do so, confirm that the anchor file still exists. */
        anchorPoll(nowTicks);
        
        if (wrapperData->exitRequested) {
            /* A new request for the JVM to be stopped has been made. */

            if (wrapperData->isLoopOutputEnabled) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Loop: exit requested");
            }
            /* Acknowledge that we have seen the exit request so we don't get here again. */
            wrapperData->exitRequested = FALSE;
            
            if (wrapperData->jState == WRAPPER_JSTATE_DOWN) {
                /** A JVM is not currently running. Nothing to do.*/
            } else if (wrapperData->jState == WRAPPER_JSTATE_LAUNCH) {
                /** A JVM is not yet running go back to the DOWN state. */
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeoutTicks = 0;
                wrapperData->jStateTimeoutTicksSet = 0;
            } else if ((wrapperData->jState == WRAPPER_JSTATE_STOPPING) ||
                (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
                (wrapperData->jState == WRAPPER_JSTATE_KILLING)) {
                /** The JVM is already being stopped, so nothing else needs to be done. */
            } else {
                /* The JVM should be running, so it needs to be stopped. */
                if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                    /* The process is gone. */
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                        "JVM shut down unexpectedly.");

                    wrapperData->jState = WRAPPER_JSTATE_DOWN;
                    wrapperData->jStateTimeoutTicks = 0;
                    wrapperData->jStateTimeoutTicksSet = 0;
                    wrapperProtocolClose();
                } else {
                    /* JVM is still up.  Try asking it to shutdown nicely. */
                    if (wrapperData->isDebugging) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                            "Sending stop signal to JVM");
                    }
                
                    wrapperProtocolFunction(WRAPPER_MSG_STOP, NULL);
                
                    /* Allow up to 5 + <shutdownTimeout> seconds for the application to stop itself. */
                    wrapperData->jState = WRAPPER_JSTATE_STOPPING;
                    wrapperData->jStateTimeoutTicks = wrapperAddToTicks(nowTicks, 5 + wrapperData->shutdownTimeout);
                    wrapperData->jStateTimeoutTicksSet = 1;
                }
            }
        }
        
        /* Do something depending on the wrapper state */
        if (wrapperData->isLoopOutputEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Loop: handle wrapper state: %s",
                wrapperGetWState(wrapperData->wState));
        }
        switch(wrapperData->wState) {
        case WRAPPER_WSTATE_STARTING:
            wStateStarting(nowTicks);
            break;
            
        case WRAPPER_WSTATE_STARTED:
            wStateStarted(nowTicks);
            break;
            
        case WRAPPER_WSTATE_STOPPING:
            wStateStopping(nowTicks);
            break;
            
        case WRAPPER_WSTATE_STOPPED:
            wStateStopped(nowTicks);
            break;
            
        default:
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unknown wState=%d", wrapperData->wState);
            break;
        }
        
        /* Do something depending on the JVM state */
        if (wrapperData->isLoopOutputEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    Loop: handle jvm state: %s",
                wrapperGetJState(wrapperData->jState));
        }
        switch(wrapperData->jState) {
        case WRAPPER_JSTATE_DOWN:
            jStateDown(nowTicks, nextSleep);
            break;
            
        case WRAPPER_JSTATE_LAUNCH:
            jStateLaunch(nowTicks, nextSleep);
            break;

        case WRAPPER_JSTATE_LAUNCHING:
            jStateLaunching(nowTicks, nextSleep);
            break;

        case WRAPPER_JSTATE_LAUNCHED:
            jStateLaunched(nowTicks, nextSleep);
            break;

        case WRAPPER_JSTATE_STARTING:
            jStateStarting(nowTicks, nextSleep);
            break;

        case WRAPPER_JSTATE_STARTED:
            jStateStarted(nowTicks, nextSleep);
            break;

        case WRAPPER_JSTATE_STOPPING:
            jStateStopping(nowTicks, nextSleep);
            break;

        case WRAPPER_JSTATE_STOPPED:
            jStateStopped(nowTicks, nextSleep);
            break;

        case WRAPPER_JSTATE_KILLING:
            jStateKilling(nowTicks, nextSleep);
            break;

        default:
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unknown jState=%d", wrapperData->jState);
            break;
        }
    } while (wrapperData->wState != WRAPPER_WSTATE_STOPPED);
    
    if (wrapperData->isLoopOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Event loop stopped.");
    }
}
