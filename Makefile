CFLAGS=-std=c11 -g -fopenmp -O2 -Wall -Wextra -Ideps -Ideps/simulations -DNDEBUG $(OPTFLAGS)
LFLAGS=-Lbuild -lm $(OPTLIBS)

SOURCES=$(wildcard src/replicator_*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
DEPSOURCES=$(wildcard deps/*.c)
DEPOBJECTS=$(patsubst %.c,%.o,$(DEPSOURCES))

LIBS=build/libreplicator.a build/libreplicator.so

TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst %.c,%,$(TEST_SRC))

TARGET=build/replicator_sim

# The Target Build
all: libs $(TARGET)

deps: build
	clib install -o deps clibs/commander clibs/timestamp
	clib install -o deps/simulations gsmcwhirter/c-simulations

libs: $(LIBS)

$(LIBS): build
	(cd deps/simulations && CC=$(CC) SRC=. make all)
	cp $(patsubst build/%,deps/simulations/build/%,$(LIBS)) build/

dev: CFLAGS=-g -fopenmp -Wall -Wextra -Iinclude $(OPTFLAGS)
dev: all

$(TARGET): CFLAGS += -fPIC
$(TARGET): LFLAGS += -lreplicator
$(TARGET): build $(DEPOBJECTS) $(OBJECTS)
	$(CC) $(CFLAGS) $(DEPOBJECTS) $(OBJECTS) $(LFLAGS) -o $@ 
	
build:
	@mkdir -p build

$(TESTS):
	$(CC) $(CFLAGS) $@.c $(LFLAGS) -o $@ 

# The Unit Tests
#.PHONY: test demo demov
.PHONY: test clean cleandeps
test: CFLAGS += -Itests
test: LFLAGS += -Lbuild -lreplicator -lurnlearning
test: $(TESTS)
	sh ./tests/runtests.sh
	
# The Cleaner
clean:
	rm -rf build dist $(OBJECTS) $(TESTS)
	rm -f tests/tests.log
	find . -name "*.gc*" -exec rm {} \;
	rm -rf `find . -name "*.dSYM" -print`

cleandeps:
	rm -rf deps

# The Checker
BADFUNCS='[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)'
check:
	@echo Files with potentially dangerous functions?
	@egrep $(BADFUNCS) $(SOURCES) || true

