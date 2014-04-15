#ifndef P_LIBLOD_H_
# define P_LIBLOD_H_                    1

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <librdf.h>
# include <errno.h>
# include <curl/curl.h>
# include <libxml/HTMLparser.h>
# include <libxml/xpath.h>
# include <libxml/xpathInternals.h>
# include <liburi.h>

# include "liblod.h"

struct lod_context_struct
{
	librdf_world *world;
	librdf_storage *storage;
	librdf_model *model;
	CURL *ch;
	struct curl_slist *headers;
	char *buf;
	size_t bufsize;
	size_t buflen;
	char *subject;
	char *document;
	long status;
	int error;
	char *errmsg;
	int world_alloc:1;
	int storage_alloc:1;
	int model_alloc:1;
	int ch_alloc:1;
};

int lod_reset_(LODCONTEXT *context);
int lod_set_error_(LODCONTEXT *context, const char *msg);
int lod_fetch_(LODCONTEXT *context);
int lod_html_discover_(LODCONTEXT *context, const char *url, char **newurl);

#endif /*!P_LIBLOD_H_*/
