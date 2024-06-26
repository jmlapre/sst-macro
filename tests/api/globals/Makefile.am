#
#   This file is part of SST/macroscale: 
#                The macroscale architecture simulator from the SST suite.
#   Copyright (c) 2009-2024, NTESS.
#   This software is distributed under the BSD License.
#   Under the terms of Contract DE-NA0003525 with NTESS,
#   the U.S. Government retains certain rights in this software.
#   For more information, see the LICENSE file in the top 
#   SST/macroscale directory.
#
include $(top_srcdir)/Makefile.common

AM_CPPFLAGS += -I$(top_srcdir)/tests/mpi/include -Wp,-w \
  -I$(srcdir) -I$(top_srcdir)/sumi -I$(top_builddir)/sumi \
  -I$(top_builddir)/tests -I$(top_srcdir)/tests

EXTRA_DIST = \
  config.py 

if INTEGRATED_SST_CORE
lib_LTLIBRARIES = libsstmac_api_globals_test.la
libsstmac_api_globals_test_la_SOURCES = testglobals.cc 
else
EXTRA_DIST += testglobals.cc 
check_PROGRAMS = testexec
testexec_SOURCES = exec.cc
testexec_LDADD = \
  $(top_builddir)/sstmac/main/libsstmac_main.la \
  $(top_builddir)/sstmac/install/libsstmac.la \
  $(top_builddir)/sprockit/sprockit/libsprockit.la


if USE_MPIPARALLEL
testexec_LDADD += $(top_builddir)/sstmac/backends/mpi/libsstmac_mpi_runtime.la
endif

endif

