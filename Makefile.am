## Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
##
## Copyright (c) 2014 BBC
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

XML2HTMLFLAGS = \
	--param "html.linksfile" "'file://$(abs_top_srcdir)/docbook-html5/res-links.xml'" \
    --param "html.navfile" "'file://$(abs_top_srcdir)/docbook-html5/res-nav.xml'" \
    --param "html.ie78css" "'http://bbcarchdev.github.io/painting-by-numbers/ie78.css'"

dist_noinst_DATA = local.css index.html \
	lod_create.3.html lod_destroy.3.html \
	lod_resolve.3.html lod_locate.3.html lod_fetch.3.html \
	lod_error.3.html lod_errmsg.3.html \
	lod_instance_destroy.3.html lod_instance_exists.3.html \
	lod_instance_stream.3.html lod_instance_uri.3.html \
	lod_instance_primarytopic.3.html

index.html: $(top_srcdir)/liblod.3.xml
	$(XML2HTML) $<

lod_create.3.html: $(top_srcdir)/lod_create.3.xml
	$(XML2HTML) $<

lod_destroy.3.html: $(top_srcdir)/lod_destroy.3.xml
	$(XML2HTML) $<

lod_resolve.3.html: $(top_srcdir)/lod_resolve.3.xml
	$(XML2HTML) $<

lod_fetch.3.html: $(top_srcdir)/lod_fetch.3.xml
	$(XML2HTML) $<

lod_locate.3.html: $(top_srcdir)/lod_locate.3.xml
	$(XML2HTML) $<

lod_error.3.html: $(top_srcdir)/lod_error.3.xml
	$(XML2HTML) $<

lod_errmsg.3.html: $(top_srcdir)/lod_errmsg.3.xml
	$(XML2HTML) $<

lod_instance_destroy.3.html: $(top_srcdir)/lod_instance_destroy.3.xml
	$(XML2HTML) $<

lod_instance_exists.3.html: $(top_srcdir)/lod_instance_exists.3.xml
	$(XML2HTML) $<

lod_instance_stream.3.html: $(top_srcdir)/lod_instance_stream.3.xml
	$(XML2HTML) $<

lod_instance_uri.3.html: $(top_srcdir)/lod_instance_uri.3.xml
	$(XML2HTML) $<

lod_instance_primarytopic.3.html: $(top_srcdir)/lod_instance_primarytopic.3.xml
	$(XML2HTML) $<
