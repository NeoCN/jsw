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
// Revision 1.1  2002/08/11 05:28:50  mortenson
// Added a new test for the new wrapper.jvm_exit.timeout property.
//
// Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
// no message
//

public class ShutdownHook {
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    public static void main(String[] args) {
        System.out.println("This application registers a shutdown hook which");
        System.out.println("should be executed after the JVM has told the Wrapper");
        System.out.println("it is exiting.");
        System.out.println("This is to test the wrapper.jvm_exit.timeout property");
        
        Runtime runtime = Runtime.getRuntime();
        runtime.addShutdownHook( new Thread() {
                public void run() {
                    System.out.println("Starting shutdown hook. Loop for 20 seconds.");
                    System.out.println("Should timeout unless this property is set: wrapper.jvm_exit.timeout=25");

                    long start = System.currentTimeMillis();
                    while(System.currentTimeMillis() - start < 20000)
                    {
                        Thread.yield();
                    }
                    System.out.println("Shutdown look complete. Should exit now.");
                }
            });
        
        System.out.println("Application complete.  Wrapper should stop, invoking the shutdown hooks.");
        System.out.println();
    }
}

