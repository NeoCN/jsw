/*
 * Copyright (c) 1999, 2012 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html
 */

/**
 * Author:
 *   Leif Mortenson <leif@tanukisoftware.com>
 */

#ifndef _WRAPPER_FILE_H
#define _WRAPPER_FILE_H

#ifdef WIN32
#include <tchar.h>
#else
#include "wrapper_i18n.h"
#endif

/*#define WRAPPER_FILE_DEBUG*/

#define WRAPPER_FILE_SORT_MODE_TIMES 100
#define WRAPPER_FILE_SORT_MODE_NAMES_ASC 101
#define WRAPPER_FILE_SORT_MODE_NAMES_DEC 102

/**
 * Returns a valid sort mode given a name: "TIMES", "NAMES_ASC", "NAMES_DEC".
 *  In the event of an invalid value, TIMES will be returned.
 */
extern int wrapperFileGetSortMode(const TCHAR *modeName);

/**
 * Returns a NULL terminated list of file names within the specified pattern.
 *  The files will be sorted new to old for TIMES.  Then incremental ordering
 *  for NAMES.  The numeric components of the names will be treated as
 *  numbers and sorted accordingly.
 */
extern TCHAR** wrapperFileGetFiles(const TCHAR* pattern, int sortMode);

/**
 * Frees the array of file names returned by wrapperFileGetFiles()
 */
extern void wrapperFileFreeFiles(TCHAR** files);

/**
 * Tests whether a file exists.
 *
 * @return TRUE if exists, FALSE otherwise.
 */
extern int wrapperFileExists(const TCHAR * filename);

#ifdef WIN32
extern int wrapperGetUNCFilePath(const TCHAR *path, int advice);
#endif

#ifdef WRAPPER_FILE_DEBUG
extern void wrapperFileTests();
#endif

/**
 * Read configuration file.
 */
typedef int (*ConfigFileReader_Callback)(void *param, const TCHAR *fileName, int lineNumber, TCHAR *config, int debugProperties);

typedef struct ConfigFileReader ConfigFileReader;
struct ConfigFileReader {
    ConfigFileReader_Callback callback;
    void *callbackParam;
    int enableIncludes;
    int debugIncludes;
    int debugProperties;
    int preload;
};

/**
 * Initialize `reader'
 */
extern void configFileReader_Initialize(ConfigFileReader *reader,
					ConfigFileReader_Callback callback,
					void *callbackParam,
					int enableIncludes);

/**
 * Reads configuration lines from the file `filename' and calls
 * `reader->callback' with the line and `reader->callbackParam'
 * specified to its arguments.
 *
 * @param reader ConfigFileReader instance
 * @param filename Name of configuration file to read
 * @param fileRequired Requires the existence of `filename'
 * @param depth Inclusion depth
 * @param parentFilename Name of the file which includes `filename'
 * @param parentLineNumber 
 *
 * @return CONFIG_FILE_READER_SUCCESS if the file was read successfully,
 *         CONFIG_FILE_READER_FAIL if there were any problems at all, or
 *         CONFIG_FILE_READER_HARD_FAIL if the problem should cascaded all the way up.
 */
#define CONFIG_FILE_READER_SUCCESS   101
#define CONFIG_FILE_READER_FAIL      102
#define CONFIG_FILE_READER_HARD_FAIL 103

extern int configFileReader_Read(ConfigFileReader *reader,
				const TCHAR *filename,
				int fileRequired,
				int depth,
				const TCHAR *parentFilename,
				int parentLineNumber);

#endif

