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

/**
 * Author:
 *   Leif Mortenson <leif@tanukisoftware.com>
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <errno.h>
#include <tchar.h>
#include <io.h>
#else
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <unistd.h>
#include <limits.h>
#include <langinfo.h>
#if defined(IRIX)
#define PATH_MAX FILENAME_MAX
#endif
#endif

#include "wrapper_file.h"
#include "logger.h"
#include "wrapper_i18n.h"
#include "wrapper.h"

#define FILES_CHUNK 5

#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MAX_INCLUDE_DEPTH 10

/**
 * Returns a valid sort mode given a name: "TIMES", "NAMES_ASC", "NAMES_DEC".
 *  In the event of an invalid value, TIMES will be returned.
 */
int wrapperFileGetSortMode(const TCHAR *modeName) {
    if (strcmpIgnoreCase(modeName, TEXT("NAMES_ASC")) == 0) {
        return WRAPPER_FILE_SORT_MODE_NAMES_ASC;
    } else if (strcmpIgnoreCase(modeName, TEXT("NAMES_DEC")) == 0) {
        return WRAPPER_FILE_SORT_MODE_NAMES_DEC;
    } else {
        return WRAPPER_FILE_SORT_MODE_TIMES;
    }
}


#ifdef WIN32
int sortFilesTimes(TCHAR **files, __time64_t *fileTimes, int cnt) {
#else
int sortFilesTimes(TCHAR **files, time_t *fileTimes, int cnt) {
#endif
    int i, j;
    TCHAR *temp;
#ifdef WIN32
    __time64_t tempTime;
#else
    time_t tempTime;
#endif

    for (i = 0; i < cnt; i++) {
        for (j = 0; j < cnt - 1; j++) {
            if (fileTimes[j] < fileTimes[j + 1]) {
                temp = files[j + 1];
                tempTime = fileTimes[j + 1];

                files[j + 1] = files[j];
                fileTimes[j + 1] = fileTimes[j];

                files[j] = temp;
                fileTimes[j] = tempTime;
            }
        }
    }

    return TRUE;
}

/**
 * Compares two strings.  Returns 0 if they are equal, -1 if file1 is bigger, 1 if file2 is bigger.
 */
int compareFileNames(const TCHAR *file1, const TCHAR *file2) {
    int pos1, pos2;
    TCHAR c1, c2;
    int numeric1, numeric2;
    long int num1, num2;
    int afterNumber = FALSE;

    pos1 = 0;
    pos2 = 0;

    while (TRUE) {
        c1 = file1[pos1];
        c2 = file2[pos2];
        /*printf("     file1[%d]=%d, file2[%d]=%d\n", pos1, c1, pos2, c2);*/

        /* Did we find the null. */
        if (c1 == 0) {
            if (c2 == 0) {
                return 0;
            } else {
                return 1;
            }
        } else {
            if (c2 == 0) {
                return -1;
            } else {
                /* Continue. */
            }
        }

        /* We have two characters. */
        numeric1 = (c1 >= TEXT('0') && c1 <= TEXT('9'));
        numeric2 = (c2 >= TEXT('0') && c2 <= TEXT('9'));

        /* See if one or both of the strings is numeric. */
        if (numeric1) {
            if (numeric2) {
                /* Both are numeric, we need to start comparing the two file names as integer values. */
                num1 = c1 - TEXT('0');
                c1 = file1[pos1 + 1];
                while (c1 >= TEXT('0') && c1 <= TEXT('9')) {
                    num1 = num1 * 10 + (c1 - TEXT('0'));
                    pos1++;
                    c1 = file1[pos1 + 1];
                }

                num2 = c2 - TEXT('0');
                c2 = file2[pos2 + 1];
                while (c2 >= TEXT('0') && c2 <= TEXT('9')) {
                    num2 = num2 * 10 + (c2 - TEXT('0'));
                    pos2++;
                    c2 = file2[pos2 + 1];
                }

                /*printf("     num1=%ld, num2=%ld\n", num1, num2);*/
                if (num1 > num2) {
                    return -1;
                } else if (num2 > num1) {
                    return 1;
                } else {
                    /* Equal, continue. */
                }
                afterNumber = TRUE;
            } else {
                /* 1 is numeric, 2 is not. */
                if (afterNumber) {
                    return -1;
                } else {
                    return 1;
                }
            }
        } else {
            if (numeric2) {
                /* 1 is not, 2 is numeric. */
                if (afterNumber) {
                    return 1;
                } else {
                    return -1;
                }
            } else {
                /* Neither is numeric. */
            }
        }

        /* Compare the characters as is. */
        if (c1 > c2) {
            return -1;
        } else if (c2 > c1) {
            return 1;
        } else {
            /* Equal, continue. */
            if (c1 == TEXT('.') || c1 == TEXT('-') || c1 == TEXT('_')) {
            } else {
                afterNumber = FALSE;
            }
        }

        pos1++;
        pos2++;
    }
}

int sortFilesNamesAsc(TCHAR **files, int cnt) {
    int i, j;
    TCHAR *temp;
    int cmp;

    for (i = 0; i < cnt; i++) {
        for (j = 0; j < cnt - 1; j++) {
            cmp = compareFileNames(files[j], files[j+1]);
            if (cmp < 0) {
                temp = files[j + 1];
                files[j + 1] = files[j];
                files[j] = temp;
            }
        }
    }

    return TRUE;
}

int sortFilesNamesDec(TCHAR **files, int cnt) {
    int i, j;
    TCHAR *temp;
    int cmp;

    for (i = 0; i < cnt; i++) {
        for (j = 0; j < cnt - 1; j++) {
            cmp = compareFileNames(files[j], files[j+1]);
            if (cmp > 0) {
                temp = files[j + 1];
                files[j + 1] = files[j];
                files[j] = temp;
            }
        }
    }

    return TRUE;
}

/**
 * Returns a NULL terminated list of file names within the specified pattern.
 *  The files will be sorted new to old for TIMES.  Then incremental ordering
 *  for NAMES.  The numeric components of the names will be treated as
 *  numbers and sorted accordingly.
 */
TCHAR** wrapperFileGetFiles(const TCHAR* pattern, int sortMode) {
    int cnt;
    int filesSize;
    TCHAR **files;
#ifdef WIN32
    int i;
    size_t dirLen;
    TCHAR *c;
    TCHAR *dirPart;
    intptr_t handle;
    struct _tfinddata64_t fblock;
    size_t fileLen;
    TCHAR **newFiles;
    __time64_t *fileTimes;
    __time64_t *newFileTimes;
#else
#ifdef WRAPPER_FILE_DEBUG
    int i;
#endif
    int result;
    glob_t g;
    int findex;
    time_t *fileTimes;
    struct stat fileStat;
#endif

#ifdef WRAPPER_FILE_DEBUG
    _tprintf(TEXT("wrapperFileGetFiles(%s, %d)\n"), pattern, sortMode);
#endif

#ifdef WIN32
    cnt = 0;
    /* Initialize the files array. */
    filesSize = FILES_CHUNK;
    files = malloc(sizeof(TCHAR *) * filesSize);
    if (!files) {
        outOfMemoryQueued(TEXT("WFGF"), 1);
        return NULL;
    }
    memset(files, 0, sizeof(TCHAR *) * filesSize);

    fileTimes = malloc(sizeof(__time64_t) * filesSize);
    if (!fileTimes) {
        outOfMemoryQueued(TEXT("WFGF"), 2);
        free(files);
        return NULL;
    }
    memset(fileTimes, 0, sizeof(__time64_t) * filesSize);

    /* Extract any path information from the beginning of the file */
    c = max(_tcsrchr(pattern, TEXT('\\')), _tcsrchr(pattern, TEXT('/')));
    if (c == NULL) {
        /* No directory component */
        dirPart = malloc(sizeof(TCHAR) * 1);
        if (!dirPart) {
            outOfMemoryQueued(TEXT("WFGF"), 3);
            return NULL;
        }
        dirPart[0] = TEXT('\0');
        dirLen = 0;
    } else {
        /* extract the directory. */
        dirLen = c - pattern + 1;
        dirPart = malloc(sizeof(TCHAR) * (dirLen + 1));
        if (!dirPart) {
            outOfMemoryQueued(TEXT("WFGF"), 4);
            return NULL;
        }
        _tcsncpy(dirPart, pattern, dirLen);
        dirPart[dirLen] = TEXT('\0');
    }

#ifdef WRAPPER_FILE_DEBUG
    _tprintf(TEXT("  dirPart=[%s]\n"), dirPart);
#endif

    /* Get the first file. */
    if ((handle = _tfindfirst64(pattern, &fblock)) > 0) {
        if ((_tcscmp(fblock.name, TEXT(".")) != 0) && (_tcscmp(fblock.name, TEXT("..")) != 0)) {
            fileLen = _tcslen(fblock.name);
            files[cnt] = malloc((_tcslen(dirPart) + _tcslen(fblock.name) + 1) * sizeof(TCHAR));
            if (!files[cnt]) {
                outOfMemoryQueued(TEXT("WFGF"), 5);
                free(fileTimes);
                wrapperFileFreeFiles(files);
                free(dirPart);
                return NULL;
            }
            _sntprintf(files[cnt], _tcslen(dirPart) + _tcslen(fblock.name) + 1, TEXT("%s%s"), dirPart, fblock.name);
            fileTimes[cnt] = fblock.time_write;
#ifdef WRAPPER_FILE_DEBUG
            _tprintf(TEXT("  files[%d]=%s, %ld\n"), cnt, files[cnt], fileTimes[cnt]);
#endif

            cnt++;
        }

        /* Look for additional files. */
        while (_tfindnext64(handle, &fblock) == 0) {
            if ((_tcscmp(fblock.name, TEXT(".")) != 0) && (_tcscmp(fblock.name, TEXT("..")) != 0)) {
                /* Make sure we have enough room in the files array. */
                if (cnt >= filesSize - 1) {
                    newFiles = malloc(sizeof(TCHAR *) * (filesSize + FILES_CHUNK));
                    if (!newFiles) {
                        outOfMemoryQueued(TEXT("WFGF"), 6);
                        free(fileTimes);
                        wrapperFileFreeFiles(files);
                        free(dirPart);
                        return NULL;
                    }
                    memset(newFiles, 0, sizeof(TCHAR *) * (filesSize + FILES_CHUNK));
                    newFileTimes = malloc(sizeof(__time64_t) * (filesSize + FILES_CHUNK));
                    if (!newFileTimes) {
                        outOfMemoryQueued(TEXT("WFGF"), 7);
                        free(newFiles);
                        free(fileTimes);
                        wrapperFileFreeFiles(files);
                        free(dirPart);
                        return NULL;
                    }
                    memset(newFileTimes, 0, sizeof(__time64_t) * (filesSize + FILES_CHUNK));
                    
                    for (i = 0; i < filesSize; i++) {
                        newFiles[i] = files[i];
                        newFileTimes[i] = fileTimes[i];
                    }
                    free(files);
                    free(fileTimes);
                    files = newFiles;
                    fileTimes = newFileTimes;
                    filesSize += FILES_CHUNK;
#ifdef WRAPPER_FILE_DEBUG
                    _tprintf(TEXT("  increased files to %d\n"), filesSize);
#endif
                }

                fileLen = _tcslen(fblock.name);
                files[cnt] = malloc((_tcslen(dirPart) + _tcslen(fblock.name) + 1) * sizeof(TCHAR));
                if (!files[cnt]) {
                    outOfMemoryQueued(TEXT("WFGF"), 8);
                    free(fileTimes);
                    wrapperFileFreeFiles(files);
                    free(dirPart);
                    return NULL;
                }
                _sntprintf(files[cnt], _tcslen(dirPart) + _tcslen(fblock.name) + 1, TEXT("%s%s"), dirPart, fblock.name);
                fileTimes[cnt] = fblock.time_write;

#ifdef WRAPPER_FILE_DEBUG
                _tprintf(TEXT("  files[%d]=%s, %ld\n"), cnt, files[cnt], fileTimes[cnt]);
#endif
                cnt++;
            }
        }

        /* Close the file search */
        _findclose(handle);
    }

    if (cnt <= 0) {
        if (errno == ENOENT) {
            /* No files matched. */
#ifdef WRAPPER_FILE_DEBUG
            _tprintf(TEXT("  No files matched.\n"));
#endif
        } else {
            /* Encountered an error of some kind. */
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Error listing files, %s: %s"), pattern, getLastErrorText());
            free(fileTimes);
            wrapperFileFreeFiles(files);
            return NULL;
        }
    }
#else

#ifdef UNICODE
    char* cPattern;
    size_t req;

    req = wcstombs(NULL, pattern, 0) + 1;
    cPattern = malloc(req);
    if(!cPattern) {
        outOfMemoryQueued(TEXT("WFGF"), 8);
        return NULL;
    }
    wcstombs(cPattern, pattern, req);

    result = glob(cPattern, GLOB_MARK | GLOB_NOSORT, NULL, &g);
    free(cPattern);
#else
    result = glob(pattern, GLOB_MARK | GLOB_NOSORT, NULL, &g);
#endif
    cnt = 0;
    if (!result) {
        if (g.gl_pathc > 0) {
            filesSize = g.gl_pathc + 1;
            files = malloc(sizeof(TCHAR *) * filesSize);
            if (!files) {
                outOfMemoryQueued(TEXT("WFGF"), 9);
                return NULL;
            }
            memset(files, 0, sizeof(TCHAR *) * filesSize);
            
            fileTimes = malloc(sizeof(time_t) * filesSize);
            if (!fileTimes) {
                outOfMemoryQueued(TEXT("WFGF"), 10);
                wrapperFileFreeFiles(files);
                return NULL;
            }
            memset(fileTimes, 0, sizeof(time_t) * filesSize);

            for (findex = 0; findex < g.gl_pathc; findex++) {
#ifdef UNICODE
                req = mbstowcs(NULL, g.gl_pathv[findex], 0);
                if (req == (size_t)-1) {
                    invalidMultiByteSequence(TEXT("GLET"), 1);
                }
                files[cnt] = malloc((req + 1) * sizeof(TCHAR));
                if (!files[cnt]) {
                    outOfMemoryQueued(TEXT("WFGF"), 11);
                    free(fileTimes);
                    wrapperFileFreeFiles(files);
                    return NULL;
                }
                mbstowcs(files[cnt], g.gl_pathv[findex], req + 1);

#else
                files[cnt] = malloc((strlen(g.gl_pathv[findex]) + 1));
                if (!files[cnt]) {
                    outOfMemoryQueued(TEXT("WFGF"), 11);
                    free(fileTimes);
                    wrapperFileFreeFiles(files);
                    return NULL;
                }

                strncpy(files[cnt], g.gl_pathv[findex], strlen(g.gl_pathv[findex]) + 1);
#endif

                /* Only try to get the modified time if it is really necessary. */
                if (sortMode == WRAPPER_FILE_SORT_MODE_TIMES) {
                    if (!_tstat(files[cnt], &fileStat)) {
                        fileTimes[cnt] = fileStat.st_mtime;
                    } else {
                        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Failed to stat %s: %s"), files[cnt], getLastErrorText());
                    }
                }
#ifdef WRAPPER_FILE_DEBUG
                printf("  files[%d]=%s, %ld\n", cnt, files[cnt], fileTimes[cnt]);
#endif
                cnt++;
            }
        } else {
#ifdef WRAPPER_FILE_DEBUG
            printf("  No files matched.\n");
#endif
            /* No files, but we still need the array. */
            filesSize = 1;
            files = malloc(sizeof(TCHAR *) * filesSize);
            if (!files) {
                outOfMemoryQueued(TEXT("WFGF"), 12);
                return NULL;
            }
            memset(files, 0, sizeof(TCHAR *) * filesSize);
            
            fileTimes = malloc(sizeof(time_t) * filesSize);
            if (!fileTimes) {
                free(files);
                outOfMemoryQueued(TEXT("WFGF"), 13);
                return NULL;
            }
            memset(fileTimes, 0, sizeof(time_t) * filesSize);
        }

        globfree(&g);
    } else if (result == GLOB_NOMATCH) {
#ifdef WRAPPER_FILE_DEBUG
        _tprintf(TEXT("  No files matched.\n"));
#endif
        /* No files, but we still need the array. */
        filesSize = 1;
        files = malloc(sizeof(TCHAR *) * filesSize);
        if (!files) {
            outOfMemoryQueued(TEXT("WFGF"), 14);
            return NULL;
        }
        memset(files, 0, sizeof(TCHAR *) * filesSize);

        fileTimes = malloc(sizeof(time_t) * filesSize);
        if (!fileTimes) {
            free(files);
            outOfMemoryQueued(TEXT("WFGF"), 15);
            return NULL;
        }
        memset(fileTimes, 0, sizeof(time_t) * filesSize);
    } else {
        /* Encountered an error of some kind. */
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Error listing files, %s: %s"), pattern, getLastErrorText());
        return NULL;
    }
#endif
    
    if (sortMode == WRAPPER_FILE_SORT_MODE_TIMES) {
        if (!sortFilesTimes(files, fileTimes, cnt)) {
            /* Failed. Reported. */
            free(fileTimes);
            wrapperFileFreeFiles(files);
            return NULL;
        }
    } else if (sortMode == WRAPPER_FILE_SORT_MODE_NAMES_DEC) {
        if (!sortFilesNamesDec(files, cnt)) {
            /* Failed. Reported. */
            free(fileTimes);
            wrapperFileFreeFiles(files);
            return NULL;
        }
    } else { /* WRAPPER_FILE_SORT_MODE_NAMES_ASC */
        if (!sortFilesNamesAsc(files, cnt)) {
            /* Failed. Reported. */
            free(fileTimes);
            wrapperFileFreeFiles(files);
            return NULL;
        }
    }

#ifdef WRAPPER_FILE_DEBUG
    _tprintf(TEXT("  Sorted:\n"));
    for (i = 0; i < cnt; i++) {
        _tprintf(TEXT("  files[%d]=%s, %ld\n"), i, files[i], fileTimes[i]);
    }
    _tprintf(TEXT("wrapperFileGetFiles(%s, %d) END\n"), pattern, sortMode);
#endif

    free(fileTimes);

    return files;
}

/**
 * Frees the array of file names returned by wrapperFileGetFiles()
 */
void wrapperFileFreeFiles(TCHAR** files) {
    int i;

    i = 0;
    while (files[i]) {
        free(files[i]);
        i++;
    }
    free(files);
}

/**
 * Tests whether a file exists.
 *
 * @return TRUE if exists, FALSE otherwise.
 */
int wrapperFileExists(const TCHAR * filename) {
    FILE * file;
    if ((file = _tfopen(filename, TEXT("r")))) {
        fclose(file);
        return TRUE;
    }
    return FALSE;
}

/**
 * @param path to check.
 * @param advice 0 if advice should be displayed.
 *
 * @return advice or advice + 1 if advice was logged.
 */
int wrapperGetUNCFilePath(const TCHAR *path, int advice) {
#ifdef WIN32
    TCHAR drive[4];
    DWORD result;

    /* See if the path starts with a drive.  Some users use forward slashes in the paths. */
    if ((path != NULL) && (_tcslen(path) >= 3) && (path[1] == TEXT(':')) && ((path[2] == TEXT('\\')) || (path[2] == TEXT('/')))) {
        _tcsncpy(drive, path, 2);
        drive[2] = TEXT('\\');
        drive[3] = TEXT('\0');
        result = GetDriveType(drive);
        if (result == DRIVE_REMOTE) {
            if (advice == 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("The following path in your Wrapper configuration file is to a mapped Network\n  Drive.  Using mapped network drives is not recommeded as they will fail to\n  be resolved correctly under certain circumstances.  Please consider using\n  UNC paths (\\\\<host>\\<share>\\path). Additional refrences will be ignored.\n  Path: %s"), path);
                advice++;
            }
        } else if (result == DRIVE_NO_ROOT_DIR) {
            if (advice == 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("The following path in your Wrapper configuration file could not be resolved.\n  Please make sure the path exists.  If the path is a network share, it may be\n  that the current user is unable to resolve it.  Please consider using UNC\n  paths (\\\\<host>\\<share>\\path) or run the service as another user\n  (see wrapper.ntservice.account). Additional refrences will be ignored.\n  Path: %s"), path);
                advice++;
            }
        }
    }
#endif
    return advice;
}

#ifdef WRAPPER_FILE_DEBUG
void wrapperFileTests() {
    TCHAR** files;

    printf("Start wrapperFileTests\n");
    files = wrapperFileGetFiles((TEXT("../logs/*.log*"), WRAPPER_FILE_SORT_MODE_TIMES);
    if (files) {
        wrapperFileFreeFiles(files);
    }

    files = wrapperFileGetFiles(TEXT("../logs/*.log*"), WRAPPER_FILE_SORT_MODE_NAMES_ASC);
    if (files) {
        wrapperFileFreeFiles(files);
    }

    files = wrapperFileGetFiles(TEXT("../logs/*.log*"), WRAPPER_FILE_SORT_MODE_NAMES_DEC);
    if (files) {
        wrapperFileFreeFiles(files);
    }
    printf("End wrapperFileTests\n");
}
#endif


/**
 * Call functions in property.c temporarily.
 */
extern void evaluateEnvironmentVariables(const TCHAR *propertyValue, TCHAR *buffer, int bufferLength);
#ifdef WIN32
#define strIgnoreCaseCmp _stricmp
extern int getEncodingByName(char* encodingMB, int *encoding);
#else
#define strIgnoreCaseCmp strcasecmp
extern int getEncodingByName(char* encodingMB, char** encoding);
#endif

/**
 * Initialize `reader'
 */
void configFileReader_Initialize(ConfigFileReader *reader,
				 ConfigFileReader_Callback callback,
				 void *callbackParam,
				 int enableIncludes)
{
    reader->callback = callback;
    reader->callbackParam = callbackParam;
    reader->enableIncludes = enableIncludes;
    reader->debugIncludes = FALSE;
    reader->debugProperties = FALSE;
    reader->preload = FALSE;
}

/**
 * Read configuration file.
 */
int configFileReader_Read(ConfigFileReader *reader,
			  const TCHAR *filename,
			  int fileRequired,
			  int depth,
			  const TCHAR *parentFilename,
			  int parentLineNumber)
{
    FILE *stream;
    char bufferMB[MAX_PROPERTY_NAME_VALUE_LENGTH];
    TCHAR expBuffer[MAX_PROPERTY_NAME_VALUE_LENGTH];
    TCHAR *trimmedBuffer;
    size_t trimmedBufferLen;
    TCHAR *c;
    TCHAR *d;
    size_t i, j;
    size_t len;
    int quoted;
    TCHAR *absoluteBuffer;
    int hadBOM;
    int lineNumber;

    char *encodingMB;
#ifdef WIN32
    int encoding;
#else
    char* encoding;
    char* interumEncoding;
#endif
    int includeRequired;
    int readResult = CONFIG_FILE_READER_SUCCESS;
    int ret;
    TCHAR *bufferW;
#ifdef WIN32
    int size;
#endif

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("configFileReader_Read('%s', required %d, depth %d, parent '%s', number %d, debugIncludes %d, preload %d)"),
        filename, fileRequired, depth, (parentFilename ? parentFilename : TEXT("<NULL>")), parentLineNumber, reader->debugIncludes, reader->preload );
#endif

    /* Look for the specified file. */
    if ((stream = _tfopen(filename, TEXT("rb"))) == NULL) {
        /* Unable to open the file. */
        if (reader->debugIncludes || fileRequired) {
            if (depth > 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("%sIncluded configuration file not found: %s\n  Referenced from: %s (line %d)\n  Current working directory: %s"),
                    (reader->debugIncludes ? TEXT("  ") : TEXT("")), filename, parentFilename, parentLineNumber, wrapperData->originalWorkingDir);
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("Configuration file not found: %s\n  Current working directory: %s"), filename, wrapperData->originalWorkingDir);
            }
        } else {
#ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Configuration file not found: %s"), filename);
#endif
        }
        return CONFIG_FILE_READER_FAIL;
    }

    if (reader->debugIncludes) {
        if (!reader->preload) {
            if (depth > 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("  Reading included configuration file, %s"), filename);
            } else {
                /* Will not actually get here because the debug includes can't be set until it is loaded.
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("Reading configuration file, %s"), filename); */
            }
        }
    }

    /* Load in the first row of configurations to check the encoding. */
    if (fgets(bufferMB, MAX_PROPERTY_NAME_VALUE_LENGTH, stream)) {
        /* If the file starts with a BOM (Byte Order Marker) then we want to skip over it. */
        if ((bufferMB[0] == (char)0xef) && (bufferMB[1] == (char)0xbb) && (bufferMB[2] == (char)0xbf)) {
            i = 3;
            hadBOM = TRUE;
        } else {
            i = 0;
            hadBOM = FALSE;
        }

        /* Does the file start with "#encoding="? */
        if ((bufferMB[i++] == '#') && (bufferMB[i++] == 'e') && (bufferMB[i++] == 'n') && (bufferMB[i++] == 'c') &&
            (bufferMB[i++] == 'o') && (bufferMB[i++] == 'd') && (bufferMB[i++] == 'i') &&
            (bufferMB[i++] == 'n') && (bufferMB[i++] == 'g') && (bufferMB[i++] == '=')) {
            encodingMB = bufferMB + i;
            i = 0;
            while ((encodingMB[i] != ' ') && (encodingMB[i] != '\n') && (encodingMB[i]  != '\r')) {
               i++;
            }
            encodingMB[i] = '\0';

            if ((hadBOM) && (strIgnoreCaseCmp(encodingMB, "UTF-8") != 0)) {
            }
            if (getEncodingByName(encodingMB, &encoding) == TRUE) {
                fclose(stream);
                return CONFIG_FILE_READER_FAIL;
            }

        } else {
#ifdef WIN32
            encoding = GetACP();
#else 
            encoding = nl_langinfo(CODESET);
 #ifdef MACOSX
            if (strlen(encoding) == 0) {
                encoding = "UTF-8";
            }
 #endif
#endif
        }
    } else {
        /* Failed to read the first line of the file. */
#ifdef WIN32
        encoding = GetACP();
#else 
        encoding = nl_langinfo(CODESET);
 #ifdef MACOSX
        if (strlen(encoding) == 0) {
            encoding = "UTF-8";
        }
 #endif
#endif
    }
    fclose(stream);

    if ((stream = _tfopen(filename, TEXT("rb"))) == NULL) {
        /* Unable to open the file. */
        if (reader->debugIncludes || fileRequired) {
            if (depth > 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("%sIncluded configuration file, %s, was not found."), (reader->debugIncludes ? TEXT("  ") : TEXT("")), filename);
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("Configuration file, %s, was not found."), filename);
            }

        } else {
#ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Configuration file not found: %s"), filename);
#endif
        }
        return CONFIG_FILE_READER_FAIL;
    }

    /* Read all of the configurations */
    lineNumber = 1;
    do {
        c = (TCHAR*)fgets(bufferMB, MAX_PROPERTY_NAME_VALUE_LENGTH, stream);
        if (c != NULL) {
#ifdef WIN32
            ret = multiByteToWideChar(bufferMB, encoding, &bufferW, TRUE);
#else
            interumEncoding = nl_langinfo(CODESET);
 #ifdef MACOSX
            if (strlen(interumEncoding) == 0) {
                interumEncoding = "UTF-8";
            }
 #endif
            ret = multiByteToWideChar(bufferMB, encoding, interumEncoding, &bufferW, TRUE);
#endif
            if (ret) {
                if (bufferW) {
                    /* bufferW contains an error message. */
                    if (!reader->preload) {
                        if (depth > 0) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                                TEXT("%sIncluded configuration file, %s, contains a problem on line #%d and could not be read. (%s)"),
                                (reader->debugIncludes ? TEXT("  ") : TEXT("")), filename, lineNumber, bufferW);
                        } else {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                                TEXT("Configuration file, %s, contains a problem on line #%d and could not be read. (%s)"), filename, lineNumber, bufferW);
                        }
                    }
                    free(bufferW);
                } else {
                    outOfMemory(TEXT("RCF"), 1);
                }
                fclose(stream);
                return CONFIG_FILE_READER_FAIL;
            }
            
#ifdef _DEBUG
            /* The line feeds are not yet stripped here. */
            /*
 #ifdef WIN32
            wprintf(TEXT("%s:%d (%d): [%s]\n"), filename, lineNumber, encoding, bufferW);
 #else
            wprintf(TEXT("%S:%d (%s to %s): [%S]\n"), filename, lineNumber, encoding, interumEncoding, bufferW);
 #endif
            */
#endif
            
            c = bufferW;
            /* Always strip both ^M and ^J off the end of the line, this is done rather
             *  than simply checking for \n so that files will work on all platforms
             *  even if their line feeds are incorrect. */
            if ((d = _tcschr(bufferW, 0x0d /* ^M */)) != NULL) {
                d[0] = TEXT('\0');
            }
            if ((d = _tcschr(bufferW, 0x0a /* ^J */)) != NULL) {
                d[0] = TEXT('\0');
            }
            /* Strip any whitespace from the front of the line. */
            trimmedBuffer = bufferW;
            while ((trimmedBuffer[0] == TEXT(' ')) || (trimmedBuffer[0] == 0x08)) {
                trimmedBuffer++;
            }

            /* If the line does not start with a comment, make sure that
             *  any comment at the end of line are stripped.  If any any point, a
             *  double hash, '##', is encountered it should be interpreted as a
             *  hash in the actual property rather than the beginning of a comment. */
            if (trimmedBuffer[0] != TEXT('#')) {
                len = _tcslen(trimmedBuffer);
                i = 0;
                quoted = 0;
                while (i < len) {
                    if (trimmedBuffer[i] == TEXT('"')) {
                        quoted = !quoted;
                    } else if ((trimmedBuffer[i] == TEXT('#')) && (!quoted)) {
                        /* Checking the next character will always be ok because it will be
                         *  '\0 at the end of the string. */
                        if (trimmedBuffer[i + 1] == TEXT('#')) {
                            /* We found an escaped #. Shift the rest of the string
                             *  down by one character to remove the second '#'.
                             *  Include the shifting of the '\0'. */
                            for (j = i + 1; j <= len; j++) {
                                trimmedBuffer[j - 1] = trimmedBuffer[j];
                            }
                            len--;
                        } else {
                            /* We found a comment. So this is the end. */
                            trimmedBuffer[i] = TEXT('\0');
                            len = i;
                        }
                    }
                    i++;
                }
            }

            /* Strip any whitespace from the end of the line. */
            trimmedBufferLen = _tcslen(trimmedBuffer);
            while ((trimmedBufferLen > 0) && ((trimmedBuffer[trimmedBufferLen - 1] == TEXT(' '))
            || (trimmedBuffer[trimmedBufferLen - 1] == 0x08))) {

                trimmedBuffer[--trimmedBufferLen] = TEXT('\0');
            }

            /* Only look at lines which contain data and do not start with a '#'
             *  If the line starts with '#include' then recurse to the include file */
            if (_tcslen(trimmedBuffer) > 0) {
                if (reader->enableIncludes && strcmpIgnoreCase(trimmedBuffer, TEXT("#include.debug")) == 0) {
                    /* Enable include file debugging. */
                    if (reader->preload == FALSE) {
                        reader->debugIncludes = TRUE;
                        if (depth == 0) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                TEXT("Base configuration file is %s"), filename);
                        }
                    } else {
                        reader->debugIncludes = FALSE;
                    }
                } else if (reader->enableIncludes
			   && ((_tcsstr(trimmedBuffer, TEXT("#include ")) == trimmedBuffer) || (_tcsstr(trimmedBuffer, TEXT("#include.required ")) == trimmedBuffer))) {
                    if (_tcsstr(trimmedBuffer, TEXT("#include.required ")) == trimmedBuffer) {
                        /* The include file is required. */
                        includeRequired = TRUE;
                        c = trimmedBuffer + 18;
                    } else {
                        /* Include file, if the file does not exist, then ignore it */
                        includeRequired = FALSE;
                        c = trimmedBuffer + 9;
                    }
                    
                    /* Strip any leading whitespace */
                    while ((c[0] != TEXT('\0')) && (c[0] == TEXT(' '))) {
                        c++;
                    }

                    /* The filename may contain environment variables, so expand them. */
                    if (reader->debugIncludes) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                            TEXT("Found #include file in %s: %s"), filename, c);
                    }
                    evaluateEnvironmentVariables(c, expBuffer, MAX_PROPERTY_NAME_VALUE_LENGTH);

                    if (reader->debugIncludes && (_tcscmp(c, expBuffer) != 0)) {
                        /* Only show this log if there were any environment variables. */
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                            TEXT("  After environment variable replacements: %s"), expBuffer);
                    }

                    /* Now obtain the real absolute path to the include file. */
#ifdef WIN32
                    /* Find out how big the absolute path will be */
                    size = GetFullPathName(expBuffer, 0, NULL, NULL); /* Size includes '\0' */
                    if (!size) {
                        if (reader->debugIncludes || includeRequired) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                TEXT("Unable to resolve the full path of included configuration file: %s (%s)\n  Referenced from: %s (line %d)\n  Current working directory: %s"),
                                expBuffer, getLastErrorText(), filename, lineNumber, wrapperData->originalWorkingDir);
                        }
                        absoluteBuffer = NULL;
                    } else {
                        absoluteBuffer = malloc(sizeof(TCHAR) * size);
                        if (!absoluteBuffer) {
                            outOfMemory(TEXT("RCF"), 1);
                        } else {
                            if (!GetFullPathName(expBuffer, size, absoluteBuffer, NULL)) {
                                if (reader->debugIncludes || includeRequired) {
                                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                        TEXT("Unable to resolve the full path of included configuration file: %s (%s)\n  Referenced from: %s (line %d)\n  Current working directory: %s"),
                                        expBuffer, getLastErrorText(), filename, lineNumber, wrapperData->originalWorkingDir);
                                }
                                free(absoluteBuffer);
                                absoluteBuffer = NULL;
                            }
                        }
                    }
#else
                    absoluteBuffer = malloc(sizeof(TCHAR) * (PATH_MAX + 1));
                    if (!absoluteBuffer) {
                        outOfMemory(TEXT("RCF"), 2);
                    } else {
                        if (_trealpath(expBuffer, absoluteBuffer) == NULL) {
                            if (reader->debugIncludes || includeRequired) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                    TEXT("Unable to resolve the full path of included configuration file: %s (%s)\n  Referenced from: %s (line %d)\n  Current working directory: %s"),
                                    expBuffer, getLastErrorText(), filename, lineNumber, wrapperData->originalWorkingDir);
                            }
                            free(absoluteBuffer);
                            absoluteBuffer = NULL;
                        }
                    }
#endif
                    if (absoluteBuffer) {
                        if (depth < MAX_INCLUDE_DEPTH) {
                            readResult = configFileReader_Read(reader, absoluteBuffer, includeRequired, depth + 1, filename, lineNumber);
                            if (readResult == CONFIG_FILE_READER_SUCCESS) {
                                /* Ok continue. */
                            } else if ((readResult == CONFIG_FILE_READER_FAIL) || (readResult == CONFIG_FILE_READER_HARD_FAIL)) {
                                /* Failed. */
                                if (includeRequired) {
                                    /* Include file was required, but we failed to read it. */
                                    if (!reader->preload) {
                                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                                            TEXT("%sThe required configuration file, %s, was not loaded.\n%s  Referenced from: %s (line %d)"),
                                            (reader->debugIncludes ? TEXT("  ") : TEXT("")), absoluteBuffer, (reader->debugIncludes ? TEXT("  ") : TEXT("")), filename, lineNumber);
                                    }
                                    readResult = CONFIG_FILE_READER_HARD_FAIL;
                                }
                                if (readResult == CONFIG_FILE_READER_HARD_FAIL) {
                                    /* Can't continue. */
                                    break;
                                } else {
                                    /* Failed but continue. */
                                    readResult = CONFIG_FILE_READER_SUCCESS;
                                }
                            } else {
                                _tprintf(TEXT("Unexpected load error %d\n"), readResult);
                                /* continue. */
                                readResult = CONFIG_FILE_READER_SUCCESS;
                            }
                        } else {
                            if (reader->debugIncludes) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                    TEXT("  Unable to include configuration file, %s, because the max include depth was reached."), absoluteBuffer);
                            }
                        }
                        free(absoluteBuffer);
                    } else {
                        if (includeRequired) {
                            /* Include file was required, but we failed to read it. */
                            if (!reader->preload) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                                    TEXT("%sThe required configuration file, %s, was not read.\n%s  Referenced from: %s (line %d)"),
                                    (reader->debugIncludes ? TEXT("  ") : TEXT("")), expBuffer, (reader->debugIncludes ? TEXT("  ") : TEXT("")), filename, lineNumber);
                            }
                            readResult = CONFIG_FILE_READER_HARD_FAIL;
                            break;
                        }
                    }
                } else if (strcmpIgnoreCase(trimmedBuffer, TEXT("#properties.debug")) == 0) {
                    if (!reader->preload) {
                        /* Enable property debugging. */
                        reader->debugProperties = TRUE;
                    }
                } else if (trimmedBuffer[0] != TEXT('#')) {
                    /*_tprintf(TEXT("%s\n"), trimmedBuffer);*/

                    if (!(*reader->callback)(reader->callbackParam, filename, lineNumber, trimmedBuffer, reader->debugProperties)) {
                        readResult = CONFIG_FILE_READER_HARD_FAIL;
                        break;
                    }
                }
            }
            
            /* Always free each line read. */
            free(bufferW);
        }
        lineNumber++;
    } while (c != NULL);

    /* Close the file */
    fclose(stream);

    return readResult;
}


