#pragma once

#include <jansson.h>

enum simperium_result {
    SIMPERIUM_SUCCESS = 0,
};

enum simperium_op {
    SIMPERIUM_OP_INVALID = 0,
    SIMPERIUM_OP_INSERT,
    SIMPERIUM_OP_DELETE,
    SIMPERIUM_OP_REPLACE,
    SIMPERIUM_OP_INCREMENT,
    SIMPERIUM_OP_LISTDIFF,
    SIMPERIUM_OP_OBJDIFF,
    SIMPERIUM_OP_STRDIFF,
};

enum simperium_protocol {
    SIMPERIUM_PROTOCOL_HTTP,
    SIMPERIUM_PROTOCOL_WS,
};

struct simperium_app;
struct simperium_session;
struct simperium_bucket;

#define MAX_APP_NAME_LEN 120
#define MAX_API_KEY_LEN 64
#define MAX_BKT_NAME_LEN 120
#define MAX_ITEM_ID_LEN 64

struct simperium_item {
    char id[MAX_ITEM_ID_LEN];
    json_t *json_data;
};

struct simperium_change {
    char id[MAX_ITEM_ID_LEN];
    enum simperium_op operation;
    json_t *json_data;
};

typedef int (*simperium_item_callback)(struct simperium_item *item, void *cb_data);

typedef int (*simperium_change_callback)(struct simperium_change *change, void *cb_data);

struct simperium_app *
simperium_app_init(const char *app_name, const char *api_key);

void
simperium_app_deinit(struct simperium_app *app);

struct simperium_bucket *
simperium_bucket_open(struct simperium_session *session, const char *bucket_name);

struct simperium_session *
simperium_session_open(struct simperium_app *app,
                       const char *user,
                       const char *passwd,
                       enum simperium_protocol protocol);

void
simperium_session_close(struct simperium_session *session);

void
simperium_bucket_close(struct simperium_bucket *bucket);

int
simperium_bucket_add_item(struct simperium_bucket *bucket, struct simperium_item *item);

int
simperium_bucket_get_item(struct simperium_bucket *bucket,
                          const char *id, simperium_item_callback cb,
                          void *cb_data);

int
simperium_bucket_remove_item(struct simperium_bucket *bucket, struct simperium_item *item);

int
simperium_bucket_all_items(struct simperium_bucket *bucket,
                           const char **cursor,
                           simperium_item_callback cb,
                           void *cb_data);

int
simperium_bucket_get_changes(struct simperium_bucket *bucket,
                             const char **cursor,
                             simperium_change_callback cb,
                             void *cb_data);
