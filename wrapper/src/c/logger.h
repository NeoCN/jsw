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
 * Revision 1.4  2002/03/07 09:23:25  mortenson
 * Go through and change the style of comments that we use so that they will not
 * cause compiler errors on older unix compilers.
 *
 * Revision 1.3  2002/02/08 05:55:55  mortenson
 * Make the syslog never unregister to avoid EventLog errors.
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

#ifndef _LOGGER_H
#define _LOGGER_H

/* * * Log source constants * * */

#define WRAPPER_SOURCE_WRAPPER -1
#define WRAPPER_SOURCE_PROTOCOL -2

/* * * Log level constants * * */

/* No logging at all. */
#define LEVEL_NONE   6

/* Too many restarts, unable to start etc. Case when the Wrapper is forced to exit. */
#define LEVEL_FATAL  5

/* JVM died, hung messages */
#define LEVEL_ERROR  4

/* Started, Stopped, Restarted messages. */
#define LEVEL_STATUS 3

/* Copyright message. and logged console output. */
#define LEVEL_INFO   2

/* Current debug messages */
#define LEVEL_DEBUG  1

/* Unknown level */
#define LEVEL_UNKNOWN  0

/* * * Function predeclaration * * */

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

extern int loggerNeedsDebug();

/* * General log functions * */
extern void log_printf( int source_id, int level, char *lpszFmt, ... );

#endif
