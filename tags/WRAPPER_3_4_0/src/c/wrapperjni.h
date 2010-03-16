/*
 * Copyright (c) 1999, 2010 Tanuki Software, Ltd.
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

#ifndef _WRAPPERJNI_H
#define _WRAPPERJNI_H

#include "org_tanukisoftware_wrapper_WrapperManager.h"

#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0
#endif


/* UTF8 String references for support on non-ASCII platforms. */
extern const char utf8MethodInit[];
#ifdef WIN32
#else
extern char *utf8ClassOrgTanukisoftwareWrapperWrapperUNIXUser;
extern char *utf8MethodSetGroup;
extern char *utf8MethodAddGroup;
extern char *utf8SigII_B_B_B_BrV;
extern char *utf8SigI_BrV;
extern char *utf8ClassOrgTanukisoftwareWrapperWrapperProcess;
extern char *utf8VrV;
extern char *utf8m_ptr;
extern char *utf8SigJ;
extern char* utf8SigBLJavaLangStringrV;
extern char* utf8MethodSendCommand;
extern char *utf8javaIOIOException;  /*java/io/IOException*/
extern char* utf8ClassJavaLangOutOfMemoryError;
extern char* utf8javaLangNullPointerException;
extern char* utf8javalangIllegalArgumentException;
extern char* utf8javalangUnsatisfiedLinkError;


#endif

/* temporally used as placeholder */
extern const char *gettext(const char *message);


extern void initUTF8Strings();
extern char* getLastErrorText();
extern int getLastError();
extern void throwJNIError(JNIEnv *env, const char *message);
extern int wrapperJNIDebugging;
extern int wrapperLockControlEventQueue();
extern int wrapperReleaseControlEventQueue();
extern void wrapperJNIHandleSignal(int signal);
extern void throwThrowable(JNIEnv *env, char *throwableClassName, const char *lpszFmt, ...);
extern jstring JNU_NewStringNative(JNIEnv *env, const char *str);
extern char* JNU_GetStringNativeChars(JNIEnv *env, jstring jstr);
#endif
