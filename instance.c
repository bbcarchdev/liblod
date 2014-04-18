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

LODINSTANCE *
lod_instance_create_(LODCONTEXT *context, librdf_statement *query, librdf_node *subject)
{
	LODINSTANCE *p;

	p = (LODINSTANCE *) calloc(1, sizeof(LODINSTANCE));
	if(!p)
	{
		lod_set_error_(context, strerror(errno));
		return NULL;
	}
	p->context = context;
	p->query = query;
	p->subject = subject;
	return p;
}

int
lod_instance_destroy(LODINSTANCE *instance)
{
	librdf_free_statement(instance->query);
	free(instance);
	return 0;
}

/* Return the URI of the instance */
librdf_uri *
lod_instance_uri(LODINSTANCE *instance)
{
	return librdf_node_get_uri(instance->subject);
}

librdf_stream *
lod_instance_stream(LODINSTANCE *instance)
{
	librdf_stream *stream;
	librdf_model *model;

	model = lod_model(instance->context);
	if(!model)
	{
		return NULL;
	}
	stream = librdf_model_find_statements(model, instance->query);
	if(!stream)
	{
		instance->context->error = 1;
		return NULL;
	}
	return stream;
}

int
lod_instance_exists(LODINSTANCE *instance)
{
	librdf_stream *stream;
	librdf_model *model;
	int e;

	model = lod_model(instance->context);
	if(!model)
	{
		return -1;
	}
	stream = librdf_model_find_statements(model, instance->query);
	if(!stream)
	{
		instance->context->error = 1;
		return -1;
	}
	e = !librdf_stream_end(stream);
	librdf_free_stream(stream);
	return e;
}

/* Return an instance representing the foaf:primaryTopic of the supplied
 * instance, if one exists.
 */
LODINSTANCE *
lod_instance_primarytopic(LODINSTANCE *instance)
{
	LODINSTANCE *inst;
	librdf_model *model;
	librdf_world *world;
	librdf_node *subject, *predicate, *object, *onode;
	librdf_statement *query, *oquery, *triple;
	librdf_stream *result, *oresult;
	
	inst = NULL;
	world = lod_world(instance->context);
	if(!world)
	{
		return NULL;
	}
	model = lod_model(instance->context);
	if(!model)
	{
		return NULL;
	}
	subject = librdf_new_node_from_node(instance->subject);
	if(!subject)
	{
		instance->context->error = 1;
		return NULL;
	}
	predicate = librdf_new_node_from_uri_string(world, (const unsigned char *) "http://xmlns.com/foaf/0.1/primaryTopic");
	query = librdf_new_statement_from_nodes(world, subject, predicate, NULL);
	result = librdf_model_find_statements(model, query);
	while(!librdf_stream_end(result))
	{
		triple = librdf_stream_get_object(result);
		object = librdf_statement_get_object(triple);
		/* Now there's an object, query the model again to see if there are
		 * any matching triples with it as a subject
		 */
		onode = librdf_new_node_from_node(object);
		oquery = librdf_new_statement_from_nodes(world, onode, NULL, NULL);
		oresult = librdf_model_find_statements(model, oquery);
		if(!librdf_stream_end(oresult))
		{
			/* A match was found: create a new LODINSTANCE representing
			 * the query and return it.
			 */
			librdf_free_stream(oresult);
			inst = lod_instance_create_(instance->context, oquery, onode);
			break;
		}
		librdf_free_stream(oresult);
		librdf_free_statement(oquery);
		librdf_stream_next(result);
	}
	librdf_free_stream(result);
	librdf_free_statement(query);
	return inst;
}
