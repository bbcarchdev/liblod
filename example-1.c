#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <liblod.h>

int
main(void)
{
	LODCONTEXT *ctx;
	LODINSTANCE *inst;
	librdf_world *world;
	librdf_serializer *serializer;
	librdf_stream *stream;

	ctx = lod_create();
	if(!ctx)
	{
		fprintf(stderr, "failed to create liblod context: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	world = lod_world(ctx);
	if(!world)
	{
		fprintf(stderr, "failed to obtain librdf_world for context: %s\n", lod_errmsg(ctx));
		lod_destroy(ctx);
		exit(EXIT_FAILURE);
	}
	/* Fetch data about the city of Oxford */
	inst = lod_resolve(ctx, "http://www.dbpedialite.org/things/22308#id", LOD_FETCH_ABSENT);
	if(!inst)
	{
		fprintf(stderr, "failed to obtain data about Oxford: %s\n", lod_errmsg(ctx));
		lod_destroy(ctx);
		exit(EXIT_FAILURE);
	}
	/* Check that the returned data does actually describe the subject we
	 * requested.
	 */
	if(!lod_instance_exists(inst))
	{
		fprintf(stderr, "no data about Oxford was found in the returned data\n");
		lod_instance_destroy(inst);
		lod_destroy(ctx);
		exit(EXIT_FAILURE);
	}
	/* Obtain a librdf_stream */
	stream = lod_instance_stream(inst);
	if(!stream)
	{
		fprintf(stderr, "failed to obtain librdf stream for the instance\n");
		lod_instance_destroy(inst);
		lod_destroy(ctx);
		exit(EXIT_FAILURE);
	}
	/* Serialise the data as Turtle */
	serializer = librdf_new_serializer(world, "turtle", NULL, NULL);
	if(!serializer)
	{
		fprintf(stderr, "failed to create serializer: %s\n", lod_errmsg(ctx));
		librdf_free_stream(stream);
		lod_instance_destroy(inst);
		lod_destroy(ctx);
		exit(1);
	}
	if(librdf_serializer_serialize_stream_to_file_handle(serializer, stdout, NULL, stream))
	{
		fprintf(stderr, "failed to serialize stream: %s\n", lod_errmsg(ctx));
		librdf_free_serializer(serializer);
		librdf_free_stream(stream);
		lod_instance_destroy(inst);
		lod_destroy(ctx);
		exit(1);
	}
	librdf_free_serializer(serializer);
	librdf_free_stream(stream);
	/* Release resources */
	lod_instance_destroy(inst);
	lod_destroy(ctx);
	return 0;
}
