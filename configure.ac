# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(sFlow toolkit, 1.3, rune.sjoen@cern.ch)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER(config.h)
AC_ARG_WITH(debugging, [  --with-debugging	  enable debugging symbols, disable optimization])
AC_LANG(C)

# Checks for programs.
AC_PROG_CC

if test "${CFLAGS+set}" = set
then
	if test "$with_debugging" == "yes"
	then
		AC_MSG_NOTICE([Configuring for debugging])
		AC_SUBST(CFLAGS, "-ggdb")
	else
		AC_SUBST(CFLAGS, "-O3")
	fi
fi

# Checks for libraries.
AC_CHECK_LIB(yaml, yaml_parser_initialize, [], [echo "Error: You need to have the yaml library installed"; exit -1 ])
AC_CHECK_LIB(pthread, pthread_create, [], [echo "Error: You need to have the pthread library installed"; exit -1 ])
AC_CHECK_LIB(rrd, rrd_create, [], [echo "Error: You need to have the rrd library installed"; exit -1 ])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([\
		arpa/inet.h errno.h fcntl.h getopt.h mqueue.h netdb.h net/ethernet.h \
		netinet/ether.h netinet/in.h netinet/ip.h netinet/tcp.h netinet/udp.h \
		pthread.h signal.h stdarg.h stdbool.h stdint.h stdio.h stdlib.h string.h \
		syslog.h sys/mman.h sys/resource.h sys/socket.h sys/stat.h sys/time.h \
		sys/types.h termios.h time.h unistd.h util.h yaml.h rrd.h mysql/mysql.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_MODE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Make sure we have the types from stdint
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([getcwd gettimeofday inet_ntoa memset mkdir socket strerror])

AC_CONFIG_FILES([Makefile src/Makefile man/Makefile doc/Makefile scripts/Makefile conf/Makefile])
AC_OUTPUT
