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
// Revision 1.10  2002/01/28 01:14:36  mortenson
// Changed default nt description.
// Looks like some tabs to spaces conversions also.
//
// Revision 1.9  2002/01/28 01:01:53  rybesh
// few minor fixes to get solaris version to compile
//
// Revision 1.8  2002/01/27 19:35:00  spocke
// Added support for wildcards on Unix classpaths and service description property.
//
// Revision 1.7  2002/01/27 16:58:32  mortenson
// Changed the log rolling defaults from -1 to 0
//
// Revision 1.6  2002/01/26 23:30:35  spocke
// Added rolling file support to logger.
//
// Revision 1.5  2002/01/24 09:43:56  mortenson
// Added new Logger code which allows log levels.
//
// Revision 1.4  2002/01/13 04:49:53  mortenson
// Added Wildcard support for Classpath entries.
//
// Revision 1.3  2001/12/11 05:19:39  mortenson
// Added the ablility to format and/or disable file logging and output to
// the console.
//
// Revision 1.2  2001/12/06 09:36:24  mortenson
// Docs changes, Added sample apps, Fixed some problems with
// relative paths  (See revisions.txt)
//
// Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
// no message
//

/**
 * Author:
 *   Leif Mortenson <leif@silveregg.co.jp>
 *   Ryan Shaw      <ryan@silveregg.co.jp>
 *
 * Version CVS $Revision$ $Date$
 */

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "property.h"
#include "wrapper.h"
#include "logger.h"

#ifdef WIN32
#include <io.h>
#include <winsock.h>
#define EADDRINUSE  WSAEADDRINUSE
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ENOTSOCK    WSAENOTSOCK
#define ECONNRESET  WSAECONNRESET

#else /* UNIX */
#include <string.h>
#include <glob.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define SOCKET         int
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define __max(x,y) (((x) > (y)) ? (x) : (y))
#define __min(x,y) (((x) < (y)) ? (x) : (y))

#ifdef SOLARIS
#include <sys/errno.h>
#include <sys/fcntl.h>

#else /* LINUX */
#include <asm/errno.h>

#endif /* UNIX */

#endif /* WIN32 */

WrapperConfig *wrapperData;
char         logBuffer[2048];
char         iLogBuffer[2048];  // Used by wrapperInnerLog
char         *keyChars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";

// Properties structure loaded in from the config file.
Properties              *properties;

// Server Socket.
SOCKET ssd = INVALID_SOCKET;
// Client Socket.
SOCKET sd = INVALID_SOCKET;

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
    default:
        name = "UNKNOWN";
        break;
    }
    return name;
}

void wrapperProtocolStartServer() {
    struct sockaddr_in addr_srv;
    int rc;
    u_short port;
    int trys;

#ifdef WIN32
    u_long dwNoBlock = TRUE;
#endif

    // Create the server socket.
    ssd = socket(PF_INET, SOCK_STREAM, 0);
    if (ssd == INVALID_SOCKET) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, "server socket creation failed. (%d)", wrapperGetLastError());
        return;
    }

    // Make the socket non-blocking
#ifdef WIN32
    rc = ioctlsocket(ssd, FIONBIO, &dwNoBlock);
#else // UNIX 
    rc = fcntl(ssd, F_SETFL, O_NONBLOCK);
#endif

    if (rc == SOCKET_ERROR) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, "server socket ioctlsocket failed. (%d)", wrapperGetLastError());
        wrapperProtocolStopServer();
        return;
    }

    // Start looking for at open server port at the given value.  Loop until a bind is successful
    port = wrapperData->port;
    trys = 0;

  tryagain:
    // Try binding to the port.
    addr_srv.sin_family = PF_INET;
    addr_srv.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr_srv.sin_port = htons(port);
#ifdef WIN32
    rc = bind(ssd, (struct sockaddr FAR *)&addr_srv, sizeof(addr_srv));
#else // UNIX
    rc = bind(ssd, (struct sockaddr *)&addr_srv, (socklen_t)sizeof(addr_srv));
#endif
    
    if (rc == SOCKET_ERROR) {

        rc = wrapperGetLastError();
        if (rc == EADDRINUSE) {
            // Address in use, try looking at the next one.
            port++;
            if (port > 65000) {
                port = 10000;
            }
            if (trys < 100) {
                trys++;
                goto tryagain;
            }
        }

        // Log an error.  This is fatal, so die.
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_FATAL, "unable to bind listener port %d. (%d)", wrapperData->port, wrapperGetLastError());

        wrapperStopProcess(rc);

        wrapperProtocolStopServer();
        return;
    }

    // If we got here, then we are bound to the port
    if (port != wrapperData->port) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_INFO, "port %d already in use, using port %d instead.", wrapperData->port, port);
    }
    wrapperData->actualPort = port;

    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "server listening on port %d.", wrapperData->actualPort);
    }

    // Tell the socket to start listening.
    rc = listen(ssd, 1);
    if (rc == SOCKET_ERROR) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, "server socket listen failed. (%d)", wrapperGetLastError());
        wrapperProtocolStopServer();
        return;
    }
}

void wrapperProtocolStopServer() {
    int rc;
    // Close the socket.
    if (ssd != INVALID_SOCKET) {
#ifdef WIN32
        rc = closesocket(ssd);
#else // UNIX
        rc = close(ssd);
#endif
        if (rc == SOCKET_ERROR) {
         if (wrapperData->isDebugging) {
             log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "server socket close failed. (%d)", wrapperGetLastError());
            }
        }
        ssd = INVALID_SOCKET;
    }

    wrapperData->actualPort = 0;

    // Close any client connection that may be open
    wrapperProtocolClose();
}

/**
 * Attempt to accept a connection from a JVM client.
 */
void wrapperProtocolOpen() {
    struct sockaddr_in addr_srv;
    int addr_srv_len;
    int rc;

#ifdef WIN32
    u_long dwNoBlock = TRUE;
#endif

    // Is the server socket open?
    if (ssd == INVALID_SOCKET) {
        // can't do anything yet.
        return;
    }

    // Is it already open?
    if (sd != INVALID_SOCKET) {
        return;
    }

    // Try accepting a socket.
    addr_srv_len = sizeof(addr_srv);
#ifdef WIN32
    sd = accept(ssd, (struct sockaddr FAR *)&addr_srv, &addr_srv_len);
#else // UNIX
    sd = accept(ssd, (struct sockaddr *)&addr_srv, (socklen_t *)&addr_srv_len);
#endif
    if (sd == INVALID_SOCKET) {
        rc = wrapperGetLastError();
        if (rc == EWOULDBLOCK) {
            // There are no incomming sockets right now.
            return;
        } else {
         if (wrapperData->isDebugging) {
              log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "socket creation failed. (%d)", rc);
            }
            return;
        }
    }

    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "accepted a socket from %s on port %d",
                 (char *)inet_ntoa(addr_srv.sin_addr), ntohs(addr_srv.sin_port));
    }

    // Make the socket non-blocking
#ifdef WIN32
    rc = ioctlsocket(sd, FIONBIO, &dwNoBlock);
#else // UNIX 
    rc = fcntl(sd, F_SETFL, O_NONBLOCK);
#endif
    if (rc == SOCKET_ERROR) {
        if (wrapperData->isDebugging) {
         log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "socket ioctlsocket failed. (%d)", wrapperGetLastError());
        }
        wrapperProtocolClose();
        return;
    }
}

void wrapperProtocolClose() {
    int rc;

    // Close the socket.
    if (sd != INVALID_SOCKET) {
#ifdef WIN32
        rc = closesocket(sd);
#else // UNIX
        rc = close(sd);
#endif
        if (rc == SOCKET_ERROR) {
         if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "socket close failed. (%d)", wrapperGetLastError());
            }
        }
        sd = INVALID_SOCKET;
    }
}

int wrapperProtocolFunction(char function, const char *message) {
    int rc;
    char buffer[1024];
    int len;

    // Open the socket if necessary
    wrapperProtocolOpen();

    if (sd == INVALID_SOCKET) {
        // A socket was not opened
        return -1;
    }

    // Build the packet
    buffer[0] = function;
    if (message == NULL) {
        buffer[1] = '\0';
        len = 2;
    } else {
        len = strlen(message);
        strcpy((char*)(buffer + 1), message);
        len += 2;
    }

    // Send the packet
    rc = send(sd, buffer, len, 0);
    if (rc == SOCKET_ERROR) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "socket send failed. (%d)", wrapperGetLastError());
        }
        wrapperProtocolClose();
        return -1;
    }
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "sent %d bytes", rc);
    }

    return 1;
}


int wrapperProtocolRead() {
    char c, code;
    int len;
    int pos;
    int err;
    char buffer[257];

    // If we have an open client socket, then use it.
    if (sd == INVALID_SOCKET) {
        // A Client socket is not open

        // Is the server socket open?
        if (ssd == INVALID_SOCKET) {
            wrapperProtocolStartServer();
            if (ssd == INVALID_SOCKET) {
                // Failed.
                return FALSE;
            }
        }

        // Try accepting a socket
        wrapperProtocolOpen();
        if (sd == INVALID_SOCKET) {
            return FALSE;
        }
    }

    // Try receiving a packet code
    len = recv(sd, &c, 1, 0);
    if (len == SOCKET_ERROR) {
        err = wrapperGetLastError();
        if (wrapperData->isDebugging) {
            if ((err != EWOULDBLOCK) && (err != ENOTSOCK) && (err != ECONNRESET)) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "socket read failed. (%d)", err);
            }
        }
        return FALSE;	
    } else if (len != 1) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "socket read no code (closed?).");
        }
        return FALSE;	
    }

    code = c;

    // Read in any message
    pos = 0;
    do {
        len = recv(sd, &c, 1, 0);
        if (len == 1) {
            if (c == 0) {
                // End of string
                len = 0;
            } else if (pos < 256) {
                buffer[pos] = c;
                pos++;
            }
        } else {
            len = 0;
        }
    } while (len == 1);
    // terminate the string;
    buffer[pos] = '\0';

    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "read a packet %d : %s", code, buffer);
    }

    switch (code) {
    case WRAPPER_MSG_STOP:
        wrapperStopRequested(atoi(buffer));
        break;
    case WRAPPER_MSG_RESTART:
        wrapperRestartRequested();
        break;
    case WRAPPER_MSG_PING:
        wrapperPingResponded();
        break;
    case WRAPPER_MSG_STOP_PENDING:
        wrapperStopPendingSignalled(atoi(buffer));
        break;
    case WRAPPER_MSG_STOPPED:
        wrapperStoppedSignalled();
        break;
    case WRAPPER_MSG_START_PENDING:
        wrapperStartPendingSignalled(atoi(buffer));
        break;
    case WRAPPER_MSG_STARTED:
        wrapperStartedSignalled();
        break;
    case WRAPPER_MSG_KEY:
        wrapperKeyRegistered(buffer);
        break;
    default:
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "received unknown packet (%d:%s)", code, buffer);
        }
        break;
    }

    return TRUE;
}



/******************************************************************************
 * Wrapper inner methods.
 *****************************************************************************/

/**
 * Launch the wrapper as a console application.
 */
int wrapperRunConsole() {
    int res;

    // Setup the wrapperData structure.
    wrapperData->wState = WRAPPER_WSTATE_STARTING;
    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->isConsole = TRUE;

    // Initialize the wrapper
    res = wrapperInitialize();
    if (res != 0) {
        return res;
    }

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "--> Wrapper Started as Console");

    // Enter main event loop
    wrapperEventLoop();

    // Clean up any open sockets.
    wrapperProtocolStopServer();
    wrapperProtocolClose();

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "<-- Wrapper Stopped");

    return wrapperData->exitCode;
}

/**
 * Launch the wrapper as a service application.
 */
int wrapperRunService() {
    int res;

    // Setup the wrapperData structure.
    wrapperData->wState = WRAPPER_WSTATE_STARTING;
    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->isConsole = FALSE;

    // Initialize the wrapper
    res = wrapperInitialize();
    if (res != 0) {
        return res;
    }

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "--> Wrapper Started as Service");

    // Enter main event loop
    wrapperEventLoop();

    // Clean up any open sockets.
    wrapperProtocolStopServer();
    wrapperProtocolClose();

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "<-- Wrapper Stopped");

    return wrapperData->exitCode;
}

/**
 * Used to ask the state engine to shut down the JVM and Wrapper
 */
void wrapperStopProcess(int exitCode) {
    // If it has not already been set, set the exit request flag in the wrapper data.
    if (!wrapperData->exitRequested) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "wrapperStopProcess(%d) called.", exitCode);
        }

        wrapperData->exitCode = exitCode;
        wrapperData->exitRequested = TRUE;
    } else {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "wrapperStopProcess(%d) called.  (IGNORED)", exitCode);
        }
    }
}

/**
 * Used to ask the state engine to shut down the JVM.
 */
void wrapperRestartProcess() {
    // If it has not already been set, set the restart request flag in the wrapper data.
    if (!wrapperData->restartRequested) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "wrapperRestartProcess() called.");
        }

        wrapperData->restartRequested = TRUE;
    } else {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "wrapperRestartProcess() called.  (IGNORED)");
        }
    }
}

/**
 * Keep track of the number of times that the JVM has been restarted within a
 *  short perioud of time.
 */
int wrapperRestartCount = 0;
time_t wrapperRestartLastTime;
int wrapperCheckRestartTimeOK() {
    time_t newtime = time(NULL);
    if (newtime - wrapperRestartLastTime < 60) {
        // Last restart was less than 60 seconds ago
        ++wrapperRestartCount;
    } else {
        wrapperRestartCount = 0;
    }
    wrapperRestartLastTime = newtime;

    if (wrapperRestartCount >= 5) {
        // Only 5 restarts are allowed in a short perioud of time before giving up
        if (wrapperData->isDebugging) {
            log_printf
                (WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, 
                 "VM died too many times w/in 60 second intervals (%d); no more tries", 
                 wrapperRestartCount);
        }
        return FALSE;
    } else {
        return TRUE;
    }
}

void wrapperStripQuotes(const char *prop, char *propStripped) {
    int len, i, j;

    len = strlen(prop);
    j = 0;
    for (i = 0; i < len; i++) {
        if (prop[i] != '\"') {
            propStripped[j] = prop[i];
            j++;
        }
    }
    propStripped[j] = '\0';
}

/**
 * Loops over and stores all necessary commands into an array which
 *  can be used to launch a process.
 * This method will only count the elements if stringsPtr is NULL.
 */
int wrapperBuildJavaCommandArrayInner(char **strings, int addQuotes) {
    int index;
    const char *prop;
    char *propStripped;
    int stripQuote;
    int initMemory = 0, maxMemory;
    char paramBuffer[128];
    int i, j, len2;
    int cpLen, cpLenAlloc;
    char *tmpString;
#ifdef WIN32
    char cpPath[512];
    char *c;
    long handle;
    int len;
    struct _finddata_t fblock;
#else
    glob_t g;
    int findex;
#endif

    index = 0;

    // Java commnd
    if (strings) {
        prop = getStringProperty(properties, "wrapper.java.command", "java");
        strings[index] = (char *)malloc(sizeof(char) * (strlen(prop) + 2 + 1));
        if (addQuotes) {
            sprintf(strings[index], "\"%s\"", prop);
        } else {
            sprintf(strings[index], "%s", prop);
        }
    }
    index++;

    // Store additional java parameters
    i = 0;
    do {
        sprintf(paramBuffer, "wrapper.java.additional.%d", i + 1);
        prop = getStringProperty(properties, paramBuffer, NULL);
        if (prop) {
            if (strlen(prop) > 0) {
                if (strings) {
                    sprintf(paramBuffer, "wrapper.java.additional.%d.stripquotes", i + 1);
                    if (addQuotes) {
                        stripQuote = FALSE;
                    } else {
                        stripQuote = getBooleanProperty(properties, paramBuffer, FALSE);
                    }
                    if (stripQuote) {
                        propStripped = (char *)malloc(sizeof(char) * strlen(prop) + 1);
                        wrapperStripQuotes(prop, propStripped);
                    } else {
                        propStripped = (char *)prop;
                    }

                    strings[index] = (char *)malloc(sizeof(char) * (strlen(propStripped) + 1));
                    sprintf(strings[index], "%s", propStripped);

                    if (stripQuote) {
                        free(propStripped);
                        propStripped = NULL;
                    }
                }
                index++;
            }
            i++;
        }
    } while (prop);

    // Initial JVM memory
    if (strings) {
        initMemory = __min(__max(getIntProperty(properties, "wrapper.java.initmemory", 8), 8), 4096); // 8 <= n <= 4096
        strings[index] = (char *)malloc(sizeof(char) * (5 + 4 + 1));  // Allow up to 4 digits.
        sprintf(strings[index], "-Xms%dm", initMemory);
    }
    index++;

    // Maximum JVM memory
    if (strings) {
        maxMemory = __min(__max(getIntProperty(properties, "wrapper.java.maxmemory", 128), initMemory), 4096);  // initMemory <= n <= 4096
        strings[index] = (char *)malloc(sizeof(char) * (5 + 4 + 1));  // Allow up to 4 digits.
        sprintf(strings[index], "-Xmx%dm", maxMemory);
    }
    index++;

    // Library Path
    if (strings) {
        prop = getStringProperty(properties, "wrapper.java.library.path", "./");
        strings[index] = (char *)malloc(sizeof(char) * (22 + strlen(prop) + 1));
        if (addQuotes) {
            sprintf(strings[index], "-Djava.library.path=\"%s\"", prop);
        } else {
            sprintf(strings[index], "-Djava.library.path=%s", prop);
        }
    }
    index++;

    // Store the classpath
    if (strings) {
        strings[index] = (char *)malloc(sizeof(char) * (10 + 1));
        sprintf(strings[index], "-classpath");
    }
    index++;
    if (strings) {
        // Build a classpath
        cpLen = 0;
        cpLenAlloc = 1024;
        strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
        
        // Add an open quote the classpath
        if (addQuotes) {
            sprintf(&(strings[index][cpLen]), "\"");
            cpLen++;
        }

        // Loop over the classpath entries adding each one
        i = 0;
        j = 0;
        do {
            sprintf(paramBuffer, "wrapper.java.classpath.%d", i + 1);
            prop = getStringProperty(properties, paramBuffer, NULL);
            if (prop) {
                len2 = strlen(prop);
                if (len2 > 0) {
                    // Does this contain wildcards?
                    if ((strchr(prop, '*') != NULL) || (strchr(prop, '?') != NULL)) {
                        // Need to do a wildcard search
#ifdef WIN32
                        // Extract any path information of the beginning of the file
                        strcpy(cpPath, prop);
                        c = max(strrchr(cpPath, '\\'), strrchr(cpPath, '/'));
                        if (c == NULL) {
                            cpPath[0] = '\0';
                        } else {
                            c[1] = '\0'; // terminate after the slash
                        }
                        len = strlen(cpPath);

                        //if (_findfirst(prop, &fblock, _A_NORMAL) != 0) {
                        if ((handle = _findfirst(prop, &fblock)) <= 0) {
                            if (errno == ENOENT) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Warning no matching files for classpath element: %s", prop);
                            } else {
                                // Encountered an error of some kind.
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Error in findfirst for classpath element: %s", prop);
                            }
                        } else {
                            len2 = strlen(fblock.name);

                            // Is there room for the entry?
                            if (cpLen + len + len2 + 3 > cpLenAlloc) {
                                // Resize the buffer
                                tmpString = strings[index];
                                cpLenAlloc += 1024;
                                strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
                                sprintf(strings[index], tmpString);
                                free(tmpString);
                            }

                            if (j > 0) {
                                strings[index][cpLen++] = wrapperClasspathSeparator; // separator
                            }
                            sprintf(&(strings[index][cpLen]), "%s%s", cpPath, fblock.name);
                            cpLen += (len + len2);
                            j++;

                            // Look for additional entries
                            while (_findnext(handle, &fblock) == 0) {
                                len2 = strlen(fblock.name);

                                // Is there room for the entry?
                                if (cpLen + len + len2 + 3 > cpLenAlloc) {
                                    // Resize the buffer
                                    tmpString = strings[index];
                                    cpLenAlloc += 1024;
                                    strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
                                    sprintf(strings[index], tmpString);
                                    free(tmpString);
                                }

                                if (j > 0) {
                                    strings[index][cpLen++] = wrapperClasspathSeparator; // separator
                                }
                                sprintf(&(strings[index][cpLen]), "%s%s", cpPath, fblock.name);
                                cpLen += (len + len2);
                                j++;
                            }

                            // Close the file search
                            _findclose(handle);
                        }
#else
                        // * * Wildcard support for unix
                        glob(prop, GLOB_MARK | GLOB_NOSORT, NULL, &g);

                        if( g.gl_pathc > 0 ) {
                            for( findex=0; findex<g.gl_pathc; findex++ ) {
                                // Is there room for the entry?
                                len2 = strlen(g.gl_pathv[findex]);
                                if (cpLen + len2 + 3 > cpLenAlloc) {
                                    // Resize the buffer
                                    tmpString = strings[index];
                                    cpLenAlloc += 1024;
                                    strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
                                    sprintf(strings[index], tmpString);
                                    free(tmpString);
                                }

                                if (j > 0) {
                                    strings[index][cpLen++] = wrapperClasspathSeparator; // separator
                                }
                                sprintf(&(strings[index][cpLen]), "%s", g.gl_pathv[findex]);
                                cpLen += len2;
                                j++;
                            }
                        } else {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Warning no matching files for classpath element: %s", prop);
                        }

                        globfree(&g);
#endif
                    } else {
                        // Is there room for the entry?
                        if (cpLen + len2 + 3 > cpLenAlloc) {
                            // Resize the buffer
                            tmpString = strings[index];
                            cpLenAlloc += 1024;
                            strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
                            sprintf(strings[index], tmpString);
                            free(tmpString);
                        }

                     if (j > 0) {
                            strings[index][cpLen++] = wrapperClasspathSeparator; // separator
                        }
                        sprintf(&(strings[index][cpLen]), prop);
                        cpLen += len2;
                        j++;
                    }
                }
                i++;
            }
        } while (prop);
        if (j == 0) {
            // No classpath, use default. always room
            if (addQuotes) {
                sprintf(&(strings[index][cpLen++]), "./");
            }
        }
        // Add ending quote
        if (addQuotes) {
            sprintf(&(strings[index][cpLen]), "\"");
            cpLen++;
        }
    }
    index++;

    // Store the Wrapper key
    if (strings) {
        wrapperBuildKey();
        strings[index] = (char *)malloc(sizeof(char) * (16 + strlen(wrapperData->key) + 1));
        if (addQuotes) {
            sprintf(strings[index], "-Dwrapper.key=\"%s\"", wrapperData->key);
        } else {
            sprintf(strings[index], "-Dwrapper.key=%s", wrapperData->key);
        }
    }
    index++;

    // Store the Wrapper server port
    if (strings) {
        strings[index] = (char *)malloc(sizeof(char) * (15 + 5 + 1));  // Port up to 5 characters
        sprintf(strings[index], "-Dwrapper.port=%d", (int)wrapperData->actualPort);
    }
    index++;

    // Store the Wrapper debug flag
    if (wrapperData->isDebugging) {
        if (strings) {
            strings[index] = (char *)malloc(sizeof(char) * (22 + 1));
            if (addQuotes) {
                sprintf(strings[index], "-Dwrapper.debug=\"TRUE\"");
            } else {
                sprintf(strings[index], "-Dwrapper.debug=TRUE");
            }
        }
        index++;
    }

    // Store the Disable Shutdown Hook flag
    if (wrapperData->isShutdownHookDisabled) {
        if (strings) {
            strings[index] = (char *)malloc(sizeof(char) * (38 + 1));
            if (addQuotes) {
                sprintf(strings[index], "-Dwrapper.disable_shutdown_hook=\"TRUE\"");
            } else {
                sprintf(strings[index], "-Dwrapper.disable_shutdown_hook=TRUE");
            }
        }
        index++;
    }

    // Store the Wrapper JVM ID.  (Get here before incremented)
    if (strings) {
        strings[index] = (char *)malloc(sizeof(char) * (16 + 5 + 1));  // jvmid up to 5 characters
        sprintf(strings[index], "-Dwrapper.jvmid=%d", (wrapperData->jvmRestarts + 1));
    }
    index++;

    // Store the main class
    if (strings) {
        prop = getStringProperty(properties, "wrapper.java.mainclass", "Main");
        strings[index] = (char *)malloc(sizeof(char) * (strlen(prop) + 1));
        sprintf(strings[index], "%s", prop);
    }
    index++;

    // Store any application parameters
    i = 0;
    do {
        sprintf(paramBuffer, "wrapper.app.parameter.%d", i + 1);
        prop = getStringProperty(properties, paramBuffer, NULL);
        if (prop) {
            if (strlen(prop) > 0) {
                if (strings) {
                    sprintf(paramBuffer, "wrapper.app.parameter.%d.stripquotes", i + 1);
                    if (addQuotes) {
                        stripQuote = FALSE;
                    } else {
                        stripQuote = getBooleanProperty(properties, paramBuffer, FALSE);
                    }
                    if (stripQuote) {
                        propStripped = (char *)malloc(sizeof(char) * strlen(prop) + 1);
                        wrapperStripQuotes(prop, propStripped);
                    } else {
                        propStripped = (char *)prop;
                    }

                    strings[index] = (char *)malloc(sizeof(char) * (strlen(propStripped) + 1));
                    sprintf(strings[index], "%s", propStripped);

                    if (stripQuote) {
                        free(propStripped);
                        propStripped = NULL;
                    }
                }
                index++;
            }
            i++;
        }
    } while (prop);

    return index;
}

/**
 * command is a pointer to a pointer of an array of character strings.
 * length is the number of strings in the above array.
 */
void wrapperBuildJavaCommandArray(char ***stringsPtr, int *length, int addQuotes) {
    // Find out how long the array needs to be first.
    *length = wrapperBuildJavaCommandArrayInner(NULL, addQuotes);

    // Allocate the correct amount of memory
    *stringsPtr = (char **)malloc(sizeof(char *) * (*length));

    // Now actually fill in the strings
    wrapperBuildJavaCommandArrayInner(*stringsPtr, addQuotes);
}

void wrapperFreeJavaCommandArray(char **strings, int length) {
    int i;

    if (strings != NULL) {
        // Loop over and free each of the strings in the array
        for (i = 0; i < length; i++) {
            if (strings[i] != NULL) {
                free(strings[i]);
                strings[i] = NULL;
            }
        }
        free(strings);
        strings = NULL;
    }
}

/**
 * The main event loop for the wrapper.  Handles all state changes and events.
 */
void wrapperEventLoop() {
    int ret;

    do {
        // Sleep for a quarter second.
#ifdef WIN32
        Sleep(250);     // milliseconds
#else // UNIX
        usleep(250000); // microseconds
#endif

        // Check the stout pipe of the child process.
        wrapperReadChildOutput();
        
        // Check for incoming data packets.
        wrapperProtocolRead();
        
        // Useful for development debugging, but not runtime debugging
        //log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
        //           "    WState=%s, JState=%s timeout=%d",
        //           wrapperGetWState(wrapperData->wState),
        //           wrapperGetJState(wrapperData->jState),
        //           (wrapperData->jStateTimeout == 0 ? 
        //            0 : wrapperData->jStateTimeout - time(NULL)));
        
        if ((wrapperData->exitRequested && (! wrapperData->exitAcknowledged))
            || wrapperData->restartRequested) {
            
            if (wrapperData->exitRequested) {
                
                // Acknowledge that we have seen the exit request.
                wrapperData->exitAcknowledged = TRUE;
                
                // If the state of the wrapper is not STOPPING or STOPPED, then
                //	set it to STOPPING
                if ((wrapperData->wState != WRAPPER_WSTATE_STOPPING) &&
                    (wrapperData->wState != WRAPPER_WSTATE_STOPPED)) {
                    wrapperData->wState = WRAPPER_WSTATE_STOPPING;
                    wrapperData->exitCode = wrapperData->exitCode;
                }
            }
            
            // Check whether the JVM is running or not
            if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                // JVM Process is gone
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM shut down unexpectedly.");
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeout = 0;
                wrapperProtocolClose();
            } else {
                // JVM is still up.  Try asking it to shutdown nicely.
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Sending stop signal to JVM");
                }
                
                wrapperProtocolFunction(WRAPPER_MSG_STOP, NULL);
                
                // Allow up to 5 seconds for the application to stop itself.
                wrapperData->jState = WRAPPER_JSTATE_STOPPING;
                wrapperData->jStateTimeout = time(NULL) + 5;
            }
            wrapperData->restartRequested = FALSE;
        }
        
        // Do something depending on the wrapper state
        switch(wrapperData->wState) {
            
        case WRAPPER_WSTATE_STARTING:
            // While the wrapper is starting up, we need to ping the service 
            //  manager to reasure it that we are still alive.

            // Tell the service manager that we are starting
            wrapperReportStatus(WRAPPER_WSTATE_STARTING, 0, 1000);
            
            // If the JVM state is now STARTED, then change the wrapper state
            //  to be STARTED as well.
            if (wrapperData->jState == WRAPPER_JSTATE_STARTED) {
                wrapperData->wState = WRAPPER_WSTATE_STARTED;
                
                // Tell the service manager that we started
                wrapperReportStatus(WRAPPER_WSTATE_STARTED, 0, 0);
            }
            break;
            
        case WRAPPER_WSTATE_STARTED:
            // Just keep running.  Nothing to do here.
            break;
            
        case WRAPPER_WSTATE_STOPPING:
            // The wrapper is stopping, we need to ping the service manager
            //  to reasure it that we are still alive.
            
            // Tell the service manager that we are stopping
            wrapperReportStatus(WRAPPER_WSTATE_STOPPING, 0, 1000);
            
            // If the JVM state is now DOWN, then change the wrapper state
            //  to be STOPPED as well.
            if (wrapperData->jState == WRAPPER_JSTATE_DOWN) {
                wrapperData->wState = WRAPPER_WSTATE_STOPPED;
                
                // Don't tell the service manager that we stopped here.  That
                //	will be done when the application actually quits.
            }
            break;
            
        case WRAPPER_WSTATE_STOPPED:
            // The wrapper is ready to stop.  Nothing to be done here.  This
            //  state will exit the event loop below.
            break;
            
        default:
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unknown wState=%d", wrapperData->wState);
            break;
        }
        
        // Do something depending on the JVM state
        switch(wrapperData->jState) {
        case WRAPPER_JSTATE_DOWN:
            // The JVM can be down for one of 3 reasons.  The first is that the
            //  wrapper is just starting.  The second is that the JVM is being
            //  restarted for some reason, and the 3rd is that the wrapper is
            //  trying to shut down.
            if ((wrapperData->wState == WRAPPER_WSTATE_STARTING) ||
                (wrapperData->wState == WRAPPER_WSTATE_STARTED)) {
                // The JVM needs to be launched.
                // See if we can launch it
                if (wrapperCheckRestartTimeOK()) {
                    wrapperPauseBeforeExecute();
                    
                    // Generate a unique key to use when communicating with the JVM
                    wrapperBuildKey();
                    
                    // Generate the command used to launch the Java process
                    wrapperBuildJavaCommand();
                    
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Launching a JVM...");
                    wrapperExecute();
                    
                    // Check if the start was successful.
                    if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                        // Failed to start the JVM.  Tell the wrapper to shutdown.
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unable to start a JVM");
                        wrapperData->wState = WRAPPER_WSTATE_STOPPING;
                    } else {
                        // The JVM was launched.  We still do not know whether the
                        //  launch will be successful.  Allow <startupTimeout> seconds before giving up.
                        //  This can take quite a while if the system is heavily loaded.
                        //  (At startup for example)
                        wrapperData->jState = WRAPPER_JSTATE_LAUNCHING;
                        wrapperData->jStateTimeout = time(NULL) + wrapperData->startupTimeout;
                    }
                } else {
                    // Unable to launch another JVM.
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                               "Too many restarts within a short period of time.  No more retries.");
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                               "  There may be a configuration problem: please check the logs.");
                    wrapperData->wState = WRAPPER_WSTATE_STOPPING;
                }
            } else {
                // The wrapper is shutting down.  Do nothing.
            }

            // Reset the last ping time
            wrapperData->lastPingTime = time(NULL);
            break;
            
        case WRAPPER_JSTATE_LAUNCHING:
            // The JVM process was launched, but we have not yet received a
            //  response to a ping.

            // Make sure that the JVM process is still up and running
            if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                // The process is gone.
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeout = 0;
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "JVM exited while loading the application.");
                wrapperProtocolClose();
            } else {
                // The process is up and running.
                // We are waiting in this state until we receive a KEY packet
                //  from the JVM attempting to register.
                // Have we waited too long already
                if (time(NULL) > wrapperData->jStateTimeout) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                               "Startup failed: Timed out waiting for signal from JVM.");

                    // Give up on the JVM and start trying to kill it.
                    wrapperKillProcess();
                }
            }
            break;

        case WRAPPER_JSTATE_LAUNCHED:
            // The Java side of the wrapper code has responded to a ping.
            //  Tell the Java wrapper to start the Java application.
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Start Application.");
            }
            ret = wrapperProtocolFunction(WRAPPER_MSG_START, "start");
            if (ret < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unable to send the start command to the JVM.");

                // Give up on the JVM and start trying to kill it.
                wrapperKillProcess();
            } else {
                // Start command send.  Start waiting for the app to signal
                //  that it has started.  Allow <startupTimeout> seconds before 
                //  giving up.  A good application will send starting signals back
                //  much sooner than this as a way to extend this time if necessary.
                wrapperData->jState = WRAPPER_JSTATE_STARTING;
                wrapperData->jStateTimeout = time(NULL) + wrapperData->startupTimeout;
            }
            break;

        case WRAPPER_JSTATE_STARTING:
            // The Java application was asked to start, but we have not yet
            //  received a started signal.

            // Make sure that the JVM process is still up and running
            if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                // The process is gone.
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeout = 0;
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "JVM exited while starting the application.");
                wrapperProtocolClose();
            } else {
                // Have we waited too long already
                if (time(NULL) > wrapperData->jStateTimeout) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                               "Startup failed: Timed out waiting for signal from JVM.");

                    // Give up on the JVM and start trying to kill it.
                    wrapperKillProcess();
                } else {
                    // Keep waiting.
                }
            }
            break;

        case WRAPPER_JSTATE_STARTED:
            // The Java application is up and running, but we need to make sure
            //  that the JVM does not die or hang.  A ping is sent whenever
            //  there is less than 25 seconds left before the server is
            //  considered to be dead.  This translates to pings starting after
            //  5 seconds and allows for lost pings and responses.

            // Make sure that the JVM process is still up and running
            if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                // The process is gone.
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeout = 0;
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "JVM exited unexpectedly.");
                wrapperProtocolClose();
            } else {
                // Have we waited too long already
                if (time(NULL) > wrapperData->jStateTimeout) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                               "JVM appears hung: Timed out waiting for signal from JVM.");

                    // Give up on the JVM and start trying to kill it.
                    wrapperKillProcess();
                } else if (time(NULL) > wrapperData->lastPingTime + 5) {
                    // It is time to send another ping to the JVM
                    ret = wrapperProtocolFunction(WRAPPER_MSG_PING, "ping");
                    if (ret < 0) {
                        // Failed to send the ping.
                        if (wrapperData->isDebugging) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM Ping Failed.");
                        }
                    }
                    wrapperData->lastPingTime = time(NULL);
                } else {
                    // Do nothing.  Keep waiting.
                }
            }
            break;

        case WRAPPER_JSTATE_STOPPING:
            // The Java application was asked to stop, but we have not yet
            //  received a stopped signal.

            // Make sure that the JVM process is still up and running
            if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                // The process is gone.
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeout = 0;
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "JVM exited unexpectedly while stopping the application.");
                wrapperProtocolClose();
            } else {
                // Have we waited too long already
                if (time(NULL) > wrapperData->jStateTimeout) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                               "Shutdown failed: Timed out waiting for signal from JVM.");

                    // Give up on the JVM and start trying to kill it.
                    wrapperKillProcess();
                } else {
                    // Keep waiting.
                }
            }
            break;

        case WRAPPER_JSTATE_STOPPED:
            // A stopped signal was received from the JVM.  A good application
            //  should exit on its own.  So wait until the timeout before
            //  killing the JVM process.
            if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                // The process is gone.
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeout = 0;
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM exited normally.");
                }
                wrapperProtocolClose();
            } else {
                // Have we waited too long already
                if (time(NULL) > wrapperData->jStateTimeout) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                               "Shutdown failed: Timed out waiting for the JVM to terminate.");

                    // Give up on the JVM and start trying to kill it.
                    wrapperKillProcess();
                } else {
                    // Keep waiting.
                }
            }
            break;

        default:
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unknown jState=%d", wrapperData->jState);
            break;
        }
    } while (wrapperData->wState != WRAPPER_WSTATE_STOPPED);
}

void wrapperBuildKey() {
    int i;
    float num;
    static int seeded = FALSE;

    // Seed the randomizer
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = TRUE;
    }

    // Start by generating a key
    num = (float)strlen(keyChars);
    
    for (i = 0; i < 16; i++) {
        wrapperData->key[i] = keyChars[(int)(rand() * num / RAND_MAX)];
    }
    wrapperData->key[16] = '\0';

    //printf("Key=%s\n", wrapperData->key);
}

#ifdef WIN32
void wrapperBuildNTServiceInfo() {
    char dependencyKey[32]; // Length of "wrapper.ntservice.dependency.nn" + '\0'
    const char *dependencies[10];
    char *work;
    int len;
    int i;

    // Load the service name
    wrapperData->ntServiceName = (char *)getStringProperty(properties, "wrapper.ntservice.name", "Wrapper");

    // Load the service display name
    wrapperData->ntServiceDisplayName = (char *)getStringProperty(properties, "wrapper.ntservice.displayname", "Wrapper");

    // Load the service description
    wrapperData->ntServiceDescription = (char *)getStringProperty(properties, "wrapper.ntservice.description", wrapperData->ntServiceDisplayName);

    // *** Build the dependency list ***
    len = 0;
    for (i = 0; i < 10; i++) {
        sprintf(dependencyKey, "wrapper.ntservice.dependency.%d", i + 1);
        dependencies[i] = getStringProperty(properties, dependencyKey, NULL);
        if (dependencies[i] != NULL) {
            if (strlen(dependencies[i]) > 0) {
                len += strlen(dependencies[i]) + 1;
            } else {
                // Ignore empty values.
                dependencies[i] = NULL;
            }
        }
    }
    // List must end with a double '\0'.  If the list is not empty then it will end with 3.  But that is fine.
    len += 2;

    // Actually build the buffer
    work = wrapperData->ntServiceDependencies = (char *)malloc(sizeof(char) * len);
    for (i = 0; i < 10; i++) {
        if (dependencies[i] != NULL) {
            strcpy(work, dependencies[i]);
            work += strlen(dependencies[i]) + 1;
        }
    }
    // Add two more nulls to the end of the list.
    work[0] = '\0';
    work[1] = '\0';
    // *** Dependency list completed ***

    // Set the service start type
    if (strcmp(_strupr((char *)getStringProperty(properties, "wrapper.ntservice.starttype", "DEMAND_START")), "AUTO_START") == 0) {
        wrapperData->ntServiceStartType = SERVICE_AUTO_START;
    } else {
        wrapperData->ntServiceStartType = SERVICE_DEMAND_START;
    }
}
#endif

#ifdef SOLARIS
int wrapperBuildUnixDaemonInfo() {
    char *name;
    
    name = (char *)getStringProperty(properties, "wrapper.pidfile", NULL);
    if (name == NULL) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "No wrapper.pidfile property in wrapper config file");
        return 1;
    } else {
        wrapperData->pidFilename = name;
    }
    return 0;
}
#endif

int wrapperLoadConfiguration() {
    // Load log file
    setLogfilePath((char *)getStringProperty(properties, "wrapper.logfile", "wrapper.log"));
    
    // Load log file format
    setLogfileFormat((char *)getStringProperty(properties, "wrapper.logfile.format", "LPTM"));

    // Load log file log level
    setLogfileLevel((char *)getStringProperty(properties, "wrapper.logfile.loglevel", "INFO"));

    // Load max log filesize log level
    setLogfileMaxFileSize((char *)getStringProperty(properties, "wrapper.logfile.maxsize", "0"));

    // Load log files level
    setLogfileMaxLogFiles((char *)getStringProperty(properties, "wrapper.logfile.maxfiles", "0"));

    // Load console format
    setConsoleLogFormat((char *)getStringProperty(properties, "wrapper.console.format", "PM"));

    // Load console log level
    setConsoleLogLevel((char *)getStringProperty(properties, "wrapper.console.loglevel", "INFO"));

    // Load syslog log level
    setSyslogLevel((char *)getStringProperty(properties, "wrapper.syslog.loglevel", "NONE"));

    // Load syslog event source name
    setSyslogEventSourceName((char *)getStringProperty(properties, "wrapper.ntservice.name", "Wrapper"));

    // Register the syslog message file
    registerSyslogMessageFile( );

    // Initialize some values not loaded
    wrapperData->exitCode = 0;

    // Get the port
    wrapperData->port = getIntProperty(properties, "wrapper.port", 15003);

    // Get the debug status (Property is deprecated but flag is still used)
    wrapperData->isDebugging = getBooleanProperty(properties, "wrapper.debug", FALSE);
    if (wrapperData->isDebugging) {
        // For backwards compatability
        setConsoleLogLevelInt(LEVEL_DEBUG);
        setLogfileLevelInt(LEVEL_DEBUG);
    } else {
        if (loggerNeedsDebug()) {
            wrapperData->isDebugging = TRUE;
        }
    }
    
    // Get the shutdown hook status
    wrapperData->isShutdownHookDisabled = getBooleanProperty(properties, "wrapper.disable_shutdown_hook", FALSE);
    
    // Get the timeout settings
    wrapperData->startupTimeout = getIntProperty(properties, "wrapper.startup.timeout", 30);
    wrapperData->pingTimeout = getIntProperty(properties, "wrapper.ping.timeout", 30);
    if (wrapperData->startupTimeout <= 0) {
        wrapperData->startupTimeout = 31557600;  // One Year.  Effectively never
    }
    if (wrapperData->pingTimeout <= 0) {
        wrapperData->pingTimeout = 31557600;  // One Year.  Effectively never
    }

#ifdef WIN32
    // Configure the NT service information
    wrapperBuildNTServiceInfo();
#endif
    
#ifdef SOLARIS
    // Configure the Unix daemon information
    return (wrapperBuildUnixDaemonInfo());
#else
    return 0;
#endif
}

/******************************************************************************
 * Protocol callback functions
 *****************************************************************************/
void wrapperKeyRegistered(char *key) {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Got key from JVM: %s", key);
    }

    switch (wrapperData->jState) {
    case WRAPPER_JSTATE_LAUNCHING:
        // We now know that the Java side wrapper code has started and
        //  registered with a key.  We still need to verify that it is
        //  the correct key however.
        if (strcmp(key, wrapperData->key) == 0) {
            // This is the correct key.
            // We now know that the Java side wrapper code has started.
            wrapperData->jState = WRAPPER_JSTATE_LAUNCHED;
            wrapperData->jStateTimeout = 0;
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Received a connection request with an incorrect key.  Waiting for another connection.");

            // This was the wrong key.  Send a response.
            wrapperProtocolFunction(WRAPPER_MSG_BADKEY, "Incorrect key.  Connection rejected.");

            // Close the current connection.  Assume that the real JVM
            //  is still out there trying to connect.  So don't change
            //  the state.  If for some reason, this was the correct
            //  JVM, but the key was wrong.  then this state will time
            //  out and recover.
            wrapperProtocolClose();
        }
    default:
        // We got a key registration that we were not expecting.  Ignore it.
        break;
    }
}

void wrapperPingResponded() {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Got ping response from JVM");
    }

    // Depending on the current JVM state, do something.
    switch (wrapperData->jState) {
    case WRAPPER_JSTATE_STARTED:
        // We got a response to a ping.  Allow 5 + <pingTimeout> more seconds before the JVM
        //  is considered to be dead.
        wrapperData->jStateTimeout = time(NULL) + 5 + wrapperData->pingTimeout;
        break;
    default:
        // We got a ping response that we were not expecting.  Ignore it.
        break;
    }
}

void wrapperStopRequested(int exitCode) {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM requested a shutdown. (%d)", exitCode);
    }

    // Get things stopping on this end.  Ask the JVM to stop again in case the
    //	user code on the Java side is not written correctly.
    wrapperStopProcess(exitCode);
}

void wrapperRestartRequested() {
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "JVM requested a restart.");
    wrapperRestartProcess();
}

/**
 * If the current state of the JVM is STOPPING then this message is used to
 *	extend the time that the wrapper will wait for a STOPPED message before
 *  giving up on the JVM and killing it.
 */
void wrapperStopPendingSignalled(int waitHint) {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM signalled a stop pending with waitHint of %d millis.", waitHint);
    }

    if (wrapperData->jState == WRAPPER_JSTATE_STARTED) {
        // Change the state to STOPPING
        wrapperData->jState = WRAPPER_JSTATE_STOPPING;
    }

    if (wrapperData->jState == WRAPPER_JSTATE_STOPPING) {
        if (waitHint < 0) {
            waitHint = 0;
        }

        wrapperData->jStateTimeout = time(NULL) + (int)ceil(waitHint / 1000.0);
    }
}

/**
 * The wrapper received a signal from the JVM that it has completed the stop
 *  process.  If the state of the JVM is STOPPING, then change the state to
 *  STOPPED.  It is possible to get this request after the Wrapper has given up
 *	waiting for the JVM.  In this case, the message is ignored.
 */
void wrapperStoppedSignalled() {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM signalled that it was stopped.");
    }

    if (wrapperData->jState == WRAPPER_JSTATE_STOPPING) {
        wrapperData->jState = WRAPPER_JSTATE_STOPPED;

        // The Java side of the wrapper signalled that it stopped
        //	allow 5 seconds for the JVM to exit.
        wrapperData->jStateTimeout = time(NULL) + 5;
    }
}

/**
 * If the current state of the JVM is STARTING then this message is used to
 *	extend the time that the wrapper will wait for a STARTED message before
 *  giving up on the JVM and killing it.
 */
void wrapperStartPendingSignalled(int waitHint) {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM signalled a start pending with waitHint of %d millis.", waitHint);
    }

    // Only process the start pending signal if the JVM state is starting or
    //  stopping.  Stopping is included because if the user hits CTRL-C while
    //  the application is starting, then the stop request will not be noticed
    //  until the application has completed its startup.
    if ((wrapperData->jState == WRAPPER_JSTATE_STARTING) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOPPING)) {
        if (waitHint < 0) {
            waitHint = 0;
        }

        wrapperData->jStateTimeout = time(NULL) + (int)ceil(waitHint / 1000.0);
    }
}

/**
 * The wrapper received a signal from the JVM that it has completed the startup
 *  process.  If the state of the JVM is STARTING, then change the state to
 *  STARTED.  It is possible to get this request after the Wrapper has given up
 *	waiting for the JVM.  In this case, the message is ignored.
 */
void wrapperStartedSignalled() {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM signalled that it was started.");
    }

    if (wrapperData->jState == WRAPPER_JSTATE_STARTING) {
        wrapperData->jState = WRAPPER_JSTATE_STARTED;

        // Give the JVM 30 seconds to respond to a ping.
        wrapperData->jStateTimeout = time(NULL) + 30;

        // Is the wrapper state STARTING?
        if (wrapperData->wState == WRAPPER_WSTATE_STARTING) {
            wrapperData->wState = WRAPPER_WSTATE_STARTED;

            if (!wrapperData->isConsole) {
                // Tell the service manager that we started
                wrapperReportStatus(WRAPPER_WSTATE_STARTED, 0, 0);
            }
        }
    }
}
