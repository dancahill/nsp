# Makefile for NullLogic Embedded Scripting Language

CC   = gcc
MAKE = make

#all: _libnesla min deps _libneslaext _libneslamath _libneslatcp cli
all: _libnesla min deps _libneslaext _libneslaldap _libneslamath _libneslaodbc _libneslatcp cli
#	@./bin/nesla scripts/samples/hello.nes
	@echo "	Run 'make targets' to list more options."
	@echo

deps:
	@./bin/nesla_m -f config.nes

targets:
	@echo
	@echo "useful targets include:"
	@echo "	all        - Builds all basic libraries and the CLI (default)"
	@echo "	cli        - Builds the Command Line Interpreter"
	@echo "	min        - Builds a very minimal interpreter"
	@echo "	cgi        - Builds the CGI host program"
	@echo "	nullgw     - Builds a host module for NullLogic Groupware"
	@echo "	install    - Installs the cli binary to /usr/bin/nesla"
	@echo "	cgiinstall - Installs the cgi binary to /var/www/cgi-bin/nesla.cgi"
	@echo "	test       - Runs a suite of test scripts"
	@echo "	test2      - Runs a more aggressive version of the first test"
	@echo

_libnesla:
	@cd src/libnesla  && $(MAKE) && cd ../..

_libneslaext:
	@cd src/libs/ext  && $(MAKE) && cd ../..

_libneslaldap:
	@cd src/libs/ldap && $(MAKE) && cd ../..

_libneslamath:
	@cd src/libs/math && $(MAKE) && cd ../..

_libneslaodbc:
	@cd src/libs/odbc && $(MAKE) && cd ../..

_libneslatcp:
	@cd src/libs/tcp  && $(MAKE) && cd ../..

cgi: all
	@cd src/hosts/cgi && $(MAKE) && cd ../../..

cli:
	@cd src/hosts/cli && $(MAKE) && cd ../../..
	@echo
	@echo -n "Standard Interpreter built"
	@./bin/nesla -e "var x='gnikrow dna ';for (i=string.len(x);i>=0;i--) print(string.sub(x, i, 1));"
	@echo
	@echo

min:
	@cd src/hosts/min     && $(MAKE) -s && cd ../../..
	@echo
	@echo -n "Minimal Interpreter built"
	@./bin/nesla_m -e "var x='gnikrow dna ';for (i=string.len(x);i>=0;i--) print(string.sub(x, i, 1));"
	@echo
	@echo

nullgw: all
	@cd src/hosts/nullgw  && $(MAKE) && cd ../../..

install: all
	@cp -pR bin/nesla /usr/bin/nesla

cgiinstall: cgi
	@mkdir -p /var/www/cgi-bin
	cp -pR bin/nesla.cgi /var/www/cgi-bin/nesla.cgi

test:
	@./bin/nesla scripts/tests/test1.nes

test2:
	@./bin/nesla scripts/tests/test2.nes

distclean: clean

clean:
	@touch src/Rules.mak
	@cd src/libnesla      && $(MAKE) -s clean && cd ../..
	@cd src/libs/ext      && $(MAKE) -s clean && cd ../../..
	@cd src/libs/ldap     && $(MAKE) -s clean && cd ../../..
	@cd src/libs/math     && $(MAKE) -s clean && cd ../../..
	@cd src/libs/odbc     && $(MAKE) -s clean && cd ../../..
	@cd src/libs/tcp      && $(MAKE) -s clean && cd ../../..
	@cd src/hosts/cgi     && $(MAKE) -s clean && cd ../../..
	@cd src/hosts/cli     && $(MAKE) -s clean && cd ../../..
	@cd src/hosts/min     && $(MAKE) -s clean && cd ../../..
	@cd src/hosts/ctest   && $(MAKE) -s clean && cd ../../..
	@cd src/hosts/nullgw  && $(MAKE) -s clean && cd ../../..
	@rm -f autom4te.cache config.log config.status *~
	@rm -f bin/*.exe lib/*.lib src/Rules.mak
	@rm -rf obj

# the rest are more useful to me than they are to you

ver:
	@make clean
	@joe `grep -lR "0\.5\." *`
	@rm `find -name *~`

showbug:
	@./bin/nesla -d scripts/tests/test1.nes

debug:
	@gdb --args ./bin/nesla_d scripts/tests/test1.nes

strace:
	@strace ./bin/nesla_d scripts/tests/test1.nes

valgrind:
	@valgrind -v --leak-check=full --leak-resolution=high --show-reachable=yes ./bin/nesla_d scripts/tests/test1.nes

ctest: all
	@cd src/hosts/ctest   && $(MAKE) && cd ../../..
	@gdb --args ./bin/ctest_d

wc:
	@wc `find src/libnesla  -name *.[ch]`
	@wc `find src/libs/ext  -name *.[ch]`
	@wc `find src/libs/ldap -name *.[ch]`
	@wc `find src/libs/math -name *.[ch]`
	@wc `find src/libs/odbc -name *.[ch]`
	@wc `find src/libs/tcp  -name *.[ch]`

time:
	@echo -e "\n\e[01;37;40mNES - dynamic tests\e[00m"
	@time ./bin/nesla_m scripts/bench/speed.nes 2>/dev/null
	@time ./bin/nesla scripts/bench/speed.nes 2>/dev/null
