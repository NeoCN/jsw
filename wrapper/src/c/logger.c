/*
 * Copyright (c) 1999, 2003 TanukiSoftware.org
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without 
 * restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sub-license , and/or 
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
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * $Log$
 * Revision 1.32  2004/01/09 19:45:03  mortenson
 * Implement the tick timer on Linux.
 *
 * Revision 1.31  2004/01/09 17:49:00  mortenson
 * Rework the logging so it is now threadsafe.
 *
 * Revision 1.30  2003/10/30 19:34:34  mortenson
 * Added a new wrapper.ntservice.console property so the console can be shown for
 * services.
 * Fixed a problem where requesting thread dumps on exit was failing when running
 * as a service.
 *
 * Revision 1.29  2003/08/02 15:50:02  mortenson
 * Implement getLastErrorText on UNIX versions.
 *
 * Revision 1.28  2003/07/04 03:57:18  mortenson
 * Convert tabs to spaces.
 *
 * Revision 1.27  2003/07/04 03:32:22  mortenson
 * The error code was being displayed twice when unable to write to the event log.
 *
 * Revision 1.26  2003/07/04 03:18:36  mortenson
 * Improve the error message displayed when the NT EventLog is full in response
 * to feature request #643617.
 *
 * Revision 1.25  2003/04/16 04:13:10  mortenson
 * Go through and clean up the computation of the number of bytes allocated in
 * malloc statements to make sure that string sizes are always multiplied by
 * sizeof(char), etc.
 *
 * Revision 1.24  2003/04/15 23:24:21  mortenson
 * Remove casts from all malloc statements.
 *
 * Revision 1.23  2003/04/15 14:17:43  mortenson
 * Clean up the code by setting all malloced variables to NULL after they are freed,
 *
 * Revision 1.22  2003/04/03 04:05:22  mortenson
 * Fix several typos in the docs.  Thanks to Mike Castle.
 *
 * Revision 1.21  2003/02/07 16:05:27  mortenson
 * Implemented feature request #676599 to enable the filtering of JVM output to
 * trigger JVM restarts or Wrapper shutdowns.
 *
 * Revision 1.20  2003/02/03 06:55:26  mortenson
 * License transfer to TanukiSoftware.org
 *
 */

/**
 * Author:
 *   Johan Sorlin   <Johan.Sorlin@Paregos.se>
 *   Leif Mortenson <leif@tanukisoftware.com>
 *
 * Version CVS $Revision$ $Date$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#include <conio.h>
#else
#include <syslog.h>
#include <strings.h>
#include <pthread.h>
#endif

#include "logger.h"

/* Global data for logger */

/* Initialize all log levels to unknown until they are set */
int currentConsoleLevel = LEVEL_UNKNOWN;
int currentLogfileLevel = LEVEL_UNKNOWN;
int currentLoginfoLevel = LEVEL_UNKNOWN;

//char szBuff[ MAX_LOG_SIZE + 1 ];
char logFilePath[ 1024 ];
char *logLevelNames[] = { "NONE  ", "DEBUG ", "INFO  ", "STATUS", "WARN  ", "ERROR ", "FATAL " };
char loginfoSourceName[ 1024 ];
int  logFileMaxSize = -1;
int  logFileMaxLogFiles = -1;

/* Defualt formats (Must be 4 chars) */
char consoleFormat[32];
char logfileFormat[32];

/* Internal function declaration */
void sendEventlogMessage( int source_id, int level, char *szBuff );
void sendLoginfoMessage( int source_id, int level, char *szBuff );
#ifdef WIN32
void writeToConsole( char *lpszFmt, ... );
#endif
void checkAndRollLogs( );

/* Thread specific work buffers. */
#ifdef WIN32
DWORD threadIds[WRAPPER_THREAD_COUNT];
#else
pthread_t threadIds[WRAPPER_THREAD_COUNT];
#endif
char *threadMessageBuffers[WRAPPER_THREAD_COUNT];
int threadMessageBufferSizes[WRAPPER_THREAD_COUNT];
char *threadPrintBuffers[WRAPPER_THREAD_COUNT];
int threadPrintBufferSizes[WRAPPER_THREAD_COUNT];

#ifdef WIN32
HANDLE consoleStdoutHandle = NULL;
void setConsoleStdoutHandle( HANDLE stdoutHandle ) {
    consoleStdoutHandle = stdoutHandle;
}
#endif

void initLogBuffers() {
    int i;

    for ( i = 0; i < WRAPPER_THREAD_COUNT; i++ ) {
        threadIds[i] = 0;
        threadMessageBuffers[i] = NULL;
        threadMessageBufferSizes[i] = 0;
        threadPrintBuffers[i] = NULL;
        threadPrintBufferSizes[i] = 0;
    }
}

/** Registers the calling thread so it can be recognized when it calls
 *  again later. */
void logRegisterThread( int thread_id ) {
#ifdef WIN32
    DWORD threadId;
    threadId = GetCurrentThreadId();
#else
    pthread_t threadId;
    threadId = pthread_self();
#endif

    if ( thread_id >= 0 && thread_id < WRAPPER_THREAD_COUNT )
    {
        threadIds[thread_id] = threadId;
    }
}

int getThreadId() {
    int i;
#ifdef WIN32
    DWORD threadId;
    threadId = GetCurrentThreadId();
#else
    pthread_t threadId;
    threadId = pthread_self();
#endif
    /* printf( "threadId=%lu\n", threadId ); */

    for ( i = 0; i < WRAPPER_THREAD_COUNT; i++ ) {
        if ( threadIds[i] == threadId ) {
            return i;
        }
    }
    
    printf( "WARNING - Encountered an unknown thread %ld in getThreadId().\n", threadId );
    return threadIds[0]; /* WRAPPER_THREAD_SIGNAL */
}

int strcmpIgnoreCase( const char *str1, const char *str2 ) {
#ifdef WIN32
    return stricmp(str1, str2);
#else /* UNIX */
    return strcasecmp(str1, str2);
#endif
}

int getLogLevelForName( char *logLevelName ) {
    if (strcmpIgnoreCase(logLevelName, "NONE") == 0) {
        return LEVEL_NONE;
    } else if (strcmpIgnoreCase(logLevelName, "FATAL") == 0) {
        return LEVEL_FATAL;
    } else if (strcmpIgnoreCase(logLevelName, "ERROR") == 0) {
        return LEVEL_ERROR;
    } else if (strcmpIgnoreCase(logLevelName, "WARN") == 0) {
        return LEVEL_WARN;
    } else if (strcmpIgnoreCase(logLevelName, "STATUS") == 0) {
        return LEVEL_STATUS;
    } else if (strcmpIgnoreCase(logLevelName, "INFO") == 0) {
        return LEVEL_INFO;
    } else if (strcmpIgnoreCase(logLevelName, "DEBUG") == 0) {
        return LEVEL_DEBUG;
    } else {
        return LEVEL_UNKNOWN;
    }
}

/* Logfile functions */

void setLogfilePath( char *log_file_path ) {
    strcpy( logFilePath, log_file_path );
}

void setLogfileFormat( char *log_file_format ) {
    if( log_file_format != NULL )
        strcpy( logfileFormat, log_file_format );
}

void setLogfileLevelInt( int log_file_level ) {
    currentLogfileLevel = log_file_level;
}

int getLogfileLevelInt() {
    return currentLogfileLevel;
}

void setLogfileLevel( char *log_file_level ) {
    setLogfileLevelInt(getLogLevelForName(log_file_level));
}

void setLogfileMaxFileSize( char *max_file_size ) {
    int multiple, i, newLength;
    char *tmpFileSizeBuff;
    char chr;

    if( max_file_size != NULL ) {
        /* Allocate buffer */
        if( (tmpFileSizeBuff = (char *) malloc(sizeof(char) * (strlen( max_file_size ) + 1))) == NULL )
            return;

        /* Generate multiple and remove unwanted chars */
        multiple = 1;
        newLength = 0;
        for( i=0; i<(int) strlen(max_file_size); i++ ) {
            chr = max_file_size[i];

            switch( chr ) {
                case 'k': /* Kilobytes */
                case 'K':
                    multiple = 1000;
                break;

                case 'M': /* Megabytes */
                case 'm':
                    multiple = 1000000;
                break;
            }

            if( (chr >= '0' && chr <= '9') || (chr == '-') )
                tmpFileSizeBuff[newLength++] = max_file_size[i];
        }
        tmpFileSizeBuff[newLength] = '\0';/* Crop string */

        logFileMaxSize = atoi( tmpFileSizeBuff );
        if( logFileMaxSize > 0 )
            logFileMaxSize *= multiple;

        /* Free memory */
        free( tmpFileSizeBuff );
        tmpFileSizeBuff = NULL;
    }
}

void setLogfileMaxFileSizeInt( int max_file_size ) {
    logFileMaxSize = max_file_size;
}

void setLogfileMaxLogFiles( char *max_log_files ) {
    if( max_log_files != NULL )
        logFileMaxLogFiles = atoi( max_log_files );
}

void setLogfileMaxLogFilesInt( int max_log_files ) {
    logFileMaxLogFiles = max_log_files;
}

/* Console functions */
void setConsoleLogFormat( char *console_log_format ) {
    if( console_log_format != NULL )
        strcpy( consoleFormat, console_log_format );
}

void setConsoleLogLevelInt( int console_log_level ) {
    currentConsoleLevel = console_log_level;
}

int getConsoleLogLevelInt() {
    return currentConsoleLevel;
}

void setConsoleLogLevel( char *console_log_level ) {
    setConsoleLogLevelInt(getLogLevelForName(console_log_level));
}

/* Syslog/eventlog functions */
void setSyslogLevelInt( int loginfo_level ) {
    currentLoginfoLevel = loginfo_level;
}

int getSyslogLevelInt() {
    return currentLoginfoLevel;
}

void setSyslogLevel( char *loginfo_level ) {
    setSyslogLevelInt(getLogLevelForName(loginfo_level));
}

void setSyslogEventSourceName( char *event_source_name ) {
    if( event_source_name != NULL )
        strcpy( loginfoSourceName, event_source_name );
}

int getLowLogLevel() {
    int lowLogLevel = (currentLogfileLevel < currentConsoleLevel ? currentLogfileLevel : currentConsoleLevel);
    lowLogLevel =  (currentLoginfoLevel < lowLogLevel ? currentLoginfoLevel : lowLogLevel);
    return lowLogLevel;
}

/* Writes to and then returns a buffer that is reused by the current thread.
 *  It should not be released. */
char* buildPrintBuffer( int thread_id, int source_id, int level, char *format, char *message ) {
    time_t    now;
    struct tm *nowTM;
    int       i;
    int       reqSize;
    int       numColumns;
    char      *pos;
    int       currentColumn;
    int       handledFormat;

    /* Build a timestamp */
    now = time( NULL );
    nowTM = localtime( &now );
    
    /* Decide the number of columns and come up with a required length for the printBuffer. */
    reqSize = 0;
    for( i = 0, numColumns = 0; i < (int)strlen( format ); i++ ) {
        switch( format[i] ) {
        case 'P':
            reqSize += 8 + 3;
            numColumns++;
            break;

        case 'L':
            reqSize += 6 + 3;
            numColumns++;
            break;

        case 'D':
            reqSize += 7 + 3;
            numColumns++;
            break;

        case 'T':
            reqSize += 19 + 3;
            numColumns++;
            break;

        case 'M':
            reqSize += strlen( message ) + 3;
            numColumns++;
            break;
        }

        /* Always add room for the null. */
        reqSize += 1;
    }

    if ( threadPrintBuffers[thread_id] == NULL ) {
        threadPrintBuffers[thread_id] = (char *)malloc( reqSize * sizeof( char ) );
        threadPrintBufferSizes[thread_id] = reqSize;
    } else if ( threadPrintBufferSizes[thread_id] < reqSize ) {
        free( threadPrintBuffers[thread_id] );
        threadPrintBuffers[thread_id] = (char *)malloc( reqSize * sizeof( char ) );
        threadPrintBufferSizes[thread_id] = reqSize;
    }

    /* Create a pointer to the beginning of the print buffer, it will be advanced
     *  as the formatted message is build up. */
    pos = threadPrintBuffers[thread_id];

    /* We now have a buffer large enough to store the entire formatted message. */
    for( i = 0, currentColumn = 0; i < (int)strlen( format ); i++ ) {
        handledFormat = 1;

        switch( format[i] ) {
        case 'P':
            switch ( source_id ) {
            case WRAPPER_SOURCE_WRAPPER:
                pos += sprintf( pos, "wrapper " );
                break;

            case WRAPPER_SOURCE_PROTOCOL:
                pos += sprintf( pos, "wrapperp" );
                break;

            default:
                pos += sprintf( pos, "jvm %-4d", source_id );
                break;
            }
            currentColumn++;
            break;

        case 'L':
            pos += sprintf( pos, "%s", logLevelNames[ level ] );
            currentColumn++;
            break;

        case 'D':
            switch ( thread_id )
            {
            case WRAPPER_THREAD_SIGNAL:
                pos += sprintf( pos, "signal " );
                break;

            case WRAPPER_THREAD_MAIN:
                pos += sprintf( pos, "main   " );
                break;

            case WRAPPER_THREAD_SRVMAIN:
                pos += sprintf( pos, "srvmain" );
                break;

            case WRAPPER_THREAD_TIMER:
                pos += sprintf( pos, "timer  " );
                break;

            default:
                pos += sprintf( pos, "unknown" );
                break;
            }
            currentColumn++;
            break;

        case 'T':
            pos += sprintf( pos, "%04d/%02d/%02d %02d:%02d:%02d", 
                nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday, 
                nowTM->tm_hour, nowTM->tm_min, nowTM->tm_sec );
            currentColumn++;
            break;

        case 'M':
            pos += sprintf( pos, "%s", message );
            currentColumn++;
            break;

        default:
            handledFormat = 0;
        }

        /* Add separator chars */
        if ( handledFormat && ( currentColumn != numColumns ) ) {
            pos += sprintf( pos, " | " );
        }
    }

    /* Return the print buffer to the caller. */
    return threadPrintBuffers[thread_id];
}

/* General log functions */
void log_printf( int source_id, int level, char *lpszFmt, ... ) {
    va_list		vargs;
    FILE		*logfileFP = NULL;
    int         thread_id;
    int         count;
    char        *printBuffer;
    
    /* The contents of this function are not thread safe but it is called by multiple
     *  threads.  To make this safe we need to either refrain from using static
     *  buffers, which would be slow.  Or make sure that a buffer is reserved for each
     *  thread.  We choose the later.
     *
     * Obtain a thread_id that will be used as an index to to the static buffer array. */
    thread_id = getThreadId();

    /* Loop until the buffer is large enough that we are able to successfully
     *  print into it. Once the buffer has grown to the largest message size,
     *  smaller messages will pass through this code without looping. */
    do {
        if ( threadMessageBufferSizes[thread_id] == 0 )
        {
            /* No buffer yet. It will be allocated below. */
            count = -1;
        }
        else
        {
            /* Try writing to the buffer. */
            va_start( vargs, lpszFmt );
#ifdef WIN32
            count = _vsnprintf( threadMessageBuffers[thread_id], threadMessageBufferSizes[thread_id], lpszFmt, vargs );
#else
            count = vsnprintf( threadMessageBuffers[thread_id], threadMessageBufferSizes[thread_id], lpszFmt, vargs );
#endif
            va_end( vargs );
            if ( ( count < 0 ) || ( count == threadMessageBufferSizes[thread_id] ) ) {
                /* If the count is exactly equal to the buffer size then a null char was not written.  It must be larger. */
                /* Failed, free the buffer so a larger one can be reallocated below. */
                free( threadMessageBuffers[thread_id] );
                count = -1;
            }
        }
        if ( count < 0 ) {
            /* We need to allocate a new buffer. */
            threadMessageBufferSizes[thread_id] += 100;
            threadMessageBuffers[thread_id] = (char*)malloc( threadMessageBufferSizes[thread_id] * sizeof(char) );
        }
    } while ( count < 0 );

    /* Console output by format */
    if( level >= currentConsoleLevel ) {
        /* Build up the printBuffer. */
        printBuffer = buildPrintBuffer( thread_id, source_id, level, consoleFormat, threadMessageBuffers[thread_id] );

        /* Write the print buffer to the console. */
#ifdef WIN32
        if ( consoleStdoutHandle != NULL ) {
            writeToConsole( "%s\n", printBuffer );
        } else {
#endif
            fprintf( stdout, "%s\n", printBuffer );
#ifdef WIN32
        }
#endif
    }

    /* Logfile output by format */
    /* Make sure that the log file does not need to be rolled. */
    checkAndRollLogs( );

    /* Log the message to the log file */
    if (level >= currentLogfileLevel) {
        logfileFP = fopen( logFilePath, "a" );
        if (logfileFP == NULL) {
            /* The log file could not be opened.  Try the default file location. */
            logfileFP = fopen( "wrapper.log", "a" );
        }
        
        if (logfileFP != NULL) {
            /* Build up the printBuffer. */
            printBuffer = buildPrintBuffer( thread_id, source_id, level, logfileFormat, threadMessageBuffers[thread_id] );
    
            fprintf( logfileFP, "%s\n", printBuffer );
    
            fclose( logfileFP );
        }
    }

    /* Loginfo/Eventlog if levels match (not by format timecodes/status allready exists in evenlog) */
    if( level >= currentLoginfoLevel ) {
        sendEventlogMessage( source_id, level, threadMessageBuffers[thread_id] );
        sendLoginfoMessage( source_id, level, threadMessageBuffers[thread_id] );
    }
}

/* Internal functions */

/**
 * Create an error message from GetLastError() using the
 *  FormatMessage API Call...
 */
#ifdef WIN32
TCHAR lastErrBuf[1024];
char* getLastErrorText() {
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                           FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL);

    /* supplied buffer is not long enough */
    if (!dwRet || ((long)1023 < (long)dwRet+14)) {
        lastErrBuf[0] = TEXT('\0');
    } else {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  /*remove cr and newline character */
        _stprintf( lastErrBuf, TEXT("%s (0x%x)"), lpszTemp, GetLastError());
    }

    if (lpszTemp) {
        GlobalFree((HGLOBAL) lpszTemp);
    }

    return lastErrBuf;
}
#else
char* getLastErrorText() {
    return strerror(errno);
}
#endif

int registerSyslogMessageFile( ) {
#ifdef WIN32
    char buffer[ 1024 ];
    char regPath[ 1024 ];
    HKEY hKey;
    DWORD categoryCount, typesSupported;

    /* Get absolute path to service manager */
    if( GetModuleFileName( NULL, buffer, _MAX_PATH ) ) {
        sprintf( regPath, "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\%s", loginfoSourceName );

        if( RegCreateKey( HKEY_LOCAL_MACHINE, regPath, (PHKEY) &hKey ) == ERROR_SUCCESS ) {
            RegCloseKey( hKey );

            if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, regPath, 0, KEY_WRITE, (PHKEY) &hKey ) == ERROR_SUCCESS ) {
                /* Set EventMessageFile */
                if( RegSetValueEx( hKey, "EventMessageFile", (DWORD) 0, (DWORD) REG_SZ, (const unsigned char *) buffer, (strlen(buffer) + 1) ) != ERROR_SUCCESS ) {
                    RegCloseKey( hKey );
                    return -1;
                }

                /* Set CategoryMessageFile */
                if( RegSetValueEx( hKey, "CategoryMessageFile", (DWORD) 0, (DWORD) REG_SZ, (const unsigned char *) buffer, (strlen(buffer) + 1) ) != ERROR_SUCCESS ) {
                    RegCloseKey( hKey );
                    return -1;
                }

                /* Set CategoryCount */
                categoryCount = 12;
                if( RegSetValueEx( hKey, "CategoryCount", (DWORD) 0, (DWORD) REG_DWORD, (LPBYTE) &categoryCount, sizeof(DWORD) ) != ERROR_SUCCESS ) {
                    RegCloseKey( hKey );
                    return -1;
                }

                /* Set TypesSupported */
                typesSupported = 7;
                if( RegSetValueEx( hKey, "TypesSupported", (DWORD) 0, (DWORD) REG_DWORD, (LPBYTE) &typesSupported, sizeof(DWORD) ) != ERROR_SUCCESS ) {
                    RegCloseKey( hKey );
                    return -1;
                }

                RegCloseKey( hKey );
                return 0;
            }
        }
    }

    return -1; /* Failure */
#else
    return 0;
#endif
}

int unregisterSyslogMessageFile( ) {
#ifdef WIN32
    /* If we deregister this application, then the event viewer will not work when the program is not running. */
    /* Don't want to clutter up the Registry, but is there another way?  */
    char regPath[ 1024 ];

    /* Get absolute path to service manager */
    sprintf( regPath, "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\%s", loginfoSourceName );

    if( RegDeleteKey( HKEY_LOCAL_MACHINE, regPath ) == ERROR_SUCCESS )
        return 0;

    return -1; /* Failure */
#else
    return 0;
#endif
}

void sendEventlogMessage( int source_id, int level, char *szBuff ) {
#ifdef WIN32
    char   header[16];
    char   **strings = (char **) malloc( 3 * sizeof( char * ) );
    WORD   eventType;
    HANDLE handle;
    WORD   eventID, categoryID;
    int    result;

    /* Build the source header */
    switch( source_id ) {
        case WRAPPER_SOURCE_WRAPPER:
            sprintf( header, "wrapper" );
        break;

        case WRAPPER_SOURCE_PROTOCOL:
            sprintf( header, "wrapperp" );
        break;

        default:
            sprintf( header, "jvm %d", source_id );
        break;
    }

    /* Build event type by level */
    switch( level ) {
        case LEVEL_FATAL:
            eventType = EVENTLOG_ERROR_TYPE;
        break;

        case LEVEL_ERROR:
        case LEVEL_WARN:
            eventType = EVENTLOG_WARNING_TYPE;
        break;

        case LEVEL_STATUS:
        case LEVEL_INFO:
        case LEVEL_DEBUG:
            eventType = EVENTLOG_INFORMATION_TYPE;
        break;
    }

    /* Handle categories */
    categoryID = 10; /* jvmxx */
    if( source_id == WRAPPER_SOURCE_WRAPPER )
        categoryID = 11; /* wrapper */
    else if( source_id == WRAPPER_SOURCE_PROTOCOL )
        categoryID = 12; /* wrapperp */
    else if( (source_id >= 1) && (source_id <= 9) )
        categoryID = source_id; /* jvm1-9 */

    /* Place event in eventlog */
    strings[0] = header;
    strings[1] = szBuff;
    strings[2] = 0;
    eventID = level;

    handle = RegisterEventSource( 0, loginfoSourceName );
    if( !handle )
        return;

    result = ReportEvent(
        handle,                   /* handle to event log */
        eventType,                /* event type */
        categoryID,				  /* event category */
        100,                      /* event identifier */
        0,                        /* user security identifier */
        2,                        /* number of strings to merge */
        0,                        /* size of binary data */
        (const char **) strings,  /* array of strings to merge */
        0                         /* binary data buffer */
    );
    if (result == 0) {
        /* If there are any errors accessing the event log, like it is full, then disable its output. */
        setSyslogLevelInt(LEVEL_NONE);

        /* Recurse so this error gets set in the log file and console.  The syslog
         *  output has been disabled so we will not get back here. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Unable to write to the EventLog due to: %s", getLastErrorText());
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Internally setting wrapper.syslog.loglevel=NONE to prevent further messages.");
    }

    DeregisterEventSource( handle );

    free( (void *) strings );
    strings = NULL;
#endif
}

void sendLoginfoMessage( int source_id, int level, char *szBuff ) {
#ifndef WIN32 /* If UNIX */
    int eventType;

    /* Build event type by level */
    switch( level ) {
        case LEVEL_FATAL:
            eventType = LOG_CRIT;
        break;

        case LEVEL_ERROR:
            eventType = LOG_ERR;
        break;

        case LEVEL_WARN:
        case LEVEL_STATUS:
            eventType = LOG_NOTICE;
        break;

        case LEVEL_INFO:
            eventType = LOG_INFO;
        break;

        case LEVEL_DEBUG:
            eventType = LOG_DEBUG;
        break;

        default:
            eventType = LOG_DEBUG;
    }

    openlog( loginfoSourceName, LOG_PID | LOG_NDELAY, LOG_USER );
    syslog( eventType, "%s", szBuff );
    closelog( );
#endif
}

#ifdef WIN32
int vWriteToConsoleBufferSize = 100;
char *vWriteToConsoleBuffer = NULL;
void vWriteToConsole( char *lpszFmt, va_list vargs ) {
    int cnt;
    DWORD wrote;

    /* This should only be called if consoleStdoutHandle is set. */
    if ( consoleStdoutHandle == NULL ) {
        return;
    }

    if ( vWriteToConsoleBuffer == NULL ) {
        vWriteToConsoleBuffer = (char *)malloc( vWriteToConsoleBufferSize * sizeof(char) );
    }

    /* The only way I could figure out how to write to the console
     *  returned by AllocConsole when running as a service was to
     *  do all of this special casing and use the handle to the new
     *  console's stdout and the WriteConsole function.  If anyone
     *  puzzling over this code knows a better way of doing this
     *  let me know.
     * WriteConsole takes a fixed buffer and does not do any expansions
     *  We need to prepare the string to be displayed ahead of time.
     *  This means storing the message into a temporary buffer.  The
     *  following loop will expand the global buffer to hold the current
     *  message.  It will grow as needed to handle any arbitrarily large
     *  user message.  The buffer needs to be able to hold all available
     *  characters + a null char. */
    while ( ( cnt = _vsnprintf( vWriteToConsoleBuffer, vWriteToConsoleBufferSize - 1, lpszFmt, vargs ) ) < 0 ) {
        /* Expand the size of the buffer */
        free( vWriteToConsoleBuffer );
        vWriteToConsoleBufferSize += 100;
        vWriteToConsoleBuffer = (char *)malloc( vWriteToConsoleBufferSize * sizeof(char) );
    }

    /* We can now write the message. */
    WriteConsole(consoleStdoutHandle, vWriteToConsoleBuffer, strlen( vWriteToConsoleBuffer ), &wrote, NULL);
}
void writeToConsole( char *lpszFmt, ... ) {
    va_list		vargs;

    va_start( vargs, lpszFmt );
    vWriteToConsole( lpszFmt, vargs );
    va_end( vargs );
}
#endif


void checkAndRollLogs() {
    struct stat fileStat;
    char *tmpLogFilePathOld;
    char *tmpLogFilePathNew;
    int result;
    int i;

    if (logFileMaxSize <= 0)
        return;

    if (stat(logFilePath, &fileStat) == 0) {
        /* printf("%s has size=%d >= %d?\n", logFilePath, fileStat.st_size, logFileMaxSize); */

        /* Does the log file need to rotated? */
        if(fileStat.st_size >= logFileMaxSize) {
            tmpLogFilePathOld = NULL;
            tmpLogFilePathNew = NULL;
#ifdef _DEBUG
                printf("Rolling log files...\n");
#endif
            /* Allocate buffers (Allow for 10 digit file indices) */
            if ((tmpLogFilePathOld = malloc(sizeof(char) * (strlen(logFilePath) + 10 + 2))) == NULL) {
                /* Don't log this as with other errors as that would cause recursion. */
                fprintf(stderr, "Out of memory.\n");
                goto cleanup;
            }
            if ((tmpLogFilePathNew = malloc(sizeof(char) * (strlen(logFilePath) + 10 + 2))) == NULL) {
                /* Don't log this as with other errors as that would cause recursion. */
                fprintf(stderr, "Out of memory.\n");
                goto cleanup;
            }

            /* We don't know how many log files need to be rotated yet, so look. */
            i = 0;
            do {
                i++;
                sprintf(tmpLogFilePathOld, "%s.%d", logFilePath, i);
                result = stat(tmpLogFilePathOld, &fileStat);
#ifdef _DEBUG
                if (result == 0) {
                    printf("Rolled log file %s exists.\n", tmpLogFilePathOld);
                }
#endif
            } while((result == 0) && ((logFileMaxLogFiles <= 0) || (i < logFileMaxLogFiles)));

            /* Remove the file with the highest index if it exists */
            sprintf(tmpLogFilePathOld, "%s.%d", logFilePath, i);
            remove(tmpLogFilePathOld);

            /* Now, starting at the highest file rename them up by one index. */
            for (; i > 1; i--) {
                sprintf(tmpLogFilePathNew, "%s", tmpLogFilePathOld);
                sprintf(tmpLogFilePathOld, "%s.%d", logFilePath, i - 1);

                if (rename(tmpLogFilePathOld, tmpLogFilePathNew) != 0) {
                    if (errno == 13) {
                        /* Don't log this as with other errors as that would cause recursion. */
                        printf("Unable to rename log file %s to %s.  File is in use by another application.\n",
                            tmpLogFilePathOld, tmpLogFilePathNew);
                    } else {
                        /* Don't log this as with other errors as that would cause recursion. */
                        printf("Unable to rename log file %s to %s. (errno %d)\n",
                            tmpLogFilePathOld, tmpLogFilePathNew, errno);
                    }
                    goto cleanup;
                }
#ifdef _DEBUG
                else {
                    printf("Renamed %s to %s\n", tmpLogFilePathOld, tmpLogFilePathNew);
                }
#endif
            }

            /* Rename the current file to the #1 index position */
            sprintf(tmpLogFilePathNew, "%s", tmpLogFilePathOld);
            if (rename(logFilePath, tmpLogFilePathNew) != 0) {
                if (errno == 13) {
                    /* Don't log this as with other errors as that would cause recursion. */
                    printf("Unable to rename log file %s to %s.  File is in use by another application.\n",
                        logFilePath, tmpLogFilePathNew);
                } else {
                    /* Don't log this as with other errors as that would cause recursion. */
                    printf("Unable to rename log file %s to %s. (errno %d)\n",
                        logFilePath, tmpLogFilePathNew, errno);
                }
                goto cleanup;
            }
#ifdef _DEBUG
            else {
                printf("Renamed %s to %s\n", logFilePath, tmpLogFilePathNew);
            }
#endif

            cleanup:
            /* Free memory */
            if (tmpLogFilePathOld != NULL) {
                free((void *)tmpLogFilePathOld);
                tmpLogFilePathOld = NULL;
            }
            if (tmpLogFilePathNew != NULL) {
                free((void *)tmpLogFilePathNew);
                tmpLogFilePathNew = NULL;
            }
        }
    }
}
