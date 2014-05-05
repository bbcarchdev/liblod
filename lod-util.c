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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <curl/curl.h>

#include "liblod.h"

#define LOD_FETCH_ALWAYS                0
#define LOD_FETCH_ABSENT                1
#define LOD_FETCH_NEVER                 2
#define LOD_FETCH_MODE                  0x000f
#define LOD_FETCH_FLAGS                 0xf000
#define LOD_FETCH_PRIMARYTOPIC          0x1000

static LODCONTEXT *context;
static int mode, flags;

static int resolve_uri(const char *uri, int mode);
static int process_command(const char *command);
static librdf_serializer *get_serializer(void);
static int perform_query(const char *query);

int
main(int argc, char **argv)
{
	int index;
	char buf[1024], *t;

	mode = LOD_FETCH_ABSENT;
	if(curl_global_init(CURL_GLOBAL_ALL))
	{
		fprintf(stderr, "%s: failed to initialise libcurl\n", argv[0]);
		return 1;
	}
	index = 1;
	printf("%s: a test harness for liblod\n", argv[0]);
	printf("Enter a URI to resolve, or .COMMAND (type '.help' for a list of commands)\n\n");	
	while(1)
	{
		/* While there are URIs specified on the command-line, process them */
		if(index < argc)
		{
			t = argv[index];
			index++;
		}
		else
		{
			fputs("> ", stdout);
			fflush(stdout);
			if(!fgets(buf, sizeof(buf), stdin))
			{
				break;
			}
			t = strchr(buf, 0);
			t--;
			while(t >= buf && isspace(*t))
			{
				*t = 0;
				t--;
			}
			t = buf;
			while(*t && isspace(*t))
			{
				t++;
			}
			if(!*t)
			{
				continue;
			}
			if(*t == '.')
			{				
				t++;
				if(process_command(t))
				{
					break;
				}
				continue;
			}
		}
		if(!context)
		{
			context = lod_create();
			if(!context)
			{
				fprintf(stderr, "failed to create context: %s\n", strerror(errno));
				return 1;
			}
		}
		resolve_uri(t, mode | flags);
	}
	if(context)
	{
		lod_destroy(context);
	}
	return 0;
}

static int
resolve_uri(const char *uri, int mode)
{
	LODINSTANCE *instance;
	librdf_serializer *serializer;
	librdf_stream *stream;
	const char *s;

	switch(mode & LOD_FETCH_MODE)
	{
	case LOD_FETCH_NEVER:
		instance = lod_locate(context, uri);
		break;
	case LOD_FETCH_ABSENT:
		instance = lod_resolve(context, uri);
		break;
	case LOD_FETCH_ALWAYS:
		instance = lod_fetch(context, uri);
	}
	if(!instance)
	{
		if(lod_error(context))
		{
			fprintf(stderr, "failed to resolve <%s>: %s\n", uri, lod_errmsg(context));
			return -1;
		}
		printf("# subject is not present in dataset\n");
	}
	else
	{
		stream = lod_instance_stream(instance);
		serializer = get_serializer();
		librdf_serializer_serialize_stream_to_file_handle(serializer, stdout, NULL, stream);
		librdf_free_serializer(serializer);
		librdf_free_stream(stream);
		printf("# subject URI is <%s>\n", librdf_uri_as_string(lod_instance_uri(instance)));
	}
	if((s = lod_subject(context)))
	{
		printf("# request URI was <%s>\n", s);
	}
	if((s = lod_document(context)))
	{
		printf("# document URI was <%s>\n", s);
	}
	lod_instance_destroy(instance);
	return 0;
}

static int
process_command(const char *command)
{
	librdf_world *world;
	librdf_serializer *serializer;
	librdf_model *model;
	librdf_node *node, *subj, *predicate;
	librdf_stream *stream;
	librdf_statement *query, *st;
	librdf_uri *uri;

	const char *doc;
	char *s;

	if(!strcmp(command, "quit") || !strcmp(command, "exit"))
	{
		return 1;
	}
	if(!strcmp(command, "reset"))
	{
		if(context)
		{
			lod_destroy(context);
			context = NULL;
		}
		return 0;
	}
	if(!strcmp(command, "dump"))
	{
		if(context)
		{
			serializer = get_serializer();
			model = lod_model(context);
			librdf_serializer_serialize_model_to_file_handle(serializer, stdout, NULL, model);
			librdf_free_serializer(serializer);
		}
		return 0;
	}
	if(!strcmp(command, "doc"))
	{
		if(!context)
		{
			fprintf(stderr, "cannot print document triples because no document has been fetched yet\n");
			return 0;
		}
		doc = lod_document(context);
		if(!doc)
		{
			fprintf(stderr, "cannot print document triples because no document has been fetched yet\n");
			return 0;
		}
		s = strdup(doc);
		resolve_uri(s, LOD_FETCH_NEVER);
		free(s);
		return 0;
	}
	if(!strcmp(command, "primary"))
	{
		if(!context)
		{
			fprintf(stderr, "cannot print primary topic triples because no document has been fetched yet\n");
			return 0;
		}
		doc = lod_document(context);
		if(!doc)
		{
			fprintf(stderr, "cannot print primary topic triples because no document has been fetched yet\n");
			return 0;
		}
		world = lod_world(context);
		model = lod_model(context);
		subj = librdf_new_node_from_uri_string(world, (const unsigned char *) doc);
		predicate = librdf_new_node_from_uri_string(world, (const unsigned char *) "http://xmlns.com/foaf/0.1/primaryTopic");
		query = librdf_new_statement_from_nodes(world, subj, predicate, NULL);
		stream = librdf_model_find_statements(model, query);
		if(librdf_stream_end(stream))
		{
			fprintf(stderr, "failed to locate a foaf:primaryTopic predicate associated with document URI <%s>\n", doc);
			librdf_free_statement(query);
			librdf_free_stream(stream);
			return 0;
		}
		st = librdf_stream_get_object(stream);
		node = librdf_statement_get_object(st);
		uri = librdf_node_get_uri(node);
		resolve_uri((const char *) librdf_uri_as_string(uri), LOD_FETCH_NEVER);
		librdf_free_statement(query);
		librdf_free_stream(stream);
		return 0;
	}
	if(!strcmp(command, "help"))
	{
		printf("Available commands:\n\n"
			   "    .help             Print this message\n"
			   "    .quit or .exit    End lod-util session\n"
			   "    .reset            Discard the current context\n"
			   "    .dump             Print all of the triples in the current context\n"
			   "    .doc              Print the triples relating to the most recently-fetched\n"
			   "                      document\n"
			   "    .primary          Print the triples relating to the primary topic of the\n"
			   "                      most recently-fetched document\n"
			   "    .fetch [MODE]     Print or set the fetch mode\n"
			   "    .follow           Toggle whether foaf:primaryTopic will be\n"
			   "                      followed if encountered\n"
			   "\n");
		
		return 0;
	}
	if(!strcmp(command, "fetch") || !strncmp(command, "fetch ", 6))
	{
		command += 5;
		while(isspace(*command))
		{
			command++;
		}
		if(!*command)
		{
			switch(mode)
			{
			case LOD_FETCH_NEVER:
				printf("will never fetch\n");
				break;
			case LOD_FETCH_ALWAYS:
				printf("will always fetch\n");
				break;
			case LOD_FETCH_ABSENT:
				printf("will conditionally fetch if URI is not already a subject in the context\n");
				break;
			default:
				break;
			}
			return 0;
		}
		if(!strcmp(command, "never"))
		{
			printf("fetching disabled\n");
			mode = LOD_FETCH_NEVER;
			return 0;
		}
		if(!strcmp(command, "always"))
		{
			printf("fetching enabled (always fetch)\n");
			mode = LOD_FETCH_ALWAYS;
			return 0;
		}
		if(!strcmp(command, "conditional") || !strcmp(command, "cond") || !strcmp(command, "absent"))
		{
			printf("fetching conditionally enabled (fetch if needed)\n");
			mode = LOD_FETCH_ABSENT;
			return 0;
		}
		fprintf(stderr, "unrecognised fetch mode: '%s'\n", command);
		fprintf(stderr, "Usage: .mode always|never|cond[itional]\n");
		return 0;
	}
	if(!strcmp(command, "follow"))
	{
		if(flags & LOD_FETCH_PRIMARYTOPIC)
		{
			flags &= ~LOD_FETCH_PRIMARYTOPIC;
			printf("will not follow foaf:primaryTopic\n");
		}
		else
		{
			flags |= LOD_FETCH_PRIMARYTOPIC;
			printf("will follow foaf:primaryTopic\n");
		}
		return 0;
	}
	if(!strcmp(command, "q") || !strncmp(command, "q ", 2))
	{
		command++;
		while(isspace(*command))
		{
			command++;
		}
		if(!*command)
		{
			fprintf(stderr, "Usage: .q SPARQL-QUERY\n");
			return 0;
		}
		return perform_query(command);
	}
	fprintf(stderr, "unrecognised command: .%s\n", command);
	return 0;
}

static librdf_serializer *
get_serializer(void)
{
	librdf_world *world;
	librdf_serializer *serializer;
	librdf_uri *uri;

	world = lod_world(context);
	serializer = librdf_new_serializer(world, "turtle", NULL, NULL);
	if(!serializer)
	{
		fprintf(stderr, "failed to create serializer: %s\n", lod_errmsg(context));
		exit(1);
	}
	uri = librdf_new_uri(world, (const unsigned char *) "http://www.w3.org/2000/01/rdf-schema#");
	librdf_serializer_set_namespace(serializer, uri, "rdfs");
	librdf_free_uri(uri);
	uri = librdf_new_uri(world, (const unsigned char *) "http://www.w3.org/2001/XMLSchema#");
	librdf_serializer_set_namespace(serializer, uri, "xsd");
	librdf_free_uri(uri);
	uri = librdf_new_uri(world, (const unsigned char *) "http://purl.org/dc/terms/");
	librdf_serializer_set_namespace(serializer, uri, "dct");
	librdf_free_uri(uri);
	uri = librdf_new_uri(world, (const unsigned char *) "http://purl.org/dc/elements/1.1/");
	librdf_serializer_set_namespace(serializer, uri, "dc");
	librdf_free_uri(uri);
	uri = librdf_new_uri(world, (const unsigned char *) "http://xmlns.com/foaf/0.1/");
	librdf_serializer_set_namespace(serializer, uri, "foaf");
	librdf_free_uri(uri);
	return serializer;
}

static int
perform_query(const char *query)
{
	librdf_world *world;
	librdf_model *model;
	librdf_query *q;
	librdf_query_results *res;

	world = lod_world(context);
	model = lod_model(context);
	q = librdf_new_query(world, "sparql", NULL, (const unsigned char *) query, NULL);
	res = librdf_query_execute(q, model);
	if(!res)
	{
		fprintf(stderr, "failed to execute query: %s\n", lod_errmsg(context));
		librdf_free_query(q);
		return 0;
	}

	librdf_query_results_to_file_handle2(res, stdout, "table", NULL, NULL, NULL);

	librdf_free_query_results(res);
	librdf_free_query(q);
	return 0;
}

	


