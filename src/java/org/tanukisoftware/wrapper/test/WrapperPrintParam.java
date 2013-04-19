package org.tanukisoftware.wrapper.test;

/*
 * Copyright (c) 1999, 2012 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html
 */

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;
import java.util.ListIterator;
import java.util.Arrays;

public class WrapperPrintParam
{
    public static void main( String[] args )
    {
        try
        {
            Class cRuntimeMXBean = Class.forName( "java.lang.management.RuntimeMXBean" );
            Class cManagementFactory = Class.forName( "java.lang.management.ManagementFactory" );
            Method mGetRuntimeMXBean = cManagementFactory.getMethod( "getRuntimeMXBean",
                                                                     null );
            Method mGetInputArguments = cRuntimeMXBean.getMethod( "getInputArguments",
                                                                  null );
            Object runtimemxBean = mGetRuntimeMXBean.invoke( null,
                                                             ( Object[] ) null );
            List jvm_args = ( List ) mGetInputArguments.invoke( runtimemxBean,
                                                                ( Object[] ) null );
            if ( jvm_args.size() < 2 )
            {
                System.out.println( jvm_args.size() + " JVM Parameter:" );
            }
            else
            {
                System.out.println( jvm_args.size() + " JVM Parameters:" );
            }
            for ( ListIterator i = jvm_args.listIterator(); i.hasNext(); )
            {
                String arg = ( String ) i.next();
                System.out.println( "  > " + arg );
            }

            List app_args = Arrays.asList( args );
            if ( app_args.size() < 2 )
            {
                System.out.println( app_args.size() + " Application Parameter:" );
            }
            else
            {
                System.out.println( app_args.size()
                        + " Application Parameters:" );
            }
            for ( ListIterator i = app_args.listIterator(); i.hasNext(); )
            {
                String arg = ( String ) i.next();
                System.out.println( "  > " + arg );
            }
        }
        catch ( ClassNotFoundException e )
        {
            e.printStackTrace();
        }
        catch ( IllegalArgumentException e )
        {
            e.printStackTrace();
        }
        catch ( IllegalAccessException e )
        {
            e.printStackTrace();
        }
        catch ( InvocationTargetException e )
        {
            e.printStackTrace();
        }
        catch ( SecurityException e )
        {
            e.printStackTrace();
        }
        catch ( NoSuchMethodException e )
        {
            e.printStackTrace();
        }
    }
}
