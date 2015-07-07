#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_tests.h"

/* Test parsing a string into a liblod-created model using librdf's parsing
 * APIs, then attempting to locate it using liblod's APIs - this time
 * expecting it to NOT be present.
 */

#include "dbpl-oxford.h"

static const char *dummy_uri = "http://liblod.example.com/xyzzy#id";

int
main(int argc, char **argv)
{
	LODCONTEXT *ctx;
	LODINSTANCE *inst;
	librdf_world *world;
	librdf_model *model;
	librdf_parser *parser;
	librdf_uri *uri;

	(void) argc;


	ctx = lod_create();
	if(!ctx)
	{
		fprintf(stderr, "%s: failed to create liblod context: %s\n", argv[0], strerror(errno));
		exit(EXIT_FAILURE);
	}
	world = lod_world(ctx);
	if(!world)
	{
		fprintf(stderr, "%s: failed to obtain librdf_world for context: %s\n", argv[0], lod_errmsg(ctx));
		lod_destroy(ctx);
		exit(EXIT_FAILURE);
	}
	model = lod_model(ctx);
	if(!model)
	{
		fprintf(stderr, "%s: failed to obtain librdf_model for context: %s\n", argv[0], lod_errmsg(ctx));
		lod_destroy(ctx);
		exit(EXIT_FAILURE);
	}
	parser = librdf_new_parser(world, "turtle", NULL, NULL);
	if(!parser)
	{
		fprintf(stderr, "%s: failed to create librdf parser for turtle: %s\n", argv[0], lod_errmsg(ctx));
		lod_destroy(ctx);
		exit(EXIT_FAILURE);
	}
	uri = librdf_new_uri(world, (const unsigned char *) oxford_doc);
	if(!uri)
	{
		fprintf(stderr, "%s: failed to create librdf URI for <%s> %s\n", argv[0], oxford_doc, lod_errmsg(ctx));
		lod_destroy(ctx);
		librdf_free_parser(parser);
		exit(EXIT_FAILURE);
	}
	if(librdf_parser_parse_string_into_model(parser, (const unsigned char *) oxford_ttl, uri, model))
	{
		fprintf(stderr, "%s: failed to parse string into model: %s\n", argv[0], lod_errmsg(ctx));
		librdf_free_uri(uri);
		librdf_free_parser(parser);
		lod_destroy(ctx);
		exit(EXIT_FAILURE);
	}
	librdf_free_parser(parser);
	librdf_free_uri(uri);
	/* Attempt to locate data about a URI we know isn't present */
	inst = lod_locate(ctx, dummy_uri);
	if(inst)
	{
		fprintf(stderr, "%s: unexpectedly found data about <%s>\n", argv[0], dummy_uri);
		lod_instance_destroy(inst);
		lod_destroy(ctx);
		exit(EXIT_FAILURE);
	}
	if(lod_error(ctx))
	{
		fprintf(stderr, "%s: unexpected error while locating triples about <%s>: %s\n", argv[0], dummy_uri, lod_errmsg(ctx));
		lod_destroy(ctx);
		exit(EXIT_FAILURE);
	}
	lod_destroy(ctx);

	return 0;
}

