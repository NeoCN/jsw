# Copyright (c) 1999, 2007 Tanuki Software, Inc.
# http://www.tanukisoftware.com
# All rights reserved.
#
# This software is the confidential and proprietary information
# of Tanuki Software.  ("Confidential Information").  You shall
# not disclose such Confidential Information and shall use it
# only in accordance with the terms of the license agreement you
# entered into with Tanuki Software.

COMPILE = gcc -O3 -Wall --pedantic

INCLUDE=$(JAVA_HOME)/include

DEFS = -I$(INCLUDE) -I$(INCLUDE)/linux

wrapper_SOURCE = wrapper.c wrapperinfo.c wrappereventloop.c wrapper_unix.c property.c logger.c

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

wrapper: $(wrapper_SOURCE)
	$(COMPILE) -lm -pthread $(wrapper_SOURCE) -o $(BIN)/wrapper

libwrapper.so: $(libwrapper_so_OBJECTS)
	${COMPILE} -lm -shared -fPIC $(libwrapper_so_OBJECTS) -o $(LIB)/libwrapper.so

%.o: %.c
	@echo '$(COMPILE) -c $<'; \
	$(COMPILE) $(DEFS) -Wp,-MD,.deps/$(*F).pp -c $<
	@-cp .deps/$(*F).pp .deps/$(*F).P; \
	tr ' ' '\012' < .deps/$(*F).pp \
	| sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	>> .deps/$(*F).P; \
	rm .deps/$(*F).pp
