package org.tanukisoftware.wrapper.jmx;

/*
 * Copyright (c) 1999, 2006 Tanuki Software Inc.
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
 */

// $Log$
// Revision 1.4  2006/02/24 05:45:58  mortenson
// Update the copyright.
//
// Revision 1.3  2005/05/23 02:40:13  mortenson
// Update the copyright information.
//
// Revision 1.2  2004/01/16 04:42:01  mortenson
// The license was revised for this version to include a copyright omission.
// This change is to be retroactively applied to all versions of the Java
// Service Wrapper starting with version 3.0.0.
//
// Revision 1.1  2003/09/03 14:39:58  mortenson
// Added a pair of MBean interfaces which allow the Wrapper to be controlled
// using JMX.  See the new JMX section in the documentation for details.
//

public interface WrapperManagerMBean
{
    /**
     * Obtain the current version of Wrapper.
     *
     * @return The version of the Wrapper.
     */
    String getVersion();
    
    /**
     * Obtain the build time of Wrapper.
     *
     * @return The time that the Wrapper was built.
     */
    String getBuildTime();
    
    /**
     * Returns the Id of the current JVM.  JVM Ids increment from 1 each time
     *  the wrapper restarts a new one.
     *
     * @return The Id of the current JVM.
     */
    int getJVMId();
    
    /**
     * Requests that the current JVM process request a thread dump.  This is
     *  the same as pressing CTRL-BREAK (under Windows) or CTRL-\ (under Unix)
     *  in the the console in which Java is running.  This method does nothing
     *  if the native library is not loaded.
     */
    void requestThreadDump();
    
    /**
     * Returns true if the JVM was launched by the Wrapper application.  False
     *  if the JVM was launched manually without the Wrapper controlling it.
     *
     * @return True if the current JVM was launched by the Wrapper.
     */
    boolean isControlledByNativeWrapper();
    
    /**
     * Returns true if the Wrapper was launched as an NT service on Windows or
     *  as a daemon process on UNIX platforms.  False if launched as a console.
     *  This can be useful if you wish to display a user interface when in
     *  Console mode.  On UNIX platforms, this is not as useful because an
     *  X display may not be visible even if launched in a console.
     *
     * @return True if the Wrapper is running as an NT service or daemon
     *         process.
     */
    boolean isLaunchedAsService();
    
    /**
     * Returns true if the wrapper.debug property, or any of the logging
     *  channels are set to DEBUG in the wrapper configuration file.  Useful
     *  for deciding whether or not to output certain information to the
     *  console.
     *
     * @return True if the Wrapper is logging any Debug level output.
     */
    boolean isDebugEnabled();
    
    /**
     * Tells the native wrapper that the JVM wants to restart, then informs
     *	all listeners that the JVM is about to shutdown before killing the JVM.
     */
    void restart();
    
    /**
     * Tells the native wrapper that the JVM wants to shut down, then informs
     *	all listeners that the JVM is about to shutdown before killing the JVM.
     *
     * @param exitCode The exit code that the Wrapper will return when it exits.
     */
    void stop( int exitCode );
    
    /**
     * Returns true if the ShutdownHook for the JVM has already been triggered.
     *  Some code needs to know whether or not the system is shutting down.
     *
     * @return True if the ShutdownHook for the JVM has already been triggered.
     */
    boolean getHasShutdownHookBeenTriggered();
}