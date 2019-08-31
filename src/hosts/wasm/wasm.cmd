@echo off
set CC=emcc
set CFLAGS=-I../../../include -s WASM=1 -s EXPORT_ALL=1 -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']" -O1
set OBJECTS=main.c ../../libnsp/block.c ../../libnsp/compile.c ../../libnsp/debug.c ../../libnsp/exec.c ../../libnsp/libc.c ../../libnsp/libn.c ../../libnsp/objects.c ../../libnsp/opcodes.c ../../libnsp/parser.c
set LIBBASE=../../libs/base/base.c ../../libs/base/base64.c ../../libs/base/dir.c ../../libs/base/file.c ../../libs/base/pipe.c ../../libs/base/regex.c ../../libs/base/rot13.c ../../libs/base/sort.c ../../libs/base/thread.c ../../libs/base/winapi.c
set TARGETDIR=..\..\..\bin\wasm
set TARGET=%TARGETDIR%\nsp-wasm.js
if not exist %TARGETDIR%\ ( mkdir %TARGETDIR% && echo %TARGETDIR% created)
echo %CC% %CFLAGS% -o %TARGET% %OBJECTS% %LIBBASE%
%CC% %CFLAGS% -o %TARGET% %OBJECTS% %LIBBASE%
