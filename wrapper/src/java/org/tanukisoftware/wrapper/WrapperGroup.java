package org.tanukisoftware.wrapper;

/*
 * Copyright (c) 1999, 2007 Tanuki Software, Inc.
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
 * A WrapperGroup contains information about a group which a user
 *  belongs to.  A WrapperGroup is obtained via a WrapperUser.
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 */
public abstract class WrapperGroup
{
    /* The name of the group. */
    private String m_group;
    
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    WrapperGroup( byte[] group )
    {
        // Decode the parameters using the default system encoding.
        m_group = new String( group );
    }
    
    /*---------------------------------------------------------------
     * Methods
     *-------------------------------------------------------------*/
    /**
     * Returns the name of the group.
     *
     * @return The name of the group.
     */
    public String getGroup()
    {
        return m_group;
    }
}

