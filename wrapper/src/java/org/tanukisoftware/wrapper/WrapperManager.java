package org.tanukisoftware.wrapper;

/*
 * Copyright (c) 1999, 2018 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html
 * 
 * 
 * Portions of the Software have been derived from source code
 * developed by Silver Egg Technology under the following license:
 * 
 * Copyright (c) 2001 Silver Egg Technology
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without 
 * restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sub-license, and/or 
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following 
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 */

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintStream;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.BindException;
import java.net.ConnectException;
import java.net.InetAddress;
import java.net.MalformedURLException;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.net.URL;
import java.security.CodeSource;
import java.security.AccessControlException;
import java.security.AccessController;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.PrivilegedAction;
import java.security.ProtectionDomain;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.StringTokenizer;
import java.util.ResourceBundle;
import java.util.Locale;
import java.util.MissingResourceException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.tanukisoftware.wrapper.event.WrapperControlEvent;
import org.tanukisoftware.wrapper.event.WrapperEvent;
import org.tanukisoftware.wrapper.event.WrapperEventListener;
import org.tanukisoftware.wrapper.event.WrapperLogFileChangedEvent;
import org.tanukisoftware.wrapper.event.WrapperSecondInvocationEvent;
import org.tanukisoftware.wrapper.event.WrapperPingEvent;
import org.tanukisoftware.wrapper.event.WrapperServiceActionEvent;
import org.tanukisoftware.wrapper.event.WrapperServiceControlEvent;
import org.tanukisoftware.wrapper.event.WrapperServicePauseEvent;
import org.tanukisoftware.wrapper.event.WrapperServiceResumeEvent;
import org.tanukisoftware.wrapper.event.WrapperTickEvent;
import org.tanukisoftware.wrapper.security.WrapperEventPermission;
import org.tanukisoftware.wrapper.security.WrapperPermission;
import org.tanukisoftware.wrapper.security.WrapperServicePermission;
import org.tanukisoftware.wrapper.security.WrapperUserEventPermission;

/**
 * Handles all communication with the native portion of the Wrapper code.
 *  Communication takes place either over a Pipe, or Socket.  In either
 *  case, the Wrapper code will initializate its end of the communication
 *  and then launch Java in a separate process.
 *
 * In the case of a socket, the Wrapper will set up a server socket which
 *  the Java code is expected to open a socket to on startup.  When the
 *  server socket is created, a port will be chosen depending on what is
 *  available to the system.  This port will then be passed to the Java
 *  process as property named "wrapper.port".  
 *
 * In the case of a pipe, the Wrapper will set up a pair of named ports
 *  and then pass in a property named "wrapper.backend=pipe".  The Wrapper
 *  and JVM will communicate via the pipes.
 *
 * For security reasons, the native code will only allow connections from
 *  localhost and will expect to receive the key specified in a property
 *  named "wrapper.key".
 *
 * When the backend connection is a socket, it does not have any timeout
 *  set by default.  For testing purposes it is possible to set a timeout
 *  in seconds using the following system property when launching the JVM.
 *  This will only be noticeable if it is shorter than the ping interval,
 *  and debug output is enabled:
 * -Dwrapper.backend.so_timeout=300
 *
 * This class is implemented as a singleton class.
 *
 * Generate JNI Headers with the following command in the build/classes
 *  directory:
 *    javah -jni -classpath ./ org.tanukisoftware.wrapper.WrapperManager
 *
 * @author Tanuki Software Development Team &lt;support@tanukisoftware.com&gt;
 */
public final class WrapperManager
    implements Runnable
{
    private static final String WRAPPER_CONNECTION_THREAD_NAME = "Wrapper-Connection";
    
    private static final int DEFAULT_PORT                = 15003;
    private static final int DEFAULT_CPU_TIMEOUT         = 10000;
    
    /** The number of milliseconds in one tick.  Used for internal system
     *   time independent time keeping. */
    private static final int TICK_MS                     = 100;
    private static final int TIMER_FAST_THRESHOLD        = 2 * 24 * 3600 * 1000 / TICK_MS; // 2 days.
    private static final int TIMER_SLOW_THRESHOLD        = 2 * 24 * 3600 * 1000 / TICK_MS; // 2 days.
    

    /**
     * Backend server can be of 3 types: socket IPv4, socket IPv6 or pipe
     */
    private static final int BACKEND_TYPE_UNKNOWN        = 0;
    private static final int BACKEND_TYPE_SOCKET_V4      = 0x01;
    private static final int BACKEND_TYPE_SOCKET_V6      = 0x02;
    private static final int BACKEND_TYPE_PIPE           = 0x04;
   
    
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
    private static final byte WRAPPER_MSG_PING_TIMEOUT   = (byte)113; /* No longer used. */
    private static final byte WRAPPER_MSG_SERVICE_CONTROL_CODE = (byte)114;
    private static final byte WRAPPER_MSG_PROPERTIES     = (byte)115;
    /** Log commands are actually 116 + the LOG LEVEL. */
    private static final byte WRAPPER_MSG_LOG            = (byte)116;
    private static final byte WRAPPER_MSG_CHILD_LAUNCH   = (byte)132;
    private static final byte WRAPPER_MSG_CHILD_TERM     = (byte)133;
    private static final byte WRAPPER_MSG_LOGFILE        = (byte)134;
    private static final byte WRAPPER_MSG_CHECK_DEADLOCK = (byte)135;
    private static final byte WRAPPER_MSG_DEADLOCK       = (byte)136;
    private static final byte WRAPPER_MSG_APPEAR_ORPHAN  = (byte)137; /* No longer used. */
    private static final byte WRAPPER_MSG_PAUSE          = (byte)138;
    private static final byte WRAPPER_MSG_RESUME         = (byte)139;
    private static final byte WRAPPER_MSG_GC             = (byte)140;
    private static final byte WRAPPER_MSG_FIRE_USER_EVENT= (byte)141;
    private static final byte WRAPPER_MSG_SECOND_INVOCATION_EVENT = (byte)142;
    
    /** Received when the user presses CTRL-C in the console on Windows or UNIX platforms. */
    public static final int WRAPPER_CTRL_C_EVENT         = 200;
    
    /** Received when the user clicks on the close button of a Console on Windows. */
    public static final int WRAPPER_CTRL_CLOSE_EVENT     = 201;
    
    /** Received when the user logs off of a Windows system. */
    public static final int WRAPPER_CTRL_LOGOFF_EVENT    = 202;
    
    /** Received when a Windows system is shutting down. */
    public static final int WRAPPER_CTRL_SHUTDOWN_EVENT  = 203;
    
    /** Received when a SIG TERM is received on a UNIX system. */
    public static final int WRAPPER_CTRL_TERM_EVENT      = 204;
    
    /** Received when a SIG HUP is received on a UNIX system. */
    public static final int WRAPPER_CTRL_HUP_EVENT       = 205;
    
    /** Received when a SIG USR1 is received on a UNIX system. */
    public static final int WRAPPER_CTRL_USR1_EVENT      = 206;
    
    /** Received when a SIG USR2 is received on a UNIX system. */
    public static final int WRAPPER_CTRL_USR2_EVENT      = 207;
    
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
    /** Log message at advice log level. */
    public static final int WRAPPER_LOG_LEVEL_ADVICE     = 7;
    /** Log message at notice log level. */
    public static final int WRAPPER_LOG_LEVEL_NOTICE     = 8;
    
    /** Service Control code which can be sent to start a service. */
    public static final int SERVICE_CONTROL_CODE_START       = 0x10000;
    
    /** Service Control code which can be sent or received to stop a service. */
    public static final int SERVICE_CONTROL_CODE_STOP        = 1;
    
    /** Service Control code which can be sent to pause a service. */
    public static final int SERVICE_CONTROL_CODE_PAUSE       = 2;
    
    /** Service Control code which can be sent to resume a paused service. */
    public static final int SERVICE_CONTROL_CODE_CONTINUE    = 3;
    
    /** Service Control code which can be sent to or received interrogate the status of a service. */
    public static final int SERVICE_CONTROL_CODE_INTERROGATE = 4;
    
    /** Service Control code which can be received when the system is shutting down. */
    public static final int SERVICE_CONTROL_CODE_SHUTDOWN    = 5;
    
    /** Service Control code which is received when the system being suspended. */
    public static final int SERVICE_CONTROL_CODE_POWEREVENT_QUERYSUSPEND       = 0x0D00;
    
    /** Service Control code which is received when permission to suspend the
     *   computer was denied by a process.  Support for this event was removed
     *   from the Windows OS starting with Vista.*/
    public static final int SERVICE_CONTROL_CODE_POWEREVENT_QUERYSUSPENDFAILED = 0x0D02;
    
    /** Service Control code which is received when the computer is about to
     *   enter a suspended state. */
    public static final int SERVICE_CONTROL_CODE_POWEREVENT_SUSPEND            = 0x0D04;
    
    /** Service Control code which is received when the system has resumed
     *   operation. This event can indicate that some or all applications did
     *   not receive a SERVICE_CONTROL_CODE_POWEREVENT_SUSPEND event.
     *   Support for this event was removed from the Windows OS starting with
     *   Vista.  See SERVICE_CONTROL_CODE_POWEREVENT_RESUMEAUTOMATIC. */
    public static final int SERVICE_CONTROL_CODE_POWEREVENT_RESUMECRITICAL     = 0x0D06;
    
    /** Service Control code which is received when the system has resumed
     *   operation after being suspended. */
    public static final int SERVICE_CONTROL_CODE_POWEREVENT_RESUMESUSPEND      = 0x0D07;
    
    /** Service Control code which is received when the battery power is low.
     *   Support for this event was removed from the Windows OS starting with
     *   Vista.  See SERVICE_CONTROL_CODE_POWEREVENT_POWERSTATUSCHANGE. */
    public static final int SERVICE_CONTROL_CODE_POWEREVENT_BATTERYLOW         = 0x0D09;
    
    /** Service Control code which is received when there is a change in the
     *   power status of the computer, such as a switch from battery power to
     *   A/C. The system also broadcasts this event when remaining battery
     *   power slips below the threshold specified by the user or if the
     *   battery power changes by a specified percentage. */
    public static final int SERVICE_CONTROL_CODE_POWEREVENT_POWERSTATUSCHANGE  = 0x0D0A;
    
    /** Service Control code which is received when the APM BIOS has signaled
     *   an APM OEM event.  Support for this event was removed from the Windows
     *   OS starting with Vista. */
    public static final int SERVICE_CONTROL_CODE_POWEREVENT_OEMEVENT           = 0x0D0B;
    
    /** Service Control code which is received when the computer has woken up
     *   automatically to handle an event. */
    public static final int SERVICE_CONTROL_CODE_POWEREVENT_RESUMEAUTOMATIC    = 0x0D12;
    
    /** Reference to the original value of System.out. */
    private static PrintStream m_out;
    
    /** Reference to the original value of System.err. */
    private static PrintStream m_err;
    
    /** Info level log channel */
    private static WrapperPrintStream m_outInfo;
    
    /** Error level log channel */
    private static WrapperPrintStream m_outError;
    
    /** Debug level log channel */
    private static WrapperPrintStream m_outDebug;
    
    /** Stores the resolved OS used in naming of native library. */
    private static String m_os;
    
    /** Stores the resolved Architecture used in naming of native library. */
    private static String m_arch;
    
    /** Flag to remember whether or not this is Windows. */
    private static boolean m_windows = false;
    
    /** Flag to remember whether or not this is MacOSX. */
    private static boolean m_macosx = false;
    
    /** Flag to remember whether or not this is AIX. */
    private static boolean m_aix = false;
    
    /** Flag to remember whether or not this is z/OS. */
    private static boolean m_zos = false;
    
    /** Flag that will be set to true once a SecurityManager has been detected and tested. */
    private static boolean m_securityManagerChecked = false;
    
    private static boolean m_disposed = false;
    
    /** The starting flag is set when the Application has been asked to start. */
    private static boolean m_starting = false;
    
    /** The started flag is set when the Application has completed its startup. */
    private static boolean m_started = false;
    private static WrapperManager m_instance = null;
    private static Thread m_hook = null;
    private static boolean m_hookTriggered = false;
    private static boolean m_hookRemoveFailed = false;
    
    /* Flag which records when the shutdownJVM method has completed. */
    private static boolean m_shutdownJVMComplete = false;
    
    /** Map which stores shutdown locks for each thread. */
    private static Map m_shutdownLockMap = new HashMap();
    
    /** Tracks the total number of outstanding shutdown locks. */
    private static int m_shutdownLocks = 0;
    
    /** Tracks the number of threads which are attempting to launch child processes. */
    private static int m_runningExecs = 0;
    
    private static String[] m_args;
    private static int m_backendType = BACKEND_TYPE_UNKNOWN;
    private static boolean m_backendConnected = false;
    private static OutputStream m_backendOS = null;
    private static InputStream m_backendIS = null;
    private static int m_port    = DEFAULT_PORT;
    private static int m_jvmPort;
    private static int m_jvmPortMin;
    private static int m_jvmPortMax;
    private static String m_wrapperPortAddress = null;
    private static String m_key;
    private static int m_soTimeout = -1;
    private static long m_cpuTimeout = DEFAULT_CPU_TIMEOUT;
    
    /** Tick count when the start method completed. */
    private static int m_startedTicks;
    
    /** The lowest configured log level in the Wrapper's configuration.  This 
     *   is set to a high value by default to disable all logging if the
     *   Wrapper does not register its low level or is not present. */
    private static int m_lowLogLevel = WRAPPER_LOG_LEVEL_NOTICE + 1;
    
    /** Flag, set when the JVM is launched that is used to remember whether
     *   or not system signals are supposed to be ignored. */
    private static boolean m_ignoreSignals = false;
    
    /** Flag which controls whether the Wrapper process is expected to close the
     *   connection after the STARTED packet is sent. */
    private static boolean m_detachStarted = false;
    
    /** Thread which processes all communications with the native code. */
    private static Thread m_commRunner;
    private static boolean m_commRunnerStarted = false;
    private static Thread m_eventRunner;
    private static int m_eventRunnerTicks;
    private static Thread m_startupRunner;
    
    /** True if the system time should be used for internal timeouts. */
    private static boolean m_useSystemTime;
    
    /** The threashold of how many ticks the timer can be fast before a
     *   warning is displayed. */
    private static int m_timerFastThreshold;
    
    /** The threashold of how many ticks the timer can be slow before a
     *   warning is displayed. */
    private static int m_timerSlowThreshold;
    
    /** Flag which controls whether or not the test methods are disabled. */ 
    private static boolean m_disableTests;
    
    /** Flag which controls whether or not the WrapperListener.stop method will
     *   be called on shutdown when the WrapperListener.start method has not
     *   returned or returned an exit code. */
    private static boolean m_listenerForceStop;
    
    /**
     * Bit depth of the currently running JVM.  Will be 32 or 64.
     *  A 64-bit JVM means that the system is also 64-bit, but a 32-bit JVM
     *  can be run either on a 32 or 64-bit system.
     */
    private static int m_jvmBits;
    
    /** An integer which stores the number of ticks since the
     *   JVM was launched.  Using an int rather than a long allows the value
     *   to be used without requiring any synchronization.  This is only
     *   used if the m_useSystemTime flag is false. */
    private static volatile int m_ticks;
    
    private static WrapperListener m_listener;
    
    private static int m_lastPingTicks;
    private static Socket m_backendSocket;
    private static boolean m_appearHung = false;
    private static int m_slowSeconds = 0;
    
    private static boolean m_ignoreUserLogoffs = false;
    
    private static boolean m_service = false;
    private static boolean m_debug = false;
    private static boolean m_logFinalizer = false;
    private static int m_jvmId = 0;
    /** Flag set when any thread initiates a stop or restart. */
    private static boolean m_stoppingInit = false;
    /** Flag set when the thread that will be in charge of actually stopping has been fixed. */
    private static boolean m_stopping = false;
    /** Thread that is in charge of stopping. */
    private static Thread m_stoppingThread;
    /** Flag set when the the thread which is charge of stopping has completed.  The Wrapper JNI code could then become invalid at any time and should no longer be trusted. */
    private static boolean m_stopped = false;
    /** If set then this message will be sent as a STOP message as soon as we connect to the Wrapper. */
    private static String m_pendingStopMessage = null;
    private static int m_exitCode;
    private static boolean m_libraryOK = false;
    private static boolean m_libraryLoaded = false;
    private static boolean m_libraryVersionOk = false;
    private static boolean m_wrapperVersionOk = false;
    private static byte[] m_commandBuffer = new byte[512];
    private static File m_logFile = null;
    
    /** The contents of the wrapper configuration. */
    private static WrapperProperties m_properties;
    
    /** List of registered WrapperEventListeners and their registered masks. */
    private static List m_wrapperEventListenerMaskList = new ArrayList();
    
    /** Array of registered WrapperEventListeners and their registered masks.
     *   Should not be referenced directly.  Access by calling
     *   getWrapperEventListenerMasks(). */
    private static WrapperEventListenerMask[] m_wrapperEventListenerMasks = null;
    
    /** Flag used to tell whether or not WrapperCoreEvents should be produced. */
    private static boolean m_produceCoreEvents = false;
    
    // message resources: eventually these will be split up
    private static WrapperResources m_res;
    
    /** Set on startup to show whether or not a Professional edition library is being used.
     *   Overriding this makes no difference as the underlying library also contains these checks.
     *   Caching this value makes it possible to reduce the number of JNI calls and avoid confusing
     *   errors on shutdown. */
    private static boolean m_professionalEdition;
    
    /** Set on startup to show whether or not a Standard edition library is being used.
     *   Overriding this makes no difference as the underlying library also contains these checks.
     *   Caching this value makes it possible to reduce the number of JNI calls and avoid confusing
     *   errors on shutdown. */
    private static boolean m_standardEdition;
    
    /**
     * Returns the WrapperResources object which is used to manage all resources for
     *  the Java Service Wrapper.
     *
     * @return the Wrapper's resouces.
     */
    public static WrapperResources getRes()
    {
        return m_res;
    }
    
    /*---------------------------------------------------------------
     * Class Initializer
     *-------------------------------------------------------------*/
    /**
     * When the WrapperManager class is first loaded, it attempts to load the
     *  configuration file specified using the 'wrapper.config' system property.
     *  When the JVM is launched from the Wrapper native code, the
     *  'wrapper.config' and 'wrapper.key' parameters are specified.
     *  The 'wrapper.key' parameter is a password which is used to verify that
     *  connections are only coming from the native Wrapper which launched the
     *  current JVM.
     */
    static
    {
        // The wraper.jar must be given AllPermissions if a security manager
        //  has been configured.  This is not a problem if one of the standard
        //  Wrapper helper classes is used to launch the JVM.
        // If however a custom WrapperListener is being implemented then this
        //  class will most likely be loaded by code that is neither part of
        //  the system, nor part of the Wrapper code base.  To avoid having
        //  to also give those classes AllPermissions as well, we do all of
        //  initialization in a Privileged block.  This means that the code
        //  only requires that the wrapper.jar has been given the required
        //  permissions.
        AccessController.doPrivileged(
            new PrivilegedAction() {
                public Object run() {
                    privilegedClassInit();
                    return null;
                }
            }
        );
    }
    
    
    /**
     * Logs information about the package of the specified class.
     *
     * @param clazz Class to log.
     */
    private static void logPackageInfo( Class clazz )
    {
        if ( m_debug )
        {
            Package pkg = WrapperManager.class.getPackage();
            if ( pkg == null )
            {
                m_outDebug.println( getRes().getString( "{0} package not found.", clazz.getName() ) );
            }
            else
            {
                m_outDebug.println( getRes().getString( "{0} package information:", clazz.getName() ) );
                m_outDebug.println( getRes().getString( "  Implementation Title: {0}", pkg.getImplementationTitle() ) );
                m_outDebug.println( getRes().getString( "  Implementation Vendor: {0}", pkg.getImplementationVendor() ) );
                m_outDebug.println( getRes().getString( "  Implementation Version: {0}", pkg.getImplementationVersion() ) );
                m_outDebug.println( getRes().getString( "  Is Sealed?: {0}", pkg.isSealed() ? getRes().getString( "True" ) : getRes().getString( "False" ) ) );
            }
            
            ProtectionDomain proDom = clazz.getProtectionDomain();
            m_outDebug.println( getRes().getString( "{0} protection domain:", clazz.getName() ) );
            CodeSource codeSource = proDom.getCodeSource();
            URL jarLocation = codeSource.getLocation();
            m_outDebug.println( getRes().getString( "  Location: {0}", jarLocation ) );
            
            // Try reading in the full jar so we can calculate its size and MD5 hash.
            try
            {
                InputStream is = jarLocation.openStream();
                try
                {
                    int jarSize = 0;
                    MessageDigest md = MessageDigest.getInstance( "MD5" );
                    int data;
                    while ( ( data = is.read() ) >= 0 )
                    {
                        jarSize++;
                        md.update( (byte)( data & 0xff ) );
                    }
                    m_outDebug.println( getRes().getString( "    Size: {0}", new Integer( jarSize ) ) );
                    
                    byte[] bytes = md.digest();
                    StringBuffer sb = new StringBuffer();
                    for ( int i = 0; i < bytes.length; i++ )
                    {
                        String val = Integer.toString( bytes[i] & 0xff, 16 ).toLowerCase();
                        if ( val.length() == 1 )
                        {
                            sb.append( "0" );
                        }
                        sb.append( val );
                    }
                    m_outDebug.println( getRes().getString( "    MD5: {0}" , sb ) );
                }
                finally
                {
                    is.close();
                }
            }
            catch ( NoSuchAlgorithmException e )
            {
                m_outDebug.println( getRes().getString( "    Unable to calculate MD5: {0}", e ) );
            }
            catch ( IOException e )
            {
                m_outDebug.println( getRes().getString( "    Unable to access location: {0}", e ) );
            }
        }
    }
    
    /**
     * The body of the static initializer is moved into a seperate method so
     *  it can be run as a PrivilegedAction.
     */
    private static void privilegedClassInit()
    {
        // Store references to the original System.out and System.err
        //  PrintStreams.  The WrapperManager will always output to the
        //  original streams so its output will always end up in the
        //  wrapper.log file even if the end user code redirects the
        //  output to another log file.
        // This is also important to be protect the Wrapper's functionality
        //  from the case where the user PrintStream enters a deadlock state.
        m_out = System.out;
        m_err = System.err;

        // Set up some log channels
        boolean streamsSet = false;
        if ( "true".equals( System.getProperty( "wrapper.use_sun_encoding" ) ) ) {
            String sunStdoutEncoding = System.getProperty( "sun.stdout.encoding" );
            if ( ( sunStdoutEncoding != null ) && !sunStdoutEncoding.equals( System.getProperty( "file.encoding" ) ) ) {
                /* We need to create the stream using the same encoding as the one used for stdout, else this will lead to encoding issues. */
                try
                {
                    m_outInfo = new WrapperPrintStream( m_out, false, sunStdoutEncoding, "WrapperManager: " );
                    m_outError = new WrapperPrintStream( m_out, false, sunStdoutEncoding, "WrapperManager Error: " );
                    m_outDebug = new WrapperPrintStream( m_out, false, sunStdoutEncoding, "WrapperManager Debug: " );
                    streamsSet = true;
                }
                catch ( UnsupportedEncodingException e )
                {
                    /* This should not happen because we always make sure the encoding exists before launching a JVM.
                     *  If any of the above streams failed, we want to fall back to streams that use the same encoding. */
                    System.out.println( "Failed to set the encoding '" + sunStdoutEncoding + "' when creating a WrapperPrintStream.\n Make sure the value of sun.stdout.encoding is correct." );
                }
            }
        }
        if ( !streamsSet )
        {
            m_outInfo = new WrapperPrintStream( m_out, "WrapperManager: " );
            m_outError = new WrapperPrintStream( m_out, "WrapperManager Error: " );
            m_outDebug = new WrapperPrintStream( m_out, "WrapperManager Debug: " );
        }
        
        // Always create an empty properties object in case we are not running
        //  in the Wrapper or the properties are never sent.
        m_properties = new WrapperProperties();
        m_properties.lock();
        
        // Create a dummy resources file so initial localization will work until the native library is loaded.
        m_res = new WrapperResources();
        
        // This must be done before attempting to access any System Properties
        //  as that could cause a SecurityException if it is too strict.
        checkSecurityManager();
        
        // Check for the debug flag
        m_debug = WrapperSystemPropertyUtil.getBooleanProperty( "wrapper.debug", false );
        
        if ( m_debug )
        {
            // This message is logged when localization is not yet initialized.
            m_outDebug.println( "WrapperManager class initialized by thread: " + Thread.currentThread().getName() + "   Using classloader: " + WrapperManager.class.getClassLoader().toString() );
        }
        
        // The copyright banner was moved into the wrapper binary.  In order to
        //  aid in the debugging of user integrations, some kind of a known message
        //  needs to be displayed on startup so it is obvious whether or not the
        //  WrapperManager class is being initialized.
        // This message is logged when localization is not yet initialized.
        m_outInfo.println( "Initializing..." );
        
        // We need to get the key before the version can be verified.
        m_key = System.getProperty( "wrapper.key" );
        
        // Store the log finalizer flag.
        m_logFinalizer = WrapperSystemPropertyUtil.getBooleanProperty( "wrapper.logFinalizers", false );
        
        // Check for the jvmID
        m_jvmId = WrapperSystemPropertyUtil.getIntProperty( "wrapper.jvmid", 1 );
        if ( m_debug )
        {
            // This message is logged when localization is not yet initialized.
            m_outDebug.println( "JVM #" + m_jvmId );
        }
        
        // Decide whether this is a 32 or 64 bit version of Java.
        m_jvmBits = WrapperSystemPropertyUtil.getIntProperty( "sun.arch.data.model", -1 );
        if ( m_jvmBits == -1 )
        {
            m_jvmBits = WrapperSystemPropertyUtil.getIntProperty( "com.ibm.vm.bitmode", -1 );
        }
        if ( m_debug )
        {
            if ( m_jvmBits > 0 )
            {
                // This message is logged when localization is not yet initialized.
                m_outDebug.println( "Running a " + m_jvmBits + "-bit JVM." );
            }
            else
            {
                // This message is logged when localization is not yet initialized.
                m_outDebug.println( "The bit depth of this JVM could not be determined." );
            }
        }
        
        // Log information about the Wrapper's package.
        logPackageInfo( WrapperManager.class );
        
        // Get the detachStarted flag.
        m_detachStarted = WrapperSystemPropertyUtil.getBooleanProperty( "wrapper.detachStarted", false );
        
        // Initialize the timerTicks to a very high value.  This means that we will
        // always encounter the first rollover (200 * WRAPPER_MS / 1000) seconds
        // after the Wrapper the starts, which means the rollover will be well
        // tested.
        m_ticks = Integer.MAX_VALUE - 200;
        
        m_useSystemTime = WrapperSystemPropertyUtil.getBooleanProperty(
            "wrapper.use_system_time", false );
        m_timerFastThreshold = WrapperSystemPropertyUtil.getIntProperty(
            "wrapper.timer_fast_threshold", TIMER_FAST_THRESHOLD ) * 1000 / TICK_MS;
        m_timerSlowThreshold = WrapperSystemPropertyUtil.getIntProperty(
            "wrapper.timer_slow_threshold", TIMER_SLOW_THRESHOLD ) * 1000 / TICK_MS;
        
        // Check to see if we should disable test methods
        m_disableTests = WrapperSystemPropertyUtil.getBooleanProperty(
            "wrapper.disable_tests", false );
        
        // Check to see if we should register a shutdown hook
        boolean disableShutdownHook = WrapperSystemPropertyUtil.getBooleanProperty(
            "wrapper.disable_shutdown_hook", false );
        
        // Check to see if the listener stop method should always be called.
        m_listenerForceStop = WrapperSystemPropertyUtil.getBooleanProperty(
            "wrapper.listener.force_stop", false );
        
        // Make it possible for a user to set the SO_TIMEOUT of the backend.  Mainly for testing.
        m_soTimeout = WrapperSystemPropertyUtil.getIntProperty( "wrapper.backend.so_timeout", -1 ) * 1000;
        
        // If the shutdown hook is not disabled, then register it.
        if ( !disableShutdownHook )
        {
            if ( m_debug )
            {
                // This message is logged when localization is not yet initialized.
                m_outDebug.println( "Registering shutdown hook" );
            }
            m_hook = new Thread( "Wrapper-Shutdown-Hook" )
            {
                /**
                 * Run the shutdown hook. (Triggered by the JVM when it is about to shutdown)
                 */
                public void run()
                {
                    // Stop the Wrapper cleanly.
                    m_hookTriggered = true;
                    
                    if ( m_debug )
                    {
                        m_outDebug.println( getRes().getString( "ShutdownHook started" ) );
                    }
                    
                    // Let the startup thread die since the shutdown hook is running.
                    m_startupRunner = null;
                    
                    // If we are not already stopping, then do so.
                    WrapperManager.stop( 0 );
                    
                    if ( m_debug )
                    {
                        m_outDebug.println( getRes().getString( "ShutdownHook complete" ) );
                    }
                    
                    // Really all done.
                    if ( m_debug )
                    {
                        m_outDebug.println( getRes().getString( "WrapperManager stopped due to {0}", getRes().getString( "Shutdown Hook" ) ) );
                    }
                    m_stopped = true;
                }
            };
            
            // Register the shutdown hook.
            Runtime.getRuntime().addShutdownHook( m_hook );
        }
        
        // Initialize connection values.
        m_backendType = BACKEND_TYPE_UNKNOWN;
        m_port = 0;
        m_jvmPort = 0;
        m_jvmPortMin = 0;
        m_jvmPortMax = 0;
        
        // A key is required for the wrapper to work correctly.  If it is not
        //  present, then assume that we are not being controlled by the native
        //  wrapper.
        if ( m_key == null )
        {
            if ( m_debug )
            {
                // This message is logged when localization is not yet initialized.
                m_outDebug.println( "Not using the Wrapper.  (key not specified)" );
            }
            
            // The wrapper will not be used, so other values will not be used.
            m_service = false;
            m_cpuTimeout = 31557600000L; // One Year.  Effectively never.
        }
        else
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "Using the Wrapper" ) );
            }
            
            if ( WrapperSystemPropertyUtil.getBooleanProperty( "wrapper.disable_console_input", false ) )
            {
                // Replace the System.in stream with one of our own to disable it.
                System.setIn( new WrapperInputStream() );
            }
           
            // Decide what the backend connection type is
            String backendType = WrapperSystemPropertyUtil.getStringProperty( "wrapper.backend", "SOCKET" );
            
            if ( backendType.equalsIgnoreCase( "PIPE" ) )
            {
                // Pipe based communication
                m_backendType = BACKEND_TYPE_PIPE;
            }
            else
            {
                // Socket based communication
                if (backendType.equalsIgnoreCase( "SOCKET" ) ) {
                    m_backendType = BACKEND_TYPE_SOCKET_V4;
                } else{
                    m_backendType = BACKEND_TYPE_SOCKET_V6;
                }
                
                // An address may not have been specified.
                if ( ( m_wrapperPortAddress = System.getProperty( "wrapper.port.address" ) ) == null )
                {
                    /* Use the default loopback */
                    if (m_backendType == BACKEND_TYPE_SOCKET_V4) {
                        m_wrapperPortAddress = "127.0.0.1";
                    } else {
                        m_wrapperPortAddress = "::1";
                    }
                }

                String sPort;
                if ( ( sPort = System.getProperty( "wrapper.port" ) ) == null )
                {
                    // This message is logged when localization is not yet initialized.
                    String msg = "The 'wrapper.port' system property was not set.";
                    m_outError.println( msg );
                    throw new ExceptionInInitializerError( msg );
                }
                try
                {
                    m_port = Integer.parseInt( sPort );
                }
                catch ( NumberFormatException e )
                {
                    // This message is logged when localization is not yet initialized.
                    String msg = "'" + sPort + "' is not a valid value for 'wrapper.port'.";
                    m_outError.println( msg );
                    throw new ExceptionInInitializerError( msg );
                }
                
                m_jvmPort =
                    WrapperSystemPropertyUtil.getIntProperty( "wrapper.jvm.port", -1 );
                m_jvmPortMin =
                    WrapperSystemPropertyUtil.getIntProperty( "wrapper.jvm.port.min", 31000 );
                m_jvmPortMax =
                    WrapperSystemPropertyUtil.getIntProperty( "wrapper.jvm.port.max", 31999 );
            }
            
            // Check for the ignore signals flag
            m_ignoreSignals = WrapperSystemPropertyUtil.getBooleanProperty(
                "wrapper.ignore_signals", false );
            
            // If this is being run as a headless server, then a flag would have been set
            m_service = WrapperSystemPropertyUtil.getBooleanProperty( "wrapper.service", false );
            
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
                    // This message is logged when localization is not yet initialized.
                    String msg = "'" + sCPUTimeout + "' is not a valid value for 'wrapper.cpu.timeout'.";
                    m_outError.println( msg );
                    throw new ExceptionInInitializerError( msg );
                }
            }
        }

        // Register the MBeans if configured to do so.
        if ( WrapperSystemPropertyUtil.getBooleanProperty(
            WrapperManager.class.getName() + ".mbean", true ) )
        {
            registerMBean( new org.tanukisoftware.wrapper.jmx.WrapperManager(),
                "org.tanukisoftware.wrapper:type=WrapperManager" );
        }
        if ( WrapperSystemPropertyUtil.getBooleanProperty(
            WrapperManager.class.getName() + ".mbean.testing", false ) )
        {
            registerMBean( new org.tanukisoftware.wrapper.jmx.WrapperManagerTesting(),
                "org.tanukisoftware.wrapper:type=WrapperManagerTesting" );
        }
        
        // Initialize the native code to trap system signals
        initializeNativeLibrary();
        
        if ( isNativeLibraryOk() )
        {
            // Get the PID of the current JVM from the native library.  Be careful as the method
            //  will not exist if the library is old.
            try
            {
                System.setProperty( "wrapper.java.pid", Integer.toString( nativeGetJavaPID() ) );
            }
            catch ( Throwable e )
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "Call to nativeGetJavaPID() failed: {0}", e ) );
                }
            }
        }
        
        // Start a thread which looks for control events sent to the
        //  process.  The thread is also used to keep track of whether
        //  the VM has been getting CPU to avoid invalid timeouts and
        //  to maintain the number of ticks since the JVM was launched.
        m_eventRunnerTicks = getTicks();
        m_eventRunner = new Thread( "Wrapper-Control-Event-Monitor" )
        {
            public void run()
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "Control event monitor thread started." ) );
                }
                
                try
                {
                    WrapperTickEventImpl tickEvent = new WrapperTickEventImpl();
                    int lastTickOffset = 0;
                    boolean first = true;
                    boolean stoppingLogged = false;
                    
                    // This loop should not exit until the WrapperManager is completely stopped.
                    while ( !m_stopped )
                    {
                        int offsetDiff;
                        if ( !m_useSystemTime )
                        {
                            // Get the tick count based on the system time.
                            int sysTicks = getSystemTicks();
                            
                            // Increment the tick counter by 1. This loop takes just slightly
                            //  more than the length of a "tick" but it is a good enough
                            //  approximation for our purposes.  The accuracy of the tick length
                            //  falls sharply when the system is under heavly load, but this
                            //  has the desired effect as the Wrapper is also much less likely
                            //  to encounter false timeouts due to the heavy load.
                            // The ticks field is volatile and a single integer, so it is not
                            //  necessary to synchronize this.
                            // When the ticks count reaches the upper limit of the int range,
                            //  it is ok to just let it overflow and wrap.
                            m_ticks++;
                            
                            // Calculate the offset between the two tick counts.
                            //  This will always work due to overflow.
                            int tickOffset = sysTicks - m_ticks;
                            
                            // The number we really want is the difference between this tickOffset
                            //  and the previous one.
                            offsetDiff = tickOffset - lastTickOffset;
                            
                            if ( first )
                            {
                                first = false;
                            }
                            else
                            {
                                if ( offsetDiff > m_timerSlowThreshold )
                                {
                                    m_outInfo.println( getRes().getString( "The timer fell behind the system clock by {0} ms." , 
                                        new Integer( offsetDiff * TICK_MS ) ) );
                                }
                                else if ( offsetDiff < - m_timerFastThreshold )
                                {
                                    m_outInfo.println( getRes().getString( "The system clock fell behind the timer by {0} ms." ,
                                            new Integer( -1 * offsetDiff * TICK_MS ) ) );
                                }
                            }
                            
                            // Store this tick offset for the net time through the loop.
                            lastTickOffset = tickOffset;
                        }
                        else
                        {
                            offsetDiff = 0;
                        }
                        
                        //m_outInfo.println( "  UNIX Time: "
                        //  + Long.toHexString( System.currentTimeMillis() )
                        //  + ", ticks=" + Integer.toHexString( getTicks() ) + ", sysTicks="
                        //  + Integer.toHexString( getSystemTicks() ) );
                            
                        // Attempt to detect whether or not we are being starved of CPU.
                        //  This will only have any effect if the m_useSystemTime flag is
                        //  set.
                        // The tick timer will always result in an age of exactly one
                        //  because it is incremented each time through this loop.
                        int nowTicks = getTicks();
                        long age = getTickAge( m_eventRunnerTicks, nowTicks );
                        if ( ( m_cpuTimeout > 0 ) && ( age > m_cpuTimeout ) )
                        {
                            m_outInfo.println( getRes().getString( "JVM Process has not received any CPU time for {0} seconds.  Extending timeouts." ,
                                new Long( age / 1000 ) ) );
                            
                            // Make sure that we don't get any ping timeouts in this event
                            m_lastPingTicks = nowTicks;
                        }
                        m_eventRunnerTicks = nowTicks;
                        
                        // If there are any listeners interrested in core events then fire
                        //  off a tick event.
                        if ( m_produceCoreEvents )
                        {
                            tickEvent.m_ticks = nowTicks;
                            tickEvent.m_tickOffset = offsetDiff;
                            fireWrapperEvent( tickEvent );
                        }
                        
                        if ( isNativeLibraryOk() )
                        {
                            // To avoid the JVM shutting down while we are in the middle of a JNI call, 
                            if ( !isShuttingDown() )
                            {
                                // Look for control events in the wrapper library.
                                //  There may be more than one.
                                int event = 0;
                                do
                                {
                                    event = WrapperManager.nativeGetControlEvent();
                                    if ( event != 0 )
                                    {
                                        WrapperManager.controlEvent( event );
                                    }
                                }
                                while ( event != 0 );
                            }
                            else if ( !stoppingLogged )
                            {
                                stoppingLogged = true;
                                if ( m_debug )
                                {
                                    m_outDebug.println( getRes().getString( "Stopped checking for control events." ) );
                                }
                            }
                        }
                        
                        // Wait before checking for another control event.
                        try
                        {
                            Thread.sleep( TICK_MS );
                        }
                        catch ( InterruptedException e )
                        {
                        }
                    }
                }
                finally
                {
                    if ( m_debug )
                    {
                        m_outDebug.println( getRes().getString( "Control event monitor thread stopped." ) );
                    }
                }
            }
        };
        m_eventRunner.setDaemon( true );
        m_eventRunner.start();
        
        // Resolve the system thread count based on the Java Version
        String fullVersion = System.getProperty( "java.fullversion" );
        String vendor = System.getProperty( "java.vm.vendor", "" );
        String os = System.getProperty( "os.name", "" ).toLowerCase();
        if ( fullVersion == null )
        {
            fullVersion = System.getProperty( "java.runtime.version" ) + " "
                + System.getProperty( "java.vm.name" );
        }

        if ( m_debug )
        {
            // Display more JVM info right after the call initialization of the
            // library.
            m_outDebug.println( getRes().getString( "Java PID       : {0}", Integer.toString( getJavaPID() ) ) );
            m_outDebug.println( getRes().getString( "Java Version   : {0}", fullVersion ) );
            m_outDebug.println( getRes().getString( "Java VM Vendor : {0}", vendor ) );
            m_outDebug.println( getRes().getString( "OS Name        : {0}", System.getProperty( "os.name", "" ) ) );
            m_outDebug.println( getRes().getString( "OS Arch        : {0}", System.getProperty( "os.arch", "" ) ) );
            m_outDebug.println();
        }
        
        // This thread will most likely be thread which launches the JVM.
        //  Once this method returns however, the main thread will likely
        //  quit.  There will be a slight delay before the Wrapper binary
        //  has a change to send a command to start the application.
        //  During this lag, the JVM may not have any non-daemon threads
        //  running and would exit.   To keep it from doing so, start a
        //  simple non-daemon thread which will run until the
        //  WrapperListener.start() method returns or the Wrapper's
        //  shutdown thread has started.
        m_startupRunner = new Thread( "Wrapper-Startup-Runner" )
        {
            public void run()
            {
                // Make sure the rest of this thread does not fall behind the application.
                Thread.currentThread().setPriority( Thread.MAX_PRIORITY );
                
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "Startup runner thread started." ) );
                }
                
                try
                {
                    while ( m_startupRunner != null )
                    {
                        try
                        {
                            Thread.sleep( 100 );
                        }
                        catch ( InterruptedException e )
                        {
                            // Ignore.
                        }
                    }
                }
                finally
                {
                    if ( m_debug )
                    {
                        m_outDebug.println( getRes().getString( "Startup runner thread stopped." ) );
                    }
                }
            }
        };
        // This thread must not be a daemon thread.
        m_startupRunner.setDaemon( false );
        m_startupRunner.start();
        
        // Create the singleton
        m_instance = new WrapperManager();
    }
    
    /*---------------------------------------------------------------
     * Finalizers
     *-------------------------------------------------------------*/
    protected void finalize()
        throws Throwable
    {
        try
        {
            // This should only happen if the class gets unloaded.
            if ( isLoggingFinalizers() )
            {
                System.out.println( "WrapperManager.finalize" );
            }
        }
        finally
        {
            super.finalize();
        }
    }

    /*---------------------------------------------------------------
     * Native Methods
     *-------------------------------------------------------------*/
    private static native void nativeInit( boolean debug );
    private static native void nativeDispose( boolean debug );
    private static native String nativeGetLibraryVersion();
    private static native int nativeGetJavaPID();
    private static native boolean nativeIsProfessionalEdition();
    private static native boolean nativeIsStandardEdition();
    private static native int nativeGetControlEvent();
    private static native int nativeRedirectPipes();
    private static native void nativeRequestThreadDump();
    private static native void accessViolationInner(); // Should start with native, but need to preserve for compatibility.
    private static native void nativeRaiseExceptionInner( int code );
    private static native void nativeRaiseFailFastExceptionInner();
    private static native void nativeSetConsoleTitle( String titleBytes );
    private static native WrapperUser nativeGetUser( boolean groups );
    private static native WrapperUser nativeGetInteractiveUser( boolean groups );
    private static native WrapperWin32Service[] nativeListServices();
    private static native WrapperWin32Service nativeSendServiceControlCode( String serviceName, int controlCode );
    private static native WrapperProcess nativeExec( String[] cmdArray, String cmdLine, WrapperProcessConfig config, boolean allowCWDOnSpawn );
    private static native String nativeWrapperGetEnv( String val ) throws NullPointerException;
    private static native WrapperResources nativeLoadWrapperResources(String domain, String folder, boolean makeActive);
    private static native boolean nativeCheckDeadLocks();
    private static native int nativeGetPortStatus(int port, String address, int protocol);
    public static native int nativeGetDpiScale();
    
    /*---------------------------------------------------------------
     * Methods
     *-------------------------------------------------------------*/
    /**
     * Returns a tick count calculated from the system clock.
     */
    private static int getSystemTicks()
    {
        // Calculate a tick count using the current system time.  The
        //  conversion from a long in ms, to an int in TICK_MS increments
        //  will result in data loss, but the loss of bits and resulting
        //  overflow is expected and Ok.
        return (int)( System.currentTimeMillis() / TICK_MS );
    }
    
    /**
     * Returns the number of ticks since the JVM was launched.  This
     *  count is not good enough to be used where accuracy is required but
     *  it allows us to implement timeouts in environments where the system
     *  time is modified while the JVM is running.
     * <p>
     * An int is used rather than a long so the counter can be implemented
     *  without requiring any synchronization.  At the tick resolution, the
     *  tick counter will overflow and wrap (every 6.8 years for 100ms ticks).
     *  This behavior is expected.  The getTickAge method should be used
     *  in cases where the difference between two ticks is required.
     *
     * Returns the tick count.
     */
    private static int getTicks()
    {
        if ( m_useSystemTime )
        {
            return getSystemTicks();
        }
        else
        {
            return m_ticks;
        }
    }
    
    /**
     * Returns the number of milliseconds that have elapsed between the
     *  start and end counters.  This method assumes that both tick counts
     *  were obtained by calling getTicks().  This method will correctly
     *  handle cases where the tick counter has overflowed and reset.
     *
     * @param start A base tick count.
     * @param end An end tick count.
     *
     * @return The number of milliseconds that are represented by the
     *         difference between the two specified tick counts.
     */
    private static long getTickAge( int start, int end )
    {
        // Important to cast the first value so that negative values are correctly
        //  cast to negative long values.
        return (long)( end - start ) * TICK_MS;
    }
    
    /**
     * Attempts to load the a native library file.
     *
     * @param name Name of the library to load.
     * @param file Name of the actual library file.
     *
     * @return null if the library was successfully loaded, an error message
     *         otherwise.
     */
    private static String loadNativeLibrary( String name, String file )
    {
        try
        {
            checkOldLibraryOnAix(file);
            System.loadLibrary( name );
            
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "  Attempt to load native library with name: {0}  Result: {1}" , file, getRes().getString( "Success!" ) ) );
            }
            
            return null;
        }
        catch ( UnsatisfiedLinkError e )
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "  Attempt to load native library with name: {0}  Result: {1}" , file, e.getMessage() ) );
            }
            String error = e.getMessage();
            if ( error == null )
            {
                error = e.toString();
            }
            return error;
        }
        catch ( Throwable e )
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "  Attempt to load native library with name: {0}  Result: {1}" , file, e.getMessage() ) );
            }
            String error = e.toString();
            return error;
        }
    }
    
    /**
     * Java 1.5 and above supports the ability to register the WrapperManager
     *  MBean internally.
     */
    private static void registerMBean( Object mbean, String name )
    {
        Class classManagementFactory;
        Class classMBeanServer;
        Class classObjectName;
        try
        {
            classManagementFactory = Class.forName( "java.lang.management.ManagementFactory" );
            classMBeanServer = Class.forName( "javax.management.MBeanServer" );
            classObjectName = Class.forName( "javax.management.ObjectName" );
        }
        catch ( ClassNotFoundException e )
        {
            if ( m_debug )
            {
                m_outDebug.println(getRes().getString( "Registering MBeans not supported by current JVM: {0}" ,  name  ) );
            }
            return;
        }
        
        try
        {
            // This code uses reflection so it combiles on older JVMs.
            // The original code is as follows:
            // javax.management.MBeanServer mbs =
            //     java.lang.management.ManagementFactory.getPlatformMBeanServer();
            // javax.management.ObjectName oName = new javax.management.ObjectName( name );
            // mbs.registerMBean( mbean, oName );
            
            // The version of the above code using reflection follows.
            Method methodGetPlatformMBeanServer =
                classManagementFactory.getMethod( "getPlatformMBeanServer", (Class[])null );
            Constructor constructorObjectName =
                classObjectName.getConstructor( new Class[] {String.class} );
            Method methodRegisterMBean = classMBeanServer.getMethod(
                "registerMBean", new Class[] {Object.class, classObjectName} );
            Object mbs = methodGetPlatformMBeanServer.invoke( (Object)null, (Object[])null );
            Object oName = constructorObjectName.newInstance( new Object[] {name} );
            methodRegisterMBean.invoke( mbs, new Object[] {mbean, oName} );
            
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "Registered MBean with Platform MBean Server: {0}",  name ) );
            }
        }
        catch ( Throwable t )
        {
            if ( t instanceof ClassNotFoundException ) {
                m_outError.println( "Using MBean requires at least a JVM version 1.5." ); 
            }
            m_outError.println( "Unable to register the " + name + " MBean." );
            t.printStackTrace( m_outError );
        }
    }
    
    /**
     * Searches for a file on a path.
     *
     * @param file File to look for.
     * @param path Path to be searched.
     *
     * @return Reference to the file object if found, otherwise null.
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
     * Generates a detailed native library base name which is made up of the
     *  base name, the os name, architecture, and the bits of the current JVM,
     *  not the platform.
     *
     * @return A detailed native library base name.
     */
    private static String generateDetailedNativeLibraryBaseName( String baseName,
                                                                 int jvmBits )
    {
        // Generate an os name.  Most names are used as is, but some are modified.
        String os = System.getProperty( "os.name", "" ).toLowerCase();
        if ( os.startsWith( "windows" ) )
        {
            os = "windows";
            m_windows = true;
        }
        else if ( os.equals( "sunos" ) )
        {
            os = "solaris";
        }
        else if ( os.equals( "hp-ux" ) || os.equals( "hp-ux64" ) )
        {
            os = "hpux";
        }
        else if ( os.equals( "mac os x" ) )
        {
            os = "macosx";
            m_macosx = true;
        }
        else if ( os.equals( "unix_sv" ) )
        {
            os = "unixware";
        }
        else if ( os.equals( "os/400" ) )
        {
            os = "os400";
        }
        else if ( os.equals( "z/os" ) )
        {
            os = "zos";
            m_zos = true;
        }
        else if ( os.indexOf("aix") > -1 )
        {
            m_aix = true;
        }
        m_os = os;
        
        // Generate an architecture name.
        String arch;
        if ( m_macosx )
        {
            arch = "universal";
        }
        else
        {
            arch = System.getProperty( "os.arch", "" ).toLowerCase();
            if ( arch.equals( "amd64" ) || arch.equals( "athlon" ) || arch.equals( "x86_64" ) ||
                arch.equals( "i686" ) || arch.equals( "i586" ) || arch.equals( "i486" ) ||
                arch.equals( "i386" ) )
            {
                arch = "x86";
            }
            else if ( arch.startsWith( "ia32" ) || arch.startsWith( "ia64" ) )
            {
                arch = "ia";
            }
            else if ( arch.startsWith( "sparc" ) )
            {
                arch = "sparc";
            }
            else if ( arch.startsWith( "ppc64le" ) )
            {
                arch = "ppcle";
            }
            else if ( arch.equals( "power" ) || arch.equals( "powerpc" ) || arch.equals( "ppc64" ) )
            {
                if ( m_aix )
                {
                    arch = "ppc";
                }
                else
                {
                    arch = "ppcbe";
                }
            }
            else if ( arch.startsWith( "pa_risc" ) || arch.startsWith( "pa-risc" ) )
            {
                arch = "parisc";
            }
            else if ( arch.startsWith( "arm" ) )
            {
                arch = System.getProperty( "wrapper.arch" );
            }
            else if ( arch.equals( "s390" ) || arch.equals( "s390x" ) )
            {
                arch = "390";
            }
        }
        m_arch = arch;
        
        return baseName + "-" + os + "-" + arch + "-" + jvmBits;
    }
    
    /** 
     * On AIX, mapLibraryName() will return "lib*.a" with the IBM SDK, and "lib*.so" with the openJDK. 
     * Shared libraries can either have .so or .a suffix on AIX, but System.loadLibrary() in openJDK only accepts '.so' file.
     *   Note : After calling this function, we will always log the libraries names with a '.so' suffix even though the loaded lib was '.a'. 
     *   This may not be perfect, but there is no way to control whether System.loadLibrary() loaded a '.a' or a '.so' file. 
     *   We could use System.load() to control the path & suffix, but we don't want to make a special implementation for AIX.
     *   We could also loop through the lib paths and check if libraries with same names & different suffix coexist, but it assumes loadLibrary() always load '.a' in priority, which is not guaranteed in future versions of Java.
     */
    private static String mapSharedLibraryName(String name) 
    {
        String result = System.mapLibraryName( name );
        if (isAIX() && result.endsWith(".a"))
        {
            result = result.substring(0, result.length() - 2).concat(".so");
        }
        return result;
    }
    
    /** 
     * On AIX, '.a' libraries are loaded prior to '.so' libraries if both coexist on the same folder. 
     * To help the user, lets warn if any '.a' file are found in the lib folders.
     */
    private static void checkOldLibraryOnAix(String libName)
    {
        if (isAIX())
        {
            if (libName.endsWith(".so")) 
            {
                libName = libName.substring(0, libName.length() - 3).concat(".a");
            }
            // We want to log the exact path of the library, so we don't pass libPaths with separators but rather loop on each path
            String pathSep = System.getProperty( "path.separator" );
            String[] libPaths = System.getProperty( "java.library.path" ).split(pathSep);
            for ( int j = 0; j < libPaths.length; j++ ) 
            {
                File libFile = locateFileOnPath( libName, libPaths[j] );
                if ( libFile != null )
                {
                    m_outInfo.println( getRes().getString( "WARNING - {0} was found in {1}.", libName, libPaths[j] ));
                    m_outInfo.println( getRes().getString( "          Recent Wrapper''s native libraries have a ''.so'' suffix." ));
                    m_outInfo.println( getRes().getString( "          Depending on the version of Java that is used, {0}", libName ));
                    m_outInfo.println( getRes().getString( "          may be loaded instead of a more recent library." ));
                    m_outInfo.println( getRes().getString( "          Please remove {0} and make sure that the latest version", libName ));
                    m_outInfo.println( getRes().getString( "          of the Wrapper''s native library is in the lib folder." ));
                }
            }
        }
    }
    
    /**
     * Searches for and then loads the native library.  This method will attempt
     *  locate the wrapper library using one of the following 3 naming 
     */
    private static void initializeNativeLibrary()
    {
        // Look for the base name of the library.
        String baseName = System.getProperty( "wrapper.native_library" );
        if ( baseName == null )
        {
            // This should only happen if an old version of the Wrapper binary is being used.
            // This message is logged when localization is not yet initialized.
            m_outInfo.println( "WARNING - The wrapper.native_library system property was not" );
            m_outInfo.println( "          set. Using the default value, 'wrapper'." );
            baseName = "wrapper";
        }
        String[] detailedNames = new String[4];
        if ( m_jvmBits > 0 )
        {
            detailedNames[0] = generateDetailedNativeLibraryBaseName( baseName, m_jvmBits );
        }
        else
        {
            detailedNames[0] = generateDetailedNativeLibraryBaseName( baseName, 32 );
            detailedNames[1] = generateDetailedNativeLibraryBaseName( baseName, 64 );
        }
        
        // Construct brief and detailed native library file names.
        String file = mapSharedLibraryName( baseName );
        String[] detailedFiles = new String[detailedNames.length];
        for ( int i = 0; i < detailedNames.length; i++ )
        {
            if ( detailedNames[i] != null )
            {
                detailedFiles[i] = mapSharedLibraryName( detailedNames[i] );
            }
        }
        
        String[] detailedErrors = new String[detailedNames.length];
        String baseError = null;
        // Try loading the native library using the detailed name first.  If that fails, use
        //  the brief name.
        if ( m_debug )
        {
            // This message is logged when localization is not yet initialized.
            m_outDebug.println( "Load native library.  There are multiple possible file names and the first to be found will be used.  Errors loading non-existing files is normal and is only a problem if they all fail." ); 
        }
        m_libraryOK = false;
        for ( int i = 0; i < detailedNames.length; i++ )
        {
            if ( detailedNames[i] != null )
            {
                detailedErrors[i] = loadNativeLibrary( detailedNames[i], detailedFiles[i] );
                if ( detailedErrors[i] == null )
                {
                    m_libraryOK = true;
                    break;
                }
            }
        }
        if ( ( !m_libraryOK ) && ( ( baseError = loadNativeLibrary( baseName, file ) ) == null ) )
        {
            m_libraryOK = true;
        }
        if ( m_libraryOK )
        {
            m_libraryLoaded = true;
            if ( m_debug )
            {
                // This message is logged when localization is not yet initialized.
                m_outDebug.println( "  Successfully loaded native library." );
            }
            
            // Cache the values of the professional and standard edition flags.
            try
            {
                m_professionalEdition = nativeIsProfessionalEdition();
            }
            catch ( Throwable e )
            {
                if ( m_debug )
                {
                    // This message is logged when localization is not yet initialized.
                    m_outDebug.println( "Call to nativeIsProfessionalEdition() failed: " + e );
                }
                m_professionalEdition = false;
            }
            try
            {
                m_standardEdition = nativeIsStandardEdition();
            }
            catch ( Throwable e )
            {
                if ( m_debug )
                {
                    // This message is logged when localization is not yet initialized.
                    m_outDebug.println( "Call to nativeIsStandardEdition() failed: " + e );
                }
                m_standardEdition = false;
            }
            
            // Try reloading the resources once the library is initialized so we get actual localized content.
            //  We do this before trying to initialize the native library intentionally so any messages will be
            //  localized correctly.  This one call is designed to handle this state.
            m_res = loadWrapperResourcesInner( System.getProperty( "wrapper.lang.domain") + "jni",
                WrapperSystemPropertyUtil.getStringProperty( "wrapper.lang.folder", "../lang" ), true );
            
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "Loaded localized resources." ) );
            }
            
            // The library was loaded correctly, so initialize it.
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "Calling native initialization method." ) );
            }
            nativeInit( m_debug );
            
            if ( m_stoppingInit )
            {
                // Certain checks in the nativeInit call can result in the JVM starting to shutdown.
                //  Avoid further JNI related messages that would be confusing.
                m_libraryOK = false;
            }
        }
        else
        {
            m_libraryLoaded = false;
            // The library could not be loaded, so we want to give the user a useful
            //  clue as to why not.
            String libPath = System.getProperty( "java.library.path" );
            m_outInfo.println();
            if ( libPath.equals( "" ) )
            {
                // No library path
                // This message is logged when localization is unavailable.
                m_outInfo.println( "ERROR - Unable to load the Wrapper's native library because the" );
                m_outInfo.println( "        java.library.path was set to ''.  Please see the" );
                m_outInfo.println( "        documentation for the wrapper.java.library.path" );
                m_outInfo.println( "        configuration property." );
            }
            else
            {
                // Attempt to locate the actual files on the path.
                String error = null;
                File libFile = null;
                for ( int i = 0; i < detailedNames.length; i++ )
                {
                    if ( detailedFiles[i] != null )
                    {
                        libFile = locateFileOnPath( detailedFiles[i], libPath );
                        if ( libFile != null )
                        {
                            error = detailedErrors[i];
                            break;
                        }
                    }
                }
                if ( libFile == null )
                {
                    libFile = locateFileOnPath( file, libPath );
                    if ( libFile != null )
                    {
                        error = baseError;
                    }
                }
                if ( libFile == null )
                {
                    // The library could not be located on the library path.
                    // This message is logged when localization is unavailable.
                    m_outInfo.println( "ERROR - Unable to load the Wrapper's native library because none of the" );
                    m_outInfo.println( "        following files:" );
                    for ( int i = 0; i < detailedNames.length; i++ )
                    {
                        if ( detailedFiles[i] != null )
                        {
                            m_outInfo.println( "          " + detailedFiles[i] );
                        }
                    }
                    m_outInfo.println( "          " + file );
                    m_outInfo.println( "        could be located on the following java.library.path:" );
                    
                    String pathSep = System.getProperty( "path.separator" );
                    StringTokenizer st = new StringTokenizer( libPath, pathSep );
                    while ( st.hasMoreTokens() )
                    {
                        File pathElement = new File( st.nextToken() );
                        m_outInfo.println( "          " + pathElement.getAbsolutePath() );
                    }
                    m_outInfo.println( "        Please see the documentation for the wrapper.java.library.path" );
                    m_outInfo.println( "        configuration property." );
                }
                else
                {
                    // The library file was found but could not be loaded for some reason.
                    // This message is logged when localization is unavailable.
                    m_outInfo.println( "ERROR - Unable to load the Wrapper's native library '" + libFile.getName() + "'." );
                    m_outInfo.println( "        The file is located on the path at the following location but" );
                    m_outInfo.println( "        could not be loaded:" );
                    m_outInfo.println( "          " + libFile.getAbsolutePath() );
                    m_outInfo.println( "        Please verify that the file is both readable and executable by the" );
                    m_outInfo.println( "        current user and that the file has not been corrupted in any way." );
                    m_outInfo.println( "        One common cause of this problem is running a 32-bit version" );
                    m_outInfo.println( "        of the Wrapper with a 64-bit version of Java, or vica versa." );
                    if ( m_jvmBits > 0 )
                    {
                        m_outInfo.println( "        This is a " + m_jvmBits + "-bit JVM." );
                    }
                    else
                    {
                        m_outInfo.println( "        The bit depth of this JVM could not be determined." );
                    }
                    m_outInfo.println( "        Reported cause:" );
                    m_outInfo.println( "          " + error );
                }
            }
            m_outInfo.println();
        }
    }
    
    /**
     * Compares the version of the wrapper which launched this JVM with that of
     *  the jar.  If they differ then a Warning message will be displayed.  The
     *  Wrapper application will still be allowed to start.
     */
    private static void verifyWrapperVersion()
    {
        // If we are not being controlled by the wrapper then return.
        if ( !WrapperManager.isControlledByNativeWrapper() )
        {
            return;
        }
        
        // Lookup the version from the wrapper.  It should have been set as a property
        //  when the JVM was launched.
        String wrapperVersion = System.getProperty( "wrapper.version" );
        if ( wrapperVersion == null )
        {
            wrapperVersion = getRes().getString( "unknown" );
        }
        if ( wrapperVersion.endsWith( "-pro" ) )
        {
            wrapperVersion = wrapperVersion.substring( 0, wrapperVersion.length() - 4 );
        }
        else if ( wrapperVersion.endsWith( "-st" ) )
        {
            wrapperVersion = wrapperVersion.substring( 0, wrapperVersion.length() - 3 );
        }
        
        if ( !WrapperInfo.getVersion().equals( wrapperVersion ) )
        {
            m_outInfo.println( getRes().getString( 
                "ERROR - The version of the Wrapper which launched this JVM is \"{0}\"\n        while the version of the Wrapper jar file currently in use\n        is \"{1}\".", wrapperVersion, WrapperInfo.getVersion() ) );
            m_outInfo.println();
            m_wrapperVersionOk = false;
        } else {
            m_wrapperVersionOk = true;
        }
    }
    
    /**
     * Compares the version of the native library with that of this jar.  If
     *  they differ then a Warning message will be displayed.  The Wrapper
     *  application will still be allowed to start.
     */
    private static void verifyNativeLibraryVersion()
    {
        // Request the version from the native library.  Be careful as the method
        //  will not exist if the library is old.
        String jniVersion;
        try
        {
            jniVersion = nativeGetLibraryVersion();
        }
        catch ( Throwable e )
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( 
                        "Call to nativeGetLibraryVersion() failed: {0}",  e ) );
            }
            jniVersion = getRes().getString( "unknown" );
        }
        
        String wrapperVersion = System.getProperty( "wrapper.version" );
        if ( wrapperVersion == null )
        {
            wrapperVersion = getRes().getString( "unknown" );
        }
        
        if ( !wrapperVersion.equals( jniVersion ) )
        {
            m_outInfo.println( getRes().getString( 
                "ERROR - The version of the Wrapper which launched this JVM is\n        \"{0}\" while the version of the native library\n        is \"{1}\".", wrapperVersion, jniVersion ) );
            m_outInfo.println();
            m_libraryVersionOk = false;
        } else {
            m_libraryVersionOk = true;
        }
    }
    
    /**
     * Checks to make sure that the configured temp directory is writable.  Failures are only logged
     *  to debug output.
     */
    private static void checkTmpDir()
    {
        File tmpDir = new File( System.getProperty( "java.io.tmpdir" ) );
        if ( m_debug )
        {
            m_outDebug.println( getRes().getString( "Java temporary directory: {0}", tmpDir ) );
        }
        
        boolean tmpDirCheck = getProperties().getProperty( "wrapper.java.tmpdir.check", "TRUE").equalsIgnoreCase( "TRUE" );
        if ( !tmpDirCheck )
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "Validation of temporary directory disabled." ) );
            }
            return;
        }
        
        boolean tmpDirRequired = getProperties().getProperty( "wrapper.java.tmpdir.required", "FALSE" ).equalsIgnoreCase( "TRUE" );
        boolean tmpDirWarnSilently = getProperties().getProperty( "wrapper.java.tmpdir.warn_silently", "TRUE" ).equalsIgnoreCase( "TRUE" );
        Exception ex = null;
        try
        {
            tmpDir = tmpDir.getCanonicalFile();
            File tempFile = new File( tmpDir, "wrapper-" + System.currentTimeMillis() + "-" + getJavaPID() );
            if ( tempFile.createNewFile() )
            {
                if ( !tempFile.delete() )
                {
                    m_outError.println( "Unable to delete temporary file: " + tempFile );
                }
            }
            else
            {
                if ( m_debug )
                {
                    m_outDebug.println( "Unable to create temporary file: " + tempFile );
                }
            }
        }
        catch ( IOException e )
        {
            ex = e;
        }
        catch ( SecurityException e )
        {
            ex = e;
        }
        
        if ( ex != null )
        {
            if ( tmpDirRequired )
            {
                m_outError.println( getRes().getString( "Unable to write to the configured Java temporary directory: {0} : {1}", tmpDir, ex.toString() ) );
                m_outError.println( getRes().getString( "Shutting down." ) );
                System.exit( 1 );
            }
            else
            {
                if ( tmpDirWarnSilently )
                {
                    if ( m_debug )
                    {
                        m_outDebug.println( getRes().getString( "Unable to write to the configured Java temporary directory: {0} : {1}", tmpDir, ex.toString() ) );
                    }
                }
                else
                {
                    m_outInfo.println( getRes().getString( "Unable to write to the configured Java temporary directory: {0} : {1}", tmpDir,ex.toString() ) );
                }
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "  The lack of a temp directory could lead to problems with features that store temporary data, including remote jar class loading." ) );
                    m_outDebug.println( getRes().getString( "  The Java temporary directory can be redefined with the java.io.tmpdir system property." ) );
                }
            }
        }
    }
    
    /**
     * Loads a WrapperResources based on the current locale of the JVM.
     *
     * @param domain Domain of the resource.
     * @param folder Location of the resource.
     *
     * @return The requested resource.
     */
    private static WrapperResources loadWrapperResourcesInner( String domain, String folder, boolean makeActive )
    {
        if ( WrapperManager.isStandardEdition() && isNativeLibraryOk() )
        {
            if ( folder == null )
            {
                folder = WrapperSystemPropertyUtil.getStringProperty( "wrapper.lang.folder", "../lang" );
            }
            
            return nativeLoadWrapperResources( domain, folder, makeActive );
        }
        else
        {
            return new WrapperResources();
        }
    }
    
    /**
     * Loads a WrapperResources based on the current locale of the JVM.
     *
     * @param domain Domain of the resource.
     * @param folder Location of the resource.
     *
     * @return The requested resource.
     */
    public static WrapperResources loadWrapperResources( String domain, String folder )
    {
        return loadWrapperResourcesInner( domain, folder, false );
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
    

    private static String[] parseCommandLine( String cmdLine )
    {
        ArrayList argList = new ArrayList();
        StringBuffer arg = new StringBuffer();
        boolean quoteMode = false;
        boolean escapeNextCharIfQuote = false;
        char c[] = cmdLine.toCharArray();
        for ( int i = 0; i < cmdLine.length(); i++ )
        {
            if ( ( c[i] == '\\' ) && !escapeNextCharIfQuote )
            {

                escapeNextCharIfQuote = true;
            }
            else
            {
                if ( Character.isWhitespace( c[i] ) && ( quoteMode == false ) )
                {
                    if ( arg.length() > 0 )
                    {
                        argList.add( arg.toString() );
                        arg.setLength( 0 );
                    }
                }
                else
                {
                    if ( c[i] == '\"' )
                    {
                        if ( escapeNextCharIfQuote == false )
                        {
                            quoteMode = ( ( quoteMode == true ) ? false : true );
                            escapeNextCharIfQuote = false;
                            continue;
                        }
                        else
                        {
                            // arg.append('\\');
                            escapeNextCharIfQuote = false;
                        }
                        arg.append( c[i] );
                    }
                    else if ( c[i] == '\\' )
                    {
                        if ( escapeNextCharIfQuote == true )
                        {
                            escapeNextCharIfQuote = false;
                        }
                        arg.append( '\\' );
                    }
                    else
                    {
                        if ( escapeNextCharIfQuote == true )
                        {
                            arg.append( '\\' );
                            escapeNextCharIfQuote = false;
                        }
                        arg.append( c[i] );
                    }
                } // else
            } // else
        } // for
        if ( arg.length() > 0 )
        {
            argList.add( arg.toString() );
        }
        String[] args = new String[ argList.size() ];
        argList.toArray( args );
        return args;
    }

    /**
     * A more powerful replacement to the java.lang.Runtime.exec method.
     * <p>
     * When the JVM exits or is terminated for any reason, the Wrapper will
     *  clean up any child processes launched with this method automatically
     *  before shutting down or launching a new JVM.
     * <p>
     * This method is the same as calling <pre>WrapperManager.exec(command, new WrapperProcessConfig());</pre>
     * <p>
     * The returned WrapperProcess object can be used to control the child
     *  process, supply input, or process output.
     * <p>
     * Professional Edition feature.
     *
     * @param command A specified system command in one String.
     *
     * @return A new WrapperProcess object for managing the subprocess.
     *
     * @throws IOException Will be thrown if an I/O error occurs 
     * @throws NullPointerException If command is null.
     * @throws IllegalArgumentException If command is empty
     * @throws SecurityException If a SecurityManager is present and its
     *                           checkExec method doesn't allow creation of a
     *                           subprocess.    
     * @throws WrapperJNIError If the native library has not been loaded or is in the
     *                         process of shutting down.
     * @throws WrapperLicenseError If the function is called other than in
     *                             the Professional Edition or if the native
     *                             library has not been loaded.
     * @throws UnsatisfiedLinkError If the posix_spawn function couldn't be found
     *
     * @see #isProfessionalEdition()
     * @see #exec(String command, WrapperProcessConfig config)
     * @see WrapperProcessConfig
     * @since Wrapper 3.4.0
     */
    public static WrapperProcess exec( String command )
        throws SecurityException, IOException, NullPointerException, IllegalArgumentException, WrapperJNIError, WrapperLicenseError, UnsatisfiedLinkError
    {
        WrapperProcess proc = exec( command, new WrapperProcessConfig() );
        return proc;
    }

    /**
     * A more powerful replacement to the java.lang.Runtime.exec method.
     * <p>
     * By configuring the WrapperProcessConfig object, it is possible to
     *  control whether or not the child process will be automatically
     *  cleaned up when the JVM exits or is terminated.  It is also possible
     *  to control how the child process is launched to work around memory
     *  issues on some platforms.
     * <p>
     * For example, on Solaris when the JVM is very large, doing a fork will
     *  duplicate the entire JVM's memory space and cause an out of memory
     *  error or JVM crash, to avoid such memory problems the child process
     *  can be launched using posix spawn as follows:
     *  <pre>WrapperManager.exec( command, new WrapperProcessConfig().setStartType( WrapperProcessConfig.POSIX_SPAWN ) );</pre>
     * <p>
     * Please review the WrapperProcessConfig class for a full list of
     *  options.
     * <p>
     * The returned WrapperProcess object can be used to control the child
     *  process, supply input, or process output.
     * <p>
     * Professional Edition feature.
     *
     * @param command A specified system command in one String.
     * @param config A WrapperProcessConfig object representing the Start/Run
     *               Configurations of the subprocess
     *
     * @return A new WrapperProcess object for managing the subprocess.
     *
     * @throws IOException Will be thrown if an I/O error occurs 
     * @throws NullPointerException If command is null.
     * @throws IllegalArgumentException If command is empty or the configuration is invalid

     * @throws SecurityException If a SecurityManager is present and its
     *                           checkExec method doesn't allow creation of a
     *                           subprocess.
     * @throws WrapperJNIError If the native library has not been loaded or is in the
     *                         process of shutting down.
     * @throws WrapperLicenseError If the function is called other than in
     *                             the Professional Edition or if the native
     *                             library has not been loaded.
     * @throws UnsatisfiedLinkError If the posix_spawn function couldn't be found
     *
     * @see #isProfessionalEdition()
     * @see WrapperProcessConfig
     * @since Wrapper 3.4.0
     */
    public static WrapperProcess exec( String command, WrapperProcessConfig config )
        throws SecurityException, IOException, NullPointerException, IllegalArgumentException, WrapperJNIError, WrapperLicenseError, UnsatisfiedLinkError
    {  
        if ( ( command == null ) || ( command.length() == 0 ) )
        {
            throw new IllegalArgumentException( getRes().getString( "No command specified" ) );
        }

        return exec( null, command, config );
    }

    /**
     * A more powerful replacement to the java.lang.Runtime.exec method.
     * <p>
     * When the JVM exits or is terminated for any reason, the Wrapper will
     *  clean up any child processes launched with this method automatically
     *  before shutting down or launching a new JVM.
     * <p>
     * This method is the same as calling <pre>WrapperManager.exec(cmdArray, new WrapperProcessConfig());</pre>
     * <p>
     * The returned WrapperProcess object can be used to control the child
     *  process, supply input, or process output.
     * <p>
     * Professional Edition feature.
     *
     * @param cmdArray A specified system command in array format for each
     *               parameter a single element.
     *
     * @return A new WrapperProcess object for managing the subprocess 
     *
     * @throws IOException Will be thrown at any error realated with Memory
     *                     allocation, or if the command does not exist.
     * @throws NullPointerException If cmdarray is null, or one of the elements
     *                              of cmdarray is null.
     * @throws IndexOutOfBoundsException If cmdarray is an empty array (has
     *                                   length 0) 

     * @throws SecurityException If a SecurityManager is present and its
     *                           checkExec method doesn't allow creation of a
     *                           subprocess.    
     * @throws IllegalArgumentException If there are any problems with the
     *                                  WrapperProcessConfig object.
     * @throws WrapperJNIError If the native library has not been loaded or is in the
     *                         process of shutting down.
     * @throws WrapperLicenseError If the function is called other than in
     *                             the Professional Edition or if the native
     *                             library has not been loaded.
     * @throws UnsatisfiedLinkError If the posix_spawn function couldn't be found
     *
     * @see #isProfessionalEdition()
     * @since Wrapper 3.4.0
     */
    public static WrapperProcess exec( String[] cmdArray )
        throws SecurityException, IOException, NullPointerException, IndexOutOfBoundsException, IllegalArgumentException, WrapperJNIError, UnsatisfiedLinkError, WrapperLicenseError
    {
        WrapperProcess proc = exec( cmdArray, new WrapperProcessConfig() );
        return proc;
    }
    
    

    /**
     * A more powerful replacement to the java.lang.Runtime.exec method.
     * <p>
     * By configuring the WrapperProcessConfig object, it is possible to
     *  control whether or not the child process will be automatically
     *  cleaned up when the JVM exits or is terminated.  It is also possible
     *  to control how the child process is launched to work around memory
     *  issues on some platforms.
     * <p>
     * For example, on Solaris when the JVM is very large, doing a fork will
     *  duplicate the entire JVM's memory space and cause an out of memory
     *  error or JVM crash, to avoid such memory problems the child process
     *  can be launched using posix spawn as follows:
     *  <pre>WrapperManager.exec( cmdArray, new WrapperProcessConfig().setStartType( WrapperProcessConfig.POSIX_SPAWN ) );</pre>
     * <p>
     * Please review the WrapperProcessConfig class for a full list of
     *  options.
     * <p>
     * The returned WrapperProcess object can be used to control the child
     *  process, supply input, or process output.
     * <p>
     * Professional Edition feature.
     *
     * @param cmdArray A specified system command in array format, for each
     *               parameter a single element.
     * @param config A WrapperProcessConfig object representing the Start/Run
     *               Configurations of the subprocess
     *
     * @return A new WrapperProcess object for managing the subprocess.
     *
     * @throws IOException Will be thrown if an I/O error occurs  
     * @throws NullPointerException If cmdarray is null, or one of the elements
     *                              of cmdarray is null.
     * @throws IndexOutOfBoundsException If cmdarray is an empty array (has
     *                                   length 0)
     * @throws SecurityException If a SecurityManager is present and its
     *                           checkExec method doesn't allow creation of a
     *                           subprocess.
     * @throws IllegalArgumentException If there are any problems with the
     *                                  WrapperProcessConfig object.
     * @throws WrapperJNIError If the native library has not been loaded or is in the
     *                         process of shutting down.
     * @throws WrapperLicenseError If the function is called other than in
     *                             the Professional Edition or if the native
     *                             library has not been loaded.
     * @throws UnsatisfiedLinkError If the posix_spawn function couldn't be found
     *
     * @see #isProfessionalEdition()
     * @since Wrapper 3.4.0
     */
    public static WrapperProcess exec( String[] cmdArray, WrapperProcessConfig config )
        throws SecurityException, IOException, NullPointerException, IndexOutOfBoundsException, IllegalArgumentException, WrapperJNIError, WrapperLicenseError, UnsatisfiedLinkError
    {
        return exec( cmdArray, null, config );
    }

    /**
     * Executes an external command.
     */
    private static WrapperProcess exec( String[] cmdArray, String cmdLine, WrapperProcessConfig config )
        throws SecurityException, IOException, NullPointerException, IndexOutOfBoundsException, IllegalArgumentException, WrapperJNIError, WrapperLicenseError, UnsatisfiedLinkError {
        
        // If the cmdArray parameter is null then the cmdLine will be parsed into an a cmdArray.
        //  Then both the cmdArray and cmdLine will be passed off to the native code.
        //  The cmdLine may be null.
        
        assertProfessionalEdition();
        
        if ( ( cmdArray == null ) && ( cmdLine == null ) )
        {
            throw new NullPointerException( getRes().getString( "No command specified" ) );

        }
        else if ( ( cmdArray != null) && ( cmdArray.length == 0 ) )
        {
            throw new IndexOutOfBoundsException( getRes().getString( "cmdArray is empty" ) );
        }
        
        if ( ( cmdArray == null ) && ( cmdLine != null ) ) {
            cmdArray = parseCommandLine( cmdLine );
        }
        
        if ( config == null )
        {
            throw new NullPointerException( getRes().getString( "config is null" ) );
        }
        
        // Make sure the call stack has permission to execute this command.
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkExec( cmdArray[0] );
        }
        
        // Keep track of how many threads are trying to execute child processes.
        //  This is critical to avoid notification failures on shutdown.
        synchronized( WrapperManager.class )
        {
            m_runningExecs++;
        }
        try
        {
            if ( isNativeLibraryOk() )
            {
                for ( int i = 0; i < cmdArray.length; i++ )
                {
                    if ( cmdArray[i] == null )
                    {
                        throw new NullPointerException( getRes().getString( "cmdarray[{0}]: Invalid element (isNull).",
                                new Integer( i)  ) );
                    }
                }
                
                // On UNIX platforms, we want to try and make sure the command is
                //  valid before we run it to avoid problems later.  Not necessary
                //  on Windows.
                if ( !m_windows )
                {
                    if ( !new File( cmdArray[0] ).exists() )
                    {
                        boolean found = false;
                        String path = nativeWrapperGetEnv( "PATH" );
                        if ( path != null )
                        {
                            String[] paths = path.split( File.pathSeparator );
                            
                            for ( int i = 0; i < paths.length; i++ )
                            {
                                File file = new File( paths[i] + File.separator + cmdArray[0] );
                                // m_outInfo.println( blu.getPath() );
                                if ( file.exists() ) 
                                {
                                    cmdArray[0] = file.getPath();
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if ( !found )
                        {
                            throw new IOException(getRes().getString( "''{0}'' not found." , cmdArray[0]  ) ); 
                        }
                    }
                }
                if ( m_debug ) 
                {
                    for ( int j = 0; j < cmdArray.length; j++ )
                    {
                        m_outDebug.println( "args[" + j + "] = " + cmdArray[j] );
                    }
                }
                return nativeExec( cmdArray, cmdLine, config.setEnvironment( config.getEnvironment() ), WrapperSystemPropertyUtil.getBooleanProperty( "wrapper.child.allowCWDOnSpawn", false ) );
            } else {
                if ( m_stopped ) {
                    // This message is logged when localization is no longer available.
                    throw new WrapperJNIError( "Wrapper native library shutting down." );
                } else {
                    // This message is logged when localization is not available.
                    throw new WrapperJNIError( "Wrapper native library not loaded." );
                }
            }
        }
        finally
        {
            synchronized( WrapperManager.class )
            {
                m_runningExecs--;
                if ( m_runningExecs <= 0 )
                {
                    WrapperManager.class.notifyAll();
                }
            }
        }
    }
    
    /**
     * Returns true if the native library has been loaded successfully, false
     *  otherwise.  This value can switch to false on shutdown when the native
     *  library can no longer be reliably referenced.
     *
     * @return True if the native library is loaded and available.
     */
    public static boolean isNativeLibraryOk()
    {
        return m_libraryOK && ( !m_stopped );
    }
    
    /**
     * Asserts that the Professional Edition of the Wrapper is being used and
     *  that the native library is available.
     *
     * @throws WrapperLicenseError If the function is called other than in
     *                             the Professional Edition or if the native
     *                             library has not been loaded.
     */
    static void assertProfessionalEdition()
        throws WrapperLicenseError
    {
        if ( !m_libraryOK )
        {
            throw new WrapperLicenseError( getRes().getString( "Requires that the Professional Edition native library be loaded.  Please check for errors earlier in the log." ) );
        }
        else if ( m_stopped )
        {
            throw new WrapperLicenseError( getRes().getString( "Requires that the Professional Edition native library be loaded, but it has already been unloaded as part of the shutdown process." ) );
        }
        else if ( !isProfessionalEdition() )
        {
            throw new WrapperLicenseError( getRes().getString( "Requires the Professional Edition." ) );
        }
    }
    
    /**
     * Returns the OS that the Wrapper has resolved.
     *  This will be the same as the WRAPPER_OS environment variable.
     *  Used when resolving the platform specific native library name.
     *
     * @return The resolved Wrapper OS name.
     *
     * @since Wrapper 3.5.31
     */
    public static String getOS()
    {
        return m_os;
    }
    
    /**
     * Returns the Architecture that the Wrapper has resolved.
     *  This will be the same as the WRAPPER_ARCH environment variable.
     *  Used when resolving the platform specific native library name.
     *
     * @return The resolved Wrapper Architecture name.
     *
     * @since Wrapper 3.5.31
     */
    public static String getArch()
    {
        return m_arch;
    }
    
    /**
     * Returns true if the current JVM is Windows.
     *
     * @return True if this is Windows.
     *
     * @since Wrapper 3.5.1
     */
    public static boolean isWindows()
    {
        return m_windows;
    }
    
    /**
     * Returns true if the current JVM is Mac OSX.
     *
     * @return True if this is Mac OSX.
     *
     * @since Wrapper 3.5.1
     */
    public static boolean isMacOSX()
    {
        return m_macosx;
    }
    
    /**
     * Returns true if the current JVM is AIX.
     *
     * @return True if this is AIX.
     *
     * @since Wrapper 3.5.27
     */
    public static boolean isAIX()
    {
        return m_aix;
    }
    
    /**
     * Returns true if the current JVM is z/OS.
     *
     * @return True if this is z/OS.
     *
     * @since Wrapper 3.5.35
     */
    private static boolean isZOS()
    {
        return m_zos;
    }
    
    /**
     * Returns true if the current Wrapper edition has support for Professional
     *  Edition features.
     *
     * False will also be returned if the native library has not been initialized correctly.
     *
     * @return True if professional features are supported.
     */
    public static boolean isProfessionalEdition()
    {
        // Use a cached value rather than calling the JNI call here to avoid confusing errors on shutdown.
        return m_professionalEdition;
    }
    
    /**
     * Returns true if the current Wrapper edition has support for Standard
     *  Edition features.
     *
     * False will also be returned if the native library has not been initialized correctly.
     *
     * @return True if standard features are supported.
     */
    public static boolean isStandardEdition()
    {
        // Use a cached value rather than calling the JNI call here to avoid confusing errors on shutdown.
        return m_standardEdition;
    }
    
    /**
     * Fires a user event user_n specified in the conf file
     *
     * @param eventNr The user-event number to fire. Must be in the range of 1-32767.
     * 
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("fireUserEvent")
     *                           permission.
     * @throws WrapperLicenseError If the function is called other than in
     *                             the Professional Edition or if the native
     *                             library has not been loaded.
     */
    public static void fireUserEvent( int eventNr )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperUserEventPermission( "fireUserEvent", String.valueOf( eventNr ) ) );
        }
        if ( eventNr <= 0 || eventNr > 32767 ) {
            throw new java.lang.IllegalArgumentException( getRes().getString( "The user-event number must be in the range of 1-32767." ) );
        }
        assertProfessionalEdition();
        
        sendCommand( WRAPPER_MSG_FIRE_USER_EVENT, String.valueOf( eventNr ) );
    }


    /**
     * Sets the title of the console in which the Wrapper is running.  This
     *  is currently only supported on Windows platforms.
     * <p>
     * As an alternative, it is also possible to set the console title from
     *  within the wrapper.conf file using the wrapper.console.title property.
     *
     * @param title The new title.  The specified string will be encoded
     *              to a byte array using the default encoding for the
     *              current platform.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("setConsoleTitle")
     *                           permission.
     *
     * @see WrapperPermission
     */
    public static void setConsoleTitle( String title )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "setConsoleTitle" ) );
        }
        
        if ( isNativeLibraryOk() )
        {
            nativeSetConsoleTitle( title );
        }
    }
    
    /**
     * Returns a WrapperUser object which describes the user under which the
     *  Wrapper is currently running.  Additional platform specific information
     *  can be obtained by casting the object to a platform specific subclass.
     *  WrapperWin32User, for example.
     *
     * @param groups True if the user's groups should be returned as well.
     *               Requesting the groups that a user belongs to increases
     *               the CPU load required to complete the call.
     *
     * @return An object describing the current user.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("getUser") permission.
     *
     * @see WrapperPermission
     */
    public static WrapperUser getUser( boolean groups )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "getUser" ) );
        }
        
        WrapperUser user = null;
        if ( isNativeLibraryOk() )
        {
            user = nativeGetUser( groups );
        }
        return user;
    }
    
    /**
     * Returns a WrapperUser object which describes the interactive user whose
     *  desktop is being interacted with.  When a service running on a Windows
     *  platform has its interactive flag set, this method will return the user
     *  who is currently logged in.  Additional platform specific information
     *  can be obtained by casting the object to a platform specific subclass.
     *  WrapperWin32User, for example.
     * <p>
     * If a user is not currently logged on then this method will return null.
     *  User code can repeatedly call this method to detect when a user has
     *  logged in.  To detect when a user has logged out, there are two options.
     *  1) The user code can continue to call this method until it returns null.
     *  2) Or if the WrapperListener method is being implemented, the
     *     WrapperListener.controlEvent method will receive a WRAPPER_CTRL_LOGOFF_EVENT
     *     event when the user logs out.
     * <p>
     * On XP systems, it is possible to switch to another account rather than
     *  actually logging out.  In such a case, the interactive user will be
     *  the first user that logged in.  This will also be the only user with
     *  which the service will interact.  If other users are logged in when the
     *  interactive user logs out, the service will not automatically switch to
     *  another logged in user.  Rather, the next user to log in will become
     *  the new user which the service will interact with.
     * <p>
     * This method will always return NULL on versions of NT prior to Windows
     *  2000.  This can not be helped as some required functions were not added
     *  to the windows API until NT version 5.0, also known as Windows 2000.
     *
     * @param groups True if the user's groups should be returned as well.
     *               Requesting the groups that a user belongs to increases
     *               the CPU load required to complete the call.
     *
     * @return The current interactive user, or null.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("getInteractiveUser")
     *                           permission.
     *
     * @see WrapperPermission
     */
    public static WrapperUser getInteractiveUser( boolean groups )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "getInteractiveUser" ) );
        }
        
        WrapperUser user = null;
        if ( isNativeLibraryOk() )
        {
            user = nativeGetInteractiveUser( groups );
        }
        return user;
    }
    
    /**
     * Returns a Properties object containing expanded the contents of the
     *  configuration file used to launch the Wrapper.
     *
     * All properties are included so it is possible to define properties
     *  not used by the Wrapper in the configuration file and have then
     *  be available in this Properties object.
     *
     * @return The contents of the Wrapper configuration file.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("getProperties")
     *                           permission.
     *
     * @see WrapperPermission
     */
    public static Properties getProperties()
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "getProperties" ) );
        }
        
        return m_properties;
    }
    
    /**
     * Returns the PID of the Wrapper process.
     *
     * A PID of 0 will be returned if the JVM was launched standalone.
     *
     * This value can also be obtained using the 'wrapper.pid' system property.
     *
     * @return The PID of the Wrpper process.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("getWrapperPID") permission.
     *
     * @see WrapperPermission
     */
    public static int getWrapperPID()
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "getWrapperPID" ) );
        }
        
        return WrapperSystemPropertyUtil.getIntProperty( "wrapper.pid", 0 );
    }
    
    /**
     * Returns the PID of the Java process.
     *
     * A PID of 0 will be returned if the native library has not been initialized.
     *
     * This value can also be obtained using the 'wrapper.java.pid' system property.
     *
     * @return The PID of the Java process.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("getJavaPID") permission.
     *
     * @see WrapperPermission
     */
    public static int getJavaPID()
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "getJavaPID" ) );
        }
        
        return WrapperSystemPropertyUtil.getIntProperty( "wrapper.java.pid", 0 );
    }
    
    /**
     * Requests that the current JVM process request a thread dump.  This is
     *  the same as pressing CTRL-BREAK (under Windows) or CTRL-\ (under Unix)
     *  in the the console in which Java is running.  This method does nothing
     *  if the native library is not loaded.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("requestThreadDump")
     *                           permission.
     *
     * @see WrapperPermission
     */
    public static void requestThreadDump()
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "requestThreadDump" ) );
        }
        
        if ( isNativeLibraryOk() )
        {
            nativeRequestThreadDump();
        }
        else
        {
            m_outInfo.println( getRes().getString( "  wrapper library not loaded." ) );
        }
    }
    
    /**
     * (Testing Method) Causes the WrapperManager to go into a state which makes the JVM appear
     *  to be hung when viewed from the native Wrapper code.  Does not have any effect when the
     *  JVM is not being controlled from the native Wrapper. Useful for testing the Wrapper 
     *  functions.
     *
     * @throws IllegalStateException If testing methods have been disabled by setting
     *                               the wrapper.disable_tests=true system property.
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("test.appearHung") permission.
     *
     * @see WrapperPermission
     */
    public static void appearHung()
    {
        if ( m_disableTests )
        {
            throw new IllegalStateException( getRes().getString( "Test methods have been disabled." ) );
        }
        
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "test.appearHung" ) );
        }
        
        m_outInfo.println( getRes().getString( "WARNING: Making JVM appear to be hung..." ) );
        m_appearHung = true;
    }
    
    /**
     * (Testing Method) Causes the WrapperManager to go into a state which makes
     *  the JVM appear to be sluggish when viewed from the native Wrapper code.
     *  Each packet received from the Wrapper is delayed by the specified number
     *  of seconds.
     *
     * If several packets are received in succession then this delay will be
     *  cumulative.  The delay reported by the Wrapper may appear to be quite a
     *  bit different if multiple packets are being delayed in series.  Some
     *  packets, like pings, which are sent multiple times may also be reported
     *  as being processed faster than this delay because the previous response
     *  is what is actually being received.
     *
     *  Does not have any effect when the JVM is not being controlled from the
     *  native Wrapper. Useful for testing the Wrapper functions.
     *
     * @param slowSeconds The number of seconds to delay reponding to any incoming
     *                    commands from the wrapper.
     *
     * @throws IllegalStateException If testing methods have been disabled by setting
     *                               the wrapper.disable_tests=true system property.
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("test.appearSlow") permission.
     *
     * @see WrapperPermission
     */
    public static void appearSlow( int slowSeconds )
    {
        if ( m_disableTests )
        {
            throw new IllegalStateException( getRes().getString( "Test methods have been disabled." ) );
        }
        
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "test.appearSlow" ) );
        }
        
        if ( slowSeconds > 0 )
        {
            m_outInfo.println( getRes().getString( "WARNING: Making JVM appear to be slow using a delay of {0} seconds...", new Integer( slowSeconds ) ) );
            m_slowSeconds = slowSeconds;
        }
        else if ( m_slowSeconds > 0 )
        {
            m_outInfo.println( getRes().getString( "Resetting the JVM delayed response to normal..." ) );
            m_slowSeconds = 0;
        }
    }
    
    /**
     * @deprecated Removed as of 3.5.8
     */
    public static void appearOrphan()
    {
    }
    
    /**
     * (Testing Method) Cause an access violation within the Java code.  Useful
     *  for testing the Wrapper functions.  This currently only crashes Sun
     *  JVMs and takes advantage of Bug #4369043 which does not exist in newer
     *  JVMs.  Use of the accessViolationNative() method is preferred.
     *
     * @throws IllegalStateException If testing methods have been disabled by setting
     *                               the wrapper.disable_tests=true system property.
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("test.accessViolation")
     *                           permission.
     *
     * @see WrapperPermission
     */
    public static void accessViolation()
    {
        if ( m_disableTests )
        {
            throw new IllegalStateException( getRes().getString( "Test methods have been disabled." ) );
        }
        
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "test.accessViolation" ) );
        }
        
        m_outInfo.println( getRes().getString(
                "WARNING: Attempting to cause an access violation..." ) );
        
        try
        {
            Class c = Class.forName( "java.lang.String" );
            java.lang.reflect.Method m = c.getDeclaredMethod( (String)null, (Class[])null );
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
                ex.printStackTrace( m_outError );
            }
        }
        
        m_outInfo.println( getRes().getString(
                "  Attempt to cause access violation failed.  JVM is still alive." ) );
    }

    /**
     * (Testing Method) Cause an access violation within native JNI code.
     *  Useful for testing the Wrapper functions. This currently causes the
     *  access violation by attempting to write to a null pointer.
     *
     * @throws IllegalStateException If testing methods have been disabled by setting
     *                               the wrapper.disable_tests=true system property.
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("test.accessViolationNative")
     *                           permission.
     *
     * @see WrapperPermission
     */
    public static void accessViolationNative()
    {
        if ( m_disableTests )
        {
            throw new IllegalStateException( getRes().getString( "Test methods have been disabled." ) );
        }
        
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "test.accessViolationNative" ) );
        }
        
        m_outInfo.println( getRes().getString(
                "WARNING: Attempting to cause an access violation..." ) );
        if ( isNativeLibraryOk() )
        {
            accessViolationInner();
        
            m_outInfo.println( getRes().getString(
                    "  Attempt to cause access violation failed.  JVM is still alive." ) );
        }
        else
        {
            m_outInfo.println( getRes().getString( "  wrapper library not loaded." ) );
        }
    }
    
    /**
     * (Testing Method) raise an exception in native code.
     *  Useful for testing the Wrapper functions.
     *
     * Ignored when not running on Windows.
     *
     * @param code Exception code to Raise.
     *
     * @throws IllegalStateException If testing methods have been disabled by setting
     *                               the wrapper.disable_tests=true system property.
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("test.raiseExceptionNative")
     *                           permission.
     *
     * @see WrapperPermission
     */
    public static void raiseExceptionNative( int code )
    {
        if ( m_disableTests )
        {
            throw new IllegalStateException( getRes().getString( "Test methods have been disabled." ) );
        }
        
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "test.raiseExceptionNative" ) );
        }
        
        m_outInfo.println( getRes().getString( "WARNING: Attempting to raise an exception in native code..." ) );
        if ( isNativeLibraryOk() )
        {
            nativeRaiseExceptionInner( code );
        
            m_outInfo.println( getRes().getString( "  Attempt to raise a native exception failed.  JVM is still alive." ) );
        }
        else
        {
            m_outInfo.println( getRes().getString( "  wrapper library not loaded." ) );
        }
    }
    
    /**
     * (Testing Method) raise a fail fast exception in native code.
     *  Useful for testing the Wrapper functions.
     *
     * Ignored when not running on Windows.
     *
     * @throws IllegalStateException If testing methods have been disabled by setting
     *                               the wrapper.disable_tests=true system property.
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("test.raiseFailFastExceptionNative")
     *                           permission.
     *
     * @see WrapperPermission
     */
    public static void raiseFailFastExceptionNative()
    {
        if ( m_disableTests )
        {
            throw new IllegalStateException( getRes().getString( "Test methods have been disabled." ) );
        }
        
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "test.raiseFailFastExceptionNative" ) );
        }
        
        m_outInfo.println( getRes().getString( "WARNING: Attempting to raise a fail fast exception in native code..." ) );
        if ( isNativeLibraryOk() )
        {
            nativeRaiseFailFastExceptionInner();
        
            m_outInfo.println( getRes().getString( "  Attempt to raise a native fail fast exception failed.  JVM is still alive." ) );
        }
        else
        {
            m_outInfo.println( getRes().getString( "  wrapper library not loaded." ) );
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
     * Returns true if the JVM should ignore user logoff events.  Mainly used
     *  within WrapperListener.controlEvent() method implemenations.
     
     * @return True if user logoff events should be ignroed.
     */
    public static boolean isIgnoreUserLogoffs()
    {
        return m_ignoreUserLogoffs;
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
     * Internal method to let Wrapper classes decide whether or not to write finalizer log output.
     */
    static boolean isLoggingFinalizers()
    {
        return m_logFinalizer;
    }
    
    /**
     * Start the Java side of the Wrapper code running.  This will make it
     *  possible for the native side of the Wrapper to detect that the Java
     *  Wrapper is up and running.
     * <p>
     * This method must be called on startup and then can only be called once
     *  so there is no reason for any security permission checks on this call.
     *
     * @param listener The WrapperListener instance which represents the
     *                 application being started.
     * @param args The argument list passed to the JVM when it was launched.
     */
    public static void start( final WrapperListener listener, final String[] args )
    {
        // As was done in the static initializer, we need to execute the following
        //  code in a privileged action so it is not necessary for the calling code
        //  to have the same privileges as the wrapper jar.
        // This is safe because this method can only be called once and that one call
        //  will presumably be made on JVM startup.
        AccessController.doPrivileged(
            new PrivilegedAction() {
                public Object run() {
                    privilegedStart( listener, args );
                    return null;
                }
            }
        );
    }
    
    /**
     * Called by the start method within a PrivilegedAction.
     *
     * @param WrapperListener The WrapperListener instance which represents
     *                        the application being started.
     * @param args The argument list passed to the JVM when it was launched.
     */
    private static void privilegedStart( WrapperListener listener, String[] args )
    {
        // Check the SecurityManager here as it is possible that it was set before this call.
        checkSecurityManager();
        
        // Just in case the user failed to provide an argument list, recover by creating one
        //  here.  This will avoid possible problems down stream.
        if ( args == null )
        {
            args = new String[0];
        }
        
        if ( m_debug )
        {
            StringBuffer sb = new StringBuffer();
            sb.append( "args[" );
            for ( int i = 0; i < args.length; i++ )
            {
                if ( i > 0 )
                {
                    sb.append( ", " );
                }
                sb.append( "\"" );
                sb.append( args[i] );
                sb.append( "\"" );
            }
            sb.append( "]" );
            
            m_outDebug.println( getRes().getString( "{0} called by thread: {1}",
                "WrapperManager.start(a " + listener.getClass().getName() + ", " + sb.toString() + ")",
                Thread.currentThread().getName() ) );
        }
        
        synchronized( WrapperManager.class )
        {  
            // Make sure that the class has not already been disposed.
            if ( m_disposed)
            {
                throw new IllegalStateException( getRes().getString( "WrapperManager has already been disposed." ) );
            }
            
            if ( m_listener != null )
            {
                throw new IllegalStateException( getRes().getString(
                    "WrapperManager has already been started with a WrapperListener." ) );
            }
            if ( listener == null )
            {
                throw new IllegalStateException( getRes().getString( "A WrapperListener must be specified." ) );
            }
            m_listener = listener;
            
            m_args = args;
            
            if ( m_debug )
            {
                Thread thisThread = Thread.currentThread();
                m_outDebug.println( getRes().getString( "Initial thread: {0} Priority: {1}", thisThread.getName(), new Integer( thisThread.getPriority() ) ) );
            }

            // Setup the thread to handle backend communications.
            startRunner();
            
            // If this JVM is being controlled by a native wrapper, then we want to
            //  wait for the command to start.  However, if this is a standalone
            //  JVM, then we want to start now.
            if ( !isControlledByNativeWrapper() )
            {
                startInner( true );
            }
        }
    }
    
    /**
     * Returns true if the JVM is in the process of shutting down.  This can be
     *  useful to avoid starting long running processes when it is known that the
     *  JVM will be shutting down shortly.
     *
     * @return true if the JVM is shutting down.
     */
    public static boolean isShuttingDown()
    {
        return m_stopping;
    }
    
    private static class ShutdownLock
        extends Object
    {
        private final Thread m_thread;
        private int m_count;
        
        private ShutdownLock( Thread thread )
        {
            m_thread = thread;
        }
    }
    
    /**
     * Increase the number of locks which will prevent the Wrapper from letting
     *  the JVM process exit on shutdown.   This is primarily useful around
     *  calls to native JNI functions in daemon threads where it has been shown
     *  that premature JVM exits can cause the JVM process to crash on shutdown.
     * <p>
     * Normal non-daemon threads should not require these locks as the very
     *  fact that the non-daemon thread is still running will prevent the JVM
     *  from shutting down.
     * <p>
     * It is possible to make multiple calls within a single thread.  Each call
     *  should always be paired with a call to releaseShutdownLock().
     *
     * @throws WrapperShuttingDownException If called after the Wrapper has
     *                                      already begun the shutdown of the
     *                                      JVM.
     */
    public static void requestShutdownLock()
        throws WrapperShuttingDownException
    {
        synchronized( WrapperManager.class )
        {
            if ( m_stopping )
            {
                throw new WrapperShuttingDownException();
            }
            
            Thread thisThread = Thread.currentThread();
            ShutdownLock lock = (ShutdownLock)m_shutdownLockMap.get( thisThread );
            if ( lock == null )
            {
                lock = new ShutdownLock( thisThread );
                m_shutdownLockMap.put( thisThread, lock );
            }
            lock.m_count++;
            m_shutdownLocks++;
            
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "{0} called by thread: {1} (New thread lock count: {2}, total lock count: {3})",
                    "WrapperManager.requestShutdownLock()",
                    thisThread.getName(), new Integer( lock.m_count ), new Integer( m_shutdownLocks ) ) );
            }
        }
    }
    
    /**
     * Called by a thread which has previously called requestShutdownLock().
     *
     * @throws IllegalStateException If called without first calling requestShutdownLock() from
     *                               the same thread.
     */
    public static void releaseShutdownLock()
        throws IllegalStateException
    {
        synchronized( WrapperManager.class )
        {
            Thread thisThread = Thread.currentThread();
            ShutdownLock lock = (ShutdownLock)m_shutdownLockMap.get( thisThread );
            if ( lock == null )
            {
                throw new IllegalStateException( getRes().getString( "requestShutdownLock was not called from this thread." ) );
            }
            
            lock.m_count--;
            m_shutdownLocks--;
            
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "{0} called by thread: {1} (New thread lock count: {2}, total lock count: {3})",
                    "WrapperManager.releaseShutdownLock()",
                    thisThread.getName(), new Integer( lock.m_count ), new Integer( m_shutdownLocks ) ) );
            }
            
            if ( lock.m_count <= 0 )
            {
                m_shutdownLockMap.remove( thisThread );
            }
            
            WrapperManager.class.notify();
        }
    }
    
    /**
     * Waits for any outstanding locks to be released before shutting down.
     */
    private static void waitForShutdownLocks()
    {
        synchronized( WrapperManager.class )
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( 
                     "wait for {0} shutdown locks to be released.", new Integer(m_shutdownLocks ) ) );
            }
            
            while ( m_shutdownLocks > 0 )
            {
                try
                {
                    WrapperManager.class.wait( 5000 );
                }
                catch ( InterruptedException e )
                {
                    // Ignore and continue.
                }
                
                if ( m_shutdownLocks > 0 )
                {
                    m_outInfo.println( getRes().getString(
                        "Waiting for {0} shutdown locks to be released..." ,                         new Integer(m_shutdownLocks ) ) );
                }
            }
        }
    }
    
    /**
     * Tells the native wrapper that the JVM wants to restart, then informs
     *  all listeners that the JVM is about to shutdown before killing the JVM.
     * <p>
     * This method will not return.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("restart") permission.
     *
     * @see WrapperPermission
     */
    public static void restart()
        throws SecurityException
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "restart" ) );
        }
        
        m_stoppingInit = true;
        
        if ( m_debug )
        {
            m_outDebug.println( getRes().getString( "{0} called by thread: {1}",
                "WrapperManager.restart()",
                Thread.currentThread().getName() ) );
        }
        
        restartInner();
    }
    
    /**
     * Tells the native wrapper that the JVM wants to restart, then informs
     *  all listeners that the JVM is about to shutdown before killing the JVM.
     * <p>
     * This method requests that the JVM be restarted but then returns.  This
     *  allows components to initiate a JVM exit and then continue, allowing
     *  a normal shutdown initiated by the JVM via shutdown hooks.  In
     *  applications which are designed to be shutdown when the user presses
     *  CTRL-C, this may result in a cleaner shutdown.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("restart") permission.
     *
     * @see WrapperPermission
     */
    public static void restartAndReturn()
        throws SecurityException
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "restart" ) );
        }
        
        m_stoppingInit = true;
        
        synchronized( WrapperManager.class )
        {
            if ( m_stopping )
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "{0} called by thread: {1} (already stopping)",
                        "WrapperManager.restartAndReturn()",
                        Thread.currentThread().getName() ) );
                }
                return;
            }
            else
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "{0} called by thread: {1}",
                        "WrapperManager.restartAndReturn()",
                        Thread.currentThread().getName() ) );
                }
            }
        }
        
        
        // To make this possible, we have to create a new thread to actually do the shutdown.
        Thread restarter = new Thread( "Wrapper-Restarter" )
        {
            public void run()
            {
                restartInner();
            }
        };
        restarter.setDaemon( false );
        restarter.start();
    }
    
    /**
     * Common code used to restart the JVM.  It is assumed that the calling
     *  thread has has passed security checks before this is called.
     */
    private static void restartInner()
    {
        boolean stopping;
        synchronized( WrapperManager.class )
        {
            stopping = m_stopping;
            if ( !stopping )
            {
                m_stopping = true;
            }
        }
        
        if ( !stopping )
        {
            // Always send the restart command
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
        
        // This is safe because we are already checking for the privilege to restart the JVM
        //  above.  If we get this far then we want the Wrapper to be able to do everything
        //  necessary to stop the JVM.
        AccessController.doPrivileged(
            new PrivilegedAction() {
                public Object run() {
                    privilegedStopInner( 0 );
                    return null;
                }
            }
        );
    }
    
    /**
     * Tells the native wrapper that the JVM wants to shut down, then informs
     *  all listeners that the JVM is about to shutdown before killing the JVM.
     * <p>
     * This method will not return.
     *
     * @param exitCode The exit code that the Wrapper will return when it exits.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("stop") permission.
     *
     * @see WrapperPermission
     */
    public static void stop( final int exitCode )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "stop" ) );
        }
        
        m_stoppingInit = true;
        
        if ( m_debug )
        {
            m_outDebug.println( getRes().getString( "{0} called by thread: {1}",
                "WrapperManager.stop(" + exitCode + ")",
                Thread.currentThread().getName() ) );
        }
        
        stopCommon( exitCode, 1000 );
        
        // This is safe because we are already checking for the privilege to stop the JVM
        //  above.  If we get this far then we want the Wrapper to be able to do everything
        //  necessary to stop the JVM.
        AccessController.doPrivileged(
            new PrivilegedAction() {
                public Object run() {
                    privilegedStopInner( exitCode );
                    return null;
                }
            }
        );
    }
    
    /**
     * Tells the native wrapper that the JVM wants to shut down, then informs
     *  all listeners that the JVM is about to shutdown before killing the JVM.
     * <p>
     * This method requests that the JVM be shutdown but then returns.  This
     *  allows components to initiate a JVM exit and then continue, allowing
     *  a normal shutdown initiated by the JVM via shutdown hooks.  In
     *  applications which are designed to be shutdown when the user presses
     *  CTRL-C, this may result in a cleaner shutdown.
     *
     * @param exitCode The exit code that the Wrapper will return when it exits.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("stop" ) permission.
     *
     * @see WrapperPermission
     */
    public static void stopAndReturn( final int exitCode )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "stop" ) );
        }
        
        m_stoppingInit = true;
        
        synchronized( WrapperManager.class )
        {
            if ( m_stopping )
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "{0} called by thread: {1} (already stopping)",
                        "WrapperManager.stopAndReturn(" + exitCode + ")",
                        Thread.currentThread().getName() ) );
                }
                return;
            }
            else
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "{0} called by thread: {1}",
                        "WrapperManager.stopAndReturn(" + exitCode + ")",
                        Thread.currentThread().getName() ) );
                }
            }
        }
        
        // To make this possible, we have to create a new thread to actually do the shutdown.
        Thread stopper = new Thread( "Wrapper-Stopper" )
        {
            public void run()
            {
                stopCommon( exitCode, 1000 );
                
                // This is safe because we are already checking for the privilege to stop the JVM
                //  above.  If we get this far then we want the Wrapper to be able to do everything
                //  necessary to stop the JVM.
                AccessController.doPrivileged(
                    new PrivilegedAction() {
                        public Object run() {
                            privilegedStopInner( exitCode );
                            return null;
                        }
                    }
                );
            }
        };
        stopper.setDaemon( false );
        stopper.start();
    }

    /**
     * Tells the native wrapper that the JVM wants to shut down and then
     *  promptly halts.  Be careful when using this method as an application
     *  will not be given a chance to shutdown cleanly.
     *
     * @param exitCode The exit code that the Wrapper will return when it exits.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("stopImmediate") permission.
     *
     * @see WrapperPermission
     */
    public static void stopImmediate( final int exitCode )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "stopImmediate" ) );
        }
        
        if ( m_debug )
        {
            m_outDebug.println( getRes().getString( "{0} called by thread: {1}",
                "WrapperManager.stopImmediate(" + exitCode + ")",
                Thread.currentThread().getName() ) );
        }
        
        stopCommon( exitCode, 250 );
        
        signalStopped( exitCode );
        
        // Really all done.
        if ( m_debug )
        {
            m_outDebug.println( getRes().getString( "WrapperManager stopped due to {0}", getRes().getString( "Halt" ) ) );
        }
        m_stopped = true;
        
        Runtime.getRuntime().halt( exitCode );
    }
    
    /**
     * Signal the native wrapper that the startup is progressing but that more
     *  time is needed.  The current startup timeout will be extended if
     *  necessary so it will be at least 'waitHint' milliseconds in the future.
     * <p>
     * This call will have no effect if the current startup timeout is already
     *  more than 'waitHint' milliseconds in the future.
     *
     * @param waitHint Time in milliseconds to allow for the startup to
     *                 complete.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("signalStarting") permission.
     *
     * @see WrapperPermission
     */
    public static void signalStarting( int waitHint )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "signalStarting" ) );
        }
        
        sendCommand( WRAPPER_MSG_START_PENDING, Integer.toString( waitHint ) );
    }

    /**
     * Signal the native wrapper that the shutdown is progressing but that more
     *  time is needed.  The current shutdown timeout will be extended if
     *  necessary so it will be at least 'waitHint' milliseconds in the future.
     * <p>
     * This call will have no effect if the current shutdown timeout is already
     *  more than 'waitHint' milliseconds in the future.
     *
     * @param waitHint Time in milliseconds to allow for the shutdown to
     *                 complete.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("signalStopping") permission.
     *
     * @see WrapperPermission
     */
    public static void signalStopping( int waitHint )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "signalStopping" ) );
        }
        
        m_stopping = true;
        sendCommand( WRAPPER_MSG_STOP_PENDING, Integer.toString( waitHint ) );
    }
    
    /**
     * This method should not normally be called by user code as it is called
     *  from within the stop and restart methods.  However certain applications
     *  which stop the JVM may need to call this method to let the wrapper code
     *  know that the shutdown was intentional.
     *
     * @param exitCode The exit code.
     * 
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("signalStopped") permission.
     *
     * @see WrapperPermission
     */
    public static void signalStopped( int exitCode )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "signalStopped" ) );
        }
        
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
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("log") permission.
     *
     * @see WrapperPermission
     */
    public static void log( int logLevel, String message )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "log" ) );
        }
        
        // Make sure that the logLevel is valid to avoid problems with the
        //  command sent to the server.
        
        if ( ( logLevel < WRAPPER_LOG_LEVEL_DEBUG ) || ( logLevel > WRAPPER_LOG_LEVEL_NOTICE ) )
        {
            throw new IllegalArgumentException( getRes().getString( "The specified logLevel is not valid." ) );
        }
        if ( message == null )
        {
            throw new IllegalArgumentException( getRes().getString( "The message parameter can not be null." ) );
        }
        
        if ( m_lowLogLevel <= logLevel )
        {
            sendCommand( (byte)( WRAPPER_MSG_LOG + logLevel ), message );
        }
    }
    
    /**
     * Returns an array of all registered services.  This method is only
     *  supported on Windows platforms which support services.  Calling this
     *  method on other platforms will result in null being returned.
     *
     * @return An array of services.
     *
     * @throws SecurityException If a SecurityManager has not been set in the
     *                           JVM or if the calling code has not been
     *                           granted the WrapperPermission "listServices"
     *                           permission.  A SecurityManager is required
     *                           for this operation because this method makes
     *                           it possible to learn a great deal about the
     *                           state of the system.
     *
     * @see WrapperPermission
     */
    public static WrapperWin32Service[] listServices()
        throws SecurityException
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm == null )
        {
            throw new SecurityException( getRes().getString( "A SecurityManager has not yet been set." ) );
        }
        else
        {
            sm.checkPermission( new WrapperPermission( "listServices" ) );
        }
        
        if ( isNativeLibraryOk() )
        {
            return nativeListServices();
        }
        else
        {
            return null;
        }
    }
    
    /**
     * Sends a service control code to the specified service.  The state of the
     *  service should be tested on return.  If the service was not currently
     *  running then the control code will not be sent.
     * <p>
     * The control code sent can be one of the system control codes:
     *  WrapperManager.SERVICE_CONTROL_CODE_START,
     *  WrapperManager.SERVICE_CONTROL_CODE_STOP,
     *  WrapperManager.SERVICE_CONTROL_CODE_PAUSE,
     *  WrapperManager.SERVICE_CONTROL_CODE_CONTINUE, or
     *  WrapperManager.SERVICE_CONTROL_CODE_INTERROGATE.  In addition, user
     *  defined codes in the range 128-255 can also be sent.
     *
     * @param serviceName Name of the Windows service which will receive the
     *                    control code.
     * @param controlCode The actual control code to be sent.  User defined
     *                    control codes should be in the range 128-255.
     *
     * @return A WrapperWin32Service containing the last known status of the
     *         service after sending the control code.  This will be null if
     *         the currently platform is not a version of Windows which
     *         supports services.
     *
     * @throws WrapperServiceException If there are any problems accessing the
     *                                 specified service.
     * @throws SecurityException If a SecurityManager has not been set in the
     *                           JVM or if the calling code has not been
     *                           granted the WrapperServicePermission
     *                           permission for the specified service and
     *                           control code.  A SecurityManager is required
     *                           for this operation because this method makes
     *                           it possible to control any service on the
     *                           system, which is of course rather dangerous.
     *
     * @see WrapperServicePermission
     */
    public static WrapperWin32Service sendServiceControlCode( String serviceName, int controlCode )
        throws WrapperServiceException, SecurityException
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm == null )
        {
            throw new SecurityException( getRes().getString( "A SecurityManager has not yet been set." ) );
        }
        else
        {
            String action;
            switch( controlCode )
            {
            case SERVICE_CONTROL_CODE_START:
                action = WrapperServicePermission.ACTION_START;
                break;
                
            case SERVICE_CONTROL_CODE_STOP:
                action = WrapperServicePermission.ACTION_STOP;
                break;
                
            case SERVICE_CONTROL_CODE_PAUSE:
                action = WrapperServicePermission.ACTION_PAUSE;
                break;
                
            case SERVICE_CONTROL_CODE_CONTINUE:
                action = WrapperServicePermission.ACTION_CONTINUE;
                break;
                
            case SERVICE_CONTROL_CODE_INTERROGATE:
                action = WrapperServicePermission.ACTION_INTERROGATE;
                break;
                
            default:
                if ( ( controlCode >= 128 ) && ( controlCode <= 255 ) ) {
                    action = WrapperServicePermission.ACTION_USER_CODE;
                } else {
                    throw new IllegalArgumentException( getRes().getString( "The specified controlCode is invalid." ) );
                }
                break;
            }
            
            sm.checkPermission( new WrapperServicePermission( serviceName, action ) );
        }
        
        WrapperWin32Service service = null;
        if ( isNativeLibraryOk() )
        {
            service = nativeSendServiceControlCode( serviceName, controlCode );
        }
        return service;
    }
    
    /**
     * Adds a WrapperEventListener which will receive WrapperEvents.  The
     *  specific events can be controlled using the mask parameter.  This API
     *  was chosen to allow for additional events in the future.
     *
     * To avoid future compatibility problems, WrapperEventListeners should
     *  always test the class of an event before making use of it.  This will
     *  avoid problems caused by new event classes added in future versions
     *  of the Wrapper.
     *
     * This method should only be called once for a given WrapperEventListener.
     *  Build up a single mask to receive events of multiple types.
     *
     * @param listener WrapperEventListener to be start receiving events.
     * @param mask A mask specifying the event types that the listener is
     *             interrested in receiving.  See the WrapperEventListener
     *             class for a full list of flags.  A mask is created by
     *             combining multiple flags using the binary '|' OR operator.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the appropriate
     *                           WrapperEventPermission(...) permission.
     *
     * @see WrapperEventPermission
     */
    public static void addWrapperEventListener( WrapperEventListener listener, long mask )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            StringBuffer sb = new StringBuffer();
            boolean first = true;
            if ( ( mask & WrapperEventListener.EVENT_FLAG_SERVICE ) != 0 )
            {
                first = false;
                sb.append( WrapperEventPermission.EVENT_TYPE_SERVICE );
            }
            if ( ( mask & WrapperEventListener.EVENT_FLAG_CONTROL ) != 0 )
            {
                if ( first )
                {
                    first = false;
                }
                else
                {
                    sb.append( "," );
                }
                sb.append( WrapperEventPermission.EVENT_TYPE_CONTROL );
            }
            if ( ( mask & WrapperEventListener.EVENT_FLAG_REMOTE_CONTROL ) != 0 )
            {
                first = false;
                sb.append( WrapperEventPermission.EVENT_TYPE_REMOTE_CONTROL );
            }
            if ( ( mask & WrapperEventListener.EVENT_FLAG_CORE ) != 0 )
            {
                if ( first )
                {
                    first = false;
                }
                else
                {
                    sb.append( "," );
                }
                sb.append( WrapperEventPermission.EVENT_TYPE_CORE );
            }
            sm.checkPermission( new WrapperEventPermission( sb.toString() ) );
        }
        
        synchronized( WrapperManager.class )
        {
            WrapperEventListenerMask listenerMask = new WrapperEventListenerMask();
            listenerMask.m_listener = listener;
            listenerMask.m_mask = mask;
            
            m_wrapperEventListenerMaskList.add( listenerMask );
            m_wrapperEventListenerMasks = null;
        }
        
        updateWrapperEventListenerFlags();
    }
    
    /**
     * Removes a WrapperEventListener so it will not longer receive WrapperEvents.
     *
     * @param listener WrapperEventListener to be stop receiving events.
     *
     * @throws SecurityException If a SecurityManager is present and the
     *                           calling thread does not have the
     *                           WrapperPermission("removeWrapperEventListener")
     *                           permission.
     *
     * @see WrapperPermission
     */
    public static void removeWrapperEventListener( WrapperEventListener listener )
    {
        SecurityManager sm = System.getSecurityManager();
        if ( sm != null )
        {
            sm.checkPermission( new WrapperPermission( "removeWrapperEventListener" ) );
        }
        
        synchronized( WrapperManager.class )
        {
            // Look for the first instance of a given listener in the list.
            for ( Iterator iter = m_wrapperEventListenerMaskList.iterator(); iter.hasNext(); )
            {
                WrapperEventListenerMask listenerMask = (WrapperEventListenerMask)iter.next();
                if ( listenerMask.m_listener == listener )
                {
                    iter.remove();
                    m_wrapperEventListenerMasks = null;
                    break;
                }
            }
        }
        
        updateWrapperEventListenerFlags();
    }
    
    /**
     * Returns the Log file currently being used by the Wrapper.  If log file
     *  rolling is enabled in the Wrapper then this file may change over time.
     *
     * @throws IllegalStateException If this method is called before the Wrapper
     *                               instructs this class to start the user
     *                               application.
     * 
     * @return The Wrapper log file.
     */
    public static File getWrapperLogFile()
    {
        File logFile = m_logFile;
        if ( logFile == null )
        {
            throw new IllegalStateException( getRes().getString( "Not yet initialized." ) );
        }
        return logFile;
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
     * Checks for the existence of a SecurityManager and then makes sure that
     *  the Wrapper jar has been granted AllPermissions.  If not then a warning
     *  will be displayed as this will most likely result in the Wrapper
     *  failing to function correctly.
     *
     * This method is called at various points in the startup as it is possible
     *  and in fact likely that any SecurityManager will be set by user code
     *  during or shortly after initialization.  Once a SecurityManager has
     *  been located and tested then this method will become a noop.
     */
    private static void checkSecurityManager()
    {
        if ( m_securityManagerChecked )
        {
            return;
        }
        
        SecurityManager securityManager = System.getSecurityManager();
        if ( securityManager != null )
        {
            if ( m_debug )
            {
                m_outDebug.println(getRes().getString( "Detected a SecurityManager: {0} " , 
                    securityManager.getClass().getName() ) );
            }
            
            try
            {
                securityManager.checkPermission( new java.security.AllPermission() );
            }
            catch ( SecurityException e )
            {
                m_outDebug.println();
                m_outDebug.println( getRes().getString(
                    "WARNING - Detected that a SecurityManager has been installed but the " ) );
                m_outDebug.println( getRes().getString(
                    "          wrapper.jar has not been granted the java.security.AllPermission" ) );
                m_outDebug.println( getRes().getString(
                    "          permission.  This will most likely result in SecurityExceptions" ) );
                m_outDebug.println( getRes().getString(
                    "          being thrown by the Wrapper." ) );
                m_outDebug.println();
            }
            
            // Always set the flag.
            m_securityManagerChecked = true;
        }
    }
    
    /**
     * Returns an array of WrapperEventListenerMask instances which can
     *  be safely used outside of synchronization.
     *
     * @return An array of WrapperEventListenerMask instances.
     */
    private static WrapperEventListenerMask[] getWrapperEventListenerMasks()
    {
        WrapperEventListenerMask[] listenerMasks = m_wrapperEventListenerMasks;
        if ( listenerMasks == null )
        {
            synchronized( WrapperManager.class )
            {
                if ( listenerMasks == null )
                {
                    listenerMasks =
                        new WrapperEventListenerMask[m_wrapperEventListenerMaskList.size()];
                    m_wrapperEventListenerMaskList.toArray( listenerMasks );
                    m_wrapperEventListenerMasks = listenerMasks;
                }
            }
        }
        
        return listenerMasks;
    }
    
    /**
     * Updates the internal flags based on the WrapperEventListeners currently
     *  registered.
     */
    private static void updateWrapperEventListenerFlags()
    {
        boolean core = false;
        
        WrapperEventListenerMask[] listenerMasks = getWrapperEventListenerMasks();
        for ( int i = 0; i < listenerMasks.length; i++ )
        {
            long mask = listenerMasks[i].m_mask;
            
            // See whether particular event types are required.
            core = core | ( ( mask & WrapperEventListener.EVENT_FLAG_CORE ) != 0 );
        }
        
        m_produceCoreEvents = core;
    }
    
    /**
     * Notifies registered listeners that an event has been fired.
     *
     * @param event Event to notify the listeners of.
     */
    private static void fireWrapperEvent( WrapperEvent event )
    {
        long eventMask = event.getFlags();
        
        WrapperEventListenerMask[] listenerMasks = getWrapperEventListenerMasks();
        for ( int i = 0; i < listenerMasks.length; i++ )
        {
            long listenerMask = listenerMasks[i].m_mask;
            
            // See if the event should be passed to this listner.
            if ( ( listenerMask & eventMask ) != 0 )
            {
                // The listener wants the event.
                WrapperEventListener listener = listenerMasks[i].m_listener;
                try
                {
                    listener.fired( event );
                }
                catch ( Throwable t )
                {
                    m_outError.println( getRes().getString( "Encountered an uncaught exception while notifying WrapperEventListener of an event:" ) );
                    t.printStackTrace( m_outError );
                }
            }
        }
    }
    
    /**
     * Executed code common to the stop and stopImmediate methods.
     */
    private static void stopCommon( int exitCode, int delay )
    {
        boolean stopping;
        synchronized( WrapperManager.class )
        {
            stopping = m_stopping;
            if ( !stopping )
            {
                m_stopping = true;
            }
        }
        
        if ( !stopping )
        {
            // Always send the stop command
            sendCommand( WRAPPER_MSG_STOP, Integer.toString( exitCode ) );
            
            if ( delay > 0 )
            {
                // Give the Wrapper a chance to register the stop command before stopping.
                // This avoids any errors thrown by the Wrapper because the JVM died before
                //  it was expected to.
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "Pausing for {0}ms to allow a clean shutdown...", new Integer( delay ) ) );
                }
                try
                {
                    Thread.sleep( delay );
                }
                catch ( InterruptedException e )
                {
                }
            }
        }
    }
    
    /**
     * Dispose of all resources used by the WrapperManager.  Closes the server
     *  socket which is used to listen for events from the Wrapper process.
     */
    private static void dispose()
    {
        synchronized( WrapperManager.class )
        {
            m_disposed = true;
            
            // Close the open backend if it exists.
            closeBackend();
            
            // Give the Connection Thread a chance to stop itself.
            try
            {
                Thread.sleep( 500 );
            }
            catch ( InterruptedException e )
            {
            }
            
            // Dispose the Native resources
            nativeDispose( m_debug );
        }
    }
    
    /**
     * Called by startInner when the WrapperListner.start method has completed.
     *
     * Only called when WrapperManager.class is synchronized.
     */
    private static void startCompleted()
    {
        m_startedTicks = getTicks();
        
        // Let the startup thread die since the application has been started.
        m_startupRunner = null;
        
        // Check the SecurityManager here as it is possible that it was set in the
        //  listener's start method.
        checkSecurityManager();
        
        // Signal that the application has started.
        signalStarted();
        
        // Wake up any threads waiting for this.
        WrapperManager.class.notifyAll();
    }
    
    /**
     * Informs the listener that it should start.
     *
     * WrapperManager.class will be synchronized when called.
     *
     * @param block True if this call should block for the WrapperListener.start
     *              method to complete.  This is true when java is being run in
     *              standalone mode without the Wrapper.
     */
    private static void startInner( boolean block )
    {
        m_starting = true;
        
        // Do any setup which should happen just before we actually start the application.
        checkTmpDir();
        
        // This method can be called from the connection thread which must be a
        //  daemon thread by design.  We need to call the WrapperListener.start method
        //  from a non-daemon thread.  This means that if the current thread is a
        //  daemon we need to launch a new thread while we wait for the start method
        //  to return.
        if ( m_listener == null )
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "No WrapperListener has been set.  Nothing to start." ) );
            }
            
            startCompleted();
        }
        else
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "calling WrapperListener.start()" ) );
            }
            
            // These arrays aren't pretty, but we need final variables for the inline
            //  class and this makes it possible to get the values back.
            final Integer[] resultF = new Integer[1];
            final Throwable[] tF = new Throwable[1];
            
            // Start in a dedicated thread.
            Thread startRunner = new Thread( "WrapperListener_start_runner" )
            {
                public void run()
                {
                    if ( m_debug )
                    {
                       m_outDebug.println( getRes().getString( "WrapperListener.start runner thread started." ) );
                    }
                    
                    // If the calling thread was the Backend handler then it is running at a higher priority.
                    //  We need to restore the thread priority to the default before proceeding to make sure
                    //  that the rest of the application runs at the correct priority.
                    Thread thisThread = Thread.currentThread();
                    thisThread.setPriority( Thread.NORM_PRIORITY );
                    if ( m_debug )
                    {
                        m_outDebug.println( getRes().getString( "Application start main thread: {0} Priority: {1}", thisThread.getName(), new Integer( thisThread.getPriority() ) ) );
                    }
                    
                    try
                    {
                        // This is user code, so don't trust it.
                        try
                        {
                            resultF[0] = m_listener.start( m_args );
                        }
                        catch ( Throwable t )
                        {
                            tF[0] = t;
                        }
                    }
                    finally
                    {
                        // Make sure the rest of this thread does not fall behind the application.
                        thisThread.setPriority( Thread.MAX_PRIORITY );
                        
                        // Now that we are back, handle the results.
                        if ( tF[0] != null )
                        {
                            m_outError.println( getRes().getString(
                                    "Error in WrapperListener.start callback.  {0}", tF[0] ) );
                            tF[0].printStackTrace( m_outError );
                            // Kill the JVM, but don't tell the wrapper that we want to stop.
                            //  This may be a problem with this instantiation only.
                            privilegedStopInner( 1 );
                            // Won't make it here.
                            return;
                        }
                        
                        if ( m_debug )
                        {
                            m_outDebug.println( getRes().getString( "returned from WrapperListener.start()" ) );
                        }
                        if ( resultF[0] != null )
                        {
                            int exitCode = resultF[0].intValue();
                            if ( m_debug )
                            {
                                m_outDebug.println( getRes().getString(
                                    "WrapperListener.start() returned an exit code of {0}.",
                                    new Integer( exitCode )  ) );
                            }
                            
                            // Signal the native code.
                            WrapperManager.stop( exitCode );
                            // Won't make it here.
                            return;
                        }
                        
                        synchronized( WrapperManager.class )
                        {
                            startCompleted();
                        }
                        
                        if ( m_debug )
                        {
                           m_outDebug.println( getRes().getString( "WrapperListener.start runner thread stopped." ) );
                        }
                    }
                }
            };
            startRunner.setDaemon( false );
            startRunner.start();
            
            if ( block )
            {
                // Wait for the start runner to complete.
                if ( m_debug )
                {
                   m_outDebug.println( getRes().getString(
                        "Waiting for WrapperListener.start runner thread to complete." ) );
                }
                while ( ( startRunner != null ) && ( startRunner.isAlive() ) )
                {
                    try
                    {
                        WrapperManager.class.wait();
                    }
                    catch ( InterruptedException e )
                    {
                        // Ignore and keep waiting.
                    }
                }
            }
        }
    }
    
    private static void shutdownJVM( int exitCode )
    {
        if ( m_debug )
        {
            m_outDebug.println(getRes().getString( "shutdownJVM({0}) Thread: {1}",
                new Integer( exitCode ), Thread.currentThread().getName() ) );
        }
        
        // Make sure that any shutdown locks are released.
        waitForShutdownLocks();
        
        // Signal that the application has stopped and the JVM is about to shutdown.
        signalStopped( exitCode );
        
        // Dispose the wrapper.
        dispose();
        
        m_shutdownJVMComplete = true;
        
        // Do not call System.exit if this is the ShutdownHook
        if ( Thread.currentThread() == m_hook )
        {
            // This is the shutdown hook, so fall through because things are
            //  already shutting down.
        }
        else
        {
            if ( m_debug )
            {
                m_outDebug.println(getRes().getString( "calling System.exit({0})", 
                         new Integer( exitCode )  ) );
            }
            safeSystemExit( exitCode );
        }
    }
    
    /**
     * A user ran into a JVM bug where a call to System exit was causing an
     *  IllegalThreadStateException to be thrown.  Not sure how widespread
     *  this problem is.  But it is easy to avoid it causing serious problems
     *  for the wrapper.
     */
    private static void safeSystemExit( int exitCode )
    {
        // Really all done.
        if ( m_debug )
        {
            m_outDebug.println( getRes().getString( "WrapperManager stopped due to {0}", getRes().getString( "System Exit" ) ) );
        }
        m_stopped = true;
        
        try
        {
            System.exit( exitCode );
        }
        catch ( IllegalThreadStateException e )
        {
            m_outError.println( getRes().getString(
                    "Attempted System.exit({0}) call failed: {1}" ,
                    new Integer( exitCode ), e.toString() ) );
            
            m_outError.println( getRes().getString( "   Trying Runtime.halt({0})",
                    new Integer( exitCode ) ) );
            Runtime.getRuntime().halt( exitCode );
        }
    }
    
    /**
     * Informs the listener that the JVM will be shut down.
     *
     * This should only be called from within a PrivilegedAction or in a
     *  context that came from a PrivilegedAction.
     */
    private static void privilegedStopInner( int exitCode )
    {
        boolean block;
        synchronized( WrapperManager.class )
        {
            // Always set the stopping flag.  (Depending on how the shutdown was initiated, it may already be set.)
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
                    throw new IllegalStateException( getRes().getString(
                        "WrapperManager.stop() can not be called recursively." ) );
                }
                
                block = true;
            }
        }
        
        if ( block )
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString(
                        "Thread, {0}, waiting for the JVM to exit.", 
                    Thread.currentThread().getName() ) );
                
                if ( Thread.currentThread() == m_hook )
                {
                    if ( !m_hookRemoveFailed )
                    {
                        m_outDebug.println( getRes().getString( "System.exit appears to have been called from within the\n  WrapperListener.stop() method.  If possible the application\n  should be modified to avoid this behavior.\n  To avoid a deadlock, this thread will only wait 5 seconds\n  for the application to shutdown.  This may result in the\n  application failing to shutdown completely before the JVM\n  exits.  Removing the offending System.exit call will\n  resolve this." ) );
                    }
                }
            }
            
            // This thread needs to be put into an infinite loop until the JVM exits.
            //  This thread can not be allowed to return to the caller, but another
            //  thread is already responsible for shutting down the JVM, so this
            //  one can do nothing but wait.
            int loops = 0;
            int wait = 50;
            while( true )
            {
                try
                {
                    Thread.sleep( wait );
                }
                catch ( InterruptedException e )
                {
                }
                
                // If this is the wrapper's shutdown hook then we only want to loop until
                //  the shutdownJVM method has completed.  We will only get into this state
                //  if user code calls System.exit from within the WrapperListener.stop
                //  method.  Failing to return here will cause the shutdown process to hang.
                // If the user code calls System.exit directly in the stop method then the
                //  m_shutdownJVMComplete flag will never be set.   Always time out after
                //  5 seconds so the JVM will not hang in such cases.
                if ( Thread.currentThread() == m_hook )
                {
                    if ( m_shutdownJVMComplete || ( loops > 5000 / wait ) )
                    {
                        if ( !m_shutdownJVMComplete )
                        {
                            if ( m_debug )
                            {
                                m_outDebug.println( getRes().getString(
                                        "Thread, {0}, continuing after 5 seconds.", 
                                    Thread.currentThread().getName() ) );
                            }
                        }
                        
                        // To keep the wrapper from showing a JVM exited unexpectedly message
                        //  on shutdown, tell the wrapper that we are ready to stop.
                        // If the WrapperListener.stop method is taking a long time, we will
                        //  also get here.  In that case, the Wrapper will still wait for
                        //  the configured exit timeout before killing the JVM process.
                        // In theory, the shutdown process of an application will only call
                        //  System.exit after the shutdown is complete so this should be Ok.
                        // Use the exit code from the thread which initiated the call rather
                        //  than this call as that one is the one we really want.
                        signalStopped( m_exitCode );
                        
                        return;
                    }
                }
                
                loops++;
            }
        }
        
        if ( m_debug )
        {
            m_outDebug.println( getRes().getString(
                    "Thread, {0}, handling the shutdown process.",
                     Thread.currentThread().getName() ) );
        }
        m_exitCode = exitCode;
        
        // If appropriate, unregister the shutdown hook.  This must be done before the
        //  user stop code is called to avoid nested shutdowns.
        if ( Thread.currentThread() != m_hook )
        {
            // We do not want the ShutdownHook to execute, so unregister it before calling exit.
            //  It can't be unregistered if it has already fired however.  The only way that this
            //  could happen is if user code calls System.exit from within the listener stop
            //  method.
            if ( ( !m_hookTriggered ) && ( m_hook != null ) )
            {
                // Remove the shutdown hook.
                try
                {
                    Runtime.getRuntime().removeShutdownHook( m_hook );
                }
                catch ( AccessControlException e )
                {
                    // This can happen if the security policy is not setup correctly.
                    m_outError.println( getRes().getString( "Unable to remove the Wrapper''s shutdownhook: {0}", e ) );
                    m_hookRemoveFailed = true;
                }
            }
        }
        
        // Only stop the listener if the app has been asked to start.  Does not need to have actually started.
        int code = exitCode;
        
        if ( ( m_listenerForceStop && m_starting ) || m_started )
        {
            // This method can be called from the connection thread which must be a
            //  daemon thread by design.  We need to call the WrapperListener.stop method
            //  from a non-daemon thread.  This means that if the current thread is a
            //  daemon we need to launch a new thread while we wait for the stop method
            //  to return.
            if ( m_listener == null )
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "No WrapperListener has been set.  Nothing to stop." ) );
                }
            }
            else
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "calling listener.stop()" ) );
                }
                
                if ( Thread.currentThread().isDaemon() )
                {
                    // This array isn't pretty, but we need final variables for the inline
                    //  class and this makes it possible to get the values back.
                    final Integer[] codeF = new Integer[] {new Integer(code)};
                    
                    // Start in a dedicated thread.
                    Thread stopRunner = new Thread( "WrapperListener_stop_runner" )
                    {
                        public void run()
                        {
                            if ( m_debug )
                            {
                               m_outDebug.println( getRes().getString( "WrapperListener.stop runner thread started." ) );
                            }
                            
                            // If the calling thread was the Backend handler then it is running at a higher priority.
                            //  We need to restore the thread priority to the default before proceeding to make sure
                            //  that the rest of the application runs at the correct priority.
                            Thread thisThread = Thread.currentThread();
                            thisThread.setPriority( Thread.NORM_PRIORITY );
                            if ( m_debug )
                            {
                                m_outDebug.println( getRes().getString( "Application stop main thread: {0} Priority: {1}", thisThread.getName(), new Integer( thisThread.getPriority() ) ) );
                            }
                            
                            try
                            {
                                // This is user code, so don't trust it.
                                try
                                {
                                    codeF[0] = new Integer( m_listener.stop( codeF[0].intValue() ) );
                                }
                                catch ( Throwable t )
                                {
                                    m_outError.println( getRes().getString(
                                        "Error in WrapperListener.stop callback." ) );
                                    t.printStackTrace( m_outError );
                                }
                            }
                            finally
                            {
                                if ( m_debug )
                                {
                                   m_outDebug.println( getRes().getString(
                                        "WrapperListener.stop runner thread stopped." ) );
                                }
                            }
                        }
                    };
                    stopRunner.setDaemon( false );
                    stopRunner.start();
                    
                    // Wait for the stop runner to complete.
                    if ( m_debug )
                    {
                       m_outDebug.println( getRes().getString(
                            "Waiting for WrapperListener.stop runner thread to complete." ) );
                    }
                    while ( ( stopRunner != null ) && ( stopRunner.isAlive() ) )
                    {
                        try
                        {
                            stopRunner.join();
                            stopRunner = null;
                        }
                        catch ( InterruptedException e )
                        {
                            // Ignore and keep waiting.
                        }
                    }
                    
                    // Get the exit code back from the array.
                    code = codeF[0].intValue();
                }
                else
                {
                    // This is user code, so don't trust it.
                    try
                    {
                        code = m_listener.stop( code );
                    }
                    catch ( Throwable t )
                    {
                        m_outError.println( getRes().getString( "Error in WrapperListener.stop callback." ) );
                        t.printStackTrace( m_outError );
                    }
                }
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "returned from listener.stop() -> {0}",
                             new Integer( code ) ) );
                }
            }
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
     *    WRAPPER_CTRL_LOGOFF_EVENT, WRAPPER_CTRL_SHUTDOWN_EVENT,
     *    WRAPPER_CTRL_TERM_EVENT, or WRAPPER_CTRL_HUP_EVENT.
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
        case WRAPPER_CTRL_TERM_EVENT:
            eventName = "WRAPPER_CTRL_TERM_EVENT";
            ignore = m_ignoreSignals;
            break;
        case WRAPPER_CTRL_HUP_EVENT:
            eventName = "WRAPPER_CTRL_HUP_EVENT";
            ignore = m_ignoreSignals;
            break;
        case WRAPPER_CTRL_USR1_EVENT:
            eventName = "WRAPPER_CTRL_USR1_EVENT";
            ignore = m_ignoreSignals;
            break;
        case WRAPPER_CTRL_USR2_EVENT:
            eventName = "WRAPPER_CTRL_USR2_EVENT";
            ignore = m_ignoreSignals;
            break;
        default:
            eventName =  getRes().getString( "Unexpected event: {0}", new Integer( event ) );
            ignore = false;
            break;
        }
        
        WrapperControlEvent controlEvent = new WrapperControlEvent( event, eventName );
        if ( ignore )
        {
            // Preconsume the event if it is set to be ignored, but go ahead and fire it so
            //  user can can still have the oportunity to recognize it.
            controlEvent.consume();
        }
        fireWrapperEvent( controlEvent );
        
        if ( ignore )
        {
            // Event will always be consumed.
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "Ignoring control event({0})", eventName ) );
            }
        }
        else
        {
            if ( controlEvent.isConsumed() )
            {
                if ( m_debug )
                {
                    m_outDebug.println(getRes().getString( "Control event({0}) was consumed by user listener.", eventName ) );
                }
            }
            else
            {
                if ( m_debug )
                {
                    m_outDebug.println(getRes().getString( "Processing control event({0})", eventName ) );
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
                        m_outError.println( getRes().getString( "Error in WrapperListener.controlEvent callback." ) );
                        t.printStackTrace( m_outError );
                    }
                }
                else
                {
                    // A listener was never registered.  Always respond by exiting.
                    //  This can happen if the user does not initialize things correctly.
                    stop( 0 );
                }
            }
        }
    }
    
    /**
     * Parses a long tab separated string of properties into an internal
     *  properties object.  Actual tabs are escaped by real tabs.
     */
    private static char PROPERTY_SEPARATOR = '\t';
    private static void readProperties( String rawProps )
    {
        WrapperProperties properties = new WrapperProperties();
        
        int len = rawProps.length();
        int first = 0;
        while ( first < len )
        {
            StringBuffer sb = new StringBuffer();
            boolean foundEnd = false;
            do
            {
                int pos = rawProps.indexOf( PROPERTY_SEPARATOR, first );
                if ( pos >= 0 )
                {
                    if ( pos > 0 )
                    {
                        sb.append( rawProps.substring( first, pos ) );
                    }
                    if ( pos < len - 1 )
                    {
                        if ( rawProps.charAt( pos + 1 ) == PROPERTY_SEPARATOR )
                        {
                            // Two separators in a row, it was escaped.
                            sb.append( PROPERTY_SEPARATOR );
                            first = pos + 2;
                        }
                        else
                        {
                            foundEnd = true;
                            first = pos + 1;
                        }
                    }
                    else
                    {
                        foundEnd = true;
                        first = pos + 1;
                    }
                }
                else
                {
                    // No more separators.  The rest is the last property.
                    sb.append( rawProps.substring( first ) );
                    foundEnd = true;
                    first = len;
                }
            }
            while ( !foundEnd );
            
            String property = sb.toString();
            
            // Parse the property.
            int pos = property.indexOf( '=' );
            if ( pos > 0 )
            {
                String key = property.substring( 0, pos );
                String value;
                if ( pos < property.length() - 1 )
                {
                    value = property.substring( pos + 1 );
                }
                else
                {
                    value = "";
                }
                
                properties.setProperty( key, value );
                
                // Process special properties
                if ( key.equals( "wrapper.ignore_user_logoffs" ) )
                {
                    m_ignoreUserLogoffs = value.equalsIgnoreCase( "true" );
                }
            }
        }
        
        // Lock the properties object and store it.
        properties.lock();
        
        m_properties = properties;
    }
    
    /**
     * Opens a socket to the Wrapper process.
     */
    private static synchronized void openBackendSocket()
    {
        if ( m_debug )
        {
            m_outDebug.println( getRes().getString( "Open socket to Wrapper...{0}",
                    Thread.currentThread().getName() ) );
        }

        InetAddress iNetAddress;
        try
        {
            iNetAddress = InetAddress.getByName( m_wrapperPortAddress );
        }
        catch ( UnknownHostException e )
        {
            // This is pretty fatal.
            m_outError.println( getRes().getString( "Unable to resolve localhost name: {0}", e ) );
            m_outError.println( getRes().getString( "Exiting JVM..." ) );
            stop( 1 );
            return; // please the compiler
        }
        
        // If the user has specified a specific port to use then we want to try that first.
        boolean connected = false;
        int tryPort;
        boolean fixedPort;
        if ( m_jvmPort >= 0 )
        {
            tryPort = m_jvmPort;
            fixedPort = true;
        }
        else
        {
            tryPort = m_jvmPortMin;
            fixedPort = false;
        }
        
        // Loop until we find a port we can connect using.
        SocketException causeException = null;
        do
        {
            // On Windows if we attempt to open a socket with a port that was recently used and still in a TIME_WAIT
            //  state, then Windows will sometimes cause a 4227 Event entry to be logged in the System EventLog.
            //  It is not possible in Java to check on the state of a port before using it so we do so in native
            //  code.  This check is not critical other than this warning, so fall through to the normal attempt if
            //  we are unable to precheck the port status for any reason.
            int portStatus;
            if ( isNativeLibraryOk() )
            {
                try
                {
                    portStatus = nativeGetPortStatus( tryPort, m_wrapperPortAddress, ( m_backendType == BACKEND_TYPE_SOCKET_V6 ? 1 : 0 ) );
                }
                catch ( UnsatisfiedLinkError e )
                {
                    // This can happen if an old native library is used.  Fall through.
                    m_outError.println( getRes().getString( "Unable to precheck status of port {0} due to: {1}",
                                new Integer( tryPort ), e.toString() ) );
                    portStatus = -1;
                }
            }
            else
            {
                portStatus = -1;
            }
            if ( portStatus <= 0 )
            {
                try
                {
                    m_backendSocket = new Socket( iNetAddress, m_port, iNetAddress, tryPort );
                    if ( m_debug )
                    {
                        m_outDebug.println(getRes().getString( "Opened Socket from {0} to {1}",
                                new Integer( tryPort ), new Integer( m_port ) ) );
                    }
                    connected = true;
                    break;
                }
                catch ( SocketException e )
                {
                    String eMessage = e.getMessage();
                    
                    if ( e instanceof ConnectException )
                    {
                        m_outError.println(getRes().getString(
                                "Failed to connect to the Wrapper at port {0}. Cause: {1}",
                                new Integer( m_port ), e ) );
                        // This is fatal because there is nobody listening.
                        m_outError.println( "Exiting JVM..." );
                        stopImmediate( 1 );
                        // For compiler
                        m_backendSocket = null;
                        return;
                    }
                    else if ( ( e instanceof BindException ) ||
                        ( ( eMessage != null ) &&
                        ( ( eMessage.indexOf( "errno: 48" ) >= 0 ) ||
                            ( eMessage.indexOf( "Address already in use" ) >= 0 ) ) ||
                            ( eMessage.indexOf( "Unrecognized Windows Sockets error: 0: JVM_Bind" ) >= 0 ) ) ) /* This message is caused by a JVM Bug: http://bugs.sun.com/view_bug.do?bug_id=6965962 */
                    {
                        // Most Java implementations throw a BindException when the port is in use,
                        //  but FreeBSD throws a SocketException with a specific message.
                        
                        // This happens if the local port is already in use.  In this case, we want
                        //  to loop and try again.
                        if ( m_debug )
                        {
                            m_outDebug.println( getRes().getString(
                                    "Unable to open socket to Wrapper from port {0}, already in use.",
                                    new Integer( tryPort ) ) );
                        }
                        
                        // Keep this exception around in case we need to log it.
                        if ( causeException == null )
                        {
                            causeException = e;
                        }
                    }
                    else
                    {
                        // Unexpected exception.
                        m_outError.println( getRes().getString( "Unexpected exception opening backend socket: {0}", e ) );
                        m_backendSocket = null;
                        return;
                    }
                }
                catch ( IOException e )
                {
                    m_outError.println( getRes().getString( "Unable to open backend socket: {0}", e ) );
                    m_backendSocket = null;
                    return;
                }
            }
            else
            {
                // The native port status returned was not 0.
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString(
                            "Unable to open socket to Wrapper from port {0}, already in use. ({1})",
                            new Integer( tryPort ), Integer.toString( portStatus ) ) );
                }
            }
            
            if ( fixedPort )
            {
                // The last port checked was the fixed port, switch to the dynamic range.
                tryPort = m_jvmPortMin;
                fixedPort = false;
            }
            else
            {
                tryPort++;
            }
        }
        while ( tryPort <= m_jvmPortMax );
        
        if ( connected )
        {
            if ( ( m_jvmPort >= 0 ) && ( m_jvmPort != tryPort ) )
            {
                m_outInfo.println(getRes().getString(
                    "Port {0} already in use, using port {1} instead.",
                    new Integer( m_jvmPort ), new Integer( tryPort ) ) );
            }
        }
        else
        {
            if ( m_jvmPortMax > m_jvmPortMin )
            {
                m_outError.println( getRes().getString(
                        "Failed to connect to the Wrapper at port {0} by binding to any ports in the range {1} to {2}.  Cause: {3}",
                        new Integer( m_port ), new Integer( m_jvmPortMin ), new Integer( m_jvmPortMax ), causeException ) );
            }
            else
            {
                m_outError.println(getRes().getString( 
                        "Failed to connect to the Wrapper at port {0} by binding to port {1}.  Cause: {2}",
                        new Integer( m_port ), new Integer( m_jvmPortMin ), causeException ) );
            }
            // This is fatal because there is nobody listening.
            m_outError.println( getRes().getString( "Exiting JVM..." ) );
            stopImmediate( 1 );
        }
        
        // Now that we have a connected socket, continue on to configure it.
        try
        {
            // Turn on the TCP_NODELAY flag.  This is very important for speed!!
            m_backendSocket.setTcpNoDelay( true );
            
            // If configured, set the SO_TIMEOUT for the socket (max block time)
            if ( m_soTimeout >= 0 )
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "Setting backend socket SO_TIMEOUT to {0}ms from {1}ms.",
                        new Integer( m_soTimeout ), new Integer( m_backendSocket.getSoTimeout() ) ) );
                }
                
                m_backendSocket.setSoTimeout( m_soTimeout );
            }
            
            m_backendOS = m_backendSocket.getOutputStream();
            m_backendIS = m_backendSocket.getInputStream();
        }
        catch ( IOException e )
        {
            m_outError.println( e );
            
            closeBackend();
            return;
        }
        
        m_backendConnected = true;
    }
    
    private static synchronized void openBackendPipe()
    {
        String s;

        if ( WrapperManager.isWindows() )
        {
            s = "\\\\.\\pipe\\wrapper-" + WrapperManager.getWrapperPID() + "-" + WrapperManager.getJVMId();
        } else {
            s = "/tmp/wrapper-" + WrapperManager.getWrapperPID() + "-" + WrapperManager.getJVMId();
        }
        try
        {
            m_backendIS = new FileInputStream( new File( s + "-out") );
            m_backendOS = new FileOutputStream( new File( s + "-in" ) );
        } catch ( IOException e ) {
            m_outInfo.println( "write error " + e );
            e.printStackTrace();
            
            closeBackend();
            return;
        } catch (Exception ex) {
            ex.printStackTrace();
            
            closeBackend();
            return;
        }
        m_backendConnected = true;
    }
    
    private static synchronized void openBackend()
    {
        m_backendConnected = false;
        
        if ( m_backendType == BACKEND_TYPE_PIPE )
        {
            openBackendPipe();
        }
        else
        {
            openBackendSocket();
        }
        
        if ( !m_backendConnected )
        {
            m_outError.println( getRes().getString( "The backend could not be initialized.  Restart to resync with the Wrapper." ) );
            restart();
            // Will not get here.
            return;
        }
        
        // The backend is open.
        
        // Send the key back to the wrapper so that the wrapper can feel safe
        //  that it is talking to the correct JVM
        sendCommand( WRAPPER_MSG_KEY, m_key );
        
        // If there is a stop pending then send it immediately now.
        if ( m_pendingStopMessage != null )
        {
            m_outDebug.println( getRes().getString( "Resend pending packet {0} : {1}", getPacketCodeName( WRAPPER_MSG_STOP ), m_pendingStopMessage ) );
            sendCommand( WRAPPER_MSG_STOP, m_pendingStopMessage );
            m_pendingStopMessage = null;
        }
    }
    
    private static synchronized void closeBackend()
    {
        if ( m_backendConnected )
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "Closing backend connection." ) );
            }
            
            // To avoid the case where a child process is launched by we fail to notify the Wrapper,
            //  we need to wait until there are no longer any child processes in the process of being launched.
            long start = System.currentTimeMillis();
            while ( m_runningExecs > 0 )
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "Waiting for {0} threads to finish launching child processes...", new Integer( m_runningExecs ) ) );
                }
                try
                {
                    WrapperManager.class.wait( 1000 );
                }
                catch ( InterruptedException e )
                {
                }
                if ( System.currentTimeMillis() - start > 30000 )
                {
                    m_outError.println( getRes().getString( "Timed out waiting for {0} threads to finish launching child processes.", new Integer( m_runningExecs ) ) );
                    break;
                }
            }
            
            // Clear the connected flag first so other threads will recognize that we
            //  are closing correctly.
            m_backendConnected = false;
            
            // On HP platforms, an attempt to close a socket while a read is blocking will
            //  cause the close to block until the read completes.  To avoid this, send an
            //  interrupt to the communications runner to abort any existing reads.
            Thread commRunner = m_commRunner;
            if ( commRunner != null )
            {
                try
                {
                    commRunner.interrupt();
                }
                catch ( SecurityException e )
                {
                    m_outError.println( getRes().getString( "Failed to interrupt communications thread: {0}", e.getMessage() ) );
                }
            }
        }
        
        if ( m_backendOS != null )
        {
            try
            {
                m_backendOS.close();
            }
            catch ( IOException e )
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "Unable to close backend output stream: {0}", e.toString() ) );
                }
            }
            m_backendOS = null;
        }
        
        if ( m_backendIS != null )
        {
            // Closing m_backendIS here will block on some platforms.  Let the Wrapper end it cleanup.
            m_backendIS = null;
        }
        
        if ( m_backendSocket != null )
        {
            try
            {
                m_backendSocket.close();
            }
            catch ( IOException e )
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "Unable to close backend socket: {0}", e.toString() ) );
                }
            }
            m_backendSocket = null;
        }
    }
    
    private static String getPacketCodeName( byte code )
    {
        String name;
    
        switch ( code )
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
    
        case WRAPPER_MSG_PING_TIMEOUT: /* No longer used. */
            name ="PING_TIMEOUT";
            break;
    
        case WRAPPER_MSG_SERVICE_CONTROL_CODE:
            name ="SERVICE_CONTROL_CODE";
            break;
    
        case WRAPPER_MSG_PROPERTIES:
            name ="PROPERTIES";
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
    
        case WRAPPER_MSG_LOG + WRAPPER_LOG_LEVEL_ADVICE:
            name ="LOG(ADVICE)";
            break;
    
        case WRAPPER_MSG_LOG + WRAPPER_LOG_LEVEL_NOTICE:
            name ="LOG(NOTICE)";
            break;
           
        case WRAPPER_MSG_CHILD_LAUNCH:
            name ="CHILD_LAUNCH";
            break;
    
        case WRAPPER_MSG_CHILD_TERM:
            name ="CHILD_TERM";
            break;

        case WRAPPER_MSG_LOGFILE:
            name ="LOGFILE";
            break;
           
        case WRAPPER_MSG_CHECK_DEADLOCK:
            name ="CHECK_DEADLOCK";
            break;
    
        case WRAPPER_MSG_DEADLOCK:
            name ="DEADLOCK";
            break;
    
        case WRAPPER_MSG_APPEAR_ORPHAN: /* No longer used. */
            name ="APPEAR_ORPHAN";
            break;
    
        case WRAPPER_MSG_PAUSE:
            name ="PAUSE";
            break;
    
        case WRAPPER_MSG_RESUME:
            name ="RESUME";
            break;
    
        case WRAPPER_MSG_GC:
            name ="GC";
            break;
    
        case WRAPPER_MSG_FIRE_USER_EVENT:
            name ="FIRE_USER_EVENT";
            break;
    
        case WRAPPER_MSG_SECOND_INVOCATION_EVENT:
            name ="SECOND_INVOCATION_EVENT";
            break;
    
        default:
            name = "UNKNOWN(" + code + ")";
            break;
        }
        return name;
    }
    
    /**
     * Send a command to the Wrapper.
     *
     * @param code Command to send.
     * @param message Message to send with the command.
     */
    private static synchronized void sendCommand( byte code, String message )
    {
        if ( m_debug )
        {
            // We want to show these messages even if we are pretending to be hung.
            if ( ( code == WRAPPER_MSG_PING ) && ( message.startsWith( "silent" ) ) )
            {
                // m_outDebug.println( "Send silent ping packet." );
            }
            else if ( !m_backendConnected )
            {
                m_outDebug.println( getRes().getString(
                        "Backend not connected, not sending packet {0} : {1}",
                        getPacketCodeName( code ), message ) );

                if ( code == WRAPPER_MSG_STOP )
                {
                    // Store this message so we can send it later if and when we connect.
                    m_pendingStopMessage = message;
                }
            }
            else
            {
                m_outDebug.println( getRes().getString( "Send a packet {0} : {1}",
                    getPacketCodeName( code ) , message ) );
            }
        }
        
        boolean sentCommand = false;
        if ( m_appearHung )
        {
            // The WrapperManager is attempting to make the JVM appear hung, so do nothing
        }
        else
        {
            if ( ( code == WRAPPER_MSG_START_PENDING ) || ( code == WRAPPER_MSG_STARTED ) )
            {
                // Set the last ping time so that the startup process does not time out
                //  thinking that the JVM has not received a Ping for too long.
                m_lastPingTicks = getTicks();
            }
            
            // If the backend is open, then send the command, otherwise just throw it away.
            if ( m_backendConnected )
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

                    m_backendOS.write( m_commandBuffer, 0, len );
                    m_backendOS.flush();
                    
                    sentCommand = true;
                }
                catch ( IOException e )
                {
                    m_outError.println( e );
                    e.printStackTrace( m_outError );
                    closeBackend();
                }
            }
        }
        
        if ( !sentCommand )
        {
            // We failed to send a command.  Some commands require that we log it as it could cause later problems.
            switch ( code )
            {
            case WRAPPER_MSG_CHILD_LAUNCH:
                m_outError.println( getRes().getString( "Failed to notify the Wrapper process that child with PID={0} was launched.  The Wrapper will not be able to make sure it is terminated when the Java process exits.", message ) );
                break;
                
            case WRAPPER_MSG_CHILD_TERM:
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "Failed to notify the Wrapper process that child with PID={0} completed.  The Wrapper will recover on its own.", message ) );
                }
                break;
                
            default:
                break;
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
    private static byte[] m_backendReadBuffer = new byte[256];
    private static void handleBackend()
    {
        WrapperPingEvent pingEvent = new WrapperPingEvent();
        
        try
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "handleBackend()" ) );
            }

            DataInputStream is = new DataInputStream( m_backendIS );
            while ( !m_disposed )
            {
                try
                {
                    // A Packet code must exist.
                    byte code = is.readByte();
                    
                    // Always read from the buffer until a null '\0' is encountered.
                    //  A multi-byte string will never have a 0 as part of another character so this should be safe for all encodings.
                    byte b;
                    int i = 0;
                    do
                    {
                        b = is.readByte();
                        if ( b != 0 )
                        {
                            if ( i >= m_backendReadBuffer.length )
                            {
                                byte[] tmp = m_backendReadBuffer;
                                m_backendReadBuffer = new byte[tmp.length + 256];
                                System.arraycopy( tmp, 0, m_backendReadBuffer, 0, tmp.length );
                            }
                            m_backendReadBuffer[i] = b;
                            i++;
                        }
                    }
                    while ( b != 0 );
                    
                    // The message should be a multi-byte UTF-8 string (except on z/OS where the system encoding is used).
                    String msg;
                    if ( !isZOS() )
                    {
                        msg = new String( m_backendReadBuffer, 0, i, "UTF-8" );
                    }
                    else
                    {
                        msg = new String( m_backendReadBuffer, 0, i );
                    }
                    
                    if ( m_appearHung )
                    {
                        // The WrapperManager is attempting to make the JVM appear hung,
                        //   so ignore all incoming requests
                    }
                    else
                    {
                        if ( m_debug )
                        {
                            String logMsg;
                            if ( code == WRAPPER_MSG_PROPERTIES )
                            {
                                // The property values are very large and distracting in the log.
                                //  Plus if any triggers are defined, then logging them will fire
                                //  the trigger.
                                logMsg = getRes().getString( "(Property Values, Size={0})", Integer.toString( i ) );
                            }
                            else
                            {
                                logMsg = msg;
                            }
                            
                            // Don't log silent pings.
                            if ( ( code == WRAPPER_MSG_PING ) && ( msg.startsWith( "silent" ) ) )
                            {
                                //m_outDebug.println( "Received silent ping packet." );
                            }
                            else
                            {
                                m_outDebug.println( getRes().getString( "Received a packet {0} : {1}",
                                        getPacketCodeName( code ) , logMsg ) );
                            }
                        }
                        
                        if ( m_slowSeconds > 0 )
                        {
                            // We have been asked to be sluggish in the JVM's response.
                            if ( m_debug )
                            {
                                m_outDebug.println( getRes().getString( "  Delay packet processing by {0} seconds.", new Integer( m_slowSeconds ) ) );
                            }
                            try
                            {
                                Thread.sleep( m_slowSeconds * 1000 );
                            }
                            catch ( InterruptedException e )
                            {
                            }
                        }
                        
                        // Ok, we got a packet.  Do something with it.
                        switch( code )
                        {
                        case WRAPPER_MSG_START:
                            // Don't start if we are already starting to stop.
                            if ( m_stoppingInit) {
                                if ( m_debug )
                                {
                                    m_outDebug.println( getRes().getString( "Java stop initiated.  Skipping application startup." ) );
                                }
                            } else {
                                startInner( false );
                            }
                            break;
                            
                        case WRAPPER_MSG_STOP:
                            // Don't do anything if we are already stopping
                            if ( !m_stopping )
                            {
                                privilegedStopInner( 0 );
                                // Should never get back here.
                            }
                            break;
                            
                        case WRAPPER_MSG_PING:
                            m_lastPingTicks = getTicks();
                            
                            sendCommand( WRAPPER_MSG_PING, msg );
                            
                            if ( m_produceCoreEvents )
                            {
                                fireWrapperEvent( pingEvent );
                            }
                            
                            break;
                            
                        case WRAPPER_MSG_CHECK_DEADLOCK:
                            boolean deadLocked = checkDeadlocks();
                            if ( deadLocked )
                            {
                                sendCommand( WRAPPER_MSG_DEADLOCK, "deadLock" );
                            }
                            break;
                            
                        case WRAPPER_MSG_BADKEY:
                            // The key sent to the wrapper was incorrect.  We need to shutdown.
                            m_outError.println( getRes().getString("Authorization key rejected by Wrapper." ) );
                            m_outError.println( getRes().getString( "Exiting JVM..." ) );
                            closeBackend();
                            privilegedStopInner( 1 );
                            break;
                            
                        case WRAPPER_MSG_LOW_LOG_LEVEL:
                            try
                            {
                                m_lowLogLevel = Integer.parseInt( msg );
                                m_debug = ( m_lowLogLevel <= WRAPPER_LOG_LEVEL_DEBUG );
                                if ( m_debug )
                                {
                                    m_outDebug.println( getRes().getString( "LowLogLevel from Wrapper is {0}",
                                        new Integer( m_lowLogLevel ) ) );
                                }
                            }
                            catch ( NumberFormatException e )
                            {
                                m_outError.println( getRes().getString(
                                        "Encountered an Illegal LowLogLevel from the Wrapper: {0}", msg ) );
                            }
                            break;
                            
                        case WRAPPER_MSG_PING_TIMEOUT:
                            /* No longer used.  This is still here in case a mix of versions are used. */
                            break;
                            
                        case WRAPPER_MSG_SERVICE_CONTROL_CODE:
                            try
                            {
                                int serviceControlCode = Integer.parseInt( msg );
                                if ( m_debug )
                                {
                                    m_outDebug.println( getRes().getString(
                                            "ServiceControlCode from Wrapper with code {0}",
                                            new Integer( serviceControlCode ) ) );
                                }
                                WrapperServiceControlEvent event =
                                    new WrapperServiceControlEvent( serviceControlCode );
                                fireWrapperEvent( event );
                            }
                            catch ( NumberFormatException e )
                            {
                                m_outError.println( getRes().getString(
                                        "Encountered an Illegal ServiceControlCode from the Wrapper: {0}", msg ) );
                            }
                            break;
                            
                        case WRAPPER_MSG_PAUSE:
                            try
                            {
                                int actionSourceCode = Integer.parseInt( msg );
                                if ( m_debug )
                                {
                                    m_outDebug.println( getRes().getString( "Pause from Wrapper with action source: {0}", WrapperServicePauseEvent.getSourceCodeName( actionSourceCode ) ) );
                                }
                                WrapperServicePauseEvent event =
                                    new WrapperServicePauseEvent( actionSourceCode );
                                fireWrapperEvent( event );
                            }
                            catch ( NumberFormatException e )
                            {
                                m_outError.println( getRes().getString(
                                        "Encountered an Illegal action source code from the Wrapper: {0}", msg ) );
                            }
                            break;
                            
                        case WRAPPER_MSG_RESUME:
                            try
                            {
                                int actionSourceCode = Integer.parseInt( msg );
                                if ( m_debug )
                                {
                                    m_outDebug.println( getRes().getString("Resume from Wrapper with action source: {0}", WrapperServiceResumeEvent.getSourceCodeName( actionSourceCode ) ) );
                                }
                                WrapperServiceResumeEvent event =
                                    new WrapperServiceResumeEvent( actionSourceCode );
                                fireWrapperEvent( event );
                            }
                            catch ( NumberFormatException e )
                            {
                                m_outError.println( getRes().getString(
                                        "Encountered an Illegal action source code from the Wrapper: {0}", msg ) );
                            }
                            break;
                            
                        case WRAPPER_MSG_GC:
                            try
                            {
                                int actionSourceCode = Integer.parseInt( msg );
                                if ( m_debug )
                                {
                                    m_outDebug.println( getRes().getString( "Garbage Collection request from Wrapper with action source: {0}", WrapperServiceActionEvent.getSourceCodeName( actionSourceCode ) ) );
                                }
                                System.gc();
                            }
                            catch ( NumberFormatException e )
                            {
                                m_outError.println( getRes().getString(
                                        "Encountered an Illegal action source code from the Wrapper: {0}", msg ) );
                            }
                            break;
                            
                        case WRAPPER_MSG_PROPERTIES:
                            readProperties( msg );
                            break;
                            
                        case WRAPPER_MSG_LOGFILE:
                            m_logFile = new File( msg );
                            WrapperLogFileChangedEvent event = new WrapperLogFileChangedEvent( m_logFile );
                            fireWrapperEvent( event );
                            break;
                        case WRAPPER_MSG_SECOND_INVOCATION_EVENT:
                            WrapperSecondInvocationEvent secondInvocationEvent = new WrapperSecondInvocationEvent( );
                            fireWrapperEvent( secondInvocationEvent );
                            sendCommand(WRAPPER_MSG_SECOND_INVOCATION_EVENT, "");
                            break;
                            
                        default:
                            // Ignore unknown messages
                            m_outInfo.println( getRes().getString(
                                    "Wrapper code received an unknown packet type: {0}", new Integer( code ) ) );
                            break;
                        }
                    }
                }
                catch ( SocketTimeoutException e )
                {
                    if ( m_debug )
                    {
                        m_outDebug.println( getRes().getString( "Backend socket timed out.  Attempting to continue.  (SO_TIMEOUT={0}ms.)",
                            new Integer( m_backendSocket.getSoTimeout() ) ) );
                    }
                }
            }
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "Backend handler loop completed.  Disposed: {0}", ( m_disposed ? "True" : "False" ) ) );
            }
            return;

        }
        catch ( SocketException e )
        {
            if ( m_debug )
            {
                if ( m_backendSocket == null )
                {
                    // This error happens if the socket is closed while reading:
                    // java.net.SocketException: Descriptor not a socket: JVM_recv in socket
                    //                           input stream read
                    m_outDebug.println( getRes().getString( "Closed backend socket (Normal): {0}", e ) );
                }
                else
                {
                    m_outDebug.println( getRes().getString( "Closed backend socket: {0}", e ) );
                }
            }
            return;
        }
        catch ( IOException e )
        {
            // This means that the connection was closed.  Allow this to return.
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "Closed backend (Normal): {0}", e ) );
            }
            return;
        }
    }
    
    /**
     * Starts up the thread to handle backend communications.
     *
     * If the Wrapper did not launch the JVM then the internal state
     *  will be set to started to allow the application startup to continue.
     */
    private static void startRunner()
    {
        if ( isControlledByNativeWrapper() )
        {
            if ( m_commRunner == null )
            {
                // Create and launch a new thread to manage this connection
                m_commRunner = new Thread( m_instance, WRAPPER_CONNECTION_THREAD_NAME );
                m_commRunner.setDaemon( true );
                m_commRunner.start();
            }
            
            // Wait to give the runner a chance to connect.
            synchronized( WrapperManager.class )
            {
                while ( !m_commRunnerStarted )
                {
                    try
                    {
                        WrapperManager.class.wait( 100 );
                    }
                    catch ( InterruptedException e )
                    {
                    }
                }
            }
        }
        else
        {
            // Immediately mark the runner as started as it will never be used.
            synchronized( WrapperManager.class )
            {
                m_commRunnerStarted = true;
                WrapperManager.class.notifyAll();
            }
        }
    }
    
    /*---------------------------------------------------------------
     * Runnable Methods
     *-------------------------------------------------------------*/
    /**
     * The main comm runner thread.
     *  This will only be called when the Wrapper is controlling the JVM.
     */
    public void run()
    {
        // Make sure that no other threads call this method.
        if ( Thread.currentThread() != m_commRunner )
        {
            throw new IllegalStateException( getRes().getString(
                "Only the communications runner thread is allowed to call this method." ) );
        }
        
        if ( m_debug )
        {
            m_outDebug.println( getRes().getString( "Communications runner thread started." ) );
        }
        
        // This thread needs to have a very high priority so that it never
        //  gets put behind other threads.
        Thread.currentThread().setPriority( Thread.MAX_PRIORITY );
        
        // Initialize the last ping tick count.
        m_lastPingTicks = getTicks();
        
        try
        {
            openBackend();
            
            // After the socket has been opened the first time, mark the thread as
            //  started.  This must be done here to make sure that exits work correctly
            //  when called on startup.
            if ( !m_commRunnerStarted )
            {
                synchronized( WrapperManager.class )
                {
                    m_commRunnerStarted = true;
                    WrapperManager.class.notifyAll();
                }
            }
            
            // Make sure that the version of the Wrapper is correct.
            verifyWrapperVersion();
            
            // Make sure that the native library's version is correct.
            if (m_libraryLoaded) {
                verifyNativeLibraryVersion();
            }
            
            if ( !m_wrapperVersionOk || !m_libraryVersionOk || !m_libraryLoaded) {
                // We need to wait until the backend is opened before stopping the JVM,
                //  else the Wrapper can't be informed that the JVM requested to stop.
                stop( 1 );
            }
            
            if ( m_backendSocket != null || m_backendConnected == true)
            {
                handleBackend();
            }
            else
            {
                // Failed, so wait for just a moment
                try
                {
                    Thread.sleep( 100 );
                }
                catch ( InterruptedException e )
                {
                }
            }
        }
        catch ( ThreadDeath td )
        {
            m_outError.println( getRes().getString( "Server daemon killed" ) );
        }
        catch ( Throwable t )
        {
            if ( !isShuttingDown() )
            {
                // Show a stack trace here because this is fairly critical
                m_outError.println( getRes().getString( "Server daemon died!" ) );
                t.printStackTrace( m_outError );
            }
            else
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "Server daemon died!" ) );
                    t.printStackTrace( m_outDebug );
                }
            }
        }
        finally
        {
            if ( m_debug )
            {
                m_outDebug.println( getRes().getString( "Returned from backend handler." ) ); 
            }
            // Always close the backend here.
            closeBackend();
            if ( !isShuttingDown() )
            {
                if ( m_detachStarted && m_started )
                {
                    // This and all further output will not be visible anywhere as the Wrapper is now gone.
                    m_outInfo.println( getRes().getString( "The backend was closed as expected." ) );
                    
                    if ( isNativeLibraryOk() )
                    {
                        nativeRedirectPipes();
                    }
                    else
                    {
                        m_outError.println( getRes().getString( "Failed to redirect stdout and stderr before the Wrapper exits.\nOutput from the JVM may block.\nPlease make sure the native library has been properly initialized."));
                    }
                }
                else
                {
                    m_outError.println( getRes().getString( "The backend was closed unexpectedly.  Restart to resync with the Wrapper." ) );
                    restart();
                    // Will not get here.
                }
            }
        }
        
        // Make sure that noone is ever left waiting for this thread to start.
        synchronized( WrapperManager.class )
        {
            if ( !m_commRunnerStarted )
            {
                m_commRunnerStarted = true;
                WrapperManager.class.notifyAll();
            }
        }
        
        if ( m_debug )
        {
            m_outDebug.println( getRes().getString( "Server daemon shut down" ) );
        }
    }
    
    /*---------------------------------------------------------------
     * Inner Classes
     *-------------------------------------------------------------*/
    /**
     * Mapping between WrapperEventListeners and their registered masks.
     *  This is necessary to support the case where the same listener is
     *  registered more than once.   It also makes it possible to reference
     *  an array of these mappings without synchronization.
     */
    private static class WrapperEventListenerMask
    {
        private WrapperEventListener m_listener;
        private long m_mask;
    }
    
    private static class WrapperTickEventImpl
        extends WrapperTickEvent
    {
        private int m_ticks;
        private int m_tickOffset;
        
        /**
         * Returns the tick count at the point the event is fired.
         *
         * @return The tick count at the point the event is fired.
         */
        public int getTicks()
        {
            return m_ticks;
        }
        
        /**
         * Returns the offset between the tick count used by the Wrapper for time
         *  keeping and the tick count generated directly from the system time.
         *
         * This will be 0 in most cases.  But will be a positive value if the
         *  system time is ever set back for any reason.  It will be a negative
         *  value if the system time is set forward or if the system is under
         *  heavy load.  If the wrapper.use_system_time property is set to TRUE
         *  then the Wrapper will be using the system tick count for internal
         *  timing and this value will always be 0.
         *
         * @return The tick count offset.
         */
        public int getTickOffset()
        {
            return m_tickOffset;
        }
    }
    
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
            m_outInfo.println( getRes().getString( "WARNING - System.in has been disabled by the wrapper.disable_console_input property.  Calls will block indefinitely." ) );
            
            // Go into a loop that will never return.
            while ( true )
            {
                synchronized( this )
                {
                    try
                    {
                        this.wait();
                    }
                    catch ( InterruptedException e )
                    {
                        // Ignore.
                    }
                }
            }
        }
    }
    
    /**
     * Scans the JVM for deadlocked threads.
     * <p>
     * Standard Edition feature.
     *
     * @see #isStandardEdition()
     * @since Wrapper 3.5.0
     */
    private static boolean checkDeadlocks()
        throws WrapperLicenseError
    {
        if ( isStandardEdition() )
        {
            boolean result = false;
            if ( isNativeLibraryOk() )
            {
                result = nativeCheckDeadLocks();
            }
            else
            {
                if ( m_debug )
                {
                    m_outDebug.println( getRes().getString( "Deadlock check skipped.  Native call unavailable." ) ); 
                }
                result = false;
            }
            return result;
        }
        else
        {
            return false;
        }
    }
}


