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

#define WS_SERVER_PORT 8080
#define WS_SERVER_ADDR "localhost"

int start_webserver();
