# Makefile for NullLogic Embedded Scripting Language

CC   = gcc
MAKE = make

all: _libnesla _libneslaext _nesla
	@./bin/nesla scripts/samples/hello.nes

_nesla:
	@$(MAKE) -C src/sapi/cli

_libnesla:
	@$(MAKE) -C src/libnesla

_libneslaext:
	@$(MAKE) -C src/libneslaext

install: _libnesla _nesla
	@cp -av bin/nesla /usr/bin/nesla

nullgw: _libnesla _nesla
	@$(MAKE) -C src/sapi/nullgw

test: _libnesla _nesla
	@./bin/nesla scripts/test1.nes

test2: _libnesla _nesla
	@./bin/nesla scripts/test2.nes

test3: _libnesla _nesla
	@./bin/nesla scripts/test3.nes

debug: _libnesla _nesla
	@gdb --args ./bin/nesla_d scripts/test.nes

strace: _libnesla _nesla
	@strace ./bin/nesla_d scripts/test.nes

clean:
	@$(MAKE) -s -C src/libnesla clean
	@$(MAKE) -s -C src/libneslaext clean
	@$(MAKE) -s -C src/sapi/cli clean
	@$(MAKE) -s -C src/sapi/nullgw clean
	@rm -rf autom4te.cache config.log config.status *~
	@rm -f bin/*.exe lib/*.lib
	@rm -rf obj

distclean: clean
