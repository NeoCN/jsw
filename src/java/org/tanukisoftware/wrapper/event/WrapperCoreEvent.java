package org.tanukisoftware.wrapper.event;

/*
 * Copyright (c) 1999, 2008 Tanuki Software, Inc.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the confidential and proprietary information
 * of Tanuki Software.  ("Confidential Information").  You shall
 * not disclose such Confidential Information and shall use it
 * only in accordance with the terms of the license agreement you
 * entered into with Tanuki Software.
 */

/**
 * WrapperCoreEvents are used to notify the listener of the internal
 *  workings of the Wrapper.
 *
 * WARNING - Great care should be taken when receiving events of this type.
 *  They are sent from within the Wrapper's internal timing thread.  If the
 *  listner takes too much time working with the event, Wrapper performance
 *  could be adversely affected.  If unsure, it is recommended that events
 *  of this type not be included.
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 */
public abstract class WrapperCoreEvent
    extends WrapperEvent
{
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    /**
     * Creates a new WrapperCoreEvent.
     */
    protected WrapperCoreEvent()
    {
    }
    
    /*---------------------------------------------------------------
     * WrapperCoreEvent Methods
     *-------------------------------------------------------------*/
    /**
     * Returns a set of event flags for which the event should be fired.
     *  This value is compared with the mask supplied when when a
     *  WrapperEventListener is registered to decide which listeners should
     *  receive the event.
     * <p>
     * If a subclassed, the return value of the super class should usually
     *  be ORed with any additional flags.
     *
     * @return a set of event flags.
     */
    public long getFlags()
    {
        return super.getFlags() | WrapperEventListener.EVENT_FLAG_CORE;
    }
}