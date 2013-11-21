/*
 * Copyright (c) 1999, 2013 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html
 */

#ifndef _LOGGER_BASE_H
#define _LOGGER_BASE_H

#include "wrapper_i18n.h"

/* * * Log level constants * * */

/* No logging at all. */
#define LEVEL_NONE   9

/* Notice messages which should always be displayed.  These never go to the syslog. */
#define LEVEL_NOTICE 8

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

/* The maximum length of a source string, not including the null character. */
#define MAX_SOURCE_LENGTH 8

/* * * Function predeclaration * * */
#define strcmpIgnoreCase(str1, str2) _tcsicmp(str1, str2)

extern TCHAR* getLastErrorText();

extern int getLastError();

extern void outOfMemory(const TCHAR *context, int id);

#endif
