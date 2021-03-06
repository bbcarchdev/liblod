##  liblod: A Linked Open Data client library
##
##  Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
##
##  Copyright (c) 2014-2016 BBC
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.

AC_INIT([liblod],m4_esyscmd([/bin/sh m4/get-version.sh]),[RESFeedback@bbc.co.uk],,[https://github.com/bbcarchdev/liblod/])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE

AC_CONFIG_HEADER([config.h])

BT_PROG_XCODE
AC_PROG_CC
BT_PROG_CC_WARN

LT_INIT

BT_ENABLE_POSIX
BT_ENABLE_XSI
BT_REQUIRE_PTHREAD
BT_REQUIRE_LIBXML2
BT_REQUIRE_LIBCURL
BT_REQUIRE_LIBRDF

dnl libedit is used only by lod-util, don't include it in $LIBS
BT_REQUIRE_LIBEDIT_INCLUDED([
AM_CPPFLAGS="$AM_CPPFLAGS $LIBEDIT_CPPFLAGS"
AM_LDFLAGS="$AM_LDFLAGS $LIBEDIT_LDFLAGS"
])

BT_REQUIRE_LIBURI_INCLUDED

use_docbook_html5=yes
BT_BUILD_DOCS

liblodhtdocdir='${docdir}'
AC_SUBST([liblodhtdocdir])

# If build_docs is 'no', then XSLT2MAN will be 'true', and this will be a
# no-op at build time, but still install the pre-built manpages.
AM_CONDITIONAL([BUILD_MANPAGES],[true])

BT_DEFINE_PREFIX

AC_CONFIG_FILES([
Makefile
m4/Makefile
docs/Makefile
t/Makefile
liblod.pc
])

AC_OUTPUT
