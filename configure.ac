AC_PREREQ(2.59)
AC_INIT(ternaryplot, 0.1, daniil.ivanov@gmail.com)
AC_CONFIG_SRCDIR([src/main.c])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(DEPS, gtk+-2.0 >= 2.8 glib-2.0 >= 2.8 cairo >= 1.0)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

GLIB_GENMARSHAL=$($PKG_CONFIG --variable=glib_genmarshal glib-2.0)
AC_SUBST(GLIB_GENMARSHAL)

AC_CHECK_LIB([m],[fmod])

AM_INIT_AUTOMAKE

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
