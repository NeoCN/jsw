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

#if defined(MACOSX) || defined(FREEBSD)
#else
#include <malloc.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef WIN32

/* MS Visual Studio 8 went and deprecated the POXIX names for functions.
 *  Fixing them all would be a big headache for UNIX versions. */
#pragma warning(disable : 4996)

#else
#include <strings.h>
#include <limits.h>
#include <sys/time.h>
#if defined(IRIX)
#define PATH_MAX FILENAME_MAX
#endif
#endif

#include "logger.h"
#include "property.h"
#include "wrapper.h"

#define MAX_INCLUDE_DEPTH 10

int debugIncludes = FALSE;

/** Stores the time that the property file began to be loaded. */
struct tm loadPropertiesTM;

const char **escapedPropertyNames = NULL;

void setInnerProperty(Property *property, const char *propertyValue);

void prepareProperty(Property *property) {
    char *oldValue;

    if (strstr(property->value, "%")) {
        /* Reset the property.  If the unreplaced environment variables are now available
         *  setting it again will cause it to be replaced correctly.  If not this will
         *  only waste time.  The value will be freed in the process so we need to
         *  keep it around. */
#ifdef _DEBUG
        printf("Unreplaced property %s=%s\n", property->name, property->value);
#endif
        oldValue = malloc(strlen(property->value) + 1);
        if (!oldValue) {
            outOfMemory("PP", 1);
        } else {
            strcpy(oldValue, property->value);
            setInnerProperty(property, oldValue);
            free(oldValue);
        }
#ifdef _DEBUG
        printf("        -> property %s=%s\n", property->name, property->value);
#endif
    }
}

/**
 * Private function to find a Property structure.
 */
Property* getInnerProperty(Properties *properties, const char *propertyName) {
    Property *property;
    int cmp;

    /* Loop over the properties which are in order and look for the specified property. */
    property = properties->first;
    while (property != NULL) {
        cmp = strcmpIgnoreCase(property->name, propertyName);
        if (cmp > 0) {
            /* This property would be after the one being looked for, so it does not exist. */
            return NULL;
        } else if (cmp == 0) {
            /* We found it. */
            prepareProperty(property);
            return property;
        }
        /* Keep looking */
        property = property->next;
    }
    /* We did not find the property being looked for. */
    return NULL;
}

void insertInnerProperty(Properties *properties, Property *newProperty) {
    Property *property;
    int cmp;

    /* Loop over the properties which are in order and look for the specified property. */
    /* This function assumes that Property is not already in properties. */
    property = properties->first;
    while (property != NULL) {
        cmp = strcmpIgnoreCase(property->name, newProperty->name);
        if (cmp > 0) {
            /* This property would be after the new property, so insert it here. */
            newProperty->previous = property->previous;
            newProperty->next = property;
            if (property->previous == NULL) {
                /* This was the first property */
                properties->first = newProperty;
            } else {
                property->previous->next = newProperty;
            }
            property->previous = newProperty;

            /* We are done, so return */
            return;
        }

        property = property->next;
    }

    /* The new property needs to be added at the end */
    newProperty->previous = properties->last;
    if (properties->last == NULL) {
        /* This will be the first property. */
        properties->first = newProperty;
    } else {
        /* Point the old last property to the new last property. */
        properties->last->next = newProperty;
    }
    properties->last = newProperty;
    newProperty->next = NULL;
}

Property* createInnerProperty() {
    Property *property;

    property = malloc(sizeof(Property));
    if (!property) {
        outOfMemory("CIP", 1);
        return NULL;
    }
    property->name = NULL;
    property->next = NULL;
    property->previous = NULL;
    property->value = NULL;

    return property;
}

/**
 * Private function to dispose a Property structure.  Assumes that the
 *    Property is disconnected already.
 */
void disposeInnerProperty(Property *property) {
    if (property->name) {
        free(property->name);
    }
    if (property->value) {
        free(property->value);
    }
    free(property);
}

char generateValueBuffer[256];

char* generateTimeValue(const char* format) {
    if (strcmpIgnoreCase(format, "YYYYMMDDHHIISS") == 0) {
        sprintf(generateValueBuffer, "%04d%02d%02d%02d%02d%02d",
        loadPropertiesTM.tm_year + 1900, loadPropertiesTM.tm_mon + 1, loadPropertiesTM.tm_mday,
        loadPropertiesTM.tm_hour, loadPropertiesTM.tm_min, loadPropertiesTM.tm_sec);
    } else if (strcmpIgnoreCase(format, "YYYYMMDD_HHIISS") == 0) {
        sprintf(generateValueBuffer, "%04d%02d%02d_%02d%02d%02d",
        loadPropertiesTM.tm_year + 1900, loadPropertiesTM.tm_mon + 1, loadPropertiesTM.tm_mday,
        loadPropertiesTM.tm_hour, loadPropertiesTM.tm_min, loadPropertiesTM.tm_sec);
    } else if (strcmpIgnoreCase(format, "YYYYMMDDHHII") == 0) {
        sprintf(generateValueBuffer, "%04d%02d%02d%02d%02d",
        loadPropertiesTM.tm_year + 1900, loadPropertiesTM.tm_mon + 1, loadPropertiesTM.tm_mday,
        loadPropertiesTM.tm_hour, loadPropertiesTM.tm_min);
    } else if (strcmpIgnoreCase(format, "YYYYMMDDHH") == 0) {
        sprintf(generateValueBuffer, "%04d%02d%02d%02d",
        loadPropertiesTM.tm_year + 1900, loadPropertiesTM.tm_mon + 1, loadPropertiesTM.tm_mday,
        loadPropertiesTM.tm_hour);
    } else if (strcmpIgnoreCase(format, "YYYYMMDD") == 0) {
        sprintf(generateValueBuffer, "%04d%02d%02d",
        loadPropertiesTM.tm_year + 1900, loadPropertiesTM.tm_mon + 1, loadPropertiesTM.tm_mday);
    } else {
        sprintf(generateValueBuffer, "{INVALID}");
    }
    return generateValueBuffer;
}

char* generateRandValue(const char* format) {
    if (strcmpIgnoreCase(format, "N") == 0) {
        sprintf(generateValueBuffer, "%01d", rand() % 10);
    } else if (strcmpIgnoreCase(format, "NN") == 0) {
        sprintf(generateValueBuffer, "%02d", rand() % 100);
    } else if (strcmpIgnoreCase(format, "NNN") == 0) {
        sprintf(generateValueBuffer, "%03d", rand() % 1000);
    } else if (strcmpIgnoreCase(format, "NNNN") == 0) {
        sprintf(generateValueBuffer, "%04d", rand() % 10000);
    } else if (strcmpIgnoreCase(format, "NNNNN") == 0) {
        sprintf(generateValueBuffer, "%04d%01d", rand() % 10000, rand() % 10);
    } else if (strcmpIgnoreCase(format, "NNNNNN") == 0) {
        sprintf(generateValueBuffer, "%04d%02d", rand() % 10000, rand() % 100);
    } else {
        sprintf(generateValueBuffer, "{INVALID}");
    }
    return generateValueBuffer;
}

/**
 * Parses a property value and populates any environment variables.  If the expanded
 *  environment variable would result in a string that is longer than bufferLength
 *  the value is truncated.
 */
void evaluateEnvironmentVariables(const char *propertyValue, char *buffer, int bufferLength) {
    const char *in;
    char *out;
    char envName[MAX_PROPERTY_NAME_LENGTH];
    char *envValue;
    char *start;
    char *end;
    size_t len;
    size_t outLen;
    size_t bufferAvailable;

    #ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "evaluateEnvironmentVariables('%s', buffer, %d)",
    propertyValue, bufferLength);
    #endif

    buffer[0] = '\0';
    in = propertyValue;
    out = buffer;
    bufferAvailable = bufferLength - 1; /* Reserver room for the null terminator */

    /* Loop until we hit the end of string. */
    while (in[0] != '\0') {
        #ifdef _DEBUG
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    initial='%s', buffer='%s'", propertyValue, buffer);
        #endif

        start = strchr(in, '%');
        if (start != NULL) {
            end = strchr(start + 1, '%');
            if (end != NULL) {
                /* A pair of '%' characters was found.  An environment */
                /*  variable name should be between the two. */
                len = (int)(end - start - 1);
                memcpy(envName, start + 1, len);
                envName[len] = '\0';

                /* See if it is a special dynamic environment variable */
                if (strstr(envName, "WRAPPER_TIME_") == envName) {
                    /* Found a time value. */
                    envValue = generateTimeValue(envName + 13);
                } else if (strstr(envName, "WRAPPER_RAND_") == envName) {
                    /* Found a time value. */
                    envValue = generateRandValue(envName + 13);
                } else {
                    /* Try looking up the environment variable. */
                    envValue = getenv(envName);
                }

                if (envValue != NULL) {
                    /* An envvar value was found. */
                    /* Copy over any text before the envvar */
                    outLen = (int)(start - in);
                    if (bufferAvailable < outLen) {
                        outLen = bufferAvailable;
                    }
                    if (outLen > 0) {
                        memcpy(out, in, outLen);
                        out += outLen;
                        bufferAvailable -= outLen;
                    }

                    /* Copy over the env value */
                    outLen = strlen(envValue);
                    if (bufferAvailable < outLen) {
                        outLen = bufferAvailable;
                    }
                    if (outLen > 0) {
                        memcpy(out, envValue, outLen);
                        out += outLen;
                        bufferAvailable -= outLen;
                    }

                    /* Terminate the string */
                    out[0] = '\0';

                    /* Set the new in pointer */
                    in = end + 1;
                } else {
                    /* Not found.  So copy over the input up until the */
                    /*  second '%'.  Leave it in case it is actually the */
                    /*  start of an environment variable name */
                    outLen = len = end - in;
                    if (bufferAvailable < outLen) {
                        outLen = bufferAvailable;
                    }
                    if (outLen > 0) {
                        memcpy(out, in, outLen);
                        out += outLen;
                        bufferAvailable -= outLen;
                    }
                    in += len;

                    /* Terminate the string */
                    out[0] = '\0';
                }
            } else {
                /* Only a single '%' char was found. Leave it as is. */
                outLen = len = strlen(in);
                if (bufferAvailable < outLen) {
                    outLen = bufferAvailable;
                }
                if (outLen > 0) {
                    memcpy(out, in, outLen);
                    out += outLen;
                    bufferAvailable -= outLen;
                }
                in += len;

                /* Terminate the string */
                out[0] = '\0';
            }
        } else {
            /* No more '%' chars in the string. Copy over the rest. */
            outLen = len = strlen(in);
            if (bufferAvailable < outLen) {
                outLen = bufferAvailable;
            }
            if (outLen > 0) {
                memcpy(out, in, outLen);
                out += outLen;
                bufferAvailable -= outLen;
            }
            in += len;

            /* Terminate the string */
            out[0] = '\0';
        }
    }
    #ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  final buffer='%s'", buffer);
    #endif
}

void setInnerProperty(Property *property, const char *propertyValue) {
    int i, count;
    /* The property value is expanded into a large buffer once, but that is temporary.  The actual
     *  value is stored in the minimum required size. */
    char buffer[MAX_PROPERTY_VALUE_LENGTH];

    /* Free any existing value */
    if (property->value != NULL) {
        free(property->value);
        property->value = NULL;
    }

    /* Set the new value using a copy of the provided value. */
    if (propertyValue == NULL) {
        property->value = NULL;
    } else {
        evaluateEnvironmentVariables(propertyValue, buffer, MAX_PROPERTY_VALUE_LENGTH);

        property->value = malloc(sizeof(char) * (strlen(buffer) + 1));
        if (!property->value) {
            outOfMemory("SIP", 1);
        } else {
            /* Strip any non valid characters like control characters. Some valid characters are
             *  less than 0 when the char is unsigned. */
            for (i = 0, count = 0; i < (int)strlen(buffer); i++) {
                /* Only add valid chars, skip control chars.  We want all chars other than those
                 *  in the range 1..31.  0 is not possible as that would be end of the string.
                 *  On most platforms, char is signed, but on PowerPC, it is unsigned.  This
                 *  means that any comparison such as >= 0 will cause a compiler error as that
                 *  would always be true.
                 * The logic below is to get the correct behavior in either case assuming no 0. */
                if ((buffer[i] < 1) || (buffer[i] > 31) || (buffer[i] == '\n')) {
                    property->value[count++] = buffer[i];
                }
            }

            /* Crop string to new size */
            property->value[count] = '\0';
        }
    }
}

/**
 * Loads the contents of a file into the specified properties.
 *  Whenever a line which starts with #include is encountered, then the rest
 *  the line will be interpreted as a cascading include file.  If the file
 *  does not exist, the include definition is ignored.
 */
int loadPropertiesInner(Properties* properties, const char* filename, int depth) {
    FILE *stream;
    char buffer[MAX_PROPERTY_NAME_VALUE_LENGTH];
    char expBuffer[MAX_PROPERTY_NAME_VALUE_LENGTH];
    char *trimmedBuffer;
    size_t trimmedBufferLen;
    char *c;
    char *d;
    size_t i, j;
    size_t len;
    int quoted;
    char *absoluteBuffer;
    #ifdef WIN32
    int size;
    #endif

    #ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "loadPropertiesInner(props, '%s', %d)", filename, depth);
    #endif

    /* Look for the specified file. */
    if ((stream = fopen(filename, "rt")) == NULL) {
        /* Unable to open the file. */
        if (debugIncludes) {
            if (depth > 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                "  Included configuration file, %s, was not found.", filename);
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                "Configuration file, %s, was not found.", filename);
            }
        } else {
            #ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Properties file not found: %s", filename);
            #endif
        }
        return 1;
    }

    if (debugIncludes) {
        if (depth > 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
            "  Loading included configuration file, %s", filename);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
            "Loading configuration file, %s", filename);
        }
    }

    /* Load in all of the properties */
    do {
        c = fgets(buffer, MAX_PROPERTY_NAME_VALUE_LENGTH, stream);
        if (c != NULL) {
            /* Always strip both ^M and ^J off the end of the line, this is done rather
             *  than simply checking for \n so that files will work on all platforms
             *  even if their line feeds are incorrect. */
            if ((d = strchr(buffer, 0x0d /* ^M */)) != NULL) {
                d[0] = '\0';
            }
            if ((d = strchr(buffer, 0x0a /* ^J */)) != NULL) { 
                d[0] = '\0';
            }
            /* Strip any whitespace from the front of the line. */
            trimmedBuffer = buffer;
            while ((trimmedBuffer[0] == ' ') || (trimmedBuffer[0] == 0x08)) {
                trimmedBuffer++;
            }

            /* If the line does not start with a comment, make sure that
             *  any comment at the end of line are stripped.  If any any point, a
             *  double hash, '##', is encountered it should be interpreted as a
             *  hash in the actual property rather than the beginning of a comment. */
            if (trimmedBuffer[0] != '#') {
                len = strlen(trimmedBuffer);
                i = 0;
                quoted = 0;
                while (i < len) {
                    if (trimmedBuffer[i] == '"') {
                        quoted = !quoted;
                    } else if ((trimmedBuffer[i] == '#') && (!quoted)) {
                        /* Checking the next character will always be ok because it will be
                         *  '\0 at the end of the string. */
                        if (trimmedBuffer[i + 1] == '#') {
                            /* We found an escaped #. Shift the rest of the string
                             *  down by one character to remove the second '#'.
                             *  Include the shifting of the '\0'. */
                            for (j = i + 1; j <= len; j++) {
                                trimmedBuffer[j - 1] = trimmedBuffer[j];
                            }
                            len--;
                        } else {
                            /* We found a comment. So this is the end. */
                            trimmedBuffer[i] = '\0';
                            len = i;
                        }
                    }
                    i++;
                }
            }

            /* Strip any whitespace from the end of the line. */
            trimmedBufferLen = strlen(trimmedBuffer);
            while ((trimmedBufferLen > 0) && ((trimmedBuffer[trimmedBufferLen - 1] == ' ')
            || (trimmedBuffer[trimmedBufferLen - 1] == 0x08))) {
                
                trimmedBuffer[trimmedBufferLen - 1] = '\0';
                trimmedBufferLen--;
            }

            /* Only look at lines which contain data and do not start with a '#'
             *  If the line starts with '#include' then recurse to the include file */
            if (strlen(trimmedBuffer) > 0) {
                if (strcmpIgnoreCase(trimmedBuffer, "#include.debug") == 0) {
                    /* Enable include file debugging. */
                    debugIncludes = TRUE;
                } else if (strstr(trimmedBuffer, "#include") == trimmedBuffer) {
                    /* Include file, if the file does not exist, then ignore it */
                    /* Strip any leading whitespace */
                    c = trimmedBuffer + 8;
                    while ((c[0] != '\0') && (c[0] == ' ')) {
                        c++;
                    }

                    if (depth < MAX_INCLUDE_DEPTH) {
                        /* The filename may contain environment variables, so expand them. */
                        if (debugIncludes) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                            "Found #include file in %s: %s", filename, c);
                        }
                        evaluateEnvironmentVariables(c, expBuffer, MAX_PROPERTY_NAME_VALUE_LENGTH);

                        if (debugIncludes && (strcmp(c, expBuffer) != 0)) {
                            /* Only show this log if there were any environment variables. */
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                            "  After environment variable replacements: %s", expBuffer);
                        }

                        /* Now obtain the real absolute path to the include file. */
                        #ifdef WIN32
                        /* Find out how big the absolute path will be */
                        size = GetFullPathName(expBuffer, 0, NULL, NULL);
                        if (!size) {
                            if (debugIncludes) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                "  Unable to resolve the full path of the configuration include file, %s: %s",
                                expBuffer, getLastErrorText());
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                "  Current working directory is: %s", wrapperData->originalWorkingDir);
                            }
                            absoluteBuffer = NULL;
                        } else {
                            absoluteBuffer = malloc(sizeof(char) * size);
                            if (!absoluteBuffer) {
                                outOfMemory("LPI", 1);
                            } else {
                                if (!GetFullPathName(expBuffer, size, absoluteBuffer, NULL)) {
                                    if (debugIncludes) {
                                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                                        "  Unable to resolve the full path of the configuration include file, %s: %s",
                                        expBuffer, getLastErrorText());
                                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                        "  Current working directory is: %s", wrapperData->originalWorkingDir);
                                    }
                                    free(absoluteBuffer);
                                    absoluteBuffer = NULL;
                                }
                            }
                        }
                        #else
                        absoluteBuffer = malloc(PATH_MAX);
                        if (!absoluteBuffer) {
                            outOfMemory("LPI", 2);
                        } else {
                            if (realpath(expBuffer, absoluteBuffer) == NULL) {
                                if (debugIncludes) {
                                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                    "  Unable to resolve the full path of the configuration include file, %s: %s",
                                    expBuffer, getLastErrorText());
                                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                    "  Current working directory is: %s", wrapperData->originalWorkingDir);
                                }
                                free(absoluteBuffer);
                                absoluteBuffer = NULL;
                            }
                        }
                        #endif
                        if (absoluteBuffer) {
                            loadPropertiesInner(properties, absoluteBuffer, depth + 1);
                            free(absoluteBuffer);
                        }
                    }
                } else if (strstr(trimmedBuffer, "include") == trimmedBuffer) {
                    /* Users sometimes remove the '#' from include statements.  Add a warning to help them notice the problem. */
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    "Include file reference missing leading '#': %s", trimmedBuffer);
                } else if (trimmedBuffer[0] != '#') {
                    /* printf("%s\n", trimmedBuffer); */

                    /* Locate the first '=' in the line, ignore lines that do not contain a '=' */
                    if ((d = strchr(trimmedBuffer, '=')) != NULL) {
                        /* Null terminate the first half of the line. */
                        *d = '\0';
                        d++;
                        addProperty(properties, trimmedBuffer, d, FALSE, FALSE, TRUE);
                    }
                }
            }
        }
    } while (c != NULL);

    /* Close the file */
    fclose(stream);

    return 0;
}

int loadProperties(Properties *properties, const char* filename) {
    /* Store the time that the property file began to be loaded. */
    #ifdef WIN32
    struct _timeb timebNow;
    #else
    struct timeval timevalNow;
    #endif
    time_t      now;
    struct tm   *nowTM;

    #ifdef WIN32
    _ftime(&timebNow);
    now = (time_t)timebNow.time;
    #else
    gettimeofday(&timevalNow, NULL);
    now = (time_t)timevalNow.tv_sec;
    #endif
    nowTM = localtime(&now);
    memcpy(&loadPropertiesTM, nowTM, sizeof(struct tm));

    return loadPropertiesInner(properties, filename, 0);
}

Properties* createProperties() {
    Properties *properties = malloc(sizeof(Properties));
    if (!properties) {
        outOfMemory("CP", 1);
        return NULL;
    }
    properties->first = NULL;
    properties->last = NULL;
    return properties;
}

void disposeProperties(Properties *properties) {
    /* Loop and dispose any Property structures */
    Property *tempProperty;
    Property *property = properties->first;
    properties->first = NULL;
    properties->last = NULL;
    while (property != NULL) {
        /* Save the next property */
        tempProperty = property->next;

        /* Clean up the current property */
        disposeInnerProperty(property);
        property = NULL;

        /* set the current property to the next. */
        property = tempProperty;
    }

    /* Dispose the Properties structure */
    free(properties);
    properties = NULL;
}

void removeProperty(Properties *properties, const char *propertyName) {
    Property *property;
    Property *next;
    Property *previous;

    /* Look up the property */
    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        /* The property did not exist, so nothing to do. */
    } else {
        next = property->next;
        previous = property->previous;

        /* Disconnect the property */
        if (next == NULL) {
            /* This was the last property */
            properties->last = previous;
        } else {
            next->previous = property->previous;
        }
        if (previous ==  NULL) {
            /* This was the first property */
            properties->first = next;
        } else {
            previous->next = property->next;
        }

        /* Now that property is disconnected, if can be disposed. */
        disposeInnerProperty(property);
    }
}

/**
 * Sets an environment variable with the specified value.
 *  The function will only set the variable if its value is changed, but if
 *  it does, the call will result in a memory leak the size of the string:
 *   "name=value".
 *
 * Return TRUE if there were any problems.
 */
int setEnv(const char *name, const char *value) {
    char *oldVal;
    char *envBuf;

    /* Get the current environment variable value so we can avoid allocating and
     *  setting the variable if it has not changed its value. */
    oldVal = getenv(name);
    if (value == NULL) {
        /*printf("clear %s=\n", name);*/
        /* Only clear the variable if it is actually set to avoid unnecessary leaks. */
        if (oldVal != NULL) {
            /* Allocate a block of memory for the environment variable.  The system uses
             *  this memory so it is not freed after we set it. We only call this on
             *  startup, so the leak is minor. */
            envBuf = malloc(sizeof(char) * (strlen(name) + 2));
            if (!envBuf) {
                outOfMemory("SE", 1);
                return TRUE;
            } else {
                sprintf(envBuf, "%s=", name);
                /* The memory pointed to by envBuf becomes part of the environment so it can
                 *  not be freed by us here. */
                if (putenv(envBuf)) {
                    printf("Unable to clear environment variable: %s\n", envBuf);
                    return TRUE;
                }
            }
        }
    } else {
        /*printf("set %s=%s\n", name, value);*/
        if ((oldVal == NULL) || (strcmp(oldVal, value) != 0)) {
            /* Allocate a block of memory for the environment variable.  The system uses
             *  this memory so it is not freed after we set it. We only call this on
             *  startup, so the leak is minor. */
            envBuf = malloc(sizeof(char) * (strlen(name) + strlen(value) + 2));
            if (!envBuf) {
                outOfMemory("SE", 2);
                return TRUE;
            } else {
                sprintf(envBuf, "%s=%s", name, value);
                /* The memory pointed to by envBuf becomes part of the environment so it can
                 *  not be freed by us here. */
                if (putenv(envBuf)) {
                    printf("Unable to set environment variable: %s\n", envBuf);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

/* Trims any whitespace from the beginning and end of the in string
 *  and places the results in the out buffer.  Assumes that the out
 *  buffer is at least as large as the in buffer. */
void trim(const char *in, char *out) {
    size_t len;
    size_t first;
    size_t last;

    len = strlen(in);
    if (len > 0) {
        first = 0;
        last = len - 1;

        /* Right Trim */
        while (((in[first] == ' ') || (in[first] == '\t')) && (first < last)) {
            first++;
        }
        /* Left Trim */
        while ((last > first) && ((in[last] == ' ') || (in[last] == '\t'))) {
            last--;
        }

        /* Copy over what is left. */
        len = last - first + 1;
        if (len > 0) {
            memcpy(out, in + first, len);
        }
    }
    out[len] = '\0';
}

/**
 * Used to set a NULL terminated list of property names whose values should be
 *  escaped when read in from a file.   '\\' will become '\' and '\n' will
 *  become '^J', all other characters following '\' will be left as is.
 *
 * @param propertyNames NULL terminated list of property names.  Property names
 *                      can contain a single '*' wildcard which will match 0 or
 *                      more characters.
 */
void setEscapedProperties(const char **propertyNames) {
    escapedPropertyNames = propertyNames;
}

/**
 * Returns true if the specified property matches one of the property names
 *  previosly set in a call to setEscapableProperties()
 *
 * @param propertyName Property name to test.
 *
 * @return TRUE if the property should be escaped.  FALSE otherwise.
 */
int isEscapedProperty(const char *propertyName) {
    size_t nameLen;
    size_t i;
    const char *pattern;
    char *wildPos;
    size_t headLen;
    size_t tailLen;
    int matched;
    size_t patternI;
    size_t nameI;

    if (escapedPropertyNames) {
        nameLen = strlen(propertyName);
        i = 0;
        while (escapedPropertyNames[i]) {
            pattern = escapedPropertyNames[i];
            if (strcmpIgnoreCase(pattern, propertyName) == 0) {
                /* Direct Match. */
#ifdef _DEBUG
                printf("Property %s matched pattern %s\n", propertyName, pattern);
#endif
                return TRUE;
            } else {
                wildPos = strchr(pattern, '*');
                if (wildPos) {
                    /* The string contains a wildcard. */

                    /* Try to match the head of the property name. */
                    headLen = wildPos - pattern;
                    if (headLen < nameLen) {
                        matched = TRUE;
                        patternI = 0;
                        nameI = 0;
                        while (patternI < headLen) {
                            if (pattern[patternI] != propertyName[nameI]) {
                                matched = FALSE;
                                break;
                            }
                            patternI++;
                            nameI++;
                        }

                        if (matched) {
                            tailLen = strlen(pattern) - headLen - 1;
                            if (tailLen < nameLen - headLen) {
                                matched = TRUE;
                                patternI = headLen + 1;
                                nameI = nameLen - tailLen;
                                while (nameI < nameLen) {
                                    if (pattern[patternI] != propertyName[nameI]) {
                                        matched = FALSE;
                                        break;
                                    }
                                    patternI++;
                                    nameI++;
                                }
                                if (matched) {
#ifdef _DEBUG
                                    printf("Property %s matched pattern %s\n", propertyName, pattern);
#endif
                                    return TRUE;
                                }
                            }
                        }
                    }
                }
            }

            i++;
        }
    }

    return FALSE;
}

/**
 * Expands escaped characters and returns a newly malloced string with the result.
 *  '\n' replaced with '^J'
 *  '\\' replaced with '\'
 *  Other escaped characters will show as is.
 *
 * @param buffer Original buffer containing escaped characters.
 *
 * @return The new expanded buffer.  It is the responsibility of the caller to free memory later.
 */
char *expandEscapedCharacters(const char* buffer) {
    size_t inPos;
    size_t outPos;
    char *outBuffer;
    int i;
    char c1, c2;

    /* First count the length of the required output buffer to hold the current line. Use the same code twice to avoid maintenance problems.  */
    outBuffer = NULL;
    for (i = 0; i < 2; i++) {
        inPos = 0;
        outPos = 0;
        do {
            c1 = buffer[inPos];
            if (c1 == '\\') {
                /* Escape. */
                c2 = buffer[inPos + 1];
                if (c2 == 'n') {
                    /* Line feed. */
                    inPos++;
                    if (outBuffer) {
                        outBuffer[outPos] = '\n';
                    }
                    outPos++;
                } else if (c2 == '\\') {
                    /* Back slash. */
                    inPos++;

                    if (outBuffer) {
                        outBuffer[outPos] = '\\';
                    }
                    outPos++;
                } else if (c2 == 0) {
                    /* Premature End of buffer.  Show the backslash. */
                    if (outBuffer) {
                        outBuffer[outPos] = '\\';
                    }
                    outPos++;
                    c1 = 0;
                } else {
                    /* Unknown char, show the unescaped backslash. */
                    inPos++;

                    if (outBuffer) {
                        outBuffer[outPos] = '\\';
                        outBuffer[outPos + 1] = c2;
                    }
                    outPos += 2;
                }
                inPos++;
            } else if (c1 == 0) {
                /* End of buffer. */
            } else {
                /* Normal character. */
                if (outBuffer) {
                    outBuffer[outPos] = c1;
                }
                outPos++;
                inPos++;
            }
        } while (c1 != 0);

        /* string terminator. */
        if (outBuffer) {
            outBuffer[outPos] = '\0';
        }
        outPos++;

        if (outBuffer) {
            /* We have have full outBuffer. Fall through. */
        } else {
            /* First pass. We need to allocate the outBuffer. */
            outBuffer = malloc(outPos);
            if (!outBuffer) {
                outOfMemory("ELF", 1);
                return NULL;
            }
        }
    }

    return outBuffer;
}


/**
 * Adds a single property to the properties structure.
 *
 * @param properties Properties structure to add to.
 * @param propertyName Name of the new Property.
 * @param propertyValue Initial property value.
 * @param finalValue True if the property should be set as static.
 * @param quotable True if the property could contain quotes.
 * @param escapable True if the propertyValue can be escaped if its propertyName
 *                  is in the list set with setEscapableProperties().
 *
 * @return The newly created Property, or NULL if there was a reported error.
 */
Property* addProperty(Properties *properties, const char *propertyName, const char *propertyValue, int finalValue, int quotable, int escapable) {
    int setValue;
    Property *property;
    char *propertyNameTrim;
    char *propertyValueTrim;
    char *propertyExpandedValue;

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "addProperty(%p, '%s', '%s', %d, %d)",
        properties, propertyName, propertyValue, finalValue, quotable);
#endif

    /* It is possible that the propertyName and or properyValue contains extra spaces. */
    propertyNameTrim = malloc(sizeof(char) * (strlen(propertyName) + 1));
    if (!propertyNameTrim) {
        outOfMemory("AP", 1);
        return NULL;
    }
    trim(propertyName, propertyNameTrim);
    propertyValueTrim = malloc(sizeof(char) * (strlen(propertyValue) + 1));
    if (!propertyValueTrim) {
        outOfMemory("AP", 2);
        free(propertyNameTrim);
        return NULL;
    }
    trim(propertyValue, propertyValueTrim);

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  trimmed name='%s', value='%s'",
        propertyNameTrim, propertyValueTrim);
#endif

    /* See if the property already exists */
    setValue = TRUE;
    property = getInnerProperty(properties, propertyNameTrim);
    if (property == NULL) {
        /* This is a new property */
        property = createInnerProperty();
        if (!property) {
            free(propertyNameTrim);
            free(propertyValueTrim);
            return NULL;
        }

        /* Store a copy of the name */
        property->name = malloc(sizeof(char) * (strlen(propertyNameTrim) + 1));
        if (!property->name) {
            outOfMemory("AP", 3);
            disposeInnerProperty(property);
            free(propertyNameTrim);
            free(propertyValueTrim);
            return NULL;
        }
        strcpy(property->name, propertyNameTrim);

        /* Insert this property at the correct location.  Value will still be null. */
        insertInnerProperty(properties, property);
    } else {
        /* The property was already set.  Only change it if non final */
        if (property->finalValue) {
            setValue = FALSE;
        }
    }
    free(propertyNameTrim);

    if (setValue) {
        if (escapable && isEscapedProperty(property->name)) {
            /* Expand the value. */
#ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "expanding value of %s", property->name);
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  value   : %s", propertyValueTrim);
#endif
            propertyExpandedValue = expandEscapedCharacters(propertyValueTrim);
            if (!propertyExpandedValue) {
                free(propertyValueTrim);
                return NULL;
            }
#ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "  expanded: %s", propertyExpandedValue);
#endif

            /* Set the property value. */
            setInnerProperty(property, propertyExpandedValue);

            free(propertyExpandedValue);
        } else {
            /* Set the property value. */
            setInnerProperty(property, propertyValueTrim);
        }

        /* Store the final flag */
        property->finalValue = finalValue;

        /* Store the quotable flag. */
        property->quotable = quotable;

        /* Prepare the property by expanding any environment variables that are defined. */
        prepareProperty(property);

        /* See if this is a special property */
        if ((strlen(property->name) > 12) && (strstr(property->name, "set.default.") == property->name)) {
            /* This property is an environment variable definition that should only
             *  be set if the environment variable does not already exist.  Get the
             *  value back out of the property as it may have had environment
             *  replacements. */
            if (getenv(property->name + 12) == NULL) {
#ifdef _DEBUG
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "set default env('%s', '%s')",
                    property->name + 12, property->value);
#endif
                setEnv(property->name + 12, property->value);
            } else {
#ifdef _DEBUG
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    "not setting default env('%s', '%s'), already set to '%s'",
                    property->name + 12, property->value, getenv(property->name + 12));
#endif
            }
        } else if ((strlen(property->name) > 4) && (strstr(property->name, "set.") == property->name)) {
            /* This property is an environment variable definition.  Get the
             *  value back out of the property as it may have had environment
             *  replacements. */
#ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "set env('%s', '%s')",
                property->name + 4, property->value);
#endif
            setEnv(property->name + 4, property->value);
        }
    }
    free(propertyValueTrim);

    return property;
}

/**
 * Takes a name/value pair in the form <name>=<value> and attempts to add
 * it to the specified properties table.
 *
 * Returns 0 if successful, otherwise 1
 */
int addPropertyPair(Properties *properties, const char *propertyNameValue, int finalValue, int quotable) {
    char buffer[MAX_PROPERTY_NAME_VALUE_LENGTH];
    char *d;

    /* Make a copy of the pair that we can edit */
    if (strlen(propertyNameValue) + 1 >= MAX_PROPERTY_NAME_VALUE_LENGTH) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, 
        "The following property name value pair is too large.  Need to increase the internal buffer size: %s", propertyNameValue);
        return 1;
    }
    strcpy(buffer, propertyNameValue);

    /* Locate the first '=' in the pair */
    if ((d = strchr(buffer, '=')) != NULL) {
        /* Null terminate the first half of the line. */
        *d = '\0';
        d++;
        addProperty(properties, buffer, d, finalValue, quotable, FALSE);

        return 0;
    } else {
        return 1;
    }
}

const char* getStringProperty(Properties *properties, const char *propertyName, const char *defaultValue) {
    Property *property;
    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        if (defaultValue != NULL) {
            property = addProperty(properties, propertyName, defaultValue, FALSE, FALSE, FALSE);
            if (property) {
                return property->value;
            } else {
                return NULL;
            }
        } else {
            return NULL;
        }
    } else {
        return property->value;
    }
}

const char* getFileSafeStringProperty(Properties *properties, const char *propertyName, const char *defaultValue) {
    Property *property;
    char *buffer;
    int i;

    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        if (defaultValue != NULL) {
            addProperty(properties, propertyName, defaultValue, FALSE, FALSE, FALSE);
        }

        return defaultValue;
    } else {
        buffer = property->value;
        if (strchr(buffer, '%')) {
            i = 0;
            while (buffer[i]) {
                if (buffer[i] == '%') {
                    buffer[i] = '_';
                }
                i++;
            }
        }
        return buffer;
    }
}

/**
 * Does a quick sort of the property values, keeping the values together.
 */
void sortStringProperties(long unsigned int *propertyIndices, char **propertyNames, char **propertyValues, int low, int high) {
    int i = low;
    int j = high;
    long int tempIndex;
    char *tempName;
    char *tempValue;
    long unsigned int x = propertyIndices[(low + high) / 2];

    do {
        while (propertyIndices[i] < x) {
            i++;
        }
        while (propertyIndices[j] > x) {
            j--;
        }
        if (i <= j) {
            /* Swap i and j values. */
            tempIndex = propertyIndices[i];
            tempName = propertyNames[i];
            tempValue = propertyValues[i];

            propertyIndices[i] = propertyIndices[j];
            propertyNames[i] = propertyNames[j];
            propertyValues[i] = propertyValues[j];

            propertyIndices[j] = tempIndex;
            propertyNames[j] = tempName;
            propertyValues[j] = tempValue;

            i++;
            j--;
        }
    } while (i <= j);

    /* Recurse */
    if (low < j) {
        sortStringProperties(propertyIndices, propertyNames, propertyValues, low, j);
    }
    if (i < high) {
        sortStringProperties(propertyIndices, propertyNames, propertyValues, i, high);
    }
}

/**
 * Returns a sorted array of all properties beginning with {propertyNameBase}.
 *  Only numerical characters can be returned between the two.
 *
 * @param properties The full properties structure.
 * @param propertyNameHead All matching properties must begin with this value.
 * @param propertyNameTail All matching properties must end with this value.
 * @param all If FALSE then the array will start with #1 and loop up until the
 *            next property is not found, if TRUE then all properties will be
 *            returned, even if there are gaps in the series.
 * @param matchAny If FALSE only numbers are allowed as placeholder
 * @param propertyNames Returns a pointer to a NULL terminated array of
 *                      property names.
 * @param propertyValues Returns a pointer to a NULL terminated array of
 *                       property values.
 *
 * @return 0 if successful, -1 if there was an error.
 */
int getStringProperties(Properties *properties, const char *propertyNameHead, const char *propertyNameTail, int all, int matchAny, char ***propertyNames, char ***propertyValues, long unsigned int **propertyIndices) {
    int j;
    int k;
    size_t headLen;
    size_t tailLen;
    size_t thisLen;
    char *thisHead;
    char *thisTail;
    size_t i;
    Property *property;
    size_t indexLen;
    char indexS[11];
    int ok;
    char c;
    int count;

    *propertyIndices = NULL;

    headLen = strlen(propertyNameHead);
    tailLen = strlen(propertyNameTail);

    for (j = 0; j < 2; j++) {
        count = 0;
        property = properties->first;
        while (property != NULL) {
            thisLen = strlen(property->name);
            if (matchAny && thisLen < headLen +  tailLen - 1) {
                /* Too short, not what we are looking for. */
            } else if (!matchAny && thisLen < headLen +  tailLen + 1) {
                /* Too short, not what we are looking for. */
            } else {
                thisHead = malloc(headLen + 1);
                if (!thisHead) {
                    outOfMemory("GSPS", 1);
                } else {
                    memcpy(thisHead, property->name, headLen);
                    thisHead[headLen] = 0;

                    if (strcmpIgnoreCase(thisHead, propertyNameHead) == 0) {
                        /* Head matches. */
                        
                        thisTail = malloc(tailLen + 1);
                        if (!thisTail) {
                            outOfMemory("GSPS", 2);
                        } else {
                            strcpy(thisTail, property->name + thisLen - tailLen);

                            if (strcmpIgnoreCase(thisTail, propertyNameTail) == 0) {
                                /* Tail matches. */
                                if (matchAny) { 
                                    indexLen = thisLen - headLen - tailLen + 1; 
                                } else {
                                    indexLen = thisLen - headLen - tailLen;
                                }
                                if (indexLen <= 10) {
                                    memcpy(indexS, property->name + headLen, indexLen);
                                    indexS[indexLen] = 0;

                                    ok = TRUE;
                                    for (i = 0; i < indexLen; i++) {
                                        c = indexS[i];
                                        if (matchAny == FALSE && ((c < '0') || (c > '9'))) {
                                            ok = FALSE;
                                            break;
                                        }
                                    }

                                    if (ok) {
                                        if (*propertyIndices) {
                                            /* We found it. */
                                            prepareProperty(property);

                                            (*propertyIndices)[count] = strtoul(indexS, NULL, 10);
                                            (*propertyNames)[count] = property->name;
                                            (*propertyValues)[count] = property->value;
                                        }

                                        count++;
                                    }
                                }
                            }

                            free(thisTail);
                        }
                    }

                    free(thisHead);
                }
            }

            /* Keep looking */
            property = property->next;
        }

        if (*propertyIndices == NULL) {
            /* First pass */

            *propertyNames = malloc(sizeof(char *) * (count + 1));
            if (!(*propertyNames)) {
                outOfMemory("GSPS", 3);
                *propertyNames = NULL;
                *propertyValues = NULL;
                *propertyIndices = NULL;
                return -1;
            }

            *propertyValues = malloc(sizeof(char *) * (count + 1));
            if (!(*propertyValues)) {
                outOfMemory("GSPS", 4);
                free(*propertyNames);
                *propertyNames = NULL;
                *propertyValues = NULL;
                *propertyIndices = NULL;
                return -1;
            }

            *propertyIndices = malloc(sizeof(long unsigned int) * (count + 1));
            if (!(*propertyIndices)) {
                outOfMemory("GSPS", 5);
                free(*propertyNames);
                free(*propertyValues);
                *propertyNames = NULL;
                *propertyValues = NULL;
                *propertyIndices = NULL;
                return -1;
            }

            if (count == 0) {
                /* The count is 0 so no need to continue through the loop again. */
                (*propertyNames)[0] = NULL;
                (*propertyValues)[0] = NULL;
                (*propertyIndices)[0] = 0;
                return 0;
            }
        } else {
            /* Second pass */
            (*propertyNames)[count] = NULL;
            (*propertyValues)[count] = NULL;
            (*propertyIndices)[count] = 0;

            sortStringProperties(*propertyIndices, *propertyNames, *propertyValues, 0, count - 1);

            /* If we don't want all of the properties then we need to remove the extra ones.
             *  Names and values are not allocated, so setting them to NULL is fine.*/
            if (!all) {
                for (k = 0; k < count; k++) {
                    if ((*propertyIndices)[k] != k + 1) {
                        (*propertyNames)[k] = NULL;
                        (*propertyValues)[k] = NULL;
                        (*propertyIndices)[k] = 0;
                    }
                }
            }
            /*
            for (k = 0; k < count; k++) {
                if ((*propertyNames)[k]) {
                    printf("[%d] #%lu: %s=%s\n", k, (*propertyIndices)[k], (*propertyNames)[k], (*propertyValues)[k]);
                }
            }
            */

            return 0;
        }
    }

    /* For compiler */
    return 0;
}

/**
 * Frees up an array of properties previously returned by getStringProperties().
 */
void freeStringProperties(char **propertyNames, char **propertyValues, long unsigned int *propertyIndices) {
    /* The property names are not malloced. */
    free(propertyNames);

    /* The property values are not malloced. */
    free(propertyValues);

    free(propertyIndices);
}


/**
 * Performs a case insensitive check of the property value against the value provided.
 *  If the property is not set then it is compared with the defaultValue.
 */
int checkPropertyEqual(Properties *properties, const char *propertyName, const char *defaultValue, const char *value) {
    Property *property;
    const char *propertyValue;

    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        propertyValue = defaultValue;
    } else {
        propertyValue = property->value;
    }

    return strcmpIgnoreCase(propertyValue, value) == 0;
}

int getIntProperty(Properties *properties, const char *propertyName, int defaultValue) {
    char buffer[16];
    Property *property;

    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        sprintf(buffer, "%d", defaultValue);
        addProperty(properties, propertyName, buffer, FALSE, FALSE, FALSE);

        return defaultValue;
    } else {
        return (int)strtol(property->value, NULL, 0);
    }
}

int getBooleanProperty(Properties *properties, const char *propertyName, int defaultValue) {
    if (defaultValue) {
        return checkPropertyEqual(properties, propertyName, "true", "true");
    } else {
        return checkPropertyEqual(properties, propertyName, "false", "true");
    }
}


int isQuotableProperty(Properties *properties, const char *propertyName) {
    Property *property;
    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        return FALSE;
    } else {
        return property->quotable;
    }
}

void dumpProperties(Properties *properties) {
    Property *property;
    property = properties->first;
    while (property != NULL) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "    name:%s value:%s", property->name, property->value);
        property = property->next;
    }
}

/** Creates a linearized representation of all of the properties.
 *  The returned buffer must be freed by the calling code. */
char *linearizeProperties(Properties *properties, char separator) {
    Property *property;
    size_t size;
    char *c;
    char *fullBuffer;
    char *buffer;
    char *work;

    /* First we need to figure out how large a buffer will be needed to linearize the properties. */
    size = 0;
    property = properties->first;
    while (property != NULL) {
        /* Add the length of the basic property. */
        size += strlen(property->name);
        size++; /* '=' */
        size += strlen(property->value);

        /* Handle any characters that will need to be escaped. */
        c = property->name;
        while ((c = strchr(c, separator)) != NULL) {
            size++;
            c++;
        }
        c = property->value;
        while ((c = strchr(c, separator)) != NULL) {
            size++;
            c++;
        }

        size++; /* separator */

        property = property->next;
    }
    size++; /* null terminated. */

    /* Now that we know how much space this will all take up, allocate a buffer. */
    fullBuffer = buffer = malloc(size);
    if (!fullBuffer) {
        outOfMemory("LP", 1);
        return NULL;
    }

    /* Now actually build up the output.  Any separator characters need to be escaped with themselves. */
    property = properties->first;
    while (property != NULL) {
        /* name */
        work = property->name;
        while ((c = strchr(work, separator)) != NULL) {
            memcpy(buffer, work, c - work + 1);
            buffer += c - work + 1;
            buffer[0] = separator;
            buffer++;
            work = c + 1;
        }
        strcpy(buffer, work);
        buffer += strlen(work);

        /* equals */
        buffer[0] = '=';
        buffer++;

        /* value */
        work = property->value;
        while ((c = strchr(work, separator)) != NULL) {
            memcpy(buffer, work, c - work + 1);
            buffer += c - work + 1;
            buffer[0] = separator;
            buffer++;
            work = c + 1;
        }
        strcpy(buffer, work);
        buffer += strlen(work);

        /* separator */
        buffer[0] = separator;
        buffer++;

        property = property->next;
    }

    /* null terminate. */
    buffer[0] = 0;
    buffer++;

    return fullBuffer;
}
