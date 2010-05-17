/*
 * Copyright (c) 1999, 2010 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.org/doc/english/licenseOverview.html
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
 */

/**
 * Author:
 *   Leif Mortenson <leif@tanukisoftware.com>
 *   Ryan Shaw
 */

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include "wrapperinfo.h"
#include "wrapper.h"
#include "logger.h"
#include "wrapper_file.h"

#ifdef WIN32
#include <direct.h>
#include <winsock.h>
#include <shlwapi.h>
#include <windows.h>

/* MS Visual Studio 8 went and deprecated the POXIX names for functions.
 *  Fixing them all would be a big headache for UNIX versions. */
#pragma warning(disable : 4996)

/* Defines for MS Visual Studio 6 */
#ifndef _INTPTR_T_DEFINED
typedef long intptr_t;
#define _INTPTR_T_DEFINED
#endif

#define EADDRINUSE  WSAEADDRINUSE
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ENOTSOCK    WSAENOTSOCK
#define ECONNRESET  WSAECONNRESET

#else /* UNIX */
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define SOCKET         int
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

#if defined(SOLARIS)
#include <sys/errno.h>
#include <sys/fcntl.h>
#elif defined(AIX) || defined(HPUX) || defined(MACOSX) || defined(OSF1)
#elif defined(IRIX)
#define PATH_MAX FILENAME_MAX
#elif defined(FREEBSD)
#include <sys/param.h>
#include <errno.h>
#else /* LINUX */
#include <asm/errno.h>
#endif

#endif /* WIN32 */

WrapperConfig *wrapperData;
char         packetBuffer[MAX_LOG_SIZE + 1];
char         *keyChars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-";

/* Properties structure loaded in from the configuration file. */
Properties              *properties;

/* Server Socket. */
SOCKET ssd = INVALID_SOCKET;
/* Client Socket. */
SOCKET sd = INVALID_SOCKET;
int loadConfiguration();

/**
 * Constructs a tm structure from a pair of Strings like "20091116" and "1514".
 *  The time returned will be in the local time zone.  This is not 100% accurate
 *  as it doesn't take into account the time zone in which the dates were
 *  originally set.
 */
struct tm getInfoTime(const char *date, const char *time) {
    struct tm buildTM;
    char temp[5];

    memset(&buildTM, 0, sizeof(struct tm));

    /* Year */
    memcpy(temp, date, 4);
    temp[4] = 0;
    buildTM.tm_year = atoi(temp) - 1900;

    /* Month */
    memcpy(temp, date + 4, 2);
    temp[2] = 0;
    buildTM.tm_mon = atoi(temp) - 1;

    /* Day */
    memcpy(temp, date + 6, 2);
    temp[2] = 0;
    buildTM.tm_mday = atoi(temp);
    
    /* Hour */
    memcpy(temp, time, 2);
    temp[2] = 0;
    buildTM.tm_hour = atoi(temp);
    
    /* Minute */
    memcpy(temp, time + 2, 2);
    temp[2] = 0;
    buildTM.tm_min = atoi(temp);

    return buildTM;
}

struct tm wrapperGetReleaseTime() {
    return getInfoTime(wrapperReleaseDate, wrapperReleaseTime);
}

struct tm wrapperGetBuildTime() {
    return getInfoTime(wrapperBuildDate, wrapperBuildTime);
}

/**
 * Adds default properties used to set global environment variables.
 *
 * These are done by setting properties rather than call setEnv directly
 *  so that it will be impossible for users to override their values by
 *  creating a "set.XXX=NNN" property in the configuration file.
 */
void wrapperAddDefaultProperties() {
    size_t bufferLen;
    char* buffer;
    
    /* IMPORTANT - If any new values are added here, this work buffer length may need to be calculated differently. */
    bufferLen = 1;
    bufferLen = __max(bufferLen, strlen("set.WRAPPER_PID=") + 10 + 1); /* 32-bit PID would be max of 10 characters */
    bufferLen = __max(bufferLen, strlen("set.WRAPPER_BITS=") + strlen(wrapperBits) + 1);
    bufferLen = __max(bufferLen, strlen("set.WRAPPER_ARCH=") + strlen(wrapperArch) + 1);
    bufferLen = __max(bufferLen, strlen("set.WRAPPER_OS=") + strlen(wrapperOS) + 1);
    bufferLen = __max(bufferLen, strlen("set.WRAPPER_HOSTNAME=") + strlen(wrapperData->hostName) + 1);
    bufferLen = __max(bufferLen, strlen("set.WRAPPER_HOST_NAME=") + strlen(wrapperData->hostName) + 1);

    buffer = malloc(bufferLen);
    if (!buffer) {
        outOfMemory("WADP", 1);
        return;
    }

    sprintf(buffer, "set.WRAPPER_PID=%d", wrapperData->wrapperPID);
    addPropertyPair(properties, buffer, TRUE, FALSE);

    sprintf(buffer, "set.WRAPPER_BITS=%s", wrapperBits);
    addPropertyPair(properties, buffer, TRUE, FALSE);

    sprintf(buffer, "set.WRAPPER_ARCH=%s", wrapperArch);
    addPropertyPair(properties, buffer, TRUE, FALSE);

    sprintf(buffer, "set.WRAPPER_OS=%s", wrapperOS);
    addPropertyPair(properties, buffer, TRUE, FALSE);

    sprintf(buffer, "set.WRAPPER_HOSTNAME=%s", wrapperData->hostName);
    addPropertyPair(properties, buffer, TRUE, FALSE);

    sprintf(buffer, "set.WRAPPER_HOST_NAME=%s", wrapperData->hostName);
    addPropertyPair(properties, buffer, TRUE, FALSE);

#ifdef WIN32
    addPropertyPair(properties, "set.WRAPPER_FILE_SEPARATOR=\\", TRUE, FALSE);
    addPropertyPair(properties, "set.WRAPPER_PATH_SEPARATOR=;", TRUE, FALSE);
#else
    addPropertyPair(properties, "set.WRAPPER_FILE_SEPARATOR=/", TRUE, FALSE);
    addPropertyPair(properties, "set.WRAPPER_PATH_SEPARATOR=:", TRUE, FALSE);
#endif

    free(buffer);
}

/**
 * This function is here to help Community Edition users who are attempting
 *  to generate a hostId.
 */
int showHostIds(int logLevel) {
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, "");
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, "The Community Edition of the Java Service Wrapper does not implement");
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, "Host Ids.");
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, "");
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, "If you have requested a trial license, or purchased a license, you");
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, "may be looking for the Standard or Professional Editions of the Java");
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, "Service Wrapper.  They can be downloaded here:");
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, "  http://wrapper.tanukisoftware.org/download");
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, "");

    return FALSE;
}

/**
 * Return TRUE if there were any problems.
 */
int wrapperLoadConfigurationProperties() {
    int i;
    int firstCall;
#ifdef WIN32
    int work;
#endif
    const char* prop;

    /* Unless this is the first call, we need to dispose the previous properties object. */
    if (properties) {
        firstCall = FALSE;
        disposeProperties(properties);
        properties = NULL;
    } else {
        firstCall = TRUE;

        /* This is the first time, so preserve the working directory. */
#ifdef WIN32
        work = GetFullPathName(".", 0, NULL, NULL);
        if (!work) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                "Unable to resolve the original working directory: %s", getLastErrorText());
            return TRUE;
        }
        wrapperData->originalWorkingDir = malloc(sizeof(char) * work);
        if (!wrapperData->originalWorkingDir) {
            outOfMemory("WLCP", 3);
            return TRUE;
        }
        if (!GetFullPathName(".", work, wrapperData->originalWorkingDir, NULL)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                "Unable to resolve the original working directory: %s", getLastErrorText());
            return TRUE;
        }
#else
        /* The solaris implementation of realpath will return a relative path if a relative
         *  path is provided.  We always need an abosulte path here.  So build up one and
         *  then use realpath to remove any .. or other relative references. */
        wrapperData->originalWorkingDir = malloc(PATH_MAX);
        if (!wrapperData->originalWorkingDir) {
            outOfMemory("WLCP", 4);
            return TRUE;
        }
        if (realpath(".", wrapperData->originalWorkingDir) == NULL) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                "Unable to resolve the original working directory: %s", getLastErrorText());
            return TRUE;
        }
#endif

        /* This is the first time, so preserve the full canonical location of the
         *  configuration file. */
#ifdef WIN32
        work = GetFullPathName(wrapperData->argConfFile, 0, NULL, NULL);
        if (!work) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                "Unable to open configuration file, %s: %s",
                wrapperData->argConfFile, getLastErrorText());
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                "Current working directory is: %s", wrapperData->originalWorkingDir);
            return TRUE;
        }
        wrapperData->configFile = malloc(sizeof(char) * work);
        if (!wrapperData->configFile) {
            outOfMemory("WLCP", 1);
            return TRUE;
        }
        if (!GetFullPathName(wrapperData->argConfFile, work, wrapperData->configFile, NULL)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                "Unable to open configuration file, %s: %s",
                wrapperData->argConfFile, getLastErrorText());
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                "Current working directory is: %s", wrapperData->originalWorkingDir);
            return TRUE;
        }
#else
        /* The solaris implementation of realpath will return a relative path if a relative
         *  path is provided.  We always need an abosulte path here.  So build up one and
         *  then use realpath to remove any .. or other relative references. */
        wrapperData->configFile = malloc(PATH_MAX);
        if (!wrapperData->configFile) {
            outOfMemory("WLCP", 2);
            return TRUE;
        }
        if (realpath(wrapperData->argConfFile, wrapperData->configFile) == NULL) {
            /* Most likely the file does not exist.  The wrapperData->configFile has the first
             *  file that could not be found.  May not be the config file directly if symbolic
             *  links are involved. */
            if (wrapperData->argConfFileDefault) {
                /* This was the default config file name.  We know that the working directory
                 *  could be resolved so the problem must be that the default config file does
                 *  not exist.  This problem will be reported later and the wrapperData->configFile
                 *  variable will have the correct full path.
                 * Fall through for now and the user will get a better error later. */
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    "Unable to open configuration file, %s: %s",
                    wrapperData->argConfFile, getLastErrorText());
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    "Current working directory is: %s", wrapperData->originalWorkingDir);
                return TRUE;
            }
        }
#endif
    }

    /* Create a Properties structure. */
    properties = createProperties();
    if (!properties) {
        return TRUE;
    }
    wrapperAddDefaultProperties();

    /* The argument prior to the argBase will be the configuration file, followed
     *  by 0 or more command line properties.  The command line properties need to be
     *  loaded first, followed by the configuration file. */
    for (i = 0; i < wrapperData->argCount; i++) {
        if (addPropertyPair(properties, wrapperData->argValues[i], TRUE, TRUE)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, 
                "The argument '%s' is not a valid property name-value pair.",
                wrapperData->argValues[i]);
            return TRUE;
        }
    }

    /* Now load the configuration file. */
    if (loadProperties(properties, wrapperData->configFile)) {
        /* File not found. */
        /* If this was a default file name then we don't want to show this as
         *  an error here.  It will be handled by the caller. */
        /* Debug is not yet available as the config file is not yet loaded. */
        if (!wrapperData->argConfFileDefault) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                "Unable to open configuration file. %s", wrapperData->configFile);
        }
        return TRUE;
    }

    /* Config file found. */
    wrapperData->argConfFileFound = TRUE;

#ifdef _DEBUG
    /* Display the active properties */
    printf("Debug Configuration Properties:\n");
    dumpProperties(properties);
#endif

    /* Load the configuration. */
    if (loadConfiguration()) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
            "Problem loading wrapper configuration file: %s", wrapperData->configFile);
        return TRUE;
    }

    if (firstCall) {
        /* If the working dir was configured, we need to extract it and preserve its value.
         *  This must be done after the configuration has been completely loaded. */
        prop = getStringProperty(properties, "wrapper.working.dir", NULL);
        if (prop && (strlen(prop) > 0)) {
#ifdef WIN32
            work = GetFullPathName(prop, 0, NULL, NULL);
            if (!work) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    "Unable to resolve the working directory %s: %s", prop, getLastErrorText());
                return TRUE;
            }
            wrapperData->workingDir = malloc(sizeof(char) * work);
            if (!wrapperData->workingDir) {
                outOfMemory("WLCP", 5);
                return TRUE;
            }
            if (!GetFullPathName(prop, work, wrapperData->workingDir, NULL)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    "Unable to resolve the working directory %s: %s", prop, getLastErrorText());
                return TRUE;
            }
#else
            /* The solaris implementation of realpath will return a relative path if a relative
             *  path is provided.  We always need an abosulte path here.  So build up one and
             *  then use realpath to remove any .. or other relative references. */
            wrapperData->workingDir = malloc(PATH_MAX);
            if (!wrapperData->workingDir) {
                outOfMemory("WLCP", 6);
                return TRUE;
            }
            if (realpath(prop, wrapperData->workingDir) == NULL) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    "Unable to resolve the working directory %s: %s", prop, getLastErrorText());
                return TRUE;
            }
#endif
        }
    }

    return FALSE;
}

void wrapperGetCurrentTime(struct timeb *timeBuffer) {
#ifdef WIN32
    ftime(timeBuffer);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    timeBuffer->time = (time_t)tv.tv_sec;
    timeBuffer->millitm = (unsigned short)(tv.tv_usec / 1000);
#endif
}

void protocolStopServer() {
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
}

void protocolStartServer() {
    struct sockaddr_in addr_srv;
    int rc;
    int port;
    int fixedPort;

    /*int optVal;*/
#ifdef WIN32
    u_long dwNoBlock = TRUE;
#endif

    /* Create the server socket. */
    ssd = socket(AF_INET, SOCK_STREAM, 0);
    if (ssd == INVALID_SOCKET) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR,
            "server socket creation failed. (%s)", getLastErrorText());
        return;
    }

    /* Make sure the socket is reused. */
    /* We actually do not want to do this as it makes it possible for more than one Wrapper
     *  instance to bind to the same port.  The second instance succeeds to bind, but any
     *  attempts to connect to that port will go to the dirst Wrapper.  This would of course
     *  cause attempts to launch the second JVM to fail.
    optVal = 1;
#ifdef WIN32
    if (setsockopt(ssd, SOL_SOCKET, SO_REUSEADDR, (char *)&optVal, sizeof(optVal)) < 0) {
#else
    if (setsockopt(ssd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)) < 0) {
#endif
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR,
            "server socket SO_REUSEADDR failed. (%s)", getLastErrorText());
        wrapperProtocolClose();
        protocolStopServer();
        return;
    }
    */

    /* Make the socket non-blocking */
#ifdef WIN32
    rc = ioctlsocket(ssd, FIONBIO, &dwNoBlock);
#else /* UNIX  */
    rc = fcntl(ssd, F_SETFL, O_NONBLOCK);
#endif

    if (rc == SOCKET_ERROR) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR,
            "server socket ioctlsocket failed. (%s)", getLastErrorText());
        wrapperProtocolClose();
        protocolStopServer();
        return;
    }

    /* If a port was specified in the configuration file then we want to
     *  try to use that port or find the next available port.  If 0 was
     *  specified, then we will silently start looking for an available
     *  port starting at 32000. */
    port = wrapperData->port;
    if (port <= 0) {
        port = wrapperData->portMin;
        fixedPort = FALSE;
    } else {
        fixedPort = TRUE;
    }

  tryagain:
    /* Try binding to the port. */
    /*log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_STATUS, "Trying port %d", port);*/
    
    /* Cleanup the addr_srv first */
    memset(&addr_srv, 0, sizeof(addr_srv));
    
    addr_srv.sin_family = AF_INET;
    addr_srv.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr_srv.sin_port = htons((u_short)port);
#ifdef WIN32
    rc = bind(ssd, (struct sockaddr FAR *)&addr_srv, sizeof(addr_srv));
#else /* UNIX */
    rc = bind(ssd, (struct sockaddr *)&addr_srv, sizeof(addr_srv));
#endif
    
    if (rc == SOCKET_ERROR) {
        rc = wrapperGetLastError();

        /* The specified port could bot be bound. */
        if (rc == EADDRINUSE) {
            /* Address in use, try looking at the next one. */
            if (fixedPort) {
                /* The last port checked was the defined fixed port, switch to the dynamic range. */
                port = wrapperData->portMin;
                fixedPort = FALSE;
                goto tryagain;
            } else {
                port++;
                if (port <= wrapperData->portMax) {
                    goto tryagain;
                }
            }
        }

        /* Log an error.  This is fatal, so die. */
        if (wrapperData->port <= 0) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_FATAL,
                "unable to bind listener to any port in the range %d-%d. (%s)",
                wrapperData->portMin, wrapperData->portMax, getLastErrorText());
        } else {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_FATAL,
                "unable to bind listener port %d, or any port in the range %d-%d. (%s)",
                wrapperData->port, wrapperData->portMin, wrapperData->portMax, getLastErrorText());
        }

        wrapperStopProcess(FALSE, getLastError());
        wrapperProtocolClose();
        protocolStopServer();
        wrapperData->exitRequested = TRUE;
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_NO;
        return;
    }

    /* If we got here, then we are bound to the port */
    if ((wrapperData->port > 0) && (port != wrapperData->port)) {
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
        wrapperProtocolClose();
        protocolStopServer();
        return;
    }
}

/**
 * Attempt to accept a connection from a JVM client.
 */
void protocolOpen() {
    struct sockaddr_in addr_srv;
    int rc;
#if defined(WIN32)
    u_long dwNoBlock = TRUE;
    u_long addr_srv_len;
#elif (defined(HPUX) && !defined(ARCH_IA)) || defined(OSF1) || defined(IRIX)
    int addr_srv_len;
#else
    socklen_t addr_srv_len;
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
    sd = accept(ssd, (struct sockaddr *)&addr_srv, &addr_srv_len);
#endif
    if (sd == INVALID_SOCKET) {
        rc = wrapperGetLastError();
        /* EWOULDBLOCK != EAGAIN on some platforms. */
        if ((rc == EWOULDBLOCK) || (rc == EAGAIN)) {
            /* There are no incomming sockets right now. */
            return;
        } else {
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                    "socket creation failed. (%s)", getLastErrorText());
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
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                "socket ioctlsocket failed. (%s)", getLastErrorText());
        }
        wrapperProtocolClose();
        return;
    }
    
    /* We got an incoming connection, so close down the listener for security reasons. */
    protocolStopServer();
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

/**
 * Returns the name of a given function code for debug purposes.
 */
char *wrapperProtocolGetCodeName(char code) {
    static char unknownBuffer[14];
    char *name;

    switch(code) {
    case WRAPPER_MSG_START:
        name = "START";
        break;

    case WRAPPER_MSG_STOP:
        name = "STOP";
        break;

    case WRAPPER_MSG_RESTART:
        name = "RESTART";
        break;

    case WRAPPER_MSG_PING:
        name = "PING";
        break;

    case WRAPPER_MSG_STOP_PENDING:
        name = "STOP_PENDING";
        break;

    case WRAPPER_MSG_START_PENDING:
        name = "START_PENDING";
        break;

    case WRAPPER_MSG_STARTED:
        name = "STARTED";
        break;

    case WRAPPER_MSG_STOPPED:
        name = "STOPPED";
        break;

    case WRAPPER_MSG_KEY:
        name = "KEY";
        break;

    case WRAPPER_MSG_BADKEY:
        name = "BADKEY";
        break;

    case WRAPPER_MSG_LOW_LOG_LEVEL:
        name = "LOW_LOG_LEVEL";
        break;

    case WRAPPER_MSG_PING_TIMEOUT:
        name = "PING_TIMEOUT";
        break;

    case WRAPPER_MSG_SERVICE_CONTROL_CODE:
        name = "SERVICE_CONTROL_CODE";
        break;

    case WRAPPER_MSG_PROPERTIES:
        name = "PROPERTIES";
        break;

    case WRAPPER_MSG_LOG + LEVEL_DEBUG:
        name = "LOG(DEBUG)";
        break;

    case WRAPPER_MSG_LOG + LEVEL_INFO:
        name = "LOG(INFO)";
        break;

    case WRAPPER_MSG_LOG + LEVEL_STATUS:
        name = "LOG(STATUS)";
        break;

    case WRAPPER_MSG_LOG + LEVEL_WARN:
        name = "LOG(WARN)";
        break;

    case WRAPPER_MSG_LOG + LEVEL_ERROR:
        name = "LOG(ERROR)";
        break;

    case WRAPPER_MSG_LOG + LEVEL_FATAL:
        name = "LOG(FATAL)";
        break;
        
    case WRAPPER_MSG_LOGFILE:
        name = "LOGFILE";
        break;
        
    case WRAPPER_MSG_APPEAR_ORPHAN:
        name = "APPEAR_ORPHAN";
        break;
        
    default:
        sprintf(unknownBuffer, "UNKNOWN(%d)", code);
        name = unknownBuffer;
        break;
    }
    return name;
}

/* Mutex for syncronization of the wrapperProtocolFunction function. */
#ifdef WIN32
HANDLE protocolMutexHandle = NULL;
#else
pthread_mutex_t protocolMutex = PTHREAD_MUTEX_INITIALIZER;
#endif


/** Obtains a lock on the protocol mutex. */
int lockProtocolMutex() {
#ifdef WIN32
    switch (WaitForSingleObject(protocolMutexHandle, INFINITE)) {
    case WAIT_ABANDONED:
        printf("Protocol mutex was abandoned.\n");
        fflush(NULL);
        return -1;
    case WAIT_FAILED:
        printf("Protocol mutex wait failed.\n");
        fflush(NULL);
        return -1;
    case WAIT_TIMEOUT:
        printf("Protocol mutex wait timed out.\n");
        fflush(NULL);
        return -1;
    default:
        /* Ok */
        break;
    }
#else
    if (pthread_mutex_lock(&protocolMutex)) {
        printf("Failed to lock the Protocol mutex. %s\n", getLastErrorText());
        return -1;
    }
#endif

    return 0;
}

/** Releases a lock on the protocol mutex. */
int releaseProtocolMutex() {
#ifdef WIN32
    if (!ReleaseMutex(protocolMutexHandle)) {
        printf("Failed to release Protocol mutex. %s\n", getLastErrorText());
        fflush(NULL);
        return -1;
    }
#else
    if (pthread_mutex_unlock(&protocolMutex)) {
        printf("Failed to unlock the Protocol mutex. %s\n", getLastErrorText());
        return -1;
    }
#endif
    return 0;
}

/**
 * Sends a command to the JVM over the backend socket.
 *
 * @param useLoggerQueue TRUE if called from a signal where the logger queue needs to be used.
 * @param function The function code to send.
 * @param message The message to send.
 *
 * @return TRUE if there was an error, FALSE otherwise.
 */
size_t protocolSendBufferSize = 0;
char *protocolSendBuffer = NULL;
int wrapperProtocolFunction(int useLoggerQueue, char function, const char *message) {
    int rc;
    int cnt;
    size_t len;
    const char *logMsg;
    int returnVal;
    int ok = TRUE;

    /* It is important than there is never more than one thread allowed in here at a time. */
    if (lockProtocolMutex()) {
        return TRUE;
    }

    /* We don't want to show the full properties log message.  It is quite long and distracting. */
    if (function == WRAPPER_MSG_PROPERTIES) {
        logMsg = "(Property Values)";
    } else {
        logMsg = message;
    }
    
    /* If we are in the orphaned JVM test mode then don't do anything. */
    if (wrapperData->isJVMOrphaned) {
        if (wrapperData->isDebugging) {
            log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                "Orphan Mode.  Skip sending packet %s : %s",
                wrapperProtocolGetCodeName(function), (message == NULL ? "NULL" : logMsg));
        }
        returnVal = FALSE;
        ok = FALSE;
    }

    if (ok) {
        /* Make sure the buffer is big enough for this message. */
        if (message == NULL) {
            len = 2;
        } else {
            len = 1 + strlen(message) + 1;
        }
        if (protocolSendBufferSize < len) {
            if (protocolSendBuffer) {
                free(protocolSendBuffer);
            }
            protocolSendBuffer = malloc(len);
            if (!protocolSendBuffer) {
                outOfMemory("WPF", 1);
                returnVal = TRUE;
                ok = FALSE;
            }
        }
    }

    if (ok) {
        if (sd == INVALID_SOCKET) {
            /* A socket was not opened */
            if (wrapperData->isDebugging) {
                log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                    "socket not open, so packet not sent %s : %s",
                    wrapperProtocolGetCodeName(function), (message == NULL ? "NULL" : logMsg));
            }
            returnVal = TRUE;
        } else {
            if (wrapperData->isDebugging) {
                if ((function == WRAPPER_MSG_PING) && (strcmp(message, "silent") == 0)) {
                    /*
                    log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                        "send a silent ping packet");
                    */
                } else {
                    log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                        "send a packet %s : %s",
                        wrapperProtocolGetCodeName(function), (message == NULL ? "NULL" : logMsg));
                }
            }
    
            /* Build the packet */
            protocolSendBuffer[0] = function;
            if (message == NULL) {
                protocolSendBuffer[1] = '\0';
            } else {
                strcpy((char*)(protocolSendBuffer + 1), message);
            }
    
            /* Send the packet */
            cnt = 0;
            do {
                if (cnt > 0) {
                    wrapperSleep(useLoggerQueue, 10);
                }
                rc = send(sd, protocolSendBuffer, (int)len, 0);
                cnt++;
            } while ((rc == SOCKET_ERROR) && (wrapperGetLastError() == EWOULDBLOCK) && (cnt < 200));
            if (rc == SOCKET_ERROR) {
                if (wrapperGetLastError() == EWOULDBLOCK) {
                    log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_PROTOCOL, LEVEL_WARN,
                        "socket send failed.  Blocked for 2 seconds.  %s",
                        getLastErrorText());
                } else {
                    if (wrapperData->isDebugging) {
                        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                            "socket send failed.  %s", getLastErrorText());
                    }
                }
                wrapperProtocolClose();
                returnVal = TRUE;
            } else {
                returnVal = FALSE;
            }
        }
    }

    /* Always make sure the mutex is released. */
    if (releaseProtocolMutex()) {
        returnVal = TRUE;
    }

    return returnVal;
}

/**
 * Checks the status of the server socket.
 *
 * The socket will be initialized if the JVM is in a state where it should
 *  be up, otherwise the socket will be left alone.
 *
 * If the forceOpen flag is set then an attempt will be made to initialize
 *  the socket regardless of the JVM state.
 *
 * Returns TRUE if the socket is open and ready on return, FALSE if not.
 */
int wrapperCheckServerSocket(int forceOpen) {
    if (ssd == INVALID_SOCKET) {
        /* The socket is not currently open and needs to be started,
         *  unless the JVM is DOWN or in a state where it is not needed. */
        if ((!forceOpen) &&
            ((wrapperData->jState == WRAPPER_JSTATE_DOWN_CLEAN) || 
             (wrapperData->jState == WRAPPER_JSTATE_LAUNCH_DELAY) ||
             (wrapperData->jState == WRAPPER_JSTATE_RESTART) ||
             (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
             (wrapperData->jState == WRAPPER_JSTATE_KILLING) ||
             (wrapperData->jState == WRAPPER_JSTATE_KILL) ||
             (wrapperData->jState == WRAPPER_JSTATE_DOWN_CHECK))) {
            /* The JVM is down or in a state where the socket is not needed. */
            return FALSE;
        } else {
            /* The socket should be open, try doing so. */
            protocolStartServer();
            if (ssd == INVALID_SOCKET) {
                /* Failed. */
                return FALSE;
            } else {
                return TRUE;
            }
        }
    } else {
        /* Socket is ready. */
        return TRUE;
    }
}

/**
 * Read any data sent from the JVM.  This function will loop and read as many
 *  packets are available.  The loop will only be allowed to go for 250ms to
 *  ensure that other functions are handled correctly.
 *
 * Returns 0 if all available data has been read, 1 if more data is waiting.
 */
int wrapperProtocolRead() {
    char c, code;
    int len;
    int pos;
    int err;
    struct timeb timeBuffer;
    time_t startTime;
    int startTimeMillis;
    time_t now;
    int nowMillis;
    time_t durr;
    
    wrapperGetCurrentTime(&timeBuffer);
    startTime = now = timeBuffer.time;
    startTimeMillis = nowMillis = timeBuffer.millitm;

    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "now=%ld, nowMillis=%d", now, nowMillis);
    */

    while ((durr = (now - startTime) * 1000 + (nowMillis - startTimeMillis)) < 250) {
        /*
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "durr=%ld", durr);
        */

        /* If we have an open client socket, then use it. */
        if (sd == INVALID_SOCKET) {
            /* A Client socket is not open */

            /* Is the server socket open? */
            if (!wrapperCheckServerSocket(FALSE)) {
                /* Socket is down.  We can not read any packets. */
                return 0;
            }

            /* Try accepting a socket */
            protocolOpen();
            if (sd == INVALID_SOCKET) {
                return 0;
            }
        }

        /* Try receiving a packet code */
        len = recv(sd, &c, 1, 0);
        if (len == SOCKET_ERROR) {
            err = wrapperGetLastError();
            if ((err != EWOULDBLOCK) && (err != EAGAIN)
                && (err != ENOTSOCK) && (err != ECONNRESET)) {
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                        "socket read failed. (%s)", getLastErrorText());
                }
                wrapperProtocolClose();
            }
            /*
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "no data");
            */
            return 0;
        } else if (len != 1) {
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "socket read no code (closed?).");
            }
            wrapperProtocolClose();
            return 0;
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
            if ((code == WRAPPER_MSG_PING) && (strcmp(packetBuffer, "silent") == 0)) {
                /*
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "read a silent ping packet");
                */
            } else {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "read a packet %s : %s",
                    wrapperProtocolGetCodeName(code), packetBuffer);
            }
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
            wrapperStopPendingSignaled(atoi(packetBuffer));
            break;

        case WRAPPER_MSG_STOPPED:
            wrapperStoppedSignaled();
            break;

        case WRAPPER_MSG_START_PENDING:
            wrapperStartPendingSignaled(atoi(packetBuffer));
            break;

        case WRAPPER_MSG_STARTED:
            wrapperStartedSignaled();
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
            wrapperLogSignaled(code - WRAPPER_MSG_LOG, packetBuffer);
            break;
            
        case WRAPPER_MSG_APPEAR_ORPHAN:
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_STATUS, "Orphan the JVM and wait for it to exit on its own...",
                wrapperProtocolGetCodeName(code), packetBuffer);
            wrapperData->isJVMOrphaned = TRUE;
            break;

        default:
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, "received unknown packet (%d:%s)", code, packetBuffer);
            }
            break;
        }

        /* Get the time again */
        wrapperGetCurrentTime(&timeBuffer);
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


/******************************************************************************
 * Wrapper inner methods.
 *****************************************************************************/
/**
 * IMPORTANT - Any logging done in here needs to be queued or it would cause a recursion problem.
 */
void wrapperLogFileChanged(const char *logFile) {
    if (wrapperData->isDebugging) {
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "active log file changed: %s", logFile);
    }
    
    /* On startup, this function will always be called the first time the log file is set,
     *  we don't want to send the command in this case as it clutters the debug log output.
     *  Besides, the JVM will not be running anyway. */
    if (wrapperData->jState != WRAPPER_JSTATE_DOWN_CLEAN) {
        wrapperProtocolFunction(FALSE, WRAPPER_MSG_LOGFILE, logFile);
    }
}
/**
 * Pre initialize the wrapper.
 */
int wrapperInitialize() {
    /* Initialize the properties variable. */
    properties = NULL;

    /* Initialize the random seed. */
    srand((unsigned)time(NULL));

    /* Make sure all values are reliably set to 0. All required values should also be
     *  set below, but this extra step will protect against future changes.  Some
     *  platforms appear to initialize maloc'd memory to 0 while others do not. */
    wrapperData = malloc(sizeof(WrapperConfig));
    if (!wrapperData) {
        outOfMemory("WI", 1);
        return 1;
    }
    memset(wrapperData, 0, sizeof(WrapperConfig));
    /* Setup the initial values of required properties. */
    wrapperData->configured = FALSE;
    wrapperData->isConsole = TRUE;
    wrapperSetWrapperState(FALSE, WRAPPER_WSTATE_STARTING);
    wrapperSetJavaState(FALSE, WRAPPER_JSTATE_DOWN_CLEAN, 0, -1);
    wrapperData->lastPingTicks = wrapperGetTicks();
    wrapperData->lastLoggedPingTicks = wrapperGetTicks();
    wrapperData->jvmCommand = NULL;
    wrapperData->exitRequested = FALSE;
    wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_INITIAL; /* The first JVM needs to be started. */
    wrapperData->exitCode = 0;
    wrapperData->jvmRestarts = 0;
    wrapperData->jvmLaunchTicks = wrapperGetTicks();
    wrapperData->failedInvocationCount = 0;
        
    if (initLogging(wrapperLogFileChanged)) {
        return 1;
    }

    setLogfilePath("wrapper.log");
    setLogfileRollMode(ROLL_MODE_SIZE);
    setLogfileFormat("LPTM");
    setLogfileLevelInt(LEVEL_DEBUG);
    setLogfileAutoClose(FALSE);
    setConsoleLogFormat("LPM");
    setConsoleLogLevelInt(LEVEL_DEBUG);
    setConsoleFlush(FALSE);
    setSyslogLevelInt(LEVEL_NONE);

#ifdef WIN32
    if (!(protocolMutexHandle = CreateMutex(NULL, FALSE, NULL))) {
        printf("Failed to create protocol mutex. %s\n", getLastErrorText());
        fflush(NULL);
        return 1;
    }
#endif
    
    /* This is a sanity check to make sure that the datatype used for tick counts is correct. */
    if (sizeof(TICKS) != 4) {
        printf("Tick size incorrect %d != 4\n", (int)sizeof(TICKS));
        fflush(NULL);
        return 1;
    }

    return 0;
}

/** Common wrapper cleanup code. */
void wrapperDispose() {
#ifdef WIN32
    if (protocolMutexHandle) {
        if (!CloseHandle(protocolMutexHandle)) {
            printf("Unable to close protocol mutex handle. %s\n", getLastErrorText());
            fflush(NULL);
        }
    }
#endif

    /* Clean up the logging system. */
    disposeLogging();
}

/**
 * Returns the file name base as a newly malloced char *.  The resulting
 *  base file name will have any path and extension stripped.
 *
 * baseName should be long enough to always contain the base name.
 *  (strlen(fileName) + 1) is safe.
 */
void wrapperGetFileBase(const char *fileName, char *baseName) {
    const char *start;
    const char *end;
    const char *c;

    start = fileName;
    end = &fileName[strlen(fileName)];

    /* Strip off any path. */
#ifdef WIN32
    c = strrchr(start, '\\');
#else
    c = strrchr(start, '/');
#endif
    if (c) {
        start = &c[1];
    }

    /* Strip off any extension. */
    c = strrchr(start, '.');
    if (c) {
        end = c;
    }

    /* Now create the new base name. */
    memcpy(baseName, start, end - start);
    baseName[end - start] = '\0';
}

/**
 * Output the version.
 */
void wrapperVersionBanner() {
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
        "Java Service Wrapper Community Edition %s-bit %s", wrapperBits, wrapperVersionRoot);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
        "  Copyright (C) 1999-2010 Tanuki Software, Ltd.  All Rights Reserved.");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
        "    http://wrapper.tanukisoftware.org");
}

/**
 * Output the application usage.
 */
void wrapperUsage(char *appName) {
    char *confFileBase;

    confFileBase = malloc(strlen(appName) + 1);
    if (!confFileBase) {
        outOfMemory("WU", 1);
        return;
    }
    wrapperGetFileBase(appName, confFileBase);
    
    setSimpleLogLevels();

    wrapperVersionBanner();
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Usage:");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  %s <command> <configuration file> [configuration properties] [...]", appName);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  %s <configuration file> [configuration properties] [...]", appName);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "     (<command> implicitly '-c')");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  %s <command>", appName);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "     (<configuration file> implicitly '%s.conf')", confFileBase);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  %s", appName);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "     (<command> implicitly '-c' and <configuration file> '%s.conf')", confFileBase);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "where <command> can be one of:");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -c  --console run as a Console application");
#ifdef WIN32
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -t  --start   starT an NT service");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -a  --pause   pAuse a started NT service");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -e  --resume  rEsume a paused NT service");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -p  --stop    stoP a running NT service");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -i  --install Install as an NT service");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -it --installstart Install and sTart as an NT service");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -r  --remove  Uninstall/Remove as an NT service");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -l=<code> --controlcode=<code> send a user controL Code to a running NT service");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -d  --dump    request a thread Dump");
    /** Return mask: installed:1 running:2 interactive:4 automatic:8 manual:16 disabled:32 */
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -q  --query   Query the current status of the service");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -qs --querysilent Silently Query the current status of the service");

    /* Omit '-s' option from help as it is only used by the service manager. */
    /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -s  --service used by service manager"); */
#endif
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -v  --version print the wrapper's version information.");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  -?  --help    print this help message");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "<configuration file> is the wrapper.conf to use.  Name must be absolute or relative");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  to the location of %s", appName);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "[configuration properties] are configuration name-value pairs which override values");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  in wrapper.conf.  For example:");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  wrapper.debug=true");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  Please note that any file references must be absolute or relative to the location");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  of the Wrapper executable.");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "");

    free(confFileBase);
}

/**
 * Parse the main arguments.
 *
 * Returns FALSE if the application should exit with an error.  A message will
 *  already have been logged.
 */
int wrapperParseArguments(int argc, char **argv) {
    char *argConfFileBase;
    char *c;

    if (argc > 1) {
        if (argv[1][0] == '-') {
            /* Syntax 1 or 3 */
            /* A command appears to have been specified. */
            wrapperData->argCommand = &argv[1][1]; /* Strip off the '-' */
            if (wrapperData->argCommand[0] == '\0') {
                wrapperUsage(argv[0]);
                return FALSE;
            }

            /* Does the argument have a value? */
            c = strchr(wrapperData->argCommand, '=');
            if (c == NULL) {
                wrapperData->argCommandArg = NULL;
            } else {
                wrapperData->argCommandArg = (char *)(c + 1);
                c[0] = '\0';
            }

            if (argc > 2) {
                /* Syntax 1 */
                /* A command and conf file were specified. */
                wrapperData->argConfFile = argv[2];
                wrapperData->argCount = argc - 3;
                wrapperData->argValues = &argv[3];
            } else {
                /* Syntax 3 */
                /* Only a command was specified.  Assume a default config file name. */
                argConfFileBase = malloc(strlen(argv[0]) + 1);
                if (!argConfFileBase) {
                    outOfMemory("WPA", 1);
                    return FALSE;
                }
                wrapperGetFileBase(argv[0], argConfFileBase);

                /* The following malloc is only called once, but is never freed. */
                wrapperData->argConfFile = malloc((strlen(argConfFileBase) + 5 + 1) * sizeof(char));
                if (!wrapperData->argConfFile) {
                    outOfMemory("WPA", 2);
                    free(argConfFileBase);
                    return FALSE;
                }
                sprintf(wrapperData->argConfFile, "%s.conf", argConfFileBase);
                wrapperData->argConfFileDefault = TRUE;
                wrapperData->argCount = argc - 2;
                wrapperData->argValues = &argv[2];
                free(argConfFileBase);
            }
        } else {
            /* Syntax 2 */
            /* A command was not specified, but there may be a config file. */
            wrapperData->argCommand = "c";
            wrapperData->argCommandArg = NULL;
            wrapperData->argConfFile = argv[1];
            wrapperData->argCount = argc - 2;
            wrapperData->argValues = &argv[2];
        }
    } else {
        /* Systax 4 */
        /* A config file was not specified.  Assume a default config file name. */
        wrapperData->argCommand = "c";
        wrapperData->argCommandArg = NULL;

        argConfFileBase = malloc(strlen(argv[0]) + 1);
        if (!argConfFileBase) {
            outOfMemory("WPA", 3);
            return FALSE;
        }
        wrapperGetFileBase(argv[0], argConfFileBase);

        /* The following malloc is only called once, but is never freed. */
        wrapperData->argConfFile = malloc((strlen(argConfFileBase) + 5 + 1) * sizeof(char));
        if (!wrapperData->argConfFile) {
            outOfMemory("WPA", 4);
            free(argConfFileBase);
            return FALSE;
        }
        sprintf(wrapperData->argConfFile, "%s.conf", argConfFileBase);
        wrapperData->argConfFileDefault = TRUE;
        wrapperData->argCount = argc - 1;
        wrapperData->argValues = &argv[1];
        free(argConfFileBase);
    }

    return TRUE;
}

/**
 * Logs a single line of child output allowing any filtering
 *  to be done in a common location.
 */
void logChildOutput(const char* log) {
    int i;

    log_printf(wrapperData->jvmRestarts, LEVEL_INFO, "%s", log);

    /* Look for output filters in the output.  Only match the first. */
    for (i = 0; i < wrapperData->outputFilterCount; i++) {
        if ((strlen(wrapperData->outputFilters[i]) > 0) && (strstr(log, wrapperData->outputFilters[i]))) {
            /* Found. */
            switch(wrapperData->outputFilterActions[i]) {
            case FILTER_ACTION_RESTART:
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Filter trigger matched.  Restarting JVM.");
                wrapperRestartProcess(FALSE);
                break;

            case FILTER_ACTION_SHUTDOWN:
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Filter trigger matched.  Shutting down.");
                wrapperStopProcess(FALSE, 1); /* Exit with an error code. */
                break;

            case FILTER_ACTION_DUMP:
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Filter trigger matched.  Requesting Thread dump.");
                wrapperRequestDumpJVMState(FALSE);
                break;

#if defined(MACOSX)
            case FILTER_ACTION_ADVICE_NIL_SERVER:
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, "");
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    "------------------------------------------------------------------------");
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    "Advice:");
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    "MACOSX is known to have problems displaying GUIs from processes running");
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    "as a daemon launched from launchd.  The above \"Returning nil _server\"");
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    "means that you are encountering this problem.  This usually results in");
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    "a long timeout which is affecting the performance of your application.");
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    "------------------------------------------------------------------------");
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, "");
                break;
#endif

            default: /* FILTER_ACTION_NONE*/
                /* Do nothing but masks later filters */
                break;
            }

            /* break out of the loop */
            break;
        }
    }
}

#define READ_BUFFER_BLOCK_SIZE 1024
char *wrapperChildWorkBuffer = NULL;
size_t wrapperChildWorkBufferSize = 0;
size_t wrapperChildWorkBufferLen = 0;
#define CHAR_LF 0x0a

/**
 * Read and process any output from the child JVM Process.
 * Most output should be logged to the wrapper log file.
 *
 * This function will only be allowed to run for 250ms before returning.  This is to
 *  make sure that the main loop gets CPU.  If there is more data in the pipe, then
 *  the function returns TRUE, otherwise FALSE.  This is a hint to the mail loop not to
 *  sleep.
 */
int wrapperReadChildOutput() {
    struct timeb timeBuffer;
    time_t startTime;
    int startTimeMillis;
    time_t now;
    int nowMillis;
    time_t durr;
    char *tempBuffer;
    char *cLF;
    int currentBlockRead;
    int defer = FALSE;
    int readThisPass = FALSE;

    if (!wrapperChildWorkBuffer) {
        /* Initialize the wrapperChildWorkBuffer.  Set its initial size to the block size + 1.
         *  This is so that we can always add a \0 to the end of it. */
        wrapperChildWorkBuffer = malloc(sizeof(char) * ((READ_BUFFER_BLOCK_SIZE * 2) + 1));
        if (!wrapperChildWorkBuffer) {
            outOfMemory("WRCO", 1);
            return FALSE;
        }
        wrapperChildWorkBufferSize = READ_BUFFER_BLOCK_SIZE * 2;
    }

    wrapperGetCurrentTime(&timeBuffer);
    startTime = now = timeBuffer.time;
    startTimeMillis = nowMillis = timeBuffer.millitm;

#ifdef DEBUG_CHILD_OUTPUT
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "wrapperReadChildOutput() BEGIN");
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "now=%ld, nowMillis=%d", now, nowMillis);
#endif
    
    /* Loop and read in CHILD_BLOCK_SIZE characters at a time.
     *
     * To keep a JVM outputting lots of content from freezing the Wrapper, we force a return every 250ms. */
    while ((durr = (now - startTime) * 1000 + (nowMillis - startTimeMillis)) < 250) {
#ifdef DEBUG_CHILD_OUTPUT
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "durr=%ld", durr);
#endif
        
        /* If there is not enough space in the work buffer to read in a full block then it needs to be extended. */
        if (wrapperChildWorkBufferLen + READ_BUFFER_BLOCK_SIZE > wrapperChildWorkBufferSize) {
#ifdef DEBUG_CHILD_OUTPUT
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Expand buffer.");
#endif
            tempBuffer = malloc(wrapperChildWorkBufferSize + sizeof(char) * (READ_BUFFER_BLOCK_SIZE + 1));
            if (!tempBuffer) {
                outOfMemory("WRCO", 2);
                return FALSE;
            }
            memcpy(tempBuffer, wrapperChildWorkBuffer, wrapperChildWorkBufferLen);
            tempBuffer[wrapperChildWorkBufferLen] = '\0';
            free(wrapperChildWorkBuffer);
            wrapperChildWorkBuffer = tempBuffer;
            wrapperChildWorkBufferSize += READ_BUFFER_BLOCK_SIZE;
#ifdef DEBUG_CHILD_OUTPUT
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "buffer now %d bytes", wrapperChildWorkBufferSize);
#endif
        }
        
#ifdef DEBUG_CHILD_OUTPUT
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Read from pipe.  buffLen=%d, buffSize=%d", wrapperChildWorkBufferLen, wrapperChildWorkBufferSize);
#endif
        if (wrapperReadChildOutputBlock(wrapperChildWorkBuffer + wrapperChildWorkBufferLen, READ_BUFFER_BLOCK_SIZE, &currentBlockRead)) {
            /* Error already reported. */
            return FALSE;
        }

        if (currentBlockRead > 0) {
            /* We read in a block, so increase the length. */
            wrapperChildWorkBufferLen += currentBlockRead;
            readThisPass = TRUE;
        }

        /* Terminate the string just to avoid errors.  The buffer has an extra character to handle this. */
        wrapperChildWorkBuffer[wrapperChildWorkBufferLen] = '\0';
        defer = FALSE;
        while ((wrapperChildWorkBufferLen > 0) && (!defer)) {
#ifdef DEBUG_CHILD_OUTPUT
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Inner loop.  buffLen=%d, buffSize=%d", wrapperChildWorkBufferLen, wrapperChildWorkBufferSize);
#endif
            /* We have something in the buffer.  Loop and see if we have a complete line to log.
             * We will always find a LF at the end of the line.  On Windows there may be a CR immediately before it. */
            cLF = strchr(wrapperChildWorkBuffer, (char)CHAR_LF);

            if (cLF != NULL) {
#ifdef WIN32
                if ((cLF > wrapperChildWorkBuffer) && ((cLF - sizeof(char))[0] == 0x0d)) {
#ifdef DEBUG_CHILD_OUTPUT
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Found CR+LF");
#endif
                    /* Replace the CR with a NULL */
                    (cLF - sizeof(char))[0] = 0;
                } else {
#endif
#ifdef DEBUG_CHILD_OUTPUT
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Found LF");
#endif
#ifdef WIN32
                }
#endif
                /* Replace the LF with a NULL */
                cLF[0] = '\0';
                
                /* We have a string to log. */
#ifdef DEBUG_CHILD_OUTPUT
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Log: [%s]", wrapperChildWorkBuffer);
#endif
                logChildOutput(wrapperChildWorkBuffer);

                /* Remove the line we just logged from the buffer by moving the rest up. */
                strcpy(wrapperChildWorkBuffer, cLF + sizeof(char));
                wrapperChildWorkBufferLen -= (cLF - wrapperChildWorkBuffer) + sizeof(char);
            } else {
                /* If we read this pass or if the last character is a CR on Windows then we always want to defer. */
                if (readThisPass
#ifdef WIN32
                        || (wrapperChildWorkBuffer[wrapperChildWorkBufferLen - 1] == 0x0d)
#endif
                    ) {
#ifdef DEBUG_CHILD_OUTPUT
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Incomplete line.  Defer: [%s]", wrapperChildWorkBuffer);
#endif
                    defer = TRUE;
                } else {
                    /* We have an incomplete line, but it was from a previous pass, so we want to log it as it may be a prompt.
                     *  This will always be the complete buffer. */
#ifdef DEBUG_CHILD_OUTPUT
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Incomplete line, but log now: [%s]", wrapperChildWorkBuffer);
#endif
                    logChildOutput(wrapperChildWorkBuffer);
                    wrapperChildWorkBuffer[0] = '\0';
                    wrapperChildWorkBufferLen = 0;
                }
            }
        }

        if (currentBlockRead <= 0) {
            /* All done for now. */
            if (wrapperChildWorkBufferLen > 0) {
#ifdef DEBUG_CHILD_OUTPUT
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "wrapperReadChildOutput() END (Incomplete)");
#endif
            } else {
#ifdef DEBUG_CHILD_OUTPUT
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "wrapperReadChildOutput() END");
#endif
            }
            return FALSE;
        }
    }

    /* If we got here then we timed out. */
#ifdef DEBUG_CHILD_OUTPUT
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "wrapperReadChildOutput() END TIMEOUT");
#endif
    return TRUE;
}

/**
 * Immediately after a JVM is launched and whenever the log file name changes,
 *  the log file name is sent to the JVM where it can be referenced by applications.
 */
void sendLogFileName() {
    wrapperProtocolFunction(FALSE, WRAPPER_MSG_LOGFILE, getLogfilePath());
}

/**
 * Immediately after a JVM is launched, the wrapper configuration is sent to the
 *  JVM where it can be used as a properties object.
 */
void sendProperties() {
    char *buffer;

    buffer = linearizeProperties(properties, '\t');
    if (buffer) {
        wrapperProtocolFunction(FALSE, WRAPPER_MSG_PROPERTIES, buffer);
        free(buffer);
    }
}


/**
 * Immediately kill the JVM process and set the JVM state to
 *  WRAPPER_JSTATE_DOWN_CHECK.
 */
void wrapperKillProcessNow() {
#ifdef WIN32
    int ret;
#endif

    /* Check to make sure that the JVM process is still running */
#ifdef WIN32
    ret = WaitForSingleObject(wrapperData->javaProcess, 0);
    if (ret == WAIT_TIMEOUT) {
#else
    if (waitpid(wrapperData->javaPID, NULL, WNOHANG) == 0) {
#endif
        /* JVM is still up when it should have already stopped itself. */

        /* The JVM process is not responding so the only choice we have is to
         *  kill it. */
#ifdef WIN32
        /* The TerminateProcess funtion will kill the process, but it
         *  does not correctly notify the process's DLLs that it is shutting
         *  down.  Ideally, we would call ExitProcess, but that can only be
         *  called from within the process being killed. */
        if (TerminateProcess(wrapperData->javaProcess, 0)) {
#else
        if (kill(wrapperData->javaPID, SIGKILL) == 0) {
#endif
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM did not exit on request, terminated");
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "JVM did not exit on request.");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                "  Attempt to terminate process failed: %s", getLastErrorText());
        }

        /* Give the JVM a chance to be killed so that the state will be correct. */
        wrapperSleep(FALSE, 500); /* 0.5 seconds */

        /* Set the exit code since we were forced to kill the JVM. */
        wrapperData->exitCode = 1;
    }

    if (wrapperData->jvmCleanupTimeout > 0) {
        wrapperSetJavaState(FALSE, WRAPPER_JSTATE_DOWN_CHECK, wrapperGetTicks(), wrapperData->jvmCleanupTimeout);
    } else {
        wrapperSetJavaState(FALSE, WRAPPER_JSTATE_DOWN_CHECK, wrapperGetTicks(), -1);
    }

    /* Remove java pid file if it was registered and created by this process. */
    if (wrapperData->javaPidFilename) {
#ifdef WIN32
        _unlink(wrapperData->javaPidFilename);
#else
        unlink(wrapperData->javaPidFilename);
#endif
    }

#ifdef WIN32
    if (!CloseHandle(wrapperData->javaProcess)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            "Failed to close the Java process handle: %s", getLastErrorText());
    }
    wrapperData->javaProcess = NULL;
    wrapperData->javaPID = 0;
#else
    wrapperData->javaPID = -1;
#endif

    /* Close any open socket to the JVM */
    wrapperProtocolClose();
}

/**
 * Puts the Wrapper into a state where the JVM will be killed at the soonest
 *  possible opportunity.  It is necessary to wait a moment if a final thread
 *  dump is to be requested.  This call wll always set the JVM state to
 *  WRAPPER_JSTATE_KILLING.
 */
void wrapperKillProcess(int useLoggerQueue) {
#ifdef WIN32
    int ret;
#endif
    int delay = 0;

    if ((wrapperData->jState == WRAPPER_JSTATE_DOWN_CLEAN) ||
        (wrapperData->jState == WRAPPER_JSTATE_LAUNCH_DELAY) ||
        (wrapperData->jState == WRAPPER_JSTATE_DOWN_CHECK)) {
        /* Already down. */
        if (wrapperData->jState == WRAPPER_JSTATE_LAUNCH_DELAY) {
            wrapperSetJavaState(useLoggerQueue, WRAPPER_JSTATE_DOWN_CLEAN, wrapperGetTicks(), 0);
        }
        return;
    }

    /* Check to make sure that the JVM process is still running */
#ifdef WIN32
    ret = WaitForSingleObject(wrapperData->javaProcess, 0);
    if (ret == WAIT_TIMEOUT) {
#else
    if (waitpid(wrapperData->javaPID, NULL, WNOHANG) == 0) {
#endif
        /* JVM is still up when it should have already stopped itself. */
        if (wrapperData->requestThreadDumpOnFailedJVMExit) {
            wrapperRequestDumpJVMState(useLoggerQueue);

            delay = 5;
        }
    }

    wrapperSetJavaState(useLoggerQueue, WRAPPER_JSTATE_KILLING, wrapperGetTicks(), delay);
}

/**
 * Launch the wrapper as a console application.
 */
int wrapperRunConsole() {
    int res;
    const char *prop;
    struct tm timeTM;

    /* Setup the wrapperData structure. */
    wrapperSetWrapperState(FALSE, WRAPPER_WSTATE_STARTING);
    wrapperSetJavaState(FALSE, WRAPPER_JSTATE_DOWN_CLEAN, 0, -1);
    wrapperData->isConsole = TRUE;

    /* Initialize the wrapper */
    res = wrapperInitializeRun();
    if (res != 0) {
        return res;
    }

#ifdef WIN32
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "--> Wrapper Started as Console");
#else
    if (wrapperData->daemonize) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "--> Wrapper Started as Daemon");
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "--> Wrapper Started as Console");
    }
#endif

    /* Log a startup banner. */
    wrapperVersionBanner();
    
    /* Make sure the tick timer is working correctly. */
    if (wrapperTickAssertions()) {
        return 1;
    }

    /* The following code will display a licensed to block if a license key is found
     *  in the Wrapper configuration.  This piece of code is required as is for
     *  Development License owners to be in complience with their development license.
     *  This code does not do any validation of the license keys and works differently
     *  from the license code found in the Standard and Professional Editions of the
     *  Wrapper. */
    prop = getStringProperty(properties, "wrapper.license.type", "");
    if (strcmpIgnoreCase(prop, "dev") == 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
            "  Licensed to %s for %s",
            getStringProperty(properties, "wrapper.license.licensee", "(LICENSE INVALID)"),
            getStringProperty(properties, "wrapper.license.dev_application", "(LICENSE INVALID)"));
    }
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "");

    if (wrapperData->isDebugging) {
        timeTM = wrapperGetReleaseTime();
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Release time: %04d/%02d/%02d %02d:%02d:%02d",
                timeTM.tm_year + 1900, timeTM.tm_mon + 1, timeTM.tm_mday,
                timeTM.tm_hour, timeTM.tm_min, timeTM.tm_sec);

        timeTM = wrapperGetBuildTime();
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Build time:   %04d/%02d/%02d %02d:%02d:%02d",
                timeTM.tm_year + 1900, timeTM.tm_mon + 1, timeTM.tm_mday,
                timeTM.tm_hour, timeTM.tm_min, timeTM.tm_sec);

        /* Display timezone information. */
        tzset();
#ifndef FREEBSD
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Timezone:     %s (%s) Offset: %ld, hasDaylight: %d",
                tzname[0], tzname[1], timezone, daylight);
#else
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Timezone:     %s (%s) Offset: %ld",
                        tzname[0], tzname[1], timezone);
#endif
        if (wrapperData->useSystemTime) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Using system timer.");
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Using tick timer.");
        }
    }

#ifdef WRAPPER_FILE_DEBUG
    wrapperFileTests();
#endif

    /* Enter main event loop */
    wrapperEventLoop();

    /* Clean up any open sockets. */
    wrapperProtocolClose();
    protocolStopServer();
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "<-- Wrapper Stopped");

    return wrapperData->exitCode;
}

/**
 * Launch the wrapper as a service application.
 */
int wrapperRunService() {
    int res;

    /* Setup the wrapperData structure. */
    wrapperSetWrapperState(FALSE, WRAPPER_WSTATE_STARTING);
    wrapperSetJavaState(FALSE, WRAPPER_JSTATE_DOWN_CLEAN, 0, -1);
    wrapperData->isConsole = FALSE;

    /* Initialize the wrapper */
    res = wrapperInitializeRun();
    if (res != 0) {
        return res;
    }

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "--> Wrapper Started as Service");

    /* Log a startup banner. */
    wrapperVersionBanner();

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "");

    if (wrapperData->isDebugging) {
        if (wrapperData->useSystemTime) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Using system timer.");
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Using tick timer.");
        }
    }

    /* Enter main event loop */
    wrapperEventLoop();

    /* Clean up any open sockets. */
    wrapperProtocolClose();
    protocolStopServer();
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "<-- Wrapper Stopped");

    return wrapperData->exitCode;
}

/**
 * Used to ask the state engine to shut down the JVM and Wrapper
 */
void wrapperStopProcess(int useLoggerQueue, int exitCode) {
    /* If we are are not aready shutting down, then do so. */
    if ((wrapperData->wState == WRAPPER_WSTATE_STOPPING) ||
        (wrapperData->wState == WRAPPER_WSTATE_STOPPED)) {
        if (wrapperData->isDebugging) {
            log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "wrapperStopProcess(%d) called while stopping.  (IGNORED)", exitCode);
        }
    } else {
        if (wrapperData->isDebugging) {
            log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "wrapperStopProcess(%d) called.", exitCode);
        }

        /* If it has not already been set, set the exit request flag. */
        if (wrapperData->exitRequested ||
            (wrapperData->jState == WRAPPER_JSTATE_DOWN_CLEAN) ||
            (wrapperData->jState == WRAPPER_JSTATE_STOP) ||
            (wrapperData->jState == WRAPPER_JSTATE_STOPPING) ||
            (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
            (wrapperData->jState == WRAPPER_JSTATE_KILLING) ||
            (wrapperData->jState == WRAPPER_JSTATE_KILL) ||
            (wrapperData->jState == WRAPPER_JSTATE_DOWN_CHECK)) {
            /* JVM is already down or going down. */
        } else {
            wrapperData->exitRequested = TRUE;
        }

        wrapperData->exitCode = exitCode;

        /* Make sure that further restarts are disabled. */
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_NO;

        /* Do not call wrapperSetWrapperState(useLoggerQueue, WRAPPER_WSTATE_STOPPING) here.
         *  It will be called by the wrappereventloop.c.jStateDown once the
         *  the JVM is completely down.  Calling it here will make it
         *  impossible to trap and restart based on exit codes. */
    }
}

/**
 * Used to ask the state engine to shut down the JVM.  This are always intentional restart requests.
 */
void wrapperRestartProcess(int useLoggerQueue) {
    /* If it has not already been set, set the restart request flag in the wrapper data. */
    if (wrapperData->exitRequested || wrapperData->restartRequested ||
        (wrapperData->jState == WRAPPER_JSTATE_DOWN_CLEAN) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOP) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOPPING) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
        (wrapperData->jState == WRAPPER_JSTATE_KILLING) ||
        (wrapperData->jState == WRAPPER_JSTATE_KILL) ||
        (wrapperData->jState == WRAPPER_JSTATE_DOWN_CHECK) ||
        (wrapperData->jState == WRAPPER_JSTATE_LAUNCH_DELAY)) { /* Down but not yet restarted. */

        if (wrapperData->isDebugging) {
            log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "wrapperRestartProcess() called.  (IGNORED)");
        }
    } else {
        if (wrapperData->isDebugging) {
            log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "wrapperRestartProcess() called.");
        }

        wrapperData->exitRequested = TRUE;
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_CONFIGURED;
    }
}

/**
 * Loops over and strips all double quotes from prop and places the
 *  stripped version into propStripped.
 *
 * The exception is double quotes that are preceeded by a backslash
 *  in this case the backslash is stripped.
 *
 * If two backslashes are found in a row, then the first escapes the
 *  second and the second is removed.
 */
void wrapperStripQuotes(const char *prop, char *propStripped) {
    size_t len;
    int i, j;

    len = strlen(prop);
    j = 0;
    for (i = 0; i < (int)len; i++) {
        if ((prop[i] == '\\') && (i < (int)len - 1)) {
            if (prop[i + 1] == '\\') {
                /* Double backslash.  Keep the first, and skip the second. */
                propStripped[j] = prop[i];
                j++;
                i++;
            } else if (prop[i + 1] == '\"') {
                /* Escaped quote.  Keep the quote. */
                propStripped[j] = prop[i + 1];
                j++;
                i++;
            } else {
                /* Include the backslash as normal. */
                propStripped[j] = prop[i];
                j++;
            }
        } else if (prop[i] == '\"') {
            /* Quote.  Skip it. */
        } else {
            propStripped[j] = prop[i];
            j++;
        }
    }
    propStripped[j] = '\0';
}

/*
 * Corrects a windows path in place by replacing all '/' characters with '\'
 *  on Windows versions.
 *
 * filename - Filename to be modified.  Could be null.
 */
void correctWindowsPath(char *filename) {
#ifdef WIN32
    char *c;

    if (filename) {
        c = (char *)filename;
        while((c = strchr(c, '/')) != NULL) {
            c[0] = '\\';
        }
    }
#endif
}

/**
 * Adds quotes around the specified string in such a way that everything is
 *  escaped correctly.  If the bufferSize is not large enough then the
 *  required size will be returned.  0 is returned if successful.
 */
size_t wrapperQuoteValue(const char* value, char *buffer, size_t bufferSize) {
    size_t len = strlen(value);
    size_t in = 0;
    size_t out = 0;
    size_t in2;
    int escape;

    /* Initial quote. */
    if (out < bufferSize) {
        buffer[out] = '"';
    }
    out++;

    /* Copy over characters of value. */
    while ((in < len) && (value[in] != '\0')) {
        escape = FALSE;
        if (value[in] == '\\') {
            /* All '\' characters in a row prior to a '"' or the end of the string need to be
             *  escaped */
            in2 = in + 1;
            while ((in2 < len) && (value[in2] == '\\')) {
                in2++;
            }
            escape = ((in2 >= len) || (value[in2] == '"'));
        } else if (value[in] == '"') {
            escape = TRUE;
        }

        if (escape) {
            /* Needs to be escaped. */
            if (out < bufferSize) {
                buffer[out] = '\\';
            }
            out++;
        }
        if (out < bufferSize) {
            buffer[out] = value[in];
        }
        out++;
        in++;
    }

    /* Trailing quote. */
    if (out < bufferSize) {
        buffer[out] = '"';
    }
    out++;

    /* Null terminate. */
    if (out < bufferSize) {
        buffer[out] = '\0';
    }
    out++;

    if (out <= bufferSize) {
        return 0;
    } else {
        return out;
    }
}

/**
 * Checks the quotes in the value and displays an error if there are any problems.
 * This can be useful to help users debug quote problems.
 */
int wrapperCheckQuotes(const char *value, const char *propName) {
    size_t len = strlen(value);
    size_t in = 0;
    size_t in2 = 0;
    int inQuote = FALSE;
    int escaped;

    while (in < len) {
        if (value[in] == '"') {
            /* Decide whether or not this '"' is escaped. */
            in2 = in - 1;
            escaped = FALSE;
            while (value[in2] == '\\') {
                escaped = !escaped;
                if (in2 > 0) {
                    in2--;
                } else {
                    break;
                }
            }
            if (!escaped) {
                inQuote = !inQuote;
            }
        } else if (inQuote) {
            /* Quoted text. */
        } else {
            /* Unquoted. white space is bad. */
            if (value[in] == ' ') {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                    "The value of property '%s', '%s' contains unquoted spaces and will most likely result in an invalid java command line.",
                    propName, value);
                return 1;
            }
        }

        in++;
    }
    if (inQuote) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            "The value of property '%s', '%s' contains an unterminated quote and will most likely result in an invalid java command line.",
            propName, value);
        return 1;
    }
    return 0;
}
#ifndef WIN32
int checkIfExecutable(const char *filename) {
    int result;
    struct stat statInfo;
    result = stat(filename, &statInfo);
    if (result < 0) {
        return 0;
    }

    if (!S_ISREG(statInfo.st_mode)) {
        return 0;
    }
    if (statInfo.st_uid == geteuid()) {
        return statInfo.st_mode & S_IXUSR;
    }
    if (statInfo.st_gid == getegid()) {
        return statInfo.st_mode & S_IXGRP;
    }
    return statInfo.st_mode & S_IXOTH;

}
#endif

int checkIfBinary(const char *filename) {
    FILE* f;
    unsigned char head[5];
    int r;
    f = fopen(filename, "rb");
    if (!f) { /*couldnt find the java command... wrapper will moan later*/
       return 1;
    } else {
        r = (int)fread(head,1, 4, f);
        if (r != 4) {
            fclose(f);
            return 0;
        }
        fclose(f);
        head[4] = '\0';
#ifdef _DEBUG
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, "Magic [0]%02x [1]%02x [2]%02x [3]%02x", head[0], head[1], head[2], head[3]);
#endif

#if defined(LINUX) || defined(FREEBSD) || defined(SOLARIS)
        if (head[1] == 'E' && head[2] == 'L' && head[3] == 'F') {
            return 1; /*ELF */
#endif
#ifdef AIX
/* http://en.wikipedia.org/wiki/XCOFF */
        if (head[0] == 1 && head[1] == 247 && head[2] == 0) {
            return 1; /*xcoff 64*/
        } else if (head[0] == 1 && head[1] == 223 && head[2] == 0) {
            return 1; /*xcoff 32*/
#elif MACOSX
        if (head[0] == 202 && head[1] == 254 && head[2] == 186 && head[3] == 190) {
            return 1; /*MACOS*/
#elif HPUX
        if (head[0] == 2 && head[1] == 16 && head[2] == 1 && head[3] == 8) {
            return 1; /*HP UX PA RISC 32*/
#elif WIN32
        if (head[0] == 'M' && head[1] == 'Z') {
            return 1; /* MS */
#endif
        } else {
            return 0;
        }
    }
}


#ifndef WIN32
char* resolveLinks(char* exe) {
    char resolvedPath[PATH_MAX];
    char* returnVal;
    if (realpath(exe, resolvedPath) == NULL) {
        return NULL;
    } else {
        returnVal = malloc( (strlen(resolvedPath) + 1) * sizeof(char));
        if (!returnVal) {
            outOfMemory("RL", 1);
            return(NULL);
        } else {
            strcpy(returnVal, resolvedPath);
            return returnVal;
        }
    }
    return NULL;
}


char* findPathOf(const char *exe) {
    char *searchPath;
    char *beg, *end;
    int stop, found;
    char pth[PATH_MAX];
    char *pth2;
    char *ret;
    char resolvedPath[PATH_MAX];
    if (strchr(exe, '/') != NULL) {
        if (realpath(exe, resolvedPath) == NULL) {
            return NULL;
        }
        strcpy(pth, resolvedPath);
        if (checkIfExecutable(pth)) {
            ret = malloc((strlen(pth) + 1) * sizeof(char));
            strcpy(ret, pth);
            return ret;
        }
        return NULL;
    }
    searchPath = getenv("PATH");
    if (searchPath == NULL) {
        return NULL;
    }
    if (strlen(searchPath) <= 0) {
        return NULL;
    }
    beg = searchPath;
    stop = 0; found = 0;
    do {
        end = strchr(beg, ':');
        if (end == NULL) {
            stop = 1;
            strncpy(pth, beg, PATH_MAX);
        } else {
            strncpy(pth, beg, end - beg);
            pth[end - beg] = '\0';
        }
        if (pth[strlen(pth) - 1] != '/') {
            strcat(pth, "/");
        }
        strcat(pth, exe);
        found = checkIfExecutable(pth);
        if (!stop) {
            beg = end + 1;
        }
    } while (!stop && !found);
    if (found) {
        pth2 = malloc((strlen(pth) + 1) * sizeof(char));
        if (!pth2) {
            outOfMemory("FPO", 1);
            return NULL;
        }
        strcpy(pth2, pth);
        return resolveLinks(pth2);
    } else {
        return NULL;
    }
}
#endif

void checkIfRegularExe(char** para) {
    char* path;
#ifndef WIN32
    path = findPathOf(*para);
    if (!path) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, "The configured wrapper.java.command could not be found, attempting to launch anyway: %s", *para);
    } else {
        free(*para);
        *para = malloc((strlen(path) + 1) * sizeof(char));
        if (!(*para)) {
            outOfMemory("CIRE", 2);
            return;
        }
        strcpy(*para, path);
#else
    int len, start;
    if (strchr(*para, '\"') != NULL) {
        start = 1;
        len = (int)strlen(*para) * sizeof(char) - 2;
    } else {
        start = 0;
        len = (int)strlen(*para) * sizeof(char);
    }
    path = malloc(sizeof(char) * (len + 1));
    if (!path) {
        outOfMemory("CIRE", 1);
    } else {
        strncpy(path, (*para) + start,len);
        path[len] = '\0';
#endif
        if (!checkIfBinary(path)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, "The value of wrapper.java.command does not appear to be a java binary.");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, "The use of scripts is not supported. Trying to continue, but some features may not work correctly..");
        }
        free(path);
    }
}

int wrapperBuildJavaCommandArrayJavaCommand(char **strings, int addQuotes, int detectDebugJVM, int index) {
    const char *prop;
    char *c;
#ifdef WIN32
    char cpPath[512];
    int found;
#endif

    if (strings) {
        prop = getStringProperty(properties, "wrapper.java.command", "java");

#ifdef WIN32
        found = 0;

        if (strcmp(prop, "") == 0) {
            /* If the java command is an empty string, we want to look for the
             *  the java command in the windows registry. */
            if (wrapperGetJavaHomeFromWindowsRegistry(cpPath)) {
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                        "Loaded java home from registry: %s", cpPath);
                }

                addProperty(properties, "set.WRAPPER_JAVA_HOME", cpPath, FALSE, FALSE, FALSE);

                strcat(cpPath, "\\bin\\java.exe");
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                        "Found Java Runtime Environment home directory in system registry.");
                }
                found = 1;
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    "The Java Runtime Environment home directory could not be located in the system registry.");
                found = 0;
                return -1;
            }
        } else {
            /* To avoid problems on Windows XP systems, the '/' characters must
             *  be replaced by '\' characters in the specified path.
             * prop is supposed to be constant, but allow this change as it is
             *  the actual value that we want. */
            correctWindowsPath((char *)prop);
    
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
        }

        if (found) {
            strings[index] = malloc(sizeof(char) * (strlen(cpPath) + 2 + 1));
            if (!strings[index]) {
                outOfMemory("WBJCAJC", 1);
                return -1;
            }
            if (addQuotes) {
                sprintf(strings[index], "\"%s\"", cpPath);
            } else {
                sprintf(strings[index], "%s", cpPath);
            }
        } else {
            strings[index] = malloc(sizeof(char) * (strlen(prop) + 2 + 1));
            if (!strings[index]) {
                outOfMemory("WBJCAJC", 2);
                return -1;
            }
            if (addQuotes) {
                sprintf(strings[index], "\"%s\"", prop);
            } else {
                sprintf(strings[index], "%s", prop);
            }
        }

        if (addQuotes) {
            wrapperCheckQuotes(strings[index], "wrapper.java.command");
        }

#else /* UNIX */

        strings[index] = malloc(sizeof(char) * (strlen(prop) + 2 + 1));
        if (!strings[index]) {
            outOfMemory("WBJCAJC", 3);
            return -1;
        }
        if (addQuotes) {
            sprintf(strings[index], "\"%s\"", prop);
        } else {
            sprintf(strings[index], "%s", prop);
        }
#endif
        checkIfRegularExe(&strings[index]);
        if (detectDebugJVM) {
            c = strstr(strings[index], "jdb");
            if (c && ((unsigned int)(c - strings[index]) == strlen(strings[index]) - 3 - 1)) {
                /* Ends with "jdb".  The jdb debugger is being used directly.  go into debug JVM mode. */
                wrapperData->debugJVM = TRUE;
            } else {
                c = strstr(strings[index], "jdb.exe");
                if (c && ((unsigned int)(c - strings[index]) == strlen(strings[index]) - 7 - 1)) {
                    /* Ends with "jdb".  The jdb debugger is being used directly.  go into debug JVM mode. */
                    wrapperData->debugJVM = TRUE;
                }
            }
        }
    }
    index++;

    return index;
}

int wrapperBuildJavaCommandArrayJavaAdditional(char **strings, int addQuotes, int detectDebugJVM, int index) {
    const char *prop;
    int i;
    size_t len;
    char paramBuffer[128];
    char paramBuffer2[128];
    int quotable;
    int stripQuote;
    char *propStripped;
    char **propertyNames;
    char **propertyValues;
    long unsigned int *propertyIndices;

    if (getStringProperties(properties, "wrapper.java.additional.", "", wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
        /* Failed */
        return -1;
    }

    i = 0;
    while (propertyNames[i]) {
        prop = propertyValues[i];
        if (prop) {
            if (strlen(prop) > 0) {
                if (strings) {
                    /* All additional parameters must begin with a - or they will be interpretted
                     *  as the being the main class name by Java. */
                    if (!((strstr(prop, "-") == prop) || (strstr(prop, "\"-") == prop))) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                            "The value of property '%s', '%s' is not a valid argument to the jvm.  Skipping.",
                            paramBuffer, prop);
                        strings[index] = malloc(sizeof(char) * 1);
                        if (!strings[index]) {
                            outOfMemory("WBJCAJA", 1);
                            return -1;
                        }
                        strings[index][0] = '\0';
                    } else {
                        quotable = isQuotableProperty(properties, paramBuffer);
                        sprintf(paramBuffer2, "wrapper.java.additional.%lu.stripquotes", propertyIndices[i]);
                        if (addQuotes) {
                            stripQuote = FALSE;
                        } else {
                            stripQuote = getBooleanProperty(properties, paramBuffer2, FALSE);
                        }
                        if (stripQuote) {
                            propStripped = malloc(sizeof(char) * (strlen(prop) + 1));
                            if (!propStripped) {
                                outOfMemory("WBJCAJA", 2);
                                return -1;
                            }
                            wrapperStripQuotes(prop, propStripped);
                        } else {
                            propStripped = (char *)prop;
                        }

                        if (addQuotes && quotable && strchr(propStripped, ' ')) {
                            len = wrapperQuoteValue(propStripped, NULL, 0);
                            strings[index] = malloc(len);
                            if (!strings[index]) {
                                outOfMemory("WBJCAJA", 3);
                                return -1;
                            }
                            wrapperQuoteValue(propStripped, strings[index], len);
                        } else {
                            strings[index] = malloc(sizeof(char) * (strlen(propStripped) + 1));
                            if (!strings[index]) {
                                outOfMemory("WBJCAJA", 4);
                                return -1;
                            }
                            sprintf(strings[index], "%s", propStripped);
                        }

                        if (addQuotes) {
                            wrapperCheckQuotes(strings[index], paramBuffer);
                        }

                        if (stripQuote) {
                            free(propStripped);
                            propStripped = NULL;
                        }
                    }

                    /* Set if this paremeter enables debugging. */
                    if (detectDebugJVM) {
                        if (strstr(strings[index], "-Xdebug") == strings[index]) {
                            wrapperData->debugJVM = TRUE;
                        }
                    }
                }
                index++;
            }
            i++;
        }
    }
    freeStringProperties(propertyNames, propertyValues, propertyIndices);

    return index;
}

int wrapperBuildJavaCommandArrayLibraryPath(char **strings, int addQuotes, int index) {
    const char *prop;
    int i, j;
    size_t len2;
    size_t cpLen, cpLenAlloc;
    char *tmpString;
    char *systemPath;
    char **propertyNames;
    char **propertyValues;
    long unsigned int *propertyIndices;

    if (strings) {
        if (wrapperData->libraryPathAppendPath) {
            /* We are going to want to append the full system path to
             *  whatever library path is generated. */
#ifdef WIN32
            systemPath = getenv("PATH");
#else
            systemPath = getenv("LD_LIBRARY_PATH");
#endif
            /* If we are going to add our own quotes then we need to make sure that the system
             *  PATH doesn't contain any of its own.  Windows allows users to do this... */
            if (addQuotes) {
                i = 0;
                j = 0;
                do {
                    if (systemPath[i] != '"') {
                        systemPath[j] = systemPath[i];
                        j++;
                    }
                    i++;
                } while (systemPath[j] != '\0');
            }
        } else {
            systemPath = NULL;
        }

        prop = getStringProperty(properties, "wrapper.java.library.path", NULL);
        if (prop) {
            /* An old style library path was specified.
             * If quotes are being added, check the last character before the
             *  closing quote. If it is a backslash then Windows will use it to
             *  escape the quote.  To make things work correctly, we need to add
             *  another backslash first so it will result in a single backslash
             *  before the quote. */
            if (systemPath) {
                strings[index] = malloc(sizeof(char) * (22 + strlen(prop) + 1 + strlen(systemPath) + 1 + 1));
                if (!strings[index]) {
                    outOfMemory("WBJCALP", 1);
                    return -1;
                }
                if (addQuotes) {
                    if ((strlen(systemPath) > 1) && (systemPath[strlen(systemPath) - 1] == '\\')) {
                        sprintf(strings[index], "-Djava.library.path=\"%s%c%s\\\"", prop, wrapperClasspathSeparator, systemPath);
                    } else {
                        sprintf(strings[index], "-Djava.library.path=\"%s%c%s\"", prop, wrapperClasspathSeparator, systemPath);
                    }
                } else {
                    sprintf(strings[index], "-Djava.library.path=%s%c%s", prop, wrapperClasspathSeparator, systemPath);
                }
            } else {
                strings[index] = malloc(sizeof(char) * (22 + strlen(prop) + 1 + 1));
                if (!strings[index]) {
                    outOfMemory("WBJCALP", 2);
                    return -1;
                }
                if (addQuotes) {
                    if ((strlen(prop) > 1) && (prop[strlen(prop) - 1] == '\\')) {
                        sprintf(strings[index], "-Djava.library.path=\"%s\\\"", prop);
                    } else {
                        sprintf(strings[index], "-Djava.library.path=\"%s\"", prop);
                    }
                } else {
                    sprintf(strings[index], "-Djava.library.path=%s", prop);
                }
            }

            if (addQuotes) {
                wrapperCheckQuotes(strings[index], "wrapper.java.library.path");
            }
        } else {
            /* Look for a multiline library path. */
            cpLen = 0;
            cpLenAlloc = 1024;
            strings[index] = malloc(sizeof(char) * cpLenAlloc);
            if (!strings[index]) {
                outOfMemory("WBJCALP", 3);
                return -1;
            }

            /* Start with the property value. */
            sprintf(&(strings[index][cpLen]), "-Djava.library.path=");
            cpLen += 20;

            /* Add an open quote to the library path */
            if (addQuotes) {
                sprintf(&(strings[index][cpLen]), "\"");
                cpLen++;
            }

            /* Loop over the library path entries adding each one */
            if (getStringProperties(properties, "wrapper.java.library.path.", "", wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
                /* Failed */
                return -1;
            }

            i = 0;
            j = 0;
            while (propertyNames[i]) {
                prop = propertyValues[i];
                if (prop) {
                    len2 = strlen(prop);
                    if (len2 > 0) {
                        /* Is there room for the entry? */
                        while (cpLen + len2 + 3 > cpLenAlloc) {
                            /* Resize the buffer */
                            tmpString = strings[index];
                            cpLenAlloc += 1024;
                            strings[index] = malloc(sizeof(char) * cpLenAlloc);
                            if (!strings[index]) {
                                outOfMemory("WBJCALP", 4);
                                return -1;
                            }
                            sprintf(strings[index], "%s", tmpString);
                            free(tmpString);
                            tmpString = NULL;
                        }
                        
                        if (j > 0) {
                            strings[index][cpLen++] = wrapperClasspathSeparator; /* separator */
                        }
                        sprintf(&(strings[index][cpLen]), "%s", prop);
                        cpLen += len2;
                        j++;
                    }
                    i++;
                }
            }
            freeStringProperties(propertyNames, propertyValues, propertyIndices);

            if (systemPath) {
                /* We need to append the system path. */
                len2 = strlen(systemPath);
                if (len2 > 0) {
                    /* Is there room for the entry? */
                    while (cpLen + len2 + 3 > cpLenAlloc) {
                        /* Resize the buffer */
                        tmpString = strings[index];
                        cpLenAlloc += 1024;
                        strings[index] = malloc(sizeof(char) * cpLenAlloc);
                        if (!strings[index]) {
                            outOfMemory("WBJCALP", 5);
                            return -1;
                        }
                        sprintf(strings[index], "%s", tmpString);
                        free(tmpString);
                        tmpString = NULL;
                    }
                    
                    if (j > 0) {
                        strings[index][cpLen++] = wrapperClasspathSeparator; /* separator */
                    }
                    sprintf(&(strings[index][cpLen]), "%s", systemPath);
                    cpLen += len2;
                    j++;
                }
            }

            if (j == 0) {
                /* No library path, use default. always room */
                sprintf(&(strings[index][cpLen++]), "./");
            }
            /* Add ending quote.  If the previous character is a backslash then
             *  Windows will use it to escape the quote.  To make things work
             *  correctly, we need to add another backslash first so it will
             *  result in a single backslash before the quote. */
            if (addQuotes) {
                if (strings[index][cpLen - 1] == '\\') {
                    sprintf(&(strings[index][cpLen]), "\\");
                    cpLen++;
                }
                sprintf(&(strings[index][cpLen]), "\"");
                cpLen++;
            }

            if (addQuotes) {
                wrapperCheckQuotes(strings[index], "wrapper.java.library.path.<n>");
            }
        }
    }
    index++;

    return index;
}

int wrapperBuildJavaClasspath(char **classpath) {
    const char *prop;
    char *propStripped;
    char *propBaseDir;
    int i, j;
    size_t cpLen, cpLenAlloc;
    size_t len2;
    char *tmpString;
    struct stat statBuffer;
    char **propertyNames;
    char **propertyValues;
    long unsigned int *propertyIndices;
    char **files;
    int cnt;
    
    /* Build a classpath */
    cpLen = 0;
    cpLenAlloc = 1024;
    *classpath = malloc(sizeof(char) * cpLenAlloc);
    if (!*classpath) {
        outOfMemory("WBJCP", 1);
        return -1;
    }
    
    /* Loop over the classpath entries adding each one. */
    if (getStringProperties(properties, "wrapper.java.classpath.", "", wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
        /* Failed */
        return -1;
    }

    i = 0;
    j = 0;
    while (propertyNames[i]) {
        prop = propertyValues[i];
        
        /* Does this contain any quotes? */
        if (strchr(prop, '"')) {
            propStripped = malloc(sizeof(char) * (strlen(prop) + 1));
            if (!propStripped) {
                outOfMemory("WBJCP", 2);
                return -1;
            }
            wrapperStripQuotes(prop, propStripped);
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "Classpath element, %s, should not contain quotes: %s, stripping and continuing: %s", propertyNames[i], prop, propStripped);
        } else {
            propStripped = (char *)prop;
        }
        
        len2 = strlen(propStripped);
        if (len2 > 0) {
            /* Does this contain wildcards? */
            if ((strchr(propStripped, '*') != NULL) || (strchr(propStripped, '?') != NULL)) {
                /* Need to do a wildcard search */
                files = wrapperFileGetFiles(propStripped, WRAPPER_FILE_SORT_MODE_NAMES_ASC);
                if (!files) {
                    /* Failed */
                    return -1;
                }

                /* Loop over the files. */
                cnt = 0;
                while (files[cnt]) {
                    len2 = strlen(files[cnt]);

                    /* Is there room for the entry? */
                    while (cpLen + len2 + 3 > cpLenAlloc) {
                        /* Resize the buffer */
                        tmpString = *classpath;
                        cpLenAlloc += 1024;
                        *classpath = malloc(sizeof(char) * cpLenAlloc);
                        if (!*classpath) {
                            outOfMemory("WBJCP", 2);
                            return -1;
                        }
                        sprintf(*classpath, "%s", tmpString);
                        free(tmpString);
                        tmpString = NULL;
                    }

                    if (j > 0) {
                        (*classpath)[cpLen++] = wrapperClasspathSeparator; /* separator */
                    }
                    sprintf(&((*classpath)[cpLen]), "%s", files[cnt]);
                    cpLen += len2;
                    j++;
                    cnt++;
                }
                wrapperFileFreeFiles(files);
            } else {
                /* This classpath entry does not contain any wildcards. */

                /* If the path element is a directory then we want to strip the trailing slash if it exists. */
                propBaseDir = (char*)propStripped;
                if ((propStripped[strlen(propStripped) - 1] == '/') || (propStripped[strlen(propStripped) - 1] == '\\')) {
                    propBaseDir = malloc(sizeof(char) * strlen(propStripped));
                    if (!propBaseDir) {
                        outOfMemory("WBJCP", 3);
                        if (propStripped != prop) {
                            free(propStripped);
                        }
                        return -1;
                    }
                    memcpy(propBaseDir, propStripped, strlen(propStripped) - 1);
                    propBaseDir[strlen(propStripped) - 1] = '\0';
                }

                /* See if it exists so we can display a debug warning if it does not. */
                if (stat(propBaseDir, &statBuffer)) {
                    /* Encountered an error of some kind. */
                    if ((errno == ENOENT) || (errno == 3)) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                            "Classpath element, %s, does not exist: %s", propertyNames[i], propStripped);
                    } else {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                            "Unable to get information of classpath element: %s (%s)",
                            propStripped, getLastErrorText());
                    }
                } else {
                    /* Got the stat info. */
                }

                /* If we allocated the propBaseDir buffer then free it up. */
                if (propBaseDir != propStripped) {
                    free(propBaseDir);
                }
                propBaseDir = NULL;

                /* Is there room for the entry? */
                while (cpLen + len2 + 3 > cpLenAlloc) {
                    /* Resize the buffer */
                    tmpString = *classpath;
                    cpLenAlloc += 1024;
                    *classpath = malloc(sizeof(char) * cpLenAlloc);
                    if (!*classpath) {
                        outOfMemory("WBJCP", 4);
                        if (propStripped != prop) {
                            free(propStripped);
                        }
                        return -1;
                    }
                    sprintf(*classpath, "%s", tmpString);
                    free(tmpString);
                    tmpString = NULL;
                }

                if (j > 0) {
                    (*classpath)[cpLen++] = wrapperClasspathSeparator; /* separator */
                }
                sprintf(&((*classpath)[cpLen]), "%s", propStripped);
                cpLen += len2;
                j++;
            }
        }

        /* If we allocated the propStripped buffer then free it up. */
        if (propStripped != prop) {
            free(propStripped);
        }
        propStripped = NULL;
        
        i++;
    }
    freeStringProperties(propertyNames, propertyValues, propertyIndices);
    if (j == 0) {
        /* No classpath, use default. always room */
        sprintf(&(*classpath[cpLen++]), "./");
    }
    
    return 0;
}

int wrapperBuildJavaCommandArrayClasspath(char **strings, int addQuotes, int index, const char *classpath) {
    size_t len;
    size_t cpLen;

    /* Store the classpath */
    if (strings) {
        strings[index] = malloc(sizeof(char) * (10 + 1));
        if (!strings[index]) {
            outOfMemory("WBJCAC", 1);
            return -1;
        }
        sprintf(strings[index], "-classpath");
    }
    index++;
    if (strings) {
        cpLen = 0;
        
        len = strlen(classpath);
        strings[index] = malloc(sizeof(char) * (len + 4));
        if (!strings[index]) {
            outOfMemory("WBJCAC", 2);
            return -1;
        }
        
        /* Add an open quote the classpath */
        if (addQuotes) {
            sprintf(&(strings[index][cpLen]), "\"");
            cpLen++;
        }
        
        sprintf(&(strings[index][cpLen]), "%s", classpath);
        cpLen += len;

        /* Add ending quote.  If the previous character is a backslash then
         *  Windows will use it to escape the quote.  To make things work
         *  correctly, we need to add another backslash first so it will
         *  result in a single backslash before the quote. */
        if (addQuotes) {
            if (strings[index][cpLen - 1] == '\\') {
                sprintf(&(strings[index][cpLen]), "\\");
                cpLen++;
            }
            sprintf(&(strings[index][cpLen]), "\"");
            cpLen++;
        }

        if (addQuotes) {
            wrapperCheckQuotes(strings[index], "wrapper.java.classpath.<n>");
        }
    }
    index++;

    return index;
}

int wrapperBuildJavaCommandArrayAppParameters(char **strings, int addQuotes, int index) {
    const char *prop;
    int i;
    int quotable;
    char *propStripped;
    int stripQuote;
    char paramBuffer2[128];
    size_t len;
    char **propertyNames;
    char **propertyValues;
    long unsigned int *propertyIndices;

    if (getStringProperties(properties, "wrapper.app.parameter.", "", wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
        /* Failed */
        return -1;
    }
    i = 0;
    while (propertyNames[i]) {
        prop = propertyValues[i];
        if (strlen(prop) > 0) {
            if (strings) {
                quotable = isQuotableProperty(properties, propertyNames[i]);
                sprintf(paramBuffer2, "wrapper.app.parameter.%lu.stripquotes", propertyIndices[i]);
                if (addQuotes) {
                    stripQuote = FALSE;
                } else {
                    stripQuote = getBooleanProperty(properties, paramBuffer2, FALSE);
                }
                if (stripQuote) {
                    propStripped = malloc(sizeof(char) * (strlen(prop) + 1));
                    if (!propStripped) {
                        outOfMemory("WBJCAAP", 1);
                        return -1;
                    }
                    wrapperStripQuotes(prop, propStripped);
                } else {
                    propStripped = (char *)prop;
                }

                if (addQuotes && quotable && strchr(propStripped, ' ')) {
                    len = wrapperQuoteValue(propStripped, NULL, 0);
                    strings[index] = malloc(len);
                    if (!strings[index]) {
                        outOfMemory("WBJCAAP", 2);
                        return -1;
                    }
                    wrapperQuoteValue(propStripped, strings[index], len);
                } else {
                    strings[index] = malloc(sizeof(char) * (strlen(propStripped) + 1));
                    if (!strings[index]) {
                        outOfMemory("WBJCAAP", 3);
                        return -1;
                    }
                    sprintf(strings[index], "%s", propStripped);
                }

                if (addQuotes) {
                    wrapperCheckQuotes(strings[index], propertyNames[i]);
                }

                if (stripQuote) {
                    free(propStripped);
                    propStripped = NULL;
                }
            }
            index++;
        }
        i++;
    }
    freeStringProperties(propertyNames, propertyValues, propertyIndices);
    return index;
}

/**
 * Loops over and stores all necessary commands into an array which
 *  can be used to launch a process.
 * This method will only count the elements if stringsPtr is NULL.
 *
 * Note - Next Out Of Memory is #47
 */
int wrapperBuildJavaCommandArrayInner(char **strings, int addQuotes, const char *classpath) {
    int index;
    int detectDebugJVM;
    const char *prop;
    int initMemory = 0, maxMemory;

    index = 0;

    detectDebugJVM = getBooleanProperty(properties, "wrapper.java.detect_debug_jvm", TRUE);

    /* Java commnd */
    if ((index = wrapperBuildJavaCommandArrayJavaCommand(strings, addQuotes, detectDebugJVM, index)) < 0) {
        return -1;
    }

    /* See if the auto bits parameter is set.  Ignored by all but the following platforms. */
#if defined(HPUX) || defined(MACOSX) || defined(SOLARIS) || defined(FREEBSD)
    if (getBooleanProperty(properties, "wrapper.java.additional.auto_bits", FALSE)) {
        if (strings) {
            strings[index] = malloc(sizeof(char) * 5);
            if (!strings[index]) {
                outOfMemory("WBJCAI", 46);
                return -1;
            }
            sprintf(strings[index], "-d%s", wrapperBits);
        }
        index++;
    }
#endif

    /* Store additional java parameters */
    if ((index = wrapperBuildJavaCommandArrayJavaAdditional(strings, addQuotes, detectDebugJVM, index)) < 0) {
        return -1;
    }

    /* Initial JVM memory */
    initMemory = getIntProperty(properties, "wrapper.java.initmemory", 0);
    if (initMemory > 0) {
        if (strings) {
            initMemory = __max(initMemory, 1); /* 1 <= n */
            strings[index] = malloc(sizeof(char) * (5 + 10 + 1));  /* Allow up to 10 digits. */
            if (!strings[index]) {
                outOfMemory("WBJCAI", 8);
                return -1;
            }
            sprintf(strings[index], "-Xms%dm", initMemory);
        }
        index++;
    } else {
            /* Set the initMemory so the checks in the maxMemory section below will work correctly. */
            initMemory = 3;
    }

    /* Maximum JVM memory */
    maxMemory = getIntProperty(properties, "wrapper.java.maxmemory", 0);
    if (maxMemory > 0) {
        if (strings) {
            maxMemory = __max(maxMemory, initMemory);  /* initMemory <= n */
            strings[index] = malloc(sizeof(char) * (5 + 10 + 1));  /* Allow up to 10 digits. */
            if (!strings[index]) {
                outOfMemory("WBJCAI", 10);
                return -1;
            }
            sprintf(strings[index], "-Xmx%dm", maxMemory);
        }
        index++;
    }

    /* Library Path */
    if ((index = wrapperBuildJavaCommandArrayLibraryPath(strings, addQuotes, index)) < 0) {
        return -1;
    }

    /* Classpath */
    if (!wrapperData->environmentClasspath) {
        if ((index = wrapperBuildJavaCommandArrayClasspath(strings, addQuotes, index, classpath)) < 0) {
            return -1;
        }
    }

    /* Store the Wrapper key */
    if (strings) {
        strings[index] = malloc(sizeof(char) * (16 + strlen(wrapperData->key) + 1));
        if (!strings[index]) {
            outOfMemory("WBJCAI", 24);
            return -1;
        }
        if (addQuotes) {
            sprintf(strings[index], "-Dwrapper.key=\"%s\"", wrapperData->key);
        } else {
            sprintf(strings[index], "-Dwrapper.key=%s", wrapperData->key);
        }
    }
    index++;

    /* Store the Wrapper server port */
    if (strings) {
        strings[index] = malloc(sizeof(char) * (15 + 5 + 1));  /* Port up to 5 characters */
        if (!strings[index]) {
            outOfMemory("WBJCAI", 25);
            return -1;
        }
        sprintf(strings[index], "-Dwrapper.port=%d", (int)wrapperData->actualPort);
    }
    index++;

    /* Store the Wrapper jvm min and max ports. */
    if (wrapperData->jvmPort > 0) {
        if (strings) {
            strings[index] = malloc(sizeof(char) * (19 + 5 + 1));  /* Port up to 5 characters */
            if (!strings[index]) {
                outOfMemory("WBJCAI", 26);
                return -1;
            }
            sprintf(strings[index], "-Dwrapper.jvm.port=%d", (int)wrapperData->jvmPort);
        }
        index++;
    }
    if (strings) {
        strings[index] = malloc(sizeof(char) * (23 + 5 + 1));  /* Port up to 5 characters */
        if (!strings[index]) {
            outOfMemory("WBJCAI", 27);
            return -1;
        }
        sprintf(strings[index], "-Dwrapper.jvm.port.min=%d", (int)wrapperData->jvmPortMin);
    }
    index++;
    if (strings) {
        strings[index] = malloc(sizeof(char) * (23 + 5 + 1));  /* Port up to 5 characters */
        if (!strings[index]) {
            outOfMemory("WBJCAI", 28);
            return -1;
        }
        sprintf(strings[index], "-Dwrapper.jvm.port.max=%d", (int)wrapperData->jvmPortMax);
    }
    index++;

    /* Store the Wrapper debug flag */
    if (wrapperData->isDebugging) {
        if (strings) {
            strings[index] = malloc(sizeof(char) * (22 + 1));
            if (!strings[index]) {
                outOfMemory("WBJCAI", 29);
                return -1;
            }
            if (addQuotes) {
                sprintf(strings[index], "-Dwrapper.debug=\"TRUE\"");
            } else {
                sprintf(strings[index], "-Dwrapper.debug=TRUE");
            }
        }
        index++;
    }
    
    /* Store the Wrapper disable console input flag. */
    if (getBooleanProperty(properties, "wrapper.disable_console_input",
#ifdef WIN32
            FALSE
#else
            wrapperData->daemonize /* We want to disable console input by default when daemonized. */
#endif
        )) {
        if (strings) {
            strings[index] = malloc(sizeof(char) * (38 + 1));
            if (!strings[index]) {
                outOfMemory("WBJCAI", 29);
                return -1;
            }
            if (addQuotes) {
                sprintf(strings[index], "-Dwrapper.disable_console_input=\"TRUE\"");
            } else {
                sprintf(strings[index], "-Dwrapper.disable_console_input=TRUE");
            }
        }
        index++;
    }

    /* Store the Wrapper listener force stop flag. */
    if (getBooleanProperty(properties, "wrapper.listener.force_stop", FALSE)) {
        if (strings) {
            strings[index] = malloc(sizeof(char) * (38 + 1));
            if (!strings[index]) {
                outOfMemory("WBJCAI", 30);
                return -1;
            }
            if (addQuotes) {
                sprintf(strings[index], "-Dwrapper.listener.force_stop=\"TRUE\"");
            } else {
                sprintf(strings[index], "-Dwrapper.listener.force_stop=TRUE");
            }
        }
        index++;
    }

    /* Store the Wrapper PID */
    if (strings) {
        strings[index] = malloc(sizeof(char) * (24 + 1)); /* Pid up to 10 characters */
        if (!strings[index]) {
            outOfMemory("WBJCAI", 31);
            return -1;
        }
#if defined(SOLARIS) && (!defined(_LP64))
        sprintf(strings[index], "-Dwrapper.pid=%ld", wrapperData->wrapperPID);
#else
        sprintf(strings[index], "-Dwrapper.pid=%d", wrapperData->wrapperPID);
#endif
    }
    index++;

    /* Store a flag telling the JVM to use the system clock. */
    if (wrapperData->useSystemTime) {
        if (strings) {
            strings[index] = malloc(sizeof(char) * (32 + 1));
            if (!strings[index]) {
                outOfMemory("WBJCAI", 32);
                return -1;
            }
            if (addQuotes) {
                sprintf(strings[index], "-Dwrapper.use_system_time=\"TRUE\"");
            } else {
                sprintf(strings[index], "-Dwrapper.use_system_time=TRUE");
            }
        }
        index++;
    } else {
        /* Only pass the timer fast and slow thresholds to the JVM if they are not default.
         *  These are only used if the system time is not being used. */
        if (wrapperData->timerFastThreshold != WRAPPER_TIMER_FAST_THRESHOLD) {
            if (strings) {
                strings[index] = malloc(sizeof(char) * (43 + 1)); /* Allow for 10 digits */
                if (!strings[index]) {
                    outOfMemory("WBJCAI", 33);
                    return -1;
                }
                if (addQuotes) {
                    sprintf(strings[index], "-Dwrapper.timer_fast_threshold=\"%d\"", wrapperData->timerFastThreshold * WRAPPER_TICK_MS / 1000);
                } else {
                    sprintf(strings[index], "-Dwrapper.timer_fast_threshold=%d", wrapperData->timerFastThreshold * WRAPPER_TICK_MS / 1000);
                }
            }
            index++;
        }
        if (wrapperData->timerSlowThreshold != WRAPPER_TIMER_SLOW_THRESHOLD) {
            if (strings) {
                strings[index] = malloc(sizeof(char) * (43 + 1)); /* Allow for 10 digits */
                if (!strings[index]) {
                    outOfMemory("WBJCAI", 34);
                    return -1;
                }
                if (addQuotes) {
                    sprintf(strings[index], "-Dwrapper.timer_slow_threshold=\"%d\"", wrapperData->timerSlowThreshold * WRAPPER_TICK_MS / 1000);
                } else {
                    sprintf(strings[index], "-Dwrapper.timer_slow_threshold=%d", wrapperData->timerSlowThreshold * WRAPPER_TICK_MS / 1000);
                }
            }
            index++;
        }
    }

    /* Always write the version of the wrapper binary as a property.  The
     *  WrapperManager class uses it to verify that the version matches. */
    if (strings) {
        strings[index] = malloc(sizeof(char) * (20 + strlen(wrapperVersion) + 1));
        if (!strings[index]) {
            outOfMemory("WBJCAI", 35);
            return -1;
        }
        if (addQuotes) {
            sprintf(strings[index], "-Dwrapper.version=\"%s\"", wrapperVersion);
        } else {
            sprintf(strings[index], "-Dwrapper.version=%s", wrapperVersion);
        }
    }
    index++;

    /* Store the base name of the native library. */
    if (strings) {
        strings[index] = malloc(sizeof(char) * (27 + strlen(wrapperData->nativeLibrary) + 1));
        if (!strings[index]) {
            outOfMemory("WBJCAI", 36);
            return -1;
        }
        if (addQuotes) {
            sprintf(strings[index], "-Dwrapper.native_library=\"%s\"", wrapperData->nativeLibrary);
        } else {
            sprintf(strings[index], "-Dwrapper.native_library=%s", wrapperData->nativeLibrary);
        }
    }
    index++;

    /* Store the ignore signals flag if configured to do so */
    if (wrapperData->ignoreSignals & WRAPPER_IGNORE_SIGNALS_JAVA) {
        if (strings) {
            strings[index] = malloc(sizeof(char) * (31 + 1));
            if (!strings[index]) {
                outOfMemory("WBJCAI", 37);
                return -1;
            }
            if (addQuotes) {
                sprintf(strings[index], "-Dwrapper.ignore_signals=\"TRUE\"");
            } else {
                sprintf(strings[index], "-Dwrapper.ignore_signals=TRUE");
            }
        }
        index++;
    }

    /* If this is being run as a service, add a service flag. */
#ifdef WIN32
    if (!wrapperData->isConsole) {
#else
    if (wrapperData->daemonize) {
#endif
        if (strings) {
            strings[index] = malloc(sizeof(char) * (24 + 1));
            if (!strings[index]) {
                outOfMemory("WBJCAI", 38);
                return -1;
            }
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
            strings[index] = malloc(sizeof(char) * (38 + 1));
            if (!strings[index]) {
                outOfMemory("WBJCAI", 39);
                return -1;
            }
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
        strings[index] = malloc(sizeof(char) * (24 + 20 + 1));
        if (!strings[index]) {
            outOfMemory("WBJCAI", 40);
            return -1;
        }
        if (addQuotes) {
            sprintf(strings[index], "-Dwrapper.cpu.timeout=\"%d\"", wrapperData->cpuTimeout);
        } else {
            sprintf(strings[index], "-Dwrapper.cpu.timeout=%d", wrapperData->cpuTimeout);
        }
    }
    index++;

    /* Store the Wrapper JVM ID.  (Get here before incremented) */
    if (strings) {
        strings[index] = malloc(sizeof(char) * (16 + 5 + 1));  /* jvmid up to 5 characters */
        if (!strings[index]) {
            outOfMemory("WBJCAI", 41);
            return -1;
        }
        sprintf(strings[index], "-Dwrapper.jvmid=%d", (wrapperData->jvmRestarts + 1));
    }
    index++;


    /* Store the main class */
    if (strings) {
        prop = getStringProperty(properties, "wrapper.java.mainclass", "Main");
        strings[index] = malloc(sizeof(char) * (strlen(prop) + 1));
        if (!strings[index]) {
            outOfMemory("WBJCAI", 42);
            return -1;
        }
        sprintf(strings[index], "%s", prop);
    }
    index++;

    /* Store any application parameters */
    if ((index = wrapperBuildJavaCommandArrayAppParameters(strings, addQuotes, index)) < 0) {
        return -1;
    }

    return index;
}

/**
 * command is a pointer to a pointer of an array of character strings.
 * length is the number of strings in the above array.
 *
 * @return TRUE if there were any problems.
 */
int wrapperBuildJavaCommandArray(char ***stringsPtr, int *length, int addQuotes, const char *classpath) {
    int reqLen;

    /* Reset the flag stating that the JVM is a debug JVM. */
    wrapperData->debugJVM = FALSE;
    wrapperData->debugJVMTimeoutNotified = FALSE;
    
    /* Find out how long the array needs to be first. */
    reqLen = wrapperBuildJavaCommandArrayInner(NULL, addQuotes, classpath);
    if (reqLen < 0) {
        return TRUE;
    }
    *length = reqLen;

    /* Allocate the correct amount of memory */
    *stringsPtr = malloc(sizeof(char *) * (*length));
    if (!stringsPtr) {
        outOfMemory("WBJCA", 1);
        return TRUE;
    }

    /* Now actually fill in the strings */
    reqLen = wrapperBuildJavaCommandArrayInner(*stringsPtr, addQuotes, classpath);
    if (reqLen < 0) {
        return TRUE;
    }

    if (wrapperData->debugJVM) {
        if ((wrapperData->startupTimeout > 0) || (wrapperData->pingTimeout > 0) ||
            (wrapperData->shutdownTimeout > 0) || (wrapperData->jvmExitTimeout > 0)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "------------------------------------------------------------------------");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "The JVM is being launched with a debugger enabled and could possibly be");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "suspended.  To avoid unwanted shutdowns, timeouts will be disabled,");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "removing the ability to detect and restart frozen JVMs.");
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "------------------------------------------------------------------------");
        }
    }

    return FALSE;
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
 * Called when the Wrapper detects that the JVM process has exited.
 *  Contains code common to all platforms.
 */
void wrapperJVMProcessExited(int useLoggerQueue, TICKS nowTicks, int exitCode) {
    int setState = TRUE;

    if (exitCode == 0) {
        /* The JVM exit code was 0, so leave any current exit code as is. */
        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            "JVM process exited with a code of %d, leaving the wrapper exit code set to %d.",
            exitCode, wrapperData->exitCode);

    } else if (wrapperData->exitCode == 0) {
        /* Update the wrapper exitCode. */
        wrapperData->exitCode = exitCode;
        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            "JVM process exited with a code of %d, setting the wrapper exit code to %d.",
            exitCode, wrapperData->exitCode);

    } else {
        /* The wrapper exit code was already non-zero, so leave it as is. */
        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            "JVM process exited with a code of %d, however the wrapper exit code was already %d.",
            exitCode, wrapperData->exitCode);
    }

    switch(wrapperData->jState) {
    case WRAPPER_JSTATE_DOWN_CLEAN:
    case WRAPPER_JSTATE_DOWN_CHECK:
        /* Shouldn't be called in this state.  But just in case. */
        if (wrapperData->isDebugging) {
            log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "JVM already down.");
        }
        setState = FALSE;
        break;

    case WRAPPER_JSTATE_LAUNCH_DELAY:
    case WRAPPER_JSTATE_RESTART:
    case WRAPPER_JSTATE_LAUNCH:
        /* We got a message that the JVM process died when we already thought is was down.
         *  Most likely this was caused by a SIGCHLD signal.  We are already in the expected
         *  state so go ahead and ignore it.  Do NOT go back to DOWN or the restart flag
         *  and all restart counts will have be lost */
        if (wrapperData->isDebugging) {
            log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "Received a message that the JVM is down when in the LAUNCH(DELAY) state.");
        }
        setState = FALSE;
        break;

    case WRAPPER_JSTATE_LAUNCHING:
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_AUTOMATIC;
        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            "JVM exited while loading the application.");
        break;

    case WRAPPER_JSTATE_LAUNCHED:
        /* Shouldn't be called in this state, but just in case. */
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_AUTOMATIC;
        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            "JVM exited before starting the application.");
        break;

    case WRAPPER_JSTATE_STARTING:
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_AUTOMATIC;
        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            "JVM exited while starting the application.");
        break;

    case WRAPPER_JSTATE_STARTED:
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_AUTOMATIC;
        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            "JVM exited unexpectedly.");
        break;

    case WRAPPER_JSTATE_STOP:
    case WRAPPER_JSTATE_STOPPING:
        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "JVM exited unexpectedly while stopping the application.");
        break;

    case WRAPPER_JSTATE_STOPPED:
        if (wrapperData->isDebugging) {
            log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                "JVM exited normally.");
        }
        break;

    case WRAPPER_JSTATE_KILLING:
    case WRAPPER_JSTATE_KILL:
        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_INFO,
            "JVM exited on its own while waiting to kill the application.");
        break;

    default:
        log_printf_queue(useLoggerQueue, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "Unexpected jState=%d in wrapperJVMProcessExited.", wrapperData->jState);
        break;
    }

    /* Only set the state to DOWN_CHECK if we are not already in a state which reflects this. */
    if (setState) {
        if (wrapperData->jvmCleanupTimeout > 0) {
            wrapperSetJavaState(useLoggerQueue, WRAPPER_JSTATE_DOWN_CHECK, wrapperGetTicks(), wrapperData->jvmCleanupTimeout);
        } else {
            wrapperSetJavaState(useLoggerQueue, WRAPPER_JSTATE_DOWN_CHECK, wrapperGetTicks(), -1);
        }
    }

    wrapperProtocolClose();

    /* Remove java pid file if it was registered and created by this process. */
    if (wrapperData->javaPidFilename) {
        unlink(wrapperData->javaPidFilename);
    }
}

void wrapperBuildKey() {
    int i;
    size_t kcNum;
    size_t num;
    static int seeded = FALSE;

    /* Seed the randomizer */
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = TRUE;
    }

    /* Start by generating a key */
    num = strlen(keyChars);

    for (i = 0; i < 16; i++) {
        /* The way rand works, this will sometimes equal num, which is too big.
         *  This is rare so just round those cases down. */
        
        /* Some platforms use very large RAND_MAX values that cause overflow problems in our math */
        if (RAND_MAX > 0x10000) {
            kcNum = (size_t)((rand() >> 8) * num / (RAND_MAX >> 8));
        } else {
            kcNum = (size_t)(rand() * num / RAND_MAX);
        }

        if (kcNum >= num) {
            kcNum = num - 1;
        }
        
        wrapperData->key[i] = keyChars[kcNum];
    }
    wrapperData->key[16] = '\0';
    
    /*
    printf("  Key=%s Len=%lu\n", wrapperData->key, strlen(wrapperData->key));
    */
}

/**
 * Updates a string value by making a copy of the original.  Any old value is
 *  first freed.
 */
void updateStringValue(char **ptr, const char *value) {
    if (*ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }

    if (value != NULL) {
        *ptr = malloc(sizeof(char) * (strlen(value) + 1));
        if (!ptr) {
            outOfMemory("USV", 1);
            /* TODO: This is pretty bad.  Not sure how to recover... */
        } else {
            strcpy(*ptr, value);
        }
    }
}

#ifdef WIN32

/* The ABOVE and BELOW normal priority class constants are not defined in MFVC 6.0 headers. */
#ifndef ABOVE_NORMAL_PRIORITY_CLASS
#define ABOVE_NORMAL_PRIORITY_CLASS 0x00008000
#endif
#ifndef BELOW_NORMAL_PRIORITY_CLASS
#define BELOW_NORMAL_PRIORITY_CLASS 0x00004000
#endif

/**
 * Return FALSE if successful, TRUE if there were problems.
 */
int wrapperBuildNTServiceInfo() {
    char *work;
    const char *priority;
    size_t len, valLen;
    int i;
    char **propertyNames;
    char **propertyValues;
    long unsigned int *propertyIndices;

    if (!wrapperData->configured) {
        /* Load the service load order group */
        updateStringValue(&wrapperData->ntServiceLoadOrderGroup, getStringProperty(properties, "wrapper.ntservice.load_order_group", ""));

        if (getStringProperties(properties, "wrapper.ntservice.dependency.", "", wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
            /* Failed */
            return TRUE;
        }
        
        /* Build the dependency list.  Decide how large the list needs to be */
        len = 0;
        i = 0;
        while (propertyNames[i]) {
            valLen = strlen(propertyValues[i]);
            if (valLen > 0) {
                len += valLen + 1;
            }
            i++;
        }
        /* List must end with a double '\0'.  If the list is not empty then it will end with 3.  But that is fine. */
        len += 2;

        /* Actually build the buffer */
        if (wrapperData->ntServiceDependencies) {
            /** This is a reload, so free up the old data. */
            free(wrapperData->ntServiceDependencies);
            wrapperData->ntServiceDependencies = NULL;
        }
        work = wrapperData->ntServiceDependencies = malloc(sizeof(char) * len);
        if (!work) {
            outOfMemory("WBNTSI", 1);
            return TRUE;
        }
        
        /* Now actually build up the list. Each value is separated with a '\0'. */
        i = 0;
        while (propertyNames[i]) {
            valLen = strlen(propertyValues[i]);
            if (valLen > 0) {
                strcpy(work, propertyValues[i]);
                work += valLen + 1;
            }
            i++;
        }
        /* Add two more nulls to the end of the list. */
        work[0] = '\0';
        work[1] = '\0';

        /* Memory allocated in work is stored in wrapperData.  The memory should not be released here. */
        work = NULL;

        freeStringProperties(propertyNames, propertyValues, propertyIndices);

        /* Set the service start type */
        if (strcmpIgnoreCase(getStringProperty(properties, "wrapper.ntservice.starttype", "DEMAND_START"), "AUTO_START") == 0) {
            wrapperData->ntServiceStartType = SERVICE_AUTO_START;
        } else {
            wrapperData->ntServiceStartType = SERVICE_DEMAND_START;
        }

        /* Set the service priority class */
        priority = getStringProperty(properties, "wrapper.ntservice.process_priority", "NORMAL");
        if ((strcmpIgnoreCase(priority, "LOW") == 0) || (strcmpIgnoreCase(priority, "IDLE") == 0)) {
            wrapperData->ntServicePriorityClass = IDLE_PRIORITY_CLASS;
        } else if (strcmpIgnoreCase(priority, "HIGH") == 0) {
            wrapperData->ntServicePriorityClass = HIGH_PRIORITY_CLASS;
        } else if (strcmpIgnoreCase(priority, "REALTIME") == 0) {
            wrapperData->ntServicePriorityClass = REALTIME_PRIORITY_CLASS;
        } else if (strcmpIgnoreCase(priority, "ABOVE_NORMAL") == 0) {
            wrapperData->ntServicePriorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
        } else if (strcmpIgnoreCase(priority, "BELOW_NORMAL") == 0) {
            wrapperData->ntServicePriorityClass = BELOW_NORMAL_PRIORITY_CLASS;
        } else {
            wrapperData->ntServicePriorityClass = NORMAL_PRIORITY_CLASS;
        }

        /* Account name */
        updateStringValue(&wrapperData->ntServiceAccount, getStringProperty(properties, "wrapper.ntservice.account", NULL));
        if (wrapperData->ntServiceAccount && (strlen(wrapperData->ntServiceAccount) <= 0)) {
            wrapperData->ntServiceAccount = NULL;
        }

        /* Acount password */
        wrapperData->ntServicePasswordPrompt = getBooleanProperty(properties, "wrapper.ntservice.password.prompt", FALSE);
        wrapperData->ntServicePasswordPromptMask = getBooleanProperty(properties, "wrapper.ntservice.password.prompt.mask", TRUE);
        updateStringValue(&wrapperData->ntServicePassword, getStringProperty(properties, "wrapper.ntservice.password", NULL));
        if (wrapperData->ntServicePassword && (strlen(wrapperData->ntServicePassword) <= 0)) {
            wrapperData->ntServicePassword = NULL;
        }
        if (!wrapperData->ntServiceAccount) {
            /* If there is not account name, then the password must not be set. */
            wrapperData->ntServicePassword = NULL;
        }

        /* Interactive */
        wrapperData->ntServiceInteractive = getBooleanProperty(properties, "wrapper.ntservice.interactive", FALSE);
        /* The interactive flag can not be set if an account is also set. */
        if (wrapperData->ntServiceAccount && wrapperData->ntServiceInteractive) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "Ignoring the wrapper.ntservice.interactive property because it can not be set when wrapper.ntservice.account is also set.");
            wrapperData->ntServiceInteractive = FALSE;
        }

        /* Pausable */
        wrapperData->ntServicePausable = getBooleanProperty(properties, "wrapper.ntservice.pausable", FALSE);
        wrapperData->ntServicePausableStopJVM = getBooleanProperty(properties, "wrapper.ntservice.pausable.stop_jvm", TRUE);

        /* Display a Console Window. */
        wrapperData->ntAllocConsole = getBooleanProperty(properties, "wrapper.ntservice.console", FALSE);
        /* Set the default hide wrapper console flag to the inverse of the alloc console flag. */
        wrapperData->ntHideWrapperConsole = !wrapperData->ntAllocConsole;

        /* Hide the JVM Console Window. */
        wrapperData->ntHideJVMConsole = getBooleanProperty(properties, "wrapper.ntservice.hide_console", TRUE);
        
        /* Make sure that a console is always generated to support thread dumps */
        wrapperData->generateConsole = getBooleanProperty(properties, "wrapper.ntservice.generate_console", TRUE);
    }

    /* Set the single invocation flag. */
    wrapperData->isSingleInvocation = getBooleanProperty(properties, "wrapper.single_invocation", FALSE);
    
    wrapperData->threadDumpControlCode = getIntProperty(properties, "wrapper.thread_dump_control_code", 255);
    if (wrapperData->threadDumpControlCode <= 0) {
        /* Disabled */
    } else if ((wrapperData->threadDumpControlCode < 128) || (wrapperData->threadDumpControlCode > 255)) {
        wrapperData->threadDumpControlCode = 255;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "Ignoring the wrapper.thread_dump_control_code property because it must be in the range 128-255 or 0.");
    }

    return FALSE;
}
#endif

#ifndef WIN32 /* UNIX */
int getSignalMode(const char *modeName, int defaultMode) {
    if (!modeName) {
        return defaultMode;
    }
    
    if (strcmpIgnoreCase(modeName, "IGNORE") == 0) {
        return WRAPPER_SIGNAL_MODE_IGNORE;
    } else if (strcmpIgnoreCase(modeName, "RESTART") == 0) {
        return WRAPPER_SIGNAL_MODE_RESTART;
    } else if (strcmpIgnoreCase(modeName, "SHUTDOWN") == 0) {
        return WRAPPER_SIGNAL_MODE_SHUTDOWN;
    } else if (strcmpIgnoreCase(modeName, "FORWARD") == 0) {
        return WRAPPER_SIGNAL_MODE_FORWARD;
    } else {
        return defaultMode;
    }
}

/**
 * Return FALSE if successful, TRUE if there were problems.
 */
int wrapperBuildUnixDaemonInfo() {
    if (!wrapperData->configured) {
        /** Get the daemonize flag. */
        wrapperData->daemonize = getBooleanProperty(properties, "wrapper.daemonize", FALSE);
        /** Configure the HUP signal handler. */
        wrapperData->signalHUPMode = getSignalMode(getStringProperty(properties, "wrapper.signal.mode.hup", NULL), WRAPPER_SIGNAL_MODE_FORWARD);

        /** Configure the USR1 signal handler. */
        wrapperData->signalUSR1Mode = getSignalMode(getStringProperty(properties, "wrapper.signal.mode.usr1", NULL), WRAPPER_SIGNAL_MODE_FORWARD);

        /** Configure the USR2 signal handler. */
        wrapperData->signalUSR2Mode = getSignalMode(getStringProperty(properties, "wrapper.signal.mode.usr2", NULL), WRAPPER_SIGNAL_MODE_FORWARD);
    }

    return FALSE;
}
#endif

int validateTimeout(const char* propertyName, int value) {
    if (value <= 0) {
        return 0;
    } else if (value > WRAPPER_TIMEOUT_MAX) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "%s must be in the range 0 to %d days (%d seconds).  Changing to %d.",
            propertyName, WRAPPER_TIMEOUT_MAX / 86400, WRAPPER_TIMEOUT_MAX, WRAPPER_TIMEOUT_MAX);
        return WRAPPER_TIMEOUT_MAX;
    } else {
        return value;
    }
}

void wrapperLoadHostName() {
    char hostName[80];

    if (gethostname(hostName, sizeof(hostName))) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, "Unable to obtain host name. %s",
            getLastErrorText());
    } else {
        wrapperData->hostName = malloc(strlen(hostName) + 1);
        if (!wrapperData->hostName) {
            outOfMemory("LHN", 1);
            return;
        }
        sprintf(wrapperData->hostName, "%s", hostName);
    }
}

int getOutputFilterActionForName(const char *actionName) {
    if (strcmpIgnoreCase(actionName, "RESTART") == 0) {
        return FILTER_ACTION_RESTART;
    } else if (strcmpIgnoreCase(actionName, "SHUTDOWN") == 0) {
        return FILTER_ACTION_SHUTDOWN;
    } else if (strcmpIgnoreCase(actionName, "DUMP") == 0) {
        return FILTER_ACTION_DUMP;
    } else {
        return FILTER_ACTION_NONE;
    }
}

int loadConfigurationTriggers() {
    const char *prop;
    char propName[256];
    int i;
    char **propertyNames;
    char **propertyValues;
    long unsigned int *propertyIndices;

    /* To support reloading, we need to free up any previously loaded filters. */
    if (wrapperData->outputFilterCount > 0) {
        for (i = 0; i < wrapperData->outputFilterCount; i++) {
            free(wrapperData->outputFilters[i]);
            wrapperData->outputFilters[i] = NULL;
        }
        free(wrapperData->outputFilters);
        wrapperData->outputFilters = NULL;
        free(wrapperData->outputFilterActions);
        wrapperData->outputFilterActions = NULL;
    }

    wrapperData->outputFilterCount = 0;
    if (getStringProperties(properties, "wrapper.filter.trigger.", "", wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
        /* Failed */
        return -1;
    }
    i = 0;
    while (propertyNames[i]) {
        wrapperData->outputFilterCount++;
        i++;
    }
#if defined(MACOSX)
    wrapperData->outputFilterCount++;
    i++;
#endif

    /* Now that a count is known, allocate memory to hold the filters and actions and load them in. */
    if (wrapperData->outputFilterCount > 0) {
        wrapperData->outputFilters = malloc(sizeof(char *) * wrapperData->outputFilterCount);
        if (!wrapperData->outputFilters) {
            outOfMemory("LC", 1);
            return -1;
        }
        wrapperData->outputFilterActions = malloc(sizeof(int) * wrapperData->outputFilterCount);
        if (!wrapperData->outputFilterActions) {
            outOfMemory("LC", 2);
            return -1;
        }

        i = 0;
        while (propertyNames[i]) {
            prop = propertyValues[i];

            wrapperData->outputFilters[i] = malloc(sizeof(char) * (strlen(prop) + 1));
            if (!wrapperData->outputFilters[i]) {
                outOfMemory("LC", 3);
                return -1;
            }
            strcpy(wrapperData->outputFilters[i], prop);

            /* Get the action */
            sprintf(propName, "wrapper.filter.action.%lu", propertyIndices[i]);
            prop = getStringProperty(properties, propName, "RESTART");
            wrapperData->outputFilterActions[i] = getOutputFilterActionForName(prop);

#ifdef _DEBUG
            printf("filter #%lu, action=%d, filter='%s'\n", propertyIndices[i], wrapperData->outputFilterActions[i], wrapperData->outputFilters[i]);
#endif
            i++;
        }

#if defined(MACOSX)
        wrapperData->outputFilters[i] = malloc(sizeof(char) * strlen(FILTER_TRIGGER_ADVICE_NIL_SERVER));
        if (!wrapperData->outputFilters[i]) {
            outOfMemory("LC", 4);
            return -1;
        }
        strcpy(wrapperData->outputFilters[i], FILTER_TRIGGER_ADVICE_NIL_SERVER);
        wrapperData->outputFilterActions[i] = FILTER_ACTION_ADVICE_NIL_SERVER;
        i++;
#endif
    }
    freeStringProperties(propertyNames, propertyValues, propertyIndices);
    
    return 0;
}

/**
 * Return FALSE if successful, TRUE if there were problems.
 */
int loadConfiguration() {
    const char* logfilePath;
    int logfileRollMode;
    char propName[256];
    const char* val;
    int startupDelay;

    /* Load log file */

    logfilePath = getFileSafeStringProperty(properties, "wrapper.logfile", "wrapper.log");
    setLogfilePath(logfilePath);
    
    /* Decide whether the classpath should be passed via the environment. */
    wrapperData->environmentClasspath = getBooleanProperty(properties, "wrapper.java.classpath.use_environment", FALSE);

    /* Decide how sequence gaps should be handled before any other properties are loaded. */
    wrapperData->ignoreSequenceGaps = getBooleanProperty(properties, "wrapper.ignore_sequence_gaps", FALSE);

    logfileRollMode = getLogfileRollModeForName(getStringProperty(properties, "wrapper.logfile.rollmode", "SIZE"));
    if (logfileRollMode == ROLL_MODE_UNKNOWN) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.logfile.rollmode invalid.  Disabling log file rolling.");
        logfileRollMode = ROLL_MODE_NONE;
    } else if (logfileRollMode == ROLL_MODE_DATE) {
        if (!strstr(logfilePath, ROLL_MODE_DATE_TOKEN)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "wrapper.logfile must contain \"%s\" for a roll mode of DATE.  Disabling log file rolling.",
                ROLL_MODE_DATE_TOKEN);
            logfileRollMode = ROLL_MODE_NONE;
        }
    }
    setLogfileRollMode(logfileRollMode);

    /* Load log file format */
    setLogfileFormat(getStringProperty(properties, "wrapper.logfile.format", "LPTM"));

    /* Load log file log level */
    setLogfileLevel(getStringProperty(properties, "wrapper.logfile.loglevel", "INFO"));

    /* Load max log filesize log level */
    setLogfileMaxFileSize(getStringProperty(properties, "wrapper.logfile.maxsize", "0"));

    /* Load log files level */
    setLogfileMaxLogFiles(getIntProperty(properties, "wrapper.logfile.maxfiles", 0));

    /* Load log file purge pattern */
    setLogfilePurgePattern(getFileSafeStringProperty(properties, "wrapper.logfile.purge.pattern", ""));

    /* Load log file purge sort */
    setLogfilePurgeSortMode(wrapperFileGetSortMode(getStringProperty(properties, "wrapper.logfile.purge.sort", "TIMES")));

    /* Get the memory output status. */
    wrapperData->logfileInactivityTimeout = __max(getIntProperty(properties, "wrapper.logfile.inactivity.timeout", 1), 0);
    setLogfileAutoClose(wrapperData->logfileInactivityTimeout <= 0);

    /* Load console format */
    setConsoleLogFormat(getStringProperty(properties, "wrapper.console.format", "PM"));

    /* Load console log level */
    setConsoleLogLevel(getStringProperty(properties, "wrapper.console.loglevel", "INFO"));

    /* Load the console flush flag. */
    setConsoleFlush(getBooleanProperty(properties, "wrapper.console.flush", FALSE));

    /* Load syslog log level */
    setSyslogLevel(getStringProperty(properties, "wrapper.syslog.loglevel", "NONE"));

#ifndef WIN32
    /* Load syslog facility */
    setSyslogFacility(getStringProperty(properties, "wrapper.syslog.facility", "USER"));
#endif

    /* Load syslog event source name */
    setSyslogEventSourceName(getStringProperty(properties, "wrapper.syslog.ident", getStringProperty(properties, "wrapper.name", getStringProperty(properties, "wrapper.ntservice.name", "wrapper"))));

    /* Register the syslog message file if syslog is enabled */
    if (getSyslogLevelInt() < LEVEL_NONE) {
        registerSyslogMessageFile();
    }

    /* To make configuration reloading work correctly with changes to the log file,
     *  it needs to be closed here. */
    closeLogfile();

    /* Initialize some values not loaded */
    wrapperData->exitCode = 0;

    /* Get the port. The int will wrap within the 0-65535 valid range, so no need to test the value. */
    wrapperData->port = getIntProperty(properties, "wrapper.port", 0);
    wrapperData->portMin = getIntProperty(properties, "wrapper.port.min", 32000);
    if ((wrapperData->portMin < 1) || (wrapperData->portMin > 65535)) {
        wrapperData->portMin = 32000;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.port.min must be in the range 1-65535.  Changing to %d.", wrapperData->portMin);
    }
    wrapperData->portMax = getIntProperty(properties, "wrapper.port.max", 32999);
    if ((wrapperData->portMax < 1) || (wrapperData->portMax > 65535)) {
        wrapperData->portMax = __min(wrapperData->portMin + 999, 65535);
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.port.min must be in the range 1-65535.  Changing to %d.", wrapperData->portMax);
    } else if (wrapperData->portMax < wrapperData->portMin) {
        wrapperData->portMax = __min(wrapperData->portMin + 999, 65535);
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.port.max must be greater than or equal to wrapper.port.min.  Changing to %d.", wrapperData->portMax);
    }
    
    /* Get the port for the JVM side of the socket. */
    wrapperData->jvmPort = getIntProperty(properties, "wrapper.jvm.port", 0);
    if (wrapperData->jvmPort > 0) {
        if (wrapperData->jvmPort == wrapperData->port) {
            wrapperData->jvmPort = 0;
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "wrapper.jvm.port must not equal wrapper.port.  Changing to the default.");
        }
    }
    wrapperData->jvmPortMin = getIntProperty(properties, "wrapper.jvm.port.min", 31000);
    if ((wrapperData->jvmPortMin < 1) || (wrapperData->jvmPortMin > 65535)) {
        wrapperData->jvmPortMin = 31000;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.jvm.port.min must be in the range 1-65535.  Changing to %d.", wrapperData->jvmPortMin);
    }
    wrapperData->jvmPortMax = getIntProperty(properties, "wrapper.jvm.port.max", 31999);
    if ((wrapperData->jvmPortMax < 1) || (wrapperData->jvmPortMax > 65535)) {
        wrapperData->jvmPortMax = __min(wrapperData->jvmPortMin + 999, 65535);
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.jvm.port.min must be in the range 1-65535.  Changing to %d.", wrapperData->jvmPortMax);
    } else if (wrapperData->jvmPortMax < wrapperData->jvmPortMin) {
        wrapperData->jvmPortMax = __min(wrapperData->jvmPortMin + 999, 65535);
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.jvm.port.max must be greater than or equal to wrapper.jvm.port.min.  Changing to %d.", wrapperData->jvmPortMax);
    }

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

    /* Get the wrapper command log level. */
    wrapperData->commandLogLevel = getLogLevelForName(
        getStringProperty(properties, "wrapper.java.command.loglevel", "DEBUG"));
    /* Get the adviser status */
    wrapperData->isAdviserEnabled = getBooleanProperty(properties, "wrapper.adviser", TRUE);
    /* The adviser is always enabled if debug is enabled. */
    if (wrapperData->isDebugging) {
        wrapperData->isAdviserEnabled = TRUE;
    }

    /* Get the use system time flag. */
    if (!wrapperData->configured) {
        wrapperData->useSystemTime = getBooleanProperty(properties, "wrapper.use_system_time", FALSE);
    }
    /* Get the timer thresholds. Properties are in seconds, but internally we use ticks. */
    wrapperData->timerFastThreshold = getIntProperty(properties, "wrapper.timer_fast_threshold", WRAPPER_TIMER_FAST_THRESHOLD * WRAPPER_TICK_MS / 1000) * 1000 / WRAPPER_TICK_MS;
    wrapperData->timerSlowThreshold = getIntProperty(properties, "wrapper.timer_slow_threshold", WRAPPER_TIMER_SLOW_THRESHOLD * WRAPPER_TICK_MS / 1000) * 1000 / WRAPPER_TICK_MS;

    /* Load the name of the native library to be loaded. */
    wrapperData->nativeLibrary = getStringProperty(properties, "wrapper.native_library", "wrapper");

    /* Get the append PATH to library path flag. */
    wrapperData->libraryPathAppendPath = getBooleanProperty(properties, "wrapper.java.library.path.append_system_path", FALSE);

    /* Get the state output status. */
    wrapperData->isStateOutputEnabled = getBooleanProperty(properties, "wrapper.state_output", FALSE);

    /* Get the tick output status. */
    wrapperData->isTickOutputEnabled = getBooleanProperty(properties, "wrapper.tick_output", FALSE);

    /* Get the loop debug output status. */
    wrapperData->isLoopOutputEnabled = getBooleanProperty(properties, "wrapper.loop_output", FALSE);

    /* Get the sleep debug output status. */
    wrapperData->isSleepOutputEnabled = getBooleanProperty(properties, "wrapper.sleep_output", FALSE);

    /* Get the memory output status. */
    wrapperData->isMemoryOutputEnabled = getBooleanProperty(properties, "wrapper.memory_output", FALSE);
    wrapperData->memoryOutputInterval = getIntProperty(properties, "wrapper.memory_output.interval", 1);

    /* Get the cpu output status. */
    wrapperData->isCPUOutputEnabled = getBooleanProperty(properties, "wrapper.cpu_output", FALSE);
    wrapperData->cpuOutputInterval = getIntProperty(properties, "wrapper.cpu_output.interval", 1);

    /* Get the shutdown hook status */
    wrapperData->isShutdownHookDisabled = getBooleanProperty(properties, "wrapper.disable_shutdown_hook", FALSE);

    /* Get the startup delay. */
    startupDelay = getIntProperty(properties, "wrapper.startup.delay", 0);
    wrapperData->startupDelayConsole = getIntProperty(properties, "wrapper.startup.delay.console", startupDelay);
    if (wrapperData->startupDelayConsole < 0) {
        wrapperData->startupDelayConsole = 0;
    }
    wrapperData->startupDelayService = getIntProperty(properties, "wrapper.startup.delay.service", startupDelay);
    if (wrapperData->startupDelayService < 0) {
        wrapperData->startupDelayService = 0;
    }

    /* Get the restart delay. */
    wrapperData->restartDelay = getIntProperty(properties, "wrapper.restart.delay", 5);
    if (wrapperData->restartDelay < 0) {
        wrapperData->restartDelay = 0;
    }

    /* Get the flag which decides whether or not configuration should be reloaded on JVM restart. */
    wrapperData->restartReloadConf = getBooleanProperty(properties, "wrapper.restart.reload_configuration", FALSE);

    /* Get the disable restart flag */
    wrapperData->isRestartDisabled = getBooleanProperty(properties, "wrapper.disable_restarts", FALSE);
    wrapperData->isAutoRestartDisabled = getBooleanProperty(properties, "wrapper.disable_restarts.automatic", wrapperData->isRestartDisabled);

    /* Get the timeout settings */
    wrapperData->cpuTimeout = getIntProperty(properties, "wrapper.cpu.timeout", 10);
    wrapperData->startupTimeout = getIntProperty(properties, "wrapper.startup.timeout", 30);
    wrapperData->pingTimeout = getIntProperty(properties, "wrapper.ping.timeout", 30);
    wrapperData->pingInterval = getIntProperty(properties, "wrapper.ping.interval", 5);
    wrapperData->pingIntervalLogged = getIntProperty(properties, "wrapper.ping.interval.logged", 1);
    wrapperData->shutdownTimeout = getIntProperty(properties, "wrapper.shutdown.timeout", 30);
    wrapperData->jvmExitTimeout = getIntProperty(properties, "wrapper.jvm_exit.timeout", 15);
    wrapperData->jvmCleanupTimeout = getIntProperty(properties, "wrapper.jvm_cleanup.timeout", 10);

    wrapperData->cpuTimeout = validateTimeout("wrapper.cpu.timeout", wrapperData->cpuTimeout);
    wrapperData->startupTimeout = validateTimeout("wrapper.startup.timeout", wrapperData->startupTimeout);
    wrapperData->pingTimeout = validateTimeout("wrapper.ping.timeout", wrapperData->pingTimeout);
    wrapperData->shutdownTimeout = validateTimeout("wrapper.shutdown.timeout", wrapperData->shutdownTimeout);
    wrapperData->jvmExitTimeout = validateTimeout("wrapper.jvm_exit.timeout", wrapperData->jvmExitTimeout);
    wrapperData->jvmCleanupTimeout = validateTimeout("wrapper.jvm_cleanup.timeout", wrapperData->jvmCleanupTimeout);

    if (wrapperData->pingInterval < 1) {
        wrapperData->pingInterval = 1;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.ping.interval must be at least 1 second.  Changing to 1.");
    } else if (wrapperData->pingInterval > 3600) {
        wrapperData->pingInterval = 3600;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.ping.interval must be less than or equal to 1 hour (3600 seconds).  Changing to 3600.");
    }
    if (wrapperData->pingIntervalLogged < 1) {
        wrapperData->pingIntervalLogged = 1;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.ping.interval.logged must be at least 1 second.  Changing to 1.");
    } else if (wrapperData->pingIntervalLogged > 86400) {
        wrapperData->pingIntervalLogged = 86400;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.ping.interval.logged must be less than or equal to 1 day (86400 seconds).  Changing to 86400.");
    }

    if ((wrapperData->pingTimeout > 0) && (wrapperData->pingTimeout < wrapperData->pingInterval + 5)) {
        wrapperData->pingTimeout = wrapperData->pingInterval + 5;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            "wrapper.ping.timeout must be at least 5 seconds longer than wrapper.ping.interval.  Changing to %d.", wrapperData->pingTimeout);
    }
    if (wrapperData->cpuTimeout > 0) {
        /* Make sure that the timeouts are all longer than the cpu timeout. */
        if ((wrapperData->startupTimeout > 0) && (wrapperData->startupTimeout < wrapperData->cpuTimeout)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "CPU timeout detection may not operate correctly during startup because wrapper.cpu.timeout is not smaller than wrapper.startup.timeout.");
        }
        if ((wrapperData->pingTimeout > 0) && (wrapperData->pingTimeout < wrapperData->cpuTimeout)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "CPU timeout detection may not operate correctly because wrapper.cpu.timeout is not smaller than wrapper.ping.timeout.");
        }
        if ((wrapperData->shutdownTimeout > 0) && (wrapperData->shutdownTimeout < wrapperData->cpuTimeout)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                "CPU timeout detection may not operate correctly during shutdown because wrapper.cpu.timeout is not smaller than wrapper.shutdown.timeout.");
        }
        /* jvmExit timeout can be shorter than the cpu timeout. */
    }

    /* Load properties controlling the number times the JVM can be restarted. */
    wrapperData->maxFailedInvocations = getIntProperty(properties, "wrapper.max_failed_invocations", 5);
    wrapperData->successfulInvocationTime = getIntProperty(properties, "wrapper.successful_invocation_time", 300);
    if (wrapperData->maxFailedInvocations < 1) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            "The value of wrapper.max_failed_invocations must not be smaller than 1.  Changing to 1.");
        wrapperData->maxFailedInvocations = 1;
    }

    /* TRUE if the JVM should be asked to dump its state when it fails to halt on request. */
    wrapperData->requestThreadDumpOnFailedJVMExit = getBooleanProperty(properties, "wrapper.request_thread_dump_on_failed_jvm_exit", FALSE);

    /* Load the output filters. */
    if (loadConfigurationTriggers()) {
        return TRUE;
    }

    /** Get the pid files if any.  May be NULL */
    if (!wrapperData->configured) {
        updateStringValue(&wrapperData->pidFilename, getFileSafeStringProperty(properties, "wrapper.pidfile", NULL));
        correctWindowsPath(wrapperData->pidFilename);
    }
    updateStringValue(&wrapperData->javaPidFilename, getFileSafeStringProperty(properties, "wrapper.java.pidfile", NULL));
    correctWindowsPath(wrapperData->javaPidFilename);

    /** Get the lock file if any.  May be NULL */
    if (!wrapperData->configured) {
        updateStringValue(&wrapperData->lockFilename, getFileSafeStringProperty(properties, "wrapper.lockfile", NULL));
        correctWindowsPath(wrapperData->lockFilename);
    }

    /** Get the java id file.  May be NULL */
    updateStringValue(&wrapperData->javaIdFilename, getFileSafeStringProperty(properties, "wrapper.java.idfile", NULL));
    correctWindowsPath(wrapperData->javaIdFilename);

    /** Get the status files if any.  May be NULL */
    if (!wrapperData->configured) {
        updateStringValue(&wrapperData->statusFilename, getFileSafeStringProperty(properties, "wrapper.statusfile", NULL));
        correctWindowsPath(wrapperData->statusFilename);
    }
    updateStringValue(&wrapperData->javaStatusFilename, getFileSafeStringProperty(properties, "wrapper.java.statusfile", NULL));
    correctWindowsPath(wrapperData->javaStatusFilename);

    /** Get the command file if any. May be NULL */
    updateStringValue(&wrapperData->commandFilename, getFileSafeStringProperty(properties, "wrapper.commandfile", NULL));
    correctWindowsPath(wrapperData->commandFilename);

    /** Get the interval at which the command file will be polled. */
    wrapperData->commandPollInterval = __min(__max(getIntProperty(properties, "wrapper.command.poll_interval", 5), 1), 3600);

    /** Get the anchor file if any.  May be NULL */
    if (!wrapperData->configured) {
        updateStringValue(&wrapperData->anchorFilename, getFileSafeStringProperty(properties, "wrapper.anchorfile", NULL));
        correctWindowsPath(wrapperData->anchorFilename);
    }

    /** Get the interval at which the anchor file will be polled. */
    wrapperData->anchorPollInterval = __min(__max(getIntProperty(properties, "wrapper.anchor.poll_interval", 5), 1), 3600);

    /** Get the umask value for the various files. */
    wrapperData->umask = getIntProperty(properties, "wrapper.umask", 0022);
    wrapperData->javaUmask = getIntProperty(properties, "wrapper.java.umask", wrapperData->umask);
    wrapperData->pidFileUmask = getIntProperty(properties, "wrapper.pidfile.umask", wrapperData->umask);
    wrapperData->lockFileUmask = getIntProperty(properties, "wrapper.lockfile.umask", wrapperData->umask);
    wrapperData->javaPidFileUmask = getIntProperty(properties, "wrapper.java.pidfile.umask", wrapperData->umask);
    wrapperData->javaIdFileUmask = getIntProperty(properties, "wrapper.java.idfile.umask", wrapperData->umask);
    wrapperData->statusFileUmask = getIntProperty(properties, "wrapper.statusfile.umask", wrapperData->umask);
    wrapperData->javaStatusFileUmask = getIntProperty(properties, "wrapper.java.statusfile.umask", wrapperData->umask);
    wrapperData->anchorFileUmask = getIntProperty(properties, "wrapper.anchorfile.umask", wrapperData->umask);
    setLogfileUmask(getIntProperty(properties, "wrapper.logfile.umask", wrapperData->umask));

    /** Flag controlling whether or not system signals should be ignored. */
    val = getStringProperty(properties, "wrapper.ignore_signals", "FALSE");
    if ((strcmpIgnoreCase(val, "TRUE") == 0) || (strcmpIgnoreCase(val, "BOTH") == 0)) {
        wrapperData->ignoreSignals = WRAPPER_IGNORE_SIGNALS_WRAPPER + WRAPPER_IGNORE_SIGNALS_JAVA;
    } else if (strcmpIgnoreCase(val, "WRAPPER") == 0) {
        wrapperData->ignoreSignals = WRAPPER_IGNORE_SIGNALS_WRAPPER;
    } else if (strcmpIgnoreCase(val, "JAVA") == 0) {
        wrapperData->ignoreSignals = WRAPPER_IGNORE_SIGNALS_JAVA;
    } else {
        wrapperData->ignoreSignals = 0;
    }

    /* Obtain the Console Title. */
    sprintf(propName, "wrapper.console.title.%s", wrapperOS);
    updateStringValue(&wrapperData->consoleTitle, getStringProperty(properties, propName, getStringProperty(properties, "wrapper.console.title", NULL)));

    /* Load the service name (Used to be windows specific so use those properties if set.) */
    updateStringValue(&wrapperData->serviceName, getStringProperty(properties, "wrapper.name", getStringProperty(properties, "wrapper.ntservice.name", "wrapper")));

    /* Load the service display name (Used to be windows specific so use those properties if set.) */
    updateStringValue(&wrapperData->serviceDisplayName, getStringProperty(properties, "wrapper.displayname", getStringProperty(properties, "wrapper.ntservice.displayname", wrapperData->serviceName)));

    /* Load the service description, default to display name (Used to be windows specific so use those properties if set.) */
    updateStringValue(&wrapperData->serviceDescription, getStringProperty(properties, "wrapper.description", getStringProperty(properties, "wrapper.ntservice.description", wrapperData->serviceDisplayName)));

#ifdef WIN32
    wrapperData->ignoreUserLogoffs = getBooleanProperty(properties, "wrapper.ignore_user_logoffs", FALSE);

    /* Configure the NT service information */
    if (wrapperBuildNTServiceInfo()) {
        return TRUE;
    }

    if (wrapperData->requestThreadDumpOnFailedJVMExit || wrapperData->commandFilename || wrapperData->generateConsole) {
        if (!wrapperData->ntAllocConsole) {
            /* We need to allocate a console in order for the thread dumps to work
             *  when running as a service.  But the user did not request that a
             *  console be visible so we want to hide it. */
            wrapperData->ntAllocConsole = TRUE;
            wrapperData->ntHideWrapperConsole = TRUE;
        }
    }

#else /* UNIX */
    /* Configure the Unix daemon information */
    if (wrapperBuildUnixDaemonInfo()) {
        return TRUE;
    }

#endif

    wrapperData->configured = TRUE;

    return FALSE;
}

/**
 * Calculates a tick count using the system time.
 *
 * We normally need 64 bits to do this calculation.  Play some games to get
 *  the correct values with 32 bit variables.
 */
TICKS wrapperGetSystemTicks() {
    struct timeb timeBuffer;
    DWORD high, low;
    TICKS sum;
#ifdef _DEBUG
    TICKS assertSum;
#endif

    wrapperGetCurrentTime(&timeBuffer);

    /* Break in half. */
    high = (DWORD)(timeBuffer.time >> 16) & 0xffff;
    low = (DWORD)(timeBuffer.time & 0xffff);

    /* Work on each half. */
    high = high * 1000 / WRAPPER_TICK_MS;
    low = (low * 1000 + timeBuffer.millitm) / WRAPPER_TICK_MS;

    /* Now combine them in such a way that the correct bits are truncated. */
    high = high + ((low >> 16) & 0xffff);
    sum = (TICKS)(((high & 0xffff) << 16) + (low & 0xffff));

    /* Check the result. */
#ifdef _DEBUG
#ifdef WIN32
    assertSum = (TICKS)((timeBuffer.time * 1000UI64 + timeBuffer.millitm) / WRAPPER_TICK_MS);
#else
    /* This will produce the following warning on some compilers:
     *  warning: ANSI C forbids long long integer constants
     * Is there another way to do this? */
    assertSum = (TICKS)((timeBuffer.time * 1000ULL + timeBuffer.millitm) / WRAPPER_TICK_MS);
#endif
    if (assertSum != sum) {
        printf("wrapperGetSystemTicks() resulted in %08x rather than %08x\n", sum, assertSum);
    }
#endif

    return sum;
}

/**
 * Returns difference in seconds between the start and end ticks.  This function
 *  handles cases where the tick counter has wrapped between when the start
 *  and end tick counts were taken.  See the wrapperGetTicks() function.
 *
 * This can be done safely in 32 bits
 */
int wrapperGetTickAgeSeconds(TICKS start, TICKS end) {
    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "      wrapperGetTickAgeSeconds(%08x, %08x) -> %08x", start, end, (int)((end - start) * WRAPPER_TICK_MS) / 1000);
    */

    /* Simply subtracting the values will always work even if end has wrapped
     *  due to overflow.
     *  0x00000001 - 0xffffffff = 0x00000002 = 2
     *  0xffffffff - 0x00000001 = 0xfffffffe = -2
     */
    return (int)((end - start) * WRAPPER_TICK_MS) / 1000;
}

/**
 * Returns difference in ticks between the start and end ticks.  This function
 *  handles cases where the tick counter has wrapped between when the start
 *  and end tick counts were taken.  See the wrapperGetTicks() function.
 *
 * This can be done safely in 32 bits
 */
int wrapperGetTickAgeTicks(TICKS start, TICKS end) {
    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "      wrapperGetTickAgeSeconds(%08x, %08x) -> %08x", start, end, (int)(end - start));
    */

    /* Simply subtracting the values will always work even if end has wrapped
     *  due to overflow.
     *  0x00000001 - 0xffffffff = 0x00000002 = 2
     *  0xffffffff - 0x00000001 = 0xfffffffe = -2
     */
    return (int)(end - start);
}

/**
 * Returns TRUE if the specified tick timeout has expired relative to the
 *  specified tick count.
 */
int wrapperTickExpired(TICKS nowTicks, TICKS timeoutTicks) {
    /* Convert to a signed value. */
    int age = nowTicks - timeoutTicks;

    if (age >= 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * Returns a tick count that is the specified number of seconds later than
 *  the base tick count.
 *
 * This calculation will work as long as the number of seconds is not large
 *  enough to require more than 32 bits when multiplied by 1000.
 */
TICKS wrapperAddToTicks(TICKS start, int seconds) {
    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "      wrapperAddToTicks(%08x, %08x) -> %08x", start, seconds, start + (seconds * 1000 / WRAPPER_TICK_MS));
    */
    return start + (seconds * 1000 / WRAPPER_TICK_MS);
}

/**
 * Do some sanity checks on the tick timer math.
 */
int wrapperTickAssertions() {
    int result = FALSE;
    TICKS ticks1, ticks2, ticksR, ticksE;
    int value1, valueR, valueE;
    
    /** wrapperGetTickAgeTicks test. */
    ticks1 = 0xfffffffe;
    ticks2 = 0xffffffff;
    valueE = 1;
    valueR = wrapperGetTickAgeTicks(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperGetTickAgeTicks(%08x, %08x) == %0d != %0d", ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }
    
    ticks1 = 0xffffffff;
    ticks2 = 0xfffffffe;
    valueE = -1;
    valueR = wrapperGetTickAgeTicks(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperGetTickAgeTicks(%08x, %08x) == %0d != %0d", ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }
    
    ticks1 = 0xffffffff;
    ticks2 = 0x00000000;
    valueE = 1;
    valueR = wrapperGetTickAgeTicks(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperGetTickAgeTicks(%08x, %08x) == %0d != %0d", ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }
    
    ticks1 = 0x00000000;
    ticks2 = 0xffffffff;
    valueE = -1;
    valueR = wrapperGetTickAgeTicks(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperGetTickAgeTicks(%08x, %08x) == %0d != %0d", ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }
    
    /** wrapperGetTickAgeSeconds test. */
    ticks1 = 0xfffffff0;
    ticks2 = 0xffffffff;
    valueE = 1;
    valueR = wrapperGetTickAgeSeconds(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperGetTickAgeSeconds(%08x, %08x) == %0d != %0d", ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }
    
    ticks1 = 0xffffffff;
    ticks2 = 0x0000000f;
    valueE = 1;
    valueR = wrapperGetTickAgeSeconds(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperGetTickAgeSeconds(%08x, %08x) == %0d != %0d", ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }
    
    ticks1 = 0x0000000f;
    ticks2 = 0xffffffff;
    valueE = -1;
    valueR = wrapperGetTickAgeSeconds(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperGetTickAgeSeconds(%08x, %08x) == %0d != %0d", ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }
    
    
    /** wrapperTickExpired test. */
    ticks1 = 0xfffffffe;
    ticks2 = 0xffffffff;
    valueE = FALSE;
    valueR = wrapperTickExpired(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperTickExpired(%08x, %08x) == %0d != %0d", ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }
    
    ticks1 = 0xffffffff;
    ticks2 = 0xffffffff;
    valueE = TRUE;
    valueR = wrapperTickExpired(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperTickExpired(%08x, %08x) == %0d != %0d", ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }
    
    ticks1 = 0xffffffff;
    ticks2 = 0x00000001;
    valueE = FALSE;
    valueR = wrapperTickExpired(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperTickExpired(%08x, %08x) == %0d != %0d", ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }
    
    ticks1 = 0x00000001;
    ticks2 = 0xffffffff;
    valueE = TRUE;
    valueR = wrapperTickExpired(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperTickExpired(%08x, %08x) == %0d != %0d", ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }
    
    /** wrapperAddToTicks test. */
    ticks1 = 0xffffffff;
    value1 = 1;
    ticksE = 0x00000009;
    ticksR = wrapperAddToTicks(ticks1, value1);
    if (ticksR != ticksE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, "Assert Failed: wrapperAddToTicks(%08x, %d) == %08x != %08x", ticks1, value1, ticksR, ticksE);
        result = TRUE;
    }
    
    return result;
}

/**
 * Sets the working directory of the Wrapper to the specified directory.
 *  The directory can be relative or absolute.
 * If there are any problems then a non-zero value will be returned.
 */
int wrapperSetWorkingDir(const char* dir) {
    int showOutput = wrapperData->configured;

    if (chdir(dir)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
            "Unable to set working directory to: %s (%s)", dir, getLastErrorText());
        return TRUE;
    }

    /* This function is sometimes called before the configuration is loaded. */
#ifdef _DEBUG
    showOutput = TRUE;
#endif

    if (showOutput) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Working directory set to: %s", dir);
    }

    /* Set a variable to the location of the binary. */
    setEnv("WRAPPER_WORKING_DIR", dir);

    return FALSE;
}

/******************************************************************************
 * Protocol callback functions
 *****************************************************************************/
void wrapperLogSignaled(int logLevel, char *msg) {
    /* */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "Got a log message from JVM: %s", msg);
    }
    /* */

    log_printf(wrapperData->jvmRestarts, logLevel, "%s", msg);
}

void wrapperKeyRegistered(char *key) {
    /* Allow for a large integer + \0 */
    char buffer[11];

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
            wrapperSetJavaState(FALSE, WRAPPER_JSTATE_LAUNCHED, 0, -1);

            /* Send the low log level to the JVM so that it can control output via the log method. */
            sprintf(buffer, "%d", getLowLogLevel());
            wrapperProtocolFunction(FALSE, WRAPPER_MSG_LOW_LOG_LEVEL, buffer);

            /* Send the ping timeout to the JVM. */
            if (wrapperData->pingTimeout >= WRAPPER_TIMEOUT_MAX) {
                /* Timeout disabled */
                sprintf(buffer, "%d", 0);
            } else {
                sprintf(buffer, "%d", wrapperData->pingTimeout);
            }
            wrapperProtocolFunction(FALSE, WRAPPER_MSG_PING_TIMEOUT, buffer);
            
            /* Send the log file name. */
            sendLogFileName();
            
            /* Send the properties. */
            sendProperties();
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Received a connection request with an incorrect key.  Waiting for another connection.");

            /* This was the wrong key.  Send a response. */
            wrapperProtocolFunction(FALSE, WRAPPER_MSG_BADKEY, "Incorrect key.  Connection rejected.");

            /* Close the current connection.  Assume that the real JVM
             *  is still out there trying to connect.  So don't change
             *  the state.  If for some reason, this was the correct
             *  JVM, but the key was wrong.  then this state will time
             *  out and recover. */
            wrapperProtocolClose();
        }
        break;

    case WRAPPER_JSTATE_STOP:
        /* We got a key registration.  This means that the JVM thinks it was
         *  being launched but the Wrapper is trying to stop.  This state
         *  will clean up correctly. */
        break;
        
    case WRAPPER_JSTATE_STOPPING:
        /* We got a key registration.  This means that the JVM thinks it was
         *  being launched but the Wrapper is trying to stop.  Now that the
         *  connection to the JVM has been opened, tell it to stop cleanly. */
        wrapperSetJavaState(FALSE, WRAPPER_JSTATE_STOP, 0, -1);
        break;

    default:
        /* We got a key registration that we were not expecting.  Ignore it. */
        break;
    }
}

void wrapperPingResponded() {
    /* Depending on the current JVM state, do something. */
    switch (wrapperData->jState) {
    case WRAPPER_JSTATE_STARTED:
        /* We got a response to a ping.  Allow 5 + <pingTimeout> more seconds before the JVM
         *  is considered to be dead. */
        if (wrapperData->pingTimeout > 0) {
            wrapperUpdateJavaStateTimeout(wrapperGetTicks(), 5 + wrapperData->pingTimeout);
        } else {
            wrapperUpdateJavaStateTimeout(wrapperGetTicks(), -1);
        }

        break;

    default:
        /* We got a ping response that we were not expecting.  Ignore it. */
        break;
    }
}

void wrapperStopRequested(int exitCode) {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            "JVM requested a shutdown. (%d)", exitCode);
    }

    /* Get things stopping on this end.  Ask the JVM to stop again in case the
     *  user code on the Java side is not written correctly. */
    wrapperStopProcess(FALSE, exitCode);
}

void wrapperRestartRequested() {
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "JVM requested a restart.");
    wrapperRestartProcess(FALSE);
}

/**
 * If the current state of the JVM is STOPPING then this message is used to
 *  extend the time that the wrapper will wait for a STOPPED message before
 *  giving up on the JVM and killing it.
 */
void wrapperStopPendingSignaled(int waitHint) {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM signaled a stop pending with waitHint of %d millis.", waitHint);
    }

    if (wrapperData->jState == WRAPPER_JSTATE_STARTED) {
        /* Change the state to STOPPING */
        wrapperSetJavaState(FALSE, WRAPPER_JSTATE_STOPPING, 0, -1);
        /* Don't need to set the timeout here because it will be set below. */
    }

    if (wrapperData->jState == WRAPPER_JSTATE_STOPPING) {
        if (waitHint < 0) {
            waitHint = 0;
        }

        wrapperUpdateJavaStateTimeout(wrapperGetTicks(), (int)ceil(waitHint / 1000.0));
    }
}

/**
 * The wrapper received a signal from the JVM that it has completed the stop
 *  process.  If the state of the JVM is STOPPING, then change the state to
 *  STOPPED.  It is possible to get this request after the Wrapper has given up
 *  waiting for the JVM.  In this case, the message is ignored.
 */
void wrapperStoppedSignaled() {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM signaled that it was stopped.");
    }

    /* The Java side of the wrapper signaled that it stopped
     *  allow 5 + jvmExitTimeout seconds for the JVM to exit. */
    if (wrapperData->jvmExitTimeout > 0) {
        wrapperSetJavaState(FALSE, WRAPPER_JSTATE_STOPPED, wrapperGetTicks(), 5 + wrapperData->jvmExitTimeout);
    } else {
        wrapperSetJavaState(FALSE, WRAPPER_JSTATE_STOPPED, 0, -1);
    }
}

/**
 * If the current state of the JVM is STARTING then this message is used to
 *  extend the time that the wrapper will wait for a STARTED message before
 *  giving up on the JVM and killing it.
 */
void wrapperStartPendingSignaled(int waitHint) {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM signaled a start pending with waitHint of %d millis.", waitHint);
    }

    /* Only process the start pending signal if the JVM state is starting or
     *  stopping.  Stopping are included because if the user hits CTRL-C while
     *  the application is starting, then the stop request will not be noticed
     *  until the application has completed its startup. */
    if ((wrapperData->jState == WRAPPER_JSTATE_STARTING) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOPPING)) {
        if (waitHint < 0) {
            waitHint = 0;
        }

        wrapperUpdateJavaStateTimeout(wrapperGetTicks(), (int)ceil(waitHint / 1000.0));
    }
}

/**
 * The wrapper received a signal from the JVM that it has completed the startup
 *  process.  If the state of the JVM is STARTING, then change the state to
 *  STARTED.  It is possible to get this request after the Wrapper has given up
 *  waiting for the JVM.  In this case, the message is ignored.
 */
void wrapperStartedSignaled() {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, "JVM signaled that it was started.");
    }

 
    if (wrapperData->jState == WRAPPER_JSTATE_STARTING) {
        /* We got a response to a ping.  Allow 5 + <pingTimeout> more seconds before the JVM
         *  is considered to be dead. */
        if (wrapperData->pingTimeout > 0) {
            wrapperSetJavaState(FALSE, WRAPPER_JSTATE_STARTED, wrapperGetTicks(), 5 + wrapperData->pingTimeout);
        } else {
            wrapperSetJavaState(FALSE, WRAPPER_JSTATE_STARTED, 0, -1);
        }
        /* Is the wrapper state STARTING? */
        if (wrapperData->wState == WRAPPER_WSTATE_STARTING) {
            wrapperSetWrapperState(FALSE, WRAPPER_WSTATE_STARTED);

            if (!wrapperData->isConsole) {
                /* Tell the service manager that we started */
                wrapperReportStatus(FALSE, WRAPPER_WSTATE_STARTED, 0, 0);
                    
            }
        }
    } else if (wrapperData->jState == WRAPPER_JSTATE_STOP) {
        /* This will happen if the Wrapper was asked to stop as the JVM is being launched. */
    } else if (wrapperData->jState == WRAPPER_JSTATE_STOPPING) {
        /* This will happen if the Wrapper was asked to stop as the JVM is being launched. */
        wrapperSetJavaState(FALSE, WRAPPER_JSTATE_STOP, 0, -1);
    }
}
