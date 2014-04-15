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

static LODCONTEXT *context;

static int resolve_uri(const char *uri, LODFETCH mode);
static int process_command(const char *command);
static librdf_serializer *get_serializer(void);

int
main(int argc, char **argv)
{
	int index;
	char buf[1024], *t;

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
		resolve_uri(t, LOD_FETCH_ABSENT);
	}
	if(context)
	{
		lod_destroy(context);
	}
	return 0;
}

static int
resolve_uri(const char *uri, LODFETCH mode)
{
	LODSUBJECT *subject;
	librdf_serializer *serializer;
	librdf_stream *stream;
	const char *s;

	subject = lod_resolve(context, uri, mode);
	if(!subject)
	{
		fprintf(stderr, "failed to resolve <%s>: %s\n", uri, lod_errmsg(context));
		return -1;
	}
	if(lod_subject_exists(subject) > 0)
	{
		stream = lod_subject_stream(subject);
		serializer = get_serializer();
		librdf_serializer_serialize_stream_to_file_handle(serializer, stdout, NULL, stream);
		librdf_free_serializer(serializer);
		librdf_free_stream(stream);
	}
	else if(lod_error(context))
	{
		fprintf(stderr, "failed to determine if subject is present: %s\n", lod_errmsg(context));
	}
	else
	{
		printf("# subject is not present in dataset\n");
	}
	if((s = lod_subject(context)))
	{
		printf("# subject URI was <%s>\n", s);
	}
	if((s = lod_document(context)))
	{
		printf("# document URI was <%s>\n", s);
	}
	lod_subject_destroy(subject);
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
			   "\n");
		
		return 0;
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


