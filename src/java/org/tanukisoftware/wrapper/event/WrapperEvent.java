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

import java.util.EventObject;

import org.tanukisoftware.wrapper.WrapperManager;

/**
 * WrapperEvents are used to notify WrapperEventListeners of various wrapper
 *  related events.
 * <p>
 * For performance reasons, some event instances may be reused by the code
 *  which fires them off.  For this reason, references to the event should
 *  never be referenced outside the scope of the WrapperListener.processEvent
 *  method.
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 */
public abstract class WrapperEvent
    extends EventObject
{
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    /**
     * Creates a new WrapperEvent.
     */
    protected WrapperEvent()
    {
        super( WrapperManager.class );
    }
    
    /*---------------------------------------------------------------
     * Methods
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
        return 0;
    }
}