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
// Revision 1.18  2003/10/12 18:01:52  mortenson
// Back out some changes which made the WrapperManager look for the native
// library by using the OS/arch as keys.  I want to solve that problem a different
// way.
//
// Revision 1.17  2003/09/12 04:03:52  mortenson
// Make it possible to load the native library using files named after the platform
// and architecture.
//
// Revision 1.16  2003/09/04 05:40:08  mortenson
// Added a new wrapper.ping.interval property which lets users control the
// frequency that the Wrapper pings the JVM.
//
// Revision 1.15  2003/09/03 14:39:58  mortenson
// Added a pair of MBean interfaces which allow the Wrapper to be controlled
// using JMX.  See the new JMX section in the documentation for details.
//
// Revision 1.14  2003/09/03 09:26:26  mortenson
// Modify the WrapperManager.isLaunchedAsService() method on UNIX systems so it
// now returns true if the Wrapper was launched with the wrapper.daemonize flag
// set.
//
// Revision 1.13  2003/09/03 02:33:38  mortenson
// Requested restarts no longer reset the restart count.
// Add new wrapper.ignore_signals property.
//
// Revision 1.12  2003/08/28 07:21:54  mortenson
// Remove output to System.out in the WrapperManager.requestThreadDump()
// method to avoid hang problems if the JVM is already hung while accessing
// the System.out object.
//
// Revision 1.11  2003/08/14 09:31:34  mortenson
// Fix a problem where the native library was missing from the release of OSX
//
// Revision 1.10  2003/07/01 14:49:45  mortenson
// Fix a problem where the JVM would sometimes hang when trying to shutdown if
// the wrapper.key parameter was passed to the JVM while not being controlled
// by the Wrapper.
//
// Revision 1.9  2003/06/07 05:18:32  mortenson
// Add a new method WrapperManager.stopImmediate which will cause the JVM to
// exit immediately without calling any stop methods or shutdown hooks.
//
// Revision 1.8  2003/05/29 09:27:14  mortenson
// Improve the debug output so that packet codes are now shown using a name
// rather than a raw number.
//
// Revision 1.7  2003/04/15 15:32:06  mortenson
// Fix a typo in a warning message.
//
// Revision 1.6  2003/04/09 06:26:14  mortenson
// Add some extra checks in the event where the native library can not be loaded
// so that the WrapperManager can differentiate between the library missing and
// not being readable due to permission problems.
//
// Revision 1.5  2003/04/03 04:05:23  mortenson
// Fix several typos in the docs.  Thanks to Mike Castle.
//
// Revision 1.4  2003/04/02 10:05:53  mortenson
// Modified the wrapper.ping.timeout property so it also controls the ping
// timeout within the JVM.  Before the timeout on responses to the Wrapper
// could be controlled, but the ping timeout within the JVM was hardcoded to
// 30 seconds.
//
// Revision 1.3  2003/03/07 02:11:18  mortenson
// Fix a problem with the wrapper.disable_shutdown_hook.  Due to a typo in the
// source, the property was being ignored.  This was broken in the 3.0.0 release.
//
// Revision 1.2  2003/03/02 04:23:31  mortenson
// Add a little more javadocs.
//
// Revision 1.1  2003/02/03 06:55:28  mortenson
// License transfer to TanukiSoftware.org
//

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.BindException;
import java.net.ConnectException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Properties;
import java.util.StringTokenizer;

import org.tanukisoftware.wrapper.resources.ResourceManager;

/**
 * Handles all communication with the native portion of the Wrapper code.
 *	The native wrapper code will launch Java in a separate process and set
 *	up a server socket which the Java code is expected to open a socket to
 *	on startup.  When the server socket is created, a port will be chosen
 *	depending on what is available to the system.  This port will then be
 *	passed to the Java process as property named "wrapper.port".
 *
 * For security reasons, the native code will only allow connections from
 *	localhost and will expect to receive the key specified in a property
 *	named "wrapper.key".
 *
 * This class is implemented as a singleton class.
 *
 * Generate JNI Headers with the following command in the build/classes
 *  directory:
 *    javah -jni -classpath ./ org.tanukisoftware.wrapper.WrapperManager
 *
 * @author Leif Mortenson <leif@tanukisoftware.com>
 * @version $Revision$
 */
public final class WrapperManager
    implements Runnable
{
    private static final String  WRAPPER_CONNECTION_THREAD_NAME = "Wrapper-Connection";
    
    private static final int DEFAULT_PORT                = 15003;
    private static final int DEFAULT_SO_TIMEOUT          = 10000;
    private static final long DEFAULT_CPU_TIMEOUT        = 10000L;
    
    private static final byte WRAPPER_MSG_START          = (byte)100;
    private static final byte WRAPPER_MSG_STOP           = (byte)101;
    private static final byte WRAPPER_MSG_RESTART        = (byte)102;
    private static final byte WRAPPER_MSG_PING           = (byte)103;
    private static final byte WRAPPER_MSG_STOP_PENDING   = (byte)104;
    private static final byte WRAPPER_MSG_START_PENDING  = (byte)105;
    private static final byte WRAPPER_MSG_STARTED        = (byte)106;
    private static final byte WRAPPER_MSG_STOPPED        = (byte)107;
    private static final byte WRAPPER_MSG_KEY            = (byte)110;
    private static final byte WRAPPER_MSG_BADKEY         = (byte)111;
    private static final byte WRAPPER_MSG_LOW_LOG_LEVEL  = (byte)112;
    private static final byte WRAPPER_MSG_PING_TIMEOUT   = (byte)113;
    
    /** Log commands are actually 116 + the LOG LEVEL. */
    private static final byte WRAPPER_MSG_LOG            = (byte)116;
    
    public static final int WRAPPER_CTRL_C_EVENT         = 200;
    public static final int WRAPPER_CTRL_CLOSE_EVENT     = 201;
    public static final int WRAPPER_CTRL_LOGOFF_EVENT    = 202;
    public static final int WRAPPER_CTRL_SHUTDOWN_EVENT  = 203;
    
    /** Log message at debug log level. */
    public static final int WRAPPER_LOG_LEVEL_DEBUG      = 1;
    /** Log message at info log level. */
    public static final int WRAPPER_LOG_LEVEL_INFO       = 2;
    /** Log message at status log level. */
    public static final int WRAPPER_LOG_LEVEL_STATUS     = 3;
    /** Log message at warn log level. */
    public static final int WRAPPER_LOG_LEVEL_WARN       = 4;
    /** Log message at error log level. */
    public static final int WRAPPER_LOG_LEVEL_ERROR      = 5;
    /** Log message at fatal log level. */
    public static final int WRAPPER_LOG_LEVEL_FATAL      = 6;
    
    private static boolean m_disposed = false;
    private static boolean m_started = false;
    private static WrapperManager m_instance = null;
    private static Thread m_hook = null;
    private static boolean m_hookTriggered = false;
    
    private static String[] m_args;
    private static int m_port    = DEFAULT_PORT;
    private static String m_key;
    private static int m_soTimeout = DEFAULT_SO_TIMEOUT;
    private static long m_cpuTimeout = DEFAULT_CPU_TIMEOUT;
    
    /** The number of threads to ignore when deciding when all application
     *   threads have completed. */
    private static int m_systemThreadCount;
    
    /** The lowest configured log level in the Wrapper's configuration.  This 
     *   is set to a high value by default to disable all logging if the
     *   Wrapper does not register its low level or is not present. */
    private static int m_lowLogLevel = WRAPPER_LOG_LEVEL_FATAL + 1;
    
    /** The maximum amount of time in ms to allow to pass without the JVM
     *   pinging the server before the JVM is terminated to allow a resynch. */
    private static int m_pingTimeout = 30000;
    
    /** Flag, set when the JVM is launched that is used to remember whether
     *   or not system signals are supposed to be ignored. */
    private static boolean m_ignoreSignals = false;
    
    /** Thread which processes all communications with the native code. */
    private static Thread m_commRunner;
    private static boolean m_commRunnerStarted = false;
    private static Thread m_eventRunner;
    private static long m_eventRunnerTime;
    
    private static WrapperListener m_listener;
    
    private static long m_lastPing;
    private static ServerSocket m_serverSocket;
    private static Socket m_socket;
    private static boolean m_shuttingDown = false;
    private static boolean m_appearHung = false;
    
    private static Method m_addShutdownHookMethod = null;
    private static Method m_removeShutdownHookMethod = null;
    
    private static boolean m_service = false;
    private static boolean m_debug = false;
    private static int m_jvmId = 0;
    private static boolean m_stopping = false;
    private static Thread m_stoppingThread;
    private static boolean m_libraryOK = false;
    private static byte[] m_commandBuffer = new byte[512];
    
    // message resources: eventually these will be split up
    private static ResourceManager m_res        = ResourceManager.getResourceManager();
    private static ResourceManager m_error      = m_res;
    private static ResourceManager m_warning    = m_res;
    private static ResourceManager m_info       = m_res;
    
    /*---------------------------------------------------------------
     * Class Initializer
     *-------------------------------------------------------------*/
    /**
     * When the WrapperManager class is first loaded, it attempts to load the
     *	configuration file specified using the 'wrapper.config' system property.
     *	When the JVM is launched from the Wrapper native code, the
     *	'wrapper.config' and 'wrapper.key' parameters are specified.
     *	The 'wrapper.key' parameter is a password which is used to verify that
     *	connections are only coming from the native Wrapper which launched the
     *	current JVM.
     */
    static
    {
        // Check for the debug flag
        m_debug = getBooleanProperty( "wrapper.debug" );
        
        // Check for the jvmID
        String jvmId = System.getProperty( "wrapper.jvmid" );
        if ( jvmId != null )
        {
            try
            {
                m_jvmId = Integer.parseInt( jvmId );
            }
            catch ( NumberFormatException e )
            {
                m_jvmId = 1;
            }
        }
        else
        {
            m_jvmId = 1;
        }
        if ( m_debug )
        {
            System.out.println( "Wrapper Manager: JVM #" + m_jvmId );
        }
        
        // Check to see if we should register a shutdown hook
        boolean disableShutdownHook =
            ( System.getProperty( "wrapper.disable_shutdown_hook" ) != null );
        
        // Locate the add and remove shutdown hook methods using reflection so
        //  that this class can be compiled on 1.2.x versions of java.
        try
        {
            m_addShutdownHookMethod =
                Runtime.class.getMethod( "addShutdownHook", new Class[] { Thread.class } );
            m_removeShutdownHookMethod =
                Runtime.class.getMethod( "removeShutdownHook", new Class[] { Thread.class } );
        }
        catch ( NoSuchMethodException e )
        {
            if ( m_debug )
            {
                System.out.println(
                    "Wrapper Manager: Shutdown hooks not supported by current JVM." );
            }
            m_addShutdownHookMethod = null;
            m_removeShutdownHookMethod = null;
            disableShutdownHook = true;
        }
        
        // If the shutdown hook is not disabled, then register it.
        if ( !disableShutdownHook )
        {
            if ( m_debug )
            {
                System.out.println( "Wrapper Manager: Registering shutdown hook" );
            }
            m_hook = new Thread( "Wrapper-Shutdown-Hook" )
            {
                /**
                 * Run the shutdown hook. (Triggered by the JVM when it is about to shutdown)
                 */
                public void run()
                {
                    if ( m_debug )
                    {
                        System.out.println( "Wrapper Manager: ShutdownHook started" );
                    }
                    
                    // Stop the Wrapper cleanly.
                    m_hookTriggered = true;
                    
                    // If we are not already stopping, then do so.
                    WrapperManager.stop( 0 );
                    
                    if ( m_debug )
                    {
                        System.out.println( "Wrapper Manager: ShutdownHook complete" );
                    }
                }
            };
            
            // Actually register the shutdown hook using reflection.
            try
            {
                m_addShutdownHookMethod.invoke( Runtime.getRuntime(), new Object[] { m_hook } );
            }
            catch ( IllegalAccessException e )
            {
                System.out.println( "Wrapper Manager: Unable to register shutdown hook: "
                    + e.getMessage() );
            }
            catch ( InvocationTargetException e )
            {
                System.out.println( "Wrapper Manager: Unable to register shutdown hook: "
                    + e.getMessage() );
            }
        }
        
        // A key is required for the wrapper to work correctly.  If it is not
        //  present, then assume that we are not being controlled by the native
        //  wrapper.
        if ( ( m_key = System.getProperty( "wrapper.key" ) ) == null )
        {
            if ( m_debug )
            {
                System.out.println( "Wrapper Manager: Not using wrapper.  (key not specified)" );
            }
            
            // The wrapper will not be used, so other values will not be used.
            m_port = 0;
            m_service = false;
            m_cpuTimeout = 31557600000L; // One Year.  Effectively never.
        }
        else
        {
            if ( m_debug )
            {
                System.out.println( "Wrapper Manager: Using wrapper" );
            }
            
            // Replace the System.in stream with one of our own to disable it.
            System.setIn( new WrapperInputStream() );
            
            // A port must have been specified.
            String sPort;
            if ( ( sPort = System.getProperty( "wrapper.port" ) ) == null )
            {
                String msg = m_res.format( "MISSING_PORT" );
                System.out.println( msg );
                throw new ExceptionInInitializerError( msg );
            }
            try
            {
                m_port = Integer.parseInt( sPort );
            }
            catch ( NumberFormatException e )
            {
                String msg = m_res.format( "BAD_PORT", sPort );
                System.out.println( msg );
                throw new ExceptionInInitializerError( msg );
            }
            
            // Check for the ignore signals flag
            m_ignoreSignals = getBooleanProperty( "wrapper.ignore_signals" );
            
            // If this is being run as a headless server, then a flag would have been set
            m_service = getBooleanProperty( "wrapper.service" );
            
            // Get the cpuTimeout
            String sCPUTimeout = System.getProperty( "wrapper.cpu.timeout" );
            if ( sCPUTimeout == null )
            {
                m_cpuTimeout = DEFAULT_CPU_TIMEOUT;
            }
            else
            {
                try
                {
                    m_cpuTimeout = Integer.parseInt( sCPUTimeout ) * 1000L;
                }
                catch ( NumberFormatException e )
                {
                    String msg = m_res.format( "BAD_CPU_TIMEOUT", sCPUTimeout );
                    System.out.println( msg );
                    throw new ExceptionInInitializerError( msg );
                }
            }
        }
        
        // Initialize the native code to trap system signals
        initializeNativeLibrary();
        
        // Start a thread which looks for control events sent to the
        //  process.  The thread is also used to keep track of whether
        //  the VM has been getting CPU to avoid invalid timeouts.
        m_eventRunnerTime = System.currentTimeMillis();
        m_eventRunner = new Thread( "Wrapper-Control-Event-Monitor" )
        {
            public void run()
            {
                while ( !m_shuttingDown )
                {
                    long now = System.currentTimeMillis();
                    long age = now - m_eventRunnerTime;
                    if ( age > m_cpuTimeout )
                    {
                        System.out.println( "JVM Process has not received any CPU time for "
                            + ( age / 1000 ) + " seconds.  Extending timeouts." );
                        
                        // Make sure that we don't get any ping timeouts in this event
                        m_lastPing = now;
                    }
                    m_eventRunnerTime = now;
                    
                    if ( m_libraryOK )
                    {
                        // Look for a control event in the wrapper library
                        int event = WrapperManager.nativeGetControlEvent();
                        if ( event != 0 )
                        {
                            WrapperManager.controlEvent( event );
                        }
                    }
                    
                    // Wait before checking for another control event.
                    try
                    {
                        Thread.sleep( 200 );
                    }
                    catch ( InterruptedException e )
                    {
                    }
                }
            }
        };
        m_eventRunner.setDaemon( true );
        m_eventRunner.start();
        
        // Resolve the system thread count based on the Java Version
        String fullVersion = System.getProperty( "java.fullversion" );
        if ( fullVersion == null )
        {
            fullVersion = System.getProperty( "java.runtime.version" ) + " "
                + System.getProperty( "java.vm.name" );
        }
        if ( fullVersion.indexOf( "JRockit" ) >= 0 )
        {
            // BEA Weblogic JRockit(R) Virtual Machine
            // This JVM handles its shutdown thread differently that IBM, Sun
            //  and Blackdown.
            m_systemThreadCount = 0;
        }
        else
        {
            // All other known JVMs have a system thread which is used by the
            //  system to trigger a JVM shutdown after all other threads have
            //  terminated.  This thread must be ignored when counting the
            //  remaining number of threads.
            m_systemThreadCount = 1;
        }
        
        if ( m_debug )
        {
            // Display more JVM infor right after the call initialization of the library.
            System.out.println( "Java Version   : " + fullVersion );
            System.out.println( "Java VM Vendor : " + System.getProperty( "java.vm.vendor" ) );
            System.out.println();
        }
        
        // Create the singleton
        m_instance = new WrapperManager();
    }

    /*---------------------------------------------------------------
     * Native Methods
     *-------------------------------------------------------------*/
    private static native void nativeInit( boolean debug );
    private static native int nativeGetControlEvent();
    private static native void nativeRequestThreadDump();
    private static native void accessViolationInner();
    
    /*---------------------------------------------------------------
     * Methods
     *-------------------------------------------------------------*/
    /**
     * Attempts to load the a native library file.
     *
     * @param name Name of the library to load.
     * @param file Name of the actual library file.
     *
     * @return True if the library was successfully loaded.
     */
    private static boolean loadNativeLibrary( String name, String file )
    {
        try
        {
            System.loadLibrary( name );
            
            if ( m_debug )
            {
                System.out.println( "Loaded native library: " + file );
            }
            
            return true;
        }
        catch ( UnsatisfiedLinkError e )
        {
            return false;
        }
    }
    
    /**
     * Searches for a file on a path.
     *
     * @param file File to look for.
     * @param path Path to be searched.
     *
     * @return Reference to thr file object if found, otherwise null.
     */
    private static File locateFileOnPath( String file, String path )
    {
        // A library path exists but the library was not found on it.
        String pathSep = System.getProperty( "path.separator" );
        
        // Search for the file on the library path to verify that it does not
        //  exist, it could be some other problem
        StringTokenizer st = new StringTokenizer( path, pathSep );
        while( st.hasMoreTokens() )
        {
            File libFile = new File( new File( st.nextToken() ), file );
            if ( libFile.exists() )
            {
                return libFile;
            }
        }
        
        return null;
    }
    
    /**
     * Searches for and then loads the native library.  This method will attempt
     *  locate the wrapper library using one of the following 3 naming 
     */
    private static void initializeNativeLibrary()
    {
        String libraryHead;
        String libraryTail;
        
        // Resolve the osname and osarch for the currect system.
        String osName = System.getProperty( "os.name" );
        if ( osName.indexOf( "Windows" ) >= 0 )
        {
            libraryHead = "";
            libraryTail = ".dll";
        }
        else if ( osName.indexOf( "Mac" ) >= 0 )
        {
            libraryHead = "lib";
            libraryTail = ".jnilib";
        }
        else
        {
            libraryHead = "lib";
            libraryTail = ".so";
        }
        
        String name = "wrapper";
        String file = libraryHead + name + libraryTail;
        
        // Attempt to load the native library using various names.
        if ( loadNativeLibrary( name, file ) )
        {
            m_libraryOK = true;
            
            // The library was loaded correctly, so initialize it.
            if ( m_debug )
            {
                System.out.println( "Calling native initialization method." );
            }
            nativeInit( m_debug );
        }
        else
        {
            m_libraryOK = false;
            
            // The library could not be loaded, so we want to give the user a useful
            //  clue as to why not.
            String libPath = System.getProperty( "java.library.path" );
            System.out.println();
            if ( libPath.equals( "" ) )
            {
                // No library path
                System.out.println(
                    "WARNING - Unable to load the Wrapper's native library because the" );
                System.out.println(
                    "          java.library.path was set to ''.  Please see the" );
                System.out.println(
                    "          documentation for the wrapper.java.library.path " );
                System.out.println(
                    "          configuration property.");
            }
            else
            {
                File libFile = locateFileOnPath( file, libPath );
                if ( libFile == null )
                {
                    // The library could not be located on the library path.
                    System.out.println(
                        "WARNING - Unable to load native library 'wrapper' because the" );
                    System.out.println(
                        "          file '" + file + "' could not be located in the following" );
                    System.out.println(
                        "          java.library.path:" );
                    String pathSep = System.getProperty( "path.separator" );
                    StringTokenizer st = new StringTokenizer( libPath, pathSep );
                    while ( st.hasMoreTokens() )
                    {
                        File pathElement = new File( st.nextToken() );
                        System.out.println( "            " + pathElement.getAbsolutePath() );
                    }
                    System.out.println(
                        "          Please see the documentation for the "
                        +          "wrapper.java.library.path" );
                    System.out.println(
                        "          configuration property." );
                }
                else
                {
                    // The library file was found but could not be loaded for some reason.
                    System.out.println(
                        "WARNING - Unable to load native library '" + file + "'.  The file" );
                    System.out.println(
                        "          is located on the path at the following location but could" );
                    System.out.println(
                        "          not be loaded:" );
                    System.out.println(
                        "            " + libFile.getAbsolutePath() );
                    System.out.println(
                        "          Please verify that the file is readable by the current user" );
                    System.out.println(
                        "          and that the file has not been corrupted in any way." );
                }
            }
            System.out.println( "          System signals will not be handled correctly." );
            System.out.println();
        }
    }
    
    /**
     * Returns true if the specified system property is set.
     *
     * @return True if the specified system property is set.
     */
    private static boolean getBooleanProperty( String name )
    {
        return ( System.getProperty( name ) != null );
    }
    
    /**
     * Obtain the current version of Wrapper.
     *
     * @return The version of the Wrapper.
     */
    public static String getVersion()
    {
        return WrapperInfo.getVersion();
    }
    
    /**
     * Obtain the build time of Wrapper.
     *
     * @return The time that the Wrapper was built.
     */
    public static String getBuildTime()
    {
        return WrapperInfo.getBuildTime();
    }
    
    /**
     * Returns the Id of the current JVM.  JVM Ids increment from 1 each time
     *  the wrapper restarts a new one.
     *
     * @return The Id of the current JVM.
     */
    public static int getJVMId()
    {
        return m_jvmId;
    }
    
    /**
     * Requests that the current JVM process request a thread dump.  This is
     *  the same as pressing CTRL-BREAK (under Windows) or CTRL-\ (under Unix)
     *  in the the console in which Java is running.  This method does nothing
     *  if the native library is not loaded.
     */
    public static void requestThreadDump()
    {
        if ( m_libraryOK )
        {
            nativeRequestThreadDump();
        }
        else
        {
            System.out.println( "  wrapper library not loaded." );
        }
    }
    
    /**
     * (Testing Method) Causes the WrapperManager to go into a state which makes the JVM appear
     *  to be hung when viewed from the native Wrapper code.  Does not have any effect when the
     *  JVM is not being controlled from the native Wrapper. Useful for testing the Wrapper 
     *  functions.
     */
    public static void appearHung()
    {
        System.out.println( "WARNING: Making JVM appear to be hung..." );
        m_appearHung = true;
    }
    
    /**
     * (Testing Method) Cause an access violation within the Java code.  Useful
     *  for testing the Wrapper functions.  This currently only crashes Sun
     *  JVMs and takes advantage of Bug #4369043 which does not exist in newer
     *  JVMs.  Use of the accessViolationNative() method is preferred.
     */
    public static void accessViolation()
    {
        System.out.println( "WARNING: Attempting to cause an access violation..." );
        
        try
        {
            Class c = Class.forName( "java.lang.String" );
            java.lang.reflect.Method m = c.getDeclaredMethod( null, null );
        }
        catch( NoSuchMethodException ex )
        {
            // Correctly did not find method.  access_violation attempt failed.  Not Sun JVM?
        }
        catch( Exception ex )
        {
            if ( ex instanceof NoSuchFieldException )
            {
                // Can't catch this in a catch because the compiler doesn't think it is being
                //  thrown.  But it is thrown on IBM jvms at least
                // Correctly did not find method.  access_violation attempt failed.  Not Sun JVM?
            }
            else
            {
                // Shouldn't get here.
                ex.printStackTrace();
            }
        }					
        
        System.out.println( "  Attempt to cause access violation failed.  JVM is still alive." );
    }

    /**
     * (Testing Method) Cause an access violation within native JNI code.  Useful for testing the
     *  Wrapper functions. This currently causes the access violation by attempting to write to 
     *  a null pointer.
     */
    public static void accessViolationNative()
    {
        System.out.println( "WARNING: Attempting to cause an access violation..." );
        if ( m_libraryOK )
        {
            accessViolationInner();
        
            System.out.println( "  Attempt to cause access violation failed.  "
                + "JVM is still alive." );
        }
        else
        {
            System.out.println( "  wrapper library not loaded." );
        }
    }
        
    /**
     * Returns true if the JVM was launched by the Wrapper application.  False
     *  if the JVM was launched manually without the Wrapper controlling it.
     *
     * @return True if the current JVM was launched by the Wrapper.
     */
    public static boolean isControlledByNativeWrapper()
    {
        return m_key != null;
    }
    
    /**
     * Returns true if the Wrapper was launched as an NT service on Windows or
     *  as a daemon process on UNIX platforms.  False if launched as a console.
     *  This can be useful if you wish to display a user interface when in
     *  Console mode.  On UNIX platforms, this is not as useful because an
     *  X display may not be visible even if launched in a console.
     *
     * @return True if the Wrapper is running as an NT service or daemon
     *         process.
     */
    public static boolean isLaunchedAsService()
    {
        return m_service;
    }
    
    /**
     * Returns true if the wrapper.debug property, or any of the logging
     *  channels are set to DEBUG in the wrapper configuration file.  Useful
     *  for deciding whether or not to output certain information to the
     *  console.
     *
     * @return True if the Wrapper is logging any Debug level output.
     */
    public static boolean isDebugEnabled()
    {
        return m_debug;
    }
    
    /**
     * Start the Java side of the Wrapper code running.  This will make it
     *  possible for the native side of the Wrapper to detect that the Java
     *  Wrapper is up and running.
     */
    public static synchronized void start( WrapperListener listener, String[] args )
    {
        System.out.println( "Wrapper (Version " + getVersion() + ")" );
        System.out.println();
        
        // Make sure that the class has not already been disposed.
        if ( m_disposed)
        {
            throw new IllegalStateException( "WrapperManager has already been disposed." );
        }
        
        if ( m_listener != null )
        {
            throw new IllegalStateException(
                "WrapperManager has already been started with a WrapperListener." );
        }
        if ( listener == null )
        {
            throw new IllegalStateException( "A WrapperListener must be specified." );
        }
        m_listener = listener;
        
        m_args = args;
        
        startRunner();
        
        // If this JVM is being controlled by a native wrapper, then we want to
        //  wait for the command to start.  However, if this is a standalone
        //  JVM, then we want to start now.
        if ( !isControlledByNativeWrapper() )
        {
            startInner();
        }
    }
    
    /**
     * Tells the native wrapper that the JVM wants to restart, then informs
     *	all listeners that the JVM is about to shutdown before killing the JVM.
     */
    public static void restart()
    {
        boolean stopping;
        synchronized(m_instance)
        {
            stopping = m_stopping;
            if ( !stopping )
            {
                m_stopping = true;
            }
        }
        
        if ( !stopping )
        {
            if ( !m_commRunnerStarted )
            {
                startRunner();
                // Wait to give the runner a chance to connect.
                try
                {
                    Thread.sleep(500);
                }
                catch ( InterruptedException e )
                {
                }
            }
            
            // Always send the stop command
            sendCommand( WRAPPER_MSG_RESTART, "restart" );
        }
        
        // Give the Wrapper a chance to register the stop command before stopping.
        // This avoids any errors thrown by the Wrapper because the JVM died before
        //  it was expected to.
        try
        {
            Thread.sleep( 1000 );
        }
        catch ( InterruptedException e )
        {
        }
        
        stopInner( 0 );
    }
    
    /**
     * Tells the native wrapper that the JVM wants to shut down, then informs
     *	all listeners that the JVM is about to shutdown before killing the JVM.
     *
     * @param exitCode The exit code that the Wrapper will return when it exits.
     */
    public static void stop( int exitCode )
    {
        stopCommon( exitCode, 1000 );
        
        stopInner( exitCode );
    }

    /**
     * Tells the native wrapper that the JVM wants to shut down and then
     *  promptly halts.  Be careful when using this method as an application
     *  will not be given a chance to shutdown cleanly.
     *
     * @param exitCode The exit code that the Wrapper will return when it exits.
     */
    public static void stopImmediate( int exitCode )
    {
        stopCommon( exitCode, 250 );
        
        signalStopped( exitCode );
        
        // Execute runtime.halt(0) using reflection so this class will
        //  compile on 1.2.x versions of Java.
        Method haltMethod;
        try
        {
            haltMethod =
                Runtime.class.getMethod( "halt", new Class[] { Integer.TYPE } );
        }
        catch ( NoSuchMethodException e )
        {
            System.out.println( "halt not supported by current JVM." );
            haltMethod = null;
        }
        
        if ( haltMethod != null )
        {
            Runtime runtime = Runtime.getRuntime();
            try
            {
                haltMethod.invoke( runtime, new Object[] { new Integer( 0 ) } );
            }
            catch ( IllegalAccessException e )
            {
                System.out.println(
                    "Unable to call runitme.halt: " + e.getMessage() );
            }
            catch ( InvocationTargetException e )
            {
                System.out.println(
                    "Unable to call runitme.halt: " + e.getMessage() );
            }
        }
        else
        {
            // Shutdown normally
            stopInner( exitCode );
        }
    }
    
    /**
     * Signal the native wrapper that the startup is progressing but that more
     *  time is needed.  The Wrapper will extend the startup timeout by the
     *  specified time.
     *
     * @param waitHint Additional time in milliseconds.
     */
    public static void signalStarting( int waitHint )
    {
        sendCommand( WRAPPER_MSG_START_PENDING, Integer.toString( waitHint ) );
    }

    /**
     * Signal the native wrapper that the shutdown is progressing but that more
     *  time is needed.  The Wrapper will extend the stop timeout by the
     *  specified time.
     *
     * @param waitHint Additional time in milliseconds.
     */
    public static void signalStopping( int waitHint )
    {
        m_stopping = true;
        sendCommand( WRAPPER_MSG_STOP_PENDING, Integer.toString( waitHint ) );
    }
    
    /**
     * This method should not normally be called by user code as it is called
     *  from within the stop and restart methods.  However certain applications
     *  which stop the JVM may need to call this method to let the wrapper code
     *  know that the shutdown was intentional.
     */
    public static void signalStopped( int exitCode )
    {
        m_stopping = true;
        sendCommand( WRAPPER_MSG_STOPPED, Integer.toString( exitCode ) );
        
        // Give the socket time to actuall send the packet to the Wrapper
        //  as this call is often immediately followed by a halt command.
        try
        {
            Thread.sleep( 250 );
        }
        catch ( InterruptedException e )
        {
            // Ignore.
        }
    }
    
    /**
     * Returns true if the ShutdownHook for the JVM has already been triggered.
     *  Some code needs to know whether or not the system is shutting down.
     *
     * @return True if the ShutdownHook for the JVM has already been triggered.
     */
    public static boolean hasShutdownHookBeenTriggered()
    {
        return m_hookTriggered;
    }
    
    /**
     * Requests that the Wrapper log a message at the specified log level.
     *  If the JVM is not being managed by the Wrapper then calls to this
     *  method will be ignored.  This method has been optimized to ignore
     *  messages at a log level which will not be logged given the current
     *  log levels of the Wrapper.
     * <p>
     * Log messages will currently by trimmed by the Wrapper at 4k (4096 bytes).
     * <p>
     * Because of differences in the way console output is collected and
     *  messages logged via this method, it is expected that interspersed
     *  console and log messages will not be in the correct order in the
     *  resulting log file.
     * <p>
     * This method was added to allow simple logging to the wrapper.log
     *  file.  This is not meant to be a full featured log file and should
     *  not be used as such.  Please look into a logging package for most
     *  application logging.
     *
     * @param logLevel The level to log the message at can be one of
     *                 WRAPPER_LOG_LEVEL_DEBUG, WRAPPER_LOG_LEVEL_INFO,
     *                 WRAPPER_LOG_LEVEL_STATUS, WRAPPER_LOG_LEVEL_WARN,
     *                 WRAPPER_LOG_LEVEL_ERROR, or WRAPPER_LOG_LEVEL_FATAL.
     * @param message The message to be logged.
     */
    public static void log( int logLevel, String message )
    {
        // Make sure that the logLevel is valid to avoid problems with the
        //  command sent to the server.
        
        if ( ( logLevel < WRAPPER_LOG_LEVEL_DEBUG ) || ( logLevel > WRAPPER_LOG_LEVEL_FATAL ) )
        {
            throw new IllegalArgumentException( "The specified logLevel is not valid." );
        }
        if ( message == null )
        {
            throw new IllegalArgumentException( "The message parameter can not be null." );
        }
        
        if ( m_lowLogLevel <= logLevel )
        {
            sendCommand( (byte)( WRAPPER_MSG_LOG + logLevel ), message );
        }
    }
    
    /*---------------------------------------------------------------
     * Constructors
     *-------------------------------------------------------------*/
    /** 
     * This class can not be instantiated.
     */
    private WrapperManager()
    {
    }
    
    /*---------------------------------------------------------------
     * Private methods
     *-------------------------------------------------------------*/
    /**
     * Executed code common to the stop and stopImmediate methods.
     */
    private static void stopCommon( int exitCode, int delay )
    {
        boolean stopping;
        synchronized( m_instance )
        {
            stopping = m_stopping;
            if ( !stopping )
            {
                m_stopping = true;
            }
        }
        
        if ( !stopping )
        {
            if ( !m_commRunnerStarted )
            {
                startRunner();
                // Wait to give the runner a chance to connect.
                try
                {
                    Thread.sleep( 500 );
                }
                catch ( InterruptedException e )
                {
                }
            }
            
            // Always send the stop command
            sendCommand( WRAPPER_MSG_STOP, Integer.toString( exitCode ) );
        }
        
        // Give the Wrapper a chance to register the stop command before stopping.
        // This avoids any errors thrown by the Wrapper because the JVM died before
        //  it was expected to.
        try
        {
            Thread.sleep( delay );
        }
        catch ( InterruptedException e )
        {
        }
    }
    
    /**
     * Dispose of all resources used by the WrapperManager.  Closes the server
     *	socket which is used to listen for events from the 
     */
    private static void dispose()
    {
        synchronized( m_instance.getClass() )
        {
            m_disposed = true;
            
            // Close the open socket if it exists.
            closeSocket();
            
            // Give the Connection Thread a chance to stop itself.
            try
            {
                Thread.sleep( 500 );
            }
            catch ( InterruptedException e )
            {
            }
        }
    }
    
    /**
     * Informs the listener that it should start.
     */
    private static void startInner()
    {
        // Set the thread priority back to normal so that any spawned threads
        //	will use the normal priority
        int oldPriority = Thread.currentThread().getPriority();
        Thread.currentThread().setPriority( Thread.NORM_PRIORITY );
        
        if ( m_debug )
        {
            System.out.println( "calling listener.start()" );
        }
        if ( m_listener != null )
        {
            // This is user code, so don't trust it.
            try
            {
                Integer result = m_listener.start( m_args );
                if ( result != null )
                {
                    int exitCode = result.intValue();
                    // Signal the native code.
                    stop( exitCode );
                    // Won't make it here.
                    return;
                }
            }
            catch ( Throwable t )
            {
                System.out.println( "Error in WrapperListener.start callback.  " + t );
                t.printStackTrace();
                // Kill the JVM, but don't tell the wrapper that we want to stop.
                //  This may be a problem with this instantiation only.
                stopInner( 1 );
                // Won't make it here.
                return;
            }
        }
        if ( m_debug )
        {
            System.out.println( "returned from listener.start()" );
        }
        
        // Crank the priority back up.
        Thread.currentThread().setPriority( oldPriority );
        
        // Signal that the application has started.
        signalStarted();
    }
    
    private static void shutdownJVM( int exitCode )
    {
        // Do not call System.exit if this is the ShutdownHook
        if ( Thread.currentThread() == m_hook )
        {
            // Signal that the application has stopped and the JVM is about to shutdown.
            signalStopped( 0 );
            
            // Dispose the wrapper.  (If the hook runs, it will do this.)
            dispose();
            
            // This is the shutdown hook, so fall through because things are
            //  already shutting down.
        }
        else
        {
            //  We do not want the ShutdownHook to execute, so unregister it before calling exit
            if ( m_hook != null )
            {
                // Remove the shutdown hook using reflection.
                try
                {
                    m_removeShutdownHookMethod.invoke(
                        Runtime.getRuntime(), new Object[] { m_hook } );
                }
                catch ( IllegalAccessException e )
                {
                    System.out.println( "Wrapper Manager: Unable to unregister shutdown hook: "
                        + e.getMessage() );
                }
                catch ( InvocationTargetException e )
                {
                    System.out.println( "Wrapper Manager: Unable to unregister shutdown hook: "
                        + e.getMessage() );
                }
                m_hook = null;
            }
            // Signal that the application has stopped and the JVM is about to shutdown.
            signalStopped( 0 );
            
            // Dispose the wrapper.  (If the hook runs, it will do this.)
            dispose();
            
            if ( m_debug )
            {
                System.out.println( "calling System.exit(" + exitCode + ")" );
            }
            System.exit( exitCode );
        }
    }
    
    /**
     * Informs the listener that the JVM will be shut down.
     */
    private static void stopInner( int exitCode )
    {
        boolean block;
        synchronized( m_instance )
        {
            // Always set the stopping flag.
            m_stopping = true;
            
            // Only one thread can be allowed to continue.
            if ( m_stoppingThread == null )
            {
                m_stoppingThread = Thread.currentThread();
                block = false;
            }
            else
            {
                if ( Thread.currentThread() == m_stoppingThread )
                {
                    throw new IllegalStateException(
                        "WrapperManager.stop() can not be called recursively." );
                }
                
                if ( Thread.currentThread() == m_hook )
                {
                    // The hook should be allowed to fall through.
                    return;
                }
                block = true;
            }
        }
        
        if ( block )
        {
            if ( m_debug )
            {
                System.out.println( "Thread, " + Thread.currentThread().getName()
                    + ", waiting for the JVM to exit." );
            }
            
            // This thread needs to be put into an infinite loop until the JVM exits.
            //  This thread can not be allowed to return to the caller, but another
            //  thread is already responsible for shutting down the JVM, so this
            //  one can do nothing but wait.
            while( true )
            {
                try
                {
                    Thread.sleep( 100 );
                }
                catch ( InterruptedException e )
                {
                }
            }
        }
        
        if ( m_debug )
        {
            System.out.println( "Thread, " + Thread.currentThread().getName()
                + ", handling the shutdown process." );
        }
        
        // Only stop the listener if the app has been started.
        int code = exitCode;
        if ( m_started )
        {
            // Set the thread priority back to normal so that any spawned threads
            //	will use the normal priority
            int oldPriority = Thread.currentThread().getPriority();
            Thread.currentThread().setPriority( Thread.NORM_PRIORITY );
            
            if ( m_debug )
            {
                System.out.println( "calling listener.stop()" );
            }
            if ( m_listener != null )
            {
                // This is user code, so don't trust it.
                try
                {
                    code = m_listener.stop( code );
                }
                catch ( Throwable t )
                {
                    System.out.println( "Error in WrapperListener.stop callback.  " + t );
                    t.printStackTrace();
                }
            }
            if ( m_debug )
            {
                System.out.println( "returned from listener.stop()" );
            }
            
            // Crank the priority back up.
            Thread.currentThread().setPriority( oldPriority );
        }

        shutdownJVM( code );
    }
    
    private static void signalStarted()
    {
        sendCommand( WRAPPER_MSG_STARTED, "" );
        m_started = true;
    }
    
    /**
     * Called by the native code when a control event is trapped by native code.
     * Can have the values: WRAPPER_CTRL_C_EVENT, WRAPPER_CTRL_CLOSE_EVENT, 
     *    WRAPPER_CTRL_LOGOFF_EVENT, or WRAPPER_CTRL_SHUTDOWN_EVENT
     * Calls 
     */
    private static void controlEvent( int event )
    {
        String eventName;
        boolean ignore;
        switch( event )
        {
        case WRAPPER_CTRL_C_EVENT:
            eventName = "WRAPPER_CTRL_C_EVENT";
            ignore = m_ignoreSignals;
            break;
        case WRAPPER_CTRL_CLOSE_EVENT:
            eventName = "WRAPPER_CTRL_CLOSE_EVENT";
            ignore = m_ignoreSignals;
            break;
        case WRAPPER_CTRL_LOGOFF_EVENT:
            eventName = "WRAPPER_CTRL_LOGOFF_EVENT";
            ignore = false;
            break;
        case WRAPPER_CTRL_SHUTDOWN_EVENT:
            eventName = "WRAPPER_CTRL_SHUTDOWN_EVENT";
            ignore = false;
            break;
        default:
            eventName = "Unexpected event: " + event;
            ignore = false;
            break;
        }
        
        if ( ignore )
        {
            if ( m_debug )
            {
                System.out.println( "Ignoring control event(" + eventName + ")" );
            }
        }
        else
        {
            if ( m_debug )
            {
                System.out.println( "Processing control event(" + eventName + ")" );
            }
            
            // This is user code, so don't trust it.
            if ( m_listener != null )
            {
                try
                {
                    m_listener.controlEvent( event );
                }
                catch ( Throwable t )
                {
                    System.out.println( "Error in WrapperListener.controlEvent callback.  " + t );
                    t.printStackTrace();
                }
            }
        }
    }

    private static synchronized Socket openSocket()
    {
        if ( m_debug )
        {
            System.out.println( "Open socket to wrapper..." );
        }

        InetAddress iNetAddress;
        try
        {
            iNetAddress = InetAddress.getByName( "127.0.0.1" );
        }
        catch ( UnknownHostException e )
        {
            // This is pretty fatal.
            System.out.println( e );
            stop( 1 );
            return null; //please the compiler
        }
        
        try
        {
            m_socket = new Socket( iNetAddress, m_port );
            if ( m_debug )
            {
                System.out.println( "Opened Socket" );
            }
        }
        catch ( BindException e )
        {
            System.out.println( "Failed to bind to the Wrapper port." );
            System.out.println( e );
            // This is fatal because the port was bad.
            System.out.println( "Exiting JVM..." );
            stopImmediate( 1 );
        }
        catch ( ConnectException e )
        {
            System.out.println( "Failed to connect to the Wrapper." );
            System.out.println( e );
            // This is fatal because there is nobody listening.
            System.out.println( "Exiting JVM..." );
            stopImmediate( 1 );
        }
        catch ( IOException e )
        {
            System.out.println( e );
            m_socket = null;
            return null;
        }
        try
        {
            // Turn on the TCP_NODELAY flag.  This is very important for speed!!
            m_socket.setTcpNoDelay( true );
            
            // Set the SO_TIMEOUT for the socket (max block time)
            if ( m_soTimeout > 0 )
            {
                m_socket.setSoTimeout( m_soTimeout );
            }
        }
        catch ( IOException e )
        {
            System.out.println( e );
        }
        
        // Send the key back to the wrapper so that the wrapper can feel safe
        //  that it is talking to the correct JVM
        sendCommand( WRAPPER_MSG_KEY, m_key );
            
        return m_socket;
    }
    
    private static synchronized void closeSocket()
    {
        if ( m_socket != null )
        {
            if ( m_debug )
            {
                System.out.println( "Closing socket." );
            }
            
            try
            {
                m_socket.close();
            }
            catch ( IOException e )
            {
            }
            finally
            {
                m_socket = null;
            }
        }
    }
    
    private static String getPacketCodeName(byte code)
    {
        String name;
    
        switch (code)
        {
        case WRAPPER_MSG_START:
            name ="START";
            break;
    
        case WRAPPER_MSG_STOP:
            name ="STOP";
            break;
    
        case WRAPPER_MSG_RESTART:
            name ="RESTART";
            break;
    
        case WRAPPER_MSG_PING:
            name ="PING";
            break;
    
        case WRAPPER_MSG_STOP_PENDING:
            name ="STOP_PENDING";
            break;
    
        case WRAPPER_MSG_START_PENDING:
            name ="START_PENDING";
            break;
    
        case WRAPPER_MSG_STARTED:
            name ="STARTED";
            break;
    
        case WRAPPER_MSG_STOPPED:
            name ="STOPPED";
            break;
    
        case WRAPPER_MSG_KEY:
            name ="KEY";
            break;
    
        case WRAPPER_MSG_BADKEY:
            name ="BADKEY";
            break;
    
        case WRAPPER_MSG_LOW_LOG_LEVEL:
            name ="LOW_LOG_LEVEL";
            break;
    
        case WRAPPER_MSG_PING_TIMEOUT:
            name ="PING_TIMEOUT";
            break;
    
        case WRAPPER_MSG_LOG + WRAPPER_LOG_LEVEL_DEBUG:
            name ="LOG(DEBUG)";
            break;
    
        case WRAPPER_MSG_LOG + WRAPPER_LOG_LEVEL_INFO:
            name ="LOG(INFO)";
            break;
    
        case WRAPPER_MSG_LOG + WRAPPER_LOG_LEVEL_STATUS:
            name ="LOG(STATUS)";
            break;
    
        case WRAPPER_MSG_LOG + WRAPPER_LOG_LEVEL_WARN:
            name ="LOG(WARN)";
            break;
    
        case WRAPPER_MSG_LOG + WRAPPER_LOG_LEVEL_ERROR:
            name ="LOG(ERROR)";
            break;
    
        case WRAPPER_MSG_LOG + WRAPPER_LOG_LEVEL_FATAL:
            name ="LOG(FATAL)";
            break;
    
        default:
            name = "UNKNOWN(" + code + ")";
            break;
        }
        return name;
    }
    
    private static synchronized void sendCommand( byte code, String message )
    {
        if ( m_debug )
        {
            System.out.println( "Send a packet " + getPacketCodeName( code ) + " : " + message );
        }
        if ( m_appearHung )
        {
            // The WrapperManager is attempting to make the JVM appear hung, so do nothing
        }
        else
        {
            // Make a copy of the reference to make this more thread safe.
            Socket socket = m_socket;
            if ( socket == null && isControlledByNativeWrapper() && ( !m_stopping ) )
            {
                // The socket is not currently open, try opening it.
                socket = openSocket();
            }
            
            if ( ( code == WRAPPER_MSG_START_PENDING ) || ( code == WRAPPER_MSG_STARTED ) )
            {
                // Set the last ping time so that the startup process does not time out
                //  thinking that the JVM has not received a Ping for too long.
                m_lastPing = System.currentTimeMillis();
            }
            
            // If the socket is open, then send the command, otherwise just throw it away.
            if ( socket != null )
            {
                try
                {
                    // It is possible that a logged message is quite large.  Expand the size
                    // of the command buffer if necessary so that it can be included.  This
                    //  means that the command buffer will be the size of the largest message.
                    byte[] messageBytes = message.getBytes();
                    if ( m_commandBuffer.length < messageBytes.length + 2 )
                    {
                        m_commandBuffer = new byte[messageBytes.length + 2];
                    }
                    
                    // Writing the bytes one by one was sometimes causing the first byte to be lost.
                    // Try to work around this problem by creating a buffer and sending the whole lot
                    // at once.
                    m_commandBuffer[0] = code;
                    System.arraycopy( messageBytes, 0, m_commandBuffer, 1, messageBytes.length );
                    int len = messageBytes.length + 2;
                    m_commandBuffer[len - 1] = 0;
                    
                    OutputStream os = socket.getOutputStream();
                    os.write( m_commandBuffer, 0, len );
                    os.flush();
                }
                catch ( IOException e )
                {
                    System.out.println( e );
                    e.printStackTrace();
                    closeSocket();
                }
            }
        }
    }
    
    /**
     * Loop reading packets from the native side of the Wrapper until the 
     *  connection is closed or the WrapperManager class is disposed.
     *  Each packet consists of a packet code followed by a null terminated
     *  string up to 256 characters in length.  If the entire packet has not
     *  yet been received, then it must not be read until the complete packet
     *  has arived.
     */
    private static void handleSocket()
    {
        byte[] buffer = new byte[256];
        try
        {
            if ( m_debug )
            {
                System.out.println( "handleSocket(" + m_socket + ")" );
            }
            DataInputStream is = new DataInputStream( m_socket.getInputStream() );
            while ( !m_disposed )
            {
                try
                {
                    // A Packet code must exist.
                    byte code = is.readByte();
                    
                    // Always read from the buffer until a null '\0' is encountered.  But only
                    //  place the first 256 characters into the buffer.
                    byte b;
                    int i = 0;
                    do
                    {
                        b = is.readByte();
                        if ( ( b != 0 ) && ( i < 256 ) )
                        {
                            buffer[i] = b;
                            i++;
                        }
                    }
                    while ( b != 0 );
                    
                    String msg = new String( buffer, 0, i );
                    
                    if ( m_appearHung )
                    {
                        // The WrapperManager is attempting to make the JVM appear hung,
                        //   so ignore all incoming requests
                    }
                    else
                    {
                        if ( m_debug )
                        {
                            System.out.println( "Received a packet " + getPacketCodeName( code )
                                + " : " + msg );
                        }
                        
                        // Ok, we got a packet.  Do something with it.
                        switch( code )
                        {
                        case WRAPPER_MSG_START:
                            startInner();
                            break;
                            
                        case WRAPPER_MSG_STOP:
                            // Don't do anything if we are already stopping
                            if ( !m_stopping )
                            {
                                stopInner( 0 );
                                // Should never get back here.
                            }
                            break;
                            
                        case WRAPPER_MSG_PING:
                            m_lastPing = System.currentTimeMillis();
                            sendCommand( WRAPPER_MSG_PING, "ok" );
                            break;
                            
                        case WRAPPER_MSG_BADKEY:
                            // The key sent to the wrapper was incorrect.  We need to shutdown.
                            System.out.println(
                                "Authorization key rejected by Wrapper.  Exiting JVM." );
                            closeSocket();
                            stopInner( 1 );
                            break;
                            
                        case WRAPPER_MSG_LOW_LOG_LEVEL:
                            try
                            {
                                m_lowLogLevel = Integer.parseInt( msg );
                                if ( m_debug )
                                {
                                    System.out.println( "Wrapper Manager: LowLogLevel from Wrapper "
                                        + "is " + m_lowLogLevel );
                                }
                            }
                            catch ( NumberFormatException e )
                            {
                                System.out.println( "Encountered an Illegal LowLogLevel from the "
                                    + "Wrapper: " + msg );
                            }
                            break;
                            
                        case WRAPPER_MSG_PING_TIMEOUT:
                            try
                            {
                                m_pingTimeout = Integer.parseInt( msg ) * 1000;
                                if ( m_debug )
                                {
                                    System.out.println( "Wrapper Manager: PingTimeout from Wrapper "
                                        + "is " + m_pingTimeout );
                                }
                            }
                            catch ( NumberFormatException e )
                            {
                                System.out.println( "Encountered an Illegal PingTimeout from the "
                                    + "Wrapper: " + msg );
                            }
                            
                            // Make sure that the so timeout is longer than the ping timeout
                            if ( m_soTimeout < m_pingTimeout )
                            {
                                m_socket.setSoTimeout( m_pingTimeout );
                            }
                            
                            break;
                            
                        default:
                            // Ignore unknown messages
                            System.out.println( "Wrapper code received an unknown packet type: "
                                + code );
                            break;
                        }
                    }
                }
                catch ( InterruptedIOException e )
                {
                    long now = System.currentTimeMillis();
                    
                    // Unless the JVM is shutting dowm we want to show warning messages and maybe exit.
                    if ( ( m_started ) && ( !m_stopping ) )
                    {
                        if ( m_debug )
                        {
                            System.out.println( "Read Timed out. (Last Ping was "
                                + ( now - m_lastPing ) + " milliseconds ago)" );
                        }
                        
                        if ( !m_appearHung )
                        {
                            long lastPingAge = now - m_lastPing;
                            long eventRunnerAge = now - m_eventRunnerTime;
                            
                            // We may have timed out because the system was extremely busy or
                            //  suspended.  Only restart due to a lack of ping events if the
                            //  event thread has been running.
                            if ( eventRunnerAge < 10000 )
                            {
                                // Only perform ping timeout checks if ping timeouts are enabled.
                                if ( m_pingTimeout > 0 )
                                {
                                    // How long has it been since we received the last ping
                                    //  from the Wrapper?
                                    if ( lastPingAge > m_pingTimeout + 90000 )
                                    {
                                        // It has been more than the ping timeout + 90 seconds,
                                        //  so just give up and kill the JVM
                                        System.out.println(
                                            "Wrapper Manager: JVM did not exit.  Give up." );
                                        System.exit(1);
                                    }
                                    else if ( lastPingAge > m_pingTimeout )
                                    {
                                        // It has been more than the ping timeout since the
                                        //  JVM was last pinged.  Ask to be stopped (and restarted).
                                        System.out.println(
                                            "Wrapper Manager: The Wrapper code did not ping the "
                                            + "JVM for " + (lastPingAge / 1000) + " seconds.  "
                                            + "Quit and let the Wrapper resynch.");
                                        
                                        // Don't do anything if we are already stopping
                                        if ( !m_stopping )
                                        {
                                            stopInner( 1 );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Check to see if all non-daemon threads have exited.
                if ( m_started )
                {
                    checkThreads();
                }
            }
            return;

        }
        catch ( SocketException e )
        {
            if ( m_debug )
            {
                if ( m_socket == null )
                {
                    // This error happens if the socket is closed while reading:
                    // java.net.SocketException: Descriptor not a socket: JVM_recv in socket
                    //                           input stream read
                }
                else
                {
                    System.out.println( "Closed socket: " + e );
                }
            }
            return;
        }
        catch ( IOException e )
        {
            // This means that the connection was closed.  Allow this to return.
            //System.out.println( e );
            //e.printStackTrace();
            return;
        }
    }
    
    /**
     * Returns a count of all non-daemon threads in the JVM, starting with the top
     *  thread group.
     *
     * @return Number of non-daemon threads.
     */
    protected static int getNonDaemonThreadCount()
    {
        // Locate the top thread group.
        ThreadGroup topGroup = Thread.currentThread().getThreadGroup();
        while ( topGroup.getParent() != null )
        {
            topGroup = topGroup.getParent();
        }
        
        // Get a list of all threads.  Use an array that is twice the total number of
        //  threads as the number of running threads may be increasing as this runs.
        Thread[] threads = new Thread[topGroup.activeCount() * 2];
        topGroup.enumerate( threads, true );
        
        // Only count any non daemon threads which are 
        //  still alive other than this thread.
        int liveCount = 0;
        for ( int i = 0; i < threads.length; i++ )
        {
            /*
            if ( threads[i] != null )
            {
                System.out.println( "Check " + threads[i].getName() + " daemon="
                + threads[i].isDaemon() + " alive=" + threads[i].isAlive() );
            }
            */
            if ( ( threads[i] != null ) && ( threads[i].isAlive() && ( !threads[i].isDaemon() ) ) )
            {
                // Do not count this thread or the wrapper connection thread
                if ( ( Thread.currentThread() != threads[i] ) && ( threads[i] != m_commRunner ) )
                {
                    // Non-Daemon living thread
                    liveCount++;
                    //System.out.println( "  -> Non-Daemon" );
                }
            }
        }
        //System.out.println( "  => liveCount = " + liveCount );
        
        return liveCount;
    }
    
    /**
     * With a normal Java application, the JVM will exit when all non-daemon
     *  threads have completed.  This does not work correctly with the wrapper
     *  because the connection thread is not a daemon.  It would also cause
     *  problems because the wrapper would not know whether the exit had been
     *  intentional or not.  This method takes care of making sure that the
     *  JVM exits when it is supposed to and makes sure that the Wrapper is
     *  propperly informed.
     */
    private static void checkThreads()
    {
        int liveCount = getNonDaemonThreadCount();
        
        // Depending on the JVM, there will always be one (or zero) non-daemon thread alive.
        //  This thread is either the main thread which has not yet completed, or a thread
        //  launched by java when the main thread completes whose job is to wait around for
        //  all other non-daemon threads to complete.  We are overriding that thread here.
        if ( liveCount <= m_systemThreadCount )
        {
            if ( m_debug )
            {
                System.out.println( "All non-daemon threads have stopped.  Exiting." );
            }
            
            // Exit normally
            WrapperManager.stop( 0 );
            // Will not get here.
        }
        else
        {
            // There are daemons running, let the JVM continue to run.
        }
    }
    
    private static void startRunner()
    {
        if ( isControlledByNativeWrapper() )
        {
            if ( m_commRunner == null )
            {
                // Create and launch a new thread to manage this connection
                m_commRunner = new Thread( m_instance, WRAPPER_CONNECTION_THREAD_NAME );
                // This thread can not be a daemon or the JVM will quit immediately
                m_commRunner.start();
            }
        }
    }
    
    /*---------------------------------------------------------------
     * Runnable Methods
     *-------------------------------------------------------------*/
    public void run()
    {
        // Make sure that no other threads call this method.
        if ( Thread.currentThread() != m_commRunner )
        {
            throw new IllegalStateException(
                "Only the comm runner thread is allowed to call this method." );
        }
        
        m_commRunnerStarted = true;
        
        // This thread needs to have a very high priority so that it never
        //	gets put behind other threads.
        Thread.currentThread().setPriority( Thread.MAX_PRIORITY );
        
        // Initialize the last ping time.
        m_lastPing = System.currentTimeMillis();
        
        boolean gotPortOnce = false;
        while ( !m_disposed )
        {
            try
            {
                try
                {
                    openSocket();
                    if ( m_socket != null )
                    {
                        handleSocket();
                    }
                    else
                    {
                        // Failed, so wait for just a moment
                        try
                        {
                            Thread.sleep( 10 );
                        }
                        catch ( InterruptedException e )
                        {
                        }
                    }
                }
                finally
                {
                    // Always close the socket here.
                    closeSocket();
                }
            }
            catch ( ThreadDeath td )
            {
                System.out.println( m_warning.format( "SERVER_DAEMON_KILLED" ) );
            }
            catch ( Throwable t )
            {
                if ( !m_shuttingDown )
                {
                    // Show a stack trace here because this is fairly critical
                    System.out.println( m_error.format( "SERVER_DAEMON_DIED" ) );
                    t.printStackTrace();
                }
            }
        }
        if ( m_debug )
        {
            System.out.println( m_info.format( "SERVER_DAEMON_SHUT_DOWN" ) );
        }
    }
    
    /*---------------------------------------------------------------
     * Inner Classes
     *-------------------------------------------------------------*/
    /**
     * When the JVM is being controlled by the Wrapper, stdin can not be used
     *  as it is undefined.  This class makes it possible to provide the user
     *  application with a descriptive error message if System.in is accessed.
     */
    private static class WrapperInputStream
        extends InputStream
    {
        /**
         * This method will always throw an IOException as the read method is
         *  not valid.
         */
        public int read()
            throws IOException
        {
            throw new IOException( "System.in can not be used when the JVM is being "
                + "controlled by the Java Service Manager." );
        }
    }
}

