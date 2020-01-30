# -*- shell-script -*-
#
#
# Copyright (c) 2020 Mellanox Technologies. All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#
#

AM_CPPFLAGS = $(coll_uch_CPPFLAGS)

coll_uch_sources = \
		coll_uch.h \
		coll_uch_debug.h \
		coll_uch_dtypes.h \
		coll_uch_module.c \
		coll_uch_component.c \
		coll_uch_ops.c

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if MCA_BUILD_ompi_coll_uch_DSO
component_noinst =
component_install = mca_coll_uch.la
else
component_noinst = libmca_coll_uch.la
component_install =
endif

mcacomponentdir = $(ompilibdir)
mcacomponent_LTLIBRARIES  = $(component_install)
mca_coll_uch_la_SOURCES = $(coll_uch_sources)
mca_coll_uch_la_LIBADD = $(top_builddir)/ompi/lib@OMPI_LIBMPI_NAME@.la \
	$(coll_uch_LIBS)
mca_coll_uch_la_LDFLAGS = -module -avoid-version $(coll_uch_LDFLAGS)

noinst_LTLIBRARIES           = $(component_noinst)
libmca_coll_uch_la_SOURCES = $(coll_uch_sources)
libmca_coll_uch_la_LIBADD  = $(coll_uch_LIBS)
libmca_coll_uch_la_LDFLAGS = -module -avoid-version $(coll_uch_LDFLAGS)

