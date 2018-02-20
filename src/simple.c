#include <argtable3.h>
#include <assert.h>
#include <curl/curl.h>
#include <jansson.h>
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
#define MAX_TOKEN_LEN 64
#define MAX_URL_LEN 300

// Data structures
struct simperium_app {
    char name[MAX_APP_NAME_LEN];
    char api_key[MAX_API_KEY_LEN];
    CURL *curl;
};

struct simperium_session {
    char token[MAX_TOKEN_LEN];
};

struct response_data {
    size_t bytes_written;
    size_t bytes_left;
    char buffer[];
};

// Helper functions
void
prv_build_url(char *url_out, const char *app_name, const char *host, const char *endpoint)
{
    strcat(url_out, host);
    strcat(url_out, app_name);
    strcat(url_out, endpoint);
}

static size_t
prv_auth_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t size_bytes = size * nmemb;
    struct response_data *rd = userp;

    if (rd->bytes_left < size_bytes) {
        // Note that this will cause an error
        size_bytes = rd->bytes_left;
    }

    memcpy(&rd->buffer[rd->bytes_written], contents, size_bytes);
    rd->bytes_written += size_bytes;
    rd->bytes_left -= size_bytes;

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
    // TODO validate user/passwd lengths
    if (app == NULL) {
        return NULL;
    }

    struct simperium_session *session = calloc(sizeof(struct simperium_session), 1);
    assert(session != NULL);

    // Set URL
    char url[MAX_URL_LEN] = {0};
    prv_build_url(url, app->name, HOST_AUTH, "/authorize/");
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
    struct response_data *resp_data = calloc(sizeof(struct response_data) + 250, 1); // FIXME
    assert(resp_data != NULL);

    resp_data->bytes_left = 250; // FIXME
    curl_easy_setopt(app->curl, CURLOPT_WRITEFUNCTION, prv_auth_callback);
    curl_easy_setopt(app->curl, CURLOPT_WRITEDATA, resp_data);

    // Run request
    CURLcode res = curl_easy_perform(app->curl);
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        goto error;
    }

    // Extract token from response
    json_t *resp_json = json_loads(resp_data->buffer, JSON_DECODE_ANY, NULL);
    if (resp_json == NULL) {
        fprintf(stderr, "malformed JSON, received: %s\n",
                resp_data->buffer);
        goto error;
    }
    const char *u, *tk, *id;
    int result = json_unpack(resp_json, "{s:s, s:s, s:s}",
                                        "username", &u,
                                        "access_token", &tk,
                                        "userid", &id);
    if (result != 0) {
        fprintf(stderr, "No token found in: %s\n",
                resp_data->buffer);
    }
    strcpy(session->token, tk);
    json_decref(resp_json);

    printf("Found token %s\n", session->token);

error:
    free(req_data);
    free(resp_data);
    return session;
}

void
simperium_session_close(struct simperium_session *session)
{
    free(session);
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

    simperium_session_close(session);

deinit_app:
    simperium_app_deinit(app);
exit:
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return exitcode;
}