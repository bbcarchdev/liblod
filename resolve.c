#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_liblod.h"

/* Resolve a LOD URI, potentially fetching data */
LODINSTANCE *
lod_resolve(LODCONTEXT *context, const char *uri, LODFETCH fetchmode)
{
	LODINSTANCE *inst;
	librdf_stream *stream;
	librdf_world *world;
	librdf_model *model;	
	librdf_node *node;
	librdf_statement *query;
	
	lod_reset_(context);
	context->subject = strdup(uri);
	if(!context->subject)
	{	   
		lod_set_error_(context, strerror(errno));
		return NULL;
	}
	world = lod_world(context);
	if(!world)
	{
		context->error = 1;
		return NULL;
	}
	model = lod_model(context);
	if(!model)
	{
		context->error = 1;
		return NULL;
	}
	if(fetchmode == LOD_FETCH_ALWAYS)
	{
		/* It doesn't matter whether triples about the subject already exist
		 * if we're going to fetch afresh anyway.
		 */
		query = NULL;
	}
	else
	{
		/* Attempt to locate triples about the subject */
		node = librdf_new_node_from_uri_string(world, (const unsigned char *) uri);
		if(!node)
		{
			context->error = 1;
			return NULL;
		}		
		query = librdf_new_statement_from_nodes(world, node, NULL, NULL);
		/* Note: node becomes owned by the statement, and freed upon error */
		if(!query)
		{
			context->error = 1;
			return NULL;
		}
		stream = librdf_model_find_statements(model, query);
		if(!stream)
		{
			context->error = 1;
			return NULL;
		}
	}
	if(fetchmode == LOD_FETCH_NEVER ||
	   (fetchmode == LOD_FETCH_ABSENT && !librdf_stream_end(stream)))
	{
		librdf_free_stream(stream);
		inst = lod_instance_create_(context, query, node);
		if(!inst)
		{
			librdf_free_statement(query);
			return NULL;
		}
		return inst;
	}
	if(query)
	{
		librdf_free_statement(query);
	}
	if(stream)
	{
		librdf_free_stream(stream);
	}
	if(lod_fetch_(context))
	{
		return NULL;
	}
	/* Now that we've successfully fetched the URI, Attempt to locate
	 * triples about the subject (possibly for the second time)
	 */
	node = librdf_new_node_from_uri_string(world, (const unsigned char *) context->subject);
	if(!node)
	{
		context->error = 1;
		return NULL;
	}		
	query = librdf_new_statement_from_nodes(world, node, NULL, NULL);
	/* Note: node becomes owned by the statement, and freed upon error */
	if(!query)
	{
		context->error = 1;
		return NULL;
	}
	inst = lod_instance_create_(context, query, node);
	if(!inst)
	{
		librdf_free_statement(query);
		return NULL;
	}
	return inst;
}

