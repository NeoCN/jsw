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
// Revision 1.1  2002/09/17 14:56:08  mortenson
// Added a test to verify that there are no memory leaks in the JVM.
//

import java.io.FileWriter;
import java.io.IOException;
import java.io.Writer;

public class Memory implements Runnable
{
    private Writer m_writer;
    private Thread m_runner;
    
    /*---------------------------------------------------------------
     * Runnable Method
     *-------------------------------------------------------------*/
    public void run()
    {
        if ( m_runner == null )
        {
            // This is the runner
            m_runner = Thread.currentThread();
        }
        else
        {
            System.out.println("Stopping..." );
            // This is the shutdown hook.  Sloppy code, but simple :-)
            m_runner = null;
            return;
        }
        
        long startTime = System.currentTimeMillis();
        long lastTest = startTime;
        try
        {
            m_writer.write( "--> Starting Memory Log\n" );
            m_writer.flush();
    
            while( m_runner != null )
            {
                long now = System.currentTimeMillis();
                System.out.println( "Running for " + ( now - startTime ) + "ms..." );
                
                if ( now - lastTest > 15000 )
                {
                    Runtime rt = Runtime.getRuntime();
                    System.gc();
                    long totalMemory = rt.totalMemory();
                    long freeMemory = rt.freeMemory();
                    long usedMemory = totalMemory - freeMemory;
                    
                    m_writer.write( "total memory=" + pad( totalMemory, 10 )
                        + ", used=" + pad( usedMemory, 10 )
                        + ", free=" + pad( freeMemory, 10 ) + "\n" );
                    m_writer.flush();
                    
                    lastTest = now;
                }
                
                try
                {
                    Thread.sleep( 250 );
                }
                catch ( InterruptedException e )
                {
                }
            }
            
            m_writer.write( "<-- Stopping Memory Log\n" );
            m_writer.flush();
            m_writer.close();
        }
        catch ( IOException e )
        {
            e.printStackTrace();
        }
    }
    
    /*---------------------------------------------------------------
     * Methods
     *-------------------------------------------------------------*/
    private static final String PADDING = "                ";
    private String pad( long n, int len )
    {
        String s = Long.toString( n );
        int sLen = s.length();
        if ( sLen < len )
        {
            s = s + PADDING.substring( 0, len - sLen );
        }
        return s;
    }
    /*---------------------------------------------------------------
     * Main Method
     *-------------------------------------------------------------*/
    public static void main(String[] args)
    {
        System.out.println("Memory Tester Running...");
        
        Memory app = new Memory();
        
        // Create a Writer for the memory output
        try
        {
            app.m_writer = new FileWriter( "memory.log" );
        }
        catch ( IOException e )
        {
            e.printStackTrace();
            return;
        }
        
        // Register a shutdown hook
        Runtime r = Runtime.getRuntime();
        r.addShutdownHook( new Thread( app, "shutdown-hook" ) );
        
        // Start the runner
        new Thread( app, "runner" ).start();
    }
}