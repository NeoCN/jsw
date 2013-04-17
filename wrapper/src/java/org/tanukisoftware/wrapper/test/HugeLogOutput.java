package org.tanukisoftware.wrapper.test;

/*
 * Copyright (c) 1999, 2013 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html
 */

import java.text.SimpleDateFormat;
import java.util.Date;

import org.tanukisoftware.wrapper.WrapperManager;

/**
 *
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 */
public class HugeLogOutput {
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    public static void main(String[] args) {
        // 62 chars long
        String subStr = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

        // This first test is to check for buffer problems increasing the size of the Wrapper's log buffer.
        System.out.println( "Print out 10 long lines of increasing length." );
        // Now loop and print this to the console 10 times.  Log the time before each line.
        StringBuffer sb = new StringBuffer();
        SimpleDateFormat df = new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.SSS" );
        for ( int i = 0; i < 10; i++ )
        {
            for ( int j = 0; j < 10; j++ )
            {
                sb.append( subStr );
            }
            String longStr = sb.toString();
            System.out.println( df.format( new Date() ) );
            System.out.println( longStr );
        }
        
        // This next test is to check for a speed problem which used to exist with VERY large log lines.
        System.out.println( "Print out 10 very long lines of output." );
        // Create a string that is 1,000,060 chars long (16130 copies)
        sb = new StringBuffer();
        for ( int i = 0; i < 16130; i++ )
        {
            sb.append( subStr );
        }
        String hugeStr = sb.toString();
        // Now loop and print this to the console 3 times.  Log the time before each line.
        for ( int i = 0; i < 3; i++ )
        {
            System.out.println( df.format( new Date() ) );
            System.out.println( hugeStr );
        }
        System.out.println( "All done." );
    }
}

