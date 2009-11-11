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
					 agentlist.c agentlist.h		\
					 configparser.c configparser.h	\
					 bufferqueue.c bufferqueue.h	\
					 messaging.c messaging.h
stcollectd_LDFLAGS = -lpthread -lrrd -lyaml -lrt

stprocessd_SOURCES = stprocessd.c					\
					 logger.c logger.h              \
					 util.c util.h                  \
					 configparser.c configparser.h	\
					 messaging.c messaging.h		\
					 dataparser.c dataparser.h		\
					 storage.c storage.h			\
					 storage_mysql.c storage_mysql.h\
					 storage_rrd.c storage_rrd.h\
					 storage_csv.c storage_csv.h	\
					 storage_dummy.c storage_dummy.h	\
					 agentlist.c agentlist.h		\
					 statistics.c statistics.h
stprocessd_LDFLAGS = -lrt -lmysqlclient
