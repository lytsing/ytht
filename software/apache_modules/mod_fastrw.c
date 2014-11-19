/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

/*
**
**      lepton@ytht.org
**	most code are original from mod_fastrw.c
*/
#include "mod_fastrw.h"

#define CORE_PRIVATE
#include "httpd.h"
#include "http_config.h"
#include "http_conf_globals.h"
#include "http_request.h"
#include "http_core.h"
#include "http_log.h"
#include "http_vhost.h"

#define ENVVAR_SCRIPT_URL "SCRIPT_URL"
#define ENVVAR_SCRIPT_URI "SCRIPT_URI"

#define ENGINE_DISABLED             1<<0
#define ENGINE_ENABLED              1<<1

typedef struct {
	int state;		/* the RewriteEngine state */
	server_rec *server;	/* the corresponding server indicator */
} fastrw_server_conf;

static void *config_server_create(pool * p, server_rec * s);
static const char *cmd_fastrwengine(cmd_parms * cmd, void *dconf, int flag);
static int hook_uri2file(request_rec * r);

/*
** +-------------------------------------------------------+
** |                                                       |
** |             static module configuration
** |                                                       |
** +-------------------------------------------------------+
*/

/*
**  Our interface to the Apache server kernel:
**
**  o  Runtime logic of a request is as following:
**       while(request or subrequest)
**           foreach(stage #0...#9)
**               foreach(module) (**)
**                   try to run hook
**
**  o  the order of modules at (**) is the inverted order as
**     given in the "Configuration" file, i.e. the last module
**     specified is the first one called for each hook!
**     The core module is always the last!
**
**  o  there are two different types of result checking and
**     continue processing:
**     for hook #0,#1,#4,#5,#6,#8:
**         hook run loop stops on first modules which gives
**         back a result != DECLINED, i.e. it usually returns OK
**         which says "OK, module has handled this _stage_" and for #1
**         this have not to mean "Ok, the filename is now valid".
**     for hook #2,#3,#7,#9:
**         all hooks are run, independend of result
**
**  o  at the last stage, the core module always
**       - says "BAD_REQUEST" if r->filename does not begin with "/"
**       - prefix URL with document_root or replaced server_root
**         with document_root and sets r->filename
**       - always return a "OK" independed if the file really exists
**         or not!
*/

    /* The section for the Configure script:
     * MODULE-DEFINITION-START
     * Name: fastrw_module
     * ConfigStart
     . ./helpers/find-dbm-lib
     if [ "x$found_dbm" = "x1" ]; then
     echo "      enabling DBM support for mod_fastrw"
     else
     echo "      disabling DBM support for mod_fastrw"
     echo "      (perhaps you need to add -ldbm, -lndbm or -lgdbm to EXTRA_LIBS)"
     CFLAGS="$CFLAGS -DNO_DBM_REWRITEMAP"
     fi
     * ConfigEnd
     * MODULE-DEFINITION-END
     */

    /* the table of commands we provide */
static const command_rec command_table[] = {
	{"FastRWEngine", cmd_fastrwengine, NULL, OR_FILEINFO, FLAG,
	 "On or Off to enable or disable (default) the whole rewriting engine"},
	{NULL}
};

    /* the main config structure */
module MODULE_VAR_EXPORT fastrw_module = {
	STANDARD_MODULE_STUFF,
	NULL,			/* module initializer                  */
	NULL,			/* create per-dir    config structures */
	NULL,			/* merge  per-dir    config structures */
	config_server_create,	/* create per-server config structures */
	NULL,			/* merge  per-server config structures */
	command_table,		/* table of config file commands       */
	NULL,			/* [#8] MIME-typed-dispatched handlers */
	hook_uri2file,		/* [#1] URI to filename translation    */
	NULL,			/* [#4] validate user id from request  */
	NULL,			/* [#5] check if the user is ok _here_ */
	NULL,			/* [#3] check access by host address   */
	NULL,			/* [#6] determine MIME type            */
	NULL,			/* [#7] pre-run fixups                 */
	NULL,			/* [#9] log a transaction              */
	NULL,			/* [#2] header parser                  */
	NULL,			/* child_init                          */
	NULL,			/* child_exit                          */
	NULL			/* [#0] post read-request              */
};

/*
**
**  per-server configuration structure handling
**
*/

static void *
config_server_create(pool * p, server_rec * s)
{
	fastrw_server_conf *a;

	a = (fastrw_server_conf *) ap_pcalloc(p, sizeof (fastrw_server_conf));

	a->state = ENGINE_DISABLED;
	a->server = s;
	return (void *) a;
}

/*
**
**  the configuration commands
**
*/

static const char *
cmd_fastrwengine(cmd_parms * cmd, void *dconf, int flag)
{
	fastrw_server_conf *sconf;

	sconf =
	    (fastrw_server_conf *) ap_get_module_config(cmd->server->
							module_config,
							&fastrw_module);

	if (cmd->path == NULL) {	/* is server command */
		sconf->state = (flag ? ENGINE_ENABLED : ENGINE_DISABLED);
	} else
		return "FastRWEngine: only valid in per-server config files";
	return NULL;
}

/*
** +-------------------------------------------------------+
** |                                                       |
** |                     runtime hooks
** |                                                       |
** +-------------------------------------------------------+
*/

/*
**
**  URI-to-filename hook
**
**  [used for the rewriting engine triggered by
**  the per-server 'RewriteRule' directives]
**
*/

/*
 *  Apply a single(!) rewrite rule
 */
inline static int
apply_fastrw(request_rec * r)
{
	char *uri;
	int urilen;
	char *ptr;

	uri = r->filename;

	urilen = strlen(r->filename);

	if (urilen == 1 && *uri == '/') {
		r->filename = "/cgi-bin/www";
		return 1;
	}

	if (urilen < (sizeof (SMAGIC) + 1 > 8 ? 8 : sizeof (SMAGIC)))
		return 0;

	switch (uri[1]) {
	case 'a':
		if (strncmp(uri, "/attach/", 8))
			return 0;
		ptr = strrchr(uri + 8, '/');
		if (!ptr)
			return 0;
		*ptr = ':';
		return 1;
	case FIRST_SMAGIC:
		if (strncmp(uri, "/" SMAGIC, sizeof (SMAGIC)))
			return 0;
		ptr = strchr(uri + sizeof (SMAGIC), '/');
		if (!ptr)
			return 0;
		if (strncmp(ptr, "/attach/", 8)) {
			if (!strncmp
			    (uri, "/" SMAGIC "BIG5", sizeof (SMAGIC) + 4))
				r->filename = "/cgi-bin/wwwbig5";
			else
				r->filename = "/cgi-bin/www";
		} else
			r->filename = "/cgi-bin/attach";
		return 1;
	}
	return 0;
}

static int
hook_uri2file(request_rec * r)
{
	void *sconf;
	fastrw_server_conf *conf;
	const char *var;
	const char *thisserver;
	char *thisport;
	const char *thisurl;
	char buf[512];
	unsigned int port;
	int rulestatus;

	/*
	 *  retrieve the config structures
	 */
	sconf = r->server->module_config;
	conf = (fastrw_server_conf *) ap_get_module_config(sconf,
							   &fastrw_module);

	/*
	 *  only do something under runtime if the engine is really enabled,
	 *  else return immediately!
	 */
	if (conf->state == ENGINE_DISABLED) {
		return DECLINED;
	}

	/*
	 *  check for the ugly API case of a virtual host section where no
	 *  mod_fastrw directives exists. In this situation we became no chance
	 *  by the API to setup our default per-server config so we have to
	 *  on-the-fly assume we have the default config. But because the default
	 *  config has a disabled rewriting engine we are lucky because can
	 *  just stop operating now.
	 */
	if (conf->server != r->server) {
		return DECLINED;
	}

	/*
	 *  add the SCRIPT_URL variable to the env. this is a bit complicated
	 *  due to the fact that apache uses subrequests and internal redirects
	 */

	if (r->main == NULL) {
		var = ap_pstrcat(r->pool, "REDIRECT_", ENVVAR_SCRIPT_URL, NULL);
		var = ap_table_get(r->subprocess_env, var);
		if (var == NULL) {
			ap_table_setn(r->subprocess_env, ENVVAR_SCRIPT_URL,
				      r->uri);
		} else {
			ap_table_setn(r->subprocess_env, ENVVAR_SCRIPT_URL,
				      var);
		}
	} else {
		var = ap_table_get(r->main->subprocess_env, ENVVAR_SCRIPT_URL);
		ap_table_setn(r->subprocess_env, ENVVAR_SCRIPT_URL, var);
	}

	/*
	 *  create the SCRIPT_URI variable for the env
	 */

	/* add the canonical URI of this URL */
	thisserver = ap_get_server_name(r);
	port = ap_get_server_port(r);
	if (ap_is_default_port(port, r)) {
		thisport = "";
	} else {
		ap_snprintf(buf, sizeof (buf), ":%u", port);
		thisport = buf;
	}
	thisurl = ap_table_get(r->subprocess_env, ENVVAR_SCRIPT_URL);

	/* set the variable */
	var =
	    ap_pstrcat(r->pool, ap_http_method(r), "://", thisserver, thisport,
		       thisurl, NULL);
	ap_table_setn(r->subprocess_env, ENVVAR_SCRIPT_URI, var);

	/* if filename was not initially set,
	 * we start with the requested URI
	 */
	if (r->filename == NULL) {
		r->filename = ap_pstrdup(r->pool, r->uri);
	}
	rulestatus = apply_fastrw(r);
	if (rulestatus) {
		r->uri = ap_pstrdup(r->pool, r->filename);
		r->filename =
		    ap_pstrcat(r->pool, "passthrough:", r->filename, NULL);
	}
	return DECLINED;
}
