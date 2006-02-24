package org.tanukisoftware.wrapper.test;

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
 */

// $Log$
// Revision 1.3  2006/02/24 05:45:59  mortenson
// Update the copyright.
//
// Revision 1.2  2005/05/23 02:39:30  mortenson
// Update the copyright information.
//
// Revision 1.1  2004/08/06 09:06:34  mortenson
// Add a test to test the setting of system properties.
//

import org.tanukisoftware.wrapper.WrapperManager;

/**
 * This test is to make sure that property values set in the wrapper config file
 *  are handled and passed into the JVM as expected.
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 * @version $Revision$
 */
public class SystemProperty
{
    private static int m_exitCode = 0;
    
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    public static void main( String[] args )
    {
        testProperty( "VAR1", "abc" );
        testProperty( "VAR2", "\\" );
        testProperty( "VAR3", "\"" );
        testProperty( "VAR4", "abc" );
        testProperty( "VAR5", "\\" );
        testProperty( "VAR6", "\\\\" );
        testProperty( "VAR7", "\"" );

        System.out.println("Main complete.");
        
        System.exit( m_exitCode );
    }
    
    private static void testProperty( String name, String expectedValue )
    {
        System.out.println( "Testing system property: " + name );
        System.out.println( "  Expected:" + expectedValue );
        
        String value = System.getProperty( name );
        System.out.println( "  Value   :" + value );
        
        if ( expectedValue.equals( value ) )
        {
            System.out.println( "  OK" );
        }
        else
        {
            System.out.println( " FAILED!!!" );
            m_exitCode = 1;
        }
        
        System.out.println();
    }
}

