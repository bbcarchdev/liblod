#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_liblod.h"

LODSUBJECT *
lod_subject_create_(LODCONTEXT *context, librdf_statement *query, librdf_node *subject)
{
	LODSUBJECT *p;

	p = (LODSUBJECT *) calloc(1, sizeof(LODSUBJECT));
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
lod_subject_destroy(LODSUBJECT *subject)
{
	librdf_free_statement(subject->query);
	free(subject);
	return 0;
}

librdf_stream *
lod_subject_stream(LODSUBJECT *subject)
{
	librdf_stream *stream;
	librdf_model *model;

	model = lod_model(subject->context);
	if(!model)
	{
		return NULL;
	}
	stream = librdf_model_find_statements(model, subject->query);
	if(!stream)
	{
		subject->context->error = 1;
		return NULL;
	}
	return stream;
}

int
lod_subject_exists(LODSUBJECT *subject)
{
	librdf_stream *stream;
	librdf_model *model;
	int e;

	model = lod_model(subject->context);
	if(!model)
	{
		return -1;
	}
	stream = librdf_model_find_statements(model, subject->query);
	if(!stream)
	{
		subject->context->error = 1;
		return -1;
	}
	e = !librdf_stream_end(stream);
	librdf_free_stream(stream);
	return e;
}


