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
 */

// $Log$
// Revision 1.5  2005/10/03 01:57:18  mortenson
// Modify the Filter test to make sure that filters containing spaces work correctly.
//
// Revision 1.4  2005/05/23 02:39:30  mortenson
// Update the copyright information.
//
// Revision 1.3  2004/01/16 04:41:55  mortenson
// The license was revised for this version to include a copyright omission.
// This change is to be retroactively applied to all versions of the Java
// Service Wrapper starting with version 3.0.0.
//
// Revision 1.2  2003/04/16 04:51:53  mortenson
// On fast machines the Wrapper was detecting that the main thread was exiting
// and causing the application to exit even though
//
// Revision 1.1  2003/04/16 04:37:49  mortenson
// Add a test case to make it easier to test the output filter feature.
//

import org.tanukisoftware.wrapper.WrapperManager;

/**
 *
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 * @version $Revision$
 */
public class Filter {
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    public static void main(String[] args) {
        System.out.println("Test the handling of filters.");
        System.out.println("The Wrapper should restart the JVM when it detects either the string:");
        System.out.println("  \"ERR OR\" or \"N ice long restart message.\", both without the");
        System.out.println("  extra space  It should ignore the string: \"NONERROR\".  Then");
        System.out.println("  it should exit when it detects the string: \"ALL DONE\", once again");
        System.out.println("  without the space.");
        System.out.println();
        
        System.out.println("The next line should be ignored:");
        System.out.println("  NONERROR");
        System.out.println();
        
        if (WrapperManager.getJVMId() >= 4) {
            // Time to shutdown
            System.out.println("The next line should cause the Wrapper to exit:");
            System.out.println("  ALLDONE");
        } else if (WrapperManager.getJVMId() == 3) {
            // Try a restart with spaces.
            System.out.println("The next line should cause the Wrapper to restart the JVM:");
            System.out.println("  Nice long restart message.");
        } else {
            System.out.println("The next line should cause the Wrapper to restart the JVM:");
            System.out.println("  ERROR");
        }
        System.out.println();
        System.out.println("The above message should be caught before this line, but this line");
        System.out.println("  will still be visible.  Wait for 5 seconds before this thread is");
        System.out.println("  allowed to complete.  This prevents the Wrapper from detecting");
        System.out.println("  that the application has completed and exiting normally.  The");
        System.out.println("  Wrapper will try to shutdown the JVM cleanly, so it will not exit");
        System.out.println("  until this thread has completed.");

        try {
            Thread.sleep(5000);
        } catch (InterruptedException e) {
        }

        System.out.println("Main complete.");
    }
}

