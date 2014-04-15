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


