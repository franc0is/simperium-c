#include <assert.h>
#include "simperium.h"
#include "simperium_private.h"
#include "simperium_endpoints.h"
#include <stdlib.h>
#include <string.h>

struct simperium_bucket *
simperium_bucket_open_ws(struct simperium_session *session, const char *bucket_name)
{
    return NULL;
}

void
simperium_bucket_close_ws(struct simperium_bucket *bucket)
{
    return;
}

int
simperium_bucket_add_item_ws(struct simperium_bucket *bucket, struct simperium_item *item)
{
    return 0;
}

int
simperium_bucket_get_item_ws(struct simperium_bucket *bucket,
                             const char *id,
                             simperium_item_callback cb,
                             void *cb_data)
{
    return 0;
}

int
simperium_bucket_remove_item_ws(struct simperium_bucket *bucket, struct simperium_item *item)
{
    return 0;
}

int
simperium_bucket_all_items_ws(struct simperium_bucket *bucket,
                              const char **cursor,
                              simperium_item_callback cb,
                              void *cb_data)
{
    return 0;
}

int
simperium_bucket_get_changes_ws(struct simperium_bucket *bucket,
                                const char **cursor,
                                simperium_change_callback cb,
                                void *cb_data)
{
    return 0;
}
