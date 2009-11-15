package org.tanukisoftware.wrapper.test;

/*
 * Copyright (c) 1999, 2009 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.org/doc/english/licenseOverview.html
 */

import org.tanukisoftware.wrapper.WrapperManager;
import org.tanukisoftware.wrapper.WrapperListener;

import java.io.IOException;

/**
 *
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 */
public class RuntimeExec
{
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    public static void main( String[] args )
    {
        System.out.println( "Launching Child Process." );
        
        String command;
        String os = System.getProperty( "os.name" ).toLowerCase();
        if ( os.indexOf( "windows" ) >= 0 )
        {
            // Windows
            command = "notepad.exe";
        }
        else if ( os.indexOf( "linux" ) >= 0 )
        {
            // Linux
            command = "vmstat 1";
        }
        else
        {
            // UNIX
            command = "ls -al";
        }
        
        Runtime runtime = Runtime.getRuntime();
        Process process;
        try
        {
            process = runtime.exec( command );
        }
        catch ( IOException e )
        {
            System.out.println( "Failed to launch child process." );
            return;
        }
        
        System.out.println( "Launched the child process." );
        
        while ( !WrapperManager.isShuttingDown() )
        {
            try
            {
                int exitValue = process.exitValue();
                System.out.println( "Child process exited unexpectedly with exit code: " + exitValue );
                return;
            }
            catch ( IllegalThreadStateException e )
            {
                // Still running.  Good.
            }
            
            try
            {
                Thread.sleep( 250 );
            }
            catch ( InterruptedException e )
            {
                // Continue
            }
        }
        
        // The Wrapper is shutting down.
        System.out.println( "Stop the child process." );
        process.destroy();
        
        System.out.println( "Wait for child process to die." );
        try
        {
            process.waitFor();
        }
        catch ( InterruptedException e )
        {
            System.out.println( "Failed to wait for child." + e );
            return;
        }
        
        int exitValue = process.exitValue();
        System.out.println( "Child exited with exit code: " + exitValue );
        
        System.out.println( "All Done." );
    }
}

