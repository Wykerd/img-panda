#include "img-panda/parsers/url_parser.h"
#include "img-panda/parsers/url.h"
#include <string.h>
#include <stdlib.h>

#define url_field_len(x) u.field_data[x].len
#define url_field_off(x) u.field_data[x].off

imp_url_t *imp_parse_url (const char* buf, size_t buflen) {
    struct http_parser_url u;
    http_parser_url_init(&u);
    if (http_parser_parse_url(buf, buflen, 0, &u)) {
        return NULL;
    };

    imp_url_t *url;
    url = malloc(sizeof(imp_url_t));
    if (!url) {
        return NULL;
    }
    url->ref = 1;

    // --- SCHEMA

    url->schema = malloc(sizeof(char) * (url_field_len(UF_SCHEMA) + 1)); 

    if (!url->schema) {
        free(url);
        return NULL;
    };

    memcpy(url->schema, buf + url_field_off(UF_SCHEMA), url_field_len(UF_SCHEMA)); 
    url->schema[url_field_len(UF_SCHEMA)] = 0;

    // --- HOST

    url->host = malloc(sizeof(char) * (url_field_len(UF_HOST) + 1)); 
    
    if (!url->host) {
        free(url->schema);
        free(url);
        return NULL;
    };
    
    memcpy(url->host, buf + url_field_off(UF_HOST), url_field_len(UF_HOST)); 
    url->host[url_field_len(UF_HOST)] = 0;

    // --- QUERY

    url->query = malloc(sizeof(char) * (url_field_len(UF_QUERY) + 1)); 
    
    if (!url->query) {
        free(url->host);
        free(url->schema);
        free(url);
        return NULL;
    };

    memcpy(url->query, buf + url_field_off(UF_QUERY), url_field_len(UF_QUERY)); 
    url->query[url_field_len(UF_QUERY)] = 0;

    // --- USERINFO

    url->userinfo = malloc(sizeof(char) * (url_field_len(UF_USERINFO) + 1)); 
    
    if (!url->userinfo) {
        free(url->query);
        free(url->host);
        free(url->schema);
        free(url);
        return NULL;
    };

    memcpy(url->userinfo, buf + url_field_off(UF_USERINFO), url_field_len(UF_USERINFO)); 
    url->userinfo[url_field_len(UF_USERINFO)] = 0;

    // --- FRAGMENT

    url->fragment = malloc(sizeof(char) * (url_field_len(UF_FRAGMENT) + 1)); 
    
    if (!url->fragment) {
        free(url->userinfo);
        free(url->query);
        free(url->host);
        free(url->schema);
        free(url);
        return NULL;
    };

    memcpy(url->fragment, buf + url_field_off(UF_FRAGMENT), url_field_len(UF_FRAGMENT)); 
    url->fragment[url_field_len(UF_FRAGMENT)] = 0;

    // --- PORT

    if (!url_field_len(UF_PORT)) {
        url->port = calloc(sizeof(char), 6);
        if (!url->port) {
            goto fail_port;
        }

        if (!strcmp(url->schema, "http")) {
            strcpy(url->port, "80");
        } else if (!strcmp(url->schema, "https")) {
            strcpy(url->port, "443");
        } else {
            url->port[0] = '0';
        }
    } else {
        url->port = malloc(sizeof(char) * (url_field_len(UF_PORT) + 1)); 
    
        if (!url->port) {
fail_port:
            free(url->fragment);
            free(url->userinfo);
            free(url->query);
            free(url->host);
            free(url->schema);
            free(url);
            return NULL;
        };

        memcpy(url->port, buf + url_field_off(UF_PORT), url_field_len(UF_PORT)); 
        url->port[url_field_len(UF_PORT)] = 0;
    }

    // --- PATH

    if (!u.field_data[UF_PATH].len) {
        url->path = calloc(sizeof(char), 2);
        
        if (!url->path) {
            goto fail_path;
        };

        url->path[0] = '/';
    } else {

        url->path = malloc(sizeof(char) * (url_field_len(UF_PATH) + 1)); 
    
        if (!url->path) {
fail_path:
            free(url->port);
            free(url->fragment);
            free(url->userinfo);
            free(url->query);
            free(url->host);
            free(url->schema);
            free(url);
            return NULL;
        };

        memcpy(url->path, buf + url_field_off(UF_PATH), url_field_len(UF_PATH)); 
        url->path[url_field_len(UF_PATH)] = 0;
    };

    return url;
}

#undef url_field_len
#undef url_field_off

imp_url_t *imp_url_clone (imp_url_t *url) {
    imp_url_t *uri = malloc(sizeof(imp_url_t));
    uri->fragment = strdup(url->fragment);
    uri->host = strdup(url->host);
    uri->path = strdup(url->path);
    uri->port = strdup(url->port);
    uri->query = strdup(url->query);
    uri->schema = strdup(url->schema);
    uri->userinfo = strdup(url->userinfo);
    uri->ref = 1;
    return uri;
}

imp_url_t *imp_url_dup (imp_url_t *url) {
    url->ref++;
    return url;
}

void imp_url_free (imp_url_t *url) {
    url->ref--;
    if (url->ref == 0) {
        free(url->schema);
        free(url->host);
        free(url->port);
        free(url->path);
        free(url->query);
        free(url->fragment);
        free(url->userinfo);
        free(url);
        url = NULL;
    }
};
