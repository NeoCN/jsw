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

import java.io.FileWriter;
import java.io.IOException;

// $Log$
// Revision 1.1  2002/08/22 13:38:56  mortenson
// Add a test to enable performace testing of Java output logging.
//
// Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
// no message
//

public class OutputLoader {
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    public static void main(String[] args) {
        System.out.println("Start outputting lots of data.");
        
        long start = System.currentTimeMillis();
        long now;
        int count = 0;
        while ((now = System.currentTimeMillis()) < start + 20000) {
            System.out.println("Testing line Out #" + (++count));
            System.err.println("Testing line Err #" + (++count));
        }
        
        System.out.println("Printed " + count + " lines of output in 20 seconds");
        
        // Write the output to a file as well, so we can see the results
        //  when output is disabled.
        try {
            FileWriter fw = new FileWriter("OutputLoader.log", true);
            fw.write("Printed " + count + " lines of output in 20 seconds\n");
            fw.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

