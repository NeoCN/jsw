package org.tanukisoftware.wrapper.event;

/*
 * Copyright (c) 1999, 2016 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html
 */

/**
 * WrapperSecondInvocationEvent is fired whenever a second
 *  instance of the Wrapper starts in single invocation mode.  
 *  The property 'wrapper.single_invocation.notify' should also 
 *  exlicitely set to true.
 */
public class WrapperSecondInvocationEvent
    extends WrapperRemoteControlEvent
{
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    /**
     * Creates a new WrapperSecondInvocationEvent.
     */
    public WrapperSecondInvocationEvent()
    {
    }
    
    /*---------------------------------------------------------------
     * WrapperSecondInvocationEvent Methods
     *-------------------------------------------------------------*/
    /**
     * Returns a string representation of the event.
     *
     * @return A string representation of the event.
     */
    public String toString()
    {
        return "WrapperSecondInvocationEvent";
    }
}