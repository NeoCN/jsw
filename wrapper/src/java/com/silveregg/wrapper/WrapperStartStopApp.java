package com.silveregg.wrapper;

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
// Revision 1.1  2002/09/14 17:06:51  mortenson
// Add new WrapperStartStopApp helper application.
//

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

public class WrapperStartStopApp implements WrapperListener, Runnable {
    /**
     * Application's start main method
     */
    private Method _startMainMethod;
    
    /**
     * Command line arguments to be passed on to the start main method
     */
    private String[] _startMainArgs;
    
    /**
     * Application's stop main method
     */
    private Method _stopMainMethod;
    
    /**
     * Should the stop process force the JVM to exit, or wait for all threads to die on their own.
     */
    private boolean _stopWait;
    
    /**
     * Command line arguments to be passed on to the stop main method
     */
    private String[] _stopMainArgs;
    
    /**
     * Gets set to true when the thread used to launch the application completes.
     */
    private boolean _mainComplete;
    
    /**
     * Exit code to be returned if the application fails to start.
     */
    private Integer _mainExitCode;
    
    /**
     * Flag used to signify that the start method is done waiting for the application to start.
     */
    private boolean _waitTimedOut;
    
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    private WrapperStartStopApp(Method startMainMethod, Method stopMainMethod, boolean stopWait, String[] stopMainArgs) {
        _startMainMethod = startMainMethod;
        _stopMainMethod = stopMainMethod;
        _stopWait = stopWait;
        _stopMainArgs = stopMainArgs;
    }
    
    /*---------------------------------------------------------------
     * Runnable Methods
     *-------------------------------------------------------------*/
    /**
     * Used to launch the application in a separate thread.
     */
    public void run() {
        Throwable t = null;
        try {
            if (WrapperManager.isDebugEnabled()) {
                System.out.println("WrapperStartStopApp: invoking start main method");
            }
            _startMainMethod.invoke(null, new Object[] {_startMainArgs});
            if (WrapperManager.isDebugEnabled()) {
                System.out.println("WrapperStartStopApp: start main method completed");
            }
            
            synchronized(this) {
                // Let the start() method know that the main method returned, in case it is 
                //  still waiting.
                _mainComplete = true;
                this.notifyAll();
            }
            
            return;
        } catch (IllegalAccessException e) {
            t = e;
        } catch (IllegalArgumentException e) {
            t = e;
        } catch (InvocationTargetException e) {
            t = e;
        }
        
        // If we get here, then an error was thrown.  If this happened quickly 
        // enough, the start method should be allowed to shut things down.
        System.out.println("Encountered an error running start main: " + t);

        // We should print a stack trace here, because in the case of an 
        // InvocationTargetException, the user needs to know what exception
        // their app threw.
        t.printStackTrace();

        showUsage();

        synchronized(this) {
            if (_waitTimedOut) {
                // Shut down here.
                WrapperManager.stop(1);
                return; // Will not get here.
            } else {
                // Let start method handle shutdown.
                _mainComplete = true;
                _mainExitCode = new Integer(1);
                this.notifyAll();
                return;
            }
        }
    }
    
    /*---------------------------------------------------------------
     * WrapperListener Methods
     *-------------------------------------------------------------*/
    /**
     * The start method is called when the WrapperManager is signalled by the 
     *	native wrapper code that it can start its application.  This
     *	method call is expected to return, so a new thread should be launched
     *	if necessary.
     * If there are any problems, then an Integer should be returned, set to
     *	the desired exit code.  If the application should continue,
     *	return null.
     */
    public Integer start(String[] args) {
        if (WrapperManager.isDebugEnabled()) {
            System.out.println("WrapperStartStopApp: start(args)");
        }
        
        Thread mainThread = new Thread(this, "WrapperStartStopAppMain");
        synchronized(this) {
            _startMainArgs = args;
            mainThread.start();
            // Wait for two seconds to give the application a chance to have failed.
            try {
                this.wait(2000);
            } catch (InterruptedException e) {
            }
            _waitTimedOut = true;
            
            if (WrapperManager.isDebugEnabled()) {
                System.out.println("WrapperStartStopApp: start(args) end.  Main Completed=" + 
                    _mainComplete + ", exitCode=" + _mainExitCode);
            }
            return _mainExitCode;
        }
    }
    
    /**
     * Called when the application is shutting down.
     */
    public int stop(int exitCode) {
        if (WrapperManager.isDebugEnabled()) {
            System.out.println("WrapperStartStopApp: stop(" + exitCode + ")");
        }
        
        // Execute the main method in the stop class
        Throwable t = null;
        try {
            if (WrapperManager.isDebugEnabled()) {
                System.out.println("WrapperStartStopApp: invoking stop main method");
            }
            _stopMainMethod.invoke(null, new Object[] {_stopMainArgs});
            if (WrapperManager.isDebugEnabled()) {
                System.out.println("WrapperStartStopApp: stop main method completed");
            }
            
            if (_stopWait) {
                int threadCnt;
                while((threadCnt = WrapperManager.getNonDaemonThreadCount()) > 1) {
                    if (WrapperManager.isDebugEnabled()) {
                        System.out.println("WrapperStartStopApp: stopping.  Waiting for " + (threadCnt - 1) + " threads to complete." );
                    }
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {}
                }
            }
            
            // Success
            return exitCode;
        } catch (IllegalAccessException e) {
            t = e;
        } catch (IllegalArgumentException e) {
            t = e;
        } catch (InvocationTargetException e) {
            t = e;
        }
        
        // If we get here, then an error was thrown.
        System.out.println("Encountered an error running stop main: " + t);

        // We should print a stack trace here, because in the case of an 
        // InvocationTargetException, the user needs to know what exception
        // their app threw.
        t.printStackTrace();

        showUsage();
        
        // Return a failure exit code
        return 1;
    }
    
    /**
     * Called whenever the native wrapper code traps a system control signal
     *  against the Java process.  It is up to the callback to take any actions
     *  necessary.  Possible values are: WrapperManager.WRAPPER_CTRL_C_EVENT, 
     *    WRAPPER_CTRL_CLOSE_EVENT, WRAPPER_CTRL_LOGOFF_EVENT, or 
     *    WRAPPER_CTRL_SHUTDOWN_EVENT
     */
    public void controlEvent(int event) {
        if (WrapperManager.isControlledByNativeWrapper()) {
            if (WrapperManager.isDebugEnabled()) {
                System.out.println("WrapperStartStopApp: controlEvent(" + event + ") Ignored");
            }
            // Ignore the event as the native wrapper will handle it.
        } else {
            if (WrapperManager.isDebugEnabled()) {
                System.out.println("WrapperStartStopApp: controlEvent(" + event + ") Stopping");
            }
            
            // Not being run under a wrapper, so this isn't an NT service and should always exit.
            //  Handle the event here.
            WrapperManager.stop(0);
            // Will not get here.
        }
    }
    
    /*---------------------------------------------------------------
     * Methods
     *-------------------------------------------------------------*/
    /**
     * Returns the main method of the specified class.  If there are any problems,
     *  an error message will be displayed and the Wrapper will be stopped.  This
     *  method will only return if it has a valid method.
     */
    private static Method getMainMethod( String className ) {
        // Look for the start class by name
        Class mainClass;
        try {
            mainClass = Class.forName(className);
        } catch (ClassNotFoundException e) {
            System.out.println("WrapperStartStopApp: Unable to locate the class " + className + ": " + e);
            showUsage();
            WrapperManager.stop(1);
            return null;  // Will not get here
        } catch (LinkageError e) {
            System.out.println("WrapperStartStopApp: Unable to locate the class " + className + ": " + e);
            showUsage();
            WrapperManager.stop(1);
            return null;  // Will not get here
        }
        
        // Look for the start method
        Method mainMethod;
        try {
            mainMethod = mainClass.getDeclaredMethod("main", new Class[] {String[].class});
        } catch (NoSuchMethodException e) {
            System.out.println("WrapperStartStopApp: Unable to locate a static main method in class " + className + ": " + e);
            showUsage();
            WrapperManager.stop(1);
            return null;  // Will not get here
        } catch (SecurityException e) {
            System.out.println("WrapperStartStopApp: Unable to locate a static main method in class " + className + ": " + e);
            showUsage();
            WrapperManager.stop(1);
            return null;  // Will not get here
        }
        
        // Make sure that the method is public and static
        int modifiers = mainMethod.getModifiers();
        if (!(Modifier.isPublic(modifiers) && Modifier.isStatic(modifiers))) {
            System.out.println("WrapperStartStopApp: The main method in class " + className + " must be declared public and static.");
            showUsage();
            WrapperManager.stop(1);
            return null;  // Will not get here
        }
        
        return mainMethod;
    }
    
    private static String[] getArgs(String[] args, int argBase) {
        // The arg at the arg base should be a count of the number of available arguments.
        int argCount;
        try {
            argCount = Integer.parseInt(args[argBase]);
        } catch (NumberFormatException e) {
            System.out.println("WrapperStartStopApp: Illegal argument count: " + args[argBase]);
            showUsage();
            WrapperManager.stop(1);
            return null;  // Will not get here
        }
        if (argCount < 0) {
            System.out.println("WrapperStartStopApp: Illegal argument count: " + args[argBase]);
            showUsage();
            WrapperManager.stop(1);
            return null;  // Will not get here
        }
        
        // Make sure that there are enough arguments in the array.
        if (args.length < argBase + 1 + argCount) {
            System.out.println("WrapperStartStopApp: Not enough argments.  Argument count of " + argCount + " was specified.");
            System.out.println("( " + args.length + " < " + argBase + " + 2 + " + argCount + ")");
            showUsage();
            WrapperManager.stop(1);
            return null;  // Will not get here
        }
        
        // Create the argument array
        String[] mainArgs = new String[argCount];
        System.arraycopy(args, argBase + 1, mainArgs, 0, argCount);
        
        return mainArgs;
    }
    
    /**
     * Displays application usage
     */
    private static void showUsage() {
        System.out.println();
        System.out.println("WrapperStartStopApp Usage:");
        System.out.println("  java com.silveregg.wrapper.WrapperStartStopApp {start_class} {start_arg_count} [start_arguments] {stop_class} {stop_wait} {stop_arg_count} [stop_arguments]");
        System.out.println();
        System.out.println("Where:");
        System.out.println("  start_class:     The fully qualified class name to run to start the application.");
        System.out.println("  start_arg_count: The number of arguments to be passed to the start class's main method.");
        System.out.println("  stop_class:      The fully qualified class name to run to stop the application.");
        System.out.println("  stop_wait:       When stopping, should the Wrapper wait for all threads to complete before exiting (true/false).");
        System.out.println("  stop_arg_count:  The number of arguments to be passed to the stop class's main method.");
        
        System.out.println("  app_parameters: The parameters that would normally be passed to the");
        System.out.println("                  application.");
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
    public static void main(String args[]) {
        // Get the class name of the application
        if (args.length < 5) {
            System.out.println("WrapperStartStopApp: Not enough argments.  Minimum 5 required.");
            showUsage();
            WrapperManager.stop(1);
            return;  // Will not get here
        }
        
        
        // Look for the start main method.
        Method startMainMethod = getMainMethod(args[0]);
        // Get the start arguments
        String[] startArgs = getArgs(args, 1);
        
        
        // Where do the stop arguments start
        int stopArgBase = 2 + startArgs.length;
        if (args.length < stopArgBase + 3) {
            System.out.println("WrapperStartStopApp: Not enough argments. Minimum 3 after start arguments.");
            showUsage();
            WrapperManager.stop(1);
            return;  // Will not get here
        }
        // Look for the stop main method.
        Method stopMainMethod = getMainMethod(args[stopArgBase]);
        // Get the stopWait flag
        boolean stopWait;
        if (args[stopArgBase + 1].equalsIgnoreCase("true")) {
            stopWait = true;
        } else if (args[stopArgBase + 1].equalsIgnoreCase("false")) {
            stopWait = false;
        } else {
            System.out.println("WrapperStartStopApp: The stop_wait argument must be either true or false.");
            showUsage();
            WrapperManager.stop(1);
            return;  // Will not get here
        }
        // Get the start arguments
        String[] stopArgs = getArgs(args, stopArgBase + 2);
        
        
        // Create the WrapperStartStopApp
        WrapperStartStopApp app = new WrapperStartStopApp(startMainMethod, stopMainMethod, stopWait, stopArgs);
        
        // Start the application.  If the JVM was launched from the native
        //  Wrapper then the application will wait for the native Wrapper to
        //  call the application's start method.  Otherwise the start method
        //  will be called immediately.
        WrapperManager.start(app, startArgs);
        
        // This thread ends, the WrapperManager will start the application after the Wrapper has
        //  been propperly initialized by calling the start method above.
    }
}

