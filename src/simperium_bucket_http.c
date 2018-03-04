#include <assert.h>
#include "simperium.h"
#include "simperium_private.h"
#include "simperium_endpoints.h"
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

static enum simperium_op
prv_op_from_string(const char *s)
{
    switch (s[0]) {
        case '+':
            return SIMPERIUM_OP_INSERT;
        case '-':
            return SIMPERIUM_OP_DELETE;
        case 'R':
            return SIMPERIUM_OP_REPLACE;
        case 'I':
            return SIMPERIUM_OP_INCREMENT;
        case 'L':
            return SIMPERIUM_OP_LISTDIFF;
        case 'O':
            return SIMPERIUM_OP_OBJDIFF;
        case 'd':
            return SIMPERIUM_OP_STRDIFF;
    }

    return SIMPERIUM_OP_INVALID;
}

struct simperium_bucket *
simperium_bucket_open_http(struct simperium_session *session, const char *bucket_name)
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
simperium_bucket_close_http(struct simperium_bucket *bucket)
{
    free(bucket);
}

int
simperium_bucket_add_item_http(struct simperium_bucket *bucket, struct simperium_item *item)
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
simperium_bucket_get_item_http(struct simperium_bucket *bucket,
                               const char *id,
                               simperium_item_callback cb,
                               void *cb_data)
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
        cb(&item, cb_data);
    }

    json_decref(resp_json);

error:
    free(resp_data.buffer);
    return rv;
}

int
simperium_bucket_remove_item_http(struct simperium_bucket *bucket, struct simperium_item *item)
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
simperium_bucket_all_items_http(struct simperium_bucket *bucket,
                                const char **cursor,
                                simperium_item_callback cb,
                                void *cb_data)
{
    struct simperium_app *app = bucket->session->app;
    char url[MAX_URL_LEN] = {0};
    const char *mark = NULL;
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

        if (cursor && *cursor) {
            strcat(url, "&since=");
            strcat(url, *cursor);
        }
        if (mark) {
            strcat(url, "&mark=");
            strcat(url, mark);
        }

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
            goto error;
        }

        // XXX JSON memory management

        // Extract token from response
        resp_json = json_loads(resp_data.buffer, JSON_DECODE_ANY, NULL);
        if (resp_json == NULL) {
            fprintf(stderr, "malformed JSON, received: %s\n",
                    resp_data.buffer);
            rv = -1;
            goto error;
        }

        if (!json_is_object(resp_json)) {
            fprintf(stderr, "no JSON object found, received %s\n",
                    resp_data.buffer);
            rv = -1;
            goto error;
        }

        const json_t *j = NULL;
        if ((j = json_object_get(resp_json, "index")) == NULL || !json_is_array(j)) {
            fprintf(stderr, "no index member found, received %s\n",
                    resp_data.buffer);
            rv = -1;
            goto error;
        }

        size_t index;
        json_t *value;
        json_array_foreach(j, index, value) {
            const json_t *id_json;
            json_t *data_json;
            if ((id_json = json_object_get(value, "id")) == NULL ||
                !json_is_string(id_json) ||
                (data_json = json_object_get(value, "d")) == NULL ||
                !json_is_object(data_json)) {
                fprintf(stderr, "cannot parse items from response %s\n",
                        resp_data.buffer);
                rv = -1;
                goto error;
            }

            struct simperium_item item = {0};
            strncpy(item.id, json_string_value(id_json), MAX_ITEM_ID_LEN);
            item.json_data = data_json;
            if (cb) {
                cb(&item, cb_data);
            }
        }

        if ((j = json_object_get(resp_json, "mark")) != NULL &&
            json_is_string(j)) {
            mark = json_string_value(j);
        } else {
            mark = NULL;
        }

        if (cursor &&
            (j = json_object_get(resp_json, "current")) != NULL &&
            json_is_string(j)) {
            const char *v = json_string_value(j);
            int l = strlen(v);
            *cursor = calloc(l + 1, 1);
            strcpy(*cursor, v);
        }
    } while (mark != NULL);

error:
    if (resp_json) {
        json_decref(resp_json);
    }
    if (resp_data.buffer) {
        free(resp_data.buffer);
    }
    return 0;
}

int
simperium_bucket_get_changes_http(struct simperium_bucket *bucket,
                                  const char **cursor,
                                  simperium_change_callback cb,
                                  void *cb_data)
{
    struct simperium_app *app = bucket->session->app;
    char url[MAX_URL_LEN] = {0};
    const char *mark = NULL;
    int rv = 0;
    json_t *resp_json = NULL;


    // XXX more elegant way to add query params
    sprintf(url, "%s/%s/%s/%s?", HOST_API,
                                app->name,
                                bucket->name,
                                ENDPOINT_CHANGES);

    if (cursor && *cursor) {
        strcat(url, "cv=");
        strcat(url, *cursor);
    }

    prv_reset_curl(app->curl);
    prv_add_auth_header(bucket->session);
    curl_easy_setopt(app->curl, CURLOPT_URL, url);

    // Set callback to handle response data
    struct response_data resp_data = {0};
    resp_data.buffer = malloc(1);
    resp_data.bytes_written = 0;
    curl_easy_setopt(app->curl, CURLOPT_WRITEFUNCTION, prv_response_callback);
    curl_easy_setopt(app->curl, CURLOPT_WRITEDATA, &resp_data);
    // XXX make configurable
    curl_easy_setopt(app->curl, CURLOPT_TIMEOUT, 2L);

    CURLcode res = curl_easy_perform(app->curl);
    if (res == CURLE_OPERATION_TIMEDOUT) {
        goto timeout;
    } else if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        rv = -1;
        goto error;
    }

    // Extract token from response
    resp_json = json_loads(resp_data.buffer, JSON_DECODE_ANY, NULL);
    if (resp_json == NULL) {
        fprintf(stderr, "malformed JSON, received: %s\n",
                resp_data.buffer);
        rv = -1;
        goto error;
    }

    if (!json_is_array(resp_json)) {
        fprintf(stderr, "no JSON array found, received %s\n",
                resp_data.buffer);
        rv = -1;
        goto error;
    }

    size_t index;
    json_t *value;
    json_t *cursor_json = NULL;
    json_array_foreach(resp_json, index, value) {
        const json_t *id_json;
        json_t *op_json;
        json_t *data_json;
        if ((id_json = json_object_get(value, "id")) == NULL ||
            !json_is_string(id_json) ||
            (op_json = json_object_get(value, "o")) == NULL ||
            !json_is_string(op_json)) {
            fprintf(stderr, "cannot parse items from response %s\n",
                    resp_data.buffer);
            rv = -1;
            goto error;
        }

        struct simperium_change change = {0};
        strncpy(change.id, json_string_value(id_json), MAX_ITEM_ID_LEN);
        change.json_data = json_object_get(value, "v");
        change.operation = prv_op_from_string(json_string_value(op_json));
        if (cb) {
            cb(&change, cb_data);
        }

        cursor_json = json_object_get(value, "cv");
    }

    if (cursor &&
        cursor_json &&
        json_is_string(cursor_json)) {
        // XXX allocate
        const char *v = json_string_value(cursor_json);
        strcpy(*cursor, v);
    }

error:
    if (resp_json) {
        json_decref(resp_json);
    }
    if (resp_data.buffer) {
        free(resp_data.buffer);
    }

timeout:
    return rv;
}
