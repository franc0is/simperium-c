#include <argtable3.h>
#include <assert.h>
#include <jansson.h>
#include <libwebsockets.h>
#include <simperium.h>
// XXX private
#include <simperium_private.h>
// XXX
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct lws *web_socket = NULL;
static int counter = 0;
static volatile int stop_received = 0;

static bool bucket_initialized = false;

#define LIBRARY_NAME "simperium-c"
#define LIBRARY_VERSION "0.1-dev"
#define CLIENT_ID "todo-ws"
#define BUCKET_NAME "todo"
#define SIMPERIUM_WS_API_VERSION 1.1

#define RX_BUFFER_BYTES (512)

static int
prepare_init_message(char *buf, struct simperium_session *session)
{
    json_t *req_json = json_pack("{s:i,s:s,s:s,s:s,s:s,s:s,s:s}",
                                  "api", 1,
                                  "clientid", CLIENT_ID,
                                  "token", session->token,
                                  "app_id", session->app->name,
                                  "library", LIBRARY_NAME,
                                  "version", LIBRARY_VERSION,
                                  "name", BUCKET_NAME);
    assert(req_json != NULL);
    char *req_data = json_dumps(req_json, JSON_ENCODE_ANY);
    json_decref(req_json);

    // XXX channels?
    int rv =  sprintf(buf, "0:init:%s", req_data);
    free(req_data);
    return rv;
}

static int
prepare_heartbeat_message(char *buf)
{
    return sprintf(buf, "h:%d", counter);
}

static int
ws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    struct simperium_session *session = (struct simperium_session *)user;
    switch (reason)
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("WS Connection Established\n");
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
        {
            char *in_str = (char *)in;
            printf("Data In: %s\n", in_str);
            break;
        }
        case LWS_CALLBACK_CLIENT_WRITEABLE:
        {
            char buf[LWS_SEND_BUFFER_PRE_PADDING + RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
            char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
            size_t n = 0;
            if (!bucket_initialized) {
                // auth init
                n = prepare_init_message(p, session);
                bucket_initialized = true;
            } else {
                // heartbeat
                n = prepare_heartbeat_message(p);
                counter = counter+2;
            }
            if (n != 0) {
                printf("Data Out: %s\n", p);
                lws_write(wsi, p, n, LWS_WRITE_TEXT);
            }
            break;
        }

        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("WS Connection Closed\n");
            web_socket = NULL;
            break;

        default:
            break;
    }

    return 0;
}

static void
sigint_handler(int dummy) {
    printf("\n");
    stop_received = 1;
}

static struct lws_protocols protocols[] =
{
    {
        "default",
        ws_callback,
        0, /* session user bytes */
        RX_BUFFER_BYTES,
    },
    { NULL, NULL, 0, 0 } /* terminator */
};

int
main(int argc, char *argv[])
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

    struct simperium_session *session = simperium_session_open(app,
                                                               user->sval[0],
                                                               passwd->sval[0],
                                                               SIMPERIUM_PROTOCOL_HTTP);
    if (!session) {
        printf("Failed to start simperium session\n");
        exitcode = 1;
        goto deinit_app;
    }

    // create ws context
    struct lws_context_creation_info info;
    memset( &info, 0, sizeof(info) );
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    struct lws_context *context = lws_create_context( &info );

    // configure ws conntection
    char ws_path[50] = {0};
    snprintf(ws_path, sizeof(ws_path), "/sock/1/%s/websocket", app_name->sval[0]);
    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = context;
    ccinfo.ssl_connection = LCCSCF_USE_SSL |
                            LCCSCF_ALLOW_SELFSIGNED |
                            LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    ccinfo.address = "api.simperium.com";
    ccinfo.port = 443;
    ccinfo.path = ws_path;
    ccinfo.host = lws_canonical_hostname( context );
    ccinfo.origin = "api.simperium.com";
    ccinfo.userdata = session;


    // we want to catch sig-int so we can cleanup
    signal(SIGINT, sigint_handler);

    time_t old = 0;
    while(!stop_received)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        /* Connect if we are not connected to the server. */
        if (!web_socket && tv.tv_sec != old)
        {
           web_socket = lws_client_connect_via_info(&ccinfo);
        }

        if (tv.tv_sec > old + 4)
        {
            /* Send a heartbeat to the server every 5 second. */
            lws_callback_on_writable(web_socket);
            old = tv.tv_sec;
        }

        lws_service( context, /* timeout_ms = */ 250 );
    }

    printf("Cleaning up!\n");
cleanup_websocket:
    lws_context_destroy( context );
close_session:
    simperium_session_close(session);
deinit_app:
    simperium_app_deinit(app);
exit:
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return exitcode;
}
