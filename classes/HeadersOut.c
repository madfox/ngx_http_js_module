
// Nginx.Headers class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>
#include <assert.h>

#include "../ngx_http_js_module.h"
#include "../strings_util.h"
#include "Request.h"
// #include "HeadersIn.h"

#include "../macroses.h"

//#define unless(a) if(!(a))
#define JS_HEADER_IN_ROOT_NAME      "Nginx.HeadersIn instance"


JSObject *ngx_http_js__nginx_headers_out__prototype;
JSClass ngx_http_js__nginx_headers_out__class;


JSObject *
ngx_http_js__nginx_headers_out__wrap(JSContext *cx, ngx_http_request_t *r)
{
	LOG2("ngx_http_js__nginx_headers_out__wrap()");
	JSObject                  *headers;
	ngx_http_js_ctx_t         *ctx;
	
	if (!(ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)))
		ngx_http_js__nginx_request__wrap(cx, r);
	
	if (ctx->js_headers)
		return ctx->js_headers;
	
	headers = JS_NewObject(cx, &ngx_http_js__nginx_headers_out__class, ngx_http_js__nginx_headers_out__prototype, NULL);
	if (!headers)
	{
		JS_ReportOutOfMemory(cx);
		return NULL;
	}
	
	if (!JS_AddNamedRoot(cx, &ctx->js_headers, JS_HEADER_IN_ROOT_NAME))
	{
		JS_ReportError(cx, "Can`t add new root %s", JS_HEADER_IN_ROOT_NAME);
		return NULL;
	}
	
	JS_SetPrivate(cx, headers, r);
	
	ctx->js_headers = headers;
	
	return headers;
}


void
ngx_http_js__nginx_headers_out__cleanup(JSContext *cx, ngx_http_request_t *r, ngx_http_js_ctx_t *ctx)
{
	LOG2("ngx_http_js__nginx_headers_out__wrap()");
	
	assert(ctx);
	
	if (!ctx->js_headers)
		return;
	
	if (!JS_RemoveRoot(cx, &ctx->js_headers))
		JS_ReportError(cx, "Can`t remove cleaned up root %s", JS_HEADER_IN_ROOT_NAME);
	
	JS_SetPrivate(cx, ctx->js_headers, NULL);
	ctx->js_headers = NULL;
}


static JSBool
method_empty(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	LOG2("Nginx.Headers#empty");
	ngx_http_request_t  *r;
	
	GET_PRIVATE();
	
	E(argc == 1 && JSVAL_IS_STRING(argv[0]), "Nginx.Headers#empty takes 1 argument: str:String");
	
	return JS_TRUE;
}



static JSBool
constructor(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}


// enum propid { HEADER_LENGTH };

static JSBool
getProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	char                       *name;
	ngx_http_core_main_conf_t  *cmcf;
	ngx_list_part_t            *part;
	ngx_http_header_t          *hh;
	ngx_table_elt_t            **ph, *h;
	u_char                     *lowcase_key;//, *cookie
	ngx_uint_t                  i, hash; // n, 
	u_int                       len;
	
	GET_PRIVATE();
	
	if (JSVAL_IS_INT(id))
	{
		switch (JSVAL_TO_INT(id))
		{
			// case REQUEST_URI:
			// *vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) r->uri.data, r->uri.len));
			// break;
			
			
		}
	}
	else if (JSVAL_IS_STRING(id) && (name = JS_GetStringBytes(JSVAL_TO_STRING(id))) != NULL)
	{
		// if (!strcmp(member_name, "constructor"))
		len = strlen(name);
		LOG("getProperty: %s, len: %u", name, len);
		
		
		// look in hashed headers
		
		lowcase_key = ngx_palloc(r->pool, len);
		if (lowcase_key == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return JS_FALSE;
		}
		
		hash = 0;
		for (i = 0; i < len; i++)
		{
			lowcase_key[i] = ngx_tolower(name[i]);
			hash = ngx_hash(hash, lowcase_key[i]);
		}
		
		
		cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
		
		hh = ngx_hash_find(&cmcf->headers_in_hash, hash, lowcase_key, len);
		
		if (hh)
		{
			if (hh->offset)
			{
				ph = (ngx_table_elt_t **) ((char *) &r->headers_in + hh->offset);
				
				if (*ph)
					*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) (*ph)->value.data, (*ph)->value.len));
				
				return JS_TRUE;
			}
		}
		
		
		// look in all headers
		
		part = &r->headers_in.headers.part;
		h = part->elts;
		
		for (i = 0; /* void */ ; i++)
		{
			if (i >= part->nelts)
			{
				if (part->next == NULL)
					break;
				
				part = part->next;
				h = part->elts;
				i = 0;
			}
			
			if (len != h[i].key.len || ngx_strcasecmp((u_char *) name, h[i].key.data) != 0)
				continue;
			
			*vp = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char *) h[i].value.data, h[i].value.len));
			
			return JS_TRUE;
		}
	}
	
	return JS_TRUE;
}

static JSBool
setProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	ngx_table_elt_t            *header;
	JSString                   *value;
	
	GET_PRIVATE();
	
	E(JSVAL_IS_STRING(id), "Nginx.Request#[]= takes a key:String and a value of a key relational type");
	
	
	LOG("setProperty: %s", JS_GetStringBytes(JSVAL_TO_STRING(id)));
	
	header = ngx_list_push(&r->headers_out.headers);
	E(header, "Can`t ngx_list_push()");
	
	header->hash = 1;
	
	E(value = JS_ValueToString(cx, *vp), "Can`t JS_ValueToString()");
	
	E(js_str2ngx_str(cx, JSVAL_TO_STRING(id), r->pool, &header->key, 0), "Can`t js_str2ngx_str(key)");
	E(js_str2ngx_str(cx, value,               r->pool, &header->value, 0), "Can`t js_str2ngx_str(value)");
	
	if (header->key.len == sizeof("Content-Length") - 1
		&& ngx_strncasecmp(header->key.data, (u_char *)"Content-Length", sizeof("Content-Length") - 1) == 0)
	{
		E(JSVAL_IS_INT(*vp), "the Content-Length value must be an Integer");
		r->headers_out.content_length_n = (off_t) JSVAL_TO_INT(*vp);
		r->headers_out.content_length = header;
	}
	
	return JS_TRUE;
}

static JSBool
delProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	return JS_TRUE;
}

JSPropertySpec ngx_http_js__nginx_headers_out__props[] =
{
	// {"uri",      REQUEST_URI,          JSPROP_READONLY,   NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


JSFunctionSpec ngx_http_js__nginx_headers_out__funcs[] = {
    // {"empty",       method_empty,          1, 0, 0},
    {0, NULL, 0, 0, 0}
};

JSClass ngx_http_js__nginx_headers_out__class =
{
	"Headers",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, delProperty, getProperty, setProperty,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_headers_out__init(JSContext *cx)
{
	JSObject    *nginxobj;
	JSObject    *global;
	jsval        vp;
	
	global = JS_GetGlobalObject(cx);
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined or is not a function");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_headers_out__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_headers_out__class,  constructor, 0,
		ngx_http_js__nginx_headers_out__props, ngx_http_js__nginx_headers_out__funcs,  NULL, NULL);
	E(ngx_http_js__nginx_headers_out__prototype, "Can`t JS_InitClass(Nginx.Headers)");
	
	return JS_TRUE;
}



// check this from time to time
// http://emiller.info/lxr/http/source/http/ngx_http_request.h#L252
// 
// typedef struct
// {
//     ngx_list_t                        headers;
//     
//     ngx_uint_t                        status;
//     ngx_str_t                         status_line;
//     
//     ngx_table_elt_t                  *server;
//     ngx_table_elt_t                  *date;
//     ngx_table_elt_t                  *content_length;
//     ngx_table_elt_t                  *content_encoding;
//     ngx_table_elt_t                  *location;
//     ngx_table_elt_t                  *refresh;
//     ngx_table_elt_t                  *last_modified;
//     ngx_table_elt_t                  *content_range;
//     ngx_table_elt_t                  *accept_ranges;
//     ngx_table_elt_t                  *www_authenticate;
//     ngx_table_elt_t                  *expires;
//     ngx_table_elt_t                  *etag;
//     
//     ngx_str_t                        *override_charset;
//     
//     size_t                            content_type_len;
//     ngx_str_t                         content_type;
//     ngx_str_t                         charset;
//     
//     ngx_array_t                       cache_control;
//     
//     off_t                             content_length_n;
//     time_t                            date_time;
//     time_t                            last_modified_time;
// }
// ngx_http_headers_out_t;

