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
 * Revision 1.17  2003/04/09 03:56:53  mortenson
 * Fix a buffer overflow problem if configuration properties referenced
 * extremely large environment variables.
 *
 * Revision 1.16  2003/04/03 16:27:13  mortenson
 * Tabs to spaces.  No other changes.
 *
 * Revision 1.15  2003/04/03 16:13:35  mortenson
 * Fix a problem where the values of environment variables set in the
 * configuration file were not correct when those values included references
 * to other environment variables.
 *
 * Revision 1.14  2003/04/03 04:05:22  mortenson
 * Fix several typos in the docs.  Thanks to Mike Castle.
 *
 * Revision 1.13  2003/03/13 15:40:41  mortenson
 * Add the ability to set environment variables from within the configuration
 * file or from the command line.
 *
 * Revision 1.12  2003/02/17 03:38:25  mortenson
 * Improve the parsing of config files so that leading and trailing white space
 * is now correctly trimmed.  It is also now possible to have comments at the
 * end of a line containing a property.
 *
 * Revision 1.11  2003/02/09 08:25:14  mortenson
 * Add the ability to use environment variable reference for the names of include
 * files.
 *
 * Revision 1.10  2003/02/08 15:49:59  mortenson
 * Implemented cascading configuration files.
 *
 * Revision 1.9  2003/02/03 06:55:26  mortenson
 * License transfer to TanukiSoftware.org
 *
 */

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#else
#include <strings.h>
#endif

#include "logger.h"
#include "property.h"

#define MAX_INCLUDE_DEPTH 10

/**
 * Private function to find a Property structure.
 */
Property* getInnerProperty(Properties *properties, const char *propertyName) {
    Property *property;
    int cmp;

    /* Loop over the properties which are in order and look for the specified property. */
    property = properties->first;
    while (property != NULL) {
        cmp = strcmp(property->name, propertyName);
        if (cmp > 0) {
            /* This property would be after the one being looked for, so it does not exist. */
            return NULL;
        } else if (cmp == 0) {
            /* We found it. */
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
        cmp = strcmp(property->name, newProperty->name);
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

    property = (Property *)malloc(sizeof(Property));
    property->name = NULL;
    property->next = NULL;
    property->previous = NULL;
    property->value = NULL;

    return property;
}

/**
 * Private function to dispose a Property structure.  Assumes that the
 *	Property is disconnected already.
 */
void disposeInnerProperty(Property *property) {
    free(property->name);
    free(property->value);
    free(property);
}

/**
 * Parses a property value and populates any environment variables.  If the expanded
 *  environment variable would result in a string that is longer than bufferLength
 *  the value is truncated.
 */
void evaluateEnvironmentVariables(const char *propertyValue, char *buffer, int bufferLength) {
    const char *in;
    char *out;
    char envName[256];
    char *envValue;
    char *start;
    char *end;
    int len;
    int outLen;
    int bufferAvailable;

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
                len = end - start - 1;
                memcpy(envName, start + 1, len);
                envName[len] = '\0';

                /* Look up the environment variable */
                envValue = getenv(envName);
                if (envValue != NULL) {
                    /* An envvar value was found. */
                    /* Copy over any text before the envvar */
                    outLen = start - in;
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
    char buffer[2048];

    /* Free any existing value */
    if (property->value != NULL) {
        free(property->value);
    }

    /* Set the new value using a copy of the provided value. */
    if (propertyValue == NULL) {
        property->value = NULL;
    } else {
        evaluateEnvironmentVariables(propertyValue, buffer, 2048);

        property->value = (char *)malloc(sizeof(char) * (strlen(buffer) + 1));

        /* Strip any non valid characters like control characters */
        for (i = 0, count = 0; i < (int)strlen(buffer); i++) {
            if (buffer[i] > 31) /* Only add valid chars, skip control chars */
                property->value[count++] = buffer[i];
        }

        /* Crop string to new size */
        property->value[count] = '\0';
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
    char buffer[1024];
    char expBuffer[2048];
    char *trimmedBuffer;
    int trimmedBufferLen;
    char *c;
    char *d;

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "loadPropertiesInner(props, '%s', %d)", filename, depth);
#endif

    /* Look for the specified file. */
    if ((stream = fopen(filename, "rt")) == NULL) {
        /* Unable to open the file. */
#ifdef _DEBUG
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "Properties file not found: %s", filename);
#endif
        return 1;
    }

    /* Load in all of the properties */
    do {
        c = fgets(buffer, 1024, stream);
        if (c != NULL) {
            /* Strip the LF off the end of the line. */
            if ((d = strchr(buffer, '\n')) != NULL) {
                d[0] = '\0';
            }

            /* Strip any whitespace from the front of the line. */
            trimmedBuffer = buffer;
            while ((trimmedBuffer[0] == ' ') || (trimmedBuffer[0] == 0x08)) {
                trimmedBuffer++;
            }

            /* If the line does not start with a comment, make sure that
             *  any comment at the end of line is stripped */
            if (trimmedBuffer[0] != '#') {
                if ((d = strchr(trimmedBuffer, '#')) != NULL) {
                    d[0] = '\0';
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
                if (strstr(trimmedBuffer, "#include") == trimmedBuffer) {
                    /* Include file, if the file does not exist, then ignore it */
                    /* Strip any leading whitespace */
                    c = trimmedBuffer + 8;
                    while ((c[0] != '\0') && (c[0] == ' ')) {
                        c++;
                    }

                    if (depth < MAX_INCLUDE_DEPTH) {
                        /* The filename may contain environment variables, so expand them. */
                        evaluateEnvironmentVariables(c, expBuffer, 2048);

                        loadPropertiesInner(properties, expBuffer, depth + 1);
                    }
                } else if (trimmedBuffer[0] != '#') {
                    /* printf("%s\n", trimmedBuffer); */

                    /* Locate the first '=' in the line */
                    if ((d = strchr(trimmedBuffer, '=')) != NULL) {
                        /* Null terminate the first half of the line. */
                        *d = '\0';
                        d++;
                        addProperty(properties, trimmedBuffer, d, FALSE);
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
    return loadPropertiesInner(properties, filename, 0);
}

Properties* createProperties() {
    Properties *properties = (Properties *)malloc(sizeof(Properties));
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

        /* set the current property to the next. */
        property = tempProperty;
    }

    /* Dispose the Properties structure */
    free(properties);
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

void setEnv( const char *name, const char *value )
{
    char *envBuf;

    envBuf = (char*)malloc(sizeof(char) * (strlen(name) + strlen(value) + 2));
    sprintf(envBuf, "%s=%s", name, value);
    if (putenv(envBuf)) {
        printf("Unable to set environment variable: %s\n", envBuf);
    }
}

void addProperty(Properties *properties, const char *propertyName, const char *propertyValue, int finalValue) {
    int setValue;
    Property *property;

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "addProperty(%p, '%s', '%s', %d)",
        properties, propertyName, propertyValue, finalValue);
#endif

    /* See if the property already exists */
    setValue = TRUE;
    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        /* This is a new property */
        property = createInnerProperty();

        /* Store a copy of the name */
        property->name = (char *)malloc(sizeof(char) * (strlen(propertyName) + 1));
        strcpy(property->name, propertyName);

        /* Insert this property at the correct location. */
        insertInnerProperty(properties, property);
    } else {
        /* The property was already set.  Only change it if non final */
        if ( property->finalValue ) {
            setValue = FALSE;
        }
    }

    if (setValue) {
        /* Set the property value. */
        setInnerProperty(property, propertyValue);

        /* Store the final flag */
        property->finalValue = finalValue;

        /* See if this is a special property */
        if ((strlen(propertyName) > 4) && (strstr(propertyName, "set.") == propertyName)) {
            /* This property is an environment variable definition.  Get the
             *  value back out of the property as it may have had environment
             *  replacements. */
#ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, "setEnv('%s', '%s')",
                propertyName + 4, property->value);
#endif
            setEnv(propertyName + 4, property->value);
        }
    }
}

/**
 * Takes a name/value pair in the form <name>=<value> and attempts to add
 * it to the specified properties table.
 *
 * Returns 0 if successful, otherwise 1
 */
int addPropertyPair(Properties *properties, const char *propertyNameValue, int finalValue) {
    char buffer[1024];
    char *d;

    /* Make a copy of the pair that we can edit */
    strcpy(buffer, propertyNameValue);

    /* Locate the first '=' in the pair */
    if ((d = strchr(buffer, '=')) != NULL) {
        /* Null terminate the first half of the line. */
        *d = '\0';
        d++;
        addProperty(properties, buffer, d, finalValue);

        return 0;
    } else {
        return 1;
    }
}

const char* getStringProperty(Properties *properties, const char *propertyName, const char *defaultValue) {
    Property *property;
    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        return defaultValue;
    } else {
        return property->value;
    }
}

int getIntProperty(Properties *properties, const char *propertyName, int defaultValue) {
    Property *property;
    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        return defaultValue;
    } else {
        return atoi(property->value);
    }
}

int getBooleanProperty(Properties *properties, const char *propertyName, int defaultValue) {
    Property *property;
    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        return defaultValue;
    } else {
        /* A value was set.  Set to true only if the value equals "true" */
#ifdef WIN32
        if (strcmp(strlwr(property->value), "true") == 0) {
#else /* UNIX */
        if (strcasecmp(property->value, "true") == 0) {
#endif
            return TRUE;
        } else {
            return FALSE;
        }
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

