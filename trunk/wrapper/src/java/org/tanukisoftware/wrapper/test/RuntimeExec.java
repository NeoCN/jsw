package org.tanukisoftware.wrapper.test;

/*
 * Copyright (c) 1999, 2010 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.org/doc/english/licenseOverview.html
 */

import org.tanukisoftware.wrapper.WrapperJNIError;
import org.tanukisoftware.wrapper.WrapperLicenseError;
import org.tanukisoftware.wrapper.WrapperManager;
import org.tanukisoftware.wrapper.WrapperProcess;
import org.tanukisoftware.wrapper.WrapperProcessConfig;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;

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
        System.out.println("Is DYNAMIC supported? A:" + WrapperProcessConfig.isSupported(WrapperProcessConfig.DYNAMIC));
        System.out.println("Is FORK_EXEC supported? A:" + WrapperProcessConfig.isSupported(WrapperProcessConfig.FORK_EXEC));
        System.out.println("Is VFORK_EXEC supported? A:" + WrapperProcessConfig.isSupported(WrapperProcessConfig.VFORK_EXEC));
        System.out.println("Is POSIX_SPAWN supported? A:" + WrapperProcessConfig.isSupported(WrapperProcessConfig.POSIX_SPAWN));

        for ( int i = 0 ; i < 10; i++ )
        {
            int num =  rand.nextInt( 7 );
            switch ( num )
            {
            case 0: 
                System.out.println( i + " start invalid child..." );

                try
                {
                    WrapperProcess proc = WrapperManager.exec( "invalid" );
                    System.out.println( i + " invalid childz is alive " + proc.isAlive() );
                }
                catch ( IllegalThreadStateException e )
                {
                    System.out.println( i + " caught invalid child..." );
                    e.printStackTrace();
                }
                catch ( IOException e )
                {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                break;
                
            case 1: 
                System.out.println( i + " start a small child, dont care about output but call waitfor..." );
                try
                {
                    WrapperProcess proc = WrapperManager.exec( "../test/simplewaiter 65 0" );
                    proc.getOutputStream().close();
                    System.out.println( i + " small child " + proc.getPID() + " is alive " + proc.isAlive() );
                    System.out.println( i + " process (PID= "+ proc.getPID() + " ) finished with code " + proc.waitFor() );

                }
                catch ( IllegalThreadStateException e )
                {
                    System.out.println( i + " caught invalid child in small child..." );
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
                System.out.println( i + " start longrunning child dont wait..." );

                try
                {
                    WrapperProcess proc = WrapperManager.exec( "../test/simplewaiter " + ( rand.nextInt( 200 ) + 1 ) + " " + rand.nextInt( 30 ), new WrapperProcessConfig().setStartType(WrapperProcessConfig.VFORK_EXEC) );
                    System.out.println( i + " long prc " + proc.getPID() + " is alive " + proc.isAlive() );
                    // System.out.println( i + " process ( PID= " + proc.getPID() + " ) finished with code " + proc.waitFor() );
                }
                catch ( IllegalThreadStateException e )
                {
                    System.out.println( i + " caught invalid child in long child..." );
                    e.printStackTrace();
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
                    System.out.println( i + " spawn a small command..." );
                    WrapperProcess p = WrapperManager.exec( "../test/simplewaiter 0 15", new WrapperProcessConfig().setStartType(WrapperProcessConfig.POSIX_SPAWN) );
                    // System.out.println(i + " " + p.toString() + " exit " + p.waitFor());
                    BufferedReader br = new BufferedReader( new InputStreamReader( p.getInputStream() ) );
                    try
                    {
                        String line = "";
                        while ( ( line = br.readLine() ) != null )
                        {
                            System.out.println( "out..:" +  line );
                        }
                    }
                    finally
                    {
                        br.close();
                    }
                }
                catch ( IOException e )
                {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                break;
                
            case 4: 
                System.out.println( i + " start a small child and read output..." );
                try
                {
                    WrapperProcessConfig wpm = new WrapperProcessConfig();
                    java.util.Map environment = wpm.getEnvironment();
                    environment.clear();
                    environment.put("TEST", "TEST123");
                   System.out.println("size of map " + environment.size());
                   //wpm.setEnvironment(null);
                    WrapperProcess proc = WrapperManager.exec( "../test/simplewaiter "+ rand.nextInt(200) +" 3", wpm );

                    proc.getOutputStream().close();
                //    System.out.println( i + " small child " + proc.getPID() + " is alive " + proc.isAlive() );
                    // System.out.println( i + " process (PID= " + proc.getPID() + " ) finished with code " + proc.waitFor() );

                    BufferedReader br = new BufferedReader( new InputStreamReader( proc.getInputStream() ) );
                    try
                    {
                        String line = "";
                        while ( ( line = br.readLine() ) != null )
                        {
                            System.out.println( "out..:" +  line );
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
                catch ( IllegalThreadStateException e )
                {
                    System.out.println( i + " caught invalid child in small child..." );
                }
                catch ( IOException e )
                {
                   
                    e.printStackTrace();
                }
                break;
            case 5:
                System.out.println( i + " start longrunning child read and wait..." );
                try
                {
                    WrapperProcess proc = WrapperManager.exec( "../test/simplewaiter " + ( rand.nextInt( 200 ) + 1 ) + " " + rand.nextInt( 30 ) );
                    System.out.println( i + " long prc " + proc.getPID() + " is alive " + proc.isAlive() );
                    BufferedReader br = new BufferedReader( new InputStreamReader( proc.getInputStream() ) );
                    try
                    {
                        String line = "";
                        while ( ( line = br.readLine() ) != null )
                        {
                            System.out.println( line );
                        }
                    }
                    finally
                    {
                        br.close();
                    }
                    System.out.println( i + " long prc " + proc.getPID() + " finished " + proc.waitFor() );
                }
                catch ( IllegalThreadStateException e )
                {
                    System.out.println( i + " caught invalid child in long child..." );
                    e.printStackTrace();
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
                    System.out.println( i + " start a small command by runtime.exec and put a wrapperexec in between.." );
                    Process p = Runtime.getRuntime().exec( "../test/simplewaiter " + ( rand.nextInt( 200 ) + 1 ) + " " + ( rand.nextInt( 20 ) + 1 ));
                    
                    WrapperProcess proc = WrapperManager.exec( "../test/simplewaiter 4 4" );
                    proc.getOutputStream().close();
                    System.out.println( i + " small child " + proc.getPID() + " is alive " + proc.isAlive() );
                    // System.out.println(i + " process (PID= " + proc.getPID() + " ) finished with code " + proc.waitFor() );

                    BufferedReader br = new BufferedReader( new InputStreamReader( proc.getInputStream() ) );
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
                    
                    System.out.println( i + " " + p.toString() + " exit " + p.waitFor() );
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
            }
        }
        try {
            System.out.println("finally start a long-running application attached to the wrapper, the wrapper will shut down soon, so the app should get killed by the wrapper...");
            WrapperProcess p = WrapperManager.exec( "../test/simplewaiter 2 1000" , new WrapperProcessConfig().setDetached(false));
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
        System.out.println( "All Done." );
    }
}

