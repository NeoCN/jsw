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
 * WrapperServiceExceptions are thrown when the Wrapper is unable to obtain
 *  information on a requested service.
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 */
public class WrapperServiceException
    extends Exception
{
    /**
     * Serial Version UID.
     */
    private static final long serialVersionUID = 5163822791166376887L;

    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    /**
     * Creates a new WrapperServiceException.
     *
     * @param message Message describing the exception.
     */
    WrapperServiceException( byte[] message )
    {
        super( new String( message ) );
    }
}

