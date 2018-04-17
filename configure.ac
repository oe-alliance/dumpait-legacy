#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#AC_PREREQ([2.69])
AC_INIT([dumpait], [2.2.0], [kos@dev3])
AM_INIT_AUTOMAKE([dumpait], [2.2.0])

#AC_CONFIG_SRCDIR([main.cpp])
AC_CONFIG_HEADERS([config.h])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

AC_CONFIG_MACRO_DIR([m4])

PKG_CHECK_MODULES(BASE, [libdvbsi++])

# Checks for libraries.
AC_SUBST(PLATFORM)

# Checks for header files.
AC_SUBST(INCPATH)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

