package com.silveregg.wrapper;

/*
 * Copyright (c) 1999, 2003 TanukiSoftware.org
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
// Revision 1.32  2003/02/03 06:55:28  mortenson
// License transfer to TanukiSoftware.org
//

/**
 * Handles all communication with the native portion of the Wrapper code.
 *	The native wrapper code will launch Java in a separate process and set
 *	up a server socket which the Java code is expected to open a socket to
 *	on startup.  When the server socket is created, a port will be chosen
 *	depending on what is available to the system.  This port will then be
 *	passed to the Java process as property named "wrapper.port".
 *
 * For security reasons, the native code will only allow connections from
 *	localhost and will expect to receive the key specified in a property
 *	named "wrapper.key".
 *
 * This class is implemented as a singleton class.
 *
 * @deprecated This class has been deprecated in favor of the
 *             org.tanukisoftware.wrapper.WrapperManager class.
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 * @version $Revision$
 */
public final class WrapperManager
{
    public static final int WRAPPER_CTRL_C_EVENT         =
        org.tanukisoftware.wrapper.WrapperManager.WRAPPER_CTRL_C_EVENT;
    public static final int WRAPPER_CTRL_CLOSE_EVENT     =
        org.tanukisoftware.wrapper.WrapperManager.WRAPPER_CTRL_CLOSE_EVENT;
    public static final int WRAPPER_CTRL_LOGOFF_EVENT    =
        org.tanukisoftware.wrapper.WrapperManager.WRAPPER_CTRL_LOGOFF_EVENT;
    public static final int WRAPPER_CTRL_SHUTDOWN_EVENT  =
        org.tanukisoftware.wrapper.WrapperManager.WRAPPER_CTRL_SHUTDOWN_EVENT;
    
    /** Log message at debug log level. */
    public static final int WRAPPER_LOG_LEVEL_DEBUG      =
        org.tanukisoftware.wrapper.WrapperManager.WRAPPER_LOG_LEVEL_DEBUG;
    /** Log message at info log level. */
    public static final int WRAPPER_LOG_LEVEL_INFO       =
        org.tanukisoftware.wrapper.WrapperManager.WRAPPER_LOG_LEVEL_INFO;
    /** Log message at status log level. */
    public static final int WRAPPER_LOG_LEVEL_STATUS     =
        org.tanukisoftware.wrapper.WrapperManager.WRAPPER_LOG_LEVEL_STATUS;
    /** Log message at warn log level. */
    public static final int WRAPPER_LOG_LEVEL_WARN       =
        org.tanukisoftware.wrapper.WrapperManager.WRAPPER_LOG_LEVEL_WARN;
    /** Log message at error log level. */
    public static final int WRAPPER_LOG_LEVEL_ERROR      =
        org.tanukisoftware.wrapper.WrapperManager.WRAPPER_LOG_LEVEL_ERROR;
    /** Log message at fatal log level. */
    public static final int WRAPPER_LOG_LEVEL_FATAL      =
        org.tanukisoftware.wrapper.WrapperManager.WRAPPER_LOG_LEVEL_FATAL;
    
    /*---------------------------------------------------------------
     * Public Methods
     *-------------------------------------------------------------*/
    /**
     * Obtain the current version of Wrapper.
     */
    public static String getVersion() {
        return org.tanukisoftware.wrapper.WrapperManager.getVersion();
    }
    
    /**
     * Obtain the build time of Wrapper.
     */
    public static String getBuildTime() {
        return org.tanukisoftware.wrapper.WrapperManager.getBuildTime();
    }
    
    /**
     * Returns the Id of the current JVM.  JVM Ids increment from 1 each time the wrapper
     *  restarts a new one.
     */
    public static int getJVMId() {
        return org.tanukisoftware.wrapper.WrapperManager.getJVMId();
    }
    
    /**
     * Requests that the current JVM process request a thread dump.  This is
     *  the same as pressing CTRL-BREAK (under Windows) or CTRL-\ (under Unix)
     *  in the the console in which Java is running.  This method does nothing
     *  if the native library is not loaded.
     */
    public static void requestThreadDump() {
        org.tanukisoftware.wrapper.WrapperManager.requestThreadDump();
    }
    
    /**
     * (Testing Method) Causes the WrapperManager to go into a state which makes the JVM appear
     *  to be hung when viewed from the native Wrapper code.  Does not have any effect when the
     *  JVM is not being controlled from the native Wrapper. Useful for testing the Wrapper 
     *  functions.
     */
    public static void appearHung() {
        org.tanukisoftware.wrapper.WrapperManager.appearHung();
    }
    
    /**
     * (Testing Method) Cause an access violation within the Java code.  Useful for testing the
     *  Wrapper functions.  This currently only crashes Sun JVMs and takes advantage of 
     *  Bug #4369043
     */
    public static void accessViolation() {
        org.tanukisoftware.wrapper.WrapperManager.accessViolation();
    }

    /**
     * (Testing Method) Cause an access violation within native JNI code.  Useful for testing the
     *  Wrapper functions. This currently causes the access violation by attempting to write to 
     *  a null pointer.
     */
    public static void accessViolationNative() {
        org.tanukisoftware.wrapper.WrapperManager.accessViolationNative();
    }
        
    /**
     * Returns true if the JVM was launched by the Wrapper application.  False if the JVM
     *  was launched manually without the Wrapper controlling it.
     */
    public static boolean isControlledByNativeWrapper() {
        return org.tanukisoftware.wrapper.WrapperManager.isControlledByNativeWrapper();
    }
    
    /**
     * Returns true if the Wrapper was launched as a service (Windows only).  False if
     *  launched as a console.  This can be useful if you wish to display a user
     *  interface when in Console mode.  On unix systems, the Wrapper is always launched
     *  as a console application, so this method will always return false.
     */
    public static boolean isLaunchedAsService() {
        return org.tanukisoftware.wrapper.WrapperManager.isLaunchedAsService();
    }
    
    /**
     * Returns true if the wrapper.debug property is set the wrapper config file.
     */
    public static boolean isDebugEnabled() {
        return org.tanukisoftware.wrapper.WrapperManager.isDebugEnabled();
    }
    
    /**
     * Start the Java side of the Wrapper code running.  This will make it
     *  possible for the native side of the Wrapper to detect that the Java
     *  Wrapper is up and running.
     */
    public static void start(WrapperListener listener, String[] args) {
        org.tanukisoftware.wrapper.WrapperManager.start( listener, args );
    }
    
    /**
     * Tells the native wrapper that the JVM wants to restart, then informs
     *	all listeners that the JVM is about to shutdown before killing the JVM.
     */
    public static void restart() {
        org.tanukisoftware.wrapper.WrapperManager.restart();
    }
    
    /**
     * Tells the native wrapper that the JVM wants to shut down, then informs
     *	all listeners that the JVM is about to shutdown before killing the JVM.
     */
    public static void stop(int exitCode) {
        org.tanukisoftware.wrapper.WrapperManager.stop( exitCode );
    }

    /**
     * Signal the native wrapper that the startup is progressing but that more
     *  time is needed.
     */
    public static void signalStarting(int waitHint) {
        org.tanukisoftware.wrapper.WrapperManager.signalStarting( waitHint );
    }
    
    /**
     * Signal the native wrapper that the shutdown is progressing but that more
     *  time is needed.
     */
    public static void signalStopping(int waitHint) {
        org.tanukisoftware.wrapper.WrapperManager.signalStopping( waitHint );
    }
    
    /**
     * This method should not normally be called by user code as it is called
     *  from within the stop and restart methods.  However certain applications
     *  which stop the JVM may need to call this method to let the wrapper code
     *  know that the shutdown was intentional.
     */
    public static void signalStopped(int exitCode) {
        org.tanukisoftware.wrapper.WrapperManager.signalStopped( exitCode );
    }
    
    /**
     * Returns true if the ShutdownHook for the JVM has already been triggered.
     */
    public static boolean hasShutdownHookBeenTriggered() {
        return org.tanukisoftware.wrapper.WrapperManager.hasShutdownHookBeenTriggered();
    }
    
    /**
     * Requests that the Wrapper log a message at the specified log level.
     *  If the JVM is not being managed by the Wrapper then calls to this
     *  method will be ignored.  This method has been optimized to ignore
     *  messages at a log level which will not be logged given the current
     *  log levels of the Wrapper.
     * <p>
     * Log messages will currently by trimmed by the Wrapper at 4k (4096 bytes).
     * <p>
     * Because of differences in the way console output is collected and
     *  messages logged via this method, it is expected that interspersed
     *  console and log messages will not be in the correct order in the
     *  resulting log file.
     * <p>
     * This method was added to allow simple logging to the wrapper.log
     *  file.  This is not meant to be a full featured log file and should
     *  not be used as such.  Please look into a logging package for most
     *  application logging.
     *
     * @param logLevel The level to log the message at can be one of
     *                 WRAPPER_LOG_LEVEL_DEBUG, WRAPPER_LOG_LEVEL_INFO,
     *                 WRAPPER_LOG_LEVEL_STATUS, WRAPPER_LOG_LEVEL_WARN,
     *                 WRAPPER_LOG_LEVEL_ERROR, or WRAPPER_LOG_LEVEL_FATAL.
     * @param message The message to be logged.
     */
    public static void log(int logLevel, String message) {
        org.tanukisoftware.wrapper.WrapperManager.log( logLevel, message );
    }
    
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    /** 
     * This class can not be instantiated.
     */
    private WrapperManager() {
    }
}

