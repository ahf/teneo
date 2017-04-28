AC_PREREQ([2.69])
AC_INIT([passphrase-identity],[1.0.0],[ahf@0x90.dk],[passphrase-identity],[])

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([m4])

AC_LANG([C])

AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
LT_INIT
LT_PATH_LD

PKG_CHECK_MODULES([SODIUM], [libsodium])

GLIB_TESTS

PASSPHRASE_IDENTITY_CFLAGS=

PREFERRED_CFLAGS="-pedantic -Werror -Wall -Wextra -Wbad-function-cast -Wcast-align -Wcast-qual -Wfloat-equal -Wformat=2 -Wformat-security -Wformat-nonliteral -Winit-self -Winline -Wmissing-prototypes -Wmissing-declarations -Wmissing-format-attribute -Wmissing-noreturn -Wpointer-arith -Wredundant-decls -Wshadow -Wswitch-default -Wunused -Wunused-result -fPIE"
LDFLAGS="$LDFLAGS -pie -z relro -z now"

for flag in $PREFERRED_CFLAGS; do
    AX_CHECK_COMPILER_FLAGS([$flag], [PASSPHRASE_IDENTITY_CFLAGS="$PASSPHRASE_IDENTITY_CFLAGS $flag"],)
done

AC_SUBST([PASSPHRASE_IDENTITY_CFLAGS])

AC_CONFIG_FILES([
    Makefile
    src/Makefile
])

AC_OUTPUT
