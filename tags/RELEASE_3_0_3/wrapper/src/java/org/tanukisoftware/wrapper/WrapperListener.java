package org.tanukisoftware.wrapper;

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
// Revision 1.2  2003/04/03 04:05:23  mortenson
// Fix several typos in the docs.  Thanks to Mike Castle.
//
// Revision 1.1  2003/02/03 06:55:28  mortenson
// License transfer to TanukiSoftware.org
//

/**
 * Applications which need to be controlled directly as a service can implement
 *  the WrapperListener interface and then register themselves with the
 *  WrapperManager on instantiation.  The WrapperManager will then control the
 *  the class as a service for its entire life-cycle.
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 * @version $Revision$
 */
public interface WrapperListener
{
    /**
     * The start method is called when the WrapperManager is signaled by the 
     *	native wrapper code that it can start its application.  This
     *	method call is expected to return, so a new thread should be launched
     *	if necessary.
     *
     * @param args List of arguments used to initialize the application.
     *
     * @return Any error code if the application should exit on completion
     *         of the start method.  If there were no problems then this
     *         method should return null.
     */
    Integer start( String[] args );
    
    /**
     * Called when the application is shutting down.  The Wrapper assumes that
     *  this method will return fairly quickly.  If the shutdown code code
     *  could potentially take a long time, then WrapperManager.stopping()
     *  should be called to extend the timeout period.  If for some reason,
     *  the stop method can not return, then it must call
     *  WrapperManager.stopped() to avoid warning messages from the Wrapper.
     *
     * @param exitCode The suggested exit code that will be returned to the OS
     *                 when the JVM exits.
     *
     * @return The exit code to actually return to the OS.  In most cases, this
     *         should just be the value of exitCode, however the user code has
     *         the option of changing the exit code if there are any problems
     *         during shutdown.
     */
    int stop( int exitCode );
    
    /**
     * Called whenever the native wrapper code traps a system control signal
     *  against the Java process.  It is up to the callback to take any actions
     *  necessary.  Possible values are: WrapperManager.WRAPPER_CTRL_C_EVENT, 
     *    WRAPPER_CTRL_CLOSE_EVENT, WRAPPER_CTRL_LOGOFF_EVENT, or 
     *    WRAPPER_CTRL_SHUTDOWN_EVENT
     *
     * @param event The system control signal.
     */
    void controlEvent( int event );
}

