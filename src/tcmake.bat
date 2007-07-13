@echo off
rem set path=c:\tc\bin;%path%
if "%1%"=="clean" goto clean
if "%1%"=="hack" goto hack
if "%1%"=="test" goto test

make -fmakefile.tc
nesla.exe ..\scripts\samples\hello.nes
goto end

:clean
del *.obj
del nesla.cfg
goto end

:hack
del *.obj
tcc -w-par -ml -v- -I..\include -enesla_m.exe libnesla\*.c hosts\min\main.c
rem if all else fails, you can build it complete with just the next line by itself
tcc -w-par -ml -v- -I..\include -enesla.exe -DHAVE_MATH libnesla\*.c libs\ext\*.c libs\math\*.c hosts\cli\main.c
del *.obj
nesla.exe -e "var x='\n\ngnikrow dna ';print('Standard Interpreter built');for (i=string.len(x);i>=0;i--) print(string.sub(x, i, 1));"
goto end

:test
nesla.exe ..\scripts\tests\test1.nes
goto end

:end
