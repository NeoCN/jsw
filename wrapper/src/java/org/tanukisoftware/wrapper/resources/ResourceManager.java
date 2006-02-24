package org.tanukisoftware.wrapper.resources;

/*
 * Copyright (c) 1999, 2006 Tanuki Software Inc.
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
 */

// $Log$
// Revision 1.7  2006/02/24 05:45:58  mortenson
// Update the copyright.
//
// Revision 1.6  2006/02/03 05:39:04  mortenson
// Remove support for image resources.  They are not used and cause problems
// when run on non-sun JVMs
//
// Revision 1.5  2005/05/23 02:40:26  mortenson
// Update the copyright information.
//
// Revision 1.4  2004/11/29 13:15:38  mortenson
// Fix some javadocs problems.
//
// Revision 1.3  2004/01/16 04:42:00  mortenson
// The license was revised for this version to include a copyright omission.
// This change is to be retroactively applied to all versions of the Java
// Service Wrapper starting with version 3.0.0.
//
// Revision 1.2  2003/04/03 04:05:22  mortenson
// Fix several typos in the docs.  Thanks to Mike Castle.
//
// Revision 1.1  2003/02/03 06:55:29  mortenson
// License transfer to TanukiSoftware.org
//

import java.util.Hashtable;
import java.util.MissingResourceException;
import java.util.ResourceBundle;
import java.text.MessageFormat;

/**
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
public class ResourceManager
{
    private static Hashtable m_resources = new Hashtable();
    
    /**
     * Whenever the Default Locale of the JVM is changed, then any existing
     *	resource bundles will need to update their values.  The
     *	_classRefreshCounter static variable gets updated whenever a refresh
     *	is done.  The m_refreshCounter variable then is used to tell whether
     *	an individual ResourceManager is up to date or not.
     */
    private int m_refreshCounter;
    private static int m_staticRefreshCounter = 0;
    
    /**
     * The ResourceBundle for this locale.
     */
    private ResourceBundle m_bundle;
    private String         m_bundleName;
    
    /**
     * Private Constructor.
     */
    private ResourceManager( String resourceName )
    {
        // Resolve the bundle name based on this class so that it will work correctly with obfuscators.
        String className = this.getClass().getName();
        // Strip off the class name at the end, keeping the ending '.'
        int pos = className.lastIndexOf( '.' );
        if ( pos > 0 )
        {
            m_bundleName = className.substring( 0, pos + 1 );
        }
        else
        {
            m_bundleName = "";
        }
        
        m_bundleName += resourceName;
        
        // Now load the bundle for the current Locale
        refreshBundle();
    }
    
    private void refreshBundle()
    {
        try
        {
            m_bundle = ResourceBundle.getBundle( m_bundleName );
        }
        catch ( MissingResourceException e )
        {
            System.out.println( e );
        }
        
        m_refreshCounter = m_staticRefreshCounter;
    }
    
    /**
     * Returns the default resource manager.
     * An instance of the ResourceManager class is created the first 
     * time the method is called.
     */
    public static ResourceManager getResourceManager()
    {
        return getResourceManager( null );
    }
    
    /**
     * Returns the named resource manager.
     * An instance of the ResourceManager class is created the first 
     * time the method is called.
     *
     * @param resourceName  The name of the desired resource
     */
    public static synchronized ResourceManager getResourceManager( String resourceName )
    {
        if ( resourceName == null )
        {
            resourceName = "resource";
        }
        ResourceManager resource = (ResourceManager)m_resources.get( resourceName );
        if ( resource == null )
        {
            resource = new ResourceManager( resourceName );
            m_resources.put( resourceName, resource );
        }
        return resource;
    }

    /**
     * Clears the resource manager's cache of bundles (this should be called 
     * if the default locale for the application changes).
     */
    public static synchronized void refresh()
    {
        m_resources = new Hashtable();
        m_staticRefreshCounter++;
    }
    
    private String getString( String key )
    {
        // Make sure that the ResourceManager is up to date in a thread safe way
        synchronized(this)
        {
            if ( m_refreshCounter != m_staticRefreshCounter )
            {
                refreshBundle();
            }
        }
        
        String msg;
        if ( m_bundle == null )
        {
            msg = key;
        }
        else
        {
            try
            {
                msg = m_bundle.getString( key );
            }
            catch ( MissingResourceException ex )
            {
                msg = key;
                System.out.println( key + " is missing from resource bundle \"" + m_bundleName
                    + "\"" );
            }
        }
        
        return msg;
    }
    
    /**
     * Returns a string that has been obtained from the resource manager
     *
     * @param key           The string that is the key to the translated message
     *
     */
    public String format( String key )
    {
        return getString( key );
    }
    
    /**
     * Returns a string that has been obtained from the resource manager then
     * formatted using the passed parameters.
     *
     * @param pattern       The string that is the key to the translated message
     * @param o0            The param passed to format replaces {0}
     *
     */
    public String format( String pattern, Object o0 )
    {
        return MessageFormat.format( getString( pattern ), new Object[] { o0 } );
    }

   /**
     * Returns a string that has been obtained from the resource manager then
     * formatted using the passed parameters.
     *
     * @param pattern       The string that is the key to the translated message
     * @param o0            The param passed to format replaces {0}
     * @param o1            The param passed to format replaces {1}
     *
     */
    public String format( String pattern, Object o0, Object o1 )
    {
        return MessageFormat.format( getString( pattern ), new Object[] { o0,o1 } );
    }

   /**
     * Returns a string that has been obtained from the resource manager then
     * formatted using the passed parameters.
     *
     * @param pattern       The string that is the key to the translated message
     * @param o0            The param passed to format replaces {0}
     * @param o1            The param passed to format replaces {1}
     * @param o2            The param passed to format replaces {2}
     *
     */
    public String format( String pattern, Object o0, Object o1, Object o2 )
    {
        return MessageFormat.format( getString( pattern ), new Object[] { o0,o1,o2 } );
    }

   /**
     * Returns a string that has been obtained from the resource manager then
     * formatted using the passed parameters.
     *
     * @param pattern       The string that is the key to the translated message
     * @param o0            The param passed to format replaces {0}
     * @param o1            The param passed to format replaces {1}
     * @param o2            The param passed to format replaces {2}
     * @param o3            The param passed to format replaces {3}
     *
     */
    public String format( String pattern, Object o0, Object o1, Object o2, Object o3 )
    {
        return MessageFormat.format( getString( pattern ), new Object[] { o0,o1,o2,o3 } );
    }

    // add more if you need them...
}

