# Microsoft Developer Studio Generated NMAKE File, Based on WrapperJNI.dsp
!IF "$(CFG)" == ""
CFG=WrapperJNI - Win32 Debug
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの WrapperJNI - Win32 Debug を設定します。
!ENDIF 

!IF "$(CFG)" != "WrapperJNI - Win32 Release" && "$(CFG)" != "WrapperJNI - Win32 Debug"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "WrapperJNI.mak" CFG="WrapperJNI - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "WrapperJNI - Win32 Release" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE "WrapperJNI - Win32 Debug" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "WrapperJNI - Win32 Release"

OUTDIR=.\WrapperJNI___Win32_Release
INTDIR=.\WrapperJNI___Win32_Release

ALL : "..\..\lib\Wrapper.dll"


CLEAN :
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\wrapperjni.obj"
	-@erase "$(INTDIR)\wrapperjni_unix.obj"
	-@erase "$(INTDIR)\wrapperjni_win.obj"
	-@erase "$(OUTDIR)\Wrapper.exp"
	-@erase "$(OUTDIR)\Wrapper.lib"
	-@erase "..\..\lib\Wrapper.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "D:\Sun\j2sdk1.4.0_03\include\\" /I "D:\Sun\j2sdk1.4.0_03\include\win32\\" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "WRAPPERJNI_EXPORTS" /Fp"$(INTDIR)\WrapperJNI.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\WrapperJNI.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\Wrapper.pdb" /machine:I386 /out:"../../lib/Wrapper.dll" /implib:"$(OUTDIR)\Wrapper.lib" 
LINK32_OBJS= \
	"$(INTDIR)\wrapperjni.obj" \
	"$(INTDIR)\wrapperjni_unix.obj" \
	"$(INTDIR)\wrapperjni_win.obj"

"..\..\lib\Wrapper.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "WrapperJNI - Win32 Debug"

OUTDIR=.\WrapperJNI___Win32_Debug
INTDIR=.\WrapperJNI___Win32_Debug
# Begin Custom Macros
OutDir=.\WrapperJNI___Win32_Debug
# End Custom Macros

ALL : "..\..\lib\Wrapper.dll" "$(OUTDIR)\WrapperJNI.bsc"


CLEAN :
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\wrapperjni.obj"
	-@erase "$(INTDIR)\wrapperjni.sbr"
	-@erase "$(INTDIR)\wrapperjni_unix.obj"
	-@erase "$(INTDIR)\wrapperjni_unix.sbr"
	-@erase "$(INTDIR)\wrapperjni_win.obj"
	-@erase "$(INTDIR)\wrapperjni_win.sbr"
	-@erase "$(OUTDIR)\Wrapper.exp"
	-@erase "$(OUTDIR)\Wrapper.lib"
	-@erase "$(OUTDIR)\Wrapper.pdb"
	-@erase "$(OUTDIR)\WrapperJNI.bsc"
	-@erase "..\..\lib\Wrapper.dll"
	-@erase "..\..\lib\Wrapper.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "D:\Sun\j2sdk1.4.0\include\\" /I "D:\Sun\j2sdk1.4.0\include\win32\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "WRAPPERJNI_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\WrapperJNI.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\WrapperJNI.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\wrapperjni.sbr" \
	"$(INTDIR)\wrapperjni_unix.sbr" \
	"$(INTDIR)\wrapperjni_win.sbr"

"$(OUTDIR)\WrapperJNI.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\Wrapper.pdb" /debug /machine:I386 /out:"../../lib/Wrapper.dll" /implib:"$(OUTDIR)\Wrapper.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\wrapperjni.obj" \
	"$(INTDIR)\wrapperjni_unix.obj" \
	"$(INTDIR)\wrapperjni_win.obj"

"..\..\lib\Wrapper.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("WrapperJNI.dep")
!INCLUDE "WrapperJNI.dep"
!ELSE 
!MESSAGE Warning: cannot find "WrapperJNI.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "WrapperJNI - Win32 Release" || "$(CFG)" == "WrapperJNI - Win32 Debug"
SOURCE=.\wrapperjni.c

!IF  "$(CFG)" == "WrapperJNI - Win32 Release"


"$(INTDIR)\wrapperjni.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WrapperJNI - Win32 Debug"


"$(INTDIR)\wrapperjni.obj"	"$(INTDIR)\wrapperjni.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\wrapperjni_unix.c

!IF  "$(CFG)" == "WrapperJNI - Win32 Release"


"$(INTDIR)\wrapperjni_unix.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WrapperJNI - Win32 Debug"


"$(INTDIR)\wrapperjni_unix.obj"	"$(INTDIR)\wrapperjni_unix.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\wrapperjni_win.c

!IF  "$(CFG)" == "WrapperJNI - Win32 Release"


"$(INTDIR)\wrapperjni_win.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WrapperJNI - Win32 Debug"


"$(INTDIR)\wrapperjni_win.obj"	"$(INTDIR)\wrapperjni_win.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

