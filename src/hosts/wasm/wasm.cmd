@cls
@set CC=emcc
@set CFLAGS=-I ../../../include -s WASM=1 -s EXPORT_ALL=1 -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']" -O1
@set OBJECTS=main.c ../../libnsp/block.c ../../libnsp/compile.c ../../libnsp/debug.c ../../libnsp/exec.c ../../libnsp/libc.c ../../libnsp/libn.c ../../libnsp/objects.c ../../libnsp/opcodes.c ../../libnsp/parser.c
@set TARGETDIR=..\..\..\bin\wasm
@set TARGET=%TARGETDIR%\nsp-wasm.js
@mkdir %TARGETDIR%
%CC% -o %TARGET% %OBJECTS% %CFLAGS%
