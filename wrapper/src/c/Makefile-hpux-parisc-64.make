JNI_HEADERS = $(JAVA_HOME)/include
DEFS = -I$(JNI_HEADERS) -I$(JNI_HEADERS)/hp-ux
 
DEFVALS = -O3 -Wall -DHPUX -D_HPUX -D_POSIX_C_SOURCE=199506L -D_XOPEN_SOURCE_EXTENDED
COMPILE = cc +z $(DEFVALS) $(DEFS)
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
	${COMPILE} $(WRAPPER_OBJECTS) -lm -o $(BIN)/wrapper

libwrapper.sl:$(LIBWRAPPER_SL_OBJECTS)
	${LINK} $(LIBWRAPPER_SL_OBJECTS) -b -o $(LIB)/libwrapper.sl

