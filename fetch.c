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
	LODRESULT rr;
	int r, count, followed_link;
	const char *uri, *fragment;
	char *t, *tempuri;
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
	r = 0;
	followed_link = 0;
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
		rr = lod_response_process(context, response);
		switch(rr)
		{
		case LODR_FAIL:
			r = 1;
			break;
		case LODR_COMPLETE:
			r = 0;
			break;
		case LODR_FOLLOW:
		case LODR_FOLLOW_REPLACE:
			free(tempuri);
			tempuri = (char *) malloc(strlen(response->target) + fraglen + 1);
			if(!tempuri)
			{
				lod_set_error_(context, strerror(errno));
				r = 1;
				break;
			}
			strcpy(tempuri, response->target);
			uri = tempuri;
			if(rr != LODR_FOLLOW_REPLACE)
			{
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
			break;
		case LODR_FOLLOW_LINK:
			/* This will only happen once per fetch loop */
			if(followed_link)
			{
				lod_set_error_(context, "a <link rel=\"alternate\"> has previously been followed in this resolution session; will not do so again");
				r = 1;
				break;
			}
			lod_push_subject_(context, response->target);
			/* response->target is now owned by the context */
			uri = response->target;
			response->target = NULL;
			followed_link = 1;
			break;
		}
		if(r || rr == LODR_COMPLETE)
		{
			break;
		}
	}
	if(count == context->max_redirects)
	{
		lod_set_error_(context, "too many redirects encountered");
		r = 1;
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
	if(code >= 300 && code <= 399)
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
