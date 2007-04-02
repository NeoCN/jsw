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

COMPILE = cc  -DHPUX -D_XOPEN_SOURCE_EXTENDED -Ae +Z

INCLUDE=$(JAVA_HOME)/include

DEFS = -I$(INCLUDE) -I$(INCLUDE)/hp-ux

wrapper_SOURCE = wrapper.c wrapperinfo.c wrappereventloop.c wrapper_unix.c property.c logger.c
wrapper_objs = wrapper.o wrapperinfo.o wrappereventloop.o wrapper_unix.o property.c logger.o

libwrapper_sl_OBJECTS = wrapperjni_unix.o wrapperinfo.o wrapperjni.o

BIN = ../../bin
LIB = ../../lib

all: init wrapper libwrapper.sl

clean:
	rm -f *.o

cleanall: clean
	rm -rf *~ .deps
	rm -f $(BIN)/wrapper $(LIB)/libwrapper.sl

init:
	if test ! -d .deps; then mkdir .deps; fi

wrapper: $(wrapper_objs)
	$(COMPILE) $(wrapper_objs) -lm -lpthread -o $(BIN)/wrapper

wrapperjni_unix.o: wrapperjni_unix.c
	${COMPILE} -c ${DEFS} $<

wrapperjni.o: wrapperjni.c
	${COMPILE} -c ${DEFS} $<

wrapperinfo.o: wrapperinfo.c
	${COMPILE} -c ${DEFS} $<

%.o: %.c
	${COMPILE} -fPIC -c ${DEFS} $<

libwrapper.sl: $(libwrapper_sl_OBJECTS)
	${COMPILE} $(libwrapper_sl_OBJECTS) -b -o $(LIB)/libwrapper.sl


