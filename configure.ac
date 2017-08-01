#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(xcur2png, 0.7.1, tksmashiw@gmail.com)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([xcur2png.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(XCUR2PNG, libpng >= 1.0.6 xcursor)
AC_SUBST(XCUR2PNG_CFLAGS)
AC_SUBST(XCUR2PNG_LIBS)

# Checks for header files.
AC_CHECK_HEADER([png.h],,AC_MSG_ERROR([png.h not found]))
AC_CHECK_HEADER([X11/Xcursor/Xcursor.h],,AC_MSG_ERROR([Xcursor.h not found]))


# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strrchr])

AC_CONFIG_FILES([Makefile debian/Makefile])
AC_OUTPUT
