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

#include <stdio.h>
#include <string.h>
#ifdef WIN32
 #include <tchar.h>
 #include <windows.h>
#else
 #include <pthread.h>
#endif
#include <errno.h>
#include "loggerjni.h"

void outOfMemory(const TCHAR *context, int id) {
    _tprintf(TEXT("WrapperJNI Error: Out of memory (%s%02d). %s\n"), context, id, getLastErrorText());fflush(NULL);
}

void invalidMultiByteSequence(const TCHAR *context, int id) {
    _tprintf(TEXT("WrapperJNI Error: Invalid multibyte Sequence found in (%s%02d). %s"), context, id, getLastErrorText());fflush(NULL);
}

/**
 * Create an error message from GetLastError() using the
 *  FormatMessage API Call...
 */
#ifdef WIN32
TCHAR lastErrBuf[1024];
TCHAR* getLastErrorText() {
    DWORD dwRet;
    TCHAR* lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (TCHAR*)&lpszTemp,
                           0,
                           NULL);

    /* supplied buffer is not long enough */
    if (!dwRet || ((long)1023 < (long)dwRet+14)) {
        lastErrBuf[0] = TEXT('\0');
    } else {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  /*remove cr and newline character */
        _sntprintf( lastErrBuf, 1024, TEXT("%s (0x%x)"), lpszTemp, GetLastError());
    }

    if (lpszTemp) {
        GlobalFree((HGLOBAL) lpszTemp);
    }

    return lastErrBuf;
}
int getLastError() {
    return GetLastError();
}
#else
TCHAR* getLastErrorText() {
#ifdef UNICODE
    char* c;
    TCHAR* t;
    size_t req;
    c = strerror(errno);
    req = mbstowcs(NULL, c, 0);
    if (req < 0) {
        invalidMultiByteSequence(TEXT("GLET"), 1);
        return NULL;
    }
    t = malloc(sizeof(TCHAR) * (req + 1));
    if (!t) {
        _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("GLET1"));
        return NULL;
    }
    mbstowcs(t, c, req + 1);
    return t;

#else
    return strerror(errno);
#endif
}
int getLastError() {
    return errno;
}
#endif
