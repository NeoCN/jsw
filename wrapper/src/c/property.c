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
// Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
// no message
//

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "property.h"

/**
 * Private function to find a Property structure.
 */
Property* getInnerProperty(Properties *properties, const char *propertyName) {
    Property *property;
    int cmp;

    // Loop over the properties which are in order and look for the specified property.
    property = properties->first;
    while (property != NULL) {
        cmp = strcmp(property->name, propertyName);
        if (cmp > 0) {
            // This property would be after the one being looked for, so it does not exist.
            return NULL;
        } else if (cmp == 0) {
            // We found it.
            return property;
        }
        // Keep looking
        property = property->next;
    }
    // We did not find the property being looked for.
    return NULL;
}

void insertInnerProperty(Properties *properties, Property *newProperty) {
    Property *property;
    int cmp;

    // Loop over the properties which are in order and look for the specified property.
    // This function assumes that Property is not already in properties.
    property = properties->first;
    while (property != NULL) {
        cmp = strcmp(property->name, newProperty->name);
        if (cmp > 0) {
            // This property would be after the new property, so insert it here.
            newProperty->previous = property->previous;
            newProperty->next = property;
            if (property->previous == NULL) {
                // This was the first property
                properties->first = newProperty;
            } else {
                property->previous->next = newProperty;
            }
            property->previous = newProperty;

            // We are done, so return
            return;
        }

        property = property->next;
    }

    // The new property needs to be added at the end
    newProperty->previous = properties->last;
    if (properties->last == NULL) {
        // This will be the first property.
        properties->first = newProperty;
    } else {
        // Point the old last property to the new last property.
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

void setInnerProperty(Property *property, const char *propertyValue) {
    // Free any existing value
    if (property->value != NULL) {
        free(property->value);
    }

    // Set the new value using a copy of the provided value.
    if (propertyValue == NULL) {
        property->value = NULL;
    } else {
        property->value = (char *)malloc(sizeof(char) * (strlen(propertyValue) + 1));
        strcpy(property->value, propertyValue);
    }
}

Properties* loadProperties(const char* filename) {
    Properties *properties;
    FILE *stream;
    //int len;
    char buffer[1024];
    char *c;
    char *d;

    // Look for the specified file.
    if ((stream = fopen(filename, "r+t")) == NULL) {
        // Unable to open the file.
        return NULL;
    }

    // Create a Properties structure.
    properties = createProperties();

    // Load in all of the properties
    do {
        c = fgets(buffer, 1024, stream);
        if (c != NULL) {
            // Strip the LF off the end of the line.
            if ((d = strchr(buffer, '\n')) != NULL) {
                *d = '\0';
            }

            // Only look at lines which contain data and do not start with a '#'
            if ((strlen(buffer) > 0) && (buffer[0] != '#')) {
                //printf("%s\n", buffer);

                // Locate the first '=' in the line
                if ((d = strchr(buffer, '=')) != NULL) {
                    // Null terminate the first half of the line.
                    *d = '\0';
                    d++;
                    addProperty(properties, buffer, d);
                }
            }
        }
    } while (c != NULL);

    // Close the file
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
    // Loop and dispose any Property structures
    Property *tempProperty;
    Property *property = properties->first;
    properties->first = NULL;
    properties->last = NULL;
    while (property != NULL) {
        // Save the next property
        tempProperty = property->next;

        // Clean up the current property
        disposeInnerProperty(property);

        // set the current property to the next.
        property = tempProperty;
    }

    // Dispose the Properties structure
    free(properties);
}

void removeProperty(Properties *properties, const char *propertyName) {
    Property *property;
    Property *next;
    Property *previous;

    // Look up the property
    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        // The property did not exist, so nothing to do.
    } else {
        next = property->next;
        previous = property->previous;

        // Disconnect the property
        if (next == NULL) {
            // This was the last property
            properties->last = previous;
        } else {
            next->previous = property->previous;
        }
        if (previous ==  NULL) {
            // This was the first property
            properties->first = next;
        } else {
            previous->next = property->next;
        }

        // Now that property is disconnected, if can be disposed.
        disposeInnerProperty(property);
    }
}

void addProperty(Properties *properties, const char *propertyName, const char *propertyValue) {
    Property *property;

    //printf("addProperty(%p, '%s', '%s')\n", properties, propertyName, propertyValue);

    // See if the property already exists
    property = getInnerProperty(properties, propertyName);
    if (property == NULL) {
        // This is a new property
        property = createInnerProperty();

        // Store a copy of the name
        property->name = (char *)malloc(sizeof(char) * (strlen(propertyName) + 1));
        strcpy(property->name, propertyName);

        // Insert this property at the correct location.
        insertInnerProperty(properties, property);
    }
    
    // Set the property value.
    setInnerProperty(property, propertyValue);
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
        // A value was set.  Set to true only if the value equals "true"
#ifdef WIN32
        if (strcmp(strlwr(property->value), "true") == 0) {
#else // UNIX
        if (strcasecmp(property->value, "true") == 0) {
#endif
            return TRUE;
        } else {
            return FALSE;
        }
    }
}

