#include "devicemgr.h"
#include <json-c/json_object.h>
#include <string.h>

/*
 * This file provides an abstraction layer to access many devices
 * simultaneously. by Dennis Gunia - 2025 - wwwm.dennisgunia.de
 */
enum SFDEVICE_STATE
{
    NEW,
    OFFLINE,
    ONLINE,
    FAILED
};
enum SFDEVICE_POWER
{
    DISABLED,
    ENABLED,
    UNKNOWN
};

struct SFDEVICE
{
    int pos_x;
    int pos_y;
    u_int16_t address;
    int rs485_descriptor;
    double reg_voltage;
    u_int32_t reg_counter;
    u_int8_t reg_status;
    u_int8_t current_flap;
    enum SFDEVICE_STATE deviceState;
    enum SFDEVICE_POWER powerState;
};

enum
{
    SFDEVICE_MAXDEV = 128,
    SFDEVICE_MAX_X = 20,
    SFDEVICE_MAX_Y = 4
};

// next free slot to register device
int nextFreeSlot = -1;
int deviceMap[SFDEVICE_MAX_X][SFDEVICE_MAX_Y];

struct SFDEVICE devices[SFDEVICE_MAXDEV];

const char *symbols[45] = {" ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
                         "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Ä", "Ö", "Ü",
                         "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ".", "-", "?", "!"};

void devicemgr_init()
{
    // reserve memory buffer
    for (int y = 0; y < SFDEVICE_MAX_Y; y++)
    {
        for (int x = 0; x < SFDEVICE_MAX_X; x++)
        {
            deviceMap[x][y] = -1; //all empty slots are -1
        }
    }
}

int devicemgr_readStatus(int device_id)
{
    double _voltage = 0;
    u_int32_t _counter = 0;
    u_int8_t _status =
        sfbus_read_status(devices[device_id].rs485_descriptor, devices[device_id].address, &_voltage, &_counter);
    if (_status == 0xFF)
    {
        devices[device_id].powerState = UNKNOWN;
        devices[device_id].deviceState = OFFLINE;
        return -1;
    }
    devices[device_id].reg_voltage = _voltage;
    devices[device_id].reg_counter = _counter;
    devices[device_id].reg_status = _status;
    devices[device_id].powerState = ~((devices[device_id].reg_status >> 4)) & 0x01;
    devices[device_id].deviceState = ONLINE;
    if ((((devices[device_id].reg_status) >> 5) & 0x01) > 0)
    {
        devices[device_id].deviceState = FAILED;
    }
    return 0;
}

json_object *devicemgr_printMap()
{
    json_object *rows_array = json_object_new_array();
    for (int y = 0; y < SFDEVICE_MAX_Y; y++)
    {
        json_object *columns_array = json_object_new_array();
        for (int x = 0; x < SFDEVICE_MAX_X; x++)
        {
            json_object_array_add(columns_array, json_object_new_int(deviceMap[x][y]));
        }
        json_object_array_add(rows_array, columns_array);
    }
    return rows_array;
}

json_object *devicemgr_printDetails(int device_id)
{
    // generate json object with status
    json_object *root = json_object_new_object();
    json_object_object_add(root, "id", json_object_new_int(device_id));
    json_object_object_add(root, "address", json_object_new_int(devices[device_id].address));
    json_object_object_add(root, "flapID", json_object_new_int(devices[device_id].current_flap));
    json_object_object_add(root, "flapChar", json_object_new_string(symbols[devices[device_id].current_flap]));
    json_object *position = json_object_new_object();
    json_object_object_add(position, "x", json_object_new_int(devices[device_id].pos_x));
    json_object_object_add(position, "y", json_object_new_int(devices[device_id].pos_y));
    json_object_object_add(root, "position", position);

    json_object *status = json_object_new_object();
    json_object_object_add(status, "voltage", json_object_new_double(devices[device_id].reg_voltage));
    json_object_object_add(status, "rotations", json_object_new_int(devices[device_id].reg_counter));
    json_object_object_add(status, "power", json_object_new_boolean(devices[device_id].powerState));
    json_object_object_add(status, "raw", json_object_new_uint64(devices[device_id].reg_status));
    switch (devices[device_id].deviceState)
    {
    case ONLINE:
        json_object_object_add(status, "device", json_object_new_string("ONLINE"));
        break;
    case OFFLINE:
        json_object_object_add(status, "device", json_object_new_string("OFFLINE"));
        break;
    case FAILED:
        json_object_object_add(status, "device", json_object_new_string("FAILED"));
        break;
    case NEW:
        json_object_object_add(status, "device", json_object_new_string("NEW"));
        break;
    }
    json_object *status_flags = json_object_new_object();
    json_object_object_add(status_flags,
                           "errorTooBig",
                           json_object_new_boolean(((devices[device_id].reg_status) >> 0) & 0x01));
    json_object_object_add(status_flags,
                           "noHome",
                           json_object_new_boolean(((devices[device_id].reg_status) >> 1) & 0x01));
    json_object_object_add(status_flags,
                           "fuseBlown",
                           json_object_new_boolean(((devices[device_id].reg_status) >> 2) & 0x01));
    json_object_object_add(status_flags,
                           "homeSense",
                           json_object_new_boolean(((devices[device_id].reg_status) >> 3) & 0x01));
    json_object_object_add(status_flags,
                           "powerDown",
                           json_object_new_boolean(((devices[device_id].reg_status) >> 4) & 0x01));
    json_object_object_add(status_flags,
                           "failSafe",
                           json_object_new_boolean(((devices[device_id].reg_status) >> 5) & 0x01));
    json_object_object_add(status_flags,
                           "busy",
                           json_object_new_boolean(((devices[device_id].reg_status) >> 6) & 0x01));
    json_object_object_add(status, "flags", status_flags);
    json_object_object_add(root, "status", status);

    return root;
}

json_object *devicemgr_printDetailsAll()
{
    json_object *root = json_object_new_object();
    json_object_object_add(root, "devices_all", json_object_new_int(nextFreeSlot + 1));
    json_object *devices_arr = json_object_new_array();
    int devices_online = 0;
    for (int i = 0; i < (nextFreeSlot + 1); i++)
    {
        devicemgr_readStatus(i);
        if (devices[i].deviceState == ONLINE)
        {
            devices_online++;
        }
        json_object_array_add(devices_arr, devicemgr_printDetails(i));
    }
    json_object_object_add(root, "map", devicemgr_printMap());
    json_object_object_add(root, "devices", devices_arr);
    json_object_object_add(root, "devices_online", json_object_new_int(devices_online));

    printf("The json representation:\n\n%s\n\n", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
    return root;
}

void setSingle(int id, char flap)
{
    // first convert char to flap id
    char test_char = toupper(flap);
    printf("find char %c\n", test_char);
    for (int ix = 0; ix < 45; ix++){
        if (*symbols[ix] == test_char){
            printf("match char %i %i %i\n",test_char, *symbols[ix], ix);
            sfbus_display_full(devices[id].rs485_descriptor, devices[id].address, ix);
            break;
        }
    }
    devices[nextFreeSlot].current_flap = flap;
}

void printText(char *text, int x, int y)
{
    for (int i = 0; i < strlen(text); i++)
    {
        int this_id = deviceMap[x + i][y];
        if (this_id >= 0)
        {
            printf("print char %c to %i\n",*(text + i), devices[this_id].address);

            setSingle(this_id, *(text + i));
            usleep(5000);
        }
    }
}

int devicemgr_register(int rs485_descriptor, u_int16_t address, int x, int y)
{
    nextFreeSlot++;
    devices[nextFreeSlot].pos_x = x;
    devices[nextFreeSlot].pos_y = y;
    devices[nextFreeSlot].address = address;
    devices[nextFreeSlot].rs485_descriptor = rs485_descriptor;
    devices[nextFreeSlot].reg_voltage = 0;
    devices[nextFreeSlot].reg_counter = 0;
    devices[nextFreeSlot].reg_status = 0;
    devices[nextFreeSlot].current_flap = 0;
    devices[nextFreeSlot].deviceState = NEW;
    devices[nextFreeSlot].powerState = DISABLED;
    // try to reach device
    devicemgr_readStatus(nextFreeSlot);
    if (deviceMap[x][y] >= 0)
    { // rest old ones
        int old_id = deviceMap[x][y];
        devices[old_id].pos_x = -1;
        devices[old_id].pos_y = -1;
    }
    deviceMap[x][y] = nextFreeSlot;
    return nextFreeSlot;
}
