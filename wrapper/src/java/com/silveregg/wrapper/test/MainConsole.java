package com.silveregg.wrapper.test;

/*
 * Copyright (c) 2001 Silver Egg Technology
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without 
 * restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sublicense, and/or 
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
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// $Log$
// Revision 1.1  2002/05/21 18:39:41  spocke
// Initial commit of console version of the AWT Test Application.
//
// Revision 1.5  2002/05/17 09:52:42  mortenson
// Add a Restart button to the TestWrapper application.
//
// Revision 1.4  2002/05/17 09:08:45  mortenson
// Make the test shutdown correctly when shutdown from a shutdown hook.
//
// Revision 1.3  2002/05/16 03:34:32  mortenson
// Added comments to the buttons that had no comments to make their use clear.
//
// Revision 1.2  2001/12/06 09:36:24  mortenson
// Docs changes, Added sample apps, Fixed some problems with
// relative paths  (See revisions.txt)
//
// Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
// no message
//

import com.silveregg.wrapper.WrapperManager;
import com.silveregg.wrapper.WrapperListener;

class ActionThread implements Runnable {
    public ActionThread(String action) {
        this.action = action;
        this.alive = true;
    }

    public void performAction( ) {
        if (action.equals("exit")) {
            WrapperManager.stop(0);
        } else if (action.equals("accessviolation")) {
            WrapperManager.accessViolation();
        } else if (action.equals("accessviolationnative")) {
            WrapperManager.accessViolationNative();
        } else if (action.equals("appearhung")) {
            WrapperManager.appearHung();
        } else if (action.equals("exit2")) {
            System.exit(0);
        } else if (action.equals("halt")) {
            Runtime.getRuntime().halt(0);
        } else if (action.equals("restart")) {
            WrapperManager.restart();
        }
    }

    public void run() {
        performAction();

        while (this.alive) {
            // Idle some
            try {
                Thread.currentThread().sleep(500);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void endThread( ) {
        this.alive = false;
    }

    private String action;
    private boolean alive;
}

public class MainConsole implements WrapperListener {
    /**************************************************************************
     * Constructors
     *************************************************************************/
    private MainConsole() {
    }

    /**************************************************************************
     * WrapperListener Methods
     *************************************************************************/
    public Integer start(String[] args) {
        Thread actionThread;

        System.out.println("start()");

        if (args.length <= 0)
            printHelp("Missing action parameter.");

        // * * Start the action thread
        this.actionThread = new ActionThread(args[0]);
        actionThread = new Thread(this.actionThread);
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
            actionThread.endThread();
        }
    }

	/**
	 * Prints the usage text.
	 *
	 * @param error_msg Error message to write with usage text
	 */
	private static void printHelp( String error_msg ) {
		System.err.println( "USAGE" );
		System.err.println( "" );
		System.err.println( "MainConsole <action>>" );
		System.err.println( "" );
		System.err.println( "[ACTIONS]" );
		System.err.println( "   exit                     : Calls WrapperManager.stop(0)" );
		System.err.println( "   access_violation         : Calls WrapperManager.accessViolation" );
		System.err.println( "   access_violation_native  : Calls WrapperManager.accessViolationNative()" );
		System.err.println( "   appearhung               : Calls WrapperManager.appearHung()" );
		System.err.println( "   exit2                    : Calls System.exit(0)" );
		System.err.println( "   halt                     : Calls  Runtime.getRuntime().halt(0)" );
		System.err.println( "   restart                  : Calls WrapperManager.restart()" );
		System.err.println( "" );
		System.err.println( "[EXAMPLE]" );
		System.err.println( "   MainConsole access_violation_native" );
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
        WrapperManager.start(new MainConsole(), args);
    }

    private ActionThread actionThread;
}

