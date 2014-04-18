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

#include "p_liblod.h"

/* Content sniffing is evil, but sometimes necessary. At least in this
 * case it's only within tightly-bound parameters and easy to remove
 * later if all of the badly-configured web servers in the world go
 * away.
 */
int
lod_sniff_(LODCONTEXT *context, char **type)
{
	const char *t;
	char *p;
	size_t len;

	t = context->buf;
	len = context->buflen;
	while(len && *t && isspace(*t))
	{
		len--;
		t++;
	}
	if(!*t || !len)
	{
		return 1;
	}
	/* There's a minimum length that is reasonable for anything we're
	 * going to actually attempt to parse -- if the buffer length is
	 * shorter than that, it's not worth attempting to do
	 * any sniffing.
	 */
	if(len < 128)
	{
		return 1;
	}
	if(t[0] == '<' && (t[1] == '!' || t[1] == '?'))
	{
		/* Guess at this being XML */
		if(!(p = strdup("application/rdf+xml")))
		{
			lod_set_error_(context, strerror(errno));
			return -1;
		}
		free(*type);
		*type = p;
		return 0;
	}
	if(!strncmp(t, "@base", 5) ||
	   !strncmp(t, "@prefix", 7) ||
	   !strncmp(t, "<http", 5))
	{
		/* Could be N-Triples or Turtle */
		if(!(p = strdup("text/turtle")))
		{
			lod_set_error_(context, strerror(errno));
			return -1;
		}
		free(*type);
		*type = p;
		return 0;
	}
	/* No match */
	return 1;
}
