## Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
##
## Copyright (c) 2014-2015 BBC
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

ACLOCAL_AMFLAGS = -I m4

LIBS =

DIST_SUBDIRS = m4 docs t

SUBDIRS = @subdirs@ docs t

EXTRA_DIST = LICENSE-2.0 README.md *.xml liblod.pc.in

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = liblod.pc

include_HEADERS = liblod.h

lib_LTLIBRARIES = liblod.la

bin_PROGRAMS = lod-util

noinst_PROGRAMS = example-1

liblod_la_SOURCES = p_liblod.h \
	context.c instance.c resolve.c fetch.c sniff.c html.c response.c

liblod_la_LIBADD = @LIBCURL_LOCAL_LIBS@ @LIBCURL_LIBS@ \
	@LIBXML2_LOCAL_LIBS@ @LIBXML2_LIBS@ \
	@LIBRDF_LOCAL_LIBS@ @LIBRDF_LIBS@ \
	@LIBURI_LOCAL_LIBS@ @LIBURI_LIBS@

liblod_la_LDFLAGS = -avoid-version

lod_util_SOURCES = lod-util.c

lod_util_LDADD = liblod.la \
	@LIBRDF_LOCAL_LIBS@ @LIBRDF_LIBS@ \
	@LIBCURL_LOCAL_LIBS@ @LIBCURL_LIBS@

example_1_SOURCES = example-1.c

example_1_LDADD = liblod.la \
	@LIBRDF_LOCAL_LIBS@ @LIBRDF_LIBS@

BRANCH ?= develop
DOCS_BRANCH ?= gh-pages

DEVELOP_SUBMODULES = m4
DOCS_SUBMODULES = docs docbook-html5

checkout:
	@for submod in $(DEVELOP_SUBMODULES) ; do \
		echo "+++ Checking out the latest version of $$submod on $(BRANCH)" >&2 ; \
		cd $$submod && \
		git checkout $(BRANCH) && \
		git pull && \
		$(MAKE) checkout BRANCH=$(BRANCH) || exit $? && \
		cd .. ; \
	done
	@for submod in $(DOCS_SUBMODULES) ; do \
		echo "+++ Checking out the latest version of $$submod on $(DOCS_BRANCH)" >&2 ; \
		cd $$submod && \
		git checkout $(DOCS_BRANCH) && \
		git pull && \
		cd .. ; \
	done
