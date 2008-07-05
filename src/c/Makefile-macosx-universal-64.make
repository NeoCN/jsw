# Copyright (c) 1999, 2008 Tanuki Software, Inc.
# http://www.tanukisoftware.com
# All rights reserved.
#
# This software is the confidential and proprietary information
# of Tanuki Software.  ("Confidential Information").  You shall
# not disclose such Confidential Information and shall use it
# only in accordance with the terms of the license agreement you
# entered into with Tanuki Software.

UNIVERSAL_SDK_HOME=/Developer/SDKs/MacOSX10.5.sdk
COMPILE = gcc -O3 -m64 -Wall -DUSE_NANOSLEEP -DMACOSX -arch ppc64 -arch x86_64 -isysroot $(UNIVERSAL_SDK_HOME) -mmacosx-version-min=10.4

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

