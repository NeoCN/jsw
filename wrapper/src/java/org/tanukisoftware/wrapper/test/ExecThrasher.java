package org.tanukisoftware.wrapper.test;

/*
 * Copyright (c) 1999, 2013 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html
 */

import org.tanukisoftware.wrapper.WrapperJNIError;
import org.tanukisoftware.wrapper.WrapperLicenseError;
import org.tanukisoftware.wrapper.WrapperManager;
import org.tanukisoftware.wrapper.WrapperProcess;
import org.tanukisoftware.wrapper.WrapperProcessConfig;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.util.Random;

/**
 *
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 */
public class ExecThrasher
{
    private static String c_simplewaiter;
    private static String c_encoding;
    
    /*---------------------------------------------------------------
     * Static Methods
     *-------------------------------------------------------------*/
    static
    {
        if ( WrapperManager.isWindows() )
        {
            c_simplewaiter = "../test/simplewaiter.exe";
        }
        else
        {
            c_simplewaiter = "../test/simplewaiter";
        }
        
        // In order to read the output from some processes correctly we need to get the correct encoding.
        //  On some systems, the underlying system encoding is different than the file encoding.
        c_encoding = System.getProperty( "sun.jnu.encoding" );
        if ( c_encoding == null )
        {
            c_encoding = System.getProperty( "file.encoding" );
            if ( c_encoding == null )
            {
                // Default to Latin1
                c_encoding = "Cp1252";
            }
        }
    }
    
    private static void handleInputStream( final InputStream is, final String encoding, final String label )
    {
        Thread runner = new Thread()
        {
            public void run()
            {
                BufferedReader br;
                String line;
                
                try
                {
                    br = new BufferedReader( new InputStreamReader( is, encoding ) );
                    try
                    {
                        while ( ( line = br.readLine() ) != null )
                        {
                            System.out.println( label + ": " + line );
                        }
                    }
                    finally
                    {
                        br.close();
                    }
                }
                catch ( IOException e )
                {
                    e.printStackTrace();
                }
            }
        };
        runner.start();
    }

    private static void handleWrapperProcessInner( WrapperProcess process, long timeoutMS, int threadId, int processId )
        throws IOException
    {
        try
        {
            handleInputStream( process.getInputStream(), c_encoding, "Thrasher Thread #" + threadId + " Process #" + processId + " stdout" );
            handleInputStream( process.getErrorStream(), c_encoding, "Thrasher Thread #" + threadId + " Process #" + processId + " stderr" );
            
            if ( timeoutMS > 0 )
            {
                long start = System.currentTimeMillis();
                while ( process.isAlive() && ( System.currentTimeMillis() - start < timeoutMS ) )
                {
                    try
                    {
                        Thread.sleep( 100 );
                    }
                    catch ( InterruptedException e )
                    {
                    }
                }
                if ( process.isAlive() )
                {
                    System.out.println( "Thrasher Thread #" + threadId + " Process #" + processId + " Timed out waiting for child.  Destroying." );
                    process.destroy();
                }
            }
        }
        finally
        {
            try
            {
                int exitCode = process.waitFor();
                System.out.println( "Thrasher Thread #" + threadId + " Process #" + processId + " exitCode: " + exitCode );
            }
            catch ( InterruptedException e )
            {
                System.out.println( "Thrasher Thread #" + threadId + " Process #" + processId + " Timed out waiting for process to complete." );
            }
        }
    }

    private static void handleWrapperProcess( String command, long timeoutMS, int threadId, int processId )
        throws IOException
    {
        handleWrapperProcessInner( WrapperManager.exec( command ), timeoutMS, threadId, processId );
    }
    
    private static void thrasher( int threadId )
    {
        // We want to work in a repeatable way.
        Random rand = new Random( threadId );
        
        System.out.println( "Thrasher Thread #" + threadId + " Begin" );
        try
        {
            for ( int processId = 0; processId < 1000; processId++ )
            {
                int seconds = rand.nextInt( 10 );
                String command = c_simplewaiter + " 0 " + seconds;
                System.out.println( "Thrasher Thread #" + threadId + " Process #" + processId + " Launch (" + seconds + " seconds)" );
                try
                {
                    handleWrapperProcess( command, ( seconds + 5 ) * 1000, threadId, processId );
                }
                catch ( IOException e )
                {
                    System.out.println( "Thrasher Thread #" + threadId + " Process #" + processId + " Launch Failed." );
                    e.printStackTrace();
                }
            }
        }
        finally
        {
            System.out.println( "Thrasher Thread #" + threadId + " End" );
        }
    }
    
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    public static void main( String[] args )
    {
        
        // In order to read the output from some processes correctly we need to get the correct encoding.
        //  On some systems, the underlying system encoding is different than the file encoding.
        c_encoding = System.getProperty( "sun.jnu.encoding" );
        if ( c_encoding == null )
        {
            c_encoding = System.getProperty( "file.encoding" );
            if ( c_encoding == null )
            {
                // Default to Latin1
                c_encoding = "Cp1252";
            }
        }
        System.out.println( "Communicate with child processes using encoding: " + c_encoding );
        
        for ( int i = 0; i < 100; i++ )
        {
            final int threadId = i;
            Thread thread = new Thread( "ExecThrasher-" + i )
            {
                public void run()
                {
                    thrasher( threadId );
                }
            };
            thread.start();
        }
    }
}
