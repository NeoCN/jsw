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
// Revision 1.9  2003/01/20 03:21:07  mortenson
// Add limited support for java 1.2.x
//
// Revision 1.8  2002/11/06 05:44:51  mortenson
// Add support for invoking a thread dump from a method call within the JVM.
//
// Revision 1.7  2002/06/02 11:38:42  mortenson
// Increase window size so that the window displays on XP correctly.
//
// Revision 1.6  2002/05/22 11:43:10  rybesh
// fixed some spelling errors
//
// Revision 1.5  2002/05/17 09:52:42  mortenson
// Add a Restart button to the TestWrapper application.
//
// Revision 1.4  2002/05/17 09:08:45  mortenson
// Make the test shutdown correctly when shutdown from a shutdown hook.
//
// Revision 1.3  2002/05/16 03:34:32  mortenson
// Added comments to the buttons that had no comments to make their use clear.
//
// Revision 1.2  2001/12/06 09:36:24  mortenson
// Docs changes, Added sample apps, Fixed some problems with
// relative paths  (See revisions.txt)
//
// Revision 1.1.1.1  2001/11/07 08:54:20  mortenson
// no message
//

import com.silveregg.wrapper.WrapperManager;
import com.silveregg.wrapper.WrapperListener;

import java.awt.Button;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Frame;
import java.awt.Label;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class Main implements WrapperListener {
    private MainFrame _frame;
    
    /**************************************************************************
     * Constructors
     *************************************************************************/
    private Main() {
    }
    
    private class MainFrame extends Frame implements ActionListener {
        MainFrame() {
            super("Wrapper Test Application");
            
            setLayout(new FlowLayout());
            
            Button exitButton = new Button("Exit");
            add(exitButton);
            exitButton.addActionListener(this);
            
            Button avButton = new Button("Access Violation");
            add(avButton);
            avButton.addActionListener(this);
            
            Button navButton = new Button("Native Access Violation");
            add(navButton);
            navButton.addActionListener(this);
            
            Button ahButton = new Button("Simulate JVM Hang");
            add(ahButton);
            ahButton.addActionListener(this);
            
            Button seButton = new Button("System.exit(0)");
            add(seButton);
            seButton.addActionListener(this);
            
            Button rhButton = new Button("Runtime.getRuntime().halt(0)");
            add(rhButton);
            rhButton.addActionListener(this);
            
            Button rsButton = new Button("Request Restart");
            add(rsButton);
            rsButton.addActionListener(this);
            
            Button tdButton = new Button("Request Thread Dump");
            add(tdButton);
            tdButton.addActionListener(this);
            
            add(new Label("The Access Violation button only works with Sun JVMs."));
            add(new Label("Also try killing the JVM process or pressing CTRL-C in the console window."));
            add(new Label("Simulate JVM Hang only has an effect when controlled by native Wrapper."));
            add(new Label("System.exit(0) should cause the Wrapper to exit."));
            add(new Label("Runtime.getRuntime().halt(0) should result in the JVM restarting."));
            add(new Label("Request Restart should cause the JVM to stop and be restarted cleanly."));
            add(new Label("Request Thread Dump should cause the JVM dump its thread states."));
            
            setSize(new Dimension(550, 290));
        }
        
        public void actionPerformed(ActionEvent event) {
            String command = event.getActionCommand();
            if (command.equals("Exit")) {
                WrapperManager.stop(0);
            } else if (command.equals("Access Violation")) {
                WrapperManager.accessViolation();
            } else if (command.equals("Native Access Violation")) {
                WrapperManager.accessViolationNative();
            } else if (command.equals("Simulate JVM Hang")) {
                WrapperManager.appearHung();
            } else if (command.equals("System.exit(0)")) {
                System.exit(0);
            } else if (command.equals("Runtime.getRuntime().halt(0)")) {
                // Execute runtime.halt(0) using reflection so this class will
                //  compile on 1.2.x versions of Java.
                Method haltMethod;
                try {
                    haltMethod = Runtime.class.getMethod("halt", new Class[] {Integer.TYPE});
                } catch (NoSuchMethodException e) {
                    System.out.println("halt not supported by current JVM.");
                    haltMethod = null;
                }
                
                if (haltMethod != null) {
                    Runtime runtime = Runtime.getRuntime();
                    try {
                        haltMethod.invoke(runtime, new Object[] {new Integer(0)});
                    } catch (IllegalAccessException e) {
                        System.out.println("Unable to call runitme.halt: " + e.getMessage());
                    } catch (InvocationTargetException e) {
                        System.out.println("Unable to call runitme.halt: " + e.getMessage());
                    }
                }
            } else if (command.equals("Request Restart")) {
                WrapperManager.restart();
            } else if (command.equals("Request Thread Dump")) {
                WrapperManager.requestThreadDump();
            }
        }
    }
    
    /**************************************************************************
     * WrapperListener Methods
     *************************************************************************/
    public Integer start(String[] args) {
        System.out.println("start()");
        
        _frame = new MainFrame();
        _frame.setVisible(true);
        
        return null;
    }
    
    public int stop(int exitCode) {
        System.out.println("stop(" + exitCode + ")");
        
        if (_frame != null) {
            if (!WrapperManager.hasShutdownHookBeenTriggered()) {
                _frame.setVisible(false);
                _frame.dispose();
            }
            _frame = null;
        }
        
        return exitCode;
    }
    
    public void controlEvent(int event) {
        System.out.println("controlEvent(" + event + ")");
        if (event == WrapperManager.WRAPPER_CTRL_C_EVENT) {
            WrapperManager.stop(0);
        }
    }
    
    /**************************************************************************
     * Main Method
     *************************************************************************/
    public static void main(String[] args) {
        System.out.println("Initializing...");
        
        // Start the application.  If the JVM was launched from the native
        //  Wrapper then the application will wait for the native Wrapper to
        //  call the application's start method.  Otherwise the start method
        //  will be called immediately.
        WrapperManager.start(new Main(), args);
    }
}

