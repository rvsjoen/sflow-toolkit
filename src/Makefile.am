# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -pedantic -std=c99 -Wextra -Wall -fno-common -ffloat-store
bin_PROGRAMS = stcollectd stconvert
stcollectd_SOURCES = stcollectd.c logger.c logger.h util.c util.h filesorter.c filesorter.h sflowparser.c sflowparser.h
stcollectd_LDFLAGS = -lcmph -lpthread
stconvert_SOURCES = stconvert.c logger.c logger.h filesorter.c filesorter.h

