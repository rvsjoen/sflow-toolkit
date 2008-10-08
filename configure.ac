# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(sFlow toolkit, 1.0, rune.sjoen@cern.ch)
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
		AC_MSG_NOTICE([You enabled debugging. Good for you.])
		AC_SUBST(CFLAGS, "-ggdb")
	else
		AC_SUBST(CFLAGS, "-O3")
	fi
fi

# Checks for libraries.
AC_CHECK_LIB(yaml,yaml_parser_initialize)
AC_CHECK_LIB(pthread,pthread_create)
AC_CHECK_LIB(rrd,rrd_create)
AC_CHECK_LIB(cmph,cmph_config_new)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h sys/time.h syslog.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_HEADER_TIME
AC_STRUCT_TM
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
