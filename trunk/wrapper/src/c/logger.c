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
// Revision 1.3  2002/01/27 15:02:45  spocke
// Fixed some Unix issues, so it compiles better.
//
// Revision 1.2  2002/01/26 23:31:03  spocke
// Added rolling file support to logger.
//
// Revision 1.1  2002/01/24 09:38:56  mortenson
// Added new Logger code contributed by Johan Sorlin
//

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
#include <string.h>

#ifdef WIN32
#include <windows.h>
#else
#include <syslog.h>
#endif

#include "logger.h"
#include "wrapper.h"

/* * Global data for logger * */

// Initialize all log levels to unknown until they are set
int currentConsoleLevel = LEVEL_UNKNOWN;
int currentLogfileLevel = LEVEL_UNKNOWN;
int currentLoginfoLevel = LEVEL_UNKNOWN;

char logFilePath[ 1024 ];
char *logLevelNames[] = { "NONE  ", "DEBUG ", "INFO  ", "STATUS", "ERROR ", "FATAL " };
char loginfoSourceName[ 1024 ];
int  logFileMaxSize = -1;
int  logFileMaxLogFiles = -1;

// * * Defualt formats (Must be 4 chars)
char consoleFormat[32];
char logfileFormat[32];

/* * Internal function declaration * */
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
#else // UNIX
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

/* * Logfile functions * */

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

void setLogfileLevel( char *log_file_level ) {
	setLogfileLevelInt(getLogLevelForName(log_file_level));
}

void setLogfileMaxFileSize( char *max_file_size ) {
	int multiple, i, newLength;
	char *tmpFileSizeBuff;
	char chr;

	if( max_file_size != NULL ) {
		/* * Allocate buffer * */
		if( (tmpFileSizeBuff = (char *) malloc( strlen( max_file_size ) + 1 )) == NULL )
			return;

		/* * Generate multiple and remove unwanted chars * */
		multiple = 0;
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
		if( logFileMaxSize != -1 )
			logFileMaxSize *= multiple;

		/* * Free memory * */
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

/* * Console functions * */
void setConsoleLogFormat( char *console_log_format ) {
	if( console_log_format != NULL )
		strcpy( consoleFormat, console_log_format );
}

void setConsoleLogLevelInt( int console_log_level ) {
	currentConsoleLevel = console_log_level;
}

void setConsoleLogLevel( char *console_log_level ) {
	setConsoleLogLevelInt(getLogLevelForName(console_log_level));
}

/* * Syslog/eventlog functions * */
void setSyslogLevelInt( int loginfo_level ) {
	currentLoginfoLevel = loginfo_level;
}

void setSyslogLevel( char *loginfo_level ) {
	setSyslogLevelInt(getLogLevelForName(loginfo_level));
}

void setSyslogEventSourceName( char *event_source_name ) {
	if( event_source_name != NULL )
		strcpy( loginfoSourceName, event_source_name );
}

int loggerNeedsDebug() {
	if ((currentLogfileLevel <= LEVEL_DEBUG) ||
		(currentConsoleLevel <= LEVEL_DEBUG) ||
		(currentLoginfoLevel <= LEVEL_DEBUG)) {
		return 1;
	} else {
		return 0;
	}
}

/* * General log functions * */
void log_printf( int source_id, int level, char *lpszFmt, ... ) {
	va_list		vargs;
	int			i;
	char		szBuff[2048];
	int			handledFormat, numColumns, currentColumn;
	FILE		*logfileFP = NULL;

	// * * Console output by format
	if( level >= currentConsoleLevel ) {
		// * * Count number of columns inorder to skip last '|' char
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
				case 'P': // * * Prefix
					writeHeaderToStream( stderr, source_id );
					currentColumn++;
				break;

				case 'L': // * * Loglevel
					writeLevelToStream( stderr, level );
					currentColumn++;
				break;

				case 'M': // * * Message
					va_start( vargs, lpszFmt );
					writeMessageToStream( stderr, lpszFmt, vargs );
					va_end( vargs );
					currentColumn++;
				break;

				case 'T': // * * Timestamp
					writeTimeToStream( stderr );
					currentColumn++;
				break;

				default:
					handledFormat = 0;
			}

			// * * Add separator chars
			if( handledFormat && (currentColumn != numColumns) )
				fprintf( stderr, " | " );
		}

		fprintf( stderr, "\n" );
	}

	// * * Logfile output by format
	checkAndRollLogs( );
	logfileFP = fopen( logFilePath, "a" );
	if( (level >= currentLogfileLevel) && (logfileFP != NULL) ) {
		// * * Count number of columns inorder to skip last '|' char
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
				case 'P': // * * Prefix
					writeHeaderToStream( logfileFP, source_id );
					currentColumn++;
				break;

				case 'L': // * * Loglevel
					writeLevelToStream( logfileFP, level );
					currentColumn++;
				break;

				case 'M': // * * Message
					va_start( vargs, lpszFmt );
					writeMessageToStream( logfileFP, lpszFmt, vargs );
					va_end( vargs );
					currentColumn++;
				break;

				case 'T': // * * Timestamp
					writeTimeToStream( logfileFP );
					currentColumn++;
				break;

				default:
					handledFormat = 0;
			}

			// * * Add separator chars
			if( handledFormat && (currentColumn != numColumns) )
				fprintf( logfileFP, " | " );
		}

		fprintf( logfileFP, "\n" );
	}
	fclose( logfileFP );

	// * * Loginfo/Eventlog if levels match (not by format timecodes/status allready exists in evenlog)
	if( level >= currentLoginfoLevel ) {
		va_list		vargs;

		va_start( vargs,lpszFmt );
		vsprintf( szBuff, lpszFmt, vargs );
		va_end( vargs );

		sendEventlogMessage( source_id, level, szBuff );
		sendLoginfoMessage( source_id, level, szBuff );
	}
}

/* * Internal functions * */

int registerSyslogMessageFile( ) {
#ifdef WIN32
	char buffer[ 1024 ];
	char regPath[ 1024 ];
	HKEY hKey;
	DWORD categoryCount, typesSupported;

	// * * Get absolute path to service manager
	if( GetModuleFileName( NULL, buffer, _MAX_PATH ) ) {
		sprintf( regPath, "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\%s", loginfoSourceName );

		if( RegCreateKey( HKEY_LOCAL_MACHINE, regPath, (PHKEY) &hKey ) == ERROR_SUCCESS ) {
			RegCloseKey( hKey );

			if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, regPath, 0, KEY_WRITE, (PHKEY) &hKey ) == ERROR_SUCCESS ) {
				// * * Set EventMessageFile
				if( RegSetValueEx( hKey, "EventMessageFile", (DWORD) 0, (DWORD) REG_SZ, (const unsigned char *) buffer, (strlen(buffer) + 1) ) != ERROR_SUCCESS ) {
					RegCloseKey( hKey );
					return -1;
				}

				// * * Set CategoryMessageFile
				if( RegSetValueEx( hKey, "CategoryMessageFile", (DWORD) 0, (DWORD) REG_SZ, (const unsigned char *) buffer, (strlen(buffer) + 1) ) != ERROR_SUCCESS ) {
					RegCloseKey( hKey );
					return -1;
				}

				// * * Set CategoryCount
				categoryCount = 12;
				if( RegSetValueEx( hKey, "CategoryCount", (DWORD) 0, (DWORD) REG_DWORD, (LPBYTE) &categoryCount, sizeof(DWORD) ) != ERROR_SUCCESS ) {
					RegCloseKey( hKey );
					return -1;
				}

				// * * Set TypesSupported
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

	return -1; // Failure
#else
	return 0;
#endif
}

int unregisterSyslogMessageFile( ) {
#ifdef WIN32
	/*
	// If we deregister this application, then the event viewer will not work when the program is not running.
	// Don't want to clutter up the Registry, but is there another way?  
	char regPath[ 1024 ];

	// * * Get absolute path to service manager
	sprintf( regPath, "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\%s", loginfoSourceName );

	if( RegDeleteKey( HKEY_LOCAL_MACHINE, regPath ) == ERROR_SUCCESS )
		return 0;

	return -1; // Failure
	*/
	return 0;
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

    // * * Build the source header
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

	// * * Build event type by level
	switch( level ) {
		case LEVEL_FATAL:
			eventType = EVENTLOG_ERROR_TYPE;
		break;

		case LEVEL_ERROR:
			eventType = EVENTLOG_WARNING_TYPE;
		break;

		case LEVEL_STATUS:
		case LEVEL_INFO:
		case LEVEL_DEBUG:
			eventType = EVENTLOG_INFORMATION_TYPE;
		break;
	}

	// * * Handle categories
	categoryID = 10; // jvmxx
	if( source_id == WRAPPER_SOURCE_WRAPPER )
		categoryID = 11; // wrapper
	else if( source_id == WRAPPER_SOURCE_PROTOCOL )
		categoryID = 12; // wrapperp
	else if( (source_id >= 1) && (source_id <= 9) )
		categoryID = source_id; // jvm1-9

	// * * Place event in eventlog
	strings[0] = header;
	strings[1] = szBuff;
	strings[2] = 0;
	eventID = level;

	handle = RegisterEventSource( 0, loginfoSourceName );
	if( !handle )
		return;

	result = ReportEvent(
		handle,                   // handle to event log
		eventType,                // event type
		categoryID,				  // event category
		100,                      // event identifier
		0,                        // user security identifier
		2,                        // number of strings to merge
		0,                        // size of binary data
		(const char **) strings,  // array of strings to merge
		0                         // binary data buffer
	);
	if (result == 0) {
		printf("ReportEvent failed errno(%d)\n", GetLastError());
	}

	DeregisterEventSource( handle );

	free( (void *) strings );
#endif
}

void sendLoginfoMessage( int source_id, int level, char *szBuff ) {
#ifndef WIN32 // If UNIX
	int eventType;

	// * * Build event type by level
	switch( level ) {
		case LEVEL_FATAL:
			eventType = LOG_CRIT;
		break;

		case LEVEL_ERROR:
			eventType = LOG_ERR;
		break;

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

    // * * Build a timestamp
    now = time( NULL );
    nowTM = localtime( &now );

	// * * Write timestamp
	fprintf( fp, "%04d/%02d/%02d %02d:%02d:%02d", 
    nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday, 
    nowTM->tm_hour, nowTM->tm_min, nowTM->tm_sec );
}

void writeHeaderToStream( FILE *fp, int source_id ) {
	char header[16];

    // * * Build the source header
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

void checkAndRollLogs( ) {
	FILE *fp;
	int fileSize;
	char *tmpLogFilePath1;
	char *tmpLogFilePath2;
	int i;

	if( (logFileMaxSize <= -1) || (logFileMaxLogFiles <= -1) )
		return;

	/* * Allocate buffers * */
	tmpLogFilePath1 = (char *) malloc( ((int) strlen( logFilePath )) + 10 );
	tmpLogFilePath2 = (char *) malloc( ((int) strlen( logFilePath )) + 10 );

	/* * Check if the allocation was successful * */
	if( (tmpLogFilePath1 == NULL) || (tmpLogFilePath2 == NULL) ) {
		free( (void *) tmpLogFilePath1 );
		free( (void *) tmpLogFilePath2 );
		return;
	}

	if( (fp = fopen( logFilePath, "rb" )) != NULL ) {
		/* * Get file size and close file * */
		fseek( fp, 0, SEEK_END );
		fileSize = ftell( fp );
		fclose( fp );

		/* * Is it time to roll them * */
		if( fileSize > logFileMaxSize ) {
			/* * Loop and rename them * */
			for( i=logFileMaxLogFiles; i>=0; i-- ) {
				sprintf( tmpLogFilePath1, "%s.%d", logFilePath, i );
				sprintf( tmpLogFilePath2, "%s.%d", logFilePath, i + 1 );

				rename( tmpLogFilePath1, tmpLogFilePath2 );
			}

			/* * Rename base file to first index * */
			sprintf( tmpLogFilePath1, "%s", logFilePath );
			sprintf( tmpLogFilePath2, "%s.1", logFilePath );
			rename( tmpLogFilePath1, tmpLogFilePath2 );

			/* * Remove the last file * */
			sprintf( tmpLogFilePath1, "%s.%d", logFilePath, logFileMaxLogFiles + 1 );
			remove( tmpLogFilePath1 );
		}
	}

	/* * Free memory * */
	free( (void *) tmpLogFilePath1 );
	free( (void *) tmpLogFilePath2 );
}
