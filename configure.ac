# hsetroot - an imlib2 powered wallpaper application
# Copyright (C) 2003 Hyriand - See COPYING
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Process this file with autoconf to produce a configure script.
# 

AC_PREREQ(2.52)
AC_INIT(hsetroot, 1.0.2, hyriand@thegraveyard.org)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(hsetroot, 1.0.2)
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC

# default set of CFLAGS (override this with ./configure CFLAGS=foo)
CFLAGS=${CFLAGS:=-O2 -g -Wall}

# Checks for header files.
AC_PATH_X
AC_HEADER_STDC
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Check for imlib2
AC_CHECK_PROGS(imlib2config_cmd, imlib2-config)
if test x$imlib2config_cmd = "x"; then
  AC_MSG_ERROR([error. Imlib2 is required to compile.])
fi

IMLIB2_CFLAGS=`$imlib2config_cmd --cflags`
AC_SUBST(IMLIB2_CFLAGS)

IMLIB2_LIBS=`$imlib2config_cmd --libs`
AC_SUBST(IMLIB2_LIBS)

# Some extra definitions for config.h
AC_DEFINE(DESCRIPTION, "yet another wallpaper application", [single line package description])

AM_CONFIG_HEADER(config/config.h)
AC_CONFIG_FILES([Makefile src/Makefile config/Makefile])

AC_OUTPUT
