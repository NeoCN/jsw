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
// Revision 1.1  2001/11/07 08:54:20  mortenson
// Initial revision
//

#include "wrapperjni.h"

int wrapperJNIDebugging = JNI_FALSE;

int lastControlEvent = 0;
int checkingControlEvent = 0;

void wrapperJNIHandleSignal(int signal) {
    // Wait for the semaphore before setting the value
    while (checkingControlEvent) {
        // Tight loop, but will be very short.
    }

    // Save the last signal
    lastControlEvent = signal;
}


/*
 * Class:     com_silveregg_wrapper_WrapperManager
 * Method:    nativeGetControlEvent
 * Signature: (V)I
 */
JNIEXPORT int JNICALL
Java_com_silveregg_wrapper_WrapperManager_nativeGetControlEvent(JNIEnv *env, jclass clazz) {
    int event;

    // Use this as a very simple semaphore
    checkingControlEvent = 1;

    // Check the event.  This is not technically thread safe, but there are
    //  very few threads coming through.
    if ((event = lastControlEvent) != 0) {
        lastControlEvent = 0;
    }

    // Clear the semaphore
    checkingControlEvent = 0;

    return event;
}

/*
 * Class:     com_silveregg_wrapper_WrapperManager
 * Method:    accessViolationInner
 * Signature: (V)V
 */
JNIEXPORT void JNICALL
Java_com_silveregg_wrapper_WrapperManager_accessViolationInner(JNIEnv *env, jclass clazz) {
    char *ptr;

    // Cause access violation
    ptr = NULL;
    ptr[0] = '\n';
}
