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
 * Revision 1.41  2003/02/03 06:55:26  mortenson
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

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include "property.h"
#include "wrapper.h"
#include "logger.h"

#ifdef WIN32
#include <io.h>
#include <winsock.h>
#include <shlwapi.h>

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
#else
#ifdef AIX
#else
#ifdef HPUX
#else /* LINUX */
#include <asm/errno.h>
#endif /* !HPUX */
#endif /* !AIX */
#endif /* !SOLARIS */

#endif /* WIN32 */

WrapperConfig *wrapperData;
char         packetBuffer[MAX_LOG_SIZE + 1];
char         *keyChars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";

/* Properties structure loaded in from the config file. */
Properties              *properties;

/* Server Socket. */
SOCKET ssd = INVALID_SOCKET;
/* Client Socket. */
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

    /* Create the server socket. */
    ssd = socket(AF_INET, SOCK_STREAM, 0);
    if (ssd == INVALID_SOCKET) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, "server socket creation failed. (%d)", wrapperGetLastError());
        return;
    }

    /* Make the socket non-blocking */
#ifdef WIN32
    rc = ioctlsocket(ssd, FIONBIO, &dwNoBlock);
#else /* UNIX  */
    rc = fcntl(ssd, F_SETFL, O_NONBLOCK);
#endif

    if (rc == SOCKET_ERROR) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, "server socket ioctlsocket failed. (%d)", wrapperGetLastError());
        wrapperProtocolStopServer();
        return;
    }

    /* Start looking for at open server port at the given value.  Loop until a bind is successful */
    port = wrapperData->port;
    trys = 0;

  tryagain:
    /* Try binding to the port. */
    
    /* Cleanup the addr_srv first */
    memset(&addr_srv, 0, sizeof(addr_srv));
    
    addr_srv.sin_family = AF_INET;
    addr_srv.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr_srv.sin_port = htons(port);
#ifdef WIN32
    rc = bind(ssd, (struct sockaddr FAR *)&addr_srv, sizeof(addr_srv));
#else /* UNIX */
    rc = bind(ssd, (struct sockaddr *)&addr_srv, sizeof(addr_srv));
#endif
    
    if (rc == SOCKET_ERROR) {

        rc = wrapperGetLastError();
        if (rc == EADDRINUSE) {
            /* Address in use, try looking at the next one. */
            port++;
            if (port > 65000) {
                port = 10000;
            }
            if (trys < 100) {
                trys++;
                goto tryagain;
            }
        }

        /* Log an error.  This is fatal, so die. */
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_FATAL, "unable to bind listener port %d. (%d)", wrapperData->port, wrapperGetLastError());

        wrapperStopProcess(rc);

        wrapperProtocolStopServer();
        return;
    }

    /* If we got here, then we are bound to the port */
    if (port != wrapperData->port) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_INFO, "port %d already in use, using port %d instead.", wrapperData->port, port);
    }
    wrapperData->actualPort = port;

    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "server listening on port %d.", wrapperData->actualPort);
    }

    /* Tell the socket to start listening. */
    rc = listen(ssd, 1);
    if (rc == SOCKET_ERROR) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, "server socket listen failed. (%d)", wrapperGetLastError());
        wrapperProtocolStopServer();
        return;
    }
}

void wrapperProtocolStopServer() {
    int rc;
    /* Close the socket. */
    if (ssd != INVALID_SOCKET) {
#ifdef WIN32
        rc = closesocket(ssd);
#else /* UNIX */
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

    /* Close any client connection that may be open */
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

    /* Is the server socket open? */
    if (ssd == INVALID_SOCKET) {
        /* can't do anything yet. */
        return;
    }

    /* Is it already open? */
    if (sd != INVALID_SOCKET) {
        return;
    }

    /* Try accepting a socket. */
    addr_srv_len = sizeof(addr_srv);
#ifdef WIN32
    sd = accept(ssd, (struct sockaddr FAR *)&addr_srv, &addr_srv_len);
#else /* UNIX */
    sd = accept(ssd, (struct sockaddr *)&addr_srv, (socklen_t *)&addr_srv_len);
#endif
    if (sd == INVALID_SOCKET) {
        rc = wrapperGetLastError();
        if (rc == EWOULDBLOCK) {
            /* There are no incomming sockets right now. */
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

    /* Make the socket non-blocking */
#ifdef WIN32
    rc = ioctlsocket(sd, FIONBIO, &dwNoBlock);
#else /* UNIX */
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

    /* Close the socket. */
    if (sd != INVALID_SOCKET) {
#ifdef WIN32
        rc = closesocket(sd);
#else /* UNIX */
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

    /* Open the socket if necessary */
    wrapperProtocolOpen();

    if (sd == INVALID_SOCKET) {
        /* A socket was not opened */
        return -1;
    }

    /* Build the packet */
    buffer[0] = function;
    if (message == NULL) {
        buffer[1] = '\0';
        len = 2;
    } else {
        len = strlen(message);
        strcpy((char*)(buffer + 1), message);
        len += 2;
    }

    /* Send the packet */
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

/**
 * Read any data sent from the JVM.  This function will loop and read as many
 *  packets are available.  The loop will only be allowed to go for 250ms to
 *  ensure that other functions are handled correctly.
 */
int wrapperProtocolRead() {
    char c, code;
    int len;
    int pos;
    int err;
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

    while((durr = (now - startTime) * 1000 + (nowMillis - startTimeMillis)) < 250) {
        /*
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "durr=%ld", durr);
        */

        /* If we have an open client socket, then use it. */
        if (sd == INVALID_SOCKET) {
            /* A Client socket is not open */

            /* Is the server socket open? */
            if (ssd == INVALID_SOCKET) {
                wrapperProtocolStartServer();
                if (ssd == INVALID_SOCKET) {
                    /* Failed. */
                    return FALSE;
                }
            }

            /* Try accepting a socket */
            wrapperProtocolOpen();
            if (sd == INVALID_SOCKET) {
                return FALSE;
            }
        }

        /* Try receiving a packet code */
        len = recv(sd, &c, 1, 0);
        if (len == SOCKET_ERROR) {
            err = wrapperGetLastError();
            if (wrapperData->isDebugging) {
                if ((err != EWOULDBLOCK) && (err != ENOTSOCK) && (err != ECONNRESET)) {
                    log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "socket read failed. (%d)", err);
                    wrapperProtocolClose();
                }
            }
            /*
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "no data");
            */
            return FALSE;	
        } else if (len != 1) {
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "socket read no code (closed?).");
            }
            wrapperProtocolClose();
            return FALSE;	
        }

        code = c;

        /* Read in any message */
        pos = 0;
        do {
            len = recv(sd, &c, 1, 0);
            if (len == 1) {
                if (c == 0) {
                    /* End of string */
                    len = 0;
                } else if (pos < MAX_LOG_SIZE) {
                    packetBuffer[pos] = c;
                    pos++;
                }
            } else {
                len = 0;
            }
        } while (len == 1);
        /* terminate the string; */
        packetBuffer[pos] = '\0';

        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "read a packet %d : %s", code, packetBuffer);
        }

        switch (code) {
        case WRAPPER_MSG_STOP:
            wrapperStopRequested(atoi(packetBuffer));
            break;

        case WRAPPER_MSG_RESTART:
            wrapperRestartRequested();
            break;

        case WRAPPER_MSG_PING:
            wrapperPingResponded();
            break;

        case WRAPPER_MSG_STOP_PENDING:
            wrapperStopPendingSignalled(atoi(packetBuffer));
            break;

        case WRAPPER_MSG_STOPPED:
            wrapperStoppedSignalled();
            break;

        case WRAPPER_MSG_START_PENDING:
            wrapperStartPendingSignalled(atoi(packetBuffer));
            break;

        case WRAPPER_MSG_STARTED:
            wrapperStartedSignalled();
            break;

        case WRAPPER_MSG_KEY:
            wrapperKeyRegistered(packetBuffer);
            break;

        case WRAPPER_MSG_LOG + LEVEL_DEBUG:
        case WRAPPER_MSG_LOG + LEVEL_INFO:
        case WRAPPER_MSG_LOG + LEVEL_STATUS:
        case WRAPPER_MSG_LOG + LEVEL_WARN:
        case WRAPPER_MSG_LOG + LEVEL_ERROR:
        case WRAPPER_MSG_LOG + LEVEL_FATAL:
            wrapperLogSignalled(code - WRAPPER_MSG_LOG, packetBuffer);
            break;

        default:
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "received unknown packet (%d:%s)", code, packetBuffer);
            }
            break;
        }

        /* Get the time again */
        ftime( &timeBuffer );
        now = timeBuffer.time;
        nowMillis = timeBuffer.millitm;
    }
    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "done durr=%ld", durr);
    */
    return TRUE;
}



/******************************************************************************
 * Wrapper inner methods.
 *****************************************************************************/

/**
 * Initialize logging.
 */
void wrapperInitializeLogging() {
    setLogfilePath("wrapper.log");
    setLogfileFormat("LPTM");
    setLogfileLevelInt(LEVEL_DEBUG);
    setConsoleLogFormat("LPM");
    setConsoleLogLevelInt(LEVEL_DEBUG);
    setSyslogLevelInt(LEVEL_NONE);
}

/**
 * Launch the wrapper as a console application.
 */
int wrapperRunConsole() {
    int res;

    /* Setup the wrapperData structure. */
    wrapperData->wState = WRAPPER_WSTATE_STARTING;
    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->isConsole = TRUE;

    /* Initialize the wrapper */
    res = wrapperInitialize();
    if (res != 0) {
        return res;
    }

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "--> Wrapper Started as Console");

    /* Enter main event loop */
    wrapperEventLoop();

    /* Clean up any open sockets. */
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

    /* Setup the wrapperData structure. */
    wrapperData->wState = WRAPPER_WSTATE_STARTING;
    wrapperData->jState = WRAPPER_JSTATE_DOWN;
    wrapperData->isConsole = FALSE;

    /* Initialize the wrapper */
    res = wrapperInitialize();
    if (res != 0) {
        return res;
    }

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "--> Wrapper Started as Service");

    /* Enter main event loop */
    wrapperEventLoop();

    /* Clean up any open sockets. */
    wrapperProtocolStopServer();
    wrapperProtocolClose();

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "<-- Wrapper Stopped");

    return wrapperData->exitCode;
}

/**
 * Used to ask the state engine to shut down the JVM and Wrapper
 */
void wrapperStopProcess(int exitCode) {
    /* If it has not already been set, set the exit request flag in the wrapper data. */
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
    /* If it has not already been set, set the restart request flag in the wrapper data. */
    if (!wrapperData->restartRequested) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "wrapperRestartProcess() called.");
        }

        wrapperData->restartRequested = TRUE;

        /* This restart was intentional, so make the JVM appear to have been running for */
        /*  a long time so the JVM will not be considered a failed launch.               */
        wrapperData->jvmLaunchTime = 0;
    } else {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "wrapperRestartProcess() called.  (IGNORED)");
        }
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
    int len, found;
    struct _finddata_t fblock;
#else
    glob_t g;
    int findex;
#endif

    index = 0;

    /* Java commnd */
    if (strings) {
        prop = getStringProperty(properties, "wrapper.java.command", "java");

#ifdef WIN32
        found = 0;

        /* To avoid problems on Windows XP systems, the '/' characters must
         *  be replaced by '\' characters in the specified path. */
        c = (char *)prop;
        while((c = strchr(c, '/')) != NULL) {
            c[0] = '\\';
        }

        /* If the full path to the java command was not specified, then we
         *  need to try and resolve it here to avoid problems later when
         *  calling CreateProcess.  CreateProcess will look in the windows
         *  system directory before searching the PATH.  This can lead to
         *  the wrong JVM being run. */
        sprintf(cpPath, "%s", prop);
        if ((PathFindOnPath((LPSTR)cpPath, (LPCSTR *)wrapperGetSystemPath())) && (!PathIsDirectory(cpPath))) {
            /*printf("Found %s on path.\n", cpPath); */
            found = 1;
        } else {
            /*printf("Could not find %s on path.\n", cpPath); */

            /* Try adding .exe to the end */
            sprintf(cpPath, "%s.exe", prop);
            if ((PathFindOnPath(cpPath, wrapperGetSystemPath())) && (!PathIsDirectory(cpPath))) {
                /*printf("Found %s on path.\n", cpPath); */
                found = 1;
            } else {
                /*printf("Could not find %s on path.\n", cpPath); */
            }
        }

        if (found) {
            strings[index] = (char *)malloc(sizeof(char) * (strlen(cpPath) + 2 + 1));
            if (addQuotes) {
                sprintf(strings[index], "\"%s\"", cpPath);
            } else {
                sprintf(strings[index], "%s", cpPath);
            }
        } else {
            strings[index] = (char *)malloc(sizeof(char) * (strlen(prop) + 2 + 1));
            if (addQuotes) {
                sprintf(strings[index], "\"%s\"", prop);
            } else {
                sprintf(strings[index], "%s", prop);
            }
        }

#else /* UNIX */

        strings[index] = (char *)malloc(sizeof(char) * (strlen(prop) + 2 + 1));
        if (addQuotes) {
            sprintf(strings[index], "\"%s\"", prop);
        } else {
            sprintf(strings[index], "%s", prop);
        }
#endif
    }
    index++;

    /* Store additional java parameters */
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

    /* Initial JVM memory */
    if (strings) {
        initMemory = __min(__max(getIntProperty(properties, "wrapper.java.initmemory", 8), 8), 4096); /* 8 <= n <= 4096 */
        strings[index] = (char *)malloc(sizeof(char) * (5 + 4 + 1));  /* Allow up to 4 digits. */
        sprintf(strings[index], "-Xms%dm", initMemory);
    }
    index++;

    /* Maximum JVM memory */
    if (strings) {
        maxMemory = __min(__max(getIntProperty(properties, "wrapper.java.maxmemory", 128), initMemory), 4096);  /* initMemory <= n <= 4096 */
        strings[index] = (char *)malloc(sizeof(char) * (5 + 4 + 1));  /* Allow up to 4 digits. */
        sprintf(strings[index], "-Xmx%dm", maxMemory);
    }
    index++;

    /* Library Path */
    if (strings) {
        prop = getStringProperty(properties, "wrapper.java.library.path", NULL);
        if (prop) {
            /* An old style library path was specified. */
            strings[index] = (char *)malloc(sizeof(char) * (22 + strlen(prop) + 1));
            if (addQuotes) {
                sprintf(strings[index], "-Djava.library.path=\"%s\"", prop);
            } else {
                sprintf(strings[index], "-Djava.library.path=%s", prop);
            }
        } else {
            /* Look for a multiline library path. */
            cpLen = 0;
            cpLenAlloc = 1024;
            strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
            
            /* Start with the property value. */
            sprintf(&(strings[index][cpLen]), "-Djava.library.path=");
            cpLen += 20;
            
            /* Add an open quote to the library path */
            if (addQuotes) {
                sprintf(&(strings[index][cpLen]), "\"");
                cpLen++;
            }
            
            /* Loop over the library path entries adding each one */
            i = 0;
            j = 0;
            do {
                sprintf(paramBuffer, "wrapper.java.library.path.%d", i + 1);
                prop = getStringProperty(properties, paramBuffer, NULL);
                if (prop) {
                    len2 = strlen(prop);
                    if (len2 > 0) {
                        /* Is there room for the entry? */
                        if (cpLen + len2 + 3 > cpLenAlloc) {
                            /* Resize the buffer */
                            tmpString = strings[index];
                            cpLenAlloc += 1024;
                            strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
                            sprintf(strings[index], tmpString);
                            free(tmpString);
                        }
                        
                        if (j > 0) {
                            strings[index][cpLen++] = wrapperClasspathSeparator; /* separator */
                        }
                        sprintf(&(strings[index][cpLen]), prop);
                        cpLen += len2;
                        j++;
                    }
                    i++;
                }
            } while (prop);
            if (j == 0) {
                /* No library path, use default. always room */
                sprintf(&(strings[index][cpLen++]), "./");
            }
            /* Add ending quote */
            if (addQuotes) {
                sprintf(&(strings[index][cpLen]), "\"");
                cpLen++;
            }
        }
    }
    index++;

    /* Store the classpath */
    if (strings) {
        strings[index] = (char *)malloc(sizeof(char) * (10 + 1));
        sprintf(strings[index], "-classpath");
    }
    index++;
    if (strings) {
        /* Build a classpath */
        cpLen = 0;
        cpLenAlloc = 1024;
        strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
        
        /* Add an open quote the classpath */
        if (addQuotes) {
            sprintf(&(strings[index][cpLen]), "\"");
            cpLen++;
        }

        /* Loop over the classpath entries adding each one */
        i = 0;
        j = 0;
        do {
            sprintf(paramBuffer, "wrapper.java.classpath.%d", i + 1);
            prop = getStringProperty(properties, paramBuffer, NULL);
            if (prop) {
                len2 = strlen(prop);
                if (len2 > 0) {
                    /* Does this contain wildcards? */
                    if ((strchr(prop, '*') != NULL) || (strchr(prop, '?') != NULL)) {
                        /* Need to do a wildcard search */
#ifdef WIN32
                        /* Extract any path information of the beginning of the file */
                        strcpy(cpPath, prop);
                        c = max(strrchr(cpPath, '\\'), strrchr(cpPath, '/'));
                        if (c == NULL) {
                            cpPath[0] = '\0';
                        } else {
                            c[1] = '\0'; /* terminate after the slash */
                        }
                        len = strlen(cpPath);

                        if ((handle = _findfirst(prop, &fblock)) <= 0) {
                            if (errno == ENOENT) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Warning no matching files for classpath element: %s", prop);
                            } else {
                                /* Encountered an error of some kind. */
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Error in findfirst for classpath element: %s", prop);
                            }
                        } else {
                            len2 = strlen(fblock.name);

                            /* Is there room for the entry? */
                            if (cpLen + len + len2 + 3 > cpLenAlloc) {
                                /* Resize the buffer */
                                tmpString = strings[index];
                                cpLenAlloc += 1024;
                                strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
                                sprintf(strings[index], tmpString);
                                free(tmpString);
                            }

                            if (j > 0) {
                                strings[index][cpLen++] = wrapperClasspathSeparator; /* separator */
                            }
                            sprintf(&(strings[index][cpLen]), "%s%s", cpPath, fblock.name);
                            cpLen += (len + len2);
                            j++;

                            /* Look for additional entries */
                            while (_findnext(handle, &fblock) == 0) {
                                len2 = strlen(fblock.name);

                                /* Is there room for the entry? */
                                if (cpLen + len + len2 + 3 > cpLenAlloc) {
                                    /* Resize the buffer */
                                    tmpString = strings[index];
                                    cpLenAlloc += 1024;
                                    strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
                                    sprintf(strings[index], tmpString);
                                    free(tmpString);
                                }

                                if (j > 0) {
                                    strings[index][cpLen++] = wrapperClasspathSeparator; /* separator */
                                }
                                sprintf(&(strings[index][cpLen]), "%s%s", cpPath, fblock.name);
                                cpLen += (len + len2);
                                j++;
                            }

                            /* Close the file search */
                            _findclose(handle);
                        }
#else
                        /* Wildcard support for unix */
                        glob(prop, GLOB_MARK | GLOB_NOSORT, NULL, &g);

                        if( g.gl_pathc > 0 ) {
                            for( findex=0; findex<g.gl_pathc; findex++ ) {
                                /* Is there room for the entry? */
                                len2 = strlen(g.gl_pathv[findex]);
                                if (cpLen + len2 + 3 > cpLenAlloc) {
                                    /* Resize the buffer */
                                    tmpString = strings[index];
                                    cpLenAlloc += 1024;
                                    strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
                                    sprintf(strings[index], tmpString);
                                    free(tmpString);
                                }

                                if (j > 0) {
                                    strings[index][cpLen++] = wrapperClasspathSeparator; /* separator */
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
                        /* Is there room for the entry? */
                        if (cpLen + len2 + 3 > cpLenAlloc) {
                            /* Resize the buffer */
                            tmpString = strings[index];
                            cpLenAlloc += 1024;
                            strings[index] = (char *)malloc(sizeof(char) * cpLenAlloc);
                            sprintf(strings[index], tmpString);
                            free(tmpString);
                        }

                        if (j > 0) {
                            strings[index][cpLen++] = wrapperClasspathSeparator; /* separator */
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
            /* No classpath, use default. always room */
            sprintf(&(strings[index][cpLen++]), "./");
        }
        /* Add ending quote */
        if (addQuotes) {
            sprintf(&(strings[index][cpLen]), "\"");
            cpLen++;
        }
    }
    index++;

    /* Store the Wrapper key */
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

    /* Store the Wrapper server port */
    if (strings) {
        strings[index] = (char *)malloc(sizeof(char) * (15 + 5 + 1));  /* Port up to 5 characters */
        sprintf(strings[index], "-Dwrapper.port=%d", (int)wrapperData->actualPort);
    }
    index++;

    /* Store the Wrapper debug flag */
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

    /* If this is being run as a service, add a service flag. */
    if (!wrapperData->isConsole) {
        if (strings) {
            strings[index] = (char *)malloc(sizeof(char) * (24 + 1));
            if (addQuotes) {
                sprintf(strings[index], "-Dwrapper.service=\"TRUE\"");
            } else {
                sprintf(strings[index], "-Dwrapper.service=TRUE");
            }
        }
        index++;
    }

    /* Store the Disable Shutdown Hook flag */
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

    /* Store the CPU Timeout value */
    if (strings) {
        /* Just to be safe, allow 20 characters for the timeout value */
        strings[index] = (char *)malloc(sizeof(char) * (24 + 20 + 1));
        if (addQuotes) {
            sprintf(strings[index], "-Dwrapper.cpu.timeout=\"%d\"", wrapperData->cpuTimeout);
        } else {
            sprintf(strings[index], "-Dwrapper.cpu.timeout=%d", wrapperData->cpuTimeout);
        }
    }
    index++;

    /* Store the Wrapper JVM ID.  (Get here before incremented) */
    if (strings) {
        strings[index] = (char *)malloc(sizeof(char) * (16 + 5 + 1));  /* jvmid up to 5 characters */
        sprintf(strings[index], "-Dwrapper.jvmid=%d", (wrapperData->jvmRestarts + 1));
    }
    index++;

    /* Store the main class */
    if (strings) {
        prop = getStringProperty(properties, "wrapper.java.mainclass", "Main");
        strings[index] = (char *)malloc(sizeof(char) * (strlen(prop) + 1));
        sprintf(strings[index], "%s", prop);
    }
    index++;

    /* Store any application parameters */
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
    /* Find out how long the array needs to be first. */
    *length = wrapperBuildJavaCommandArrayInner(NULL, addQuotes);

    /* Allocate the correct amount of memory */
    *stringsPtr = (char **)malloc(sizeof(char *) * (*length));

    /* Now actually fill in the strings */
    wrapperBuildJavaCommandArrayInner(*stringsPtr, addQuotes);
}

void wrapperFreeJavaCommandArray(char **strings, int length) {
    int i;

    if (strings != NULL) {
        /* Loop over and free each of the strings in the array */
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
    time_t now;
    time_t lastCycleTime = time(NULL);

    do {
        /* Sleep for a quarter second. */
#ifdef WIN32
        Sleep(250);     /* milliseconds */
#else /* UNIX */
        usleep(250000); /* microseconds */
#endif

        /* Check the stout pipe of the child process. */
        wrapperReadChildOutput();
        
        /* Check for incoming data packets. */
        wrapperProtocolRead();
        
        /* Get the current time for use in this cycle. */
        now = time(NULL);

        /* Has the process been getting CPU? */
        if (now - lastCycleTime > wrapperData->cpuTimeout ) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO,
                "Wrapper Process has not received any CPU time for %d seconds.  Extending timeouts.",
                now - lastCycleTime);

            if (wrapperData->jStateTimeout > 0) {
                wrapperData->jStateTimeout = wrapperData->jStateTimeout + (now - lastCycleTime);
            }
        }
        lastCycleTime = now;

        /* Useful for development debugging, but not runtime debugging */
        if (wrapperData->isStateOutputEnabled) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                       "    WrapperState=%s, JVMState=%s JVMStateTimeout=%d",
                       wrapperGetWState(wrapperData->wState),
                       wrapperGetJState(wrapperData->jState),
                       (wrapperData->jStateTimeout == 0 ? 
                        0 : wrapperData->jStateTimeout - now));
        }
        
        if ((wrapperData->exitRequested && (! wrapperData->exitAcknowledged))
            || wrapperData->restartRequested) {
            
            if (wrapperData->exitRequested) {
                
                /* Acknowledge that we have seen the exit request. */
                wrapperData->exitAcknowledged = TRUE;
                
                /* If the state of the wrapper is not STOPPING or STOPPED, then */
                /*	set it to STOPPING */
                if ((wrapperData->wState != WRAPPER_WSTATE_STOPPING) &&
                    (wrapperData->wState != WRAPPER_WSTATE_STOPPED)) {
                    wrapperData->wState = WRAPPER_WSTATE_STOPPING;
                    wrapperData->exitCode = wrapperData->exitCode;
                }
            }
            
            if ((wrapperData->jState == WRAPPER_JSTATE_DOWN) || (wrapperData->jState == WRAPPER_JSTATE_LAUNCH)) {
                /** A JVM is not currently running. Nothing to do.*/
            } else {
                /* The JVM should be running, so it needs to be stopped. */
                if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                    /* JVM Process is gone */
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM shut down unexpectedly.");
                    wrapperData->jState = WRAPPER_JSTATE_DOWN;
                    wrapperData->jStateTimeout = 0;
                    wrapperProtocolClose();
                } else {
                    /* JVM is still up.  Try asking it to shutdown nicely. */
                    if (wrapperData->isDebugging) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Sending stop signal to JVM");
                    }
                
                    wrapperProtocolFunction(WRAPPER_MSG_STOP, NULL);
                
                    /* Allow up to 5 + <shutdownTimeout> seconds for the application to stop itself. */
                    wrapperData->jState = WRAPPER_JSTATE_STOPPING;
                    wrapperData->jStateTimeout = now + 5 + wrapperData->shutdownTimeout;
                }
            }
            wrapperData->restartRequested = FALSE;
        }
        
        /* Do something depending on the wrapper state */
        switch(wrapperData->wState) {
            
        case WRAPPER_WSTATE_STARTING:
            /* While the wrapper is starting up, we need to ping the service  */
            /*  manager to reasure it that we are still alive. */

            /* Tell the service manager that we are starting */
            wrapperReportStatus(WRAPPER_WSTATE_STARTING, 0, 1000);
            
            /* If the JVM state is now STARTED, then change the wrapper state */
            /*  to be STARTED as well. */
            if (wrapperData->jState == WRAPPER_JSTATE_STARTED) {
                wrapperData->wState = WRAPPER_WSTATE_STARTED;
                
                /* Tell the service manager that we started */
                wrapperReportStatus(WRAPPER_WSTATE_STARTED, 0, 0);
            }
            break;
            
        case WRAPPER_WSTATE_STARTED:
            /* Just keep running.  Nothing to do here. */
            break;
            
        case WRAPPER_WSTATE_STOPPING:
            /* The wrapper is stopping, we need to ping the service manager */
            /*  to reasure it that we are still alive. */
            
            /* Tell the service manager that we are stopping */
            wrapperReportStatus(WRAPPER_WSTATE_STOPPING, 0, 1000);
            
            /* If the JVM state is now DOWN, then change the wrapper state */
            /*  to be STOPPED as well. */
            if (wrapperData->jState == WRAPPER_JSTATE_DOWN) {
                wrapperData->wState = WRAPPER_WSTATE_STOPPED;
                
                /* Don't tell the service manager that we stopped here.  That */
                /*	will be done when the application actually quits. */
            }
            break;
            
        case WRAPPER_WSTATE_STOPPED:
            /* The wrapper is ready to stop.  Nothing to be done here.  This */
            /*  state will exit the event loop below. */
            break;
            
        default:
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unknown wState=%d", wrapperData->wState);
            break;
        }
        
        /* Do something depending on the JVM state */
        switch(wrapperData->jState) {
        case WRAPPER_JSTATE_DOWN:
            /* The JVM can be down for one of 3 reasons.  The first is that the
             *  wrapper is just starting.  The second is that the JVM is being
             *  restarted for some reason, and the 3rd is that the wrapper is
             *  trying to shut down. */
            if ((wrapperData->wState == WRAPPER_WSTATE_STARTING) ||
                (wrapperData->wState == WRAPPER_WSTATE_STARTED)) {
                /* A JVM needs to be launched. Decide on a time to wait before launching the JVM. */
                if (wrapperData->jvmRestarts > 0) {
                    /* This is not the first JVM, so make sure that we still want to launch. */
                    if (now - wrapperData->jvmLaunchTime >= wrapperData->successfulInvocationTime) {
                        /* The previous JVM invocation was running long enough that its invocation */
                        /*   should be considered a success.  Reset the failedInvocationStart to   */
                        /*   start the count fresh.                                                */
                        wrapperData->failedInvocationCount = 0;

                        /* Set the state to launch after the restart delay. */
                        wrapperData->jState = WRAPPER_JSTATE_LAUNCH;
                        wrapperData->jStateTimeout = now + wrapperData->restartDelay;
                    } else {
                        /* The last JVM invocation died quickly and was considered to have */
                        /*  been a faulty launch.  Increase the failed count.              */
                        wrapperData->failedInvocationCount++;

                        if (wrapperData->isDebugging) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, 
                                "JVM was only running for %d seconds leading to a failed restart count of %d.",
                                (now - wrapperData->jvmLaunchTime), wrapperData->failedInvocationCount);
                        }

                        /* See if we are allowed to try restarting the JVM again. */
                        if (wrapperData->failedInvocationCount < wrapperData->maxFailedInvocations) {
                            /* Try reslaunching the JVM */

                            /* Set the state to launch after the restart delay. */
                            wrapperData->jState = WRAPPER_JSTATE_LAUNCH;
                            wrapperData->jStateTimeout = now + wrapperData->restartDelay;
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
                    /* This will me the first invocation. */
                    wrapperData->failedInvocationCount = 0;

                    /* Set the state to launch immediately. */
                    wrapperData->jState = WRAPPER_JSTATE_LAUNCH;
                    wrapperData->jStateTimeout = now;
                }
            } else {
                /* The wrapper is shutting down.  Do nothing. */
            }

            /* Reset the last ping time */
            wrapperData->lastPingTime = now;
            break;
            
        case WRAPPER_JSTATE_LAUNCH:
            /* The Waiting state is set from the DOWN state if a JVM had
             *  previously been launched the Wrapper will wait in this state
             *  until the restart delay has expired.  If this was the first
             *  invocation, then the state timeout will be set to the current
             *  time causing the new JVM to be launced immediately. */
            if ((wrapperData->wState == WRAPPER_WSTATE_STARTING) ||
                (wrapperData->wState == WRAPPER_WSTATE_STARTED)) {

                /* Is it time to proceed? */
                if (now > wrapperData->jStateTimeout) {
                    /* Launch the new JVM */

                    /* Set the launch time to the curent time */
                    wrapperData->jvmLaunchTime = time(NULL);

                    /* Generate a unique key to use when communicating with the JVM */
                    wrapperBuildKey();
                
                    /* Generate the command used to launch the Java process */
                    wrapperBuildJavaCommand();
                
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Launching a JVM...");
                    wrapperExecute();
                
                    /* Check if the start was successful. */
                    if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                        /* Failed to start the JVM.  Tell the wrapper to shutdown. */
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unable to start a JVM");
                        wrapperData->wState = WRAPPER_WSTATE_STOPPING;
                    } else {
                        /* The JVM was launched.  We still do not know whether the
                         *  launch will be successful.  Allow <startupTimeout> seconds before giving up.
                         *  This can take quite a while if the system is heavily loaded.
                         *  (At startup for example) */
                        wrapperData->jState = WRAPPER_JSTATE_LAUNCHING;
                        wrapperData->jStateTimeout = now + wrapperData->startupTimeout;
                    }
                }
            } else {
                /* The wrapper is shutting down.  Switch to the down state. */
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
            }
            break;

        case WRAPPER_JSTATE_LAUNCHING:
            /* The JVM process was launched, but we have not yet received a */
            /*  response to a ping. */

            /* Make sure that the JVM process is still up and running */
            if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                /* The process is gone. */
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeout = 0;
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "JVM exited while loading the application.");
                wrapperProtocolClose();
            } else {
                /* The process is up and running.
                 * We are waiting in this state until we receive a KEY packet
                 *  from the JVM attempting to register.
                 * Have we waited too long already */
                if (now > wrapperData->jStateTimeout) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                               "Startup failed: Timed out waiting for signal from JVM.");

                    /* Give up on the JVM and start trying to kill it. */
                    wrapperKillProcess();
                }
            }
            break;

        case WRAPPER_JSTATE_LAUNCHED:
            /* The Java side of the wrapper code has responded to a ping.
             *  Tell the Java wrapper to start the Java application. */
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Start Application.");
            }
            ret = wrapperProtocolFunction(WRAPPER_MSG_START, "start");
            if (ret < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unable to send the start command to the JVM.");

                /* Give up on the JVM and start trying to kill it. */
                wrapperKillProcess();
            } else {
                /* Start command send.  Start waiting for the app to signal
                 *  that it has started.  Allow <startupTimeout> seconds before 
                 *  giving up.  A good application will send starting signals back
                 *  much sooner than this as a way to extend this time if necessary. */
                wrapperData->jState = WRAPPER_JSTATE_STARTING;
                wrapperData->jStateTimeout = now + wrapperData->startupTimeout;
            }
            break;

        case WRAPPER_JSTATE_STARTING:
            /* The Java application was asked to start, but we have not yet
             *  received a started signal. */

            /* Make sure that the JVM process is still up and running */
            if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                /* The process is gone. */
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeout = 0;
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "JVM exited while starting the application.");
                wrapperProtocolClose();
            } else {
                /* Have we waited too long already */
                if (now > wrapperData->jStateTimeout) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                               "Startup failed: Timed out waiting for signal from JVM.");

                    /* Give up on the JVM and start trying to kill it. */
                    wrapperKillProcess();
                } else {
                    /* Keep waiting. */
                }
            }
            break;

        case WRAPPER_JSTATE_STARTED:
            /* The Java application is up and running, but we need to make sure
             *  that the JVM does not die or hang.  A ping is sent whenever
             *  there is less than 25 seconds left before the server is
             *  considered to be dead.  This translates to pings starting after
             *  5 seconds and allows for lost pings and responses. */

            /* Make sure that the JVM process is still up and running */
            if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                /* The process is gone. */
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeout = 0;
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "JVM exited unexpectedly.");
                wrapperProtocolClose();
            } else {
                /* Have we waited too long already */
                if (now > wrapperData->jStateTimeout) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                               "JVM appears hung: Timed out waiting for signal from JVM.");

                    /* Give up on the JVM and start trying to kill it. */
                    wrapperKillProcess();
                } else if (now > wrapperData->lastPingTime + 5) {
                    /* It is time to send another ping to the JVM */
                    ret = wrapperProtocolFunction(WRAPPER_MSG_PING, "ping");
                    if (ret < 0) {
                        /* Failed to send the ping. */
                        if (wrapperData->isDebugging) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM Ping Failed.");
                        }
                    }
                    wrapperData->lastPingTime = now;
                } else {
                    /* Do nothing.  Keep waiting. */
                }
            }
            break;

        case WRAPPER_JSTATE_STOPPING:
            /* The Java application was asked to stop, but we have not yet
             *  received a stopped signal. */

            /* Make sure that the JVM process is still up and running */
            if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                /* The process is gone. */
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeout = 0;
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                           "JVM exited unexpectedly while stopping the application.");
                wrapperProtocolClose();
            } else {
                /* Have we waited too long already */
                if (now > wrapperData->jStateTimeout) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                               "Shutdown failed: Timed out waiting for signal from JVM.");

                    /* Give up on the JVM and start trying to kill it. */
                    wrapperKillProcess();
                } else {
                    /* Keep waiting. */
                }
            }
            break;

        case WRAPPER_JSTATE_STOPPED:
            /* A stopped signal was received from the JVM.  A good application
             *  should exit on its own.  So wait until the timeout before
             *  killing the JVM process. */
            if (wrapperGetProcessStatus() == WRAPPER_PROCESS_DOWN) {
                /* The process is gone. */
                wrapperData->jState = WRAPPER_JSTATE_DOWN;
                wrapperData->jStateTimeout = 0;
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM exited normally.");
                }
                wrapperProtocolClose();
            } else {
                /* Have we waited too long already */
                if (now > wrapperData->jStateTimeout) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                               "Shutdown failed: Timed out waiting for the JVM to terminate.");

                    /* Give up on the JVM and start trying to kill it. */
                    wrapperKillProcess();
                } else {
                    /* Keep waiting. */
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

    /* Seed the randomizer */
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = TRUE;
    }

    /* Start by generating a key */
    num = (float)strlen(keyChars);
    
    for (i = 0; i < 16; i++) {
        wrapperData->key[i] = keyChars[(int)(rand() * num / RAND_MAX)];
    }
    wrapperData->key[16] = '\0';

    /*printf("Key=%s\n", wrapperData->key); */
}

#ifdef WIN32
void wrapperBuildNTServiceInfo() {
    char dependencyKey[32]; /* Length of "wrapper.ntservice.dependency.nn" + '\0' */
    const char *dependencies[10];
    char *work;
    int len;
    int i;

    /* Load the service name */
    wrapperData->ntServiceName = (char *)getStringProperty(properties, "wrapper.ntservice.name", "Wrapper");

    /* Load the service display name */
    wrapperData->ntServiceDisplayName = (char *)getStringProperty(properties, "wrapper.ntservice.displayname", "Wrapper");

    /* Load the service description, default to nothing */
    wrapperData->ntServiceDescription = (char *)getStringProperty(properties, "wrapper.ntservice.description", "");

    /* *** Build the dependency list *** */
    len = 0;
    for (i = 0; i < 10; i++) {
        sprintf(dependencyKey, "wrapper.ntservice.dependency.%d", i + 1);
        dependencies[i] = getStringProperty(properties, dependencyKey, NULL);
        if (dependencies[i] != NULL) {
            if (strlen(dependencies[i]) > 0) {
                len += strlen(dependencies[i]) + 1;
            } else {
                /* Ignore empty values. */
                dependencies[i] = NULL;
            }
        }
    }
    /* List must end with a double '\0'.  If the list is not empty then it will end with 3.  But that is fine. */
    len += 2;

    /* Actually build the buffer */
    work = wrapperData->ntServiceDependencies = (char *)malloc(sizeof(char) * len);
    for (i = 0; i < 10; i++) {
        if (dependencies[i] != NULL) {
            strcpy(work, dependencies[i]);
            work += strlen(dependencies[i]) + 1;
        }
    }
    /* Add two more nulls to the end of the list. */
    work[0] = '\0';
    work[1] = '\0';
    /* *** Dependency list completed *** */
    /* Memory allocated in work is stored in wrapperData.  The memory should not be released here. */
    work = NULL;


    /* Set the service start type */
    if (strcmp(_strupr((char *)getStringProperty(properties, "wrapper.ntservice.starttype", "DEMAND_START")), "AUTO_START") == 0) {
        wrapperData->ntServiceStartType = SERVICE_AUTO_START;
    } else {
        wrapperData->ntServiceStartType = SERVICE_DEMAND_START;
    }


    /* Set the service priority class */
    work = _strupr((char *)getStringProperty(properties, "wrapper.ntservice.process_priority", "NORMAL"));
    if ( (strcmp(work, "LOW") == 0) || (strcmp(work, "IDLE") == 0) ) {
        wrapperData->ntServicePriorityClass = IDLE_PRIORITY_CLASS;
    } else if (strcmp(work, "HIGH") == 0) {
        wrapperData->ntServicePriorityClass = HIGH_PRIORITY_CLASS;
    } else if (strcmp(work, "REALTIME") == 0) {
        wrapperData->ntServicePriorityClass = REALTIME_PRIORITY_CLASS;
    } else {
        wrapperData->ntServicePriorityClass = NORMAL_PRIORITY_CLASS;
    }

    /* Account name */
    wrapperData->ntServiceAccount = (char *)getStringProperty(properties, "wrapper.ntservice.account", NULL);
    if ( wrapperData->ntServiceAccount && ( strlen( wrapperData->ntServiceAccount ) <= 0 ) )
    {
        wrapperData->ntServiceAccount = NULL;
    }

    /* Acount password */
    wrapperData->ntServicePassword = (char *)getStringProperty(properties, "wrapper.ntservice.password", NULL);
    if ( wrapperData->ntServicePassword && ( strlen( wrapperData->ntServicePassword ) <= 0 ) )
    {
        wrapperData->ntServicePassword = NULL;
    }
    if ( !wrapperData->ntServiceAccount )
    {
        /* If there is not account name, then the password must not be set. */
        wrapperData->ntServicePassword = NULL;
    }

    /* Interactive */
    wrapperData->ntServiceInteractive = getBooleanProperty( properties, "wrapper.ntservice.interactive", FALSE );
}
#endif

#ifndef WIN32 /* UNIX */
void wrapperBuildUnixDaemonInfo() {
    /** Get the pid file if any.  May be NULL */
    wrapperData->pidFilename = (char *)getStringProperty(properties, "wrapper.pidfile", NULL);
    
    /** Get the daemonize flag. */
    wrapperData->daemonize = getBooleanProperty(properties, "wrapper.daemonize", FALSE);
}
#endif

int wrapperLoadConfiguration() {
    /* Load log file */
    setLogfilePath((char *)getStringProperty(properties, "wrapper.logfile", "wrapper.log"));
    
    /* Load log file format */
    setLogfileFormat((char *)getStringProperty(properties, "wrapper.logfile.format", "LPTM"));

    /* Load log file log level */
    setLogfileLevel((char *)getStringProperty(properties, "wrapper.logfile.loglevel", "INFO"));

    /* Load max log filesize log level */
    setLogfileMaxFileSize((char *)getStringProperty(properties, "wrapper.logfile.maxsize", "0"));

    /* Load log files level */
    setLogfileMaxLogFiles((char *)getStringProperty(properties, "wrapper.logfile.maxfiles", "0"));

    /* Load console format */
    setConsoleLogFormat((char *)getStringProperty(properties, "wrapper.console.format", "PM"));

    /* Load console log level */
    setConsoleLogLevel((char *)getStringProperty(properties, "wrapper.console.loglevel", "INFO"));

    /* Load syslog log level */
    setSyslogLevel((char *)getStringProperty(properties, "wrapper.syslog.loglevel", "NONE"));

    /* Load syslog event source name */
    setSyslogEventSourceName((char *)getStringProperty(properties, "wrapper.ntservice.name", "Wrapper"));

    /* Register the syslog message file if syslog is enabled */
    if (getSyslogLevelInt() < LEVEL_NONE) {
        registerSyslogMessageFile( );
    }

    /* Initialize some values not loaded */
    wrapperData->exitCode = 0;

    /* Get the port */
    wrapperData->port = getIntProperty(properties, "wrapper.port", 15003);

    /* Get the debug status (Property is deprecated but flag is still used) */
    wrapperData->isDebugging = getBooleanProperty(properties, "wrapper.debug", FALSE);
    if (wrapperData->isDebugging) {
        /* For backwards compatability */
        setConsoleLogLevelInt(LEVEL_DEBUG);
        setLogfileLevelInt(LEVEL_DEBUG);
    } else {
        if (getLowLogLevel() <= LEVEL_DEBUG) {
            wrapperData->isDebugging = TRUE;
        }
    }
    
    /* Get the state output status. */
    wrapperData->isStateOutputEnabled = getBooleanProperty(properties, "wrapper.state_output", FALSE);

    /* Get the shutdown hook status */
    wrapperData->isShutdownHookDisabled = getBooleanProperty(properties, "wrapper.disable_shutdown_hook", FALSE);
    
    /* Get the restart delay. */
    wrapperData->restartDelay = getIntProperty(properties, "wrapper.restart.delay", 5);
    if (wrapperData->restartDelay < 0) {
        wrapperData->restartDelay = 0;
    }

    /* Get the timeout settings */
    wrapperData->cpuTimeout = getIntProperty(properties, "wrapper.cpu.timeout", 10);
    wrapperData->startupTimeout = getIntProperty(properties, "wrapper.startup.timeout", 30);
    wrapperData->pingTimeout = getIntProperty(properties, "wrapper.ping.timeout", 30);
    wrapperData->shutdownTimeout = getIntProperty(properties, "wrapper.shutdown.timeout", 30);
    wrapperData->jvmExitTimeout = getIntProperty(properties, "wrapper.jvm_exit.timeout", 5);
    if (wrapperData->startupTimeout <= 0) {
        wrapperData->startupTimeout = 31557600;  /* One Year.  Effectively never */
    }
    if (wrapperData->pingTimeout <= 0) {
        wrapperData->pingTimeout = 31557600;  /* One Year.  Effectively never */
    }
    if (wrapperData->shutdownTimeout <= 0) {
        wrapperData->shutdownTimeout = 31557600;  /* One Year.  Effectively never */
    }
    if (wrapperData->jvmExitTimeout <= 0) {
        wrapperData->jvmExitTimeout = 31557600;  /* One Year.  Effectively never */
    }
    if (wrapperData->cpuTimeout <= 0) {
        wrapperData->cpuTimeout = 31557600;  /* One Year.  Effectively never */
    } else {
        /* Make sure that the timeouts are all longer than the cpu timeout. */
        if ( wrapperData->startupTimeout < wrapperData->cpuTimeout ) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "CPU timeout detection may not operate correctly during startup because wrapper.cpu.timeout is not smaller than wrapper.startup.timeout.");
        }
        if ( wrapperData->pingTimeout < wrapperData->cpuTimeout ) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "CPU timeout detection may not operate correctly because wrapper.cpu.timeout is not smaller than wrapper.ping.timeout.");
        }
        if ( wrapperData->shutdownTimeout < wrapperData->cpuTimeout ) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "CPU timeout detection may not operate correctly during shutdown because wrapper.cpu.timeout is not smaller than wrapper.shutdown.timeout.");
        }
        /* jvmExit timeout can be shorter than the cpu timeout. */
    }

    /* Load properties controlling the number times the JVM can be restarted. */
    wrapperData->maxFailedInvocations = getIntProperty(properties, "wrapper.max_failed_invocations", 5);
    wrapperData->successfulInvocationTime = getIntProperty(properties, "wrapper.successful_invocation_time", 300);
    if ( wrapperData->maxFailedInvocations < 1 ) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            "The value of wrapper.max_failed_invocations must not be smaller than 1.  Changing to 1.");
        wrapperData->maxFailedInvocations = 1;
    }

    /* TRUE if the JVM should be asked to dump its state when it fails to halt on request. */
    wrapperData->requestThreadDumpOnFailedJVMExit = getBooleanProperty(properties, "wrapper.request_thread_dump_on_failed_jvm_exit", FALSE);

#ifdef WIN32
    /* Configure the NT service information */
    wrapperBuildNTServiceInfo();
#else /* UNIX */
    /* Configure the Unix daemon information */
    wrapperBuildUnixDaemonInfo();
#endif
    
    return 0;
}

/******************************************************************************
 * Protocol callback functions
 *****************************************************************************/
void wrapperLogSignalled(int logLevel, char *msg) {
    /* */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Got a log message from JVM: %s", msg);
    }
    /* */

    log_printf(wrapperData->jvmRestarts, logLevel, "%s", msg);
}

void wrapperKeyRegistered(char *key) {
    char buffer[7];

    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Got key from JVM: %s", key);
    }

    switch (wrapperData->jState) {
    case WRAPPER_JSTATE_LAUNCHING:
        /* We now know that the Java side wrapper code has started and
         *  registered with a key.  We still need to verify that it is
         *  the correct key however. */
        if (strcmp(key, wrapperData->key) == 0) {
            /* This is the correct key. */
            /* We now know that the Java side wrapper code has started. */
            wrapperData->jState = WRAPPER_JSTATE_LAUNCHED;
            wrapperData->jStateTimeout = 0;

            /* Send the low log level to the JVM so that it can control output via the log method. */
            sprintf(buffer, "%d", getLowLogLevel());
            wrapperProtocolFunction(WRAPPER_MSG_LOW_LOG_LEVEL, buffer);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Received a connection request with an incorrect key.  Waiting for another connection.");

            /* This was the wrong key.  Send a response. */
            wrapperProtocolFunction(WRAPPER_MSG_BADKEY, "Incorrect key.  Connection rejected.");

            /* Close the current connection.  Assume that the real JVM
             *  is still out there trying to connect.  So don't change
             *  the state.  If for some reason, this was the correct
             *  JVM, but the key was wrong.  then this state will time
             *  out and recover. */
            wrapperProtocolClose();
        }
    default:
        /* We got a key registration that we were not expecting.  Ignore it. */
        break;
    }
}

void wrapperPingResponded() {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Got ping response from JVM");
    }

    /* Depending on the current JVM state, do something. */
    switch (wrapperData->jState) {
    case WRAPPER_JSTATE_STARTED:
        /* We got a response to a ping.  Allow 5 + <pingTimeout> more seconds before the JVM
         *  is considered to be dead. */
        wrapperData->jStateTimeout = time(NULL) + 5 + wrapperData->pingTimeout;
        break;
    default:
        /* We got a ping response that we were not expecting.  Ignore it. */
        break;
    }
}

void wrapperStopRequested(int exitCode) {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM requested a shutdown. (%d)", exitCode);
    }

    /* Get things stopping on this end.  Ask the JVM to stop again in case the
     *	user code on the Java side is not written correctly. */
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
        /* Change the state to STOPPING */
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

        /* The Java side of the wrapper signalled that it stopped
         *	allow 5 + jvmExitTimeout seconds for the JVM to exit. */
        wrapperData->jStateTimeout = time(NULL) + 5 + wrapperData->jvmExitTimeout;
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

    /* Only process the start pending signal if the JVM state is starting or
     *  stopping.  Stopping is included because if the user hits CTRL-C while
     *  the application is starting, then the stop request will not be noticed
     *  until the application has completed its startup. */
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

        /* Give the JVM 30 seconds to respond to a ping. */
        wrapperData->jStateTimeout = time(NULL) + 30;

        /* Is the wrapper state STARTING? */
        if (wrapperData->wState == WRAPPER_WSTATE_STARTING) {
            wrapperData->wState = WRAPPER_WSTATE_STARTED;

            if (!wrapperData->isConsole) {
                /* Tell the service manager that we started */
                wrapperReportStatus(WRAPPER_WSTATE_STARTED, 0, 0);
            }
        }
    }
}
