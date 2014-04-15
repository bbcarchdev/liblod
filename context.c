#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_liblod.h"

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
	return p;
}

/* Free a LOD context */
int
lod_destroy(LODCONTEXT *context)
{
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
	free(context->subject);
	free(context->document);
	free(context->errmsg);
	free(context->buf);
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
	if(context->ch)
	{
		return context->ch;
	}
	context->ch_alloc = 1;
	context->ch = curl_easy_init();
	if(!context->ch)
	{
		lod_set_error_(context, "failed to create new cURL handle");
		return NULL;
	}
	context->headers = curl_slist_append(NULL, "Accept: text/turtle;q=0.8, application/rdf+xml;q=0.75, */*;q=0.1");
	context->headers = curl_slist_append(context->headers, "User-Agent: liblod/1 (+https://github.com/bbcarchdev/liblod)");
	curl_easy_setopt(context->ch, CURLOPT_HTTPHEADER, context->headers);
	curl_easy_setopt(context->ch, CURLOPT_VERBOSE, 0);
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
	fprintf(stderr, "[%d] '%s'\n", level, librdf_log_message_message(message));
	if(level >= LIBRDF_LOG_ERROR)
	{
		fprintf(stderr, "setting error condition\n");
		lod_set_error_(context, librdf_log_message_message(message));
	}
	return 1;
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
