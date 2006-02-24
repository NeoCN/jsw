package com.silveregg.wrapper;

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
// Revision 1.6  2006/02/24 05:45:57  mortenson
// Update the copyright.
//
// Revision 1.5  2005/05/23 02:38:19  mortenson
// Update the copyright information.
//
// Revision 1.4  2004/01/16 04:42:01  mortenson
// The license was revised for this version to include a copyright omission.
// This change is to be retroactively applied to all versions of the Java
// Service Wrapper starting with version 3.0.0.
//
// Revision 1.3  2003/04/03 04:05:23  mortenson
// Fix several typos in the docs.  Thanks to Mike Castle.
//
// Revision 1.2  2003/02/03 06:55:28  mortenson
// License transfer to TanukiSoftware.org
//

/**
 * @deprecated This class has been deprecated in favor of the
 *             org.tanukisoftware.wrapper.WrapperStartStopApp class.
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 * @version $Revision$
 */
public class WrapperStartStopApp
{
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    /**
     * Disallow instantiation of this class.
     */
    private WrapperStartStopApp()
    {
    }
    
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    /**
     * Used to Wrapper enable a standard Java application.  This main
     *  expects the first argument to be the class name of the application
     *  to launch.  All remaining arguments will be wrapped into a new
     *  argument list and passed to the main method of the specified
     *  application.
     */
    public static void main( String args[] )
    {
        org.tanukisoftware.wrapper.WrapperStartStopApp.main( args );
    }
}

