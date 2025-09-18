#include "wsserver.h"

/*
 * This section provides a web server to controll the
 * device manager through web sockets
 * 
 * by Dennis Gunia - 2025 - www.dennisgunia.de
 */

json_object *(*commandparser_func)(json_object *);

// this sections handles ws connections and communications
// called on opening websocket client
void onopen(ws_cli_conn_t client)
{
    char *cli;
    cli = ws_getaddress(client);
    printf("Connection opened, addr: %s\n", cli);
}

// called on closing websocket client
void onclose(ws_cli_conn_t client)
{
    char *cli;
    cli = ws_getaddress(client);
    printf("Connection closed, addr: %s\n", cli);
}

// called on receiving websocket message
void onmessage(ws_cli_conn_t client, const unsigned char *msg, uint64_t size, int type)
{
    char *cli = ws_getaddress(client);
    printf("received message: %s (%zu), from: %s\n", msg, size, cli);

    json_tokener *tok = json_tokener_new();
    json_object *req = json_tokener_parse_ex(tok, msg, size);
    enum json_tokener_error jerr;
    jerr = json_tokener_get_error(tok);
    if (jerr != json_tokener_success)
    {
        // check if request can be parsed, if not retrun error
        send_json_error(client, "parsing error", json_tokener_error_desc(jerr));
    }
    else
    {
        // if it can be parsed, get command.
        json_object *commandObj = json_object_object_get(req, "command");
        printf("test");
        if (commandObj != NULL)
        {
            char *command = json_object_to_json_string(commandObj);
            // get key
            json_object *res = commandparser_func(req);
            if (res == NULL)
            {
                send_json_error(client, "internal error", "");
            }
            else
            {
                send_json_response(client, res);
            }
            free(res);
        }
        else
        {
            // if key is missing, send error
            send_json_error(client, "format error", "missing key: command");
        }
    }
    free(tok); // always free tokenizer, to prevent memory leak
}

void send_json_response(ws_cli_conn_t client, json_object *res)
{
    char *message = json_object_to_json_string_ext(res, JSON_C_TO_STRING_PRETTY);
    ws_sendframe_txt(client, message);
}

void send_json_error(ws_cli_conn_t client, char *error, char *detail)
{
    json_object *root = json_object_new_object();
    json_object_object_add(root, "error", json_object_new_string(error));
    json_object_object_add(root, "detail", json_object_new_string(detail));
    send_json_response(client, root);
}

// starting webserver
int start_webserver(json_object *(*commandparser_func_ptr)(json_object *))
{
    commandparser_func = commandparser_func_ptr;
    ws_socket(&(struct ws_server){/*
         * Bind host, such as:
         * localhost -> localhost/127.0.0.1
         * 0.0.0.0   -> global IPv4
         * ::        -> global IPv4+IPv6 (Dual stack)
         */
                                  .host = WS_SERVER_ADDR,
                                  .port = WS_SERVER_PORT,
                                  .thread_loop = 0,
                                  .timeout_ms = 1000,
                                  .evs.onopen = &onopen,
                                  .evs.onclose = &onclose,
                                  .evs.onmessage = &onmessage});

    return (0);
}