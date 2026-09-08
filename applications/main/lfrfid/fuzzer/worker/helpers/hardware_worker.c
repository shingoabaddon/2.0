#include "hardware_worker.h"
#include "furi.h"

#include <lfrfid/lfrfid_dict_file.h>
#include <lfrfid/lfrfid_worker.h>

#define TAG "Fuzzer HW worker"

struct HardwareWorker {
    LFRFIDWorker* proto_worker;
    ProtocolId protocol_id;
    ProtocolDict* protocols_items;
};

HardwareWorker* hardware_worker_alloc() {
    HardwareWorker* instance = malloc(sizeof(HardwareWorker));
    instance->protocols_items = protocol_dict_alloc(lfrfid_protocols, LFRFIDProtocolMax);

    instance->proto_worker = lfrfid_worker_alloc(instance->protocols_items);
    return instance;
}

void hardware_worker_free(HardwareWorker* instance) {
    lfrfid_worker_free(instance->proto_worker);

    protocol_dict_free(instance->protocols_items);
    free(instance);
}

void hardware_worker_start_thread(HardwareWorker* instance) {
    lfrfid_worker_start_thread(instance->proto_worker);
}

void hardware_worker_stop_thread(HardwareWorker* instance) {
    lfrfid_worker_stop(instance->proto_worker);
    lfrfid_worker_stop_thread(instance->proto_worker);
}

void hardware_worker_emulate_start(HardwareWorker* instance) {
    lfrfid_worker_emulate_start(instance->proto_worker, instance->protocol_id);
}

void hardware_worker_stop(HardwareWorker* instance) {
    lfrfid_worker_stop(instance->proto_worker);
}

void hardware_worker_set_protocol_data(
    HardwareWorker* instance,
    uint8_t* payload,
    uint8_t payload_size) {
    protocol_dict_set_data(
        instance->protocols_items, instance->protocol_id, payload, payload_size);
}

void hardware_worker_get_protocol_data(
    HardwareWorker* instance,
    uint8_t* payload,
    uint8_t payload_size) {
    protocol_dict_get_data(
        instance->protocols_items, instance->protocol_id, payload, payload_size);
}

static bool hardware_worker_protocol_is_valid(HardwareWorker* instance) {
    if(instance->protocol_id != PROTOCOL_NO) {
        return true;
    }
    return false;
}

bool hardware_worker_set_protocol_id_by_name(HardwareWorker* instance, const char* protocol_name) {
    instance->protocol_id =
        protocol_dict_get_protocol_by_name(instance->protocols_items, protocol_name);
    return (instance->protocol_id != PROTOCOL_NO);
}

HwProtocolID hardware_worker_get_protocol_id(HardwareWorker* instance) {
    if(hardware_worker_protocol_is_valid(instance)) {
        return instance->protocol_id;
    }
    return -1;
}

bool hardware_worker_load_key_from_file(HardwareWorker* instance, const char* filename) {
    bool res = false;

    ProtocolId loaded_proto_id = lfrfid_dict_file_load(instance->protocols_items, filename);
    if(loaded_proto_id == PROTOCOL_NO) {
        FURI_LOG_W(TAG, "Cant load file");
    } else {
        instance->protocol_id = loaded_proto_id;
        res = true;
    }

    return res;
}

bool hardware_worker_save_key(HardwareWorker* instance, const char* path) {
    furi_assert(instance);
    bool res;

    res = lfrfid_dict_file_save(instance->protocols_items, instance->protocol_id, path);

    return res;
}
