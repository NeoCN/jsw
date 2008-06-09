package org.tanukisoftware.wrapper;

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
 * A WrapperGroup contains information about a group which a user
 *  belongs to.  A WrapperGroup is obtained via a WrapperUser.
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 */
public class WrapperWin32Group
    extends WrapperGroup
{
    /** The current SID of the Group. */
    private String m_sid;
    
    /** The domain of the User Account. */
    private String m_domain;
    
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    WrapperWin32Group( byte[] sid, byte[] user, byte[] domain )
    {
        super( user );
        
        // Decode the parameters using the default system encoding.
        m_sid = new String( sid );
        m_domain = new String( domain );
    }
    
    /*---------------------------------------------------------------
     * Methods
     *-------------------------------------------------------------*/
    /**
     * Returns the current Security Identifier (SID) of the user account.
     *
     * @return The SID of the user account.
     */
    public String getSID()
    {
        return m_sid;
    }
    
    /**
     * Returns the domain name of the user account.
     *
     * @return The domain name of the user account.
     */
    public String getDomain()
    {
        return m_domain;
    }
    
    /**
     * Returns the full name of the group.
     *
     * @return The full name of the group.
     */
    public String getAccount()
    {
        return m_domain + "/" + getGroup();
    }
    
    public String toString()
    {
        return "WrapperWin32Group[" + getAccount() + "]";
    }
}

