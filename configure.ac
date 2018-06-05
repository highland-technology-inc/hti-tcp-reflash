#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([hti-tcp-reflash], [1.0], [pbailey@highlandtechnoly.com])
AM_INIT_AUTOMAKE

LT_PREREQ([2.2])
LT_INIT([dlopen])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([hti-tcp-reflash/reflash.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AM_PROG_CC_C_O

AC_CHECK_HEADERS([unistd.h stdio.h stdlib.h string.h stdint.h \
                  ctype.h sys/socket.h netinet/in.h arpa/inet.h \
                  netdb.h errno.h assert.h], , \
                  [AC_MSG_ERROR([Could not find a necessary header])])

AC_C_INLINE

AC_CHECK_FUNCS([send recv socket gethostbyname setsockopt bind connect], \
               ,[AC_MSG_ERROR([Essential socket header missing])])
AC_CONFIG_FILES([Makefile
                 hti-tcp-reflash/Makefile
                 man/Makefile])

AC_OUTPUT

echo \
"-------------------------------------------------

${PACKAGE_NAME} Version ${PACKAGE_VERSION}

Prefix: '${prefix}'.
Compiler: '${CC} ${CFLAGS} ${CPPFLAGS}'

Now type 'make @<:@<target>@:>@'
    where the optional <target> is:
        all         - build all binaries
        install     - install everything

--------------------------------------------------"
