/*
 * Copyright (c) 1999, 2010 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.org/doc/english/licenseOverview.html
 */

/**
 * Author:
 *   Leif Mortenson <leif@tanukisoftware.com>
 */

#ifndef _WRAPPER_FILE_H
#define _WRAPPER_FILE_H

/*#define WRAPPER_FILE_DEBUG*/

#define WRAPPER_FILE_SORT_MODE_TIMES 100
#define WRAPPER_FILE_SORT_MODE_NAMES_ASC 101
#define WRAPPER_FILE_SORT_MODE_NAMES_DEC 102

/**
 * Returns a valid sort mode given a name: "TIMES", "NAMES_ASC", "NAMES_DEC".
 *  In the event of an invalid value, TIMES will be returned.
 */
extern int wrapperFileGetSortMode(const char *modeName);

/**
 * Returns a NULL terminated list of file names within the specified pattern.
 *  The files will be sorted new to old for TIMES.  Then incremental ordering
 *  for NAMES.  The numeric components of the names will be treated as
 *  numbers and sorted accordingly.
 */
extern char** wrapperFileGetFiles(const char* pattern, int sortMode);

/**
 * Frees the array of file names returned by wrapperFileGetFiles()
 */
extern void wrapperFileFreeFiles(char** files);

/**
 * @param path to check.
 * @param advice 0 if advice should be displayed.
 *
 * @return advice or advice + 1 if advice was logged.
 */
extern int wrapperGetUNCFilePath(const char *path, int advice);

#ifdef WRAPPER_FILE_DEBUG
extern void wrapperFileTests();
#endif

#endif
