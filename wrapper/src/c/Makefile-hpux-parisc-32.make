# Copyright (c) 1999, 2008 Tanuki Software, Inc.
# http://www.tanukisoftware.com
# All rights reserved.
#
# This software is the confidential and proprietary information
# of Tanuki Software.  ("Confidential Information").  You shall
# not disclose such Confidential Information and shall use it
# only in accordance with the terms of the license agreement you
# entered into with Tanuki Software.


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


