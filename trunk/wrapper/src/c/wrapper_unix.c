/*
 * Copyright (c) 1999, 2003 TanukiSoftware.org
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
 * $Log$
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

char wrapperClasspathSeparator = ':';

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
                   "Could not dump JVM state: %s", (char *)strerror(errno));
    }
}

/**
 * Handle interrupt signals (i.e. Crtl-C).
 */
void handleInterrupt(int sig_num) {
    signal(SIGINT, handleInterrupt);

    if (wrapperData->exitRequested) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Forcing immediate shutdown.");
        wrapperKillProcess();
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Shutting down.");
        wrapperStopProcess(0);
    }
}

/**
 * Handle quit signals (i.e. Crtl-\).
 */
void handleQuit(int sig_num) {
    signal(SIGQUIT, handleInterrupt); 
    requestDumpJVMState();
}

/**
 * Handle termination signals (i.e. machine is shutting down).
 */
void handleTermination(int sig_num) {
    signal(SIGTERM, handleTermination); 

    if (wrapperData->exitRequested) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Forcing immediate shutdown.");
        wrapperKillProcess();
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Shutting down.");
        wrapperStopProcess(0);
    }
}

/**
 * Execute initialization code to get the wrapper set up.
 */
int wrapperInitialize() {
    int retval = 0;

    /* Set handlers for signals */
    if (signal(SIGINT,  handleInterrupt)   == SIG_ERR ||
        signal(SIGQUIT, handleQuit)        == SIG_ERR ||
        signal(SIGTERM, handleTermination) == SIG_ERR) {
        retval = -1;
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
    wrapperData->jvmCommand = (char **)malloc(sizeof(char *) * length + 1); 
    /*                        number of arguments + 1 for a NULL pointer at the end */
    for (i = 0; i <= length; i++) {
        if (i < length) {
            wrapperData->jvmCommand[i] = (char *)malloc(sizeof(char *) * strlen(strings[i]) + 1);
            sprintf(wrapperData->jvmCommand[i], strings[i]);
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
    int pipedes[2];
    pid_t proc;

    /* Create the pipe. */
    if (pipe (pipedes) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                   "Could not init pipe: %s", (char *)strerror(errno));
        return;
    }
    
    /* Fork off the child. */
    proc = fork();
    
    if (proc == -1) {
        /* Fork failed. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                   "Could not spawn JVM process: %s", (char *)strerror(errno));
        
        /* Close the pipe descriptors. */
        (void)close(pipedes[STDIN_FILENO]);
        (void)close(pipedes[STDOUT_FILENO]);
        
    } else {
        /* Fork succeeded: increment the process ID for logging. */
        wrapperData->jvmRestarts++;

        if (proc == 0) {
            /* We are the child side. */
            
            /* Send output to the pipe. */
            if (dup2(pipedes[STDOUT_FILENO], STDOUT_FILENO) < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "Unable to set JVM's stdout: %s", (char *)strerror(errno));
                return;
            }
        
            /* Send errors to the pipe. */
            if (dup2(pipedes[STDOUT_FILENO], STDERR_FILENO) < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "Unable to set JVM's stderr: %s", (char *)strerror(errno));
                return;
            }
        
            /* Close the pipe descriptors. */
            (void)close(pipedes[STDIN_FILENO]);
            (void)close(pipedes[STDOUT_FILENO]);
            
            /* Child process: execute the JVM. */
            execvp(wrapperData->jvmCommand[0], wrapperData->jvmCommand);
            
            /* We reached this point...meaning we were unable to start. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unable to start JVM: %s (%d)", (char *)strerror(errno), errno);
        
        } else {
            /* We are the parent side. */
            jvmPid = proc;
            jvmOut = pipedes[STDIN_FILENO];

            /* Close the unused pipe descriptor. */
            (void)close(pipedes[STDOUT_FILENO]);
            
            /* Mark our side of the pipe so that it won't block
             * and will close on exec, so new children won't see it. */
            (void)fcntl(jvmOut, F_SETFL, O_NONBLOCK);
            (void)fcntl(jvmOut, F_SETFD, FD_CLOEXEC);
        }
    }
}

/**
 * Checks on the status of the JVM Process.
 * Returns WRAPPER_PROCESS_UP or WRAPPER_PROCESS_DOWN
 */
int wrapperGetProcessStatus() {
    int retval;
    int status;

    retval = waitpid(jvmPid, NULL, WNOHANG);

    if (retval < 0) {
        /* Wait failed. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                   "Critical error: wait for JVM process failed (%s)", (char *)strerror(errno));
        exit(1);

    } else if (retval > 0) {
        /* JVM has exited. */
        status = WRAPPER_PROCESS_DOWN;

    } else {
        status = WRAPPER_PROCESS_UP;
    }
    
    return status;
}

/**
 * This function does nothing on Unix machines.
 */
void wrapperReportStatus(int status, int errorCode, int waitHint) {
    return;
}

/**
 * Read and process any output from the child JVM Process.
 * Most output should be logged to the wrapper log file.
 *
 * This function will only read up to 50 lines of data before returning this is to
 *  make sure that the main loop gets CPU.  If there is more data in the pipe then
 *  the function returns -1, otherwise 0.  This is a hint to the mail loop not to
 *  sleep.
 */
int wrapperReadChildOutput() {
    int readSize;
    ssize_t bytesRead;
    char readBuf [1025];
    char writeBuf[1025];
    int r, w; /* readBufPos, writeBufPos */
    int count;
    int retCode;
    
    retCode = -1;
    if (jvmOut != -1) {
        /* Loop and read as much input as is available.  When a large amount of output is
         *  being piped from the JVM this can lead to the main event loop not getting any
         *  CPU for an extended period of time.  To avoid that problem, this loop is only
         *  allowed to cycle 50 times before returning.  After 50 times, switch to a less
         *  efficient method of reading data because we need to make sure that we have
         *  not read past a line break before returning. */
        count = 0;
        while(1) {
            if ( count < 50 ) {
                readSize = 1024;
            } else {
                readSize = 1;
            }

            /* Fill read buffer. */
            bytesRead = read(jvmOut, readBuf, readSize);
        
            if (bytesRead <= 0) {
                retCode = 0;
                break;
            }
            /* Terminate the read buffer. */
            readBuf[bytesRead] = '\0';
        
            /* Step through chars in read buffer. */
            w = 0;
            for (r = 0; r < bytesRead; r++) {
                if (readBuf[r] == (char)0x0a) {
                    /* Line feed; write out buffer and reset it. */
                    writeBuf[w] = '\0';
                    wrapperLogChildOutput(writeBuf);
                    w = 0;

                    if ( count >= 50 ) {
                        // This last line was read byte by byte, now exit.
                        break;
                    }

                    count++;
                } else {
                    /* Add character to write buffer. */
                    writeBuf[w++] = readBuf[r];
                }
            }
        
            /* Write out the rest of the buffer. */
            if (w > 0) {
                writeBuf[w] = '\0';
                wrapperLogChildOutput(writeBuf);
                w = 0;

                if ( count >= 50 ) {
                    // This last line was read byte by byte, now exit.
                    break;
                }

                count++;
            }
        }
    }
    
    return retCode;
}

/**
 * Kill the JVM Process immediately and set the JVM State to WRAPPER_JSTATE_DOWN
 */
void wrapperKillProcess() {

    /* Check to make sure that the JVM process is still running */
    if (waitpid(jvmPid, NULL, WNOHANG) == 0) {
        /* JVM is still up when it should have already stopped itself. */
        if (wrapperData->requestThreadDumpOnFailedJVMExit) {
            requestDumpJVMState();

            usleep(1000000); /* 1 second in microseconds */
        }

        /* Kill it immediately. */
        kill(jvmPid, SIGKILL);
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM did not exit on request, terminated");

        /* Give the JVM a chance to be killed so that the state will be correct. */
        usleep(500000); /* 0.5 seconds in microseconds */
    }

    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->jStateTimeout = 0;
    jvmPid = -1;

    /* Close any open socket to the JVM */
    wrapperProtocolClose();
}

/**
 * Show usage.
 */
void wrapperUsage(char *appName) {
    printf("Usage: %s <file> [config properties] [...]\n", appName);
    printf("<file> is the application config file.\n");
    printf("\n");
    printf("[config properties] are configuration name-value pairs which override values\n");
    printf("  in wrapper.conf.  For example:\n");
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
    
    umask(0); /* clear file creation mask */
    
    /* first fork */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Spawning intermediate process...");
    }	
    if ((pid = fork()) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Could not spawn daemon process: %s",
                   (char *)strerror(errno));
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
    
    setsid(); /* become session leader */
    signal(SIGHUP, SIG_IGN); /* don't let future opens allocate controlling terminals */
    
    /* second fork */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Spawning daemon process...");
    }	
    if ((pid = fork()) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Could not spawn daemon process: %s",
                   (char *)strerror(errno));
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
    wrapperData = (WrapperConfig *)malloc(sizeof(WrapperConfig));
    wrapperData->isConsole = TRUE;
    wrapperData->wState = WRAPPER_WSTATE_STARTING;
    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->jStateTimeout = 0;
    wrapperData->lastPingTime = 0;
    wrapperData->jvmCommand = NULL;
    wrapperData->exitRequested = FALSE;
    wrapperData->exitAcknowledged = FALSE;
    wrapperData->exitCode = 0;
    wrapperData->restartRequested = FALSE;
    wrapperData->jvmRestarts = 0;
    wrapperData->jvmLaunchTime = time(NULL);
    wrapperData->failedInvocationCount = 0;
        
    wrapperInitializeLogging();
    
    if (argc < 2) {
        wrapperUsage(argv[0]);
        exit(1);
        
    } else if (strcmp(argv[1],"--help") == 0) {
        wrapperUsage(argv[0]);
        exit(0);
        
    } else {
        /* Create a Properties structure. */
        properties = createProperties();

        /* The first argument is the config file, followed by 0 or more
         *  command line properties.  The command line properties need to be
         *  loaded first, followed by the config file. */
        for (i = 2; i < argc; i++) {
            if (addPropertyPair(properties, argv[i], TRUE)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, 
                    "The argument '%s' is not a valid property name-value pair.", argv[i]);
                exit(1);
            }
        }

        /* Now load the config file. */
        if (loadProperties(properties, argv[1])) {
            /* File not found. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Unable to open wrapper config file: %s", argv[1]);
            exit(1);
        
        } else {
            /* Store the config file name. */
            wrapperData->configFile = argv[1];
            
            /* Display the active properties */
            /*
            printf("Debug Config Properties:\n");
            dumpProperties(properties);
            */

            /* Apply properties to the WrapperConfig structure. */
            if (wrapperLoadConfiguration()) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    "Problem loading wrapper config file: %s", argv[1]);
                exit(1);
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
                         wrapperData->pidFilename, (char *)strerror(errno));
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
