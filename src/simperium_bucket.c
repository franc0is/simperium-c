#include <assert.h>
#include "simperium.h"
#include "simperium_private.h"
#include <stdlib.h>
#include <string.h>

struct simperium_bucket *
simperium_bucket_open_ws(struct simperium_session *session, const char *bucket_name);

void
simperium_bucket_close_ws(struct simperium_bucket *bucket);

int
simperium_bucket_add_item_ws(struct simperium_bucket *bucket, struct simperium_item *item);

int
simperium_bucket_get_item_ws(struct simperium_bucket *bucket,
                             const char *id,
                             simperium_item_callback cb,
                             void *cb_data);

int
simperium_bucket_remove_item_ws(struct simperium_bucket *bucket, struct simperium_item *item);

int
simperium_bucket_all_items_ws(struct simperium_bucket *bucket,
                              const char **cursor,
                              simperium_item_callback cb,
                              void *cb_data);

int
simperium_bucket_get_changes_ws(struct simperium_bucket *bucket,
                                const char **cursor,
                                simperium_change_callback cb,
                                void *cb_data);

struct simperium_bucket *
simperium_bucket_open_http(struct simperium_session *session, const char *bucket_name);

void
simperium_bucket_close_http(struct simperium_bucket *bucket);

int
simperium_bucket_add_item_http(struct simperium_bucket *bucket, struct simperium_item *item);


int
simperium_bucket_get_item_http(struct simperium_bucket *bucket,
                               const char *id,
                               simperium_item_callback cb,
                               void *cb_data);

int
simperium_bucket_remove_item_http(struct simperium_bucket *bucket, struct simperium_item *item);

int
simperium_bucket_all_items_http(struct simperium_bucket *bucket,
                                const char **cursor,
                                simperium_item_callback cb,
                                void *cb_data);

int
simperium_bucket_get_changes_http(struct simperium_bucket *bucket,
                                  const char **cursor,
                                  simperium_change_callback cb,
                                  void *cb_data);

#define PROTOCOL_CALL(protocol, function, ...)  \
do {                                            \
    if (protocol == SIMPERIUM_PROTOCOL_HTTP) {  \
        return function##_http(__VA_ARGS__);    \
    } else {                                    \
        return function##_ws(__VA_ARGS__);      \
    }                                           \
} while (0)

struct simperium_bucket *
simperium_bucket_open(struct simperium_session *session, const char *bucket_name)
{
    PROTOCOL_CALL(session->protocol, simperium_bucket_open, session, bucket_name);
}

void
simperium_bucket_close(struct simperium_bucket *bucket)
{
    PROTOCOL_CALL(bucket->session->protocol, simperium_bucket_close, bucket);
}

int
simperium_bucket_add_item(struct simperium_bucket *bucket, struct simperium_item *item)
{
    PROTOCOL_CALL(bucket->session->protocol, simperium_bucket_add_item, bucket, item);
}

int
simperium_bucket_get_item(struct simperium_bucket *bucket,
                          const char *id, simperium_item_callback cb,
                          void *cb_data)
{
    PROTOCOL_CALL(bucket->session->protocol, simperium_bucket_get_item, bucket, id, cb, cb_data);
}

int
simperium_bucket_remove_item(struct simperium_bucket *bucket, struct simperium_item *item)
{
    PROTOCOL_CALL(bucket->session->protocol, simperium_bucket_remove_item, bucket, item);
}

int
simperium_bucket_all_items(struct simperium_bucket *bucket,
                           const char **cursor,
                           simperium_item_callback cb,
                           void *cb_data)
{
    PROTOCOL_CALL(bucket->session->protocol, simperium_bucket_all_items, bucket, cursor, cb, cb_data);
}

int
simperium_bucket_get_changes(struct simperium_bucket *bucket,
                             const char **cursor,
                             simperium_change_callback cb,
                             void *cb_data)
{
    PROTOCOL_CALL(bucket->session->protocol, simperium_bucket_get_changes, bucket, cursor, cb, cb_data);
}
