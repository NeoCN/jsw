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
 * Revision 1.11  2003/08/02 15:50:03  mortenson
 * Implement getLastErrorText on UNIX versions.
 *
 * Revision 1.10  2003/07/04 03:18:36  mortenson
 * Improve the error message displayed when the NT EventLog is full in response
 * to feature request #643617.
 *
 * Revision 1.9  2003/04/03 04:05:22  mortenson
 * Fix several typos in the docs.  Thanks to Mike Castle.
 *
 * Revision 1.8  2003/02/07 16:05:27  mortenson
 * Implemented feature request #676599 to enable the filtering of JVM output to
 * trigger JVM restarts or Wrapper shutdowns.
 *
 * Revision 1.7  2003/02/03 06:55:26  mortenson
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

#ifndef _LOGGER_H
#define _LOGGER_H

/* * * Log source constants * * */

#define WRAPPER_SOURCE_WRAPPER -1
#define WRAPPER_SOURCE_PROTOCOL -2

#define MAX_LOG_SIZE 4096

/* * * Log level constants * * */

/* No logging at all. */
#define LEVEL_NONE   7

/* Too many restarts, unable to start etc. Case when the Wrapper is forced to exit. */
#define LEVEL_FATAL  6

/* JVM died, hung messages */
#define LEVEL_ERROR  5

/* Warning messages. */
#define LEVEL_WARN   4

/* Started, Stopped, Restarted messages. */
#define LEVEL_STATUS 3

/* Copyright message. and logged console output. */
#define LEVEL_INFO   2

/* Current debug messages */
#define LEVEL_DEBUG  1

/* Unknown level */
#define LEVEL_UNKNOWN  0

/* * * Function predeclaration * * */
extern int strcmpIgnoreCase( const char *str1, const char *str2 );

/* * Logfile functions * */
extern void setLogfilePath( char *log_file_path );
extern void setLogfileFormat( char *log_file_format );
extern void setLogfileLevelInt( int log_file_level );
extern int getLogfileLevelInt();
extern void setLogfileLevel( char *log_file_level );
extern void setLogfileMaxFileSize( char *max_file_size );
extern void setLogfileMaxFileSizeInt( int max_file_size );
extern void setLogfileMaxLogFiles( char *max_log_files );
extern void setLogfileMaxLogFilesInt( int max_log_files );

/* * Console functions * */
extern void setConsoleLogFormat( char *console_log_format );
extern void setConsoleLogLevelInt( int console_log_level );
extern int getConsoleLogLevelInt();
extern void setConsoleLogLevel( char *console_log_level );

/* * Syslog/eventlog functions * */
extern void setSyslogLevelInt( int loginfo_level );
extern int getSyslogLevelInt();
extern void setSyslogLevel( char *loginfo_level );
extern void setSyslogEventSourceName( char *event_source_name );
extern int registerSyslogMessageFile( );
extern int unregisterSyslogMessageFile( );

extern int getLowLogLevel();

/* * General log functions * */
extern void log_printf( int source_id, int level, char *lpszFmt, ... );

extern char* getLastErrorText();

#endif
