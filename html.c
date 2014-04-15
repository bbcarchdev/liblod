#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_liblod.h"

static void lod_html_xml_generic_error_(void * ctx, const char * msg, ...);
static void lod_html_xml_structured_error_(void *userData, xmlErrorPtr error);

int
lod_html_discover_(LODCONTEXT *context, const char *url, char **newurl)
{
	htmlParserCtxtPtr ctx;
	xmlDoc *doc;
	xmlXPathContextPtr xpctx;
	xmlXPathObjectPtr result;
	xmlChar *rel, *type, *href;
	int i;
	URI *base, *dest;

	*newurl = NULL;
	base = uri_create_str(url, NULL);
	if(!base)
	{
		lod_set_error_(context, strerror(errno));
		return -1;
	}
	/* Parse the buffer using libxml2's HTML parser */
	ctx = htmlNewParserCtxt();
	if(!ctx)
	{
		return -1;
	}
	xmlSetGenericErrorFunc(ctx, lod_html_xml_generic_error_);
	xmlSetStructuredErrorFunc(ctx, lod_html_xml_structured_error_);
	doc = htmlCtxtReadMemory(ctx, context->buf, context->buflen, url, NULL, 0);
	if(!doc)
	{
		lod_set_error_(context, "failed to parse HTML document");
		return -1;
	}
	xpctx = xmlXPathNewContext(doc);
	if(!xpctx)
	{
		lod_set_error_(context, "failed to create XPath context");
		xmlFreeDoc(doc);
		xmlFreeParserCtxt(ctx);
		return -1;
	}
	result = xmlXPathEvalExpression((const xmlChar *) "//link", xpctx);
	xmlXPathFreeContext(xpctx);
	if(xmlXPathNodeSetIsEmpty(result->nodesetval))
	{
		xmlXPathFreeObject(result);
		xmlFreeDoc(doc);
		xmlFreeParserCtxt(ctx);
		return 0;
	}
	dest = NULL;
	for(i = 0; i < result->nodesetval->nodeNr; i++)
	{
		rel = xmlGetProp(result->nodesetval->nodeTab[i], (const xmlChar *) "rel");
		type = xmlGetProp(result->nodesetval->nodeTab[i], (const xmlChar *) "type");
		href = xmlGetProp(result->nodesetval->nodeTab[i], (const xmlChar *) "href");
		if(rel && type && href)
		{
			if(!strcmp((const char *) rel, "alternate") &&
			   (!strcmp((const char *) type, "text/turtle") ||
				!strcmp((const char *) type, "application/rdf+xml")))
			{
				dest = uri_create_str((const char *) href, base);
			}
		}
		xmlFree(rel);
		xmlFree(type);
		xmlFree(href);
		if(dest)
		{
			break;
		}
	}
	if(dest)
	{
		*newurl = uri_stralloc(dest);
		uri_destroy(dest);
	}
	uri_destroy(base);
	xmlXPathFreeObject(result);
	xmlFreeDoc(doc);
	xmlFreeParserCtxt(ctx);
	if(*newurl)
	{
		return 1;
	}
	return 0;
}

static void
lod_html_xml_generic_error_(void * ctx, const char * msg, ...)
{
	(void) ctx;
	(void) msg;
}

static void
lod_html_xml_structured_error_(void *userData, xmlErrorPtr error)
{
	(void) userData;
	(void) error;
}
 

