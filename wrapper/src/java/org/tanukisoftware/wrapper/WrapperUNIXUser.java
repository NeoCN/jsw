package org.tanukisoftware.wrapper;

/*
 * Copyright (c) 1999, 2003 TanukiSoftware.org
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without 
 * restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sub-license , and/or 
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
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// $Log$
// Revision 1.1  2004/01/10 17:17:26  mortenson
// Add the ability to request user information.
//

import java.util.Date;

/**
 * A WrapperUser contains information about a user account on the platform
 *  running the Wrapper.  A WrapperUser is obtained by calling
 *  WrapperManager.getUser() or WrapperManager.getInteractiveUser().
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 * @version $Revision$
 */
public class WrapperUNIXUser
    extends WrapperUser
{
    /** The UID of the user. */
    private int m_uid;
    
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    WrapperUNIXUser( int uid, byte[] user )
    {
        super( user );
        
        m_uid = uid;
    }
    
    /*---------------------------------------------------------------
     * Methods
     *-------------------------------------------------------------*/
    /**
     * Returns the UID of the user account.
     *
     * @return The UID of the user account.
     */
    public int getUID()
    {
        return m_uid;
    }
    
    void addGroup( byte[] sid, byte[] user, byte[] domain )
    {
        //addGroup( new WrapperUSERGroup( gid, group ) );
    }
    
    /**
     * Returns a string representation of the user.
     *
     * @return A string representation of the user.
     */
    public String toString()
    {
        StringBuffer sb = new StringBuffer();
        sb.append( "WrapperUNIXUser[" );
        sb.append( getUID() );
        sb.append( ", " );
        sb.append( getUser() );
        
        sb.append( ", groups {" );
        WrapperGroup[] groups = getGroups();
        for ( int i = 0; i < groups.length; i++ )
        {
            if ( i > 0 )
            {
                sb.append( ", " );
            }
            sb.append( groups[i].toString() );
        }
        sb.append( "}" );
        
        sb.append( "]" );
        return sb.toString();
    }
}

