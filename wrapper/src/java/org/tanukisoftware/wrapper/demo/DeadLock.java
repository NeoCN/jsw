package org.tanukisoftware.wrapper.demo;



public class DeadLock
{
    private int m_id;
    private Object m_obj1;
    private Object m_obj2;

    protected DeadLock( int id, Object obj1, Object obj2 )
    {
        m_id = id;
        m_obj1 = obj1;
        m_obj2 = obj2;

        Thread runner = new Thread( "Locker-" + m_id )
        {
            public void run()
            {
                System.out.println( "Locker-" + m_id + ": Started" );

                try
                {
                    lockFirst();
                }
                catch ( Throwable t )
                {
                    t.printStackTrace();

                }

                System.out.println( "Locker-" + m_id + ": Complete" );
            }
        };
        runner.start();
    }

    /*---------------------------------------------------------------
     * Methods
     *-------------------------------------------------------------*/
    private void lockSecond()
    {
        System.out.println( "Locker-" + m_id + ": Try locking " + m_obj2.toString() + "..." );
        synchronized ( m_obj2 )
        {
            System.out.println( "Locker-" + m_id + ": Oops! Locked " + m_obj2.toString() );
        }
    }

    public void create3ObjectDeadlock()
    {
        Object obj1 = new Object();
        Object obj2 = new Object();
        Object obj3 = new Object();

        new DeadLock( 1, obj1, obj2 );
        new DeadLock( 2, obj2, obj3 );
        new DeadLock( 3, obj3, obj1 );
    }

    public void create2ObjectDeadlock()
    {
        Object obj1 = new Object();
        Object obj2 = new Object();

        new DeadLock( 1, obj1, obj2 );
        new DeadLock( 2, obj2, obj1 );
    }

    private void lockFirst()
    {
        System.out.println( "Locker-" + m_id + ": Locking " + m_obj1.toString() + "..." );
        synchronized ( m_obj1 )
        {
            System.out.println( "Locker-" + m_id + ": Locked " + m_obj1.toString() );

            try
            {
                Thread.sleep( 2000 );
            }
            catch ( InterruptedException e )
            {
            }

            lockSecond();
        }
    }
}