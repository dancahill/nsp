# Microsoft Developer Studio Project File - Name="NesTray" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=NesTray - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NesTray.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NesTray.mak" CFG="NesTray - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NesTray - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "obj"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\bin"
# PROP Intermediate_Dir "..\..\..\obj\NesTray"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "HAVE_ODBC" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\..\include" /D "HAVE_ODBC" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /pdb:none /machine:I386
# SUBTRACT LINK32 /map
# Begin Target

# Name "NesTray - Win32 Release"
# Begin Group "Headers"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\..\..\include\nesla\nesla.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Source"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=.\nestray.c
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter "*.ico, *.rc"
# Begin Source File

SOURCE=.\nestray.ico
# End Source File
# Begin Source File

SOURCE=.\nestray.rc
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\lib\libnesla.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\libneslaext.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\libneslamath.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\libneslaodbc.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\libneslatcp.lib
# End Source File
# End Target
# End Project
