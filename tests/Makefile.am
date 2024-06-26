# vim : set ts=4 sw=4 foldmethod=marker:
#   This file is part of SST/macroscale:
#                The macroscale architecture simulator from the SST suite.
#   Copyright (c) 2009-2024, NTESS.
#   This software is distributed under the BSD License.
#   Under the terms of Contract DE-NA0003525 with NTESS,
#   the U.S. Government retains certain rights in this software.
#   For more information, see the LICENSE file in the top
#   SST/macroscale directory.
#

# SINGLETESTS are each run with their own specific rule.  The other classes of tests (CORETESTS, CONFIGTESTS,
# etc.) get prefixes to the test names (test_core_apps_, test_configs_, etc.) which allows each set of test
# to be built/run by generic rules for that set.

include $(top_srcdir)/Makefile.common

AM_CPPFLAGS += \
  -I$(top_builddir)/sst-dumpi \
	-I$(top_srcdir)/sst-dumpi \
  -I$(srcdir) \
	-I$(top_builddir)/tests \
	-I$(top_srcdir)/tests



SUBDIRS = sumi api/mpi api/globals

if USE_REPLACEMENT_HEADERS
SUBDIRS += external
endif

# runtest arguments are:
#   <timeout, in seconds> <output file> <executable> <arguments passed to executable...>
PYRUNTEST = $(top_builddir)/tests/runtest
PYVALIDATE = $(top_builddir)/tests/checktest

NOOP = $(top_builddir)/tests/noop
DIFF=diff -u
if INTEGRATED_SST_CORE
SSTMACEXEC = $(top_builddir)/bin/sstmac-check
else
SSTMACEXEC = $(top_builddir)/bin/sstmac
endif
CHKSUF=chk-out
OUTSUF=ref-out

EXTRA_DIST = \
  reference \
  clang_src2src \
  test_configs \
  test_traces \
  test_blas.cc \
  test_utilities.cc \
  test_pthread.cc \
  sstmac_testutil.h \
  api/parameters.ini \
  runtest \
  checktest \
  checkdiff \
  noop \
  testsuite_default_sst_macro.py


CORE_LIBS = ../sstmac/install/libsstmac.la ../sprockit/sprockit/libsprockit.la
if USE_MPIPARALLEL
CORE_LIBS += ../sstmac/backends/mpi/libsstmac_mpi_runtime.la
endif

CORETESTS=

include Makefile.shared_core_tests
include Makefile.clang_tests
include Makefile.config_tests
include Makefile.api_tests
if !INTEGRATED_SST_CORE
include Makefile.check_programs
include Makefile.single_tests
include Makefile.stats_tests
include Makefile.alloc_tests
include Makefile.external_tests
include Makefile.core_tests
endif
include Makefile.skeleton_tests




#------------------------------------------------------------------------------------------#
#  Collect and check outputs                                                            {{{#
#------------------------------------------------------------------------------------------#

CHECK_TESTS = \
  $(CORETESTS) \
  $(CLANGCHECKTESTS) \
  $(STATSTESTS) \
  $(CONFIGTESTS) \
  $(COVERAGETESTS) \
  $(ALLOCTESTS) \
  $(SINGLETESTS) \
  $(UNITTESTS)

if USE_REPLACEMENT_HEADERS
CHECK_TESTS += $(EXTERNALTESTS)
endif

INSTALLCHECK_TESTS = \
  $(SKELETONTESTS) \
  $(TUTORIALTESTS)

if INTEGRATED_SST_CORE
INSTALLCHECK_TESTS += $(APITESTS)
else
CHECK_TESTS += $(APITESTS)
endif


print-tests:
	echo $(CHECK_TESTS)

check-local: $(CHECK_TESTS:%=%.$(CHKSUF))
	$(PYVALIDATE) $+

installcheck-local: $(INSTALLCHECK_TESTS:%=%.$(CHKSUF))
	$(PYVALIDATE) $+

#------------------------------------------------------------------------------------------#

#------------------------------------------------------------------------------------------#
#  Clean                                                                                {{{#
#------------------------------------------------------------------------------------------#

clean-local: clean-skeletons
	rm -f *.$(CHKSUF)
	rm -f *.tmp-out
	rm -f net.dot
	rm -f callgrind.out
	rm -f tracer_nodemap.txt
	rm -f *.csv
	rm -f nodes_app*.out
	rm -rf traces
	rm -f *.bin *.meta *.map
	rm -f router_study_app_params.ini
	rm -f *temp*.out
	rm -f *.ERROR
	rm -f test.model
	rm -f *.pp.*

clean-skeletons:
	rm -rf skeletons


#------------------------------------------------------------------------------------------#

