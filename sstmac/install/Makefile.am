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

# Don't ever change this or Ubuntu linker be unkind
if INTEGRATED_SST_CORE
noinst_LTLIBRARIES = libsstmac.la
else
lib_LTLIBRARIES = libsstmac.la
libsstmac_la_LDFLAGS = -version-info @SSTMAC_LIBVERSION@
endif

libsstmac_la_LIBADD = 

libsstmac_la_SOURCES =
nodist_EXTRA_libsstmac_la_SOURCES = dummy.cxx
libsstmac_la_LIBADD += \
  ../../sprockit/sprockit/libsprockit.la \
  ../../sumi/libsumi.la \
  ../backends/common/libsstmac_backends.la \
  ../backends/native/libsstmac_native.la \
  ../common/libsstmac_common.la \
  ../dumpi_util/libsstmac_dumpi.la \
  ../hardware/libsstmac_hw.la \
  ../libraries/machines/libsstmac_machines.la \
  ../libraries/pthread/libsstmac_pthread.la \
  ../libraries/omp/libsstmac_omp.la \
  ../main/libsstmac_driver.la \
  ../skeletons/libsstmac_skeletons.la \
  ../test_skeletons/libsstmac_test_skeletons.la \
  ../software/libsstmac_sw.la \
  ../../sumi-mpi/libsstmac_mpi.la

#if HAVE_CLANG
#libsstmac_la_LIBADD += ../llvm/libsstmac_inst.la
#endif

#if HAVE_OTF2
#libsstmac_la_LIBADD += $(OTF2_LDFLAGS) $(OTF2_LIBS)
#libsstmac_la_LIBADD += ../../sst-dumpi/dumpi/libotf2dump/libotf2dump.la
#endif

if USE_MPIPARALLEL
libsstmac_la_LIBADD += ../backends/mpi/libsstmac_mpi_runtime.la 
endif

SUBDIRS = $(STD_SUBDIRS) 
DIST_SUBDIRS = $(STD_SUBDIRS) 





if INTEGRATED_SST_CORE
lib_LTLIBRARIES = libmacro.la
libsstmac_la_LIBADD += ../sst_core/libsstmac_sst_core.la
libmacro_la_SOURCES =
nodist_EXTRA_libmacro_la_SOURCES = dummy.cxx

libmacro_la_LIBADD = \
  libsstmac.la

libmacro_la_LDFLAGS = -module -shared -version-info @SSTMAC_LIBVERSION@

SST_REGISTER_TOOL=@sst_prefix@/bin/sst-register

install-exec-hook:
	  $(SST_REGISTER_TOOL) SST_MACRO SST_MACRO_LIBDIR=@libdir@
	  $(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE sst-macro=$(abs_srcdir)/../..
	  $(SST_REGISTER_TOOL) SST_ELEMENT_TESTS sst-macro=$(abs_srcdir)/../../tests

endif

