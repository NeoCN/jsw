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
public class WrapperUNIXGroup
    extends WrapperGroup
{
    /** The GID of the Group. */
    private int m_gid;
    
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    WrapperUNIXGroup( int gid, byte[] name )
    {
        super( name );

        m_gid = gid;
    }
    
    /*---------------------------------------------------------------
     * Methods
     *-------------------------------------------------------------*/
    /**
     * Returns the GID of the group.
     *
     * @return The GID of the group.
     */
    public int getGID()
    {
        return m_gid;
    }
    
    public String toString()
    {
        return "WrapperUNIXGroup[" + getGID() + ", " + getGroup() + "]";
    }
}

