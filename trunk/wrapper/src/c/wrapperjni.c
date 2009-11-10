/*
 * Copyright (c) 1999, 2009 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.org/doc/english/licenseOverview.html
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
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#endif

#include "wrapperinfo.h"
#include "wrapperjni.h"

int wrapperJNIDebugging = JNI_FALSE;

#define CONTROL_EVENT_QUEUE_SIZE 10
int controlEventQueue[CONTROL_EVENT_QUEUE_SIZE];
int controlEventQueueLastReadIndex = 0;
int controlEventQueueLastWriteIndex = 0;

/* Temporary placeholder until 3.5.0 */
const char *gettext(const char *message) {
    return message;
}
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


jstring JNU_NewStringNative(JNIEnv *env, const char *str) {
    jstring result;

    jclass Class_java_lang_String;
    jmethodID MID_String_init;
    jbyteArray bytes = 0;
    size_t len;
    
    if ((*env)->EnsureLocalCapacity(env, 2) < 0) {
        return NULL; /* out of memory error */
    }
    len = strlen(str);
    bytes = (*env)->NewByteArray(env, (jsize)len);
    if (bytes != NULL) {
        (*env)->SetByteArrayRegion(env, bytes, 0, (jsize)len, (jbyte *)str);
        Class_java_lang_String = (*env)->FindClass(env, utf8ClassJavaLangString);
        MID_String_init = (*env)->GetMethodID(env, Class_java_lang_String, utf8MethodInit, utf8Sig_BrV);
        result = (*env)->NewObject(env, Class_java_lang_String, MID_String_init, bytes);
        (*env)->DeleteLocalRef(env, bytes);
        return result;
    } /* else fall through */
    
    return NULL;
}

char *JNU_GetStringNativeChars(JNIEnv *env, jstring jstr) {
    jbyteArray bytes = 0;
    jthrowable exc;
    jclass Class_java_lang_String = NULL;
    jmethodID MID_String_getBytes = NULL;

    char *result = 0;
    if ((*env)->EnsureLocalCapacity(env, 2) < 0) {
        return 0; /* out of memory error */
    }
    if ((Class_java_lang_String = (*env)->FindClass(env, utf8ClassJavaLangString)) != NULL &&
            (MID_String_getBytes = (*env)->GetMethodID(env, Class_java_lang_String, utf8MethodGetBytes, utf8Sigr_B)) != NULL){
        bytes = (*env)->CallObjectMethod(env, jstr, MID_String_getBytes);
        exc = (*env)->ExceptionOccurred(env);
        if (!exc) {
            jint len = (*env)->GetArrayLength(env, bytes);
            result = (char *)malloc(len + 1);
            if (!result) {
                throwThrowable(env, utf8ClassJavaLangOutOfMemoryError, gettext("WrapperJNI Error: %s"), getLastErrorText());
                (*env)->DeleteLocalRef(env, bytes);
                return 0;
            }
            (*env)->GetByteArrayRegion(env, bytes, 0, len, (jbyte *)result);
            result[len] = 0; /* NULL-terminate */
        } else {
            (*env)->DeleteLocalRef(env, exc);
        }
        (*env)->DeleteLocalRef(env, bytes);
    }
    return result;
}

/**
 * Returns a new buffer containing the UTF8 characters for the specified native string.
 *
 * It is the responsibility of the caller to free the returned buffer.
 */
char *getUTF8Chars(JNIEnv *env, const char *nativeChars) {
    jstring js;
    jsize jlen;
    const char *stringChars;
    jboolean isCopy;
    char *utf8Chars = NULL;
    
    js = JNU_NewStringNative(env, nativeChars);
    if (js != NULL) {
        jlen = (*env)->GetStringUTFLength(env, js);
        utf8Chars = malloc(jlen + 1);
        if (!utf8Chars) {
            printf("Out of memory GUC(1)\n");fflush(NULL);
            return NULL;
        }

        stringChars = ((*env)->GetStringUTFChars(env, js, &isCopy));
        if (stringChars != NULL) {
            memcpy(utf8Chars, stringChars, jlen);
            utf8Chars[jlen] = '\0';
            
            (*env)->ReleaseStringUTFChars(env, js, stringChars);
        } else {
            printf("Out of memory GUC(2)\n");fflush(NULL);
            free(utf8Chars);
            return NULL;
        }
        
        (*env)->DeleteLocalRef(env, js);
    }
    
    return utf8Chars;
}

void initUTF8Strings(JNIEnv *env) {
    /* Now do the rest of the strings using our helper function. */
    utf8ClassJavaLangSystem = getUTF8Chars(env, "java/lang/System");
    utf8ClassOrgTanukisoftwareWrapperWrapperManager = getUTF8Chars(env, "org/tanukisoftware/wrapper/WrapperManager");
    utf8ClassJavaLangOutOfMemoryError = getUTF8Chars(env, "java/lang/OutOfMemoryError");
    utf8MethodGetProperty = getUTF8Chars(env, "getProperty");
    utf8MethodStopAndReturn = getUTF8Chars(env, "stopAndReturn");
    utf8MethodGetBytes = getUTF8Chars(env, "<getBytes>");
    utf8SigLjavaLangStringrLjavaLangString = getUTF8Chars(env, "(Ljava/lang/String;)Ljava/lang/String;");
    utf8SigLjavaLangStringrV = getUTF8Chars(env, "(Ljava/lang/String;)V");
    utf8SigIrV = getUTF8Chars(env, "(I)V");
    utf8Sigr_B = getUTF8Chars(env, "()[B");
#ifdef WIN32
#else
    utf8ClassOrgTanukisoftwareWrapperWrapperUNIXUser = getUTF8Chars(env, "org/tanukisoftware/wrapper/WrapperUNIXUser");
    utf8MethodSetGroup = getUTF8Chars(env, "setGroup");
    utf8MethodAddGroup = getUTF8Chars(env, "addGroup");
    utf8SigII_B_B_B_BrV = getUTF8Chars(env, "(II[B[B[B[B)V");
    utf8SigI_BrV = getUTF8Chars(env, "(I[B)V");
#endif
}

void throwThrowable(JNIEnv *env, char *throwableClassName, const char *lpszFmt, ...) {
    va_list vargs;
    int messageBufferSize = 0;
    char *messageBuffer = NULL;
    int count;
    jclass jThrowableClass;
    jmethodID constructor;
    jstring jMessageBuffer;
    jobject jThrowable;

    do {
        if ( messageBufferSize == 0 )
        {
            /* No buffer yet. Allocate one to get started. */
            messageBufferSize = 100;
            messageBuffer = (char*)malloc( messageBufferSize * sizeof(char));
            if (!messageBuffer) {
                printf("Out of memory TIOE(1)\n");fflush(NULL);
                return;
            }
        }

        /* Try writing to the buffer. */
        va_start( vargs, lpszFmt );
        #ifdef WIN32
        count = _vsnprintf(messageBuffer, messageBufferSize, lpszFmt, vargs);
        #else
        count = vsnprintf(messageBuffer, messageBufferSize, lpszFmt, vargs);
        #endif
        va_end( vargs );
        if ((count < 0) || (count >= (int)messageBufferSize)) {
            /* If the count is exactly equal to the buffer size then a null char was not written.
             *  It must be larger.
             * Windows will return -1 if the buffer is too small. If the number is
             *  exact however, we still need to expand it to have room for the null.
             * UNIX will return the required size. */

            /* Free the old buffer for starters. */
            free(messageBuffer);

            /* Decide on a new buffer size. */
            if (count <= (int)messageBufferSize) {
                messageBufferSize += 50;
            } else if (count + 1 <= (int)messageBufferSize + 50) {
                messageBufferSize += 50;
            } else {
                messageBufferSize = count + 1;
            }

            messageBuffer = (char*)malloc(messageBufferSize * sizeof(char));
            if (!messageBuffer) {
                printf(gettext("Out of memory %s\n"), "TIOE(2)");fflush(NULL);
                messageBufferSize = 0;
                return;
            }

            /* Always set the count to -1 so we will loop again. */
            count = -1;
        }
    } while ( count < 0 );

    /* We have the messageBuffer */
    if ((jThrowableClass = (*env)->FindClass(env, throwableClassName)) != NULL) {
        if ((constructor = (*env)->GetMethodID(env, jThrowableClass, utf8MethodInit, utf8SigLjavaLangStringrV)) != NULL) {
            if ((jMessageBuffer = JNU_NewStringNative(env, messageBuffer)) != NULL) {
                if ((jThrowable = (*env)->NewObject(env, jThrowableClass, constructor, jMessageBuffer)) != NULL) {
                    if ((*env)->Throw(env, jThrowable)){
                        printf(gettext("WrapperJNI Error: Unable to throw %s with message: %s"), throwableClassName, messageBuffer); fflush(NULL);
                    }
                    (*env)->DeleteLocalRef(env, jThrowable);
                } else {
                    printf(gettext("WrapperJNI Error: Unable to create instance of class, '%s' to report exception: %s"),
                        throwableClassName, messageBuffer); fflush(NULL);
                }
                (*env)->DeleteLocalRef(env, jMessageBuffer);
            } else {
                printf(gettext("WrapperJNI Error: Unable to create string to report '%s' exception: %s"),
                    throwableClassName, messageBuffer); fflush(NULL);
            }
        } else {
            printf(gettext("WrapperJNI Error: Unable to find constructor for class, '%s' to report exception: %s"),
                throwableClassName, messageBuffer); fflush(NULL);
        }
        (*env)->DeleteLocalRef(env, jThrowableClass);
    } else {
        printf(gettext("WrapperJNI Error: Unable to load class, '%s' to report exception: %s"),
            throwableClassName, messageBuffer); fflush(NULL);
    }
    free(messageBuffer);
}

void wrapperJNIHandleSignal(int signal) {
    if (wrapperLockControlEventQueue()) {
        /* Failed.  Should have been reported. */
        printf("WrapperJNI Error: Signal %d trapped, but ignored.\n", signal);
        fflush(NULL);
        return;
    }

#ifdef _DEBUG
    printf(" Queue Write 1 R:%d W:%d E:%d\n", controlEventQueueLastReadIndex, controlEventQueueLastWriteIndex, signal);
    fflush(NULL);
#endif
    controlEventQueueLastWriteIndex++;
    if (controlEventQueueLastWriteIndex >= CONTROL_EVENT_QUEUE_SIZE) {
        controlEventQueueLastWriteIndex = 0;
    }
    controlEventQueue[controlEventQueueLastWriteIndex] = signal;
#ifdef _DEBUG
    printf(" Queue Write 2 R:%d W:%d\n", controlEventQueueLastReadIndex, controlEventQueueLastWriteIndex);
    fflush(NULL);
#endif

    if (wrapperReleaseControlEventQueue()) {
        /* Failed.  Should have been reported. */
        return;
    }
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeGetLibraryVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeGetLibraryVersion(JNIEnv *env, jclass clazz) {
    jstring version;

    version = JNU_NewStringNative(env, wrapperVersion);

    return version;
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeIsProfessionalEdition
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeIsProfessionalEdition(JNIEnv *env, jclass clazz) {
    return JNI_FALSE;
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeIsStandardEdition
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeIsStandardEdition(JNIEnv *env, jclass clazz) {
    return JNI_FALSE;
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeGetControlEvent
 * Signature: (V)I
 */
JNIEXPORT jint JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeGetControlEvent(JNIEnv *env, jclass clazz) {
    int event = 0;

    if (wrapperLockControlEventQueue()) {
        /* Failed.  Should have been reported. */
        return 0;
    }

    if (controlEventQueueLastWriteIndex != controlEventQueueLastReadIndex) {
#ifdef _DEBUG
        printf(" Queue Read 1 R:%d W:%d\n", controlEventQueueLastReadIndex, controlEventQueueLastWriteIndex);
        fflush(NULL);
#endif
        controlEventQueueLastReadIndex++;
        if (controlEventQueueLastReadIndex >= CONTROL_EVENT_QUEUE_SIZE) {
            controlEventQueueLastReadIndex = 0;
        }
        event = controlEventQueue[controlEventQueueLastReadIndex];
#ifdef _DEBUG
        printf(" Queue Read 2 R:%d W:%d E:%d\n", controlEventQueueLastReadIndex, controlEventQueueLastWriteIndex, event);
        fflush(NULL);
#endif
    }

    if (wrapperReleaseControlEventQueue()) {
        /* Failed.  Should have been reported. */
        return event;
    }

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
