#include <argtable3.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Hosts
const char *HOST_AUTH = "https://auth.simperium.com/1/";
const char *HOST_API = "https://api.simperium.com/1/";

// Endpoints
const char *ENDPOINT_AUTH = "/authorize/";

// Limits
#define MAX_APP_NAME_LEN 120
#define MAX_API_KEY_LEN 64
#define MAX_URL_LEN 300

// Data structures
struct simperium_app {
    char app_name[MAX_APP_NAME_LEN];
    char api_key[MAX_API_KEY_LEN];
    CURL *curl;
};

struct simperium_session {
    struct simperium_app *app;
    char url[MAX_URL_LEN];
};

// Helper functions
static int
prv_app_set_url(struct simperium_session *session, const char *host, const char *endpoint)
{
    // XXX we could allocate URL on the stack perhaps
    strcat(session->url, host);
    strcat(session->url, session->app->app_name);
    strcat(session->url, endpoint);
    curl_easy_setopt(session->app->curl, CURLOPT_URL, session->url);
}

// Public API
struct simperium_app *
simperium_app_init(const char *app_name, const char *api_key)
{
    struct simperium_app *app = calloc(sizeof(struct simperium_app), 1);
    if (app == NULL) {
        return NULL;
    }

    curl_global_init(CURL_GLOBAL_SSL);
    CURL *curl = curl_easy_init();
    if (!curl) {
        goto error;
    }

    if (strnlen(app_name, MAX_APP_NAME_LEN) == MAX_APP_NAME_LEN ||
        strnlen(api_key, MAX_API_KEY_LEN) == MAX_API_KEY_LEN) {
        goto error;
    }

    strncpy(app->app_name, app_name, MAX_APP_NAME_LEN);
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
simperium_app_login(struct simperium_app *app, const char *user, const char *passwd)
{
    // TODO validate user/passwd lengths
    if (app == NULL) {
        return NULL;
    }

    struct simperium_session *session = calloc(sizeof(struct simperium_session), 1);
    if (session == NULL) {
        return NULL;
    }

    session->app = app;

    prv_app_set_url(session, HOST_AUTH, "/authorize/");

    //res = curl_easy_perform(curl);

    return session;
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
    }

    struct simperium_session *session = simperium_app_login(app, user->sval[0], passwd->sval[0]);
    if (!session) {
        printf("Failed to start simperium session\n");
    }

exit:
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return exitcode;
}