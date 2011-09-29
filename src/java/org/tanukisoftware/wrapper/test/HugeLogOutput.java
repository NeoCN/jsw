package org.tanukisoftware.wrapper.test;

/*
 * Copyright (c) 1999, 2011 Tanuki Software, Ltd.
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
        
        // Create a string that is 1,000,060 chars long (16130 copies)
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < 16130; i++ )
        {
            sb.append( subStr );
        }
        String longStr = sb.toString();
        
        // Now loop and print this to the console 10 times.  Log the time before each line 
        System.out.println( "Print out 10 very long lines of output." );
        SimpleDateFormat df = new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.SSS" );
        for ( int i = 0; i < 10; i++ )
        {
            System.out.println( df.format( new Date() ) );
            System.out.println( longStr );
        }
        System.out.println( "All done." );
    }
}

