#include <assert.h>
#include "endpoints.h"
#include "simperium.h"
#include "simperium_private.h"
#include <stdlib.h>
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

static void
prv_add_auth_header(struct simperium_session *session)
{
    char auth_hdr[MAX_TOKEN_LEN + 20];
    sprintf(auth_hdr, "X-Simperium-Token: %s", session->token);

    // FIXME never freed, so we leak headers every time we call
    struct curl_slist *headers = NULL;
    // XXX should probably add other headers (e.g. content-type)
    headers = curl_slist_append(headers, auth_hdr);
    curl_easy_setopt(session->app->curl, CURLOPT_HTTPHEADER, headers);
}


struct simperium_bucket *
simperium_bucket_open(struct simperium_session *session, const char *bucket_name)
{
    assert(session != NULL);

    struct simperium_bucket *bucket = calloc(sizeof(struct simperium_bucket), 1);
    assert(bucket != NULL);

    // XXX client_id ?
    bucket->session = session;
    strncpy(bucket->name, bucket_name, MAX_BKT_NAME_LEN);

    return bucket;
}

void
simperium_bucket_close(struct simperium_bucket *bucket)
{
    free(bucket);
}

int
simperium_bucket_add_item(struct simperium_bucket *bucket, struct simperium_item *item)
{
    // XXX if item key is null generate one
    struct simperium_app *app = bucket->session->app;
    char url[MAX_URL_LEN] = {0};
    sprintf(url, "%s/%s/%s/%s/%s", HOST_API,
                                   app->name,
                                   bucket->name,
                                   ENDPOINT_ITEM,
                                   item->id);

    prv_reset_curl(app->curl);
    prv_add_auth_header(bucket->session);
    curl_easy_setopt(app->curl, CURLOPT_URL, url);
    curl_easy_setopt(app->curl, CURLOPT_POSTFIELDS, item->data);

    CURLcode res = curl_easy_perform(app->curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        return -1;
    } else {
        return 0;
    }
}

int
simperium_bucket_get_item(struct simperium_bucket *bucket, struct simperium_item *item)
{
    struct simperium_app *app = bucket->session->app;
    char url[MAX_URL_LEN] = {0};
    sprintf(url, "%s/%s/%s/%s/%s", HOST_API,
                                   app->name,
                                   bucket->name,
                                   ENDPOINT_ITEM,
                                   item->id);

    prv_reset_curl(app->curl);
    prv_add_auth_header(bucket->session);
    curl_easy_setopt(app->curl, CURLOPT_URL, url);

    // Set callback to handle response data
    struct response_data resp_data;
    resp_data.bytes_written = 0;
    resp_data.buffer = malloc(1);
    curl_easy_setopt(app->curl, CURLOPT_WRITEFUNCTION, prv_auth_callback);
    curl_easy_setopt(app->curl, CURLOPT_WRITEDATA, &resp_data);

    CURLcode res = curl_easy_perform(app->curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        free(resp_data.buffer);
    }

    item->data = resp_data.buffer;
}

int
simperium_bucket_remove_item(struct simperium_bucket *bucket, struct simperium_item *item)
{
    struct simperium_app *app = bucket->session->app;
    char url[MAX_URL_LEN] = {0};
    sprintf(url, "%s/%s/%s/%s/%s", HOST_API,
                                   app->name,
                                   bucket->name,
                                   ENDPOINT_ITEM,
                                   item->id);

    prv_reset_curl(app->curl);
    prv_add_auth_header(bucket->session);
    curl_easy_setopt(app->curl, CURLOPT_URL, url);
    curl_easy_setopt(app->curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    CURLcode res = curl_easy_perform(app->curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        return -1;
    } else {
        return 0;
    }
}

