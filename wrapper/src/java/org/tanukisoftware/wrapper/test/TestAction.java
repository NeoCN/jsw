package org.tanukisoftware.wrapper.test;

/*
 * Copyright (c) 1999, 2003 TanukiSoftware.org
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without 
 * restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sub-license , and/or 
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following 
 * conditions:
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
// Revision 1.4  2003/10/18 07:51:10  mortenson
// The DeadlockPrintStream should not be set until after the WrapperManager class
// has been initialized.
//
// Revision 1.3  2003/10/18 07:35:30  mortenson
// Add test cases to test how the wrapper handles it when the System.out stream
// becomes deadlocked.  This can happen if buggy usercode overrides those streams.
//
// Revision 1.2  2003/04/03 04:05:22  mortenson
// Fix several typos in the docs.  Thanks to Mike Castle.
//
// Revision 1.1  2003/02/03 06:55:29  mortenson
// License transfer to TanukiSoftware.org
//

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import org.tanukisoftware.wrapper.WrapperManager;
import org.tanukisoftware.wrapper.WrapperListener;

/**
 *
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 * @version $Revision$
 */
public class TestAction implements WrapperListener {
    private DeadlockPrintStream m_out;
    private DeadlockPrintStream m_err;

    private ActionRunner m_actionRunner;
    
    /**************************************************************************
     * Constructors
     *************************************************************************/
    private TestAction() {
    }

    /**************************************************************************
     * WrapperListener Methods
     *************************************************************************/
    public Integer start(String[] args) {
        Thread actionThread;

        System.out.println("start()");

        m_out = new DeadlockPrintStream( System.out );
        System.setOut( m_out );
        m_err = new DeadlockPrintStream( System.err );
        System.setErr( m_err );
        
        if (args.length <= 0)
            printHelp("Missing action parameter.");

        // * * Start the action thread
        m_actionRunner = new ActionRunner(args[0]);
        actionThread = new Thread(m_actionRunner);
        actionThread.start();

        return null;
    }
    
    public int stop(int exitCode) {
        System.out.println("stop(" + exitCode + ")");
        
        return exitCode;
    }
    
    public void controlEvent(int event) {
        System.out.println("controlEvent(" + event + ")");
        if (event == WrapperManager.WRAPPER_CTRL_C_EVENT) {
            //WrapperManager.stop(0);
            m_actionRunner.endThread();
        }
    }

    /**************************************************************************
     * Inner Classes
     *************************************************************************/
    private class ActionRunner implements Runnable {
        private String m_action;
        private boolean m_alive;
        
        public ActionRunner(String action) {
            m_action = action;
            m_alive = true;
        }
    
        public void performAction( ) {
            if (m_action.equals("stop")) {
                WrapperManager.stop(0);
            } else if (m_action.equals("access_violation")) {
                WrapperManager.accessViolation();
            } else if (m_action.equals("access_violation_native")) {
                WrapperManager.accessViolationNative();
            } else if (m_action.equals("appear_hung")) {
                WrapperManager.appearHung();
            } else if (m_action.equals("exit")) {
                System.exit(0);
            } else if (m_action.equals("halt")) {
                // Execute runtime.halt(0) using reflection so this class will
                //  compile on 1.2.x versions of Java.
                Method haltMethod;
                try {
                    haltMethod = Runtime.class.getMethod("halt", new Class[] {Integer.TYPE});
                } catch (NoSuchMethodException e) {
                    System.out.println("halt not supported by current JVM.");
                    haltMethod = null;
                }
                
                if (haltMethod != null) {
                    Runtime runtime = Runtime.getRuntime();
                    try {
                        haltMethod.invoke(runtime, new Object[] {new Integer(0)});
                    } catch (IllegalAccessException e) {
                        System.out.println("Unable to call runitme.halt: " + e.getMessage());
                    } catch (InvocationTargetException e) {
                        System.out.println("Unable to call runitme.halt: " + e.getMessage());
                    }
                }
            } else if (m_action.equals("restart")) {
                WrapperManager.restart();
            } else if (m_action.equals("dump")) {
                WrapperManager.requestThreadDump();
            } else if (m_action.equals("deadlock_out")) {
                System.out.println("Deadlocking System.out and System.err ...");
                m_out.setDeadlock(true);
                m_err.setDeadlock(true);
            } else {
                printHelp("\"" + m_action + "\" is an unknown action.");
                WrapperManager.stop(0);
            }
        }
    
        public void run() {
            // Wait for 5 seconds so that the startup will complete.
            try {
                Thread.sleep(5000);
            } catch (InterruptedException e) {}
            
            performAction();
    
            while (m_alive) {
                // Idle some
                try {
                    Thread.currentThread().sleep(500);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    
        public void endThread( ) {
            m_alive = false;
        }
    }
    
    /**
     * Prints the usage text.
     *
     * @param error_msg Error message to write with usage text
     */
    private static void printHelp(String error_msg) {
        System.err.println( "USAGE" );
        System.err.println( "" );
        System.err.println( "TestAction <action>" );
        System.err.println( "" );
        System.err.println( "[ACTIONS]" );
        System.err.println( "  Actions which should cause the Wrapper to exit cleanly:" );
        System.err.println( "   stop                     : Calls WrapperManager.stop(0)" );
        System.err.println( "   exit                     : Calls System.exit(0)" );
        System.err.println( "  Actions which should cause the Wrapper to restart the JVM:" );
        System.err.println( "   access_violation         : Calls WrapperManager.accessViolation" );
        System.err.println( "   access_violation_native  : Calls WrapperManager.accessViolationNative()" );
        System.err.println( "   appear_hung              : Calls WrapperManager.appearHung()" );
        System.err.println( "   halt                     : Calls Runtime.getRuntime().halt(0)" );
        System.err.println( "   restart                  : Calls WrapperManager.restart()" );
        System.err.println( "   dump                     : Calls WrapperManager.requestThreadDump()" );
        System.err.println( "   deadlock_out             : Deadlocks the JVM's System.out and err streams." );
        System.err.println( "" );
        System.err.println( "[EXAMPLE]" );
        System.err.println( "   TestAction access_violation_native " );
        System.err.println( "" );
        System.err.println( "ERROR: " + error_msg );
        System.err.println( "" );

        System.exit( -1 );
    }

    /**************************************************************************
     * Main Method
     *************************************************************************/
    public static void main(String[] args) {
        System.out.println("Initializing...");
        
        // Start the application.  If the JVM was launched from the native
        //  Wrapper then the application will wait for the native Wrapper to
        //  call the application's start method.  Otherwise the start method
        //  will be called immediately.
        WrapperManager.start(new TestAction(), args);
    }
}

