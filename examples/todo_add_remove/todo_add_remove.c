#include <argtable3.h>
#include <assert.h>
#include <jansson.h>
#include <simperium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
prv_item_callback(struct simperium_item *item, void *cb_data)
{
    printf("> ITEM %s:  %s\n", item->id, json_dumps(item->json_data, JSON_ENCODE_ANY));
}

int
main(int argc, char **argv)
{
    struct arg_lit *help;
    struct arg_str *app_name, *api_key, *user, *passwd;
    struct arg_end *end;

    /* the global arg_xxx structs are initialised within the argtable */
    void *argtable[] = {
        help     = arg_litn(NULL, "help", 0, 1, "display this help and exit"),
        app_name = arg_str1("a", "app_name", "NAME", "The name of your simperium app"),
        api_key  = arg_str1("k", "api_key", "KEY", "The api key of your simperium app"),
        user     = arg_str1("u", "user", "USER", "The username to login with"),
        passwd   = arg_str1("p", "password", "PASSWORD", "The password for a given user"),
        end      = arg_end(20),
    };

    const char* progname = "simple";
    int exitcode = 0;
    int nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0)
    {
        printf("Usage: %s", progname);
        arg_print_syntax(stdout, argtable, "\n");
        printf("Simple Simperium client.\n\n");
        arg_print_glossary(stdout, argtable, "  %-25s %s\n");
        exitcode = 0;
        goto exit;
    }

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0)
    {
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors(stdout, end, progname);
        printf("Try '%s --help' for more information.\n", progname);
        exitcode = 1;
        goto exit;
    }

    struct simperium_app *app = simperium_app_init(app_name->sval[0], api_key->sval[0]);
    if (!app) {
        printf("Failed to initialize simperium application\n");
        exitcode = 1;
        goto exit;
    }

    struct simperium_session *session = simperium_session_open(app, user->sval[0], passwd->sval[0]);
    if (!session) {
        printf("Failed to start simperium session\n");
        exitcode = 1;
        goto deinit_app;
    }

    struct simperium_bucket *todo_bkt = simperium_bucket_open(session, "todo");
    if (!todo_bkt) {
        printf("Failed to open todo bucket\n");
        exitcode = 1;
        goto close_session;
    }

#if 1
    for (int i = 0; i < 100; ++i) {
        struct simperium_item item = {
            .json_data = json_loads("{\"title\": \"Watch Battle Royale\",\"done\": false}", JSON_DECODE_ANY, NULL),
        };
        sprintf(item.id, "%d", i);
        int err = simperium_bucket_add_item(todo_bkt, &item);
        if (err != 0) {
            printf("Failed to add bucket item\n");
        }
    }

    printf("Added 100 items to todo bucket\n");

    int err = simperium_bucket_all_items(todo_bkt, NULL, prv_item_callback, NULL);
    printf("\n");

    for (int i = 0; i < 100; ++i) {
        struct simperium_item item = {0};
        sprintf(item.id, "%d", i);
        err = simperium_bucket_remove_item(todo_bkt, &item);
        if (err != 0) {
            printf("Failed to remove bucket item\n");
        }
    }

    printf("Cleaned up\n");
#else
    const char *id = "12345678";

    struct simperium_item item = {0};
    strncpy(item.id, id, MAX_ITEM_ID_LEN);
    item.json_data = json_loads("{\"title\": \"Watch Battle Royale\",\"done\": false}", JSON_DECODE_ANY, NULL);

    int err = simperium_bucket_add_item(todo_bkt, &item);
    if (err == 0) {
        printf("Added one item to todo bucket\n");
    }

    err = simperium_bucket_get_item(todo_bkt, id, prv_item_callback, NULL);
    if (err == 0) {
        // FIXME
    }

    err = simperium_bucket_remove_item(todo_bkt, &item);
    if (err == 0) {
        printf("Removed item from todo bucket\n");
    }
#endif

close_bucket:
    simperium_bucket_close(todo_bkt);
close_session:
    simperium_session_close(session);
deinit_app:
    simperium_app_deinit(app);
exit:
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return exitcode;
}