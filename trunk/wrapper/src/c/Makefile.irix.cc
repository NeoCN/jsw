# Makefile for SGI IRIX 6.5 (may work on other versions as well but not tested)
# MIPSpro Compilers: Version 7.3.1.3m
COMPILE = cc -DIRIX -KPIC

INCLUDE=$(JAVA_HOME)/include

DEFS = -I$(INCLUDE) -I$(INCLUDE)/irix 

realpath_OBJECTS = realpath.o

wrapper_OBJECTS = wrapper.o wrapperinfo.o wrapper_unix.o property.o logger.o

libwrapper_so_OBJECTS = wrapperjni_unix.o wrapperinfo.o wrapperjni.o

BIN = ../../bin
LIB = ../../lib

all: init realpath wrapper libwrapper.so

clean:
	rm -f *.o

cleanall: clean
	rm -rf *~ .deps
	rm -f $(BIN)/realpath $(BIN)/wrapper $(LIB)/libwrapper.so

init:
	if test ! -d .deps; then mkdir .deps; fi

realpath: $(realpath_OBJECTS)
	$(COMPILE) $(realpath_OBJECTS) -o $(BIN)/realpath

wrapper: $(wrapper_OBJECTS)
	$(COMPILE) $(wrapper_OBJECTS) -o $(BIN)/wrapper -lm

libwrapper.so: $(libwrapper_so_OBJECTS)
	${COMPILE} -shared -no_unresolved -n32 -all $(libwrapper_so_OBJECTS) -o $(LIB)/libwrapper.so

%.o: %.c
	@echo '$(COMPILE) -c $<'; \
	$(COMPILE) $(DEFS) -c $<

