@echo off
echo Building C Targets
call %1\VC98\BIN\VCVARS32.BAT

NMAKE.EXE -f Wrapper.mak CFG="Wrapper - Win32 Release" ALL
if not errorlevel 1 goto ok1
exit %ERRORLEVEL%

:ok1
NMAKE.EXE -f WrapperJNI.mak CFG="WrapperJNI - Win32 Release" ALL
if not errorlevel 1 goto end
exit %ERRORLEVEL%

:end
