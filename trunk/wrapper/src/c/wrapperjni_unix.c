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
 * Revision 1.11  2004/01/10 17:17:26  mortenson
 * Add the ability to request user information.
 *
 * Revision 1.10  2003/11/03 10:27:46  mortenson
 * Fix some link errors.
 *
 * Revision 1.9  2003/11/02 20:55:54  mortenson
 * Implement stubs for the user methods.
 *
 * Revision 1.8  2003/10/31 11:10:46  mortenson
 * Add a getLastErrorText function so we can display more user friendly messages
 * within the native library.
 *
 * Revision 1.7  2003/10/31 05:59:34  mortenson
 * Added a new method, setConsoleTitle, to the WrapperManager class which
 * enables the application to dynamically set the console title.
 *
 * Revision 1.6  2003/04/03 04:05:22  mortenson
 * Fix several typos in the docs.  Thanks to Mike Castle.
 *
 * Revision 1.5  2003/02/03 06:55:27  mortenson
 * License transfer to TanukiSoftware.org
 *
 */

#ifndef WIN32

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "wrapperjni.h"

static pid_t wrapperProcessId = -1;

/**
 * Handle interrupt signals (i.e. Crtl-C).
 */
void handleInterrupt(int sig_num) {
    signal(SIGINT, handleInterrupt);
    wrapperJNIHandleSignal(org_tanukisoftware_wrapper_WrapperManager_WRAPPER_CTRL_C_EVENT);
}

/**
 * Handle termination signals (i.e. machine is shutting down).
 */
void handleTermination(int sig_num) {
    signal(SIGTERM, handleTermination); 
    wrapperJNIHandleSignal(org_tanukisoftware_wrapper_WrapperManager_WRAPPER_CTRL_SHUTDOWN_EVENT);
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeInit
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeInit(JNIEnv *env, jclass clazz, jboolean debugging) {
    wrapperJNIDebugging = debugging;

    if (wrapperJNIDebugging) {
        /* This is useful for making sure that the JNI call is working. */
        printf("Inside native WrapperManager initialization method\n");
        fflush(NULL);
    }

    /* Set handlers for signals */
    signal(SIGINT,  handleInterrupt);
    signal(SIGTERM, handleTermination);

    /* Store the current process Id */
    wrapperProcessId = getpid();
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeRequestThreadGroup
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeRequestThreadDump(JNIEnv *env, jclass clazz) {
    if (wrapperJNIDebugging) {
        printf("Sending SIGQUIT event to process group %d.\n", wrapperProcessId);
        fflush(NULL);
    }
    if (kill(wrapperProcessId, SIGQUIT) < 0) {
        printf("Unable to send SIGQUIT to JVM process: %s\n", getLastErrorText());
        fflush(NULL);
    }
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeSetConsoleTitle
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeSetConsoleTitle(JNIEnv *env, jclass clazz, jbyteArray jTitleBytes) {
    if (wrapperJNIDebugging) {
        printf("Setting the console title not supported on UNIX platforms.\n");
        fflush(NULL);
    }
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeGetUser
 * Signature: (Z)Lorg/tanukisoftware/wrapper/WrapperUser;
 */
/*#define UVERBOSE*/
JNIEXPORT jobject JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeGetUser(JNIEnv *env, jclass clazz, jboolean groups) {
    jclass wrapperUserClass;
    jmethodID constructor;
    uid_t uid;
    struct passwd *pw;
    jbyteArray jUser;
    jobject wrapperUser = NULL;

    printf("nativeGetUser\n");
    fflush(NULL);

    /* Look for the WrapperUser class. Ignore failures as JNI throws an exception. */
    if ((wrapperUserClass = (*env)->FindClass(env, "org/tanukisoftware/wrapper/WrapperUNIXUser")) != NULL) {

        /* Look for the constructor. Ignore failures. */
        if ((constructor = (*env)->GetMethodID(env, wrapperUserClass, "<init>", "(I[B)V")) != NULL) {

            uid = geteuid();
            pw = getpwuid(uid);

            /* Create the arguments to the constructor as java objects */

            /* User byte array */
            jUser = (*env)->NewByteArray(env, strlen(pw->pw_name));
            (*env)->SetByteArrayRegion(env, jUser, 0, strlen(pw->pw_name), (jbyte*)pw->pw_name);

            /* Now create the new wrapperUser using the constructor arguments collected above. */
            wrapperUser = (*env)->NewObject(env, wrapperUserClass, constructor, uid, jUser);

            /* If the caller requested the user's groups then look them up. */
            if (groups) {
                
            }
        }
    }

    return wrapperUser;
}


/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeGetInteractiveUser
 * Signature: (Z)Lorg/tanukisoftware/wrapper/WrapperUser;
 */
JNIEXPORT jobject JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeGetInteractiveUser(JNIEnv *env, jclass clazz, jboolean groups) {
    /* If the DISPLAY environment variable is set then assume that this user
     *  has access to an X display, in which case we will return the same thing
     *  as nativeGetUser. */
    if (getenv("DISPLAY")) {
        /* This is an interactive JVM since it has access to a display. */
        return Java_org_tanukisoftware_wrapper_WrapperManager_nativeGetUser(env, clazz, groups);
    } else {
        /* There is no DISPLAY variable, so assume that this JVM is non-interactive. */
        return NULL;
    }
}

#endif
