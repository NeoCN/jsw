package org.tanukisoftware.wrapper.test;

/*
 * Copyright (c) 1999, 2005 Tanuki Software Inc.
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
// Revision 1.15  2005/10/13 05:52:16  mortenson
// Implement the ability to catch control events using the WrapperEventLisener.
//
// Revision 1.14  2005/08/24 06:53:39  mortenson
// Add stopAndReturn and restartAndReturn methods.
//
// Revision 1.13  2005/05/23 02:39:30  mortenson
// Update the copyright information.
//
// Revision 1.12  2004/12/08 04:54:27  mortenson
// Make it possible to access the contents of the Wrapper configuration file from
// within the JVM.
//
// Revision 1.11  2004/11/26 08:41:22  mortenson
// Implement reading from System.in
//
// Revision 1.10  2004/11/22 09:35:45  mortenson
// Add methods for controlling other services.
//
// Revision 1.9  2004/11/22 04:06:42  mortenson
// Add an event model to make it possible to communicate with user applications in
// a more flexible way.
//
// Revision 1.8  2004/10/18 09:35:46  mortenson
// Remove the gc call in the main loop.  It was causing a noticeable hickup in the
// CPU usage every 5 seconds.
//
// Revision 1.7  2004/08/06 08:05:26  mortenson
// Add test case which dumps the system properties.  Useful for testing.
//
// Revision 1.6  2004/08/06 07:56:20  mortenson
// Add test case which runs idle.  Useful to test some operations.
//
// Revision 1.5  2004/04/15 06:42:11  mortenson
// Fix a typo in the access_violation_native action.
//
// Revision 1.4  2004/03/27 14:39:20  mortenson
// Add actions for the stopImmediate method.
//
// Revision 1.3  2004/01/16 04:41:55  mortenson
// The license was revised for this version to include a copyright omission.
// This change is to be retroactively applied to all versions of the Java
// Service Wrapper starting with version 3.0.0.
//
// Revision 1.2  2004/01/15 09:50:30  mortenson
// Fix some problems where the Wrapper was not handling exit codes correctly.
//
// Revision 1.1  2004/01/10 15:44:15  mortenson
// Rework the test wrapper app so there is less code duplication.
//

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Enumeration;
import java.util.Properties;

import org.tanukisoftware.wrapper.WrapperManager;
import org.tanukisoftware.wrapper.WrapperServiceException;
import org.tanukisoftware.wrapper.WrapperWin32Service;
import org.tanukisoftware.wrapper.event.WrapperControlEvent;
import org.tanukisoftware.wrapper.event.WrapperEvent;
import org.tanukisoftware.wrapper.event.WrapperEventListener;

/**
 * @author Leif Mortenson <leif@tanukisoftware.com>
 * @version $Revision$
 */
public abstract class AbstractActionApp
    implements WrapperEventListener
{
    private DeadlockPrintStream m_out;
    private DeadlockPrintStream m_err;
    
    private Thread m_runner;
    private Thread m_consoleRunner;
    
    private boolean m_users;
    private boolean m_groups;
    
    private long m_eventMask = 0xffffffffffffffffL;
    private String m_serviceName = "testWrapper";
    
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    protected AbstractActionApp() {
        m_runner = new Thread( "WrapperActionTest_Runner" )
        {
            public void run()
            {
                while ( true )
                {
                    if ( m_users )
                    {
                        System.out.println( "The current user is: "
                            + WrapperManager.getUser( m_groups ) );
                        System.out.println( "The current interactive user is: "
                            + WrapperManager.getInteractiveUser( m_groups ) );
                    }
                    synchronized( AbstractActionApp.class )
                    {
                        try
                        {
                            AbstractActionApp.class.wait( 5000 );
                        }
                        catch ( InterruptedException e )
                        {
                        }
                    }
                }
            }
        };
        m_runner.setDaemon( true );
        m_runner.start();
    }
    
    /*---------------------------------------------------------------
     * WrapperEventListener Methods
     *-------------------------------------------------------------*/
    /**
     * Called whenever a WrapperEvent is fired.  The exact set of events that a
     *  listener will receive will depend on the mask supplied when
     *  WrapperManager.addWrapperEventListener was called to register the
     *  listener.
     *
     * Listener implementations should never assume that they will only receive
     *  events of a particular type.   To assure that events added to future
     *  versions of the Wrapper do not cause problems with user code, events
     *  should always be tested with "if ( event instanceof {EventClass} )"
     *  before casting it to a specific event type.
     *
     * @param event WrapperEvent which was fired.
     */
    public void fired( WrapperEvent event )
    {
        System.out.println( "Received event: " + event );
        if ( event instanceof WrapperControlEvent )
        {
            System.out.println( "  Consume and ignore." );
            ((WrapperControlEvent)event).consume();
        }
    }
    
    /*---------------------------------------------------------------
     * Methods
     *-------------------------------------------------------------*/
    protected void setEventMask( long eventMask )
    {
        m_eventMask = eventMask;
    }
    
    protected void setServiceName( String serviceName )
    {
        m_serviceName = serviceName;
    }
    
    protected void prepareSystemOutErr()
    {
        m_out = new DeadlockPrintStream( System.out );
        System.setOut( m_out );
        m_err = new DeadlockPrintStream( System.err );
        System.setErr( m_err );
    }
    protected boolean doAction( String action )
    {
        if ( action.equals( "stop0" ) )
        {
            WrapperManager.stop( 0 );
            
        }
        else if ( action.equals( "stop1" ) )
        {
            WrapperManager.stop( 1 );
            
        }
        else if ( action.equals( "exit0" ) )
        {
            System.exit( 0 );
            
        }
        else if ( action.equals( "exit1" ) )
        {
            System.exit( 1 );
            
        }
        else if ( action.equals( "stopimmediate0" ) )
        {
            WrapperManager.stopImmediate( 0 );
        }
        else if ( action.equals( "stopimmediate1" ) )
        {
            WrapperManager.stopImmediate( 1 );
        }
        else if ( action.equals( "stopandreturn0" ) )
        {
            WrapperManager.stopAndReturn( 0 );
        }
        else if ( action.equals( "halt" ) )
        {
            // Execute runtime.halt(0) using reflection so this class will
            //  compile on 1.2.x versions of Java.
            Method haltMethod;
            try
            {
                haltMethod = Runtime.class.getMethod( "halt", new Class[] { Integer.TYPE } );
            }
            catch ( NoSuchMethodException e )
            {
                System.out.println( "halt not supported by current JVM." );
                haltMethod = null;
            }
            
            if ( haltMethod != null )
            {
                Runtime runtime = Runtime.getRuntime();
                try
                {
                    haltMethod.invoke( runtime, new Object[] { new Integer( 0 ) } );
                }
                catch ( IllegalAccessException e )
                {
                    System.out.println( "Unable to call runitme.halt: " + e.getMessage() );
                }
                catch ( InvocationTargetException e )
                {
                    System.out.println( "Unable to call runitme.halt: " + e.getMessage() );
                }
            }
        }
        else if ( action.equals( "restart" ) )
        {
            WrapperManager.restart();
            
        }
        else if ( action.equals( "restartandreturn" ) )
        {
            WrapperManager.restartAndReturn();
            
        }
        else if ( action.equals( "access_violation" ) )
        {
            WrapperManager.accessViolation();
            
        }
        else if ( action.equals( "access_violation_native" ) )
        {
            WrapperManager.accessViolationNative();
            
        }
        else if ( action.equals( "appear_hung" ) )
        {
            WrapperManager.appearHung();
            
        }
        else if ( action.equals( "dump" ) )
        {
            WrapperManager.requestThreadDump();
            
        }
        else if ( action.equals( "deadlock_out" ) )
        {
            System.out.println( "Deadlocking System.out and System.err ..." );
            m_out.setDeadlock( true );
            m_err.setDeadlock( true );
            
        }
        else if ( action.equals( "users" ) )
        {
            if ( !m_users )
            {
                System.out.println( "Begin polling the current and interactive users." );
                m_users = true;
            }
            else if ( m_groups )
            {
                System.out.println( "Stop polling for group info." );
                m_groups = false;
            }
            else
            {
                System.out.println( "Stop polling the current and interactive users." );
                m_users = false;
            }
            
            synchronized( AbstractActionApp.class )
            {
                AbstractActionApp.class.notifyAll();
            }
        }
        else if ( action.equals( "groups" ) )
        {
            if ( ( !m_users ) || ( !m_groups ) )
            {
                System.out.println( "Begin polling the current and interactive users with group info." );
                m_users = true;
                m_groups = true;
            }
            else
            {
                System.out.println( "Stop polling for group info." );
                m_groups = false;
            }
            
            synchronized( AbstractActionApp.class )
            {
                AbstractActionApp.class.notifyAll();
            }
        }
        else if ( action.equals( "console" ) )
        {
            if ( m_consoleRunner == null )
            {
                m_consoleRunner = new Thread( "console-runner" )
                {
                    public void run()
                    {
                        System.out.println();
                        System.out.println( "Start prompting for actions." );
                        try
                        {
                            BufferedReader r = new BufferedReader(new InputStreamReader(System.in));
                            String line;
                            try
                            {
                                do {
                                    System.out.println("Input an action (return stops prompting):");
                                    line = r.readLine();
                                    if ((line != null) && (!line.equals(""))) {
                                        System.out.println("Read action: " + line );
                                        if ( !doAction( line ) )
                                        {
                                            System.out.println( "Unknown action: " + line );
                                        }
                                    }
                                } while ((line != null) && (!line.equals("")));
                            }
                            catch ( IOException e )
                            {
                                e.printStackTrace();
                            }
                        }
                        finally
                        {
                            System.out.println( "Stop prompting for actions." );
                            System.out.println();
                            m_consoleRunner = null;
                        }
                    }
                };
                m_consoleRunner.setDaemon( true );
                m_consoleRunner.start();
            }
        }
        else if ( action.equals( "idle" ) )
        {
            System.out.println( "Run idle." );
            m_users = false;
            m_groups = false;
            
            synchronized( AbstractActionApp.class )
            {
                AbstractActionApp.class.notifyAll();
            }
        }
        else if ( action.equals( "properties" ) )
        {
            System.out.println( "Dump System Properties:" );
            Properties props = System.getProperties();
            for ( Enumeration enum = props.propertyNames(); enum.hasMoreElements(); )
            {
                String name = (String)enum.nextElement();
                System.out.println( "  " + name + "=" + props.getProperty( name ) );
            }
            System.out.println();
        }
        else if ( action.equals( "configuration" ) )
        {
            System.out.println( "Dump Wrapper Properties:" );
            Properties props = WrapperManager.getProperties();
            for ( Enumeration enum = props.propertyNames(); enum.hasMoreElements(); )
            {
                String name = (String)enum.nextElement();
                System.out.println( "  " + name + "=" + props.getProperty( name ) );
            }
            System.out.println();
        }
        else if ( action.equals( "listener" ) )
        {
            System.out.println( "Updating Event Listeners:" );
            WrapperManager.removeWrapperEventListener( this );
            WrapperManager.addWrapperEventListener( this, m_eventMask );
        }
        else if ( action.equals( "service_list" ) )
        {
            WrapperWin32Service[] services = WrapperManager.listServices();
            if ( services == null )
            {
                System.out.println( "Services not supported by current platform." );
            }
            else
            {
                System.out.println( "Registered Services:" );
                for ( int i = 0; i < services.length; i++ )
                {
                    System.out.println( "  " + services[i] );
                }
            }
        }
        else if ( action.equals( "service_interrogate" ) )
        {
            try
            {
                WrapperWin32Service service = WrapperManager.sendServiceControlCode(
                    m_serviceName, WrapperManager.SERVICE_CONTROL_CODE_INTERROGATE );
                System.out.println( "Service after interrogate: " + service );
            }
            catch ( WrapperServiceException e )
            {
                e.printStackTrace();
            }
        }
        else if ( action.equals( "service_start" ) )
        {
            try
            {
                WrapperWin32Service service = WrapperManager.sendServiceControlCode(
                    m_serviceName, WrapperManager.SERVICE_CONTROL_CODE_START );
                System.out.println( "Service after start: " + service );
            }
            catch ( WrapperServiceException e )
            {
                e.printStackTrace();
            }
        }
        else if ( action.equals( "service_stop" ) )
        {
            try
            {
                WrapperWin32Service service = WrapperManager.sendServiceControlCode(
                    m_serviceName, WrapperManager.SERVICE_CONTROL_CODE_STOP );
                System.out.println( "Service after stop: " + service );
            }
            catch ( WrapperServiceException e )
            {
                e.printStackTrace();
            }
        }
        else if ( action.equals( "service_user" ) )
        {
            try
            {
                for ( int i = 128; i < 256; i+=10 )
                {
                    WrapperWin32Service service = WrapperManager.sendServiceControlCode(
                        m_serviceName, i );
                    System.out.println( "Service after user code " + i + ": " + service );
                }
            }
            catch ( WrapperServiceException e )
            {
                e.printStackTrace();
            }
        }
        else
        {
            // Unknown action
            return false;
        
        }
        
        return true;
    }
}

