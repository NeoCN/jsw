package com.silveregg.wrapper.resources;

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

import java.util.*;
import java.text.MessageFormat;
import com.sun.image.codec.jpeg.JPEGCodec;
import com.sun.image.codec.jpeg.JPEGImageDecoder;
import java.awt.image.BufferedImage;
import java.io.InputStream;
import java.io.IOException;

/**
 *
 * Some helper functions for handling i18n issues. One instance of this class
 * should be created for each resource.<P>
 *
 * The ResourceManager is created by a call to <CODE>getResourceManager()</CODE>.
 * The (optional) parameter is the name of the desired resource, not including the
 * <code>.properties</code> suffix.
 *
 * For example,<P>
 * <CODE>
 * ResourceManager res = getResourceBundle();
 * </CODE>
 * <P>to get the default resources, or<P>
 * <CODE>
 * ResourceManager res = getResourceBundle("sql");
 * </CODE>
 * <P>
 * to load the resources in <code>sql.properties</code>.
 *
 * To use the ResourceManager make a call to any of the <CODE>format()</CODE>
 * methods. If a string is not found in the bundle the key is returned and a 
 * message is logged to the debug channel for this class.
 *
 */
public class ResourceManager {
    private static Hashtable _resources = new Hashtable();
    
    /**
     * Whenever the Default Locale of the JVM is changed, then any existing
     *	resource bundles will need to update their values.  The
     *	_classRefreshCounter static variable gets updated whenever a refresh
     *	is done.  The _refreshCounter variable then is used to tell whether
     *	an individual ResourceManager is up to date or not.
     */
    private int _refreshCounter;
    private static int _staticRefreshCounter = 0;
    
    /**
     * The ResourceBundle for this locale.
     */
    private ResourceBundle _bundle;
    private String         _bundleName;
    
    /**
     * Private Constructor.
     */
    private ResourceManager(String resourceName) {
        // Resolve the bundle name based on this class so that it will work correctly with obfuscators.
        String className = this.getClass().getName();
        // Strip off the class name at the end, keeping the ending '.'
        int pos = className.lastIndexOf('.');
        if (pos > 0) {
            _bundleName = className.substring(0, pos + 1);
        } else {
            _bundleName = "";
        }
        
        _bundleName += resourceName;
        
        // Now load the bundle for the current Locale
        refreshBundle();
    }
    
    private void refreshBundle() {
        try {
            _bundle = ResourceBundle.getBundle(_bundleName);
        } catch (MissingResourceException e) {
            System.out.println(e);
        }
        
        _refreshCounter = _staticRefreshCounter;
    }
    
    /**
     * Returns the default resource manager.
     * An instance of the ResourceManager class is created the first 
     * time the method is called.
     */
    public static ResourceManager getResourceManager() {
        return getResourceManager(null);
    }
    
    /**
     * Returns the named resource manager.
     * An instance of the ResourceManager class is created the first 
     * time the method is called.
     *
     * @param resourceName  The name of the desired resource
     */
    public static synchronized ResourceManager getResourceManager(String resourceName) {
        if (resourceName == null) {
            resourceName = "resource";
        }
        ResourceManager resource = (ResourceManager)_resources.get(resourceName);
        if (resource == null) {
            resource = new ResourceManager(resourceName);
            _resources.put(resourceName, resource);
        }
        return resource;
    }

    /**
     * Clears the resource manager's cache of bundles (this should be called 
     * if the default locale for the application changes).
     */
    public static synchronized void refresh() {
        _resources = new Hashtable();
        _staticRefreshCounter++;
    }
    
    private String getString(String key) {
        // Make sure that the ResourceManager is up to date in a thread safe way
        synchronized(this) {
            if (_refreshCounter != _staticRefreshCounter) {
                refreshBundle();
            }
        }
        
        String msg;
        if (_bundle == null) {
            msg = key;
        } else {
            try {
                msg = _bundle.getString(key);
            } catch (MissingResourceException ex) {
                msg = key;
                System.out.println(key + " is missing from resource bundle \"" + _bundleName+"\"");
            }
        }
        return msg;
    }

    /**
     * Returns the requested buffered image resource.
     *
     * @param filename The name of the file containing the desired image
     */
    public BufferedImage getBufferedImage(String filename) {
        BufferedImage image = null;
        InputStream is = getClass().getResourceAsStream(filename);
        if (is != null) {
            try {
                JPEGImageDecoder decoder = JPEGCodec.createJPEGDecoder(is);
                image = decoder.decodeAsBufferedImage();
            } catch (IOException e) {}
        }
        return image;
    }
    
    /**
     * Returns a string that has been obtained from the resource manager
     *
     * @param key           The string that is the key to the translated message
     *
     */
    public String format(String key) {
        return getString(key);
    }
    
    /**
     * Returns a string that has been obtained from the resource manager then
     * formatted using the passed parameters.
     *
     * @param key           The string that is the key to the translated message
     * @param o0            The param passed to format replaces {0}
     *
     */
    public String format(String pattern, Object o0) {
        return MessageFormat.format(getString(pattern), new Object[] {o0});
    }

   /**
     * Returns a string that has been obtained from the resource manager then
     * formatted using the passed parameters.
     *
     * @param key           The string that is the key to the translated message
     * @param o0            The param passed to format replaces {0}
     * @param o1            The param passed to format replaces {1}
     *
     */
    public String format(String pattern, Object o0, Object o1) {
        return MessageFormat.format(getString(pattern), new Object[] {o0,o1});
    }

   /**
     * Returns a string that has been obtained from the resource manager then
     * formatted using the passed parameters.
     *
     * @param key           The string that is the key to the translated message
     * @param o0            The param passed to format replaces {0}
     * @param o1            The param passed to format replaces {1}
     * @param o2            The param passed to format replaces {2}
     *
     */
    public String format(String pattern, Object o0, Object o1, Object o2) {
        return MessageFormat.format(getString(pattern), new Object[] {o0,o1,o2});
    }

   /**
     * Returns a string that has been obtained from the resource manager then
     * formatted using the passed parameters.
     *
     * @param key           The string that is the key to the translated message
     * @param o0            The param passed to format replaces {0}
     * @param o1            The param passed to format replaces {1}
     * @param o2            The param passed to format replaces {2}
     * @param o3            The param passed to format replaces {3}
     *
     */
    public String format(String pattern, Object o0, Object o1, Object o2, Object o3) {
        return MessageFormat.format(getString(pattern), new Object[] {o0,o1,o2,o3});
    }

    // add more if you need them...
}

