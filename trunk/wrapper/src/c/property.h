/*
 * Copyright (c) 1999, 2004 Tanuki Software
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of the Java Service Wrapper and associated
 * documentation files (the "Software"), to deal in the Software
 * without  restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sub-license,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
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
 * 
 *
 * $Log$
 * Revision 1.9  2004/01/16 04:41:59  mortenson
 * The license was revised for this version to include a copyright omission.
 * This change is to be retroactively applied to all versions of the Java
 * Service Wrapper starting with version 3.0.0.
 *
 * Revision 1.8  2003/09/09 14:18:10  mortenson
 * Fix a problem where not all properties specified on the command line worked
 * correctly when they included spaces.
 *
 * Revision 1.7  2003/08/02 06:49:13  mortenson
 * Changed the way environment variables are loaded from the registry on Windows
 * platforms so users will no longer get warning messages about not being able
 * to handle very large environment variables.
 *
 * Revision 1.6  2003/04/03 04:05:22  mortenson
 * Fix several typos in the docs.  Thanks to Mike Castle.
 *
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

/* This defines the largest environment variable that we are able
 *  to work with.  It can be expanded if needed. */
#define MAX_PROPERTY_NAME_LENGTH 512
#define MAX_PROPERTY_VALUE_LENGTH 16384
#define MAX_PROPERTY_NAME_VALUE_LENGTH MAX_PROPERTY_NAME_LENGTH + 1 + MAX_PROPERTY_VALUE_LENGTH

typedef struct Property Property;
struct Property {
    char *name;              /* The name of the property. */
    char *value;             /* The value of the property. */
    int finalValue;          /* TRUE if the Property can not be changed. */
    int quotable;            /* TRUE if quotes can be optionally added around the value. */
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
extern void addProperty(Properties *properties, const char *propertyName, const char *propertyValue, int finalValue, int quotable);

/**
 * Takes a name/value pair in the form <name>=<value> and attempts to add
 * it to the specified properties table.
 *
 * Returns 0 if successful, otherwise 1
 */
extern int addPropertyPair(Properties *properties, const char *propertyNameValue, int finalValue, int quotable);

extern const char* getStringProperty(Properties *properties, const char *propertyName, const char *defaultValue);

extern int getIntProperty(Properties *properties, const char *propertyName, int defaultValue);

extern int getBooleanProperty(Properties *properties, const char *propertyName, int defaultValue);

extern int isQuotableProperty(Properties *properties, const char *propertyName);

extern void dumpProperties(Properties *properties);

#endif
