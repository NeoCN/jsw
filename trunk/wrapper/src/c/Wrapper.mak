# Microsoft Developer Studio Generated NMAKE File, Based on Wrapper.dsp
!IF "$(CFG)" == ""
CFG=Wrapper - Win32 Debug
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの Wrapper - Win32 Debug を設定します。
!ENDIF 

!IF "$(CFG)" != "Wrapper - Win32 Release" && "$(CFG)" != "Wrapper - Win32 Debug"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "Wrapper.mak" CFG="Wrapper - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "Wrapper - Win32 Release" ("Win32 (x86) Console Application" 用)
!MESSAGE "Wrapper - Win32 Debug" ("Win32 (x86) Console Application" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "Wrapper - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\wrapper.exe"


CLEAN :
	-@erase "$(INTDIR)\logger.obj"
	-@erase "$(INTDIR)\messages.res"
	-@erase "$(INTDIR)\property.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\wrapper.obj"
	-@erase "$(INTDIR)\wrapper_unix.obj"
	-@erase "$(INTDIR)\wrapper_win.obj"
	-@erase "$(INTDIR)\wrapperinfo.obj"
	-@erase "..\..\bin\wrapper.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\Wrapper.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

RSC=rc.exe
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\messages.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Wrapper.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib shlwapi.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\wrapper.pdb" /machine:I386 /out:"../../bin/wrapper.exe" 
LINK32_OBJS= \
	"$(INTDIR)\logger.obj" \
	"$(INTDIR)\property.obj" \
	"$(INTDIR)\wrapper.obj" \
	"$(INTDIR)\wrapper_unix.obj" \
	"$(INTDIR)\wrapper_win.obj" \
	"$(INTDIR)\messages.res" \
	"$(INTDIR)\wrapperinfo.obj"

"..\..\bin\wrapper.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Wrapper - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "..\..\bin\wrapper.exe" "$(OUTDIR)\Wrapper.bsc"


CLEAN :
	-@erase "$(INTDIR)\logger.obj"
	-@erase "$(INTDIR)\logger.sbr"
	-@erase "$(INTDIR)\messages.res"
	-@erase "$(INTDIR)\property.obj"
	-@erase "$(INTDIR)\property.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\wrapper.obj"
	-@erase "$(INTDIR)\wrapper.sbr"
	-@erase "$(INTDIR)\wrapper_unix.obj"
	-@erase "$(INTDIR)\wrapper_unix.sbr"
	-@erase "$(INTDIR)\wrapper_win.obj"
	-@erase "$(INTDIR)\wrapper_win.sbr"
	-@erase "$(INTDIR)\wrapperinfo.obj"
	-@erase "$(INTDIR)\wrapperinfo.sbr"
	-@erase "$(OUTDIR)\Wrapper.bsc"
	-@erase "$(OUTDIR)\wrapper.pdb"
	-@erase "..\..\bin\wrapper.exe"
	-@erase "..\..\bin\wrapper.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "DEBUG" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Wrapper.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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

RSC=rc.exe
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\messages.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Wrapper.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\logger.sbr" \
	"$(INTDIR)\property.sbr" \
	"$(INTDIR)\wrapper.sbr" \
	"$(INTDIR)\wrapper_unix.sbr" \
	"$(INTDIR)\wrapper_win.sbr" \
	"$(INTDIR)\wrapperinfo.sbr"

"$(OUTDIR)\Wrapper.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib shlwapi.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\wrapper.pdb" /debug /machine:I386 /out:"../../bin/wrapper.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\logger.obj" \
	"$(INTDIR)\property.obj" \
	"$(INTDIR)\wrapper.obj" \
	"$(INTDIR)\wrapper_unix.obj" \
	"$(INTDIR)\wrapper_win.obj" \
	"$(INTDIR)\messages.res" \
	"$(INTDIR)\wrapperinfo.obj"

"..\..\bin\wrapper.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("Wrapper.dep")
!INCLUDE "Wrapper.dep"
!ELSE 
!MESSAGE Warning: cannot find "Wrapper.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Wrapper - Win32 Release" || "$(CFG)" == "Wrapper - Win32 Debug"
SOURCE=.\logger.c

!IF  "$(CFG)" == "Wrapper - Win32 Release"


"$(INTDIR)\logger.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Wrapper - Win32 Debug"


"$(INTDIR)\logger.obj"	"$(INTDIR)\logger.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\property.c

!IF  "$(CFG)" == "Wrapper - Win32 Release"


"$(INTDIR)\property.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Wrapper - Win32 Debug"


"$(INTDIR)\property.obj"	"$(INTDIR)\property.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\wrapper.c

!IF  "$(CFG)" == "Wrapper - Win32 Release"


"$(INTDIR)\wrapper.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Wrapper - Win32 Debug"


"$(INTDIR)\wrapper.obj"	"$(INTDIR)\wrapper.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\wrapper_unix.c

!IF  "$(CFG)" == "Wrapper - Win32 Release"


"$(INTDIR)\wrapper_unix.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Wrapper - Win32 Debug"


"$(INTDIR)\wrapper_unix.obj"	"$(INTDIR)\wrapper_unix.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\wrapper_win.c

!IF  "$(CFG)" == "Wrapper - Win32 Release"


"$(INTDIR)\wrapper_win.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Wrapper - Win32 Debug"


"$(INTDIR)\wrapper_win.obj"	"$(INTDIR)\wrapper_win.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\wrapperinfo.c

!IF  "$(CFG)" == "Wrapper - Win32 Release"


"$(INTDIR)\wrapperinfo.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Wrapper - Win32 Debug"


"$(INTDIR)\wrapperinfo.obj"	"$(INTDIR)\wrapperinfo.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\messages.rc

"$(INTDIR)\messages.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

