#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_liblod.h"

#define BUFSIZE                         512
#define BUFMAX                          (256 * 1024 * 1024)

static size_t lod_fetch_write_(char *ptr, size_t size, size_t nmemb, void *userdata);

/* Unconditionally fetch some LOD and parse it into the existing model */
int
lod_fetch_(LODCONTEXT *context)
{
	librdf_world *world;
	librdf_model *model;
	librdf_uri *baseuri;
	librdf_parser *parser;
	CURL *ch;
	CURLcode e;
	int r, count, followed_link;
	long code;
	const char *base, *uri, *fragment;
	char *t, *p, *type, *newuri, *tempuri, errbuf[64];
	size_t fraglen;

	tempuri = NULL;
	if(lod_push_subject_(context, context->subject))
	{
		return -1;
	}
	/* Save the fragment in case we need to apply it to a
	 * a redirect URI
	 */
	if((fragment = strchr(context->subject, '#')))
	{
		fraglen = strlen(fragment);
	}
	else
	{
		fraglen = 0;
	}
	parser = NULL;
	baseuri = NULL;
	r = 0;
	followed_link = 0;
	world = lod_world(context);
	if(!world)
	{
		return -1;
	}
	model = lod_model(context);
	if(!model)
	{
		return -1;
	}
	ch = lod_curl(context);
	if(!ch)
	{
		return -1;
	}
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *) context);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, lod_fetch_write_);
	curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 0);
	uri = context->subjects[0];
	type = NULL;
	for(count = 0; count < context->max_redirects; count++)
	{
		curl_easy_setopt(ch, CURLOPT_URL, uri);
		free(type);
		type = NULL;
		free(context->buf);
		context->buf = NULL;
		context->bufsize = 0;
		context->buflen = 0;
		e = curl_easy_perform(ch);
		if(e)
		{
			if(!context->error)
			{
				lod_set_error_(context, curl_easy_strerror(e));
			}
			r = 1;
			break;
		}
		if((e = curl_easy_getinfo(context->ch, CURLINFO_RESPONSE_CODE, &code)))
		{
			lod_set_error_(context, curl_easy_strerror(e));
			r = 1;
			break;
		}
		context->status = code;
		if((e = curl_easy_getinfo(ch, CURLINFO_EFFECTIVE_URL, &base)))
		{
			lod_set_error_(context, curl_easy_strerror(e));
			r = 1;
			break;
		}
		p = strdup(base);
		if(!p)
		{
			lod_set_error_(context, strerror(errno));
			r = 1;
			break;
		}
		t = strchr(p, '#');
		if(t)
		{
			*t = 0;
		}
		free(context->document);
		context->document = p;
		p = NULL;
		if(code >= 200 && code <= 299 && context->buf)
		{
			/* Success response */
			if((e = curl_easy_getinfo(context->ch, CURLINFO_CONTENT_TYPE, &p) != CURLE_OK))
			{
				lod_set_error_(context, curl_easy_strerror(e));
				r = 1;
				break;
			}
			if(!(type = strdup(p)))
			{
				lod_set_error_(context, strerror(errno));
				r = 1;
				break;
			}
			t = strchr(type, ';');
			/* XXX determine charset for passing to HTML parser */
			if(t)
			{
				*t = 0;
			}
			if(!strcmp(type, "text/html") ||
			   !strcmp(type, "application/xhtml+xml") ||
			   !strcmp(type, "application/vnd.wap.xhtml+xml") ||
			   !strcmp(type, "application/vnd.ctv.xhtml+xml") ||
			   !strcmp(type, "application/vnd.hbbtv.xhtml+xml"))
			{
				free(type);
				type = NULL;
				/* Perform link autodiscovery if we haven't previously done
				 * so.
				 */
				if(followed_link)
				{
					lod_set_error_(context, "a <link rel=\"alternate\"> has previously been followed in this resolution session; will not do so again");
					r = 1;
					break;
				}
				r = lod_html_discover_(context, uri, &newuri);
				if(r < 0)
				{
					lod_set_error_(context, "failed to parse HTML for autodiscovery");
					break;
				}
				else if(r == 0)
				{
					/* Autodiscovery failed */
					lod_set_error_(context, "failed to discover link to RDF representation from HTML document\n");
					r = 1;
					break;
				}
				r = 0;
				lod_push_subject_(context, newuri);
				uri = newuri;
				followed_link = 1;
				continue;
			}
			break;
		}
		if(code > 300 && code <= 399)
		{
			/* Redirect response */
			free(tempuri);
			tempuri = NULL;
			newuri = NULL;
			if((e = curl_easy_getinfo(context->ch, CURLINFO_REDIRECT_URL, &newuri)))
			{
				lod_set_error_(context, curl_easy_strerror(e));
				r = 1;
				break;
			}
			tempuri = (char *) malloc(strlen(newuri) + fraglen + 1);
			if(!tempuri)
			{
				lod_set_error_(context, strerror(errno));
				r = 1;
				break;
			}
			strcpy(tempuri, newuri);
			uri = tempuri;
			if(code == 303)
			{
				/* In the case of a 303, the new URI is simply used
				 * in the subsequent request -- we shouldn't ever push
				 * it onto the potential subject list
				 */
				continue;
			}
			if(fragment)
			{
				t = strchr(tempuri, '#');
				if(t)
				{
					strcpy(t, fragment);
				}
				else
				{
					strcat(tempuri, fragment);
				}
			}
			lod_push_subject_(context, tempuri);
			tempuri = NULL;
			continue;
		}
		/* Some other status code */
		r = 1;
		sprintf(errbuf, "HTTP status %ld\n", code);
		lod_set_error_(context, errbuf);
		break;
	}
	if(count == context->max_redirects)
	{
		lod_set_error_(context, "too many redirects encountered");
		r = 1;
	}
	if(!r)
	{
		parser = librdf_new_parser(world, NULL, type, NULL);
		if(!parser)
		{
			r = 1;
		}
	}
	if(!r)
	{
		baseuri = librdf_new_uri(world, (const unsigned char *) context->document);
		if(!baseuri)
		{
			r = 1;
		}
	}
	if(!r)
	{
		r = librdf_parser_parse_counted_string_into_model(parser, (unsigned char *) context->buf, context->buflen, baseuri, model);
		if(context->error)
		{
			r = 1;
		}
	}
	if(baseuri)
	{
		librdf_free_uri(baseuri);
	}
	if(parser)
	{
		librdf_free_parser(parser);
	}
	free(tempuri);
	free(type);
	free(context->buf);
	context->buf = NULL;
	context->bufsize = 0;
	context->buflen = 0;	
	if(r)
	{
		context->error = 1;
		return -1;
	}
	return 0;
}

/* Invoked by libcurl when payload data is received */
static size_t
lod_fetch_write_(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	LODCONTEXT *context;
	size_t toalloc;
	char *p;

	context = (LODCONTEXT *) userdata;
	size *= nmemb;
	if(context->buflen + size >= context->bufsize)
	{
		toalloc = ((context->bufsize + size) / BUFSIZE + 1) * BUFSIZE;
		if(toalloc > BUFMAX)
		{
			lod_set_error_(context, strerror(ENOMEM));
			return 0;
		}
		p = (char *) realloc(context->buf, toalloc);
		if(!p)
		{
			lod_set_error_(context, strerror(errno));
			return 0;
		}
		context->bufsize = toalloc;
		context->buf = p;
	}
	memcpy(&(context->buf[context->buflen]), ptr, size);
	context->buflen += size;
	return size;
}
