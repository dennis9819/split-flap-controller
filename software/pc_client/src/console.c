#include "console.h"

const char *device_config_file = "./flapconfig.json";
int fd;
// command handlers

// dump config/ all devices
void cmd_dm_dump(json_object *req, json_object *res)
{
    devicemgr_printDetailsAll(res);
}

// describe single device
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

// register new device
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

        printf("[INFO][console] register new device wit addr %i at (%i,%i)", address, x, y);

        int newId = devicemgr_register(fd, address, x, y, -1);
        json_object_object_add(res, "id", json_object_new_int(newId));
    }
}

// refresh all devices
void cmd_dm_refresh(json_object *req, json_object *res)
{
    int devices_online = devicemgr_refresh();
    json_object_object_add(res, "devices_online", json_object_new_int(devices_online));
}


// remove device
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

// save all devices
void cmd_dm_save(json_object *req, json_object *res)
{
    devicemgr_save(device_config_file);
    json_object_object_add(res, "ack", json_object_new_boolean(true));
}

// load all devices
void cmd_dm_load(json_object *req, json_object *res)
{
    devicemgr_load(device_config_file);
    json_object_object_add(res, "ack", json_object_new_boolean(true));
}


// print string on display
void cmd_dm_print(json_object *req, json_object *res)
{
    json_object *jx = json_object_object_get(req, "x");
    json_object *jy = json_object_object_get(req, "y");
    json_object *jstr = json_object_object_get(req, "string");
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
        char *str = json_object_get_string(jstr);
        devicemgr_printText(str, x, y);
        json_object_object_add(res, "ack", json_object_new_boolean(true));
    }
}

// set flap on display
void cmd_dm_print_single(json_object *req, json_object *res)
{
    json_object *jx = json_object_object_get(req, "x");
    json_object *jy = json_object_object_get(req, "y");
    json_object *jflap = json_object_object_get(req, "flap");
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

        devicemgr_printFlap(flap, x, y);
        json_object_object_add(res, "ack", json_object_new_boolean(true));
    }
}


// ping device
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

// set device address
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


// command parser
json_object *parse_command(json_object *req)
{
    json_object *commandObj;
    json_object *res = json_object_new_object();
    json_object_object_get_ex(req, "command", &commandObj);
    char *command = json_object_get_string(commandObj);
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


void start_console(int _fd)
{
    fd = _fd;
    // init device manager
    devicemgr_init(fd);
    // start server
    start_webserver(&parse_command);
}