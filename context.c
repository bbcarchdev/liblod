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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_liblod.h"

#define MAX_REDIRECTS                   32

/* Create a new LOD context */
LODCONTEXT *
lod_create(void)
{
	LODCONTEXT *p;

	p = (LODCONTEXT *) calloc(1, sizeof(LODCONTEXT));
	if(!p)
	{
		return NULL;
	}
	p->max_redirects = MAX_REDIRECTS;
	return p;
}

/* Free a LOD context */
int
lod_destroy(LODCONTEXT *context)
{
	lod_reset_(context);
	if(context->model && context->model_alloc)
	{
		librdf_free_model(context->model);
	}
	if(context->storage && context->storage_alloc)
	{
		librdf_free_storage(context->storage);
	}
	if(context->world && context->world_alloc)
	{
		librdf_free_world(context->world);
	}
	if(context->headers)
	{
		curl_slist_free_all(context->headers);
	}
	if(context->ch && context->ch_alloc)
	{
		curl_easy_cleanup(context->ch);
	}
	free(context->accept);
	free(context);
	return 0;
}

/* Obtain the librdf world used by the context */
librdf_world *
lod_world(LODCONTEXT *context)
{
	if(context->world)
	{
		return context->world;
	}
	context->world_alloc = 1;
	context->world = librdf_new_world();
	if(!context->world)
	{
		lod_set_error_(context, "failed to construct new librdf_world");
		return NULL;
	}
	librdf_world_open(context->world);
	librdf_world_set_logger(context->world, (void *) context, lod_librdf_logger);
	return context->world;
}

/* Set the librdf world which will be used by the context */
int
lod_set_world(LODCONTEXT *context, librdf_world *world)
{	
	if(context->model && context->model_alloc)
	{
		librdf_free_model(context->model);
	}
	context->model = NULL;
	if(context->storage && context->storage_alloc)
	{
		librdf_free_storage(context->storage);
	}
	context->storage = NULL;
	if(context->world && context->world_alloc)
	{
		librdf_free_world(context->world);
	}
	context->world = world;
	context->world_alloc = 0;
	return 0;
}

/* Obtain the librdf storage used by the context */
librdf_storage *
lod_storage(LODCONTEXT *context)
{
	librdf_world *world;
	
	if(context->storage)
	{
		return context->storage;
	}
	context->storage_alloc = 1;
	world = lod_world(context);
	if(!world)
	{
		return NULL;
	}
	context->storage = librdf_new_storage(world, "hashes", NULL, "hash-type='memory',contexts='yes'");
	if(!context->storage)
	{
		return NULL;
	}
	return context->storage;
}

/* Set the librdf storage which will be used by the context (must be created
 * using the context's librdf world). Setting a new librdf storage will cause
 * any existing model associated with the context to be freed if it was
 * automatically allocated, and discarded otherwise.
 */
int
lod_set_storage(LODCONTEXT *context, librdf_storage *storage)
{
	if(context->model && context->model_alloc)
	{
		librdf_free_model(context->model);
	}
	context->model = NULL;
	if(context->storage && context->storage_alloc)
	{
		librdf_free_storage(context->storage);
	}
	context->storage = storage;
	return 0;
}

/* Obtain the librdf model used by the context */
librdf_model *
lod_model(LODCONTEXT *context)
{
	librdf_world *world;
	librdf_storage *storage;

	if(context->model)
	{
		return context->model;
	}
	context->model_alloc = 1;
	world = lod_world(context);
	if(!world)
	{
		return NULL;
	}
	storage = lod_storage(context);
	if(!storage)
	{
		return NULL;
	}
	context->model = librdf_new_model(world, storage, NULL);
	if(!context->model)
	{
		return NULL;
	}
	return context->model;
}

/* Set the librdf model which will be used by the context (must be created
 * using the context's librdf world)
 */
int
lod_set_model(LODCONTEXT *context, librdf_model *model)
{
	if(context->model && context->model_alloc)
	{
		librdf_free_model(context->model);
	}
	context->model = model;
	context->model_alloc = 0;
	return 0;
}

/* Obtain a cURL handle for a context */
CURL *
lod_curl(LODCONTEXT *context)
{
	const char *ua, *accept;

	if(context->ch)
	{
		return context->ch;
	}
	ua = lod_useragent(context);
	if(!ua)
	{
		lod_set_error_(context, "failed to obtain User-Agent for context");
		return NULL;
	}
	accept = lod_accept(context);
	if(!accept)
	{
		lod_set_error_(context, "failed to obtain Accept header for context");
		return NULL;
	}	
	context->ch_alloc = 1;
	context->ch = curl_easy_init();
	if(!context->ch)
	{
		lod_set_error_(context, "failed to create new cURL handle");
		return NULL;
	}
	context->headers = curl_slist_append(NULL, accept);
	context->headers = curl_slist_append(context->headers, ua);
	curl_easy_setopt(context->ch, CURLOPT_HTTPHEADER, context->headers);
	curl_easy_setopt(context->ch, CURLOPT_VERBOSE, context->verbose);
	return context->ch;
}

/* Set the cURL handle which will be used for future fetches by the context */
int
lod_set_curl(LODCONTEXT *context, CURL *ch)
{
	if(context->headers)
	{
		curl_slist_free_all(context->headers);
	}
	context->headers = NULL;
	if(context->ch && context->ch_alloc)
	{
		curl_easy_cleanup(context->ch);
	}
	context->ch_alloc = 0;
	context->ch = ch;
	return 0;
}

/* Return the subject URI (after following any relevant redirects) that was
 * most recently resolved, if any.
 */
const char *
lod_subject(LODCONTEXT *context)
{
	return context->subject;
}

/* Return the document URL (after following any relevant redirects) that was
 * most recently fetched from (if any).
 */
const char *
lod_document(LODCONTEXT *context)
{
	return context->document;
}

/* Return the HTTP status code from the most recent resolution request; a
 * return value of zero means no fetch was performed.
 */
long
lod_status(LODCONTEXT *context)
{
	return context->status;
}

/* Return the context error state from the most recent resolution request;
 * a value of zero means no error has occurred.
 */
int
lod_error(LODCONTEXT *context)
{
	return context->error;
}

/* Return the error message from the most recent resolution request */
const char *
lod_errmsg(LODCONTEXT *context)
{
	if(!context->error)
	{
		return NULL;
	}
	if(!context->errmsg)
	{
		return "Unknown error";
	}
	return context->errmsg;
}

/* Logging function which is set on librdf_world objects via
 * librdf_world_set_logger(); note that the context must be
 * specified as the "user_data" parameter.
 */
int
lod_librdf_logger(void *userdata, librdf_log_message *message)
{
	LODCONTEXT *context;
	librdf_log_level level;

	context = (LODCONTEXT *) userdata;
	level = librdf_log_message_level(message);
	if(level >= LIBRDF_LOG_ERROR)
	{
		lod_set_error_(context, librdf_log_message_message(message));
	}
	return 1;
}

/* Return the default User-agent header */
const char *
lod_useragent(LODCONTEXT *context)
{
	(void) context;

	return "User-Agent: liblod/1 (+https://github.com/bbcarchdev/liblod)";
}

/* Return the default Accept header */
const char *
lod_accept(LODCONTEXT *context)
{
	librdf_world *world;
	size_t nbytes;
	unsigned int c, i;
	const raptor_syntax_description *desc;
	char *p;

	if(context->accept)
	{
		return context->accept;
	}
	world = lod_world(context);
	if(!world)
	{
		return NULL;
	}
	nbytes = 32;  
	for(c = 0; (desc = librdf_parser_get_description(world, c)); c++)
	{
		for(i = 0; i < desc->mime_types_count; i++)
		{
			nbytes += strlen(desc->mime_types[i].mime_type) + 10;
		}
	}
	context->accept = (char *) calloc(1, nbytes);
	if(!context->accept)
	{
		lod_set_error_(context, strerror(errno));
		return NULL;
	}
	strcpy(context->accept, "Accept: ");
	p = strchr(context->accept, 0);
	for(c = 0; (desc = librdf_parser_get_description(world, c)); c++)
	{
		for(i = 0; i < desc->mime_types_count; i++)
		{
			if(desc->mime_types[i].q >= 10)
			{
				p += sprintf(p, "%s;q=1.0, ", desc->mime_types[i].mime_type);
			}
			else
			{
				p += sprintf(p, "%s;q=0.%u, ", desc->mime_types[i].mime_type, desc->mime_types[i].q);
			}
		}
	}
	strcpy(p, "*/*;q=0.1");
	return context->accept;
}

/* Set the error state and an error message on a context */
int 
lod_set_error_(LODCONTEXT *context, const char *msg)
{
	context->error = 1;
	/* Only the first error between calls to lod_reset_() will be stored */
	if(!context->errmsg)
	{
		context->errmsg = strdup(msg);
	}
	return 0;
}

/* Reset the internal state of a context */
int
lod_reset_(LODCONTEXT *context)
{
	int i;

	if(context->subjects)
	{
		for(i = 0; i < context->nsubjects; i++)
		{
			if(context->subjects[i] == context->subject)
			{
				context->subject = NULL;
			}
			if(context->subjects[i] == context->document)
			{
				context->document = NULL;
			}
			free(context->subjects[i]);
		}
		free(context->subjects);
	}
	context->subjects = NULL;
	context->nsubjects = 0;
	context->status = 0;
	context->error = 0;
	free(context->errmsg);
	context->errmsg = NULL;
	free(context->document);
	context->document = NULL;
	free(context->subject);
	context->subject = NULL;
	free(context->buf);
	context->buf = NULL;
	context->buflen = 0;
	context->bufsize = 0;
	return 0;
}

/* Add a subject URI to the context; the string becomes owned by the
 * the context.
 */
int
lod_push_subject_(LODCONTEXT *context, char *uri)
{
	if(!context->subjects)
	{
		context->subjects = (char **) calloc(context->max_redirects + 1, sizeof(char *));
		if(!context->subjects)
		{
			lod_set_error_(context, strerror(errno));
			return -1;
		}
		context->nsubjects = 0;
	}
	if(context->nsubjects >= context->max_redirects)
	{
		lod_set_error_(context, strerror(ENOMEM));
		return -1;
	}
	context->subjects[context->nsubjects] = uri;
	context->nsubjects++;
	return 0;
}
