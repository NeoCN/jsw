@echo off
echo Building C Targets
call %1\VC98\BIN\VCVARS32.BAT

NMAKE.EXE -f Wrapper64.mak CFG="Wrapper - Win64 Release" ALL
if not errorlevel 1 goto ok1
exit %ERRORLEVEL%

:ok1
NMAKE.EXE -f WrapperJNI64.mak CFG="WrapperJNI - Win64 Release" ALL
if not errorlevel 1 goto end
exit %ERRORLEVEL%

:end
