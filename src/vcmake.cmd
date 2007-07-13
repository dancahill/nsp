@ECHO OFF
cd ..
if not exist .\bin mkdir bin
if not exist .\lib mkdir lib
if not exist .\obj mkdir obj

if "%1%"=="hack" goto hack
if "%1%"=="test" goto test

@ECHO libnesla
CD SRC\LIBNESLA
NMAKE /C /S /F MAKEFILE.VC
CD ..\..

CD SRC\HOSTS\MIN
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
bin\nesla_m.exe -e "var x='\n\ngnikrow dna ';print('Minimal Interpreter built');for (i=string.len(x);i>=0;i--) print(string.sub(x, i, 1));"

@ECHO libneslaext
CD SRC\LIBS\EXT
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
@ECHO libneslamath
CD SRC\LIBS\MATH
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
@ECHO libneslaodbc
CD SRC\LIBS\ODBC
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
@ECHO libneslatcp
CD SRC\LIBS\TCP
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
@ECHO libneslazip
CD SRC\LIBS\ZIP
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
@ECHO nesla-cli
CD SRC\HOSTS\CLI
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..
bin\nesla.exe -e "var x='\n\ngnikrow dna ';print('Standard Interpreter built');for (i=string.len(x);i>=0;i--) print(string.sub(x, i, 1));"
rem bin\nesla.exe scripts\samples\hello.nes

@ECHO NesTray
CD SRC\HOSTS\NESTRAY
NMAKE /C /S /F NESTRAY.MAK
CD ..\..\..

@ECHO nesla-cgi
CD SRC\HOSTS\CGI
NMAKE /C /S /F MAKEFILE.VC
CD ..\..\..

cd src

goto end

:hack
cl.exe /DWIN32 /I./include src/libnesla/*.c src/hosts/min/main.c -Febin/nesla_h.exe
del *.obj
goto end

:test
bin\nesla.exe scripts\tests\test1.nes

:end
