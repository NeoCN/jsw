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
// Revision 1.4  2001/12/11 05:19:39  mortenson
// Added the ablility to format and/or disable file logging and output to
// the console.
//
// Revision 1.3  2001/12/07 07:29:38  rybesh
// finished making wrapper scripts/executable relocatable on unix
//
// Revision 1.2  2001/12/07 01:48:28  rybesh
// updated unix scripts + code to make paths relative, more easily relocatable
//
// Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
// no message
//

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
 * Handle interrupt signals (i.e. Crtl-C).
 */
void handleInterrupt(int sig_num) {
    signal(SIGINT, handleInterrupt); 
    wrapperLog(WRAPPER_SOURCE_WRAPPER, "Shutting down.");
    wrapperStopProcess(0);
}

/**
 * Handle termination signals (i.e. machine is shutting down).
 */
void handleTermination(int sig_num) {
    signal(SIGTERM, handleTermination); 
    wrapperLog(WRAPPER_SOURCE_WRAPPER, "Shutting down.");
    wrapperStopProcess(0);
}

/**
 * Execute initialization code to get the wrapper set up.
 */
int wrapperInitialize() {
    int retval = 0;

    // Set handlers for signals
    if (signal(SIGINT,  handleInterrupt)   == SIG_ERR ||
        signal(SIGTERM, handleTermination) == SIG_ERR) {
        retval = -1;
    }

    return retval;
}

void wrapperBuildJavaCommand() {
    char **strings;
    int length, i;

    // If this is not the first time through, then dispose the old command array
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

    // Build the Java Command Strings
    strings = NULL;
    length = 0;
    wrapperBuildJavaCommandArray(&strings, &length, FALSE);
    
    if (wrapperData->isDebugging) {
        for (i = 0; i < length; i++) {
            wrapperLogIS(WRAPPER_SOURCE_WRAPPER, "Command[%d] : %s", i, strings[i]);
        }
    }

    // Allocate memory to hold array of command strings
    wrapperData->jvmCommand = (char **)malloc(sizeof(char *) * length + 1); 
    //                        number of arguments + 1 for a NULL pointer at the end
    for (i = 0; i <= length; i++) {
        if (i < length) {
            wrapperData->jvmCommand[i] = (char *)malloc(sizeof(char *) * strlen(strings[i]) + 1);
            sprintf(wrapperData->jvmCommand[i], strings[i]);
        } else {
            wrapperData->jvmCommand[i] = NULL;
        }
    }
    
    // Free up the temporary command array
    wrapperFreeJavaCommandArray(strings, length);
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
        usleep(5000000); // microseconds
    }
}

/**
 * Launches a JVM process and stores it internally.
 */
void wrapperExecute() {
    int pipedes[2];
    pid_t proc;

    // Create the pipe.
    if (pipe (pipedes) < 0) {
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, 
                    "Could not init pipe: %s", (char *)strerror(errno));
        return;
    }
    
    // Fork off the child.
    proc = fork();
    
    if (proc == -1) {
        // Fork failed.
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, 
                    "Could not spawn JVM process: %s", (char *)strerror(errno));
        
        // Close the pipe descriptors.
        (void)close(pipedes[STDIN_FILENO]);
        (void)close(pipedes[STDOUT_FILENO]);
        
    } else {
        // Fork succeeded: increment the process ID for logging.
        wrapperData->jvmRestarts++;

        if (proc == 0) {
            // We are the child side. 
            
            // Send output to the pipe.
            if (dup2(pipedes[STDOUT_FILENO], STDOUT_FILENO) < 0) {
                wrapperLogS(WRAPPER_SOURCE_WRAPPER, 
                            "Unable to set JVM's stdout: %s", (char *)strerror(errno));
                return;
            }
        
            // Send errors to the pipe.
            if (dup2(pipedes[STDOUT_FILENO], STDERR_FILENO) < 0) {
                wrapperLogS(WRAPPER_SOURCE_WRAPPER, 
                            "Unable to set JVM's stderr: %s", (char *)strerror(errno));
                return;
            }
        
            // Close the pipe descriptors.
            (void)close(pipedes[STDIN_FILENO]);
            (void)close(pipedes[STDOUT_FILENO]);
            
            // Child process: execute the JVM.
            execvp(wrapperData->jvmCommand[0], wrapperData->jvmCommand);
            
            // We reached this point...meaning we were unable to start.
            wrapperLogSI(WRAPPER_SOURCE_WRAPPER, "Unable to start JVM: %s (%d)", (char *)strerror(errno), errno);
        
        } else {
            // We are the parent side.
            jvmPid = proc;
            jvmOut = pipedes[STDIN_FILENO];

            // Close the unused pipe descriptor.
            (void)close(pipedes[STDOUT_FILENO]);
            
            // Mark our side of the pipe so that it won't block
            // and will close on exec, so new children won't see it.
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
        // Wait failed.
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, 
                    "Critical error: wait for JVM process failed (%s)", (char *)strerror(errno));
        exit(1);

    } else if (retval > 0) {
        // JVM has exited.
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
 */
void wrapperReadChildOutput() {
    ssize_t bytesRead;
    char readBuf [1025];
    char writeBuf[1025];
    int r, w; // readBufPos, writeBufPos
    
    if (jvmOut != -1) {
        while (1) {
            
            // Fill read buffer.
            bytesRead = read(jvmOut, readBuf, 1024);
            
            if (bytesRead <= 0) {
                break;
            }
            // Terminate the read buffer.
            readBuf[bytesRead] = '\0';
            
            // Step through chars in read buffer.
            w = 0;
            for (r = 0; r < bytesRead; r++) {
                if (readBuf[r] == (char)0x0a) {
                    // Line feed; write out buffer and reset it.
                    writeBuf[w] = '\0';
                    wrapperLog(wrapperData->jvmRestarts, writeBuf);
                    w = 0;
                } else {
                    // Add character to write buffer.
                    writeBuf[w++] = readBuf[r];
                }
            }
            
            // Write out the rest of the buffer.
        if (w > 0) {
                writeBuf[w] = '\0';
                wrapperLog(wrapperData->jvmRestarts, writeBuf);
                w = 0;
            }
        }
    }
}

/**
 * Kill the JVM Process immediately and set the JVM State to WRAPPER_JSTATE_DOWN
 */
void wrapperKillProcess() {

    // Check to make sure that the JVM process is still running
    if (waitpid(jvmPid, NULL, WNOHANG) == 0) {
        // JVM is still up.  Kill it immediately.
        kill(jvmPid, SIGKILL);
        wrapperLog(WRAPPER_SOURCE_WRAPPER, "JVM did not exit on request, terminated");
    }

    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->jStateTimeout = 0;
    jvmPid = -1;

    // Close any open socket to the JVM
    wrapperProtocolClose();
}

/**
 * Show usage.
 */
void wrapperUsage(char *appName) {
    printf("Usage: %s <file>\n", appName);
    printf("<file> is the application config file.\n");
    printf("\n");
    printf("Options:  --help\n");
}

#ifdef SOLARIS
int writePidFile() {
    FILE *pid_fp = NULL;
    mode_t old_umask;

    //enter_suid();
    old_umask = umask(022);
    pid_fp = fopen(wrapperData->pidFilename, "w");
    umask(old_umask);
    //leave_suid();

    if (pid_fp != NULL) {
        fprintf(pid_fp, "%d\n", (int)getpid());
        fclose(pid_fp);
    } else {
    return 1;
    }
    return 0;
}
#endif

/**
 * Sets the working directory to that of the current executable
 */
int setWorkingDir(char* path) {
    char* resolved;
    char* pos;
    
    resolved = malloc(PATH_MAX * sizeof(char));

    // Get the full path and filename of this program
    if (realpath(path, resolved) == NULL) {
        wrapperLogSS(WRAPPER_SOURCE_WRAPPER, 
                     "Unable to get the full path to %s: %s", path, (char *)strerror(errno));
        return 1;
    }
    
    // The wrapperData->isDebugging flag will never be set here, so we can't really use it.
#ifdef DEBUG
    wrapperLogS(WRAPPER_SOURCE_WRAPPER, "Path to executable: %s", resolved);
#endif
    
    // To get the path, strip everything off after the last '\'
    pos = strrchr(resolved, '/');
    if (pos == NULL) {
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "Unable to extract dirname from: %s", resolved);
        return 1;
    } else {
        // Clip the path at the position of the last slash
        pos[0] = (char)0;
    }
    
    if (chdir(resolved)) {
        wrapperLogS(WRAPPER_SOURCE_WRAPPER, "Unable to change working directory: %s", (char *)strerror(errno));
        return 1;
    }
    
    // The wrapperData->isDebugging flag will never be set here, so we can't really use it.
#ifdef DEBUG
    wrapperLogS(WRAPPER_SOURCE_WRAPPER, "Working directory set to: %s", resolved);
#endif

    free(resolved);
    
    return 0;
}

/*******************************************************************************
 * Main function                                                               *
 *******************************************************************************/

int main(int argc, char **argv) {
    int exitStatus;

    // Initialize the WrapperConfig structure.
    wrapperData = (WrapperConfig *)malloc(sizeof(WrapperConfig));
    wrapperData->isConsole = TRUE;
    wrapperData->wState = WRAPPER_WSTATE_STARTING;
    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->logFile = NULL;
    wrapperData->logFileFormat = "PTM";
    wrapperData->consoleFormat = "PM";
    wrapperData->jvmCommand = NULL;
    wrapperData->exitRequested = FALSE;
    wrapperData->exitAcknowledged = FALSE;
    wrapperData->exitCode = 0;
    wrapperData->restartRequested = FALSE;
    wrapperData->jvmRestarts = 0;
        
    if (setWorkingDir(argv[0])) {
        exit(1);
    }
    
    if (argc != 2) {
        wrapperUsage(argv[0]);
        exit(1);
        
    } else if (strcmp(argv[1],"--help") == 0) {
        wrapperUsage(argv[0]);
        exit(0);
        
    } else {
        properties = loadProperties(argv[1]);
        if (properties == NULL) {
            // File not found.
            wrapperLogS(WRAPPER_SOURCE_WRAPPER, "Unable to open wrapper config file: %s", argv[1]);
            exit(1);
        
        } else {
            // Store the config file name.
            wrapperData->configFile = argv[1];
            
            // Apply properties to the WrapperConfig structure.
            if (wrapperLoadConfiguration()) {
                wrapperLogS
                    (WRAPPER_SOURCE_WRAPPER, "Problem loading wrapper config file: %s", argv[1]);
                exit(1);
            }
#ifdef SOLARIS
            // Write pid file.
            if (writePidFile()) {
         wrapperLogSS
            (WRAPPER_SOURCE_WRAPPER,
             "ERROR: Could not write pid file %s: %s",
             wrapperData->pidFilename, (char *)strerror(errno));
        exit(1);
        }
#endif
            exitStatus = wrapperRunConsole();
            
#ifdef SOLARIS
            // Remove pid file.
            unlink(wrapperData->pidFilename);
#endif
            exit(exitStatus);
        }
    }
}

#endif // ifndef WIN32
