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
// Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
// no message
//

public interface WrapperListener {
    /**
     * The start method is called when the WrapperManager is signalled by the 
     *	native wrapper code that it can start its application.  This
     *	method call is expected to return, so a new thread should be launched
     *	it necessary.
     * If there are any problems, then an Integer should be returned, set to
     *	the desired exit code.  If the application should continue,
     *	return null.
     */
    Integer start(String[] args);
    
    /**
     * Called when the application is shutting down.
     */
    int stop(int exitCode);
    
    /**
     * Called whenever the native wrapper code traps a system control signal
     *  against the Java process.  It is up to the callback to take any actions
     *  necessary.  Possible values are: WrapperManager.WRAPPER_CTRL_C_EVENT, 
     *    WRAPPER_CTRL_CLOSE_EVENT, WRAPPER_CTRL_LOGOFF_EVENT, or 
     *    WRAPPER_CTRL_SHUTDOWN_EVENT
     */
    void controlEvent(int event);
}

