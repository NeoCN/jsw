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
 * Revision 1.12  2003/11/02 20:29:29  mortenson
 * Add the ability to get information about the user account which is running the
 * Wrapper as well as the user account with which the Wrapper is interacting.
 *
 * Revision 1.11  2003/10/31 11:10:46  mortenson
 * Add a getLastErrorText function so we can display more user friendly messages
 * within the native library.
 *
 * Revision 1.10  2003/10/31 05:59:34  mortenson
 * Added a new method, setConsoleTitle, to the WrapperManager class which
 * enables the application to dynamically set the console title.
 *
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
#include <time.h>
#include <tlhelp32.h>
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

/**
 * Looks up the name of the explorer.exe file in the registry.  It may change
 *  in a future version of windows, so this is the safe thing to do.
 */
char explorerExe[1024];
void
initExplorerExeName() {
	/* Location: "\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Shell" */
	sprintf(explorerExe, "Explorer.exe");
}

BOOL GetTextualSid(
    PSID pSid,            // binary Sid
    LPTSTR TextualSid,    // buffer for Textual representation of Sid
    LPDWORD lpdwBufferLen // required/provided TextualSid buffersize
    )
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    // Validate the binary SID.

    if(!IsValidSid(pSid)) return FALSE;

    // Get the identifier authority value from the SID.

    psia = GetSidIdentifierAuthority(pSid);

    // Get the number of subauthorities in the SID.

    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    // Compute the buffer length.
    // S-SID_REVISION- + IdentifierAuthority- + subauthorities- + NULL

    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    // Check input buffer length.
    // If too small, indicate the proper size and set last error.

    if (*lpdwBufferLen < dwSidSize)
    {
        *lpdwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Add 'S' prefix and revision number to the string.

    dwSidSize=wsprintf(TextualSid, TEXT("S-%lu-"), dwSidRev );

    // Add SID identifier authority to the string.

    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    }
    else
    {
        dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                    TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    // Add SID subauthorities to the string.
    //
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        dwSidSize+=wsprintf(TextualSid + dwSidSize, TEXT("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }

    return TRUE;
}

void
dumpSIDAccount(SID *sid) {
	TCHAR sidText[1024];
	DWORD sidTextSize;
	TCHAR userName[1024];
	TCHAR domainName[1024];
	DWORD userNameSize;
	DWORD domainNameSize;
	SID_NAME_USE sidType;

	sidTextSize = sizeof(sidText);
	GetTextualSid(sid, sidText, &sidTextSize);

	userNameSize = sizeof(userName);
	domainNameSize = sizeof(domainName);
	if (LookupAccountSid(NULL, sid, userName, &userNameSize, domainName, &domainNameSize, &sidType)) {
		printf("    Sid: %s = %s/%s\n", sidText, domainName, userName);
		flushall();
	} else {
		printf("    Unable to locate account for Sid: %s : %s\n", sidText, getLastErrorText());
		flushall();
	}
}

/**
 * Converts a FILETIME to a time_t structure.
 */
time_t
fileTimeToTimeT(FILETIME *filetime) {
	SYSTEMTIME utc;
	SYSTEMTIME local;
	TIME_ZONE_INFORMATION timeZoneInfo;
	struct tm tm;

	FileTimeToSystemTime(filetime, &utc);
	GetTimeZoneInformation(&timeZoneInfo);
	SystemTimeToTzSpecificLocalTime(&timeZoneInfo, &utc, &local);

	tm.tm_sec = local.wSecond;
	tm.tm_min = local.wMinute;
	tm.tm_hour = local.wHour;
	tm.tm_mday = local.wDay;
	tm.tm_mon = local.wMonth - 1;
	tm.tm_year = local.wYear - 1900;
	tm.tm_wday = local.wDayOfWeek;
	tm.tm_yday = -1;
	tm.tm_isdst = -1;
	return mktime(&tm);
}

/**
 * Looks for the login time given a user SID.  The login time is found by looking
 *  up the SID in the registry.
 */
time_t
getUserLoginTime(TCHAR *sidText) {
	HKEY     userKey;
	int      i;
	TCHAR    userKeyName[MAX_PATH];
	DWORD    userKeyNameSize;
	FILETIME lastTime;
	time_t   loginTime;

	loginTime = 0;

	/* Open a key to the HKRY_USERS registry. */
	if (RegOpenKey(HKEY_USERS, NULL, &userKey) != ERROR_SUCCESS) {
		printf("Error opening registry for HKEY_USERS: %s\n", getLastErrorText());
        flushall();
		return loginTime;
	}

	/* Loop over the users */
	i = 0;
	userKeyNameSize = sizeof(userKeyName);
	while (RegEnumKeyEx(userKey, i, userKeyName, &userKeyNameSize, NULL, NULL, NULL, &lastTime) == ERROR_SUCCESS) {
		if (stricmp(sidText, userKeyName) == 0) {
			/* We found the SID! */

			/* Convert the FILETIME to UNIX time. */
			loginTime = fileTimeToTimeT(&lastTime);

			break;
		}

		userKeyNameSize = sizeof(userKeyName);
		i++;
	}

	/* Always close the userKey. */
	RegCloseKey(userKey);

	return loginTime;
}

void
setUserGroups(JNIEnv *env, jclass wrapperUserClass, jobject wrapperUser, HANDLE hProcessToken) {
	jmethodID addGroup;

	TOKEN_GROUPS *tokenGroups;
	DWORD tokenGroupsSize;
	DWORD i;

	DWORD sidTextSize;
	TCHAR *sidText;
	TCHAR *groupName;
	DWORD groupNameSize;
	TCHAR *domainName;
	DWORD domainNameSize;
	SID_NAME_USE sidType;

	jbyteArray jSID;
	jbyteArray jGroupName;
	jbyteArray jDomainName;

	/* Look for the method used to add groups to the user. */
	if (addGroup = (*env)->GetMethodID(env, wrapperUserClass, "addGroup", "([B[B[B)V")) {
		/* Get the TokenGroups info from the token. */
		GetTokenInformation(hProcessToken, TokenGroups, NULL, 0, &tokenGroupsSize);
		tokenGroups = (TOKEN_GROUPS *)malloc(tokenGroupsSize);
		if (GetTokenInformation(hProcessToken, TokenGroups, tokenGroups, tokenGroupsSize, &tokenGroupsSize)) {
			/* Loop over each of the groups and add each one to the user. */
			for (i = 0; i < tokenGroups->GroupCount; i++) {
				/* Get the text representation of the sid. */
				sidTextSize = 0;
				GetTextualSid(tokenGroups->Groups[i].Sid, NULL, &sidTextSize);
				sidText = (TCHAR*)malloc(sizeof(TCHAR) * sidTextSize);
				GetTextualSid(tokenGroups->Groups[i].Sid, sidText, &sidTextSize);
				
				/* We now have an SID, use it to lookup the account. */
				groupNameSize = 0;
				domainNameSize = 0;
				LookupAccountSid(NULL, tokenGroups->Groups[i].Sid, NULL, &groupNameSize, NULL, &domainNameSize, &sidType);
				groupName = (TCHAR*)malloc(sizeof(TCHAR) * groupNameSize);
				domainName = (TCHAR*)malloc(sizeof(TCHAR) * domainNameSize);
				if (LookupAccountSid(NULL, tokenGroups->Groups[i].Sid, groupName, &groupNameSize, domainName, &domainNameSize, &sidType)) {
					/*printf("SID=%s, group=%s/%s\n", sidText, domainName, groupName);*/

					/* Create the arguments to the constructor as java objects */

					/* SID byte array */
					jSID = (*env)->NewByteArray(env, strlen(sidText));
					(*env)->SetByteArrayRegion(env, jSID, 0, strlen(sidText), sidText);

					/* GroupName byte array */
					jGroupName = (*env)->NewByteArray(env, strlen(groupName));
					(*env)->SetByteArrayRegion(env, jGroupName, 0, strlen(groupName), groupName);

					/* DomainName byte array */
					jDomainName = (*env)->NewByteArray(env, strlen(domainName));
					(*env)->SetByteArrayRegion(env, jDomainName, 0, strlen(domainName), domainName);

					/* Now actually add the group to the user. */
					(*env)->CallVoidMethod(env, wrapperUser, addGroup, jSID, jGroupName, jDomainName);
				} else {
					/* This is normal as some accounts do not seem to be mappable. */
					/*
					printf("Unable to locate account for Sid, %s: %s\n", sidText, getLastErrorText());
					flushall();
					*/
				}
				free(sidText);
				free(groupName);
				free(domainName);
			}
		} else {
			printf("Unable to get token information: %s\n", getLastErrorText());
			flushall();
		}

		free(tokenGroups);
	}
}

/**
 * Creates and returns a WrapperUser instance to represent the user who owns
 *  the specified process Id.
 */
jobject
createWrapperUserForProcess(JNIEnv *env, DWORD processId) {
	HANDLE hProcess;
	HANDLE hProcessToken;
	TOKEN_USER *tokenUser;
	DWORD tokenUserSize;

	DWORD sidTextSize;
	TCHAR *sidText;
	TCHAR *userName;
	DWORD userNameSize;
	TCHAR *domainName;
	DWORD domainNameSize;
	SID_NAME_USE sidType;
	time_t loginTime;

	jclass wrapperUserClass;
	jmethodID constructor;
	jbyteArray jSID;
	jbyteArray jUserName;
	jbyteArray jDomainName;
	jobject wrapperUser = NULL;

	if (hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId)) {
		if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hProcessToken)) {
			GetTokenInformation(hProcessToken, TokenUser, NULL, 0, &tokenUserSize);
			tokenUser = (TOKEN_USER *)malloc(tokenUserSize);
			if (GetTokenInformation(hProcessToken, TokenUser, tokenUser, tokenUserSize, &tokenUserSize)) {

				/* Get the text representation of the sid. */
				sidTextSize = 0;
				GetTextualSid(tokenUser->User.Sid, NULL, &sidTextSize);
				sidText = (TCHAR*)malloc(sizeof(TCHAR) * sidTextSize);
				GetTextualSid(tokenUser->User.Sid, sidText, &sidTextSize);

				/* We now have an SID, use it to lookup the account. */
				userNameSize = 0;
				domainNameSize = 0;
				LookupAccountSid(NULL, tokenUser->User.Sid, NULL, &userNameSize, NULL, &domainNameSize, &sidType);
				userName = (TCHAR*)malloc(sizeof(TCHAR) * userNameSize);
				domainName = (TCHAR*)malloc(sizeof(TCHAR) * domainNameSize);
				if (LookupAccountSid(NULL, tokenUser->User.Sid, userName, &userNameSize, domainName, &domainNameSize, &sidType)) {

					/* Get the time that this user logged in. */
					loginTime = getUserLoginTime(sidText);

					/* Look for the WrapperUser class. Ignore failures as JNI throws an exception. */
					if (wrapperUserClass = (*env)->FindClass(env, "org/tanukisoftware/wrapper/WrapperWin32User")) {

						/* Look for the constructor. Ignore failures. */
						if (constructor = (*env)->GetMethodID(env, wrapperUserClass, "<init>", "([B[B[BI)V")) {

							/* Create the arguments to the constructor as java objects */

							/* SID byte array */
							jSID = (*env)->NewByteArray(env, strlen(sidText));
							(*env)->SetByteArrayRegion(env, jSID, 0, strlen(sidText), sidText);

							/* UserName byte array */
							jUserName = (*env)->NewByteArray(env, strlen(userName));
							(*env)->SetByteArrayRegion(env, jUserName, 0, strlen(userName), userName);

							/* DomainName byte array */
							jDomainName = (*env)->NewByteArray(env, strlen(domainName));
							(*env)->SetByteArrayRegion(env, jDomainName, 0, strlen(domainName), domainName);

							/* Now create the new wrapperUser using the constructor arguments collected above. */
							wrapperUser = (*env)->NewObject(env, wrapperUserClass, constructor, jSID, jUserName, jDomainName, loginTime);

							setUserGroups(env, wrapperUserClass, wrapperUser, hProcessToken);
						}
					}
				} else {
					/* This is normal as some accounts do not seem to be mappable. */
					/*
					printf("Unable to locate account for Sid, %s: %s\n", sidText, getLastErrorText());
					flushall();
					*/
				}
				free(sidText);
				free(userName);
				free(domainName);
			} else {
				printf("Unable to get token information: %s\n", getLastErrorText());
				flushall();
			}
			free(tokenUser);
		} else {
			printf("Unable to open process token: %s\n", getLastErrorText());
			flushall();
		}

		CloseHandle(hProcess);
	} else {
		printf("Unable to open process: %s\n", getLastErrorText());
		flushall();
	}

	return wrapperUser;
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

	/* Initialize the explorer.exe name. */
	initExplorerExeName();
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
        printf("Unable to send BREAK event to JVM process: %s\n", getLastErrorText());
        flushall();
    }
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeSetConsoleTitle
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeSetConsoleTitle(JNIEnv *env, jclass clazz, jbyteArray jTitleBytes) {
    jbyte *titleBytes = (*env)->GetByteArrayElements(env, jTitleBytes, 0);

    if (wrapperJNIDebugging) {
        printf("Setting the console title to: %s\n", titleBytes);
        flushall();
    }

    SetConsoleTitle(titleBytes);

    (*env)->ReleaseByteArrayElements(env, jTitleBytes, titleBytes, JNI_ABORT);
}

WINADVAPI
HDESK
WINAPI
GetInputDesktop();

BOOL CALLBACK
enumDesktopProc(LPTSTR lpszDesktop, LPARAM lParam) {
	HDESK desktop;
	DWORD sidBufferSize;
	char *sidBuffer;
	//DWORD sidTextSize;
	//char sidText[1024];

	printf("  enum desktop %s\n", lpszDesktop);
	flushall();

	if (desktop = OpenDesktop(lpszDesktop, 0, FALSE, MAXIMUM_ALLOWED)) {
		GetUserObjectInformation(desktop, UOI_USER_SID, NULL, 0, &sidBufferSize);
		sidBuffer = (char*)malloc(sizeof(char) * sidBufferSize);
		if (GetUserObjectInformation(desktop, UOI_USER_SID, sidBuffer, sidBufferSize, &sidBufferSize)) {
			//sidTextSize = sizeof(sidText);
			//GetTextualSid((SID *)sidBuffer, sidText, &sidTextSize);
			//printf("    desktop SID => %s\n", sidText);
			//flushall();

			dumpSIDAccount((SID *)sidBuffer);
		} else {
			printf("Unable to retrieve desktop User Information: %s\n", getLastErrorText());
			flushall();
		}

		free(sidBuffer);

		CloseDesktop(desktop);
	} else {
		printf("Unable to open the desktop: %s\n", getLastErrorText());
		flushall();
	}

	return TRUE;
}

BOOL CALLBACK
enumWindowStationProc(LPTSTR lpszWindowStation, LPARAM lParam) {
	HWINSTA winStation;

	printf(" enum station %s\n", lpszWindowStation);
	flushall();

	if (winStation = OpenWindowStation(lpszWindowStation, FALSE, MAXIMUM_ALLOWED)) {
		EnumDesktops(winStation, enumDesktopProc, 0);

		CloseWindowStation(winStation);
	} else {
		printf("Unable to open the window station: %s\n", getLastErrorText());
		flushall();
	}

	return TRUE;
}

void
dumpDesktops() {
	EnumWindowStations(enumWindowStationProc, 0);
}

void
dumpUserDesktops() {
	char *explorerExe;
	HANDLE snapshot;
	PROCESSENTRY32 processEntry;
	THREADENTRY32 threadEntry;
	BOOL foundThread;
	HDESK desktop;
	DWORD sidBufferSize;
	char *sidBuffer;
	DWORD sidTextSize;
	char sidText[1024];

	HANDLE hProcess;
	HANDLE hProcessToken;
	TOKEN_USER *tokenUser;
	DWORD tokenUserSize;

	// Get the name of the explorer executable from the registry.
	explorerExe = "Explorer.exe";

	if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS + TH32CS_SNAPTHREAD, 0)) >= 0) {
		processEntry.dwSize = sizeof(processEntry);
		if (Process32First(snapshot, &processEntry)) {
			do {
				// We are only interrested in the Explorer processes.
				if (stricmp(explorerExe, processEntry.szExeFile) == 0) {
					printf("Process size=%ld, cnt=%ld, id=%ld, parentId=%ld, moduleId=%ld, threads=%ld, exe=%s\n",
						processEntry.dwSize, processEntry.cntUsage, processEntry.th32ProcessID,
						processEntry.th32ParentProcessID, processEntry.th32ModuleID, processEntry.cntThreads,
						processEntry.szExeFile);
					flushall();

					if (hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processEntry.th32ProcessID)) {
						if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hProcessToken)) {
							GetTokenInformation(hProcessToken, TokenUser, NULL, 0, &tokenUserSize);
							tokenUser = (TOKEN_USER *)malloc(tokenUserSize);
							if (GetTokenInformation(hProcessToken, TokenUser, tokenUser, tokenUserSize, &tokenUserSize)) {
								dumpSIDAccount(tokenUser->User.Sid);
							} else {
								printf("Unable to get token information %ld %ld: %s\n", sizeof(tokenUser), tokenUserSize, getLastErrorText());
								flushall();
							}
							free(tokenUser);
						} else {
							printf("Unable to open process token: %s\n", getLastErrorText());
							flushall();
						}

						CloseHandle(hProcess);
					} else {
						printf("Unable to open process: %s\n", getLastErrorText());
						flushall();
					}

					// Now look for a thread which is owned by the explorer process.
					threadEntry.dwSize = sizeof(threadEntry);
					if (Thread32First(snapshot, &threadEntry)) {
						foundThread = FALSE;
						do {
							// We are only interrested in threads that belong to the current Explorer process.
							if (threadEntry.th32OwnerProcessID == processEntry.th32ProcessID) {
								printf("  Thread id=%ld\n", threadEntry.th32ThreadID);
								flushall();

								if (desktop = GetThreadDesktop(threadEntry.th32ThreadID)) {
									GetUserObjectInformation(desktop, UOI_USER_SID, NULL, 0, &sidBufferSize);
									sidBuffer = (char*)malloc(sizeof(char) * sidBufferSize);
									if (GetUserObjectInformation(desktop, UOI_USER_SID, sidBuffer, sidBufferSize, &sidBufferSize)) {
										sidTextSize = sizeof(sidText);
										GetTextualSid((SID *)sidBuffer, sidText, &sidTextSize);
										printf("    desktop SID => %s\n", sidText);
										flushall();
									} else {
										printf("Unable to retrieve desktop User Information: %s\n", getLastErrorText());
										flushall();
									}

									free(sidBuffer);
									CloseDesktop(desktop);
								} else {
									printf("GetThreadDesktop failed: %s\n", getLastErrorText());
									flushall();
								}

								// We only need the first thread, so break
								foundThread = TRUE;
								break;
							}
						} while (Thread32Next(snapshot, &threadEntry));

						if (!foundThread && (GetLastError() != ERROR_NO_MORE_FILES)) {
							printf("Unable to get next thread entry: %s\n", getLastErrorText());
							flushall();
						}
					} else if (GetLastError() != ERROR_NO_MORE_FILES) {
						printf("Unable to get first thread entry: %s\n", getLastErrorText());
						flushall();
					}
				}
			} while (Process32Next(snapshot, &processEntry));

			if (GetLastError() != ERROR_NO_MORE_FILES) {
				printf("Unable to get next process entry: %s\n", getLastErrorText());
				flushall();
			}
		} else if (GetLastError() != ERROR_NO_MORE_FILES) {
			printf("Unable to get first process entry: %s\n", getLastErrorText());
			flushall();
		}

		CloseHandle(snapshot);
	} else {
        printf("Toolhelp snapshot failed: %s\n", getLastErrorText());
        flushall();
	}
}

/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeGetUser
 * Signature: ()Lorg/tanukisoftware/wrapper/WrapperUser;
 */
/*#define UVERBOSE*/
JNIEXPORT jobject JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeGetUser(JNIEnv *env, jclass clazz) {
	DWORD processId;

#ifdef UVERBOSE
	printf("nativeGetUser()\n");
	flushall();
#endif

	/* Get the current processId. */
	processId = GetCurrentProcessId();

	return createWrapperUserForProcess(env, processId);
}


/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeGetInteractiveUser
 * Signature: ()Lorg/tanukisoftware/wrapper/WrapperUser;
 */
/*#define IUVERBOSE*/
JNIEXPORT jobject JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeGetInteractiveUser(JNIEnv *env, jclass clazz) {
	HANDLE snapshot;
	PROCESSENTRY32 processEntry;
	THREADENTRY32 threadEntry;
	BOOL foundThread;
	HDESK desktop;

	jobject wrapperUser = NULL;

#ifdef IUVERBOSE
	printf("nativeGetInteractiveUser()\n");
	flushall();
#endif

	/* In order to be able to return the interactive user, we first need to locate the
	 *  logged on user whose desktop we are able to open.  On XP systems, there will be
	 *  more than one user with a desktop, but only the first one to log on will allow
	 *  up to open its desktop.  On all NT systems, there will be additional logged on
	 *  users if there are other services running. */
	if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS + TH32CS_SNAPTHREAD, 0)) >= 0) {
		processEntry.dwSize = sizeof(processEntry);
		if (Process32First(snapshot, &processEntry)) {
			do {
				// We are only interrested in the Explorer processes.
				if (stricmp(explorerExe, processEntry.szExeFile) == 0) {
#ifdef IUVERBOSE
					printf("Process size=%ld, cnt=%ld, id=%ld, parentId=%ld, moduleId=%ld, threads=%ld, exe=%s\n",
						processEntry.dwSize, processEntry.cntUsage, processEntry.th32ProcessID,
						processEntry.th32ParentProcessID, processEntry.th32ModuleID, processEntry.cntThreads,
						processEntry.szExeFile);
					flushall();
#endif

					/* Now look for a thread which is owned by the explorer process. */
					threadEntry.dwSize = sizeof(threadEntry);
					if (Thread32First(snapshot, &threadEntry)) {
						foundThread = FALSE;
						do {
							/* We are only interrested in threads that belong to the current Explorer process. */
							if (threadEntry.th32OwnerProcessID == processEntry.th32ProcessID) {
#ifdef IUVERBOSE
								printf("  Thread id=%ld\n", threadEntry.th32ThreadID);
								flushall();
#endif

								// We have a thread, now see if we can gain access to its desktop
								if (desktop = GetThreadDesktop(threadEntry.th32ThreadID)) {
									/* We got the desktop!   We now know that this is the thread and thus
									 *  process that we have been looking for.   Unfortunately it does not
									 *  appear that we can get the Sid of the account directly from this
									 *  desktop.  I tried using GetUserObjectInformation, but the Sid
									 *  returned does not seem to map to a valid account. */

									wrapperUser = createWrapperUserForProcess(env, processEntry.th32ProcessID);
								} else {
#ifdef IUVERBOSE
									printf("GetThreadDesktop failed: %s\n", getLastErrorText());
									flushall();
#endif
								}

								/* We only need the first thread, so break */
								foundThread = TRUE;
								break;
							}
						} while (Thread32Next(snapshot, &threadEntry));

						if (!foundThread && (GetLastError() != ERROR_NO_MORE_FILES)) {
#ifdef IUVERBOSE
							printf("Unable to get next thread entry: %s\n", getLastErrorText());
							flushall();
#endif
						}
					} else if (GetLastError() != ERROR_NO_MORE_FILES) {
						printf("Unable to get first thread entry: %s\n", getLastErrorText());
						flushall();
					}
				}
			} while (Process32Next(snapshot, &processEntry));

#ifdef IUVERBOSE
			if (GetLastError() != ERROR_NO_MORE_FILES) {
				printf("Unable to get next process entry: %s\n", getLastErrorText());
				flushall();
			}
#endif
		} else if (GetLastError() != ERROR_NO_MORE_FILES) {
			printf("Unable to get first process entry: %s\n", getLastErrorText());
			flushall();
		}

		CloseHandle(snapshot);
	} else {
        printf("Toolhelp snapshot failed: %s\n", getLastErrorText());
        flushall();
	}

	return wrapperUser;
}


/*
 * Class:     org_tanukisoftware_wrapper_WrapperManager
 * Method:    nativeGetLoggedOnUser
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL
Java_org_tanukisoftware_wrapper_WrapperManager_nativeGetLoggedOnUser(JNIEnv *env, jclass clazz) {
	HKEY     userKey;
	int      i;
	TCHAR    userKeyName[MAX_PATH];
	DWORD    userKeyNameSize;
	FILETIME lastTime;
	int      tokenCount;
	int      revision;
	DWORD    identifierAuthorityVal;
	SID_IDENTIFIER_AUTHORITY identifierAuthority;
	BYTE     subAuthorityCount;
	DWORD    subAuthority0, subAuthority1, subAuthority2, subAuthority3, subAuthority4, subAuthority5, subAuthority6, subAuthority7;
	SID      *userSID;

	TCHAR    userName[1024];
	TCHAR    userName2[1024];
	DWORD    userNameSize;
	TCHAR    domainName[1024];
	TCHAR    domainName2[1024];
	DWORD    domainNameSize;
	SID_NAME_USE sidType;
	SYSTEMTIME loginTime;

	HWINSTA winStation;
	HDESK desktop;
	char *buffer;
	DWORD needed;

	DWORD sidBufferSize;
	char sidBuffer[1024];

	//wrapperJNIDebugging = TRUE;
    if (wrapperJNIDebugging) {
        printf("Looking for Logged on users.\n");
        flushall();
    }

	// Open a key to the HKRY_USERS registry.
	if (RegOpenKey(HKEY_USERS, NULL, &userKey) != ERROR_SUCCESS) {
		printf("Error opening registry for HKEY_USERS: %s\n", getLastErrorText());
        flushall();
		return NULL;
	}
	
	// Loop over the users
	i = 0;
	userKeyNameSize = sizeof( userKeyName );
	while (RegEnumKeyEx(userKey, i, userKeyName, &userKeyNameSize, NULL, NULL, NULL, &lastTime) == ERROR_SUCCESS) {
		// Ignore non user keys
		if ((!strstr(userKeyName, "Classes")) && (!strstr(userKeyName, "DEFAULT"))) {
		    //if (wrapperJNIDebugging) {
				printf("  Testing SID: %s\n", userKeyName);
				flushall();
			//}

			// Extract an SID from the name
			tokenCount = sscanf(userKeyName, "S-%d-%x-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu",
				&revision, &identifierAuthorityVal,
				&subAuthority0, &subAuthority1, &subAuthority2, &subAuthority3,
				&subAuthority4, &subAuthority5, &subAuthority6, &subAuthority7);

			// The SID is only valid if at least one sub authority exists.
			if (tokenCount > 2) {
				// Convert the identifier authority value

				identifierAuthority.Value[0] = 0;
				identifierAuthority.Value[1] = 0;
				identifierAuthority.Value[2] = (BYTE)((identifierAuthorityVal >> 24) & 0xff);
				identifierAuthority.Value[3] = (BYTE)((identifierAuthorityVal >> 16) & 0xff);
				identifierAuthority.Value[4] = (BYTE)((identifierAuthorityVal >> 8) & 0xff);
				identifierAuthority.Value[5] = (BYTE)(identifierAuthorityVal & 0xff);

				// Allocate the SID
				subAuthorityCount = tokenCount - 2;
				userSID = NULL;
				if (AllocateAndInitializeSid(&identifierAuthority, subAuthorityCount,
					subAuthority0, subAuthority1, subAuthority2, subAuthority3,
					subAuthority4, subAuthority5, subAuthority6, subAuthority7, &userSID)) {

					sidBufferSize = sizeof(sidBuffer);
					GetTextualSid(userSID, sidBuffer, &sidBufferSize);
					printf("    => %s\n", sidBuffer);
					flushall();

					// Using the SID, lookup the account information
					userNameSize = sizeof(userName);
					domainNameSize = sizeof(domainName);
					if (LookupAccountSid(NULL, userSID, userName, &userNameSize, domainName, &domainNameSize, &sidType)) {
						// We are only interested in user accounts.
						if (sidType == SidTypeUser) {
							// We found a logged in user.  What a journey!
							FileTimeToLocalFileTime(&lastTime, &lastTime);
							FileTimeToSystemTime(&lastTime, &loginTime);
							//if (wrapperJNIDebugging) {
								printf("  %s/%s logged in at %04d/%02d/%02d %02d:%02d:%02d\n",
									domainName, userName,
									loginTime.wYear, loginTime.wMonth, loginTime.wDay,
									loginTime.wHour, loginTime.wMinute, loginTime.wSecond);
								flushall();
							//}

							if (winStation = OpenWindowStation("WinSta0", FALSE, MAXIMUM_ALLOWED)) {
								printf("Got the station: %p.\n", winStation);
								flushall();
								if (desktop = GetInputDesktop()) {
								//if (desktop = OpenDesktop("Default", 0, FALSE, MAXIMUM_ALLOWED)) {
								//if (desktop = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED)) {
									printf("Got the desktop: %p.\n", desktop);
									flushall();

									// Find out who owns the desktop by requesting the SID, we first need to find out how big the SID is
									GetUserObjectInformation(desktop, UOI_USER_SID, NULL, 0, &needed);
									buffer = (char*)malloc(sizeof(char) * needed);
									if (GetUserObjectInformation(desktop, UOI_USER_SID, buffer, needed, &needed)) {
										sidBufferSize = sizeof(sidBuffer);
										GetTextualSid((SID *)buffer, sidBuffer, &sidBufferSize);
										printf("    desktop SID => %s\n", sidBuffer);
										flushall();

										// Now lookup the account name and domain of this SID
										userNameSize = sizeof(userName2);
										domainNameSize = sizeof(domainName2);
										if (LookupAccountSid(NULL, (SID *)buffer, userName2, &userNameSize, domainName2, &domainNameSize, &sidType)) {
											printf("    Desktop owner: %s/%s\n", domainName2, userName2);
											flushall();
										} else {
											printf("Unable to locate account for desktop Sid: %d %d %s\n", ERROR_NONE_MAPPED, GetLastError(), getLastErrorText());
											flushall();
										}
									} else {
										printf("Unable to retrieve the User Information: %s\n", getLastErrorText());
										flushall();
									}
									free(buffer);

									CloseDesktop(desktop);
								} else {
									printf("Unable to open the desktop: %s\n", getLastErrorText());
									flushall();
								}
								CloseWindowStation(winStation);
							} else {
								printf("Unable to open the window station: %s\n", getLastErrorText());
								flushall();
							}
						}
					} else {
						// The account could not be located.  It is normal for this to happen for some accounts.
						//printf("Unable to locate account for Sid, %s: %s\n", userKeyName, getLastErrorText());
						//flushall();
					}

					// Always free up the SID
					FreeSid(userSID);
				} else {
					printf("Unable to initialize an Sid: %s\n", getLastErrorText());
					flushall();
					break;
				}
			}
		}

		userKeyNameSize = sizeof(userKeyName);
		i++;
	}

	// Always close the userKey.
	RegCloseKey(userKey);

	dumpUserDesktops();
	//dumpDesktops();

	return NULL;
}

#endif
