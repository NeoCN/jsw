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
 */

// $Log$
// Revision 1.2  2001/11/08 04:22:28  mortenson
// Had been having a problem with the windows build where the WIN32
// symbol was not defined sometimes.  Figured out that that was the
// cause of a strange build problem enabling me to remove the
// workaround code.  wrapperjni_win.c will now fail to build if the symbol
// is not defined.
//
// Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
// no message
//

#ifndef WIN32
// For some reason this is not defined sometimes when I build on MFVC 6.0 $%$%$@@!!
// This causes a compiler error to let me know about the problem.  Anyone with any
// ideas as to why this sometimes happens or how to fix it, please let me know.
barf
#endif

#ifdef WIN32

#include <windows.h>
#include "wrapperjni.h"

/**
 * Handler to take care of the case where the user hits CTRL-C when the wrapper
 *  is being run as a console.  If this is not done, then the Java process
 *  would exit due to a CTRL_LOGOFF_EVENT when a user logs off even if the
 *  application is installed as a service.
 */
int wrapperConsoleHandler(int key) {
    int event;

    // Call the control callback in the java code
    switch(key) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        event = com_silveregg_wrapper_WrapperManager_WRAPPER_CTRL_C_EVENT;
        break;
    case CTRL_CLOSE_EVENT:
        event = com_silveregg_wrapper_WrapperManager_WRAPPER_CTRL_CLOSE_EVENT;
        break;
    case CTRL_LOGOFF_EVENT:
        event = com_silveregg_wrapper_WrapperManager_WRAPPER_CTRL_LOGOFF_EVENT;
        break;
    case CTRL_SHUTDOWN_EVENT:
        event = com_silveregg_wrapper_WrapperManager_WRAPPER_CTRL_SHUTDOWN_EVENT;
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

    return TRUE; // We handled the event.
}

/*
 * Class:     com_silveregg_wrapper_WrapperManager
 * Method:    nativeInit
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_com_silveregg_wrapper_WrapperManager_nativeInit(JNIEnv *env, jclass clazz, jboolean debugging) {
    wrapperJNIDebugging = debugging;

    if (wrapperJNIDebugging) {
        // This is useful for making sure that the JNI call is working.
        printf("Inside native WrapperManager initialization method\n");
        flushall();
    }

    // Initialize the CTRL-C handler
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)wrapperConsoleHandler, TRUE);
}
#endif
