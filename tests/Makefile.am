# This file is part of GDBM.                                   -*- autoconf -*-
# Copyright (C) 2011-2025 Free Software Foundation, Inc.
#
# GDBM is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# GDBM is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GDBM. If not, see <http://www.gnu.org/licenses/>. */

EXTRA_DIST = \
 $(TESTSUITE_AT)\
 testsuite\
 package.m4

DISTCLEANFILES       = atconfig
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE)

## ------------ ##
## package.m4.  ##
## ------------ ##

$(srcdir)/package.m4: $(top_srcdir)/configure.ac $(srcdir)/Makefile.am
	$(AM_V_GEN){                                      \
	  echo '# Signature of the current package.'; \
	  echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])'; \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])'; \
	  echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])'; \
	  echo 'm4_define([AT_PACKAGE_VERSION_MAJOR], [@GDBM_VERSION_MAJOR@])'; \
	  echo 'm4_define([AT_PACKAGE_VERSION_MINOR], [@GDBM_VERSION_MINOR@])'; \
	  echo 'm4_define([AT_PACKAGE_VERSION_PATCH], [@GDBM_VERSION_PATCH@])'; \
	  echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])'; \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	} >$(srcdir)/package.m4

#

## ------------ ##
## Test suite.  ##
## ------------ ##

TESTSUITE_AT = \
 testsuite.at\
 blocksize00.at\
 blocksize01.at\
 blocksize02.at\
 cloexec00.at\
 cloexec01.at\
 cloexec02.at\
 cloexec03.at\
 closerr.at\
 coalesce.at\
 conv.at\
 dbmcreate00.at\
 dbmdel00.at\
 dbmdel01.at\
 dbmdel02.at\
 dbmcvt.at\
 dbmfetch00.at\
 dbmfetch01.at\
 dbmfetch02.at\
 dbmfetch03.at\
 dumpload.at\
 create00.at\
 delete00.at\
 delete01.at\
 delete02.at\
 gdbmtool00.at\
 gdbmtool01.at\
 gdbmtool02.at\
 gdbmtool03.at\
 gdbmtool04.at\
 fetch00.at\
 fetch01.at\
 setopt00.at\
 setopt01.at\
 setopt02.at\
 lockwait_ret.at\
 lockwait_sig.at\
 version.at\
 wordwrap.at

TESTSUITE = $(srcdir)/testsuite
M4=m4

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 $(TESTSUITE_AT)
	$(AUTOTEST) -I $(srcdir) testsuite.at -o $@.tmp
	mv $@.tmp $@

atconfig: $(top_builddir)/config.status
	cd $(top_builddir) && ./config.status tests/$@

clean-local:
	test ! -f $(TESTSUITE) || $(SHELL) $(TESTSUITE) --clean

check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) $(TESTSUITE)

# Run the test suite on the *installed* tree.
#installcheck-local:
#	$(SHELL) $(TESTSUITE) AUTOTEST_PATH=$(exec_prefix)/bin

## --------------- ##
## num2word et al. ##
## --------------- ##

if COMPAT_OPT
 DBMINCLUDES = -I$(top_srcdir)/compat
 DBMPROGS = dtload dtdump dtfetch dtdel d_creat_ce
endif 

check_PROGRAMS = \
 closerr\
 fdop\
 g_open_ce\
 g_reorg_ce\
 gtcacheopt\
 gtconv\
 gtdel\
 gtdump\
 gtfetch\
 gtload\
 gtopt\
 gtrecover\
 gtver\
 num2word\
 t_dumpload\
 t_lockwait\
 t_wordwrap\
 $(DBMPROGS)

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(top_srcdir)/tools $(DBMINCLUDES)

noinst_HEADERS=progname.h

LDADD = ../src/libgdbm.la

dtload_LDADD = ../src/libgdbm.la ../compat/libgdbm_compat.la
dtdump_LDADD = ../src/libgdbm.la ../compat/libgdbm_compat.la
dtfetch_LDADD = ../src/libgdbm.la ../compat/libgdbm_compat.la
dtdel_LDADD = ../src/libgdbm.la ../compat/libgdbm_compat.la
d_creat_ce_LDADD = ../src/libgdbm.la ../compat/libgdbm_compat.la
t_wordwrap_LDADD = ../tools/libgdbmapp.a @LTLIBINTL@

SUBDIRS = dejagnu
