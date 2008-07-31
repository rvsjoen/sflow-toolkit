CC= g++
#CFLAGS=-ggdb -dv -pg -dr
CFLAGS=-ggdb #-ansi -pedantic -Wall -Wextra #-Werror
#CFLAGS=-O3
LDFLAGS=-lpthread

LIBS=

COLLECTORFLAGS=
EXECFILE=collector

all: collector

clean:
	rm -f *.o

distclean: clean
	rm -f $(EXECFILE)

.PHONY: all clean

file_nametest: filesorter util logger
	$(CC) -o file_nametest file_nametest.c logger.o filesorter.o util.o

collector: sflow_parser filesorter logger util collector.c
	$(CC) -o $(EXECFILE) $(LDFLAGS) $(CFLAGS) collector.c sflow_parser.o filesorter.o logger.o util.o

sflow_parser: logger sflow_parser.c
	$(CC) $(CFLAGS) -c sflow_parser.c

filesorter: logger filesorter.c
	$(CC) $(CFLAGS) -c filesorter.c

logger: logger.c
	$(CC) $(CFLAGS) -c logger.c

util: util.c
	$(CC) $(CFLAGS) -c util.c

sftconvert: util filesorter logger
	$(CC) -o sftconvert $(CFLAGS) sftconvert.c util.o filesorter.o logger.o

fileio_test:
	$(CC) -o fileio_test fileio_test.c

install:
	cp $(EXECFILE) /home/sjoen/work/sflow/collector
	cp sftconvert /home/sjoen/work/sflow/collector
