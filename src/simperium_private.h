#pragma once

#include <curl/curl.h>

#define MAX_TOKEN_LEN 64

struct response_data {
    size_t bytes_written;
    char *buffer;
};

struct simperium_app {
    char name[MAX_APP_NAME_LEN];
    char api_key[MAX_API_KEY_LEN];
    CURL *curl;
};

struct simperium_session {
    struct simperium_app *app;
    char token[MAX_TOKEN_LEN];
};

struct simperium_bucket {
    struct simperium_session *session;
    char name[MAX_BKT_NAME_LEN];
};

