/*
 * Copyright (c) 1999, 2003 TanukiSoftware.org
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
 * Revision 1.5  2003/03/13 15:40:41  mortenson
 * Add the ability to set environment variables from within the configuration
 * file or from the command line.
 *
 * Revision 1.4  2003/02/03 06:55:26  mortenson
 * License transfer to TanukiSoftware.org
 *
 */

#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef _PROPERTY_H
#define _PROPERTY_H

typedef struct Property Property;
struct Property {
    char *name;              /* The name of the property. */
    char *value;             /* The value of the property. */
    int finalValue;          /* TRUE if the Property can not be changed. */
    Property *next;          /* Pointer to the next Property in a linked list */
    Property *previous;      /* Pointer to the next Property in a linked list */
};

typedef struct Properties Properties;
struct Properties {
    Property *first;         /* Pointer to the first property. */
    Property *last;          /* Pointer to the last property.  */
};

/**
 * Create a Properties structure loaded in from the specified file.
 *  Must call disposeProperties to free up allocated memory.
 */
extern int loadProperties(Properties *properties, const char* filename);

/**
 * Create a Properties structure.  Must call disposeProperties to free up
 *  allocated memory.
 */
extern Properties* createProperties();

/**
 * Free all memory allocated by a Properties structure.  The properties
 *  pointer will no longer be valid.
 */
extern void disposeProperties(Properties *properties);

/**
 * Remove a single Property from a Properties.  All associated memory is
 *  freed up.
 */
extern void removeProperty(Properties *properties, const char *propertyName);

/**
 *
 */
extern void addProperty(Properties *properties, const char *propertyName, const char *propertyValue, int finalValue);

/**
 * Takes a name/value pair in the form <name>=<value> and attempts to add
 * it to the specified properties table.
 *
 * Returns 0 if successful, otherwise 1
 */
extern int addPropertyPair(Properties *properties, const char *propertyNameValue, int finalValue);

extern const char* getStringProperty(Properties *properties, const char *propertyName, const char *defaultValue);

extern int getIntProperty(Properties *properties, const char *propertyName, int defaultValue);

extern int getBooleanProperty(Properties *properties, const char *propertyName, int defaultValue);

extern void dumpProperties(Properties *properties);

#endif
