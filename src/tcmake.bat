@echo off
set path=c:\tc\bin;%path%
make -fmakefile.tc
nesla.exe ..\scripts\samples\hello.nes
rem nesla.exe ..\scripts\tests\test1.nes
