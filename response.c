/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright (c) 2015 BBC
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_liblod.h"

#define BUFSIZE                         512
#define BUFMAX                          (256 * 1024 * 1024)

/* Create a response object for population by a fetch-uri callback */
LODRESPONSE *
lod_response_create(void)
{
	LODRESPONSE *p;

	p = (LODRESPONSE *) calloc(1, sizeof(LODRESPONSE));
	if(!p)
	{
		return NULL;
	}
	return p;
}

/* Reset a response object ready for reuse.
 * Note that this may avoid freeing the internal payload buffer to allow
 * its allocated space to be re-used for the following request.
  */
int
lod_response_reset(LODRESPONSE *resp)
{
	size_t c;

	resp->status = 0;
	free(resp->errmsg);
	resp->errmsg = NULL;
	resp->buflen = 0;
	free(resp->uri);
	resp->uri = NULL;
	free(resp->target);
	resp->target = NULL;
	free(resp->type);
	resp->type = NULL;
	for(c = 0; c < resp->nheaders; c++)
	{
		free(resp->headers[c]);
	}
	free(resp->headers);
	resp->headers = NULL;
	return 0;
}

/* Destroy a response object previously allocated by
 * lod_response_create().
 */
int
lod_response_destroy(LODRESPONSE *resp)
{
	size_t c;

	free(resp->errmsg);
	free(resp->buf);
	free(resp->uri);
	free(resp->target);
	free(resp->type);
	for(c = 0; c < resp->nheaders; c++)
	{
		free(resp->headers[c]);
	}
	free(resp->headers);
	free(resp);
	return 0;
}

/* Set the HTTP status code in a response */
int
lod_response_set_status(LODRESPONSE *resp, long status)
{
	resp->status = status;
	return 0;
}

/* Set the error message in a response */
int
lod_response_set_error(LODRESPONSE *resp, const char *errmsg)
{
	char *p;

	p = (char *) strdup(errmsg);
	if(!p)
	{
		return -1;
	}
	free(resp->errmsg);
	resp->errmsg = p;
	return 0;
}

/* Set the effective URI of a request in a response */
int
lod_response_set_uri(LODRESPONSE *resp, const char *uri)
{
	char *p, *t;

	p = (char *) strdup(uri);
	if(!p)
	{
		lod_response_set_error(resp, "failed to duplicate effective request URI");
		return -1;
	}
	t = strchr(p, '#');
	if(t)
	{
		*t = 0;
	}  
	free(resp->uri);
	resp->uri = p;
	return 0;
}

/* Set the redirect target URI of a request in a response */
int
lod_response_set_target(LODRESPONSE *resp, const char *uri)
{
	char *p;

	p = (char *) strdup(uri);
	if(!p)
	{
		lod_response_set_error(resp, "failed to duplicate target URI");
		return -1;
	}
	free(resp->target);
	resp->target = p;
	return 0;
}

/* Set the MIME type of a payload in a response */
int
lod_response_set_type(LODRESPONSE *resp, const char *type)
{
	char *p;

	p = (char *) strdup(type);
	if(!p)
	{
		lod_response_set_error(resp, "failed to duplicate MIME type");
		return -1;
	}
	free(resp->type);
	resp->type = p;
	return 0;
}

/* Assign the payload of a response
 * NOTE: The payload must be allocated with malloc(), realloc() or calloc()
 * The heap block will be owned by the response and can be freed at any
 * time by liblod once set.
 */
int lod_response_set_payload(LODRESPONSE *resp, char *payload, size_t length);

/* Assign the payload of a response by duplicating a buffer */
int lod_response_set_payload_copy(LODRESPONSE *resp, const char *payload, size_t length);

/* Append a byte sequence to the payload of a response */
int
lod_response_append_payload(LODRESPONSE *resp, const char *bytes, size_t size)
{
	size_t toalloc;
	char *p;

	if(resp->buflen + size >= resp->bufsize)
	{
		toalloc = ((resp->bufsize + size) / BUFSIZE + 1) * BUFSIZE;
		if(toalloc > BUFMAX)
		{
			lod_response_set_error(resp, strerror(ENOMEM));
			return -1;
		}
		p = (char *) realloc(resp->buf, toalloc);
		if(!p)
		{
			lod_response_set_error(resp, strerror(errno));
			return -1;
		}
		resp->bufsize = toalloc;
		resp->buf = p;
	}
	memcpy(&(resp->buf[resp->buflen]), bytes, size);
	resp->buflen += size;
	return 0;
}

/* Reset the payload of a response */
int
lod_response_reset_payload(LODRESPONSE *resp)
{
	resp->buflen = 0;
	return 0;
}

/* Process a response as part of a fetch loop */
LODRESULT
lod_response_process(LODCONTEXT *context, LODRESPONSE *response)
{
	int r;
	char *newuri, *t;
	char errbuf[64];
	librdf_world *world;
	librdf_model *model;
	librdf_uri *baseuri;
	librdf_parser *parser;
	
	context->status = response->status;
	world = lod_world(context);
	if(!world)
	{
		return LODR_FAIL;
	}
	model = lod_model(context);
	if(!model)
	{
		return LODR_FAIL;
	}
	if(!response->uri)
	{
		lod_set_error_(context, "cannot process response because no canonical URL was set");
		return LODR_FAIL;
	}
	if(response->target)
	{
		if(response->status == 303)
		{
			return LODR_FOLLOW_REPLACE;
		}
		return LODR_FOLLOW;
	}
	if(response->status < 200 || response->status > 299)
	{
		/* Some other status code */
		sprintf(errbuf, "HTTP status %ld", response->status);
		lod_set_error_(context, errbuf);
		return LODR_FAIL;		
	}
	if(!response->buf)
	{
		/* XXX empty payload but suitable Link header present should be
		 * acceptable.
		 */
		lod_set_error_(context, "cannot parse an empty payload");
		return LODR_FAIL;
	}
	if(response->type)
	{
		t = strchr(response->type, ';');
		/* XXX determine charset for passing to HTML parser */
		if(t)
		{
			*t = 0;
		}
		if(!strcmp(response->type, "text/html") ||
		   !strcmp(response->type, "application/xhtml+xml") ||
		   !strcmp(response->type, "application/vnd.wap.xhtml+xml") ||
		   !strcmp(response->type, "application/vnd.ctv.xhtml+xml") ||
		   !strcmp(response->type, "application/vnd.hbbtv.xhtml+xml"))
		{
			newuri = NULL;
			r = lod_html_discover_(context, response, response->uri, &newuri);
			if(r < 0)
			{
				lod_set_error_(context, "failed to parse HTML for RDF autodiscovery");
				return LODR_FAIL;
			}
			else if(r == 0 || !newuri)
			{
				lod_set_error_(context, "failed to discover link to RDF representation from HTML document\n");
				return LODR_FAIL;
			}
			free(response->target);
			response->target = newuri;
			return LODR_FOLLOW_LINK;
		}		
	}
	if(!response->type ||
	   !strcmp(response->type, "text/plain") ||
	   !strcmp(response->type, "application/octet-stream") ||
	   !strcmp(response->type, "application/x-unknown"))
	{
		if((r = lod_sniff_(context, response)))
		{
			lod_set_error_(context, "failed to determine serialisation (via content sniffing)");
			return LODR_FAIL;
		}
	}
	if(!response->uri)
	{
		lod_set_error_(context, "no document URI has been set; cannot parse payload\n");
		return LODR_FAIL;
	}
	parser = librdf_new_parser(world, NULL, response->type, NULL);
	if(!parser)
	{
		lod_set_error_(context, "failed to create RDF parser");
		return LODR_FAIL;
	}
	free(context->document);
	context->document = response->uri;
	response->uri = NULL;
	r = 0;
	baseuri = librdf_new_uri(world, (const unsigned char *) context->document);
	if(!baseuri)
	{	
		lod_set_error_(context, "failed to create RDF URI");
		r = 1;
	}
	if(!r)
	{
		r = librdf_parser_parse_counted_string_into_model(parser, (unsigned char *) response->buf, response->buflen, baseuri, model);
		if(context->error)
		{
			r = 1;
		}
	}
	if(baseuri)
	{
		librdf_free_uri(baseuri);
	}
	if(parser)
	{
		librdf_free_parser(parser);
	}
	if(r)
	{
		return LODR_FAIL;
	}
	return LODR_COMPLETE;
}
