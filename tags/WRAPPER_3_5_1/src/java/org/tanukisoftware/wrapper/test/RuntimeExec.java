package org.tanukisoftware.wrapper.test;

/*
 * Copyright (c) 1999, 2010 Tanuki Software, Ltd.
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
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Random;

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
        Random rand = new Random();
        System.out.println( Main.getRes().getString( "Is DYNAMIC supported? A:" ) + WrapperProcessConfig.isSupported( WrapperProcessConfig.DYNAMIC ) );
        System.out.println( Main.getRes().getString( "Is FORK_EXEC supported? A:" ) + WrapperProcessConfig.isSupported( WrapperProcessConfig.FORK_EXEC ) );
        System.out.println( Main.getRes().getString( "Is VFORK_EXEC supported? A:" ) + WrapperProcessConfig.isSupported( WrapperProcessConfig.VFORK_EXEC ) );
        System.out.println( Main.getRes().getString( "Is POSIX_SPAWN supported? A:" ) + WrapperProcessConfig.isSupported( WrapperProcessConfig.POSIX_SPAWN ) );

        for ( int i = 1 ; i < 8; i++ )
        {
            switch ( i )
            {
            case 1: 
                System.out.println( i + Main.getRes().getString( " start a small child, dont care about output but call waitfor..." ) );
                try
                {
                    WrapperProcess proc = WrapperManager.exec( "../test/simplewaiter 65 1" );
                    proc.getOutputStream().close();
                    System.out.println( Main.getRes().getString( "{0} small child {1} is alive {2}",  new Object[]{ new Integer( i ), new Integer( proc.getPID() ) , new Boolean( proc.isAlive() ) } ) );
                    System.out.println( Main.getRes().getString( "{0} process (PID= {1}) finished with code {2}", new Object[]{ new Integer( i ), new Integer ( proc.getPID() ), new Integer( proc.waitFor() ) } ) );
                }
                catch ( InterruptedException e )
                {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                catch ( IOException e )
                {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                break;

            case 2:
                System.out.println( i + Main.getRes().getString( " start longrunning child dont wait..." ) );
                try
                {
                    WrapperProcess proc;
                    System.out.println( i + Main.getRes().getString( " first, try to vfork..." ) ); 
                    if ( WrapperProcessConfig.isSupported( WrapperProcessConfig.VFORK_EXEC ) ) 
                    {
                        System.out.println( i + Main.getRes().getString( " vfork is supported" ) ); 
                        proc = WrapperManager.exec( "../test/simplewaiter " + ( rand.nextInt( 200 ) + 1 ) + " " + rand.nextInt( 30 ), new WrapperProcessConfig().setStartType(WrapperProcessConfig.VFORK_EXEC) );                    	
                    }
                    else
                    {
                        System.out.println( i + Main.getRes().getString( " vfork is not supported" ) ); 
                        proc = WrapperManager.exec( "../test/simplewaiter " + ( rand.nextInt( 200 ) + 1 ) + " " + rand.nextInt( 30 ) );
                    }
                    
                    System.out.println( i + Main.getRes().getString( " longrunning child {0} is alive {1}" , new Object[]{ new Integer( proc.getPID() ), new Boolean( proc.isAlive() ) } ) );
                    // System.out.println( i + " process ( PID= " + proc.getPID() + " ) finished with code " + proc.waitFor() );
                }
                catch ( IOException e )
                {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                break;

            case 3: 
                try
                {
                    System.out.println( i + Main.getRes().getString( " spawn a small command..." ) );
                    WrapperProcess p;
                    if ( WrapperProcessConfig.isSupported( WrapperProcessConfig.POSIX_SPAWN ) )
                    {
                        System.out.println( i + Main.getRes().getString( " spawn is supported." ) );
                        p = WrapperManager.exec( "../test/simplewaiter 0 15", new WrapperProcessConfig().setStartType(WrapperProcessConfig.POSIX_SPAWN) );
                    }
                    else
                    {
                        System.out.println( i + Main.getRes().getString( " spawn is not supported." ) );
                        p = WrapperManager.exec( "../test/simplewaiter 0 15" );
                    }
                    // System.out.println(i + " " + p.toString() + " exit " + p.waitFor());
                    BufferedReader br = new BufferedReader( new InputStreamReader( p.getInputStream() ) );
                    try
                    {
                        String line = "";
                        while ( ( line = br.readLine() ) != null )
                        {
                            System.out.println( i + " out..:" +  line );
                        }
                    }
                    finally
                    {
                        br.close();
                    }
                }
                catch ( Exception e )
                {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                break;

            case 4: 
                System.out.println( i + Main.getRes().getString( " start a small child, change the environment and read output..." ) );
                try
                {
                    WrapperProcessConfig wpm = new WrapperProcessConfig();
                    java.util.Map environment = wpm.getEnvironment();
                    System.out.println( i + Main.getRes().getString( " size of Environment map (before calling clear()) = " ) + environment.size() );
                    environment.clear();
                    environment.put( "TEST", "TEST123" );
                    System.out.println( i + Main.getRes().getString( " size of Environment map = " ) + environment.size() );
                    WrapperProcess proc = WrapperManager.exec( "../test/simplewaiter "+ rand.nextInt(200) +" 3", wpm );
                    proc.getOutputStream().close();

                    System.out.println( i + Main.getRes().getString( " small child {0} is alive {1}" , new Object[]{ new Integer( proc.getPID() ), new Boolean( proc.isAlive() ) } ) );
                    BufferedReader br = new BufferedReader( new InputStreamReader( proc.getInputStream() ) );
                    try
                    {
                        String line = "";
                        while ( ( line = br.readLine() ) != null )
                        {
                            System.out.println( i + " out..:" +  line );
                        }
                    }
                    finally
                    {
                        br.close();
                    }

                    br = new BufferedReader( new InputStreamReader( proc.getErrorStream() ) );
                    try
                    {
                        String line = "";
                        while ((line = br.readLine()) != null )
                        {
                            System.out.println( line );
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
                break;
            case 5:
                System.out.println( i + Main.getRes().getString( " start longrunning child, change working dir, call waitFor and finally read output..." ) );
                try
                {
                    WrapperProcessConfig wpm = new WrapperProcessConfig();
                    if ( WrapperProcessConfig.isSupported( WrapperProcessConfig.FORK_EXEC ) || WrapperProcessConfig.isSupported( WrapperProcessConfig.VFORK_EXEC ) )
                    {
                        wpm.setStartType( WrapperProcessConfig.isSupported( WrapperProcessConfig.FORK_EXEC ) ? WrapperProcessConfig.FORK_EXEC : WrapperProcessConfig.VFORK_EXEC );
                        System.out.println( i + Main.getRes().getString( " changing the working directory is supported" ) );
                        wpm.setWorkingDirectory( new File("..") );
                    }
                    else
                    {
                        System.out.println( i + Main.getRes().getString( " changing the working directory is not supported" ) );
                    }
                    WrapperProcess proc;
                    try
                    {
                        System.out.println( i + Main.getRes().getString( " try to call dir" ) ); 
                        proc = WrapperManager.exec( "cmd.exe /c dir", wpm );
                    }
                    catch ( IOException e )
                    {
                        System.out.println( i + Main.getRes().getString(  " dir failed. most likely we are not on Windows, try ls -l before giving up." ) );
                        proc = WrapperManager.exec( "ls -l", wpm );
                    }

                    System.out.println( i + " PID = " + proc.getPID() );
                    System.out.println( i + Main.getRes().getString( " process (PID= {0}) finished with code " , new Integer( proc.getPID() ) ) +  proc.waitFor() );

                    System.out.println( i + Main.getRes().getString( " now read the output" ) );
                    BufferedReader br = new BufferedReader( new InputStreamReader( proc.getInputStream() ) );
                    try
                    {
                        String line = "";
                        while ( ( line = br.readLine() ) != null )
                        {
                            System.out.println( i + " out..:" + line );
                        }
                    }
                    finally
                    {
                        br.close();
                    }
                }
                catch ( InterruptedException e )
                {
                    e.printStackTrace();
                }
                catch (IOException e)
                {
                    e.printStackTrace();
                }
                break;

            case 6: 
                try
                {
                    System.out.println( i + Main.getRes().getString( " start a small command by Runtime.exec and put a wrapperexec in between.." ) );
                    Process p = Runtime.getRuntime().exec( "../test/simplewaiter " + ( rand.nextInt( 200 ) + 1 ) + " " + ( rand.nextInt( 20 ) + 1 ));
                    
                    WrapperProcess proc = WrapperManager.exec( "../test/simplewaiter 4 4" );
                    proc.getOutputStream().close();
                    System.out.println( i + Main.getRes().getString( " small child {0} is alive {1}" , new Object[]{ new Integer( proc.getPID() ), new Boolean( proc.isAlive() ) } ) );
                    // System.out.println(i + " Main.getRes (PID= " + proc.getPID() + " ) finished with code " + proc.waitFor() );
                    BufferedReader br = new BufferedReader( new InputStreamReader( proc.getInputStream() ) );
                    try
                    {
                        String line = "";
                        while ((line = br.readLine()) != null )
                        {
                            System.out.println( i + " out..:" + line );
                        }
                    }
                    finally
                    {
                        br.close();
                    }

                    System.out.println( i + " " + p.toString() + Main.getRes().getString( " Runtime.exec exit " ) + p.waitFor() );
                }
                catch ( IOException e )
                {
                    e.printStackTrace();
                }
                catch ( InterruptedException e )
                {
                    e.printStackTrace();
                }
                break;

            case 7: 
                System.out.println( i + Main.getRes().getString(  " start invalid child..." ) );
                try
                {
                    WrapperProcess proc = WrapperManager.exec( "invalid" );
                    System.out.println( i + Main.getRes().getString( " invalid child is alive "  )+ proc.isAlive() );
                }
                catch ( IOException e )
                {
                    System.out.println( i + Main.getRes().getString( " caught an invalid child..." ) );
                }
                break;
            }
        }
        try {
            System.out.println( Main.getRes().getString( "finally start a long-running application attached to the wrapper, the wrapper will shut down soon, so the app should get killed by the wrapper..." ) );
            WrapperProcess p = WrapperManager.exec( "../test/simplewaiter 2 1000" , new WrapperProcessConfig().setDetached(true));
        } catch (SecurityException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (NullPointerException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (WrapperJNIError e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (WrapperLicenseError e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        System.out.println( Main.getRes().getString( "All Done." ) );
    }
}
