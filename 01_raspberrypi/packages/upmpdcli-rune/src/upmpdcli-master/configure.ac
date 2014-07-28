AC_INIT([upmpdcli], [0.7.1], [jfd@lesbonscomptes.com],
             [upmpdcli], [http://www.lesbonscomptes.com/upmpdcli])
AC_PREREQ([2.53])
AC_CONFIG_SRCDIR([libupnpp/upnpplib.hxx])
AC_CONFIG_HEADERS([libupnpp/config.h])

AM_INIT_AUTOMAKE([1.10 no-define subdir-objects foreign])
AC_DISABLE_STATIC
LT_INIT

AC_PROG_CXX

# libupnp is configured with large file support, and we need to do the same,
# else a difference in off_t size impacts struct File_Info and prevents the
# vdir to work. This does make a difference, for exemple, for Raspbian
# on the Raspberry PI. Use the same directives as libupnp's configure.ac
AC_TYPE_SIZE_T
AC_TYPE_OFF_T
AC_DEFINE([_LARGE_FILE_SOURCE], [], [Large files support])
AC_DEFINE([_FILE_OFFSET_BITS], [64], [File Offset size])

AC_CHECK_LIB([rt], [clock_gettime], [], [])
AC_CHECK_LIB([pthread], [pthread_create], [], [])
AC_CHECK_LIB([ixml], [ixmlDocument_getElementsByTagName], [], 
		     AC_MSG_ERROR([libixml not found]))
AC_CHECK_LIB([upnp], [UpnpInit], [], AC_MSG_ERROR([libupnp not found]))
AC_CHECK_LIB([expat], [XML_ParserCreate], [], 
		      AC_MSG_ERROR([libexpat not found]))
LIBUPNPP_LIBS="$LIBS"

AC_CHECK_LIB([mpdclient], [mpd_connection_new], [],
                     AC_MSG_ERROR([libmpdclient not found]))

LIBS=""
                     
AC_CHECK_FUNCS([getifaddrs] [UpnpSetLogLevel])

AC_SUBST(LIBUPNPP_LIBS)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
