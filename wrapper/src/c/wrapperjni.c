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
 * Revision 1.11  2005/05/23 02:37:55  mortenson
 * Update the copyright information.
 *
 * Revision 1.10  2004/07/15 14:16:37  mortenson
 * Make variables used by the signal polling code to remove a compiler warning
 * on 64-bit HP-UX systems.
 *
 * Revision 1.9  2004/01/16 04:42:00  mortenson
 * The license was revised for this version to include a copyright omission.
 * This change is to be retroactively applied to all versions of the Java
 * Service Wrapper starting with version 3.0.0.
 *
 * Revision 1.8  2003/11/05 16:56:25  mortenson
 * Get the version checking code working on Linux.
 *
 * Revision 1.7  2003/10/31 11:18:48  mortenson
 * Fix a compiler error on linux caused by missing headers.
 *
 * Revision 1.6  2003/10/31 11:10:46  mortenson
 * Add a getLastErrorText function so we can display more user friendly messages
 * within the native library.
 *
 * Revision 1.5  2003/04/03 04:05:22  mortenson
 * Fix several typos in the docs.  Thanks to Mike Castle.
 *
 * Revision 1.4  2003/02/03 06:55:27  mortenson
 * License transfer to TanukiSoftware.org
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#endif

#include "wrapperinfo.h"
#include "wrapperjni.h"

int wrapperJNIDebugging = JNI_FALSE;

volatile int lastControlEvent = 0;
volatile int checkingControlEvent = 0;

/**
 * Create an error message from GetLastError() using the
 *  FormatMessage API Call...
 */
#ifdef WIN32
TCHAR lastErrBuf[1024];
char* getLastErrorText() {
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                           FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL);

    /* supplied buffer is not long enough */
    if (!dwRet || ((long)1023 < (long)dwRet+14)) {
        lastErrBuf[0] = TEXT('\0');
    } else {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  /*remove cr and newline character */
        _stprintf( lastErrBuf, TEXT("%s (0x%x)"), lpszTemp, GetLastError());
    }

    if (lpszTemp) {
        GlobalFree((HGLOBAL) lpszTemp);
    }

    return lastErrBuf;
}
#else
char* getLastErrorText() {
    return strerror(errno);
}
#endif

void wrapperJNIHandleSignal(int signal) {
    /* Wait for the semaphore before setting the value */
    while (checkingControlEvent) {
        /* Tight loop, but will be very short. */
    }

    /* Save the last signal */
    lastControlEvent = signal;
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeGetLibraryVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeGetLibraryVersion(JNIEnv *env, jclass clazz) {
    jstring version;

    version = (*env)->NewStringUTF(env, wrapperVersion);

    return version;
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeGetControlEvent
 * Signature: (V)I
 */
JNIEXPORT jint JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeGetControlEvent(JNIEnv *env, jclass clazz) {
    int event;

    /* Use this as a very simple semaphore */
    checkingControlEvent = 1;

    /* Check the event.  This is not technically thread safe, but there are
     *  very few threads coming through. */
    if ((event = lastControlEvent) != 0) {
        lastControlEvent = 0;
    }

    /* Clear the semaphore */
    checkingControlEvent = 0;

    return event;
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    accessViolationInner
 * Signature: (V)V
 */
JNIEXPORT void JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_accessViolationInner(JNIEnv *env, jclass clazz) {
    char *ptr;

    /* Cause access violation */
    ptr = NULL;
    ptr[0] = '\n';
}
