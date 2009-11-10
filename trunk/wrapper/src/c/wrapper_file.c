/*
 * Copyright (c) 1999, 2009 Tanuki Software, Ltd.
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

#include "wrapper_file.h"
#include "logger.h"

#include <stdio.h>
#ifdef WIN32
#include <errno.h>
#include <tchar.h>
#include <io.h>
#else
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define FILES_CHUNK 5

#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/**
 * Returns a valid sort mode given a name: "TIMES", "NAMES_ASC", "NAMES_DEC".
 *  In the event of an invalid value, TIMES will be returned.
 */
int wrapperFileGetSortMode(const char *modeName) {
    if (strcmpIgnoreCase(modeName, "NAMES_ASC") == 0) {
        return WRAPPER_FILE_SORT_MODE_NAMES_ASC;
    } else if (strcmpIgnoreCase(modeName, "NAMES_DEC") == 0) {
        return WRAPPER_FILE_SORT_MODE_NAMES_DEC;
    } else {
        return WRAPPER_FILE_SORT_MODE_TIMES;
    }
}


#ifdef WIN32
int sortFilesTimes(char **files, __time64_t *fileTimes, int cnt) {
#else
int sortFilesTimes(char **files, time_t *fileTimes, int cnt) {
#endif
    int i, j;
    char *temp;
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
int compareFileNames(const char *file1, const char *file2) {
    int pos1, pos2;
    char c1, c2;
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
        numeric1 = (c1 >= '0' && c1 <= '9');
        numeric2 = (c2 >= '0' && c2 <= '9');
        
        /* See if one or both of the strings is numeric. */
        if (numeric1) {
            if (numeric2) {
                /* Both are numeric, we need to start comparing the two file names as integer values. */
                num1 = c1 - '0';
                c1 = file1[pos1 + 1];
                while (c1 >= '0' && c1 <= '9') {
                    num1 = num1 * 10 + (c1 - '0');
                    pos1++;
                    c1 = file1[pos1 + 1];
                }
                
                num2 = c2 - '0';
                c2 = file2[pos2 + 1];
                while (c2 >= '0' && c2 <= '9') {
                    num2 = num2 * 10 + (c2 - '0');
                    pos2++;
                    c2 = file2[pos2 + 1];
                }
                
                /*printf("     num1=%ld, num2=%ld\n", num1, num2);*/
                if (num1 > num2) {
                    return -1;
                } else if (num2 > num1 ) {
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
            if (c1 == '.' || c1 == '-' || c1 == '_') {
            } else {
                afterNumber = FALSE;
            }
        }
        
        pos1++;
        pos2++;
    }
}

int sortFilesNamesAsc(char **files, int cnt) {
    int i, j;
    char *temp;
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

int sortFilesNamesDec(char **files, int cnt) {
    int i, j;
    char *temp;
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
char** wrapperFileGetFiles(const char* pattern, int sortMode) {
    int cnt;
    int filesSize;
    char **files;
#ifdef WIN32
    int i;
    size_t dirLen;
    char *c;
    char *dirPart;
    intptr_t handle;
    struct _tfinddata64_t fblock;
    size_t fileLen;
    char **newFiles;
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
    printf("wrapperFileGetFiles(%s, %d)\n", pattern, sortMode);
#endif
    
#ifdef WIN32
    cnt = 0;
    /* Initialize the files array. */
    filesSize = FILES_CHUNK;
    files = malloc(sizeof(char *) * filesSize);
    if (!files) {
        outOfMemoryQueued("WFGF", 1);
        return NULL;
    }
    memset(files, 0, sizeof(char *) * filesSize);
    
    fileTimes = malloc(sizeof(__time64_t) * filesSize);
    if (!fileTimes) {
        outOfMemoryQueued("WFGF", 2);
        free(files);
        return NULL;
    }
    memset(fileTimes, 0, sizeof(__time64_t) * filesSize);
    
    /* Extract any path information from the beginning of the file */
    c = max(strrchr(pattern, '\\'), strrchr(pattern, '/'));
    if (c == NULL) {
        /* No directory component */
        dirPart = malloc(sizeof(char) * 1);
        if (!dirPart) {
            outOfMemoryQueued("WFGF", 3);
            return NULL;
        }
        dirPart[0] = '\0';
        dirLen = 0;
    } else {
        /* extract the directory. */
        dirLen = c - pattern + 1;
        dirPart = malloc(dirLen + 1);
        if (!dirPart) {
            outOfMemoryQueued("WFGF", 4);
            return NULL;
        }
        memcpy(dirPart, pattern, dirLen);
        dirPart[dirLen] = '\0';
    }

#ifdef WRAPPER_FILE_DEBUG
    printf("  dirPart=[%s]\n", dirPart);
#endif
    
    /* Get the first file. */
    if ((handle = _tfindfirst64(pattern, &fblock)) > 0) {
        if ((strcmp(fblock.name, ".") != 0) && (strcmp(fblock.name, "..") != 0)) {
            fileLen = _tcslen(fblock.name);
            files[cnt] = malloc((_tcslen(dirPart) + _tcslen(fblock.name) + 1 ) * sizeof(TCHAR));
            if (!files[cnt]) {
                outOfMemoryQueued("WFGF", 5);
                free(fileTimes);
                wrapperFileFreeFiles(files);
                free(dirPart);
                return NULL;
            }
            sprintf(files[cnt], "%s%s", dirPart, fblock.name);
            fileTimes[cnt] = fblock.time_write;
#ifdef WRAPPER_FILE_DEBUG
            printf("  files[%d]=%s, %ld\n", cnt, files[cnt], fileTimes[cnt]);
#endif
            
            cnt++;
        }
        
        /* Look for additional files. */
        while (_tfindnext64(handle, &fblock) == 0) {
            if ((strcmp(fblock.name, ".") != 0) && (strcmp(fblock.name, "..") != 0)) {
                /* Make sure we have enough room in the files array. */
                if (cnt >= filesSize - 1) {
                    newFiles = malloc(sizeof(char *) * (filesSize + FILES_CHUNK));
                    if (!newFiles) {
                        outOfMemoryQueued("WFGF", 6);
                        free(fileTimes);
                        wrapperFileFreeFiles(files);
                        free(dirPart);
                        return NULL;
                    }
                    memset(newFiles, 0, sizeof(char *) * (filesSize + FILES_CHUNK));
                    newFileTimes = malloc(sizeof(__time64_t) * (filesSize + FILES_CHUNK));
                    if (!newFileTimes) {
                        outOfMemoryQueued("WFGF", 7);
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
                    printf("  increased files to %d\n", filesSize);
#endif
                }
                
                fileLen = strlen(fblock.name);
                files[cnt] = malloc((_tcslen(dirPart) + _tcslen(fblock.name) + 1 ) * sizeof(TCHAR));
                if (!files[cnt]) {
                    outOfMemoryQueued("WFGF", 8);
                    free(fileTimes);
                    wrapperFileFreeFiles(files);
                    free(dirPart);
                    return NULL;
                }
                sprintf(files[cnt], "%s%s", dirPart, fblock.name);
                fileTimes[cnt] = fblock.time_write;
                
#ifdef WRAPPER_FILE_DEBUG
                printf("  files[%d]=%s, %ld\n", cnt, files[cnt], fileTimes[cnt]);
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
            printf("  No files matched.\n");
#endif
        } else {
            /* Encountered an error of some kind. */
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Error listing files, %s: %s", pattern, getLastErrorText());
            free(fileTimes);
            wrapperFileFreeFiles(files);
            return NULL;
        }
    }
#else
    cnt = 0;
    result = glob(pattern, GLOB_MARK | GLOB_NOSORT, NULL, &g);
    if (!result) {
        if (g.gl_pathc > 0) {
            filesSize = g.gl_pathc + 1;
            files = malloc(sizeof(char *) * filesSize);
            if (!files) {
                outOfMemoryQueued("WFGF", 9);
                return NULL;
            }
            memset(files, 0, sizeof(char *) * filesSize);
            
            fileTimes = malloc(sizeof(time_t) * filesSize);
            if (!fileTimes) {
                outOfMemoryQueued("WFGF", 10);
                wrapperFileFreeFiles(files);
                return NULL;
            }
            memset(fileTimes, 0, sizeof(time_t) * filesSize);
            
            for (findex=0; findex < g.gl_pathc; findex++) {
                files[cnt] = malloc(strlen(g.gl_pathv[findex]) + 1);
                if (!files[cnt]) {
                    outOfMemoryQueued("WFGF", 11);
                    free(fileTimes);
                    wrapperFileFreeFiles(files);
                    return NULL;
                }
                strcpy(files[cnt], g.gl_pathv[findex]);
                
                /* Only try to get the modified time if it is really necessary. */
                if (sortMode == WRAPPER_FILE_SORT_MODE_TIMES) {
                    if (!stat(files[cnt], &fileStat)) {
                        fileTimes[cnt] = fileStat.st_mtime;
                    } else {
                        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, "Failed to stat %s: %s", files[cnt], getLastErrorText());
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
            files = malloc(sizeof(char *) * filesSize);
            if (!files) {
                outOfMemoryQueued("WFGF", 12);
                return NULL;
            }
            memset(files, 0, sizeof(char *) * filesSize);
            
            fileTimes = malloc(sizeof(time_t) * filesSize);
            if (!fileTimes) {
                outOfMemoryQueued("WFGF", 13);
                return NULL;
            }
            memset(fileTimes, 0, sizeof(time_t) * filesSize);
        }
        
        globfree(&g);
    } else if (result == GLOB_NOMATCH) {
#ifdef WRAPPER_FILE_DEBUG
        printf("  No files matched.\n");
#endif
        /* No files, but we still need the array. */
        filesSize = 1;
        files = malloc(sizeof(char *) * filesSize);
        if (!files) {
            outOfMemoryQueued("WFGF", 14);
            return NULL;
        }
        memset(files, 0, sizeof(char *) * filesSize);
        
        fileTimes = malloc(sizeof(time_t) * filesSize);
        if (!fileTimes) {
            outOfMemoryQueued("WFGF", 15);
            return NULL;
        }
        memset(fileTimes, 0, sizeof(time_t) * filesSize);
    } else {
        /* Encountered an error of some kind. */
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, "Error listing files, %s: %s", pattern, getLastErrorText());
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
    } else {
        if (!sortFilesNamesAsc(files, cnt)) {
            /* Failed. Reported. */
            free(fileTimes);
            wrapperFileFreeFiles(files);
            return NULL;
        }
    }
    
#ifdef WRAPPER_FILE_DEBUG
    printf("  Sorted:\n");
    for (i = 0; i < cnt; i++) {
        printf("  files[%d]=%s, %ld\n", i, files[i], fileTimes[i]);
    }
    printf("wrapperFileGetFiles(%s, %d) END\n", pattern, sortMode);
#endif

    free(fileTimes);
    
    return files;
}

/**
 * Frees the array of file names returned by wrapperFileGetFiles()
 */
void wrapperFileFreeFiles(char** files) {
    int i;
    
    i = 0;
    while (files[i]) {
        free(files[i]);
        i++;
    }
    free(files);
}

#ifdef WRAPPER_FILE_DEBUG
void wrapperFileTests() {
    char** files;
    
    files = wrapperFileGetFiles("../logs/*.log*", WRAPPER_FILE_SORT_MODE_TIMES);
    if (files) {
        wrapperFileFreeFiles(files);
    }
    
    files = wrapperFileGetFiles("../logs/*.log*", WRAPPER_FILE_SORT_MODE_NAMES_ASC);
    if (files) {
        wrapperFileFreeFiles(files);
    }
    
    files = wrapperFileGetFiles("../logs/*.log*", WRAPPER_FILE_SORT_MODE_NAMES_DEC);
    if (files) {
        wrapperFileFreeFiles(files);
    }
}
#endif
