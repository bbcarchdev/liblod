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

int
main(int argc, char **argv)
{
	LODCONTEXT *context;
	librdf_stream *stream;
	librdf_model *model;
	librdf_statement *st;
	int count, index;
	char buf[1024], *t;
	const char *s;

	if(curl_global_init(CURL_GLOBAL_ALL))
	{
		fprintf(stderr, "%s: failed to initialise libcurl\n", argv[0]);
		return 1;
	}
	context = NULL;
	index = 1;
	printf("%s: a test harness for liblod\n", argv[0]);
	printf("Enter a URI to resolve, or .COMMAND (type '.help' for a list of commands)\n\n");	
	while(1)
	{
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
				if(!strcmp(t, "quit") || !strcmp(t, "exit"))
				{
					break;
				}
				else if(!strcmp(t, "reset"))
				{
					if(context)
					{
						lod_destroy(context);
						context = NULL;
					}
				}
				else if(!strcmp(t, "dump"))
				{
					if(context)
					{
						model = lod_model(context);
						librdf_model_print(model, stdout);
					}
				}
				else if(!strcmp(t, "help"))
				{
					printf("Available commands:\n\n"
						   "    .help             Print this message\n"
						   "    .quit or .exit    End lod-util session\n"
						   "    .reset            Discard the current context\n"
						   "    .dump             Print all of the triples in the current context\n\n");
				}
				else
				{
					fprintf(stderr, "unrecognised command: .%s\n", t);
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
		stream = lod_resolve(context, t, LOD_FETCH_ABSENT);
		if(!stream)
		{
			fprintf(stderr, "failed to resolve <%s>: %s\n", t, lod_errmsg(context));
			continue;
		}
		count = 0;
		while(!librdf_stream_end(stream))
		{
			st = librdf_stream_get_object(stream);
			librdf_statement_print(st, stdout);
			fputc('\n', stdout);
			count++;
			librdf_stream_next(stream);
		}
		printf("# %d triples\n", count);
		if((s = lod_subject(context)))
		{
			printf("# subject URI was <%s>\n", s);
		}
		if((s = lod_document(context)))
		{
			printf("# document URI was <%s>\n", s);
		}
		librdf_free_stream(stream);
	}
	if(context)
	{
		lod_destroy(context);
	}
	return 0;
}
