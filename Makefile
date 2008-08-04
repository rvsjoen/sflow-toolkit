CC			:= gcc
CFLAGS		:= -ggdb -Wall -Wextra --std=c99 -pedantic
LIBS		:= pthread
CMD_RM		:= rm -f

EXEC_COLLECTOR 		:= stcollectd
EXEC_CONVERTER		:= stconvert
EXEC_TEST_FILEIO	:= stfileio

SOURCES 	:= $(wildcard *.c)
OBJS 		:= $(patsubst %.c, %.o, $(SOURCES))

all: $(EXEC_COLLECTOR) $(EXEC_CONVERTER) $(EXEC_TEST_FILEIO)

$(EXEC_COLLECTOR): $(EXEC_COLLECTOR).c filesorter.o util.o logger.o sflowparser.o
	$(CC) $(CFLAGS) $(addprefix -l,$(LIBS))  -o $(EXEC_COLLECTOR) $^

$(EXEC_CONVERTER): $(EXEC_CONVERTER).c filesorter.o logger.o util.o
	$(CC) $(CFLAGS) $(addprefix -l,$(LIBS))  -o $(EXEC_CONVERTER) $^

$(EXEC_TEST_FILEIO): $(EXEC_TEST_FILEIO).c
	$(CC) $(CFLAGS) -o $(EXEC_TEST_FILEIO) $^

clean:
	$(CMD_RM) *.o
	$(CMD_RM) *.d

distclean: clean
	$(CMD_RM) $(EXEC_COLLECTOR)
	$(CMD_RM) $(EXEC_CONVERTER)
	$(CMD_RM) $(EXEC_TEST_FILEIO)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

#include $(patsubst %.c, %.d, $(SOURCES))

.PHONY: all clean
