Java Service Wrapper Documentation.

The Java Service Wrapper was originally implemented to solve two problems.

1) We needed to be able to install our application as an NT service.  There
were other solutions to this, but none of them were flexible enough to meet
our needs and all required recompilation for each application.  We also wanted
a solution which could be used to run our application on Win32, Linux and
Solaris platforms.

2) The stability of Java applications with early versions HotSpot left much to
be desired.  The JVM would occasionally crash or freeze up.  We needed a way to
detect when this had happened and get our application up and running as quickly
as possible.



Overview:



API Overview:
The Java-side API for Wrapper is quite simple and can be run in one of two modes.

1) WrapperSimpleApp class
The first is to use the WrapperSimpleApp class to launch your Java application
with out any modifications.  This class handles all communication with the native
wrapper and simply launches your application by calling its main method as would
be done from the command line.

The drawback to this method is that when your service is stopped, it is killed as
if the user had hit CTRL-C in the console.  Your application will not have a
chance to call any shutdown code.

2) Implement WrapperListener
The second and recommended method is to have your application implement the
WrapperListener interface.  This interface provides three methods used to inform
your application when it should start and stop as well as when a system control
event takes place (for example, the user hitting CTRL-C, or logging out under
windows)

The WrapperListener class has the following methods.
----
public interface WrapperListener {
    /**
     * The start method is called when the WrapperManager is signaled by the 
     *	native wrapper code that it can start its application.  This
     *	method call is expected to return, so a new thread should be launched
     *	it necessary.
     * If there are any problems, then an Integer should be returned, set to
     *	the desired exit code.  If the application should continue,
     *	return null.
     */
    Integer start(String[] args);
    
    /**
     * Called when the application is shutting down.
     */
    int stop(int exitCode);
    
    /**
     * Called whenever the native wrapper code traps a system control signal
     *  against the Java process.  It is up to the callback to take any actions
     *  necessary.  Possible values are: WrapperManager.WRAPPER_CTRL_C_EVENT, 
     *    WRAPPER_CTRL_CLOSE_EVENT, WRAPPER_CTRL_LOGOFF_EVENT, or 
     *    WRAPPER_CTRL_SHUTDOWN_EVENT
     */
    void controlEvent(int event);
}
----

An application implementing the WrapperListener should not directly start the
application in the main method.  Everything that would normally be in the main
method should be moved into the start method.
The main method should simply contain the following code:

----
public class MyApp implements WrapperListener {
    ... Application methods ...
    ... WrapperListener methods...

    public static void main(String[] args) {
        // Start the application.  If the JVM was launched from the native
        //  Wrapper then the application will wait for the native Wrapper to
        //  call the application's start method.  Otherwise the start method
        //  will be called immediately.
        WrapperManager.start(new MyApp(), args);
    }
}
----

Configuration file Overview:
Wrapper uses a configuration file to control how Java is launched.
The Wrapper config file contains the following information:
----
#********************************************************************
# Wrapper parameters
#********************************************************************
# Java Application
wrapper.java.command=@java.home@/bin/java

# Java Main class
wrapper.java.mainclass=com.silveregg.wrapper.test.Main

# Java Classpath (include wrapper.jar)  Add class path elements as needed starting from 1
wrapper.java.classpath.1=@wrapper.home@/lib/wrapper.jar
wrapper.java.classpath.2=@wrapper.home@/lib/wrappertest.jar

# Java Library Path (location of wrapper.lib)
wrapper.java.library.path=@java.library.path@

# Java Additional Parameters
#wrapper.java.additional.1=-Xrs
#wrapper.java.additional.2=-Xcheck:jni
#wrapper.java.additional.3=-verbose:jni

# Initial Java Heap Size (in MB)
wrapper.java.initmemory=16

# Maximum Java Heap Size (in MB)
wrapper.java.maxmemory=64

# Application parameters.  Add parameters as needed starting from 1
wrapper.app.parameter.1=

# Port which the native wrapper code will attempt to connect to
wrapper.port=1777

# Log file to use for wrapper output logging.
wrapper.logfile=testwrapper.log

# Number of seconds to allow for the JVM to be launched and contact the wrapper before the
#  wrapper should assume that the JVM is hung and terminate the JVM process.  0 means never
#  time out.  Defaults to 30 seconds.
wrapper.startup.timeout=30

# Number of seconds to allow between the wrapper pinging the JVM and the response.  0 means
#  never time out.  Defaults to 30 seconds.
wrapper.ping.timeout=30

# Show debug messages
wrapper.debug=TRUE

#********************************************************************
# Wrapper Unix daemon parameters
#********************************************************************
# File to write process ID to
wrapper.pidfile=/var/run/aigent.pid

#********************************************************************
# Wrapper NT Service parameters
#********************************************************************
# WARNING - Do not modify any of these parameters when an application
#  using this configuration file has been installed as a service.
#  Please uninstall the service before modifying this section.  The
#  service can then be reinstalled.

# Name of the service
wrapper.ntservice.name=WrapperTest

# Display name of the service
wrapper.ntservice.displayname=Wrapper Test

# Service dependencies.  Add dependencies as needed starting from 1
wrapper.ntservice.dependency.1=

# Mode in which the service is installed.  AUTO_START or DEMAND_START
wrapper.ntservice.starttype=AUTO_START
----

Explanation of wrapper configuration file properties:

Platform independant properties:
wrapper.java.command
    Location of the Java executable
    Example:
        wrapper.java.command=C:/jdk1.3/jre/bin/java

wrapper.java.mainclass=com.silveregg.wrapper.test.Main
    Class to execute when the wrapper starts the application.  If your
    application implements WrapperListener, then this will be that main
    class.  Otherwise, this should be com.silveregg.wrapper.WrapperSimpleApp
    Then your main class will be listed as the first application parameter
    below.
    Example (WrapperListener case):
        wrapper.java.mainclass=com.widgetsrus.MyAppMain
    Example (WrapperSimpleApp case):
        wrapper.java.mainclass=com.silveregg.wrapper.WrapperSimpleApp

wrapper.java.classpath.<n>
    Java Classpath to use.  You should have a series of properties listing up
    the various class path elements to use when launching the application.
    Each element has a property name which starts with wrapper.java.classpath.
    and ends with an integer number counting up from 1.  There can be no
    missing numbers.
    This list must contain the wrapper.jar file.  It can contain jar files as
    well as directories contains class files.
    Example:
        wrapper.java.classpath.1=C:/MyApp/lib/wrapper.jar
        wrapper.java.classpath.2=C:/MyApp/lib/myapp.jar
        wrapper.java.classpath.3=C:/MyApp/lib/mysql.jar
        wrapper.java.classpath.4=C:/MyApp/classes

wrapper.java.library.path
    Value of the Java library path.  This is a directory which contains any
    native (JNI) libraries used by the application.  You must place the
    Wrapper.DLL or libWrapper.so file in the directory specified.
    Example:
        wrapper.java.library.path=C:/MyApp/lib

wrapper.java.additional.<n>
    Additional Java parameters to pass to Java when it is launched.  These are
    not parameters for your application, but rather parameters for the Java
    virtual machine.  Each element has a property name which starts with 
    wrapper.java.additional. and ends with an integer number counting up 
    from 1.  There can be no missing numbers.
    For each entry, you can also have a corresponding strip quotes flag which
    will remove the quotes from the element when run on Linux or Solaris
    machines.  This was necessary to allow for file references which contain
    spaces.  For parameters like the one below it would not otherwise be
    possible to decide when to remove quotes and when to leave them alone
    Example:
        wrapper.java.additional.1=-Xrs
        wrapper.java.additional.2=-Dprop=TRUE
        wrapper.java.additional.3=-Dmyapp.data="C:/Program Files/MyAppData"
        wrapper.java.additional.3.stripquotes=TRUE

wrapper.java.initmemory
    The initial amount of memory in megabytes that the JVM should use.  Some
    JVMs work more efficiently if this value is high rather than low.
    Example:
        wrapper.java.initmemory=16

wrapper.java.maxmemory
    The maximum amount of memory in megabytes that the JVM will be allowed to
    use.  Java is set to 64MB by default.
    Example:
        wrapper.java.maxmemory=64

wrapper.app.parameter.<n>
    Application parameters to pass to your application when it is launched.
    These are the parameters passed to your application's main method.
    Each element has a property name which starts with 
    wrapper.java.parameter. and ends with an integer number counting up 
    from 1.  There can be no missing numbers.
    When the value of wrapper.java.mainclass is 
    com.silveregg.wrapper.WrapperSimpleApp, the first parameter to the
    application must be the name of the class which contains your main
    method.  All other parameters are then passed to your application's
    main method in order.
    For each entry, you can also have a corresponding strip quotes flag which
    will remove the quotes from the element when run on Linux or Solaris
    machines.  This was necessary to allow for file references which contain
    spaces.  For parameters like the one below it would not otherwise be
    possible to decide when to remove quotes and when to leave them alone
    Example:
        wrapper.java.mainclass=com.silveregg.wrapper.WrapperSimpleApp
        wrapper.app.parameter.1=com.widgetsrus.MyAppMain
        wrapper.app.parameter.2=-d
        wrapper.app.parameter.3=-c"C:/MyApp/conf/myapp.conf"
        wrapper.app.parameter.3.stripquotes=TRUE
        wrapper.app.parameter.4=-p4

wrapper.port
    A port used to communicate between the Wrapper executable and the
    Java application.  Used to monitor the health of the Java application.
    This can be any value. If at runtime, the specified value is in use,
    then a warning will be printed to the console and the next available
    port will be used in its place.
    Example:
        wrapper.port=1777

wrapper.logfile
    Log file to which all output to the console will be logged.  The logfile
    also contains timestamps.
    Example:
        wrapper.logfile=testwrapper.log

wrapper.startup.timeout
    Number of seconds to allow for the JVM to be launched and contact the
    wrapper before the wrapper should assume that the JVM is hung and 
    terminate the JVM process.  0 means never time out.  Defaults to 
    30 seconds.  Increase this value if your application takes too long
    to start.  You can also make calls to the WrapperManager.signalStarting(n)
    method to indicate that the application is not hung, but needs more
    time to start.
    Example:
        wrapper.startup.timeout=30

wrapper.ping.timeout
    Number of seconds to allow between the wrapper pinging the JVM and the
    response.  0 means never time out.  Defaults to 30 seconds.  Usually you
    do not need to change this, however, if your system often has periods of
    time where the CPU reaches 100% and you are having problems with the
    Wrapper thinking that the JVM has hung, this can be lengthened.  Be
    careful though, if you set this to one hour, then the Wrapper will not
    restart a hung JVM for one hour.
    Example:
        wrapper.ping.timeout=30

wrapper.debug
    Turn on detailed Wrapper debug information.
    Example:
        wrapper.debug=TRUE

Unix specific properties:
wrapper.pidfile
    File to write process ID to
    Example:
        wrapper.pidfile=/var/run/myapp.pid

Windows NT/2000 specific properties:
NT Service properties:
  WARNING - Do not modify any of these parameters when an application
    using this configuration file has been installed as a service.
    Please uninstall the service before modifying this section.  The
    service can then be reinstalled.

wrapper.ntservice.name
    Name of the NT service when installed
    Example:
        wrapper.ntservice.name=MyApp

wrapper.ntservice.displayname
    Display name of the NT service when installed
    Example:
        wrapper.ntservice.displayname=My Application

wrapper.ntservice.dependency.<n>
    Names of any other Services which must be running before this service
    can be started.  Stopping any of the listed services will also stop
    this service.  Add dependencies as needed starting from 1
    Example:
        wrapper.ntservice.dependency.1=MySQL

wrapper.ntservice.starttype
    Mode in which the service is installed.  AUTO_START starts the service
    automatically when the system is rebooted. Or DEMAND_START which requires
    that the service me started manually.
    Example:
        wrapper.ntservice.starttype=AUTO_START


Launching your application under Wrapper.

Windows NT/2000:

To Run your application as a console, execute the following command.
    C:/MyApp/bin> Wrapper.exe -c C:\MyApp\conf\wrapper.conf

To install the application as an NT service, execute.
    C:/MyApp/bin> Wrapper.exe -i C:\MyApp\conf\wrapper.conf
This will create a file in the bin directory called ~serviceMyApp.exe
This file contains all information needed to execute your application as
an NT service and should not be run manually.
And then to start it, either reboot, go to the services control panel, or
execute:
    C:/MyApp/bin> net start MyApp

To uninstall the application as an NT service, execute.
    C:/MyApp/bin> Wrapper.exe -r C:\MyApp\conf\wrapper.conf

These commands should normally be placed in batch files in the application's
bin directory to make them easier to use.

Linux/Solaris:

To run your application from the command line:
    $ wrapper -c [your application's wrapper config file]

In the src/bin subdirectory you will find some shell (bash and sh)script 
templates for starting and stopping wrapped applications cleanly in. To
use these scripts with your application, just set the APP_HOME (base dir
of your application), APP_NAME, APP_LONG_NAME, and WRAPPER_CONF (path to
your application's wrapper config file) variables in the scripts to the 
appropriate values.

You may need to tweak some of the other values (like PIDDIR in the sh
script) to get them to fit your environment. 


