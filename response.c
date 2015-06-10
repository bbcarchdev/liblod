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
	free(resp->uri);
	resp->uri = p;
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
