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
 *
 * $Log$
 * Revision 1.8  2003/01/28 09:25:47  mortenson
 * Added support for building the wrapper on AIX and HPUX systems.  Thanks for
 * the patches involved go out to Ashish Gawarikar and William Lee.
 *
 * Revision 1.7  2002/10/24 04:48:43  mortenson
 * Fixed a problem where the wrapper.conf was being open with both read and
 * write locks when a read lock is all that is needed.
 *
 * Revision 1.6  2002/10/16 14:47:32  mortenson
 * Add support for environment variable evaluation in configuration file.
 *
 * Revision 1.5  2002/03/07 09:23:25  mortenson
 * Go through and change the style of comments that we use so that they will not
 * cause compiler errors on older unix compilers.
 *
 * Revision 1.4  2002/01/27 16:57:29  mortenson
 * Fixed a compiler warning.
 *
 * Revision 1.3  2002/01/27 14:58:27  spocke
 * Fixed bug issue when reading Windows config files in Unix.
 * Control characters like CR was not handled.
 *
 * Revision 1.2  2002/01/10 08:19:37  mortenson
 * Added the ability to override properties from the command line.
 *
 * Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
 * no message
 *
 */

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "property.h"

#ifdef WIN32
#else
#include <strings.h>
#endif

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
 * Parses a property value and populates any environment variables.
 */
void evaluateEnvironmentVariables(const char *propertyValue, char *buffer) {
    const char *in;
    char *out;
    char envName[256];
    char *envValue;
    char *start;
    char *end;
    int len;

#ifdef _DEBUG
    printf("evaluateEnvironmentVariables('%s', buffer)\n", propertyValue);
#endif

    buffer[0] = '\0';
    in = propertyValue;
    out = buffer;

    /* Loop until we hit the end of string. */
    while (in[0] != '\0') {
#ifdef _DEBUG
        printf("  in='%s', out='%s'\n", in, out);
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
                    len = start - in;
                    memcpy(out, in, len);
                    out += len;
                    /* Copy over the env value */
                    strcpy(out, envValue);
                    out += strlen(out);
                    /* Set the new in pointer */
                    in = end + 1;
                } else {
                    /* Not found.  So copy over the input up until the */
                    /*  second '%'.  Leave it in case it is actually the */
                    /*  start of an environment variable name */
                    len = end - in;
                    memcpy(out, in, len);
                    out += len;
                    out[0] = '\0';
                    in += len;
                }
            } else {
                /* Only a single '%' char was found. Leave it as is. */
                strcpy(out, in);
                in += strlen(in);
                out += strlen(out);
            }
        } else {
            /* No more '%' chars in the string. Copy over the rest. */
            strcpy(out, in);
            in += strlen(in);
            out += strlen(out);
        }
    }
#ifdef _DEBUG
    printf("  final buffer='%s'\n", buffer);
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
        evaluateEnvironmentVariables(propertyValue, buffer);

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

Properties* loadProperties(const char* filename) {
    Properties *properties;
    FILE *stream;
    char buffer[1024];
    char *c;
    char *d;

    /* Look for the specified file. */
    if ((stream = fopen(filename, "rt")) == NULL) {
        /* Unable to open the file. */
        return NULL;
    }

    /* Create a Properties structure. */
    properties = createProperties();

    /* Load in all of the properties */
    do {
        c = fgets(buffer, 1024, stream);
        if (c != NULL) {
            /* Strip the LF off the end of the line. */
            if ((d = strchr(buffer, '\n')) != NULL) {
                *d = '\0';
            }

            /* Only look at lines which contain data and do not start with a '#' */
            if ((strlen(buffer) > 0) && (buffer[0] != '#')) {
                /* printf("%s\n", buffer); */

                /* Locate the first '=' in the line */
                if ((d = strchr(buffer, '=')) != NULL) {
                    /* Null terminate the first half of the line. */
                    *d = '\0';
                    d++;
                    addProperty(properties, buffer, d);
                }
            }
        }
    } while (c != NULL);

    /* Close the file */
    fclose(stream);

    return properties;
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

void addProperty(Properties *properties, const char *propertyName, const char *propertyValue) {
    Property *property;

    /* printf("addProperty(%p, '%s', '%s')\n", properties, propertyName, propertyValue); */

    /* See if the property already exists */
    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        /* This is a new property */
        property = createInnerProperty();

        /* Store a copy of the name */
        property->name = (char *)malloc(sizeof(char) * (strlen(propertyName) + 1));
        strcpy(property->name, propertyName);

        /* Insert this property at the correct location. */
        insertInnerProperty(properties, property);
    }
    
    /* Set the property value. */
    setInnerProperty(property, propertyValue);
}

/**
 * Takes a name/value pair in the form <name>=<value> and attempts to add
 * it to the specified properties table.
 *
 * Returns 0 if successful, otherwise 1
 */
int addPropertyPair(Properties *properties, const char *propertyNameValue) {
    char buffer[1024];
    char *d;

    /* Make a copy of the pair that we can edit */
    strcpy(buffer, propertyNameValue);

    /* Locate the first '=' in the pair */
    if ((d = strchr(buffer, '=')) != NULL) {
        /* Null terminate the first half of the line. */
        *d = '\0';
        d++;
        addProperty(properties, buffer, d);

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
        printf("    name:%s value:%s\n", property->name, property->value);
        property = property->next;
    }
}

