# NullLogic Embedded Scripting Language
# GCC Makefile
# run 'make'

CC      = gcc
MAKE    = make
NESLITE = ./bin/neslite

#.SILENT:

all: _all
	@echo "	Run 'make targets' to list more options."
	@echo

_all: dirs libnesla lite deps static_libs cli

dirs:
	@mkdir -p bin
	@mkdir -p lib
	@mkdir -p lib/shared
	@mkdir -p int

libnesla: dirs
	@cd src/libnesla     && $(MAKE) && cd ../..

deps: lite
	@cd src/libs && ../../$(NESLITE) autoconf.nes gcc posix && cd ../..

depsl: lite
	@cd src/libs && ../../$(NESLITE) autoconfl.nes gcc posix && cd ../..

reconf: lite
	@rm -f src/config.???
	@cd src/libs && ../../$(NESLITE) autoconf.nes gcc posix && cd ../..

# rules for libs

static_libs: deps
	@cd src/libs/cdb     && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/crypto  && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/dl      && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/ext     && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/ldap    && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/math    && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/mysql   && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/odbc    && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/pipe    && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/pgsql   && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/regex   && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/sqlite3 && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/ssh     && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/tcp     && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..
	@cd src/libs/zip     && ../../../$(NESLITE) -e "global _LINKAGE_='static';include('Makefile.nes');" && cd ../../..

static: static_libs

shared_libs: deps
	@cd src/libs/cdb      && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/crypto   && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/ext      && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/ldap     && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/math     && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/mysql    && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/odbc     && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/pipe     && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/pgsql    && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/regex    && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/sqlite3  && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/src2html && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/ssh      && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/tcp      && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/test     && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
	@cd src/libs/zip      && ../../../$(NESLITE) -e "global _LINKAGE_='shared';include('Makefile.nes');" && cd ../../..
#	@./bin/nesla -e "dl.loadlib('./lib/shared/libneslatest.so');print('\n'+dltest()+'\n');"

shared: shared_libs

# rules for hosts
cgi: _all
	@cd src/hosts/cgi     && $(MAKE) && cd ../../..

cli:
	@cd src/hosts/cli     && $(MAKE) -s && cd ../../..
	@echo
	@echo -n "Standard Interpreter built"
	@./bin/nesla -e "var x='gnikrow dna ';for (i=string.len(x);i>=0;i--) print(string.sub(x, i, 1));"
	@echo
	@echo

ctest: _all
	@cd src/hosts/ctest   && $(MAKE) && cd ../../..
	@gdb --args ./bin/ctest_d

lite: libnesla
	@cd src/hosts/lite    && $(MAKE) -s && cd ../../..
	@echo
	@echo -n "Minimal Interpreter built"
	@$(NESLITE) -e "var x='gnikrow dna ';for (i=string.len(x);i>=0;i--) print(string.sub(x, i, 1));"
	@echo
	@echo

nullgw: _all
	@cd src/hosts/nullgw  && $(MAKE) && cd ../../..

# rules for everything else
clean: #lite
	@touch src/config.mak
	@cd src/libnesla      && $(MAKE) -s clean && cd ../..
	@cd src/hosts/cgi     && $(MAKE) -s clean && cd ../../..
	@cd src/hosts/cli     && $(MAKE) -s clean && cd ../../..
	@cd src/hosts/ctest   && $(MAKE) -s clean && cd ../../..
	@cd src/hosts/nullgw  && $(MAKE) -s clean && cd ../../..
	@cd src/hosts/lite    && $(MAKE) -s clean && cd ../../..
#	@cd src/libs/cdb      && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/crypto   && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/dl       && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/ext      && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/ldap     && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/math     && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/mysql    && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/odbc     && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/pipe     && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/pgsql    && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/regex    && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/sqlite3  && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/src2html && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/ssh      && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/test     && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/tcp      && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/winapi   && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@cd src/libs/zip      && ../../../$(NESLITE) Makefile.nes clean && cd ../../..
#	@rm -rf bin lib int `find -name *.o`
	@rm -rf bin lib int

distclean: clean
	@rm -rf bin lib int
	@rm -f src/config.???

install: _all
	@cp -pR bin/nesla /usr/bin/nesla

install_libs: _all shared install
	@mkdir -p /usr/local/lib/nesla
	@cp -pR lib/*.a /usr/local/lib/nesla/
	@cp -pR lib/shared/*.so /usr/local/lib/nesla/

# minimal one-line command for making a minimal unoptimized binary
hack:
	$(CC) -I./include src/libnesla/*.c src/hosts/lite/main.c -o bin/nesla_h

tcchack:
	tcc -I./include -o neslite src/libnesla/*.c src/hosts/lite/main.c
	./neslite -e "var x='\n\ngnikrow dna ';print('Standard Interpreter built');for (i=string.len(x);i>=0;i--) print(string.sub(x, i, 1));"

targets:
	@echo
	@echo "useful targets include:"
	@echo "	all         - Builds all basic libraries and the CLI (default)"
	@echo "	cli         - Builds the Command Line Interpreter"
	@echo "	lite        - Builds a very minimal interpreter"
	@echo "	shared_libs - Builds all extension modules as shared libraries"
	@echo "	static_libs - Builds all extension modules as static libraries"
	@echo "	cgi         - Builds the CGI host program"
	@echo "	nullgw      - Builds a host module for NullLogic Groupware"
	@echo "	install     - Installs the cli binary to /usr/bin/nesla"
	@echo "	cgiinstall  - Installs the cgi binary to /var/www/cgi-bin/nesla.cgi"
	@echo "	test        - Runs a suite of test scripts"
	@echo "	test2       - Runs a more aggressive version of the first test"
	@echo

cgiinstall: cgi
	@mkdir -p /var/www/cgi-bin
	cp -pR bin/nesla.cgi /var/www/cgi-bin/nesla.cgi

test:
	@./bin/nesla scripts/tests/test1.nes

testlite:
	@$(NESLITE) -f scripts/tests/test1.nes

test2:
	@./bin/nesla scripts/tests/test2.nes

# the rest are more useful to me than they are to you

ver:
	@make clean
	@joe `grep -lR "0\.9\." *`
	@rm `find -name *~`

showbug:
	@./bin/nesla -d scripts/tests/test1.nes

debug:
	@gdb --args ./bin/nesla_d scripts/tests/test1.nes

debuglite:
	@gdb --args ./bin/neslite_d scripts/tests/test1.nes

strace:
	@strace ./bin/nesla_d scripts/tests/test1.nes

valgrind:
	@valgrind -v --leak-check=full --leak-resolution=high --show-reachable=yes ./bin/nesla_d scripts/tests/test1.nes

valgrindlite:
	@valgrind -v --leak-check=full --leak-resolution=high --show-reachable=yes $(NESLITE)_d scripts/tests/test1.nes

datestamp:
#	cd scripts && touch --date="`date +%Y-%m-%d` 01:00:00" `find *` && cd ..
#	touch --date="`date +'%Y-%m-%d %H:%M:%S'`" `find *`
	touch --date="`date +'%Y-%m-%d %H:00:00'`" `find *`

wc:
	@wc `find . -name *.[ch]`
	@wc src/libnesla/*.c

docs:
	@cd doc/syntax && ../../$(NESLITE) syntax.nes && cd ../..

time:
	@$(NESLITE) -f scripts/tests/speed.nes
