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

/* Resolve a LOD URI, potentially fetching data */
LODINSTANCE *
lod_resolve(LODCONTEXT *context, const char *uri, LODFETCH fetchmode)
{
	LODINSTANCE *inst, *primary;
	librdf_stream *stream;
	librdf_world *world;
	librdf_model *model;	
	librdf_node *node;
	librdf_statement *query;
	char *p;
	int i;
	LODFETCH mode;

	mode = fetchmode & LOD_FETCH_MODE;
	/* Duplicate the URI first, in case it's actually a string belonging
	 * to the context itself which would get deallocated by lod_reset_()
	 */
	p = strdup(uri);
	if(!p)
	{
		lod_set_error_(context, strerror(errno));
		return NULL;
	}		
	lod_reset_(context);
	context->subject = p;
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
	if(mode == LOD_FETCH_ALWAYS)
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
			librdf_free_statement(query);
			return NULL;
		}
	}
	if(mode == LOD_FETCH_NEVER ||
	   (mode == LOD_FETCH_ABSENT && !librdf_stream_end(stream)))
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
	inst = NULL;
	for(i = 0; i < context->nsubjects; i++)
	{
		if(inst)
		{
			lod_instance_destroy(inst);
		}
		/* Now that we've successfully fetched the URI, Attempt to locate
		 * triples about the subject (possibly for the second time)
		 */
		node = librdf_new_node_from_uri_string(world, (const unsigned char *) context->subjects[i]);
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
		if(lod_instance_exists(inst))
		{
			if(fetchmode & LOD_FETCH_PRIMARYTOPIC)
			{
				primary = lod_instance_primarytopic(inst);
				if(primary)
				{
					lod_instance_destroy(inst);
					inst = primary;
				}
				else if(context->error)
				{
					lod_instance_destroy(inst);
					return NULL;
				}
			}
			break;
		}
	}
	return inst;
}

