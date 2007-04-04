# Makefile for NullLogic Embedded Scripting Language

CC   = gcc
MAKE = gmake

all: _libnesla _libneslaext _libneslatcp _nesla
	@./bin/nesla scripts/samples/hello.nes

install: all
	@cp -pR bin/nesla /usr/bin/nesla

_nesla:
	@$(MAKE) -C src/sapi/cli

_libnesla:
	@$(MAKE) -C src/libnesla

_libneslaext:
	@$(MAKE) -C src/libneslaext

_libneslatcp:
	@$(MAKE) -C src/libneslatcp

nullgw: all
	@$(MAKE) -C src/sapi/nullgw

ctest: all
	@$(MAKE) -C src/sapi/ctest
	@./bin/ctest_d

test:
	@./bin/nesla scripts/tests/test1.nes

test2:
	@./bin/nesla scripts/tests/test2.nes

debug:
	@gdb --args ./bin/nesla_d scripts/tests/test1.nes

strace:
	@strace ./bin/nesla_d scripts/tests/test1.nes

valgrind:
	@valgrind -v --leak-check=full --show-reachable=yes ./bin/nesla_d scripts/tests/test1.nes

clean:
	@$(MAKE) -s -C src/libnesla clean
	@$(MAKE) -s -C src/libneslaext clean
	@$(MAKE) -s -C src/libneslatcp clean
	@$(MAKE) -s -C src/sapi/cli clean
	@$(MAKE) -s -C src/sapi/ctest clean
	@$(MAKE) -s -C src/sapi/nullgw clean
	@rm -rf autom4te.cache config.log config.status *~
	@rm -f bin/*.exe lib/*.lib
	@rm -rf obj

ver:
	@joe `grep -lR "0\.2\." *`
	@rm `find -name *~`

distclean: clean
