#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wsserver/ws.h>

#define WS_SERVER_PORT 8080
#define WS_SERVER_ADDR "localhost"

int start_webserver();
