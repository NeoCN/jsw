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
// Revision 1.2  2002/09/10 16:03:03  mortenson
// Improve the performance of the log from JVM feature by allowing for more than
// one log message each time through the main loop.
//
// Revision 1.1  2002/09/09 17:19:47  mortenson
// Add ability to log to specific log levels from within the Wrapper.
//

import com.silveregg.wrapper.WrapperManager;

/**
 * Tests the various log levels via the WrapperManager.log() method.
 */
public class LogOutput {
    private static void sleep() {
        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {}
    }
    
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    public static void main(String[] args) {
        System.out.println("Test the various log levels...");
        WrapperManager.log(WrapperManager.WRAPPER_LOG_LEVEL_DEBUG,  "Debug output");
        WrapperManager.log(WrapperManager.WRAPPER_LOG_LEVEL_INFO,   "Info output");
        WrapperManager.log(WrapperManager.WRAPPER_LOG_LEVEL_STATUS, "Status output");
        WrapperManager.log(WrapperManager.WRAPPER_LOG_LEVEL_WARN,   "Warn output");
        WrapperManager.log(WrapperManager.WRAPPER_LOG_LEVEL_ERROR,  "Error output");
        WrapperManager.log(WrapperManager.WRAPPER_LOG_LEVEL_FATAL,  "Fatal output");
        
        // Let things catch up as the timing of WrapperManager.log output and System.out
        //  output can not be guaranteed.
        sleep();
        
        System.out.println("Put the logger through its paces...");
        WrapperManager.log(WrapperManager.WRAPPER_LOG_LEVEL_INFO,  "Special C characters in %s %d % %%");
        sleep();
        WrapperManager.log(WrapperManager.WRAPPER_LOG_LEVEL_INFO,   "");
        sleep();
        
        String sa = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
        StringBuffer sb = new StringBuffer();
        for (int i = 0; i < 100; i++) {
            sb.append(sa);
        }
        WrapperManager.log(WrapperManager.WRAPPER_LOG_LEVEL_INFO, sb.toString());
        sleep();

        sb = new StringBuffer();
        for (int i = 0; i < 100; i++) {
            sb.append(sa);
            sb.append("\n");
        }
        WrapperManager.log(WrapperManager.WRAPPER_LOG_LEVEL_INFO, sb.toString());
        sleep();
        
        for (int i = 0; i < 100; i++) {
            WrapperManager.log(WrapperManager.WRAPPER_LOG_LEVEL_INFO, sa);
        }
    }
}

