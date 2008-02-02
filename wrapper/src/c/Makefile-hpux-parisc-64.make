# Copyright (c) 1999, 2008 Tanuki Software, Inc.
# http://www.tanukisoftware.com
# All rights reserved.
#
# This software is the confidential and proprietary information
# of Tanuki Software.  ("Confidential Information").  You shall
# not disclose such Confidential Information and shall use it
# only in accordance with the terms of the license agreement you
# entered into with Tanuki Software.


JNI_HEADERS = $(JAVA_HOME)/include
DEFS = -I$(JNI_HEADERS) -I$(JNI_HEADERS)/hp-ux
 
DEFVALS = -O3 -Wall -DHPUX
COMPILE = cc +z +DD64 $(DEFVALS) $(DEFS)
LINK = ld

SOURCE = wrapper.c wrapperinfo.c wrappereventloop.c wrapper_unix.c property.c logger.c wrapperjni_unix.c wrapperjni.c
WRAPPER_OBJECTS = wrapper.o wrapperinfo.o wrappereventloop.o wrapper_unix.o property.o logger.o
LIBWRAPPER_SL_OBJECTS = wrapperjni_unix.o wrapperjni.o wrapperinfo.o 

BIN = ../../bin
LIB = ../../lib
 
all: cleanall init compile wrapper libwrapper.sl

clean:
	rm -f *.o

cleanall:clean
	rm -rf *~ .deps
	rm -f $(BIN)/wrapper $(LIB)/libwrapper.sl
	
init:
	if test ! -d .deps; then mkdir .deps; fi
 
compile:$(SOURCE)
	${COMPILE} -c $(SOURCE)

wrapper:$(WRAPPER_OBJECTS)
	${COMPILE} $(WRAPPER_OBJECTS) -lpthread -lc -lm -o $(BIN)/wrapper

libwrapper.sl:$(LIBWRAPPER_SL_OBJECTS)
	${LINK} $(LIBWRAPPER_SL_OBJECTS) -b -o $(LIB)/libwrapper.sl

