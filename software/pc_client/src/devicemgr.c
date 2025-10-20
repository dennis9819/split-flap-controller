/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
 *
 * This section provides an abstraction layer to access many devices
 * simultaneously
 */

#include "devicemgr.h"
#include "logging/logger.h"
#include <json-c/json_object.h>
#include <string.h>
#include <sys/types.h>

enum SFDEVICE_STATE
{
    UNALLOCATED, // device slot not allocated
    NEW,         // device slot allocated, but is not yet refreshed
    OFFLINE,     // device is allocated, but not reachable
    ONLINE,      // device is online and reachable
    FAILED,      // device is online, but in fail-safe mode
    REMOVED      // device has been removed and can be reallocated
};
enum SFDEVICE_POWER
{
    DISABLED, // motor power disabled
    ENABLED,  // motor power enabled
    UNKNOWN   // power state unknown (device offline)
};


struct SFDEVICE
{
    int pos_x;                       // position in matrix
    int pos_y;                       // position in matrix
    u_int16_t address;               // device address
    u_int16_t calibration;           // calibration offset value
    int rs485_descriptor;            // rs485 file descriptor
    double reg_voltage;              // last read voltage
    u_int32_t reg_counter;           // last rotation counter
    u_int8_t reg_status;             // last status register
    u_int8_t current_flap;           // current flap position
    enum SFDEVICE_STATE deviceState; // device state
    enum SFDEVICE_POWER powerState;  // power state
};

enum
{
    SFDEVICE_MAXDEV = 128,  // maximum number of devices supported
    SFDEVICE_MAX_X = 20,    // maximum x size of device matrix
    SFDEVICE_MAX_Y = 4,     // maximum y size of device matrix
    JSON_MAX_LINE_LEN = 256 // maximum length of a line in json file
};


int nextFreeSlot = -1;                         // next free slot in device array
int deviceMap[SFDEVICE_MAX_X][SFDEVICE_MAX_Y]; // device map matrix
int deviceFd;                                  // rs485 file descriptor
struct SFDEVICE devices[SFDEVICE_MAXDEV];      // device array

// symbol table for flap characters
const char *symbols[45] = {" ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
                           "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Ä", "Ö", "Ü",
                           "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ".", "-", "?", "!"};

/*
* Initialize device manager.
* Clear device map and set all devices as unallocated
*
* @param fd rs485 file descriptor
*/
void devicemgr_init(int fd)
{
    deviceFd = fd; // store rs485 file descriptor
    // reserve memory buffer
    for (int y = 0; y < SFDEVICE_MAX_Y; y++)
    {
        for (int x = 0; x < SFDEVICE_MAX_X; x++)
        {
            deviceMap[x][y] = -1; //all empty slots are -1
        }
    }
    for (int ix = 0; ix < SFDEVICE_MAXDEV; ix++)
    {
        devices[ix].address = 0; // Adress 0 is only used for new units. should never be used for active unit
        devices[ix].deviceState = UNALLOCATED; // mark all devices as unallocated
    }
}
/*
* Read status from device and store it in device struct
* Returns 0 on success, -1 on read error, -2 if device not defined
*
* @param device_id ID of device to read
*/
int devicemgr_readStatus(int device_id)
{
    if (devices[device_id].address > 0)
    { // only if defined
        double _voltage = 0;
        u_int32_t _counter = 0;
        u_int8_t _status =
            sfbus_read_status(devices[device_id].rs485_descriptor, devices[device_id].address, &_voltage, &_counter);

        if (_status == 0xFF) // error reading status
        {
            devices[device_id].powerState = UNKNOWN;
            devices[device_id].deviceState = OFFLINE;
            return -1;
        }
        devices[device_id].reg_voltage = _voltage; // store read values
        devices[device_id].reg_counter = _counter;
        devices[device_id].reg_status = _status;
        devices[device_id].powerState = ~((devices[device_id].reg_status >> 4)) & 0x01;
        devices[device_id].deviceState = ONLINE;
        if ((((devices[device_id].reg_status) >> 5) & 0x01) > 0) // fail safe active
        {
            devices[device_id].deviceState = FAILED;
        }
        return 0; // success
    }
    else
    {
        return -2;
    }
}

/*
* Read calibration data from device and store it in device struct
* Returns 0 on success, -1 on read error, -2 if device not online
* 
* @param device_id ID of device to read
*/
int devicemgr_readCalib(int device_id)
{
    if (devices[device_id].deviceState == ONLINE)
    {
        char *buffer_r = malloc(SFBUS_MAX_BUFFER_SIZE);
        if (sfbus_read_eeprom(devices[device_id].rs485_descriptor, devices[device_id].address, buffer_r) > 0)
        {
            uint16_t calib_data = (*(buffer_r + 2) & 0xFF | ((*(buffer_r + 3) << 8) & 0xFF00));
            devices[device_id].calibration = calib_data;
            free(buffer_r);
        }
        else
        {
            log_message(LOG_ERROR, "Error reading eeprom from %i", device_id);
            free(buffer_r);
            return -1;
        }
    }
    else
    {
        return -2;
    }
}

/*
* Generate json object with device map
*
* @return json object with device map
*/
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

/*
* Generate json object with device details
*
* @param device_id ID of device to print
* @param root json object to add details to
*/
void devicemgr_printDetails(int device_id, json_object *root)
{
    // generate json object with status
    json_object_object_add(root, "id", json_object_new_int(device_id));
    json_object_object_add(root, "address", json_object_new_int(devices[device_id].address));
    json_object_object_add(root, "calibration", json_object_new_int(devices[device_id].calibration));
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
    switch (devices[device_id].deviceState) // device state
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
    case REMOVED:
        json_object_object_add(status, "device", json_object_new_string("REMOVED"));
        break;
    default:
        json_object_object_add(status, "device", json_object_new_string("UNALLOCATED"));
        break;
    }
    json_object *status_flags = json_object_new_object(); // status flags
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
}

/*
* Generate json object with details for all valid devices
*
* @param root json object to add details to
*/
void devicemgr_printDetailsAll(json_object *root)
{
    json_object_object_add(root, "devices_all", json_object_new_int(nextFreeSlot + 1));
    json_object *devices_arr = json_object_new_array();
    int devices_online = 0;
    for (int i = 0; i < (nextFreeSlot + 1); i++)
    {
        if (devices[i].address > 0)
        {
            devicemgr_readStatus(i);
            if (devices[i].deviceState == ONLINE)
            {
                devices_online++;
            }
            json_object *device = json_object_new_object();
            devicemgr_printDetails(i, device);
            json_object_array_add(devices_arr, device);
        }
    }
    json_object_object_add(root, "map", devicemgr_printMap());
    json_object_object_add(root, "devices", devices_arr);
    json_object_object_add(root, "devices_online", json_object_new_int(devices_online));
}

/*
* Set single device to flap character
*
* @param id ID of device to set
* @param flap character to set
*/
void setSingle(int id, char flap)
{
    // first convert char to flap id
    char test_char = toupper(flap);
    // printf("find char %c\n", test_char);
    for (int ix = 0; ix < 45; ix++)
    {
        if (*symbols[ix] == test_char)
        {
            // printf("match char %i %i %i\n", test_char, *symbols[ix], ix);
            sfbus_display_full(devices[id].rs485_descriptor, devices[id].address, ix);
            devices[id].current_flap = ix;
            break;
        }
    }
}

/*
* Set single device to raw flap ID
*
* @param id ID of device to set
* @param flap flap ID to set
*/
void devicemgr_setSingleRaw(int id, int flap)
{
    sfbus_display_full(devices[id].rs485_descriptor, devices[id].address, flap);
    devices[id].current_flap = flap;
}

/*
* Print text to device matrix starting at position (x,y)
*
* @param text text to print
* @param x x position to start printing
* @param y y position to start printing
*/
void devicemgr_printText(const char *text, int x, int y)
{
    for (int i = 0; i < strlen(text); i++)
    {
        int this_id = deviceMap[x + i][y];
        if (this_id >= 0)
        {
            log_message(LOG_DEBUG, "print char '%c' to id:%i", *(text + i), devices[this_id].address);

            setSingle(this_id, *(text + i));
        }
    }
}

/*
* Print char to device matrix at position (x,y)
*
* @param flap flap ID to print
* @param x x position to print
* @param y y position to print
*/
void devicemgr_printFlap(int flap, int x, int y)
{
    int this_id = deviceMap[x][y];
    if (this_id >= 0)
    {
        devicemgr_setSingleRaw(this_id, flap);
    }
}

/*
* Clear all devices in matrix (set to flap 0)
*/
void devicemgr_clearscreen()
{
    for (int ix = 0; ix < SFDEVICE_MAXDEV; ix++)
    {
        if (devices[ix].address > 0)
        {
            if (devices[ix].current_flap != 0)
            {
                devicemgr_setSingleRaw(ix, 0);
            }
        }
    }
}

/*
* Register new device in device manager and add it to device map
*
* @param rs485_descriptor rs485 file descriptor
* @param address device address
* @param x x position in device matrix
* @param y y position in device matrix
* @param nid optional device ID to use (set to -1 to auto assign)
* @return device ID assigned
*/
int devicemgr_register(int rs485_descriptor, u_int16_t address, int x, int y, int nid)
{
    if (nid < 0)
    {
        nextFreeSlot++;
        nid = nextFreeSlot;
    }
    log_message(LOG_INFO, "Register new device with addr %i at (%i,%i) with id %i", address, x, y, nid);

    devices[nid].pos_x = x;
    devices[nid].pos_y = y;
    devices[nid].address = address;
    devices[nid].calibration = 0;
    devices[nid].rs485_descriptor = rs485_descriptor;
    devices[nid].reg_voltage = 0;
    devices[nid].reg_counter = 0;
    devices[nid].reg_status = 0;
    devices[nid].current_flap = 0;
    devices[nid].deviceState = NEW;
    devices[nid].powerState = DISABLED;
    // try to reach device
    devicemgr_readStatus(nid);
    devicemgr_readCalib(nid);
    if (deviceMap[x][y] >= 0)
    { // rest old ones
        int old_id = deviceMap[x][y];
        devices[old_id].pos_x = -1;
        devices[old_id].pos_y = -1;
    }
    deviceMap[x][y] = nid;
    return nid;
}

/*
* Refresh all devices and update their status
*
* @return number of online devices
*/
int devicemgr_refresh()
{
    int devices_online = 0;
    for (int ix = 0; ix < SFDEVICE_MAXDEV; ix++)
    {
        if (devices[ix].address > 0)
        {
            devicemgr_readStatus(ix);
            if (devices[ix].deviceState == ONLINE)
            {
                devices_online++;
            }
        }
    }
    return devices_online;
}

/*
* Remove device from device manager
*
* @param id ID of device to remove
*/
int devicemgr_remove(int id)
{
    devices[nextFreeSlot].deviceState = REMOVED;
    devices[nextFreeSlot].address = 0;
    devices[nextFreeSlot].rs485_descriptor = -1;
    return 0;
}

/*
* Save device manager configuration to json file
*
* @param file path to json file
* @return 0 on success
*/
int devicemgr_save(char *file)
{
    json_object *root = json_object_new_object();
    json_object_object_add(root, "nextFreeSlot", json_object_new_int(nextFreeSlot));
    json_object *device_array = json_object_new_array();
    for (int ix = 0; ix < SFDEVICE_MAXDEV; ix++)
    {
        if (devices[ix].address > 0)
        {
            json_object *device = json_object_new_object();
            devicemgr_printDetails(ix, device);
            json_object_array_add(device_array, device);
        }
    }

    json_object_object_add(root, "devices", device_array);

    const char *data = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
    log_message(LOG_INFO, "store config data to %s\n", file);

    FILE *fptr;
    fptr = fopen(file, "w");
    fwrite(data, sizeof(char), strlen(data), fptr);
    fclose(fptr);
    return 0;
}

/*
* Load device manager configuration from json file and parse contents
* Verify all required keys are present and refresh device status
* 
* @param file path to json file
* @return 0 on success, -1 on error
*/
int devicemgr_load(char *file)
{
    FILE *fptr;
    char *line_in_file = malloc(JSON_MAX_LINE_LEN); // maximum of 256 bytes per line;

    log_message(LOG_INFO, "load config data from %s\n", file);

    fptr = fopen(file, "r");
    json_tokener *tok = json_tokener_new();
    json_object *jobj = NULL;
    int stringlen = 0;
    enum json_tokener_error jerr;

    do
    {
        char *read_ret = fgets(line_in_file, JSON_MAX_LINE_LEN, fptr); // read line from file
        stringlen = strlen(line_in_file);
        // printf("Read line with chars: %i : %s", stringlen, line_in_file); // only for testing
        jobj = json_tokener_parse_ex(tok, line_in_file, stringlen);
        if (read_ret == NULL)
        {
            break;
        }
    } while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
    if (jerr != json_tokener_success)
    {
        free(fptr); //free file pointer
        log_message(LOG_ERROR, "%s", json_tokener_error_desc(jerr));
        // Handle errors, as appropriate for your application.
        return -1;
    }

    // cleanup
    free(fptr); //free file pointer
    free(tok);  //free tokenizer

    // dump loadad data to terminal ( for tetsting)
    // char *data = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY);
    // printf("%s",data);

    // load data
    json_object *next_free;
    if (!json_object_object_get_ex(jobj, "nextFreeSlot", &next_free))
    {
        log_message(LOG_ERROR, "%s", "Key 'nextFreeSlot' not found.");
        return -1;
    }
    else
    {
        nextFreeSlot = json_object_get_int(next_free);
        free(next_free);
    }

    // clear config
    devicemgr_init(deviceFd);

    // load devices
    json_object *devices;
    if (!json_object_object_get_ex(jobj, "devices", &devices))
    {
        log_message(LOG_ERROR, "%s", "Key 'devices' not found.");
        return -1;
    }
    else
    {
        int arraylen = json_object_array_length(devices);
        for (int i = 0; i < arraylen; i++)
        {
            devicemgr_load_single(json_object_array_get_idx(devices, i));
        }

        free(devices);
    }
}

/*
* Load single device from json object and register it
* Verify all required keys are present
*
* @param device_obj json object with device data
* @return 0 on success, -1 on error
*/
int devicemgr_load_single(json_object *device_obj)
{
    json_object *jid = json_object_object_get(device_obj, "id");
    json_object *jaddr = json_object_object_get(device_obj, "address");
    json_object *jpos = json_object_object_get(device_obj, "position");
    json_object *jposx = json_object_object_get(jpos, "x");
    json_object *jposy = json_object_object_get(jpos, "y");
    // verify values are present
    if (jid == NULL)
    {
        log_message(LOG_ERROR, "Key 'device.%s' not found", "id");
        return -1;
    }
    if (jaddr == NULL)
    {
        log_message(LOG_ERROR, "Error: Key 'address.%s' not found", "id");
        return -1;
    }
    if (jposx == NULL)
    {
        log_message(LOG_ERROR, "Error: Key 'device.%s' not found", "position.x");
        return -1;
    }
    if (jposy == NULL)
    {
        log_message(LOG_ERROR, "Error: Key 'device.%s' not found", "position.y");
        return -1;
    }

    // create device
    devicemgr_register(deviceFd,
                       json_object_get_int(jaddr),
                       json_object_get_int(jposx),
                       json_object_get_int(jposy),
                       json_object_get_int(jid));
}