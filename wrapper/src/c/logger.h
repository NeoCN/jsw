/*
 * Copyright (c) 1999, 2005 Tanuki Software Inc.
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of the Java Service Wrapper and associated
 * documentation files (the "Software"), to deal in the Software
 * without  restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sub-license,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
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
 * 
 *
 * $Log$
 * Revision 1.24  2005/11/07 07:04:52  mortenson
 * Make it possible to configure the umask for all files created by the Wrapper and
 * that of the JVM.
 *
 * Revision 1.23  2005/05/23 02:37:54  mortenson
 * Update the copyright information.
 *
 * Revision 1.22  2004/10/20 07:55:35  mortenson
 * Make sure that the logfile is flushed in a timely manner rather than leaving
 * it entirely up to the OS.
 *
 * Revision 1.21  2004/10/19 11:48:20  mortenson
 * Rework logging so that the logfile is kept open.  Results in a 4 fold speed increase.
 *
 * Revision 1.20  2004/09/16 04:04:32  mortenson
 * Close the Handle to the logging mutex on shutdown.
 *
 * Revision 1.19  2004/08/06 16:17:04  mortenson
 * Added a new wrapper.java.command.loglevel property which makes it possible
 * to control the log level of the generated java command.
 *
 * Revision 1.18  2004/07/05 07:43:53  mortenson
 * Fix a deadlock on solaris by being very careful that we never perform any direct
 * logging from within a signal handler.
 *
 * Revision 1.17  2004/06/06 15:28:05  mortenson
 * Fix a synchronization problem in the logging code which would
 * occassionally cause the Wrapper to crash with an Access Violation.
 * The problem was only encountered when the tick timer was enabled,
 * and was only seen on multi-CPU systems.  Bug #949877.
 *
 * Revision 1.16  2004/03/20 16:55:49  mortenson
 * Add an adviser feature to help cut down on support requests from new users.
 *
 * Revision 1.15  2004/01/16 04:41:58  mortenson
 * The license was revised for this version to include a copyright omission.
 * This change is to be retroactively applied to all versions of the Java
 * Service Wrapper starting with version 3.0.0.
 *
 * Revision 1.14  2004/01/09 18:31:36  mortenson
 * define the DWORD symbol so it can used.
 *
 * Revision 1.13  2004/01/09 17:49:00  mortenson
 * Rework the logging so it is now threadsafe.
 *
 * Revision 1.12  2003/10/30 19:34:34  mortenson
 * Added a new wrapper.ntservice.console property so the console can be shown for
 * services.
 * Fixed a problem where requesting thread dumps on exit was failing when running
 * as a service.
 *
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

#ifdef WIN32
#include <windows.h>
#endif

#ifndef DWORD
#define DWORD unsigned long
#endif

/* * * Log source constants * * */

#define WRAPPER_SOURCE_WRAPPER -1
#define WRAPPER_SOURCE_PROTOCOL -2

/* * * Log thread constants * * */
/* These are indexes in an array so they must be sequential, start
 *  with zero and be one less than the final WRAPPER_THREAD_COUNT */
#define WRAPPER_THREAD_SIGNAL   0
#define WRAPPER_THREAD_MAIN     1
#define WRAPPER_THREAD_SRVMAIN  2
#define WRAPPER_THREAD_TIMER    3
#define WRAPPER_THREAD_COUNT    4

#define MAX_LOG_SIZE 4096

/* * * Log level constants * * */

/* No logging at all. */
#define LEVEL_NONE   8

/* Advisor messages which should always be displayed.  These never go to the syslog. */
#define LEVEL_ADVICE 7

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

#ifdef WIN32
extern void setConsoleStdoutHandle( HANDLE stdoutHandle );
#endif

/* * * Function predeclaration * * */
extern int strcmpIgnoreCase( const char *str1, const char *str2 );

/* * Logfile functions * */
extern void setLogfilePath( char *log_file_path );
extern void setLogfileUmask( int log_file_umask );
extern void setLogfileFormat( char *log_file_format );
extern void setLogfileLevelInt( int log_file_level );
extern int getLogfileLevelInt();
extern void setLogfileLevel( char *log_file_level );
extern void setLogfileMaxFileSize( char *max_file_size );
extern void setLogfileMaxFileSizeInt( int max_file_size );
extern void setLogfileMaxLogFiles( char *max_log_files );
extern void setLogfileMaxLogFilesInt( int max_log_files );
extern DWORD getLogfileActivity();
extern void closeLogfile();
extern void setLogfileAutoClose(int autoClose);
extern void flushLogfile();

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
extern int initLogging();
extern int disposeLogging();
extern int getLogLevelForName( const char *logLevelName );
extern void logRegisterThread( int thread_id );
extern void log_printf( int source_id, int level, char *lpszFmt, ... );
extern void log_printf_queue( int useQueue, int source_id, int level, char *lpszFmt, ... );

extern char* getLastErrorText();
extern int getLastError();
extern void maintainLogger();

#endif
