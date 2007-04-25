@ECHO OFF
if not exist .\bin mkdir bin
if not exist .\lib mkdir lib
if not exist .\obj mkdir obj

if "%1%"=="test" goto test

@ECHO libnesla
CD SRC\LIBNESLA
NMAKE /C /S /F MAKEFILE.VC
CD ..\..

CD SRC\HOSTS\MIN
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
bin\nesla_m.exe scripts\samples\hello.nes

@ECHO libneslamath
CD SRC\LIBS\MATH
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
@ECHO libneslaext
CD SRC\LIBS\EXT
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
@ECHO libneslatcp
CD SRC\LIBS\TCP
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
@ECHO nesla-cli
CD SRC\HOSTS\CLI
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
bin\nesla.exe scripts\samples\hello.nes

@ECHO nesla-cgi
CD SRC\HOSTS\CGI
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..

goto end

:test
bin\nesla.exe scripts\tests\test1.nes

:end
