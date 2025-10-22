/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
 *
 */

#include "console.h"

char *device_config_file;
int fd;
/*
* Command parser for wsserver.
* Parses incoming json commands and executes corresponding functions.
*/


/*
* command: dm_dump
* description: dump all device details as json to websocket
*
* request format: { "command": "dm_dump" }
* response format: { ... all device details ... }
* 
* @param req: json request object
* @param res: json response object
*/
void cmd_dm_dump(json_object *req, json_object *res)
{
    devicemgr_printDetailsAll(res);
}

/*
* command: dm_describe
* description: describe device with id as json to websocket
*
* request format: { "command": "dm_describe", "id": <device_id> }
* response format: { ... device details ... }
* 
* @param req: json request object
* @param res: json response object
*/
void cmd_dm_describe(json_object *req, json_object *res)
{
    json_object *id;
    if (json_object_object_get_ex(req, "id", &id))
    {
        devicemgr_printDetails(json_object_get_int(id), res);
    }
    else
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: id"));
    }
}

/*
* command: dm_register
* description: register new device at address with x,y position
* 
* request format: { "command": "dm_register", "address": <address>, "x": <x>, "y": <y> }
* response format: { "id": <new_device_id> }
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dm_register(json_object *req, json_object *res)
{
    json_object *jaddress, *jx, *jy;
    int address, x, y;
    if (!json_object_object_get_ex(req, "address", &jaddress))
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: address"));
    }
    else if (!json_object_object_get_ex(req, "x", &jx))
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: x"));
    }
    else if (!json_object_object_get_ex(req, "y", &jy))
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: y"));
    }
    else
    {
        address = json_object_get_int(jaddress);
        x = json_object_get_int(jx);
        y = json_object_get_int(jy);

        int newId = devicemgr_register(fd, address, x, y, -1);
        json_object_object_add(res, "id", json_object_new_int(newId));
    }
}

/*
* command: dm_refresh
* description: refresh all devices and update their status
* 
* request format: { "command": "dm_refresh" }
* response format: { "devices_online": <number_of_online_devices> }
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dm_refresh(json_object *req, json_object *res)
{
    int devices_online = devicemgr_refresh();
    json_object_object_add(res, "devices_online", json_object_new_int(devices_online));
}


/*
* command: dm_remove
* description: remove device with id from device manager
*
* request format: { "command": "dm_remove", "id": <device_id> }
* response format: { "ack": true }
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dm_remove(json_object *req, json_object *res)
{
    json_object *jid;
    int id;
    if (!json_object_object_get_ex(req, "id", &jid))
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: id"));
    }
    else
    {
        id = json_object_get_int(jid);
        devicemgr_remove(id);
        json_object_object_add(res, "ack", json_object_new_boolean(true));
    }
}

/*
* command: dm_save
* description: save all devices to config file
*
* request format: { "command": "dm_save" }
* response format: { "ack": true }
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dm_save(json_object *req, json_object *res)
{
    devicemgr_save(device_config_file);
    json_object_object_add(res, "ack", json_object_new_boolean(true));
}

/*
* command: dm_load
* description: load all devices from config file
*
* request format: { "command": "dm_load" }
* response format: { "ack": true }
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dm_load(json_object *req, json_object *res)
{
    devicemgr_load(device_config_file);
    json_object_object_add(res, "ack", json_object_new_boolean(true));
}


/*
* command: dm_print
* description: print text to display at position (x,y)
*
* request format: { "command": "dm_print", "x": <x>, "y": <y>, "string": <text>, "full": <true/false> }
* response format: { "ack": true }
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dm_print(json_object *req, json_object *res)
{
    json_object *jx = json_object_object_get(req, "x");
    json_object *jy = json_object_object_get(req, "y");
    json_object *jstr = json_object_object_get(req, "string");
    json_object *jfullrot = json_object_object_get(req, "full");
    if (jx == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: x"));
    }
    else if (jy == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: y"));
    }
    else if (jstr == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: string"));
    }
    else
    {
        int x = json_object_get_int(jx);
        int y = json_object_get_int(jy);
        const char *str = json_object_get_string(jstr);
        if (jfullrot == NULL)
        {
            devicemgr_printText(str, x, y, DISPLAY_FULLROTATION);
        }
        else if (json_object_get_boolean(jfullrot) == false)
        {
            devicemgr_printText(str, x, y, DISPLAY_DIRECT);
        }
        else
        {
            devicemgr_printText(str, x, y, DISPLAY_FULLROTATION);
        }

        json_object_object_add(res, "ack", json_object_new_boolean(true));
        send_json_history(req);
    }
}

/*
* command: dm_print_single
* description: print single flap to display at position (x,y)
*
* request format: { "command": "dm_print_single", "x": <x>, "y": <y>, "flap": <flap_id>, "full": <true/false> }
* response format: { "ack": true }
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dm_print_single(json_object *req, json_object *res)
{
    json_object *jx = json_object_object_get(req, "x");
    json_object *jy = json_object_object_get(req, "y");
    json_object *jflap = json_object_object_get(req, "flap");
    json_object *jfullrot = json_object_object_get(req, "full");
    if (jx == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: x"));
    }
    else if (jy == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: y"));
    }
    else if (jflap == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: string"));
    }
    else
    {
        int x = json_object_get_int(jx);
        int y = json_object_get_int(jy);
        int flap = json_object_get_int(jflap);
        if (jfullrot == NULL)
        {
            devicemgr_printFlap(flap, x, y, DISPLAY_FULLROTATION);
        }
        else if (json_object_get_boolean(jfullrot) == false)
        {
            devicemgr_printFlap(flap, x, y, DISPLAY_DIRECT);
        }
        else
        {
            devicemgr_printFlap(flap, x, y, DISPLAY_FULLROTATION);
        }
        json_object_object_add(res, "ack", json_object_new_boolean(true));
    }
}

/*
* command: dm_clear
* description: clear all displays (set to flap 0)
*
* request format: { "command": "dm_clear" }
* response format: { "ack": true }
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dm_clear(json_object *req, json_object *res)
{
    devicemgr_clearscreen();
    json_object_object_add(res, "ack", json_object_new_boolean(true));
}


/*
* command: dr_ping
* description: ping device at address
*
* request format: { "command": "dr_ping", "address": <address> }
* response format: { "success": true/false }
*
* @param req: json request object
* @param res: json response object  
*/
void cmd_dr_ping(json_object *req, json_object *res)
{
    json_object *jaddr = json_object_object_get(req, "address");
    if (jaddr == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: address"));
    }
    else
    {
        if (sfbus_ping(fd, json_object_get_int(jaddr)) == 0)
        {
            json_object_object_add(res, "success", json_object_new_boolean(true));
        }
        else
        {
            json_object_object_add(res, "success", json_object_new_boolean(false));
        }
    }
}

/*
* command: dr_setaddress
* description: set new address for device
*
* request format: { "command": "dr_setaddress", "address": <current_address>, "newaddress": <new_address> }
* response format: { "success": true/false }    
*
* @param req: json request object   
* @param res: json response object
*/
void cmd_dr_setaddress(json_object *req, json_object *res)
{
    json_object *jaddr = json_object_object_get(req, "address");
    json_object *jaddrn = json_object_object_get(req, "newaddress");
    if (jaddr == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: address"));
    }
    else if (jaddrn == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: newaddress"));
    }
    else
    {
        if (sfbusu_write_address(fd, json_object_get_int(jaddr), json_object_get_int(jaddrn)) == 0)
        {
            json_object_object_add(res, "success", json_object_new_boolean(true));
        }
        else
        {
            json_object_object_add(res, "success", json_object_new_boolean(false));
        }
    }
}

/*
* command: dr_setcalibration
* description: set calibration value for device
*
* request format: { "command": "dr_setcalibration", "address": <address>, "calibration": <calibration_value> }
* response format: { "success": true/false }
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dr_setcalibration(json_object *req, json_object *res)
{
    json_object *jaddr = json_object_object_get(req, "address");
    json_object *jcal = json_object_object_get(req, "calibration");
    if (jaddr == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: address"));
    }
    else if (jcal == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: calibration"));
    }
    else
    {
        if (sfbusu_write_calibration(fd, json_object_get_int(jaddr), json_object_get_int(jcal)) == 0)
        {
            json_object_object_add(res, "success", json_object_new_boolean(true));
        }
        else
        {
            json_object_object_add(res, "success", json_object_new_boolean(false));
        }
    }
}

/*
* command: dr_reset
* description: reset device at address  
*
* request format: { "command": "dr_reset", "address": <address> }
* response format: { "ack": true }
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dr_reset(json_object *req, json_object *res)
{
    json_object *jaddr = json_object_object_get(req, "address");
    if (jaddr == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: address"));
    }
    else
    {
        sfbus_reset_device(fd, json_object_get_int(jaddr));
        json_object_object_add(res, "ack", json_object_new_boolean(true));
    }
}

/*
* command: dr_display
* description: set display flap at address
*
* request format: { "command": "dr_display", "address": <address>, "flap": <flap_id>, "full": <true/false> }
* response format: { "ack": true }
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dr_display(json_object *req, json_object *res)
{
    json_object *jaddr = json_object_object_get(req, "address");
    json_object *jflap = json_object_object_get(req, "flap");
    json_object *jfullrot = json_object_object_get(req, "full");
    if (jaddr == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: address"));
    }
    else if (jflap == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: flap"));
    }
    else
    {
        if (jfullrot == NULL)
        {
            sfbus_display(fd, json_object_get_int(jaddr), json_object_get_int(jflap));
        }
        else if (json_object_get_boolean(jfullrot) == false)
        {
            sfbus_display(fd, json_object_get_int(jaddr), json_object_get_int(jflap));
        }
        else
        {
            sfbus_display_full(fd, json_object_get_int(jaddr), json_object_get_int(jflap));
        }
        json_object_object_add(res, "ack", json_object_new_boolean(true));
    }
}

/*
* command: dr_power
* description: set motor power state at address
*
* request format: { "command": "dr_power", "address": <address>, "power": <true/false> }
* response format: { "ack": true }  
*
* @param req: json request object
* @param res: json response object
*/
void cmd_dr_power(json_object *req, json_object *res)
{
    json_object *jaddr = json_object_object_get(req, "address");
    json_object *jpower = json_object_object_get(req, "power");
    if (jaddr == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: address"));
    }
    else if (jpower == NULL)
    {
        json_object_object_add(res, "error", json_object_new_string("format error"));
        json_object_object_add(res, "detail", json_object_new_string("missing key: power"));
    }
    else
    {
        if (json_object_get_boolean(jpower) == false)
        {
            sfbus_motor_power(fd, json_object_get_int(jaddr), 0);
        }
        else
        {
            sfbus_motor_power(fd, json_object_get_int(jaddr), 1);
        }
        json_object_object_add(res, "ack", json_object_new_boolean(true));
    }
}


/*
* Command parser for wsserver.
* Parses incoming json commands and executes corresponding functions.
*
* @param req: json request object
* @return json response object
*/
json_object *parse_command(json_object *req)
{
    json_object *commandObj;
    json_object *res = json_object_new_object();
    json_object_object_get_ex(req, "command", &commandObj);
    const char *command = json_object_get_string(commandObj);
    free(commandObj);
    // command 'table'
    if (strcmp(command, "dm_dump") == 0)
    {
        cmd_dm_dump(req, res);
        return res;
    }
    else if (strcmp(command, "dm_describe") == 0)
    {
        cmd_dm_describe(req, res);
        return res;
    }
    else if (strcmp(command, "dm_register") == 0)
    {
        cmd_dm_register(req, res);
        return res;
    }
    else if (strcmp(command, "dm_remove") == 0)
    {
        cmd_dm_remove(req, res);
        return res;
    }
    else if (strcmp(command, "dm_refresh") == 0)
    {
        cmd_dm_refresh(req, res);
        return res;
    }
    else if (strcmp(command, "dm_save") == 0)
    {
        cmd_dm_save(req, res);
        return res;
    }
    else if (strcmp(command, "dm_load") == 0)
    {
        cmd_dm_load(req, res);
        return res;
    }
    else if (strcmp(command, "dm_print") == 0)
    {
        cmd_dm_print(req, res);
        return res;
    }
    else if (strcmp(command, "dm_clear") == 0)
    {
        cmd_dm_clear(req, res);
        return res;
    }
    else if (strcmp(command, "dr_ping") == 0)
    {
        cmd_dr_ping(req, res);
        return res;
    }
    else if (strcmp(command, "dr_setaddress") == 0)
    {
        cmd_dr_setaddress(req, res);
        return res;
    }
    else if (strcmp(command, "dr_setcalibration") == 0)
    {
        cmd_dr_setcalibration(req, res);
        return res;
    }
    else if (strcmp(command, "dr_reset") == 0)
    {
        cmd_dr_reset(req, res);
        return res;
    }
    else if (strcmp(command, "dr_display") == 0)
    {
        cmd_dr_display(req, res);
        return res;
    }
    else if (strcmp(command, "dr_power") == 0)
    {
        cmd_dr_power(req, res);
        return res;
    }
    else
    {
        json_object_object_add(res, "error", json_object_new_string("invalid command"));
        json_object_object_add(res, "detail", json_object_new_string(""));
        return res;
    }
    return NULL;
}

/*
* Start console with webserver and device manager
*
* @param _fd: rs485 file descriptor
* @param configFile: path to device manager config file
*/
void start_console(int _fd, char *configFile)
{
    device_config_file = configFile; // set config file path
    fd = _fd;                        // set rs485 file descriptor
    start_webserver(&parse_command); // start webserver with command parser
}