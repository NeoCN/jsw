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
 * Revision 1.9  2003/06/19 05:23:40  mortenson
 * Fix a problem where the JVM was not receiving CTRL-C and CTRL-CLOSE events
 * when running under the Wrapper on Windows.
 *
 * Revision 1.8  2003/04/03 04:05:22  mortenson
 * Fix several typos in the docs.  Thanks to Mike Castle.
 *
 * Revision 1.7  2003/02/03 06:55:27  mortenson
 * License transfer to TanukiSoftware.org
 *
 */

#ifndef WIN32
/* For some reason this is not defined sometimes when I build on MFVC 6.0 $%$%$@@!!
 * This causes a compiler error to let me know about the problem.  Anyone with any
 * ideas as to why this sometimes happens or how to fix it, please let me know. */
barf
#endif

#ifdef WIN32

#include <windows.h>
#include "wrapperjni.h"

static DWORD wrapperProcessId = 0;

/**
 * Handler to take care of the case where the user hits CTRL-C when the wrapper
 *  is being run as a console.  If this is not done, then the Java process
 *  would exit due to a CTRL_LOGOFF_EVENT when a user logs off even if the
 *  application is installed as a service.
 */
int wrapperConsoleHandler(int key) {
    int event;

    /* Call the control callback in the java code */
    switch(key) {
    case CTRL_C_EVENT:
        event = org_tanukisoftware_wrapper_WrapperManager_WRAPPER_CTRL_C_EVENT;
        break;
    case CTRL_BREAK_EVENT:
        /* This is a request to do a thread dump. Let the JVM handle this. */
        return FALSE;
    case CTRL_CLOSE_EVENT:
        event = org_tanukisoftware_wrapper_WrapperManager_WRAPPER_CTRL_CLOSE_EVENT;
        break;
    case CTRL_LOGOFF_EVENT:
        event = org_tanukisoftware_wrapper_WrapperManager_WRAPPER_CTRL_LOGOFF_EVENT;
        break;
    case CTRL_SHUTDOWN_EVENT:
        event = org_tanukisoftware_wrapper_WrapperManager_WRAPPER_CTRL_SHUTDOWN_EVENT;
        break;
    default:
        event = key;
    }
    if (wrapperJNIDebugging) {
        printf("Got Control Signal %d->%d\n", key, event);
        flushall();
    }

    wrapperJNIHandleSignal(event);

    if (wrapperJNIDebugging) {
        printf("Handled signal\n");
        flushall();
    }

    return TRUE; /* We handled the event. */
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeInit
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeInit(JNIEnv *env, jclass clazz, jboolean debugging) {
    char szPath[512];

    wrapperJNIDebugging = debugging;

    if (wrapperJNIDebugging) {
        /* This is useful for making sure that the JNI call is working. */
        printf("Initializing WrapperManager native library.\n");
        flushall();

        if (GetModuleFileName(NULL, szPath, 512) == 0){
            printf("Unable to retrieve the Java process file name.\n");
            flushall();
        } else {
            printf("Java Executable: %s\n", szPath);
            flushall();
        }
    }

    /* Make sure that the handling of CTRL-C signals is enabled for this process. */
    SetConsoleCtrlHandler(NULL, FALSE);

    /* Initialize the CTRL-C handler */
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)wrapperConsoleHandler, TRUE);

    /* Store the current process Id */
    wrapperProcessId = GetCurrentProcessId();
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeRequestThreadDump
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeRequestThreadDump(JNIEnv *env, jclass clazz) {
    if (wrapperJNIDebugging) {
        printf("Sending BREAK event to process group %ld.\n", wrapperProcessId);
        flushall();
    }
    if ( GenerateConsoleCtrlEvent( CTRL_BREAK_EVENT, wrapperProcessId ) == 0 ) {
        printf("Unable to send BREAK event to JVM process.  Err(%ld)\n", GetLastError());
        flushall();
    }
}

#endif
