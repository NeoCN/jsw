package org.tanukisoftware.wrapper.test;

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
// Revision 1.1  2003/02/03 06:55:29  mortenson
// License transfer to TanukiSoftware.org
//

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;

/**
 *
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 * @version $Revision$
 */
public class SystemIn {
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    public static void main(String[] args) {
        System.out.println("Test the functionality of System.in when run as a console application.");
        System.out.println("If being controlled by the Wrapper, then an IOException should be thrown.");
        System.out.println("If run as a standard Java app, then no errors should be thrown.");
        BufferedReader r = new BufferedReader(new InputStreamReader(System.in));
        try {
            String line;
            do {
                System.out.println("Type something (return quits):");
                System.out.print(" Input: ");
                line = r.readLine();
                if (!line.equals("")) {
                    System.out.println("   Got: " + line );
                }
            } while (!line.equals(""));
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

