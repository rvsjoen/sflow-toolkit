# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -pedantic -std=c99 -Wextra -Wall -fno-common -ffloat-store -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
bin_PROGRAMS = stcollectd stprocessd
stcollectd_SOURCES = stcollectd.c 					\
					 logger.c logger.h 				\
					 util.c util.h 					\
					 samplestore.c samplestore.h 	\
					 sflowparser.c sflowparser.h 	\
					 sflow.h 						\
					 statistics.c statistics.h		\
					 configparser.c configparser.h	\
					 agentlist.c agentlist.h		\
					 bufferqueue.c bufferqueue.h	\
					 messaging.c messaging.h
stcollectd_LDFLAGS = -lcmph -lpthread -lrrd -lyaml -lrt

stprocessd_SOURCES = stprocessd.c					\
					 logger.c logger.h              \
					 util.c util.h                  \
					 messaging.c messaging.h		\
					 dataparser.c dataparser.h		\
					 storage.c storage.h			\
					 storage_mysql.c storage_mysql.h\
					 configparser.c configparser.h	\
					 statistics.c statistics.h
stprocessd_LDFLAGS = -lrt -lmysqlclient
