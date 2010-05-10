#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>

#include <ngx_http_js_module.h>
#include <strings_util.h>
#include <classes/Request.h>

#include <nginx_js_macroses.h>


JSObject *ngx_http_js__nginx_variables__prototype;
JSClass ngx_http_js__nginx_variables__class;
static JSClass* private_class = &ngx_http_js__nginx_variables__class;


JSObject *
ngx_http_js__nginx_variables__wrap(JSContext *cx, ngx_http_request_t *r)
{
	JSObject                  *variables;
	ngx_http_js_ctx_t         *ctx;
	
	TRACE();
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	if (ctx == NULL)
	{
		return NULL;
	}
	
	if (ctx->js_variables != NULL)
	{
		return ctx->js_variables;
	}
	
	variables = JS_NewObject(cx, &ngx_http_js__nginx_variables__class, ngx_http_js__nginx_variables__prototype, NULL);
	if (variables == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return NULL;
	}
	
	if (!JS_SetReservedSlot(cx, ctx->js_request, NGX_JS_REQUEST_SLOT__VARIABLES, OBJECT_TO_JSVAL(variables)))
	{
		JS_ReportError(cx, "can't set slot NGX_JS_REQUEST_SLOT__VARIABLES(%d)", NGX_JS_REQUEST_SLOT__VARIABLES);
		return NULL;
	}
	
	JS_SetPrivate(cx, variables, r);
	
	ctx->js_variables = variables;
	
	return variables;
}


void
ngx_http_js__nginx_variables__cleanup(ngx_http_js_ctx_t *ctx, ngx_http_request_t *r, JSContext *cx)
{
	ngx_assert(ctx);
	
	if (ctx->js_variables == NULL)
	{
		return;
	}
	
	JS_SetPrivate(cx, ctx->js_variables, NULL);
	ctx->js_variables = NULL;
}

static JSBool
constructor(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	TRACE();
	JS_ReportError(cx, "new Nginx.Request.Variables() can be constucted only with request.variables");
	return JS_FALSE;
}


static JSBool
getProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	ngx_str_t                   var;
	ngx_uint_t                  hash;
	ngx_http_variable_value_t  *vv;
	JSString                   *key_jss;
	u_char                     *key, *lowcase;
	size_t                      len;
	
	TRACE();
	GET_PRIVATE(r);
	
	key_jss = JS_ValueToString(cx, id);
	if (key_jss == NULL)
	{
		return JS_FALSE;
	}
	
	key = (u_char *) JS_GetStringBytes(key_jss);
	if (key == NULL)
	{
		JS_ReportError(cx, "can't get the C string of the property name");
		return JS_FALSE;
	}
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "variables[\"%s\"]", key);
	
	len = ngx_strlen(key);
	
	lowcase = ngx_pnalloc(r->pool, len);
	if (lowcase == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	hash = ngx_hash_strlow(lowcase, key, len);
	
	var.len = len;
	var.data = lowcase;
	
	vv = ngx_http_get_variable(r, &var, hash);
	if (vv == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	
	if (vv->not_found)
	{
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	
	NGX_STRING_to_JS_STRING_to_JSVAL(cx, *vv, *vp);
	
	return JS_TRUE;
}

static JSBool
setProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t         *r;
	
	TRACE();
	GET_PRIVATE(r);
	
	if (JSVAL_IS_STRING(id))
	{
		char        *name;
		
		name = JS_GetStringBytes(JSVAL_TO_STRING(id));
		if (name == NULL)
		{
			JS_ReportError(cx, "can't get the C string of the property name");
			return JS_FALSE;
		}
		
		ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "variables[\"%s\"] = ?", name);
		
		// NGX_STRING_to_JS_STRING_to_JSVAL(cx, cookie_value, *vp);
	}
	
	return JS_TRUE;
}


static JSPropertySpec ngx_http_js__nginx_variables__props[] =
{
	// {"length",                 1,          JSPROP_READONLY,   NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


static JSFunctionSpec ngx_http_js__nginx_variables__funcs[] =
{
	// {"empty",       method_empty,          0, 0, 0},
	{0, NULL, 0, 0, 0}
};

JSClass ngx_http_js__nginx_variables__class =
{
	"Variables",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, getProperty, setProperty,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_variables__init(JSContext *cx, JSObject *global)
{
	JSObject    *nginxobj;
	jsval        vp;
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_variables__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_variables__class,  constructor, 0,
		ngx_http_js__nginx_variables__props, ngx_http_js__nginx_variables__funcs,  NULL, NULL);
	E(ngx_http_js__nginx_variables__prototype, "Can`t JS_InitClass(Nginx.Request.Variables)");
	
	return JS_TRUE;
}