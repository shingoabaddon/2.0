#include "lfrfid_dict_file.h"
#include <storage/storage.h>
#include <flipper_format/flipper_format.h>

#define LFRFID_DICT_FILETYPE "Flipper RFID key"

bool lfrfid_dict_file_save(ProtocolDict* dict, ProtocolId protocol, const char* filename) {
    furi_check(dict);
    furi_check(protocol != PROTOCOL_NO);
    furi_check(filename);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* file = flipper_format_file_alloc(storage);
    size_t data_size = protocol_dict_get_data_size(dict, protocol);
    uint8_t* data = malloc(data_size);
    bool result = false;

    do {
        if(!flipper_format_file_open_always(file, filename)) break;
        if(!flipper_format_write_header_cstr(file, LFRFID_DICT_FILETYPE, 1)) break;

        // TODO FL-3517: write comment about protocol types into file

        if(!flipper_format_write_string_cstr(
               file, "Key type", protocol_dict_get_name(dict, protocol)))
            break;

        // TODO FL-3517: write comment about protocol sizes into file

        protocol_dict_get_data(dict, protocol, data, data_size);

        if(!flipper_format_write_hex(file, "Data", data, data_size)) break;
        result = true;
    } while(false);

    flipper_format_free(file);
    furi_record_close(RECORD_STORAGE);
    free(data);

    return result;
}
