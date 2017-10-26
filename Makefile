ifneq ($(SRC),)
VPATH=$(SRC)
endif

CFLAGS += -I. -I$(SRC) -Wall -std=c99 -D_BSD_SOURCE=1 -D_GNU_SOURCE=1

LIBS += -lrt

all: ldr-reader

ldr-reader: ldr-reader.o sysfsgpio.o utils.o
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f *.o ldr-reader
