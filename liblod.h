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

#ifndef LIBLOD_H_
# define LIBLOD_H_                      1

# include <librdf.h>
# include <curl/curl.h>

typedef struct lod_context_struct LODCONTEXT;
typedef struct lod_instance_struct LODINSTANCE;

/* Flags which alter how lod_resolve() behaves */
typedef enum
{
	/* Never fetch data, even if it doesn't exist in the model */
	LOD_FETCH_NEVER = 0,
	/* Fetch data only if the subject isn't present in the model */
	LOD_FETCH_ABSENT = 1,
	/* Always fetch data, even if the subject is already present */
	LOD_FETCH_ALWAYS = 2,
	/* Mask for fetch modes */
	LOD_FETCH_MODE = 0x000f,
	/* Mask for flags */
	LOD_FETCH_FLAGS = 0xf000,
	/* Follow foaf:primaryTopic, if present */
	LOD_FETCH_PRIMARYTOPIC = 0x1000
} LODFETCH;

/* Create a new LOD context */
LODCONTEXT *lod_create(void);

/* Free a LOD context created by lod_create() */
int lod_destroy(LODCONTEXT *context);

/* Obtain the librdf world used by the context */
librdf_world *lod_world(LODCONTEXT *context);

/* Set the librdf world which will be used by the context. Setting a new
 * librdf world will cause any existing storage and model associated with
 * the context to be freed if it was automatically allocated, and discarded
 * otherwise.
 *
 * It's the caller's responsibility to free the world once the context has
 * been destroyed.
 */
int lod_set_world(LODCONTEXT *context, librdf_world *world);

/* Obtain the librdf storage used by the context */
librdf_storage *lod_storage(LODCONTEXT *context);

/* Set the librdf storage which will be used by the context (must be created
 * using the context's librdf world). Setting a new librdf storage will cause
 * any existing model associated with the context to be freed if it was
 * automatically allocated, and discarded otherwise.
 *
 * It's the caller's responsibility to free the storage once the context has
 * been destroyed.
 */
int lod_set_storage(LODCONTEXT *context, librdf_storage *storage);

/* Obtain the librdf model used by the context */
librdf_model *lod_model(LODCONTEXT *context);

/* Set the librdf model which will be used by the context (must be created
 * using the context's librdf world).
 *
 * It's the caller's responsibility to free the model once the context has
 * been destroyed.
 */
int lod_set_model(LODCONTEXT *context, librdf_model *model);

/* Obtain a cURL handle for a context */
CURL *lod_curl(LODCONTEXT *context);

/* Set the cURL handle which will be used for future fetches by the context.
 *
 * Note that if an explicit cURL handle is supplied, liblod will not set
 * any HTTP request headers (including Accept), and the caller must do it
 * instead.
 *
 * It's the caller's responsibility to free the handle once the context has
 * been destroyed.
 */
int lod_set_curl(LODCONTEXT *context, CURL *ch);

/* Return the subject URI (after following any relevant redirects) that was
 * most recently resolved, if any.
 *
 * The pointer returned by lod_subject() is valid until the next call to
 * lod_destroy() or lod_resolve() (whichever is invoked sooner), and must not
 * be freed by the caller.
 */
const char *lod_subject(LODCONTEXT *context);

/* Return the document URL (after following any relevant redirects) that was
 * most recently fetched from (if any).
 */
const char *lod_document(LODCONTEXT *context);

/* Return the HTTP status code from the most recent resolution request; a
 * return value of zero means no fetch was performed.
 */
long lod_status(LODCONTEXT *context);

/* Return the context error state from the most recent resolution request;
 * a value of zero means no error has occurred.
 */
int lod_error(LODCONTEXT *context);

/* Return the error message from the most recent resolution request */
const char *lod_errmsg(LODCONTEXT *context);

/* Logging function which is set on librdf_world objects via
 * librdf_world_set_logger(); note that the context must be
 * specified as the "user_data" parameter.
 */
int lod_librdf_logger(void *userdata, librdf_log_message *message);

/* Return the default User-agent header */
const char *lod_useragent(LODCONTEXT *context);

/* Return the default Accept header */
const char *lod_accept(LODCONTEXT *context);

/* Resolve a LOD URI, potentially fetching data */
LODINSTANCE *lod_resolve(LODCONTEXT *context, const char *uri, LODFETCH fetchmode);

/* Attempt to locate a subject within the context's model, but don't
 * try to fetch it all.
 */
LODINSTANCE *lod_locate(LODCONTEXT *context, const char *uri);

/* Fetch data about a subject, fetching the data about it (irrespective of
 * whether it already exists in the model.
 */
LODINSTANCE *lod_fetch(LODCONTEXT *context, const char *uri);

/* Free an instance returned by lod_resolve() -- note that the triples remain
 * part of the context until it is destroyed, and so a subsequent call to
 * lod_resolve(context, "uri", LOD_FETCH_NEVER); would return a new subject
 * referencing the same triples.
 */
int lod_instance_destroy(LODINSTANCE *instance);

/* Return the URI of the instance */
librdf_uri *lod_instance_uri(LODINSTANCE *instance);

/* Return a stream filtering the triples in the context by subject */
librdf_stream *lod_instance_stream(LODINSTANCE *instance);

/* Return 1 if the subject exists in the related context */
int lod_instance_exists(LODINSTANCE *instance);

/* Return an instance representing the foaf:primaryTopic of the supplied
 * instance, if one exists.
 */
LODINSTANCE *lod_instance_primarytopic(LODINSTANCE *instance);

#endif /*!LIBLOD_H_*/
