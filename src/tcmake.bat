@echo off
set path=c:\tc\bin;%path%
if "%1%"=="clean" goto clean
if "%1%"=="test" goto test

make -fmakefile.tc
nesla.exe ..\scripts\samples\hello.nes
goto end

:clean
del *.obj
del nesla.cfg
goto end

:test
nesla.exe ..\scripts\tests\test1.nes
goto end

:end
