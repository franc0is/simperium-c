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
prv_response_callback(void *contents, size_t size, size_t nmemb, void *userp)
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
    curl_easy_setopt(app->curl, CURLOPT_POSTFIELDS, json_dumps(item->json_data, JSON_ENCODE_ANY));

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
simperium_bucket_get_item(struct simperium_bucket *bucket, const char *id, simperium_item_callback cb)
{
    int rv = 0;
    struct simperium_app *app = bucket->session->app;
    char url[MAX_URL_LEN] = {0};
    sprintf(url, "%s/%s/%s/%s/%s", HOST_API,
                                   app->name,
                                   bucket->name,
                                   ENDPOINT_ITEM,
                                   id);

    prv_reset_curl(app->curl);
    prv_add_auth_header(bucket->session);
    curl_easy_setopt(app->curl, CURLOPT_URL, url);

    // Set callback to handle response data
    struct response_data resp_data;
    resp_data.bytes_written = 0;
    resp_data.buffer = malloc(1);
    curl_easy_setopt(app->curl, CURLOPT_WRITEFUNCTION, prv_response_callback);
    curl_easy_setopt(app->curl, CURLOPT_WRITEDATA, &resp_data);

    CURLcode res = curl_easy_perform(app->curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        rv = -1;
        goto error;
    }

    json_t *resp_json = json_loads(resp_data.buffer, JSON_DECODE_ANY, NULL);
    if (resp_json == NULL) {
        fprintf(stderr, "malformed JSON, received: %s\n",
                resp_data.buffer);
        rv = -1;
        goto error;
    }

    if (cb) {
        struct simperium_item item = {0};
        strncpy(item.id, id, MAX_ITEM_ID_LEN);
        item.json_data = resp_json;
        cb(&item);
    }

    json_decref(resp_json);

error:
    free(resp_data.buffer);
    return rv;
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

int
simperium_bucket_all_items(struct simperium_bucket *bucket, simperium_item_callback cb)
{
    struct simperium_app *app = bucket->session->app;
    char url[MAX_URL_LEN] = {0};
    char *mark = NULL;
    int rv = 0;
    struct response_data resp_data;
    json_t *resp_json = NULL;

    resp_data.buffer = malloc(1);

    do {
        // XXX more elegant way to add query params
        sprintf(url, "%s/%s/%s/%s?data=1", HOST_API,
                                           app->name,
                                           bucket->name,
                                           ENDPOINT_INDEX);

        prv_reset_curl(app->curl);
        prv_add_auth_header(bucket->session);
        curl_easy_setopt(app->curl, CURLOPT_URL, url);

        // Set callback to handle response data
        resp_data.bytes_written = 0;
        curl_easy_setopt(app->curl, CURLOPT_WRITEFUNCTION, prv_response_callback);
        curl_easy_setopt(app->curl, CURLOPT_WRITEDATA, &resp_data);

        CURLcode res = curl_easy_perform(app->curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
            rv = -1;
            break;
        }

        // Extract token from response
        resp_json = json_loads(resp_data.buffer, JSON_DECODE_ANY, NULL);
        if (resp_json == NULL) {
            fprintf(stderr, "malformed JSON, received: %s\n",
                    resp_data.buffer);
            rv = -1;
            break;
        }

        if (!json_is_object(resp_json)) {
            fprintf(stderr, "no JSON object found, received %s\n",
                    resp_data.buffer);
            rv = -1;
            break;
        }

        const json_t *j = NULL;
        if ((j = json_object_get(resp_json, "index")) == NULL || !json_is_array(j)) {
            fprintf(stderr, "no index member found, received %s\n",
                    resp_data.buffer);
            rv = -1;
            break;
        }

        if ((j = json_object_get(resp_json, "mark")) != NULL) {
            // XXX this will infinite loop rn
            // mark = json_value(j);
        }
    } while (mark != NULL);

    if (resp_json) {
        json_decref(resp_json);
    }
    if (resp_data.buffer) {
        free(resp_data.buffer);
    }
    return 0;
}
