/*
 * Copyright (c) 1999, 2010 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html
 */

#ifdef CUNIT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "wrapper_i18n.h"
#include "wrapper_hashmap.h"

/**
 * The suite initialization function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite1(void) {
    return 0;
}

/**
 * The suite cleanup function.
 * Closes the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite1(void) {
    return 0;
}

/**
 * Simple test that passes.
 */
void testPass(void) {
    CU_ASSERT(0 == 0);
}

/**
 * Simple test that passes.
 */
void testFail(void) {
    CU_ASSERT(0 == 1);
}

/********************************************************************
 * Common Tools
 *******************************************************************/
TCHAR *randomChars = TEXT("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-");
#define WORK_BUFFER_LEN 4096
TCHAR workBuffer[WORK_BUFFER_LEN];

int getRandom(int min, int max) {
    int num;
    int rNum;
    
    num = max + 1 - min;
    if (num <= 0) {
        return min;
    }
    
    /* Some platforms use very large RAND_MAX values that cause overflow problems in our math */
    if (RAND_MAX > 0x10000) {
        rNum = (int)((rand() >> 8) * num / (RAND_MAX >> 8));
    } else {
        rNum = (int)(rand() * num / RAND_MAX);
    }
    
    return min + rNum;
}

/**
 * Creates a string of random characters that is within the specified range of lengths.
 * It is the responsibility of the caller to free up the string.
 *
 * @param minLen Minimum Length of the string.
 * @param maxLen Maximum Length of the string.
 *
 * @return the requested string, or NULL if out of memory.
 */
TCHAR *buildRandomString(int minLen, int maxLen) {
    int num;
    int len;
    TCHAR *str;
    int i;
    
    num = _tcslen(randomChars);
    
    len = getRandom(minLen, maxLen);
    
    str = malloc(sizeof(TCHAR) * (len + 1));
    if (!str) {
        return NULL;
    }
    
    for (i = 0; i < len; i++) {
        str[i] = randomChars[getRandom(0, num - 1)];
    }
    str[len] = TEXT('\0');
    
    return str;
}

/**
 * Creates a string of random characters that is within the specified range of lengths.
 * It is the responsibility of the caller to free up the string.
 *
 * @param minLen Minimum Length of the string.
 * @param maxLen Maximum Length of the string.
 *
 * @return the requested string, or NULL if out of memory.
 */
TCHAR *buildRandomStrinWithTail(int minLen, int maxLen, int tail) {
    int num;
    size_t len;
    size_t strLen;
    TCHAR *str;
    size_t i;
    TCHAR tailStr[32];
    
    _sntprintf(tailStr, 32, TEXT("-%d"), tail);
    
    num = _tcslen(randomChars);
    
    len = getRandom(minLen, maxLen);
    
    strLen = len + _tcslen(tailStr) + 1; 
    str = malloc(sizeof(TCHAR) * strLen);
    if (!str) {
        return NULL;
    }
    
    for (i = 0; i < len; i++) {
        str[i] = randomChars[getRandom(0, num - 1)];
    }
    str[len] = TEXT('\0');
    _tcsncat(str, tailStr, strLen);
    
    return str;
}

/**
 * Frees up an array and its contents.  Depends on the values being NULL if they are not allocated.
 *
 * @param array Array to be freed.
 */
void freeTCHARArray(TCHAR **array, int len) {
    int i;
    
    if (array) {
        for (i = 0; i < len; i++) {
            if (array[i]) {
                free(array[i]);
            }
        }
        
        free(array);
    }
}

/********************************************************************
 * Hash Map Tests
 *******************************************************************/
void hashMapCommon(int buckets, int valueCount) {
    PHashMap hashMap;
    int i;
    TCHAR **keys = NULL;
    TCHAR **values = NULL;
    const TCHAR *value;
    
    hashMap = newHashMap(buckets);
    
    if (valueCount > 0) {
        keys = malloc(sizeof(TCHAR*) * valueCount);
        if (!keys) {
            CU_FAIL(TEXT("Out of memory HMC1"));
            freeHashMap(hashMap);
            return;
        }
        memset(keys, 0, sizeof(TCHAR*) * valueCount);
        
        values = malloc(sizeof(TCHAR*) * valueCount);
        if (!values) {
            CU_FAIL(TEXT("Out of memory HMC2"));
            freeTCHARArray(keys, valueCount);
            freeHashMap(hashMap);
            return;
        }
        memset(values, 0, sizeof(TCHAR*) * valueCount);
        
        /* Generate and add key-value pairs. */
        for (i = 0; i < valueCount; i++) {
            keys[i] = buildRandomStrinWithTail(1, 20, i);
            if (!keys[i]) {
                CU_FAIL(TEXT("Out of memory HMC3"));
                freeTCHARArray(keys, valueCount);
                freeTCHARArray(values, valueCount);
                freeHashMap(hashMap);
                return;
            }
            
            values[i] = buildRandomString(1, 255);
            if (!values[i]) {
                CU_FAIL(TEXT("Out of memory HMC3"));
                freeTCHARArray(keys, valueCount);
                freeTCHARArray(values, valueCount);
                freeHashMap(hashMap);
                return;
            }
            
            hashMapPutKWVW(hashMap, keys[i], values[i]);
        }
        
#ifdef _DEBUG_HASHMAP
        dumpHashMapStats(hashMap);
#endif		
        
        /* Now check to make sure all of the values were set correctly. */
        for (i = 0; i < valueCount; i++) {
            value = hashMapGetKWVW(hashMap, keys[i]);
            if (value) {
                if (_tcscmp(values[i], value) != 0) {
                    _sntprintf(workBuffer, WORK_BUFFER_LEN, TEXT("hashMapGetKWVW(map, \"%s\") returned \"%s\" rather than expected \"%s\"."), keys[i], value, values[i]);
                    _tprintf(TEXT("%s\n"), workBuffer);
                    CU_FAIL(workBuffer);
                } else {
                    _sntprintf(workBuffer, WORK_BUFFER_LEN, TEXT("hashMapGetKWVW(map, \"%s\") returned \"%s\" as expected."), keys[i], value);
                    CU_PASS(workBuffer);
                }
            } else {
                _sntprintf(workBuffer, WORK_BUFFER_LEN, TEXT("hashMapGetKWVW(map, \"%s\") returned NULL rather than expected \"%s\"."), keys[i], values[i]);
                _tprintf(TEXT("%s\n"), workBuffer);
                CU_FAIL(workBuffer);
            }
        }
        
        /* Check for a value that will not be in the map. */
        value = hashMapGetKWVW(hashMap, TEXT("$"));
        if (value) {
            _sntprintf(workBuffer, WORK_BUFFER_LEN, TEXT("hashMapGetKWVW(map, \"$\") returned \"%s\" rather than expected NULL."), value);
            _tprintf(TEXT("%s\n"), workBuffer);
            CU_FAIL(workBuffer);
        } else {
            _sntprintf(workBuffer, WORK_BUFFER_LEN, TEXT("hashMapGetKWVW(map, \"$\") returned NULL as expected."));
            CU_PASS(workBuffer);
        }
        
        freeTCHARArray(keys, valueCount);
        freeTCHARArray(values, valueCount);
    }
    
    freeHashMap(hashMap);
}

/**
 * Make sure we can create and destroy an empty hash map.
 */
void testHashMapEmpty() {
    hashMapCommon(100, 0);
}

/**
 * Make sure we can create and destroy an sparsely filled hash map that has many empty buckets.
 */
void testHashMapSparse() {
    hashMapCommon(100, 10);
}

/**
 * Make sure we can create and destroy an sparsely filled hash map that has many empty buckets.
 */
void testHashMapLarge() {
    hashMapCommon(100, 10000);
}

/* The main() function for setting up and running the tests.
 * Returns a CUE_SUCCESS on successful running, another
 * CUnit error code on failure.
 */
int main()
{
    CU_pSuite pSuite = NULL;
    CU_pSuite hashMapSuite = NULL;
    
    /* Initialize the random seed. */
    srand((unsigned)time(NULL));
    
    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();
    
    /* add a suite to the registry */
    pSuite = CU_add_suite("Suite_1", init_suite1, clean_suite1);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    hashMapSuite = CU_add_suite("HashMap Suite", NULL, NULL);
    if (NULL == hashMapSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    /* add the tests to the suite */
    /*
    if ((NULL == CU_add_test(pSuite, "test of pass", testPass)) ||
        (NULL == CU_add_test(pSuite, "test of fail", testFail)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    */
    
    CU_add_test(hashMapSuite, "empty HashMap", testHashMapEmpty);
    CU_add_test(hashMapSuite, "sparce HashMap", testHashMapSparse);
    CU_add_test(hashMapSuite, "large HashMap", testHashMapLarge);
    
    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

#endif
