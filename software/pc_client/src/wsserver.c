/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
 *
 */

#include "wsserver.h"
#include "logging/logger.h"
/*
 * This section provides a web server to controll the
 * device manager through web sockets
 */

json_object *(*commandparser_func)(json_object *);

// this sections handles ws connections and communications

/*
* called on opening websocket client
* @param client: websocket client connection
*/
void ws_opencon(ws_cli_conn_t client)
{
    char *cli;
    cli = ws_getaddress(client);
    log_message(LOG_DEBUG, "WebSocket connection opened, addr: %s", cli);
}

/*
* called on closing websocket client
* @param client: websocket client connection
*/
void ws_closecon(ws_cli_conn_t client)
{
    char *cli;
    cli = ws_getaddress(client);
    log_message(LOG_DEBUG, "WebSocket connection closed, addr: %s", cli);
}

/*
* called on receiving websocket message
* Handles incoming websocket messages, parses json commands and sends responses.
*
* @param client: websocket client connection
* @param msg: received message
* @param size: size of received message
* @param type: type of message
*/
void ws_messagehandler(ws_cli_conn_t client, const unsigned char *msg, uint64_t size, int type)
{
    char *cli = ws_getaddress(client);
    log_message(LOG_DEBUG, "WebSocket received message: %s (%zu), from: %s", msg, size, cli);

    json_tokener *tok = json_tokener_new();
    json_object *req = json_tokener_parse_ex(tok, (const char*)msg, size);
    enum json_tokener_error jerr;
    jerr = json_tokener_get_error(tok);
    if (jerr != json_tokener_success)
    {
        // check if request can be parsed, if not retrun error
        send_json_error(client, "WebSocket JSON parsing error", json_tokener_error_desc(jerr));
    }
    else
    {
        // if it can be parsed, get command.
        json_object *commandObj = json_object_object_get(req, "command");
        if (commandObj != NULL)
        {
            const char *command = json_object_to_json_string(commandObj);
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

/*
* send json response to websocket client
*
* @param client: websocket client connection
* @param res: json response object
*/
void send_json_response(ws_cli_conn_t client, json_object *res)
{
    const char *message = json_object_to_json_string_ext(res, JSON_C_TO_STRING_PRETTY);
    ws_sendframe_txt(client, message);
}

/*
* send json error to websocket client
* 
* @param client: websocket client connection
* @param error: error message
* @param detail: error detail message
*/
void send_json_error(ws_cli_conn_t client, char *error, const char *detail)
{
    json_object *root = json_object_new_object();
    json_object_object_add(root, "error", json_object_new_string(error));
    json_object_object_add(root, "detail", json_object_new_string(detail));
    send_json_response(client, root);
}

/* 
* Start websocket server
*
* @param commandparser_func_ptr: pointer to command parser function
* @return 0 on success
*/
int start_webserver(json_object *(*commandparser_func_ptr)(json_object *))
{
    commandparser_func = commandparser_func_ptr;
    log_message(LOG_INFO, "Websocket server started at ws://%s:%d", WS_SERVER_ADDR, WS_SERVER_PORT);

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
                                  .evs.onopen = &ws_opencon,
                                  .evs.onclose = &ws_closecon,
                                  .evs.onmessage = &ws_messagehandler});

    return (0);
}