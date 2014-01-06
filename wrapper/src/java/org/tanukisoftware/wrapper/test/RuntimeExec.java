package org.tanukisoftware.wrapper.test;

/*
 * Copyright (c) 1999, 2014 Tanuki Software, Ltd.
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
public class RuntimeExec
{
    private static String c_encoding;
    
    /*---------------------------------------------------------------
     * Static Methods
     *-------------------------------------------------------------*/
    static
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
    
    private static void handleJavaProcessInner( Process process )
        throws IOException, InterruptedException
    {
        try
        {
            handleInputStream( process.getInputStream(), c_encoding, "stdout" );
            handleInputStream( process.getErrorStream(), c_encoding, "stderr" );
        }
        finally
        {
            int exitCode = process.waitFor();
            System.out.println( "exitCode: " + exitCode );
        }
    }
    
    private static void handleJavaProcess( String command )
        throws IOException, InterruptedException
    {
        handleJavaProcessInner( Runtime.getRuntime().exec( command ) );
    }
    
    private static void handleJavaProcess( String[] command )
        throws IOException, InterruptedException
    {
        handleJavaProcessInner( Runtime.getRuntime().exec( command ) );
    }

    private static void handleWrapperProcessInner( WrapperProcess process, long timeoutMS )
        throws IOException, InterruptedException
    {
        try
        {
            handleInputStream( process.getInputStream(), c_encoding, "stdout" );
            handleInputStream( process.getErrorStream(), c_encoding, "stderr" );
            
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
                    System.out.println( "Timed out waiting for child.  Destroying." );
                    process.destroy();
                }
            }
        }
        finally
        {
            int exitCode = process.waitFor();
            System.out.println( "exitCode: " + exitCode );
        }
    }

    private static void handleWrapperProcess( String command, long timeoutMS )
        throws IOException, InterruptedException
    {
        handleWrapperProcessInner( WrapperManager.exec( command ), timeoutMS );
    }

    private static void handleWrapperProcess( String[] command, long timeoutMS )
        throws IOException, InterruptedException
    {
        handleWrapperProcessInner( WrapperManager.exec( command ), timeoutMS );
    }
    
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    public static void main( String[] args )
    {
        final String simplewaiter;
        if ( WrapperManager.isWindows() )
        {
            simplewaiter = "../test/simplewaiter.exe";
        }
        else
        {
            simplewaiter = "../test/simplewaiter";
        }
        
        System.out.println( "Communicate with child processes using encoding: " + c_encoding );
        
        Random rand = new Random();
        System.out.println( Main.getRes().getString( "Is DYNAMIC supported? A:" ) + WrapperProcessConfig.isSupported( WrapperProcessConfig.DYNAMIC ) );
        System.out.println( Main.getRes().getString( "Is FORK_EXEC supported? A:" ) + WrapperProcessConfig.isSupported( WrapperProcessConfig.FORK_EXEC ) );
        System.out.println( Main.getRes().getString( "Is VFORK_EXEC supported? A:" ) + WrapperProcessConfig.isSupported( WrapperProcessConfig.VFORK_EXEC ) );
        System.out.println( Main.getRes().getString( "Is POSIX_SPAWN supported? A:" ) + WrapperProcessConfig.isSupported( WrapperProcessConfig.POSIX_SPAWN ) );

        try
        {
            // Simple Test (String)
            System.out.println();
            System.out.println( "Verifying correct parsing of the command:" );
            System.out.println( "First a single command line: " + simplewaiter  + " -v \"test 123\" test 123 \"\\\"test\\\"\"" );
            
            String s = simplewaiter + " -v \"test 123\" test 123 \"\\\"test\\\"";
            
            System.out.println( "Runtime.exec:" );
            handleJavaProcess( s );
            
            System.out.println( "Now WrapperManager.exec:" );
            handleWrapperProcess( s, 0 );
            
            System.out.println( "First test finished. " );
            
            
            // Simple Test (String[])
            System.out.println();
            System.out.println( "Next a pass the command as array: " + simplewaiter + " -v \"test 123\" test 123 \"\\\"test\\\"\"" );
            String s2[] = { simplewaiter, "-v", "\"test 123\"", "test 123", "\"\\\"test\\\"\"" };
            
            System.out.println( "Runtime.exec:" );
            handleJavaProcess( s2 );

            System.out.println( "Now WrapperManager.exec:" );
            handleWrapperProcess( s2, 0 );
            
            System.out.println( "Second test finished. " );
        }
        catch ( Exception e )
        {
            e.printStackTrace();
        }

        int i = 0;
        System.out.println();
        System.out.println( i + Main.getRes().getString( " start a small child process, dont care about output but call waitfor..." ) );
        try
        {
            WrapperProcess proc = WrapperManager.exec( simplewaiter + " 65 1" );
            proc.getOutputStream().close();
            System.out.println( Main.getRes().getString( "{0} small child process {1} is alive {2}",  new Object[] { Integer.toString( i ), Integer.toString( proc.getPID() ) , Boolean.toString( proc.isAlive() ) } ) );
            System.out.println( Main.getRes().getString( "{0} child process (PID= {1}) finished with code {2}", new Object[] { Integer.toString( i ), Integer.toString ( proc.getPID() ), Integer.toString( proc.waitFor() ) } ) );
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
        
        i++;
        System.out.println();
        System.out.println( i + Main.getRes().getString( " start longrunning child process dont wait..." ) );
        try
        {
            WrapperProcess proc;
            System.out.println( i + Main.getRes().getString( " first, try to vfork..." ) ); 
            if ( WrapperProcessConfig.isSupported( WrapperProcessConfig.VFORK_EXEC ) ) 
            {
                System.out.println( i + Main.getRes().getString( " vfork is supported" ) ); 
                proc = WrapperManager.exec( simplewaiter + " " + ( rand.nextInt( 200 ) + 1 ) + " " + rand.nextInt( 30 ), new WrapperProcessConfig().setStartType(WrapperProcessConfig.VFORK_EXEC) );                    	
            }
            else
            {
                System.out.println( i + Main.getRes().getString( " vfork is not supported" ) ); 
                proc = WrapperManager.exec( simplewaiter + " " + ( rand.nextInt( 200 ) + 1 ) + " " + rand.nextInt( 30 ) );
            }
            
            System.out.println( i + Main.getRes().getString( " longrunning child process {0} is alive {1}" , new Object[]{ Integer.toString( proc.getPID() ), Boolean.toString( proc.isAlive() ) } ) );
            // System.out.println( i + " process ( PID= " + proc.getPID() + " ) finished with code " + proc.waitFor() );
        }
        catch ( IOException e )
        {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        
        i++;
        System.out.println();
        System.out.println( i + Main.getRes().getString( " spawn a small child process..." ) );
        try
        {
            WrapperProcess p;
            if ( WrapperProcessConfig.isSupported( WrapperProcessConfig.POSIX_SPAWN ) )
            {
                System.out.println( i + Main.getRes().getString( " posix_spawn is supported." ) );
                p = WrapperManager.exec( simplewaiter + " 0 15", new WrapperProcessConfig().setStartType( WrapperProcessConfig.POSIX_SPAWN) );
            }
            else
            {
                System.out.println( i + Main.getRes().getString( " spawn is not supported." ) );
                p = WrapperManager.exec( simplewaiter + " 0 15" );
            }
            // System.out.println(i + " " + p.toString() + " exit " + p.waitFor());
            BufferedReader br = new BufferedReader( new InputStreamReader( p.getInputStream(), c_encoding ) );
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
        
        i++;
        System.out.println();
        System.out.println( i + Main.getRes().getString( " start a small child process, change the environment and read output..." ) );
        try
        {
            WrapperProcessConfig wpm = new WrapperProcessConfig();
            java.util.Map environment = wpm.getEnvironment();
            System.out.println( i + Main.getRes().getString( " size of Environment map (before calling clear()) = " ) + environment.size() );
            environment.clear();
            environment.put( "TEST", "TEST123" );
            System.out.println( i + Main.getRes().getString( " size of Environment map = " ) + environment.size() );
            WrapperProcess proc = WrapperManager.exec( simplewaiter + " "+ rand.nextInt(200) +" 3", wpm );
            proc.getOutputStream().close();

            System.out.println( i + Main.getRes().getString( " small child process {0} is alive {1}" , new Object[]{ Integer.toString( proc.getPID() ), Boolean.toString( proc.isAlive() ) } ) );
            BufferedReader br = new BufferedReader( new InputStreamReader( proc.getInputStream(), c_encoding ) );
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

            br = new BufferedReader( new InputStreamReader( proc.getErrorStream(), c_encoding ) );
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
        
        i++;
        System.out.println();
        System.out.println( i + Main.getRes().getString( " start longrunning child process, change working dir, call waitFor and finally read output..." ) );
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
            System.out.println( i + Main.getRes().getString( " child process (PID= {0}) finished with code " , Integer.toString( proc.getPID() ) ) +  proc.waitFor() );

            System.out.println( i + Main.getRes().getString( " now read the output" ) );
            BufferedReader br = new BufferedReader( new InputStreamReader( proc.getInputStream(), c_encoding ) );
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
        
        i++;
        System.out.println();
        System.out.println( i + Main.getRes().getString( " start a small child process by Runtime.exec and put a wrapperexec in between.." ) );
        try
        {
            Process p = Runtime.getRuntime().exec( simplewaiter + " " + ( rand.nextInt( 200 ) + 1 ) + " " + ( rand.nextInt( 20 ) + 1 ));
            
            WrapperProcess proc = WrapperManager.exec( simplewaiter + " 4 4" );
            proc.getOutputStream().close();
            System.out.println( i + Main.getRes().getString( " small child process {0} is alive {1}" , new Object[]{ Integer.toString( proc.getPID() ), Boolean.toString( proc.isAlive() ) } ) );
            // System.out.println(i + " Main.getRes (PID= " + proc.getPID() + " ) finished with code " + proc.waitFor() );
            BufferedReader br = new BufferedReader( new InputStreamReader( proc.getInputStream(), c_encoding ) );
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
        
        i++;
        System.out.println();
        System.out.println( i + Main.getRes().getString(  " start invalid child process..." ) );
        try
        {
            WrapperProcess proc = WrapperManager.exec( "invalid" );
            System.out.println( i + Main.getRes().getString( " invalid child process is alive "  )+ proc.isAlive() );
        }
        catch ( IOException e )
        {
            System.out.println( i + Main.getRes().getString( " caught an invalid child process..." ) );
        }
        
        i++;
        System.out.println();
        System.out.println( i + Main.getRes().getString(  " slow child process..." ) );
        try
        {
            String command = simplewaiter + " 0 5";
            handleWrapperProcess( command, 10000 );
        }
        catch ( InterruptedException e )
        {
            e.printStackTrace();
        }
        catch ( IOException e )
        {
            e.printStackTrace();
        }
        
        i++;
        System.out.println();
        System.out.println( i + Main.getRes().getString(  " abort child process..." ) );
        try
        {
            String command = simplewaiter + " 0 30";
            handleWrapperProcess( command, 10000 );
        }
        catch ( InterruptedException e )
        {
            e.printStackTrace();
        }
        catch ( IOException e )
        {
            e.printStackTrace();
        }
        
        System.out.println();
        System.out.println( Main.getRes().getString( "finally start a long-running child process attached to the wrapper, the wrapper will shut down soon, so the child process should get killed by the wrapper..." ) );
        try {
            WrapperProcess p = WrapperManager.exec( simplewaiter + " 2 1000" , new WrapperProcessConfig().setDetached(false));
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
        
        System.out.println();
        if ( WrapperManager.getJVMId() == 1 )
        {
            // First invocation.
            System.out.println( Main.getRes().getString( "All Done. Restarting..." ) );
            WrapperManager.restart();
        }
        else
        {
            // Second invocation.
            //  Register a long shutdownhook which will cause the Wrapper to timeout and kill the JVM.
            System.out.println( Main.getRes().getString( "All Done. Registering long shutdown hook and stopping.\nWrapper should timeout and kill the JVM, cleaning up all processes in the process." ) );
            
            Runtime.getRuntime().addShutdownHook( new Thread()
            {
                public void run() {
                    System.out.println( Main.getRes().getString( "Starting shutdown hook. Loop for 25 seconds.") );
                    System.out.println( Main.getRes().getString( "Should timeout unless this property is set: wrapper.jvm_exit.timeout=30" ) );
    
                    long start = System.currentTimeMillis();
                    boolean failed = false;
                    while ( System.currentTimeMillis() - start < 25000 )
                    {
                        if ( !failed )
                        {
                            try
                            {
                                WrapperProcess proc = WrapperManager.exec( simplewaiter + " 0 25" );
                                System.out.println( Main.getRes().getString( "Launched child...") );
                            }
                            catch ( WrapperJNIError e )
                            {
                                System.out.println( Main.getRes().getString( "Unable to launch child process because JNI library unavailable. Normal on shutdown.") );
                                failed = true;
                            }
                            catch ( IOException e )
                            {
                                System.out.println( Main.getRes().getString( "Unexpected problem launching child process: {0}", e.toString() ) );
                                failed = true;
                            }
                        }
                        
                        try
                        {
                            Thread.sleep( 250 );
                        }
                        catch ( InterruptedException e )
                        {
                            // Ignore
                        }
                    }
                    System.out.println( Main.getRes().getString( "Shutdown hook complete. Should exit now." ) );
                }
            } );
            
            System.exit( 0 );
        }
    }
}
