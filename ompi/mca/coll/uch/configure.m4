# -*- shell-script -*-
#
#
# Copyright (c) 2011      Mellanox Technologies. All rights reserved.
# Copyright (c) 2015      Research Organization for Information Science
#                         and Technology (RIST). All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#


# MCA_coll_uch_CONFIG([action-if-can-compile],
#                      [action-if-cant-compile])
# ------------------------------------------------
AC_DEFUN([MCA_ompi_coll_uch_CONFIG],[
    AC_CONFIG_FILES([ompi/mca/coll/uch/Makefile])

    OMPI_CHECK_UCH([coll_uch],
                     [coll_uch_happy="yes"],
                     [coll_uch_happy="no"])

    AS_IF([test "$coll_uch_happy" = "yes"],
          [coll_uch_WRAPPER_EXTRA_LDFLAGS="$coll_uch_LDFLAGS"
           coll_uch_CPPFLAGS="$coll_uch_CPPFLAGS"
           coll_uch_WRAPPER_EXTRA_LIBS="$coll_uch_LIBS"
           $1],
          [$2])

    # substitute in the things needed to build uch
    AC_SUBST([coll_uch_CFLAGS])
    AC_SUBST([coll_uch_CPPFLAGS])
    AC_SUBST([coll_uch_LDFLAGS])
    AC_SUBST([coll_uch_LIBS])
])dnl

