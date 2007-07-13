@echo off
rem set path=c:\tcc\tcc;%path%
if "%1%"=="clean" goto clean
if "%1%"=="hack" goto hack
if "%1%"=="test" goto test

rem make -fmakefile.tcc
nesla.exe ..\scripts\samples\hello.nes
goto end

:clean
del *.obj
del nesla.cfg
goto end

:hack
tcc -DTINYCC -IC:\tcc\include -I..\include -LC:\tcc\lib -ltcc -o nesla_m.exe libnesla\*.c hosts\min\main.c
tcc -DTINYCC -IC:\tcc\include -I..\include -LC:\tcc\lib -ltcc -o nesla.exe libnesla\*.c libs\ext\*.c libs\math\*.c hosts\cli\main.c
nesla.exe -e "var x='\n\ngnikrow dna ';print('Standard Interpreter built');for (i=string.len(x);i>=0;i--) print(string.sub(x, i, 1));"
goto end

:test
nesla.exe ..\scripts\tests\test1.nes
goto end

:end
