#include <assert.h>
#include <jansson.h>
#include "simperium.h"
#include "simperium_private.h"
#include "simperium_endpoints.h"
#include <string.h>

// FIXME duplicated
static void
prv_reset_curl(CURL *curl)
{
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
#ifdef DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
}

// FIXME duplicated
static size_t
prv_auth_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t size_bytes = size * nmemb;
    struct response_data *rd = (struct response_data *)userp;

    rd->buffer = realloc(rd->buffer, rd->bytes_written + size_bytes + 1);
    assert(rd->buffer != NULL);

    memcpy(&rd->buffer[rd->bytes_written], contents, size_bytes);
    rd->bytes_written += size_bytes;
    rd->buffer[rd->bytes_written] = 0;

    return size_bytes;
}

// Public API
struct simperium_app *
simperium_app_init(const char *app_name, const char *api_key)
{
    struct simperium_app *app = calloc(sizeof(struct simperium_app), 1);
    assert(app != NULL);

    curl_global_init(CURL_GLOBAL_SSL);
    CURL *curl = curl_easy_init();
    if (!curl) {
        goto error;
    }

    if (strnlen(app_name, MAX_APP_NAME_LEN) == MAX_APP_NAME_LEN ||
        strnlen(api_key, MAX_API_KEY_LEN) == MAX_API_KEY_LEN) {
        goto error;
    }

    strncpy(app->name, app_name, MAX_APP_NAME_LEN);
    strncpy(app->api_key, api_key, MAX_API_KEY_LEN);
    app->curl = curl;

    return app;

error:
    curl_global_cleanup();
    free(app);
    return NULL;
}

void
simperium_app_deinit(struct simperium_app *app)
{
    if (!app || !app->curl) {
        return;
    }

    curl_easy_cleanup(app->curl);
    curl_global_cleanup();
    free(app);
}

struct simperium_session *
simperium_session_open(struct simperium_app *app, const char *user, const char *passwd)
{
    assert(app != NULL);

    struct simperium_session *session = calloc(sizeof(struct simperium_session), 1);
    assert(session != NULL);

    session->app = app;

    prv_reset_curl(app->curl);

    // Set URL
    char url[MAX_URL_LEN] = {0};
    sprintf(url, "%s/%s/%s/", HOST_AUTH, app->name, ENDPOINT_AUTH);
    curl_easy_setopt(app->curl, CURLOPT_URL, url);

    // Set POST data
    json_t *req_json = json_pack("{s:s, s:s, s:s}",
                                  "client_id", app->api_key,
                                  "username", user,
                                  "password", passwd);
    assert(req_json != NULL);
    char *req_data = json_dumps(req_json, JSON_ENCODE_ANY);
    curl_easy_setopt(app->curl, CURLOPT_POSTFIELDS, req_data);
    json_decref(req_json);

    // Set callback to handle response data
    struct response_data resp_data;
    resp_data.bytes_written = 0;
    resp_data.buffer = malloc(1);
    curl_easy_setopt(app->curl, CURLOPT_WRITEFUNCTION, prv_auth_callback);
    curl_easy_setopt(app->curl, CURLOPT_WRITEDATA, &resp_data);

    // Run request
    CURLcode res = curl_easy_perform(app->curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        goto error;
    }

    // Extract token from response
    json_t *resp_json = json_loads(resp_data.buffer, JSON_DECODE_ANY, NULL);
    if (resp_json == NULL) {
        fprintf(stderr, "malformed JSON, received: %s\n",
                resp_data.buffer);
        goto error;
    }
    const char *u, *tk, *id;
    int result = json_unpack(resp_json, "{s:s, s:s, s:s}",
                                        "username", &u,
                                        "access_token", &tk,
                                        "userid", &id);
    if (result != 0) {
        fprintf(stderr, "No token found in: %s\n",
                resp_data.buffer);
    }
    strcpy(session->token, tk);
    json_decref(resp_json);

error:
    free(req_data);
    free(resp_data.buffer);
    return session;
}

void
simperium_session_close(struct simperium_session *session)
{
    free(session);
}

