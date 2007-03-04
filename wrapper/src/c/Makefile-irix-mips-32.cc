# Copyright (c) 1999, 2007 Tanuki Software Inc.
# 
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of the Java Service Wrapper and associated
# documentation files (the "Software"), to deal in the Software
# without  restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sub-license,
# and/or sell copies of the Software, and to permit persons to
# whom the Software is furnished to do so, subject to the
# following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
# NON-INFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

# Makefile for SGI IRIX 6.5 (may work on other versions as well but not tested)
# MIPSpro Compilers: Version 7.3.1.3m
COMPILE = cc -DIRIX -KPIC

INCLUDE=$(JAVA_HOME)/include

DEFS = -I$(INCLUDE) -I$(INCLUDE)/irix 

wrapper_OBJECTS = wrapper.o wrapperinfo.o wrappereventloop.o wrapper_unix.o property.o logger.o

libwrapper_so_OBJECTS = wrapperjni_unix.o wrapperinfo.o wrapperjni.o

BIN = ../../bin
LIB = ../../lib

all: init wrapper libwrapper.so

clean:
	rm -f *.o

cleanall: clean
	rm -rf *~ .deps
	rm -f $(BIN)/wrapper $(LIB)/libwrapper.so

init:
	if test ! -d .deps; then mkdir .deps; fi

wrapper: $(wrapper_OBJECTS)
	$(COMPILE) $(wrapper_OBJECTS) -o $(BIN)/wrapper -lm

libwrapper.so: $(libwrapper_so_OBJECTS)
	${COMPILE} -shared -no_unresolved -n32 -all $(libwrapper_so_OBJECTS) -o $(LIB)/libwrapper.so

%.o: %.c
	@echo '$(COMPILE) -c $<'; \
	$(COMPILE) $(DEFS) -c $<

