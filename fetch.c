/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright (c) 2014-2015 BBC
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

static int lod_fetch_curl_(LODCONTEXT *context, const char *uri, LODRESPONSE *response);
static size_t lod_fetch_write_(char *ptr, size_t size, size_t nmemb, void *userdata);

/* Unconditionally fetch some LOD and parse it into the existing model */
int
lod_fetch_(LODCONTEXT *context)
{
	LODRESPONSE *response;
	librdf_world *world;
	librdf_model *model;
	librdf_uri *baseuri;
	librdf_parser *parser;
	CURLcode e;
	int r, count, followed_link;
	const char *uri, *fragment;
	char *t, *newuri, *tempuri, errbuf[64];
	size_t fraglen;

	tempuri = NULL;
	if(lod_push_subject_(context, context->subject))
	{
		return -1;
	}
	/* Save the fragment in case we need to apply it to a
	 * a redirect URI
	 */
	if((fragment = strchr(context->subject, '#')))
	{
		fraglen = strlen(fragment);
	}
	else
	{
		fraglen = 0;
	}
	parser = NULL;
	baseuri = NULL;
	r = 0;
	followed_link = 0;
	world = lod_world(context);
	if(!world)
	{
		return -1;
	}
	model = lod_model(context);
	if(!model)
	{
		return -1;
	}
	response = lod_response_create();
	if(!response)
	{
		lod_set_error_(context, "failed to create response object");
		return -1;
	}
	uri = context->subjects[0];
	for(count = 0; count < context->max_redirects; count++)
	{
		lod_response_reset(response);
		r = lod_fetch_curl_(context, uri, response);
		if(r || response->status <= 0 || response->errmsg)
		{
			if(response->errmsg)
			{
				lod_set_error_(context, response->errmsg);
			}
			else
			{
				lod_set_error_(context, "an unknown error occurred while fetching the resource");
			}
			r = 1;
			break;
		}
		context->status = response->status;
		if(response->target)
		{
			/* Follow a redirect */
			free(tempuri);
			tempuri = NULL;
			newuri = response->target;
			if((e = curl_easy_getinfo(context->ch, CURLINFO_REDIRECT_URL, &newuri)))
			{
				lod_set_error_(context, curl_easy_strerror(e));
				r = 1;
				break;
			}
			tempuri = (char *) malloc(strlen(newuri) + fraglen + 1);
			if(!tempuri)
			{
				lod_set_error_(context, strerror(errno));
				r = 1;
				break;
			}
			strcpy(tempuri, newuri);
			uri = tempuri;
			if(response->status == 303)
			{
				/* In the case of a 303, the new URI is simply used
				 * in the subsequent request -- we shouldn't ever push
				 * it onto the potential subject list
				 */
				continue;
			}
			if(fragment)
			{
				t = strchr(tempuri, '#');
				if(t)
				{
					strcpy(t, fragment);
				}
				else
				{
					strcat(tempuri, fragment);
				}
			}
			lod_push_subject_(context, tempuri);
			/* tempuri is now owned by context */
			tempuri = NULL;
			continue;
		}
		if(response->status >= 200 && response->status <= 299 && response->buf)
		{			
			if(!response->type)
			{
				lod_set_error_(context, "failed to process payload because no content type is available");
				r = 1;
				break;
			}
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
				/* Perform link autodiscovery if we haven't previously done
				 * so.
				 */
				if(followed_link)
				{
					lod_set_error_(context, "a <link rel=\"alternate\"> has previously been followed in this resolution session; will not do so again");
					r = 1;
					break;
				}
				r = lod_html_discover_(context, response, uri, &newuri);
				if(r < 0)
				{
					lod_set_error_(context, "failed to parse HTML for RDF autodiscovery");
					break;
				}
				else if(r == 0)
				{
					/* Autodiscovery failed */
					lod_set_error_(context, "failed to discover link to RDF representation from HTML document\n");
					r = 1;
					break;
				}
				r = 0;
				/* tempuri is now owned by context */
				lod_push_subject_(context, newuri);
				uri = newuri;
				followed_link = 1;
				continue;
			}
			break;			
		}
		/* Some other status code */
		r = 1;
		sprintf(errbuf, "HTTP status %ld", response->status);
		lod_set_error_(context, errbuf);
		break;
	}
	if(count == context->max_redirects)
	{
		lod_set_error_(context, "too many redirects encountered");
		r = 1;
	}
	if(!r)
	{
		if(!response->type ||
		   !strcmp(response->type, "text/plain") ||
		   !strcmp(response->type, "application/octet-stream") ||
		   !strcmp(response->type, "application/x-unknown"))
		{
			if((r = lod_sniff_(context, response)))
			{
				lod_set_error_(context, "failed to determine serialisation");
			}
		}
	}
	if(!r)
	{
		parser = librdf_new_parser(world, NULL, response->type, NULL);
		if(!parser)
		{
			lod_set_error_(context, "failed to create RDF parser");
			r = 1;
		}
	}
	if(!r)
	{
		if(!response->uri)
		{
			lod_set_error_(context, "no document URI has been set; cannot parse payload\n");
			r = 1;
		}
	}
	if(!r)
	{
		free(context->document);
		context->document = response->uri;
		response->uri = NULL;
		baseuri = librdf_new_uri(world, (const unsigned char *) context->document);
		if(!baseuri)
		{
			lod_set_error_(context, "failed to create RDF URI");
			r = 1;
		}
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
	free(tempuri);
	lod_response_destroy(response);
	if(r)
	{
		context->error = 1;
		return -1;
	}
	return 0;
}

/* The default implementation of a LODFETCHURI callback using cURL */
static int
lod_fetch_curl_(LODCONTEXT *context, const char *uri, LODRESPONSE *response)
{
	CURL *ch;
	CURLcode e;
	long code;
	char *str;

	ch = lod_curl(context);
	if(!ch)
	{
		return -1;
	}
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *) response);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, lod_fetch_write_);
	curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 0);
	curl_easy_setopt(ch, CURLOPT_URL, uri);
	e = curl_easy_perform(ch);
	if(e)
	{
		lod_response_set_error(response, curl_easy_strerror(e));
		return -1;
	}
	if((e = curl_easy_getinfo(context->ch, CURLINFO_RESPONSE_CODE, &code)))
	{
		lod_response_set_error(response, curl_easy_strerror(e));
		return -1;
	}
	if(lod_response_set_status(response, code))
	{
		return -1;
	}
	if((e = curl_easy_getinfo(ch, CURLINFO_EFFECTIVE_URL, &str)))
	{
		lod_response_set_error(response, curl_easy_strerror(e));
		return -1;
	}
	if(lod_response_set_uri(response, str))
	{
		return -1;
	}
	if((e = curl_easy_getinfo(ch, CURLINFO_CONTENT_TYPE, &str)))
	{
		lod_response_set_error(response, curl_easy_strerror(e));
		return -1;
	}
	if(lod_response_set_type(response, str))
	{
		return -1;
	}
	if(code > 300 && code <= 399)
	{
		if((e = curl_easy_getinfo(ch, CURLINFO_REDIRECT_URL, &str)))
		{
			lod_response_set_error(response, curl_easy_strerror(e));
			return -1;
		}
		if(lod_response_set_target(response, str))
		{
			return -1;
		}
	}
	return 0;
}

/* Invoked by libcurl when payload data is received */
static size_t
lod_fetch_write_(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	LODRESPONSE *response;

	response = (LODRESPONSE *) userdata;
	size *= nmemb;
	if(lod_response_append_payload(response, ptr, size))
	{
		return 0;
	}
	return size;
}
