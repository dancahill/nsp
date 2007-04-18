@ECHO OFF
if not exist .\bin mkdir bin
if not exist .\lib mkdir lib
if not exist .\obj mkdir obj

if "%1%"=="test" goto test

@ECHO libnesla
CD SRC\LIBNESLA
NMAKE /C /S /F MAKEFILE.VC
CD ..\..

CD SRC\SAPI\MIN
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
bin\nesla_m.exe scripts\samples\hello.nes

@ECHO libneslaext
CD SRC\LIBNESLAEXT
NMAKE /C /S /F MAKEFILE.VC
CD ..\..
@ECHO libneslatcp
CD SRC\LIBNESLATCP
NMAKE /C /S /F MAKEFILE.VC
CD ..\..
@ECHO nesla-cli
CD SRC\SAPI\CLI
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
bin\nesla.exe scripts\samples\hello.nes
goto end

:test
bin\nesla.exe scripts\tests\test1.nes

:end
