#include "devicemgr.h"
#include <json-c/json_object.h>
#include <string.h>

/*
 * This section provides an abstraction layer to access many devices
 * simultaneously. 
 * 
 * by Dennis Gunia - 2025 - www.dennisgunia.de
 */
enum SFDEVICE_STATE
{
    UNALLOCATED,
    NEW,
    OFFLINE,
    ONLINE,
    FAILED,
    REMOVED
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
    u_int16_t calibration;
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
    SFDEVICE_MAX_Y = 4,
    JSON_MAX_LINE_LEN = 256
};

// next free slot to register device
int nextFreeSlot = -1;
int deviceMap[SFDEVICE_MAX_X][SFDEVICE_MAX_Y];
int deviceFd;
struct SFDEVICE devices[SFDEVICE_MAXDEV];

const char *symbols[45] = {" ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
                           "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Ä", "Ö", "Ü",
                           "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ".", "-", "?", "!"};

void devicemgr_init(int fd)
{
    deviceFd = fd;
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
        devices[ix].deviceState = UNALLOCATED;
    }
}

int devicemgr_readStatus(int device_id)
{
    if (devices[device_id].address > 0)
    { // only if defined
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
    else
    {
        return -2;
    }
}

int devicemgr_readCalib(int device_id)
{
    if (devices[device_id].deviceState == ONLINE)
    {
        char *buffer_r = malloc(256);
        if (sfbus_read_eeprom(devices[device_id].rs485_descriptor, devices[device_id].address, buffer_r) > 0)
        {
            uint16_t calib_data = (*(buffer_r + 2) & 0xFF | ((*(buffer_r + 3) << 8) & 0xFF00));
            devices[device_id].calibration = calib_data;
            free(buffer_r);
        }
        else
        {
            printf("Error reading eeprom from %i\n", device_id);
            free(buffer_r);
            return -1;
        }
    }
    else
    {
        return -2;
    }
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
    case REMOVED:
        json_object_object_add(status, "device", json_object_new_string("REMOVED"));
        break;
    default:
        json_object_object_add(status, "device", json_object_new_string("UNALLOCATED"));
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
}

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

void setSingle(int id, char flap)
{
    // first convert char to flap id
    char test_char = toupper(flap);
    printf("find char %c\n", test_char);
    for (int ix = 0; ix < 45; ix++)
    {
        if (*symbols[ix] == test_char)
        {
            printf("match char %i %i %i\n", test_char, *symbols[ix], ix);
            sfbus_display_full(devices[id].rs485_descriptor, devices[id].address, ix);
            break;
        }
    }
    devices[nextFreeSlot].current_flap = flap;
}

void setSingleRaw(int id, int flap)
{
    sfbus_display_full(devices[id].rs485_descriptor, devices[id].address, flap);
    devices[nextFreeSlot].current_flap = flap;
}

void devicemgr_printText(char *text, int x, int y)
{
    for (int i = 0; i < strlen(text); i++)
    {
        int this_id = deviceMap[x + i][y];
        if (this_id >= 0)
        {
            printf("print char %c to %i\n", *(text + i), devices[this_id].address);

            setSingle(this_id, *(text + i));
        }
    }
}

void devicemgr_printFlap(int flap, int x, int y)
{
    int this_id = deviceMap[x][y];
    if (this_id >= 0)
    {
        setSingleRaw(this_id, flap);
    }
}

int devicemgr_register(int rs485_descriptor, u_int16_t address, int x, int y, int nid)
{
    if (nid < 0)
    {
        nextFreeSlot++;
        nid = nextFreeSlot;
    }

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

// refreshes status of all devices
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

// remove devices from system
int devicemgr_remove(int id)
{
    devices[nextFreeSlot].deviceState = REMOVED;
    devices[nextFreeSlot].address = 0;
    devices[nextFreeSlot].rs485_descriptor = NULL;
    return 0;
}

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

    char *data = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);
    printf("[INFO][console] store data to %s\n", file);

    FILE *fptr;
    fptr = fopen(file, "w");
    fwrite(data, sizeof(char), strlen(data), fptr);
    fclose(fptr);
}

int devicemgr_load(char *file)
{
    FILE *fptr;
    const char *line_in_file = malloc(JSON_MAX_LINE_LEN); // maximum of 256 bytes per line;
    fptr = fopen(file, "r");
    json_tokener *tok = json_tokener_new();
    json_object *jobj = NULL;
    int stringlen = 0;
    enum json_tokener_error jerr;

    do
    {
        int read_ret = fgets(line_in_file, JSON_MAX_LINE_LEN, fptr); // read line from file
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
        fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
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
        fprintf(stderr, "Error: %s\n", "Key 'nextFreeSlot' not found.");
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
        fprintf(stderr, "Error: %s\n", "Key 'devices' not found.");
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
        fprintf(stderr, "Error: Key 'device.%s' not found\n", "id");
        return -1;
    }
    if (jaddr == NULL)
    {
        fprintf(stderr, "Error: Key 'address.%s' not found\n", "id");
        return -1;
    }
    if (jposx == NULL)
    {
        fprintf(stderr, "Error: Key 'device.%s' not found\n", "position.x");
        return -1;
    }
    if (jposy == NULL)
    {
        fprintf(stderr, "Error: Key 'device.%s' not found\n", "position.y");
        return -1;
    }

    // create device
    devicemgr_register(deviceFd,
                       json_object_get_int(jaddr),
                       json_object_get_int(jposx),
                       json_object_get_int(jposy),
                       json_object_get_int(jid));
}