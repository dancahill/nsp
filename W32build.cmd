@ECHO OFF
mkdir bin
mkdir lib
mkdir obj
@ECHO libnesla
CD SRC\LIBNESLA
NMAKE /C /S /F MAKEFILE.VC
CD ..\..
@ECHO libneslaext
CD SRC\LIBNESLAEXT
NMAKE /C /S /F MAKEFILE.VC
CD ..\..
@ECHO nesla-cli
CD SRC\SAPI\CLI
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
bin\nesla.exe scripts\samples\hello.nes
