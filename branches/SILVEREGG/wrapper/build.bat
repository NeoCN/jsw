@echo off

echo -------------------
echo Aigent Build System
echo -------------------

set OLD_ANT_HOME=%ANT_HOME%
set OLD_AIGENT_TOOLS=%AIGENT_TOOLS%

if not "%AIGENT_TOOLS%"=="" goto runAnt

if exist "tools\bin\ant.bat" set AIGENT_TOOLS=tools

if not "%AIGENT_TOOLS%"=="" goto runAnt

echo "Unable to locate tools directory at tools/."
echo "Aborting."
goto end

:runAnt
set ANT_HOME=%AIGENT_TOOLS%
call %AIGENT_TOOLS%\bin\ant.bat -logger org.apache.tools.ant.NoBannerLogger -emacs -Dtools.dir=%AIGENT_TOOLS% %1 %2 %3 %4 %5 %6 %7 %8

:end
set AIGENT_TOOLS=%OLD_AIGENT_TOOLS%
set OLD_AIGENT_TOOLS=
set ANT_HOME=%OLD_ANT_HOME%
set OLD_ANT_HOME=

