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

#ifndef _PROPERTY_H
#define _PROPERTY_H

#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0
#endif

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

extern int setEnv( const char *name, const char *value );

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

extern const char* getFileSafeStringProperty(Properties *properties, const char *propertyName, const char *defaultValue);

/**
 * Returns a sorted array of all properties beginning with {propertyNameBase}.
 *Å@ Only numerical characters can be returned betweenÅ@the two.
 *
 * @param properties The full properties structure.
 * @param propertyNameHead All matching properties must begin with this value.
 * @param all If FALSE then the array will start with #1 and loop up until the
 *            next property is not found, if TRUE then all properties will be
 *            returned, even if there are gaps in the series.
 * @param propertyNames Returns a pointer to a NULL terminated array of
 *                      property names.
 * @param propertyValues Returns a pointer to a NULL terminated array of
 *                       property values.
 *
 * @return 0 if successful, -1 if there was an error.
 */
extern int getStringProperties(Properties *properties, const char *propertyNameHead, const char *propertyNameTail, int all, char ***propertyNames, char ***propertyValues, long unsigned int **propertyIndices);

/**
 * Frees up an array of properties previously returned by getStringProperties().
 */
extern void freeStringProperties(char **propertyNames, char **propertyValues, long unsigned int *propertyIndices);

extern int checkPropertyEqual(Properties *properties, const char *propertyName, const char *defaultValue, const char *value);

extern int getIntProperty(Properties *properties, const char *propertyName, int defaultValue);

extern int getBooleanProperty(Properties *properties, const char *propertyName, int defaultValue);

extern int isQuotableProperty(Properties *properties, const char *propertyName);

extern void dumpProperties(Properties *properties);

/** Creates a linearized representation of all of the properties.
 *  The returned buffer must be freed by the calling code. */
extern char *linearizeProperties(Properties *properties, char separator);

#endif
