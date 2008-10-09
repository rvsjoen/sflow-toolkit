# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -pedantic -std=c99 -Wextra -Wall -fno-common -ffloat-store
bin_PROGRAMS = stcollectd
stcollectd_SOURCES = stcollectd.c 					\
					 logger.c logger.h 				\
					 util.c util.h 					\
					 filesorter.c filesorter.h 		\
					 sflowparser.c sflowparser.h 	\
					 sflow.h 						\
					 statistics.c statistics.h		\
					 configparser.c configparser.h
stcollectd_LDFLAGS = -lcmph -lpthread -lrrd -lyaml
