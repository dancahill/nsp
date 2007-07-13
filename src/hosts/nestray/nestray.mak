# Microsoft Developer Studio Generated NMAKE File, Based on nestray.dsp
CFG=NesTray - Win32 Release

!IF "$(CFG)" != "NesTray - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nestray.mak" CFG="NesTray - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NesTray - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=.\..\..\..\bin
INTDIR=.\..\..\..\obj\NesTray
# Begin Custom Macros
OutDir=.\..\..\..\bin
# End Custom Macros

ALL : "$(OUTDIR)\nestray.exe"


CLEAN :
	-@erase "$(INTDIR)\nestray.obj"
	-@erase "$(INTDIR)\nestray.res"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\nestray.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\..\..\include" /D "HAVE_ODBC" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\nestray.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\nestray.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /pdb:none /machine:I386 /out:"$(OUTDIR)\nestray.exe" 
LINK32_OBJS= \
	"$(INTDIR)\nestray.obj" \
	"$(INTDIR)\nestray.res" \
	"..\..\..\lib\libnesla.lib" \
	"..\..\..\lib\libneslaext.lib" \
	"..\..\..\lib\libneslamath.lib" \
	"..\..\..\lib\libneslaodbc.lib" \
	"..\..\..\lib\libneslatcp.lib" \
	"..\..\..\lib\libneslazip.lib"

"$(OUTDIR)\nestray.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<


!IF "$(CFG)" == "NesTray - Win32 Release"
SOURCE=.\nestray.c

"$(INTDIR)\nestray.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\nestray.rc

"$(INTDIR)\nestray.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

