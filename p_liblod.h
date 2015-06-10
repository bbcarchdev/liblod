/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright (c) 2014 BBC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef P_LIBLOD_H_
# define P_LIBLOD_H_                    1

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <ctype.h>

# include <librdf.h>
# include <curl/curl.h>
# include <libxml/HTMLparser.h>
# include <libxml/xpath.h>
# include <libxml/xpathInternals.h>
# include <liburi.h>

# include "liblod.h"

struct lod_context_struct
{
	librdf_world *world;
	librdf_storage *storage;
	librdf_model *model;
	CURL *ch;
	struct curl_slist *headers;
	char *subject;
	char *document;
	long status;
	int error;
	char *errmsg;
	int max_redirects;
	char **subjects;	
	int nsubjects;
	char *accept;
	LODFETCHURI fetch_uri;
	int verbose:1;
	int world_alloc:1;
	int storage_alloc:1;
	int model_alloc:1;
	int ch_alloc:1;
};

struct lod_instance_struct
{
	LODCONTEXT *context;
	librdf_statement *query;
	librdf_node *subject;
};

struct lod_response_struct
{
	/* HTTP status, or zero for a low-level error */
	long status;
	/* Error state, if any */
	char *errmsg;
	/* Payload */
	char *buf;
	size_t bufsize;
	size_t buflen;
	/* The 'effective URI' */
	char *uri;
	/* The redirect target URI */
	char *target;
	/* The payload MIME type */
	char *type;
	/* Response headers, in RFC822/HTTP format */
	char **headers;
	size_t nheaders;
};

int lod_reset_(LODCONTEXT *context);
int lod_set_error_(LODCONTEXT *context, const char *msg);
int lod_fetch_(LODCONTEXT *context);
int lod_html_discover_(LODCONTEXT *context, LODRESPONSE *response, const char *url, char **newurl);
int lod_push_subject_(LODCONTEXT *context, char *uri);
int lod_sniff_(LODCONTEXT *context, LODRESPONSE *response);

LODINSTANCE *lod_instance_create_(LODCONTEXT *context, librdf_statement *query, librdf_node *subject);

#endif /*!P_LIBLOD_H_*/
