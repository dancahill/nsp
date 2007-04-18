# Makefile for NullLogic Embedded Scripting Language

CC   = gcc
MAKE = make

all: _libnesla min _libneslaext _libneslatcp cli
	@./bin/nesla scripts/samples/hello.nes
#	@./bin/nesla -e "print ('hello from nesla');if (type(http)!='null') { print('+http'); } print('\r\n');"

install: all
	@cp -pR bin/nesla /usr/bin/nesla

_libnesla:
	@cd src/libnesla    && $(MAKE) && cd ../..

_libneslaext:
	@cd src/libneslaext && $(MAKE) && cd ../..

_libneslatcp:
	@cd src/libneslatcp && $(MAKE) && cd ../..

cli:
	@cd src/sapi/cli    && $(MAKE) && cd ../../..

min: _libnesla
	@cd src/sapi/min    && $(MAKE) && cd ../../..
	@./bin/nesla_m scripts/samples/hello.nes

ctest: all
	@cd src/sapi/ctest  && $(MAKE) && cd ../../..
	@./bin/ctest_d

nullgw: all
	@cd src/sapi/nullgw && $(MAKE) && cd ../../..

test:
	@./bin/nesla scripts/tests/test1.nes

test2:
	@./bin/nesla scripts/tests/test2.nes

debug:
	@gdb --args ./bin/nesla_d scripts/tests/test1.nes

strace:
	@strace ./bin/nesla_d scripts/tests/test1.nes

valgrind:
	@valgrind -v --leak-check=full --leak-resolution=high --show-reachable=yes ./bin/nesla_d scripts/tests/test1.nes

clean:
	@cd src/libnesla    && $(MAKE) -s clean && cd ../..
	@cd src/libneslaext && $(MAKE) -s clean && cd ../..
	@cd src/libneslatcp && $(MAKE) -s clean && cd ../..
	@cd src/sapi/cli    && $(MAKE) -s clean && cd ../../..
	@cd src/sapi/min    && $(MAKE) -s clean && cd ../../..
	@cd src/sapi/ctest  && $(MAKE) -s clean && cd ../../..
	@cd src/sapi/nullgw && $(MAKE) -s clean && cd ../../..
	@rm -rf autom4te.cache config.log config.status *~
	@rm -f bin/*.exe lib/*.lib
	@rm -rf obj

ver:
	@make clean
	@joe `grep -lR "0\.4\." *`
	@rm `find -name *~`

distclean: clean

wc:
#	wc `find . -name *.[ch] ! -path './sqlite/*'
	@wc `find src/libnesla -name *.[ch]`
	@wc `find src/libneslaext -name *.[ch]`
	@wc `find src/libneslatcp -name *.[ch]`
