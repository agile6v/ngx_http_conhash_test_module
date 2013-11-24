/*
 * Copyright (C) agile6v
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <nginx.h>
#include "ngx_consistent_hash.h"

/*
 *  1 = add             curl -s "http://127.0.0.1/conhash?cmd=1&value=nodeA"
 *  2 = del             curl -s "http://127.0.0.1/conhash?cmd=2&value=nodeA"
 *  3 = search          curl -s "http://127.0.0.1/conhash?cmd=3&value=agile6v"
 *  4 = traverse        curl -s "http://127.0.0.1/conhash?cmd=4"
 *  5 = clear           curl -s "http://127.0.0.1/conhash?cmd=5"
 */

ngx_module_t  ngx_http_conhash_test_module;
 
#define     KEY_STR          "cmd"
#define     VALUE_STR        "value"
#define     BRACKET_L        "("
#define     BRACKET_R        ")"
#define     PROMPT_STR       ") is in the node "
#define     COMMA_STR        ", "

static char *ngx_http_conhash_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_http_conhash_test_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_conhash_test_init_main_conf(ngx_conf_t *cf, void *conf);

static ngx_int_t ngx_http_conhash_test_add(ngx_http_request_t *r, ngx_conhash_t *conhash, ngx_chain_t *out);
static ngx_int_t ngx_http_conhash_test_del(ngx_http_request_t *r, ngx_conhash_t *conhash, ngx_chain_t *out);
static ngx_int_t ngx_http_conhash_test_search(ngx_http_request_t *r, ngx_conhash_t *conhash, ngx_chain_t *out);
static ngx_int_t ngx_http_conhash_test_traverse(ngx_http_request_t *r, ngx_conhash_t *conhash, ngx_chain_t *out);
static ngx_int_t ngx_http_conhash_test_clear(ngx_http_request_t *r, ngx_conhash_t *conhash, ngx_chain_t *out);

static void ngx_http_conhash_test_make_len(ngx_conhash_vnode_t *vnode, void *data);
static void ngx_http_conhash_test_make_data(ngx_conhash_vnode_t *vnode, void *data);

ngx_conhash_ctx_t ngx_http_conhash_test_ctx = {NULL, &ngx_http_conhash_test_module};
    
typedef struct {
    ngx_conhash_t           *conhash;
} ngx_http_conhash_test_main_conf_t;

static ngx_command_t ngx_http_conhash_test_module_commands[] = {

    { ngx_string("conhash_test"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_conhash_test,
      0,
      0,
      NULL },
      
    { ngx_string("conhash_test_zone"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE12,
      ngx_conhash_shm_set_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_conhash_test_main_conf_t, conhash),
      &ngx_http_conhash_test_ctx},

    ngx_null_command
};

static ngx_http_module_t  ngx_http_conhash_test_module_ctx = {
    NULL,                                   /* preconfiguration */
    NULL,                                   /* postconfiguration */
    ngx_http_conhash_test_create_main_conf, /* create main configuration */
    ngx_http_conhash_test_init_main_conf,   /* init main configuration */
    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */
    NULL,                                   /* create location configuration */
    NULL                                    /* merge location configuration */
};

ngx_module_t  ngx_http_conhash_test_module = {
    NGX_MODULE_V1,
    &ngx_http_conhash_test_module_ctx,     /* module context */
    ngx_http_conhash_test_module_commands, /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_conhash_test_handler(ngx_http_request_t *r)
{
    ngx_int_t                            rc, cmd;
    ngx_chain_t                          out;
    ngx_str_t                            value;
    ngx_conhash_t                       *conhash;
    ngx_http_conhash_test_main_conf_t   *ctmcf;
    
    ctmcf = ngx_http_get_module_main_conf(r, ngx_http_conhash_test_module);
    
    conhash = ctmcf->conhash;

    out.buf = NULL;
    out.next = NULL;
    
    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }
    
    if (conhash == NULL) {
        return NGX_DECLINED;
    }
    
    rc = ngx_http_arg(r, (u_char *) KEY_STR, sizeof(KEY_STR) - 1, &value);
    if (rc != NGX_OK) {
        return rc;
    }
    
    cmd = ngx_atoi(value.data, value.len);
    if (cmd < 1 && cmd > 4) {
        return NGX_DECLINED;
    }
    
    switch (cmd) {
        case 1:         //  add
            rc = ngx_http_conhash_test_add(r, conhash, &out);
            break;
        case 2:         //  del
            rc = ngx_http_conhash_test_del(r, conhash, &out);
            break;
        case 3:         //  search
            rc = ngx_http_conhash_test_search(r, conhash, &out);
            break;
        case 4:         //  traverse
            rc = ngx_http_conhash_test_traverse(r, conhash, &out);
            break;
        case 5:         //  clear
            rc = ngx_http_conhash_test_clear(r, conhash, &out);
            break;
    }
    
    if (rc != NGX_OK) {
        return rc;
    }
    
    r->headers_out.status = NGX_HTTP_OK;
    ngx_str_set(&r->headers_out.content_type, "text/plain");
    
    if (out.buf != NULL) {
        r->headers_out.content_length_n = ngx_buf_size(out.buf);
    } else {
        r->header_only = 1;
        r->headers_out.content_length_n = 0;
    }
    
    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }
    
    return ngx_http_output_filter(r, &out);
}

static char *
ngx_http_conhash_test(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;
    
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_conhash_test_handler;

    return NGX_CONF_OK;
}

static void *
ngx_http_conhash_test_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_conhash_test_main_conf_t  *ctmcf;

    ctmcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_conhash_test_main_conf_t));
    if (ctmcf == NULL) {
        return NULL;
    }
    
    ctmcf->conhash = NGX_CONF_UNSET_PTR;
    
    return ctmcf;
}

static char *
ngx_http_conhash_test_init_main_conf(ngx_conf_t *cf, void *conf)
{
    ngx_http_conhash_test_main_conf_t  *ctmcf = conf;
    
    if (ctmcf->conhash == NGX_CONF_UNSET_PTR) {
        ctmcf->conhash = NULL;
    }

    return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_conhash_test_add(ngx_http_request_t *r, ngx_conhash_t *conhash, ngx_chain_t *out)
{
    ngx_str_t                            value;
    ngx_int_t                            rc;
    ngx_buf_t                           *b;
    
    rc = ngx_http_arg(r, (u_char *) VALUE_STR, sizeof(VALUE_STR) - 1, &value);
    if (rc != NGX_OK) {
        return rc;
    }
    
    b = ngx_create_temp_buf(r->pool, 1024);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    rc = ngx_conhash_add_node(conhash, value.data, value.len, NULL);
    if (rc == NGX_OK) {
        b->last = ngx_sprintf(b->last, "Add node successfully!" CRLF);
    }
    
    if (rc == NGX_DECLINED) {
        b->last = ngx_sprintf(b->last, "The node already exists!" CRLF);
        rc = NGX_OK;
    }
    
    if (rc == NGX_AGAIN) {
        b->last = ngx_sprintf(b->last, "The conhash space is not enough!" CRLF);
        rc = NGX_OK;
    }
    
    b->last_buf = 1;
    out->buf = b;
    
    return rc;
}

static ngx_int_t
ngx_http_conhash_test_del(ngx_http_request_t *r, ngx_conhash_t *conhash, ngx_chain_t *out)
{
    ngx_str_t                            value;
    ngx_int_t                            rc;
    ngx_buf_t                           *b;
    
    rc = ngx_http_arg(r, (u_char *) VALUE_STR, sizeof(VALUE_STR) - 1, &value);
    if (rc != NGX_OK) {
        return rc;
    }
    
    b = ngx_create_temp_buf(r->pool, 1024);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    
    rc = ngx_conhash_del_node(conhash, value.data, value.len);
    if (rc == NGX_OK) {
        b->last = ngx_sprintf(b->last, "Delete node successfully!" CRLF);
    }
    
    if (rc == NGX_DECLINED) {
        b->last = ngx_sprintf(b->last, "The node does not exists!" CRLF);
        rc = NGX_OK;
    }
    
    b->last_buf = 1;
    out->buf = b;
    
    return rc;
}

static ngx_int_t
ngx_http_conhash_test_search(ngx_http_request_t *r, ngx_conhash_t *conhash, ngx_chain_t *out)
{
    ngx_str_t                            value;
    ngx_int_t                            rc;
    ngx_conhash_vnode_t                 *vnode;
    size_t                               len;
    ngx_buf_t                           *b;
    ngx_rbtree_key_t                     key;

    rc = ngx_http_arg(r, (u_char *) VALUE_STR, sizeof(VALUE_STR) - 1, &value);
    if (rc != NGX_OK) {
        return rc;
    }
    
    vnode = ngx_conhash_lookup_node(conhash, value.data, value.len);
    if (vnode == NULL) {
        b = ngx_create_temp_buf(r->pool, 1024);
        if (b == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        
        b->last = ngx_sprintf(b->last, "The node tree is empty!" CRLF);
        
        goto done;
    }
    
    len = value.len + sizeof(BRACKET_L) - 1 + NGX_OFF_T_LEN + sizeof(PROMPT_STR) - 1 + 
          ngx_strlen(vnode->hnode->name) + sizeof(BRACKET_L) - 1 + vnode->name.len + 
          sizeof(COMMA_STR) - 1 + NGX_OFF_T_LEN + sizeof(BRACKET_R) - 1 + sizeof(CRLF) - 1;
    
    b = ngx_create_temp_buf(r->pool, len);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    
    key = conhash->hash_func(value.data, value.len);
    
    b->last = ngx_sprintf(b->last, "%V" BRACKET_L "%ui" PROMPT_STR "%s" BRACKET_L "%V" COMMA_STR "%ui"\
                BRACKET_R CRLF, &value, key, vnode->hnode->name, &vnode->name, vnode->node.key);

done:

    b->last_buf = 1;
    out->buf = b;
    
    return NGX_OK;
}

static ngx_int_t
ngx_http_conhash_test_traverse(ngx_http_request_t *r, ngx_conhash_t *conhash, ngx_chain_t *out)
{
    size_t                len;
    ngx_buf_t            *b;
    ngx_int_t             rc;
    
    rc = ngx_conhash_node_traverse(conhash, ngx_http_conhash_test_make_len, &len);
    if (rc == NGX_DECLINED) {
    
        b = ngx_create_temp_buf(r->pool, 1024);
        if (b == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        
        b->last = ngx_sprintf(b->last, "The node tree is empty!" CRLF);
        
        goto done;
    }
    
    b = ngx_create_temp_buf(r->pool, len);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    
    //  NOTE:   Try to finish at a time.
    rc = ngx_conhash_node_traverse(conhash, ngx_http_conhash_test_make_data, b);
    if (rc == NGX_DECLINED) {
    
        b = ngx_create_temp_buf(r->pool, 1024);
        if (b == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        
        b->last = ngx_sprintf(b->last, "The node tree is empty!" CRLF);
    }

done:

    b->last_buf = 1;
    out->buf = b;
    
    return NGX_OK;
}

static ngx_int_t 
ngx_http_conhash_test_clear(ngx_http_request_t *r, ngx_conhash_t *conhash, ngx_chain_t *out)
{
    ngx_conhash_clear(conhash);
    
    return NGX_OK;
}

static void
ngx_http_conhash_test_make_len(ngx_conhash_vnode_t *vnode, void *data)
{
    size_t *len = data;
    
    *len += vnode->name.len + sizeof(BRACKET_L) - 1 + NGX_OFF_T_LEN + 
           sizeof(BRACKET_R) - 1 + sizeof(CRLF) - 1;
}

static void
ngx_http_conhash_test_make_data(ngx_conhash_vnode_t *vnode, void *data)
{
    ngx_buf_t   *b = data;
    
    b->last = ngx_sprintf(b->last, "%V" BRACKET_L "%ui" BRACKET_R CRLF, &vnode->name, vnode->node.key);
}

