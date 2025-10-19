/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
 *
 */

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wsserver/ws.h>

#define WS_SERVER_PORT 8089
#define WS_SERVER_ADDR "localhost"

int start_webserver();

void send_json_error(ws_cli_conn_t client, char *error, const char *detail);
void send_json_response(ws_cli_conn_t client, json_object *res);

void ws_messagehandler(ws_cli_conn_t client, const unsigned char *msg, uint64_t size, int type);
void ws_opencon(ws_cli_conn_t client);
void ws_closecon(ws_cli_conn_t client);