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

#include <stdio.h>
#include <signal.h>
#include "wrapperjni.h"

/**
 * Handle interrupt signals (i.e. Crtl-C).
 */
void handleInterrupt(int sig_num) {
    signal(SIGINT, handleInterrupt);
    wrapperJNIHandleSignal(com_silveregg_wrapper_WrapperManager_WRAPPER_CTRL_C_EVENT);
}

/**
 * Handle termination signals (i.e. machine is shutting down).
 */
void handleTermination(int sig_num) {
    signal(SIGTERM, handleTermination); 
    wrapperJNIHandleSignal(com_silveregg_wrapper_WrapperManager_WRAPPER_CTRL_SHUTDOWN_EVENT);
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
        fflush(NULL);
    }

    // Set handlers for signals
    signal(SIGINT,  handleInterrupt);
    signal(SIGTERM, handleTermination);
}
#endif
