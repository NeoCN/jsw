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

UNIVERSAL_SDK_HOME=/Developer/SDKs/MacOSX10.4u.sdk
COMPILE = gcc -O3 -Wall -DUSE_NANOSLEEP -DMACOSX -arch ppc -arch i386 -isysroot $(UNIVERSAL_SDK_HOME)

DEFS = -I$(UNIVERSAL_SDK_HOME)/System/Library/Frameworks/JavaVM.framework/Headers

wrapper_SOURCE = wrapper.c wrapperinfo.c wrappereventloop.c wrapper_unix.c property.c logger.c

libwrapper_so_OBJECTS = wrapperjni_unix.o wrapperinfo.o wrapperjni.o

BIN = ../../bin
LIB = ../../lib

all: init wrapper libwrapper.jnilib

clean:
	rm -f *.o

cleanall: clean
	rm -rf *~ .deps
	rm -f $(BIN)/wrapper $(LIB)/libwrapper.jnilib

init:
	if test ! -d .deps; then mkdir .deps; fi

wrapper: $(wrapper_SOURCE)
	$(COMPILE) -DMACOSX $(wrapper_SOURCE) -o $(BIN)/wrapper

libwrapper.jnilib: $(libwrapper_so_OBJECTS)
	$(COMPILE) -bundle -o $(LIB)/libwrapper.jnilib $(libwrapper_so_OBJECTS)

%.o: %.c
	$(COMPILE) -c $(DEFS) $<

