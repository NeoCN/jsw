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
 * Revision 1.17  2002/11/15 16:30:14  spocke
 * Fixed bug where wrapper.logfile.maxsize produced 0 if it didn't have an k or m multiple.
 *
 * Revision 1.16  2002/09/10 16:01:12  mortenson
 * Fix some java c++ style comments that slipped into the code.
 *
 * Revision 1.15  2002/09/09 17:19:43  mortenson
 * Add ability to log to specific log levels from within the Wrapper.
 *
 * Revision 1.14  2002/09/09 15:52:41  mortenson
 * Fix an allignment problem with WARN level output.
 *
 * Revision 1.13  2002/08/11 05:21:28  mortenson
 * Add a Warning level to the logger
 *
 * Revision 1.12  2002/05/08 03:59:43  mortenson
 * Fix a problem where the log output was not being directed to a file called
 *  wrapper.log in the same directory as the Wrapper binary in the event that the
 *  configured wrapper log file could not be accessed.
 * I think there was also a file handle leak there that I noticed when I was looking at
 *  the code.
 *
 * Revision 1.11  2002/03/29 05:23:21  mortenson
 * Fix Bug #531880 involving percent characters in JVM output.
 *
 * Revision 1.10  2002/03/07 09:23:25  mortenson
 * Go through and change the style of comments that we use so that they will not
 * cause compiler errors on older unix compilers.
 *
 * Revision 1.9  2002/02/19 10:34:11  spocke
 * Fixed bug issue with core dumps when trying to close an un-opened log file.
 *
 * Revision 1.8  2002/02/08 05:55:55  mortenson
 * Make the syslog never unregister to avoid EventLog errors.
 *
 * Revision 1.7  2002/02/02 16:02:13  spocke
 * re-enabled the unregisterSyslogMessageFile it's now executed when
 * a service is removed.
 *
 * Revision 1.6  2002/01/28 19:05:01  spocke
 * Changed registerSyslogMessageFile so it only places Eventlog registry info
 * when the syslog is enabled.
 *
 * Revision 1.5  2002/01/27 19:30:58  spocke
 * Added include on errno.h to enable compilation on Unix.
 *
 * Revision 1.4  2002/01/27 16:59:54  mortenson
 * Modified the log rolling code so that a max log files value is not required.
 *
 * Revision 1.3  2002/01/27 15:02:45  spocke
 * Fixed some Unix issues, so it compiles better.
 *
 * Revision 1.2  2002/01/26 23:31:03  spocke
 * Added rolling file support to logger.
 *
 * Revision 1.1  2002/01/24 09:38:56  mortenson
 * Added new Logger code contributed by Johan Sorlin
 *
 */

/**
 * Author:
 *   Johan Sorlin   <Johan.Sorlin@Paregos.se>
 *   Leif Mortenson <leif@silveregg.co.jp>
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
#else
#include <syslog.h>
#endif

#include "logger.h"
#include "wrapper.h"

/* Global data for logger */

/* Initialize all log levels to unknown until they are set */
int currentConsoleLevel = LEVEL_UNKNOWN;
int currentLogfileLevel = LEVEL_UNKNOWN;
int currentLoginfoLevel = LEVEL_UNKNOWN;

char szBuff[ MAX_LOG_SIZE + 1 ];
char logFilePath[ 1024 ];
char *logLevelNames[] = { "NONE  ", "DEBUG ", "INFO  ", "STATUS", "WARN  ", "ERROR ", "FATAL " };
char loginfoSourceName[ 1024 ];
int  logFileMaxSize = -1;
int  logFileMaxLogFiles = -1;

/* Defualt formats (Must be 4 chars) */
char consoleFormat[32];
char logfileFormat[32];

/* Internal function declaration */
int registerEventlogMessageFile( );
int unregisterEventlogMessageFile( );
void sendEventlogMessage( int source_id, int level, char *szBuff );
void sendLoginfoMessage( int source_id, int level, char *szBuff );
void writeTimeToStream( FILE *fp );
void writeHeaderToStream( FILE *fp, int source_id );
void writeLevelToStream( FILE *fp, int level );
void writeMessageToStream( FILE *fp, char *lpszFmt, va_list vargs );
void checkAndRollLogs( );

int strcmpIgnoreCase( char *str1, char *str2 ) {
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
        if( (tmpFileSizeBuff = (char *) malloc( strlen( max_file_size ) + 1 )) == NULL )
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

/* General log functions */
void log_printf( int source_id, int level, char *lpszFmt, ... ) {
    va_list		vargs;
    int			i;
    int			handledFormat, numColumns, currentColumn;
    FILE		*logfileFP = NULL;

    /* Console output by format */
    if( level >= currentConsoleLevel ) {
        /* Count number of columns inorder to skip last '|' char */
        for( i = 0, numColumns = 0; i < (int)strlen( consoleFormat ); i++ ) {
            switch( consoleFormat[i] ) {
                case 'P':
                case 'L':
                case 'M':
                case 'T':
                    numColumns++;
                break;
            }
        }

        for( i = 0, currentColumn = 0; i < (int)strlen( consoleFormat ); i++ ) {
            handledFormat = 1;

            switch( consoleFormat[i] ) {
                case 'P': /* Prefix */
                    writeHeaderToStream( stderr, source_id );
                    currentColumn++;
                break;

                case 'L': /* Loglevel */
                    writeLevelToStream( stderr, level );
                    currentColumn++;
                break;

                case 'M': /* Message */
                    va_start( vargs, lpszFmt );
                    writeMessageToStream( stderr, lpszFmt, vargs );
                    va_end( vargs );
                    currentColumn++;
                break;

                case 'T': /* Timestamp */
                    writeTimeToStream( stderr );
                    currentColumn++;
                break;

                default:
                    handledFormat = 0;
            }

            /* Add separator chars */
            if( handledFormat && (currentColumn != numColumns) )
                fprintf( stderr, " | " );
        }

        fprintf( stderr, "\n" );
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
            /* Count number of columns inorder to skip last '|' char */
            for( i = 0, numColumns = 0; i < (int)strlen( logfileFormat ); i++ ) {
                switch( logfileFormat[i] ) {
                    case 'P':
                    case 'L':
                    case 'M':
                    case 'T':
                        numColumns++;
                    break;
                }
            }
    
            for( i = 0, currentColumn = 0; i < (int)strlen( logfileFormat ); i++ ) {
                handledFormat = 1;
    
                switch( logfileFormat[i] ) {
                    case 'P': /* Prefix */
                        writeHeaderToStream( logfileFP, source_id );
                        currentColumn++;
                    break;
    
                    case 'L': /* Loglevel */
                        writeLevelToStream( logfileFP, level );
                        currentColumn++;
                    break;
    
                    case 'M': /* Message */
                        va_start( vargs, lpszFmt );
                        writeMessageToStream( logfileFP, lpszFmt, vargs );
                        va_end( vargs );
                        currentColumn++;
                    break;
    
                    case 'T': /* Timestamp */
                        writeTimeToStream( logfileFP );
                        currentColumn++;
                    break;
    
                    default:
                        handledFormat = 0;
                }
    
                /* Add separator chars */
                if( handledFormat && (currentColumn != numColumns) )
                    fprintf( logfileFP, " | " );
            }
    
            fprintf( logfileFP, "\n" );
    
            fclose( logfileFP );
        }
    }

    /* Loginfo/Eventlog if levels match (not by format timecodes/status allready exists in evenlog) */
    if( level >= currentLoginfoLevel ) {
        va_list		vargs;

        va_start( vargs,lpszFmt );
        vsprintf( szBuff, lpszFmt, vargs );
        va_end( vargs );

        sendEventlogMessage( source_id, level, szBuff );
        sendLoginfoMessage( source_id, level, szBuff );
    }
}

/* Internal functions */

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
    char header[16];
    char **strings = (char **) malloc( 3 * sizeof( char * ) );
    WORD   eventType;
    HANDLE handle;
    WORD   eventID, categoryID;
    int result;

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
        printf("ReportEvent failed errno(%d)\n", GetLastError());
    }

    DeregisterEventSource( handle );

    free( (void *) strings );
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

void writeTimeToStream( FILE *fp ) {
    time_t		now;
    struct tm	*nowTM;

    /* Build a timestamp */
    now = time( NULL );
    nowTM = localtime( &now );

    /* Write timestamp */
    fprintf( fp, "%04d/%02d/%02d %02d:%02d:%02d", 
    nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday, 
    nowTM->tm_hour, nowTM->tm_min, nowTM->tm_sec );
}

void writeHeaderToStream( FILE *fp, int source_id ) {
    char header[16];

    /* Build the source header */
    switch( source_id ) {
        case WRAPPER_SOURCE_WRAPPER:
            sprintf( header, "wrapper " );
        break;

        case WRAPPER_SOURCE_PROTOCOL:
            sprintf( header, "wrapperp" );
        break;

        default:
            sprintf( header, "jvm %-4d", source_id );
        break;
    }

    fprintf( fp, "%s", header );
}

void writeLevelToStream( FILE *fp, int level ) {
    fprintf( fp, "%s", logLevelNames[ level ] );
}

void writeMessageToStream( FILE *fp, char *lpszFmt, va_list vargs ) {
    vfprintf( fp, lpszFmt, vargs );
}

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
            if ((tmpLogFilePathOld = (char *)malloc(((int)strlen(logFilePath)) + 10 + 2)) == NULL) {
                /* Don't log this as with other errors as that would cause recursion. */
                fprintf(stderr, "Out of memory.\n");
                goto cleanup;
            }
            if ((tmpLogFilePathNew = (char *)malloc(((int)strlen(logFilePath)) + 10 + 2)) == NULL) {
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
            }
            if (tmpLogFilePathNew != NULL) {
                free((void *)tmpLogFilePathNew);
            }
        }
    }
}
