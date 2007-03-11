@echo off
setlocal

rem Copyright (c) 1999, 2007 Tanuki Software Inc.
rem
rem Java Service Wrapper windows build script.  This script is designed to be
rem  called by the ant build.xml file.
rem

rem %1 Visual Studio environment script
rem %2 Makefile name
rem %3 build targets
rem %4 build targets
rem %5 build targets

echo Configuring the Visual Studio environment...
call %1

echo Run the make file with the %3 build target...
nmake /f %2 /c %3 %4 %5
