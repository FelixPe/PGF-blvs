AC_INIT(src/PGFimage.cpp)

dnl The library interface version
dnl see documentation for versioning in the libtool manual
dnl NOTE: these should only be updated before public releases

dnl The current version of the interface. If interfaces has been 
dnl added removed changed bump this
INTERFACE_CURRENT=6

dnl If the source has been changed bump this
INTERFACE_REVISION=7

dnl If any interfaces has been added since last release, bump this
dnl If any interfaces has been removed, set this to 0
dnl The age is always less than CURRENT
INTERFACE_AGE=0

INTERFACE_VERSION=$INTERFACE_CURRENT:$INTERFACE_REVISION:$INTERFACE_AGE

AC_SUBST(INTERFACE_VERSION)

PACKAGE_RELEASE=2

AC_SUBST(PACKAGE_RELEASE)

AM_INIT_AUTOMAKE(libpgf,6.12.24)
AC_PROG_LIBTOOL
AM_CONFIG_HEADER( config.h )
AC_PROG_CXX

dnl Check for doxygen support
AC_PATH_PROG([DOXYGEN], [doxygen])
AC_PATH_PROG([DOT], [dot])
AM_CONDITIONAL(HAS_DOXYGEN, test $DOXYGEN && $DOT)

# Generate output
AC_OUTPUT(Makefile
    src/Makefile
    include/Makefile
    doc/Makefile
    doc/Doxyfile
    libpgf.spec
    libpgf.pc
)
