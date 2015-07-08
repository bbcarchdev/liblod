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

EXTRA_DIST = *.xml

if BUILD_MANPAGES

notrans_dist_man3_MANS = liblod.man \
	lod_create.man lod_destroy.man \
	lod_resolve.man lod_locate.man lod_fetch.man \
	lod_world.man lod_storage.man lod_model.man lod_curl.man \
	lod_set_world.man lod_set_storage.man lod_set_model.man \
	lod_set_curl.man lod_error.man lod_errmsg.man \
	lod_instance_destroy.man lod_instance_exists.man \
	lod_instance_stream.man lod_instance_uri.man \
	lod_instance_primarytopic.man
							
SUFFIXES = .3.xml .man
							
.3.xml.man:
	$(XML2MAN) $<
	mv $*.3 $@

endif

dist_liblodhtdoc_DATA = local.css index.html \
	lod_create.3.html lod_destroy.3.html \
	lod_resolve.3.html lod_locate.3.html lod_fetch.3.html \
	lod_world.3.html lod_storage.3.html lod_model.3.html lod_curl.3.html \
	lod_set_world.3.html lod_set_storage.3.html lod_set_model.3.html \
	lod_set_curl.3.html \
	lod_error.3.html lod_errmsg.3.html \
	lod_instance_destroy.3.html lod_instance_exists.3.html \
	lod_instance_stream.3.html lod_instance_uri.3.html \
	lod_instance_primarytopic.3.html

index.html: $(srcdir)/liblod.3.xml $(XSLTFILES) $(NAV) $(LINKS)
	$(XML2HTML) $<

lod_create.3.html: $(srcdir)/lod_create.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_destroy.3.html: $(srcdir)/lod_destroy.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_resolve.3.html: $(srcdir)/lod_resolve.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_fetch.3.html: $(srcdir)/lod_fetch.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_locate.3.html: $(srcdir)/lod_locate.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_world.3.html: $(srcdir)/lod_world.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_storage.3.html: $(srcdir)/lod_storage.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_model.3.html: $(srcdir)/lod_model.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_curl.3.html: $(srcdir)/lod_curl.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_set_world.3.html: $(srcdir)/lod_set_world.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_set_storage.3.html: $(srcdir)/lod_set_storage.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_set_model.3.html: $(srcdir)/lod_set_model.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_set_curl.3.html: $(srcdir)/lod_set_curl.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_error.3.html: $(srcdir)/lod_error.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_errmsg.3.html: $(srcdir)/lod_errmsg.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_instance_destroy.3.html: $(srcdir)/lod_instance_destroy.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_instance_exists.3.html: $(srcdir)/lod_instance_exists.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_instance_stream.3.html: $(srcdir)/lod_instance_stream.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_instance_uri.3.html: $(srcdir)/lod_instance_uri.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<

lod_instance_primarytopic.3.html: $(srcdir)/lod_instance_primarytopic.3.xml $(XSLTFILES) $(NAV) $(LINKS) 
	$(XML2HTML) $<
