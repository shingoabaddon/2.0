#include "hardware_worker.h"
#include "furi.h"

#include <lib/ibutton/ibutton_worker.h>
#include <lib/ibutton/ibutton_key.h>

#define TAG "Fuzzer HW worker"

struct HardwareWorker {
    iButtonWorker* proto_worker;
    iButtonProtocolId protocol_id;
    iButtonProtocols* protocols_items;
    iButtonKey* key;
};

HardwareWorker* hardware_worker_alloc() {
    HardwareWorker* instance = malloc(sizeof(HardwareWorker));
    instance->protocols_items = ibutton_protocols_alloc();
    instance->key =
        ibutton_key_alloc(ibutton_protocols_get_max_data_size(instance->protocols_items));

    instance->proto_worker = ibutton_worker_alloc(instance->protocols_items);
    return instance;
}

void hardware_worker_free(HardwareWorker* instance) {
    ibutton_worker_free(instance->proto_worker);

    ibutton_key_free(instance->key);
    ibutton_protocols_free(instance->protocols_items);
    free(instance);
}

void hardware_worker_start_thread(HardwareWorker* instance) {
    ibutton_worker_start_thread(instance->proto_worker);
}

void hardware_worker_stop_thread(HardwareWorker* instance) {
    ibutton_worker_stop(instance->proto_worker);
    ibutton_worker_stop_thread(instance->proto_worker);
}

void hardware_worker_emulate_start(HardwareWorker* instance) {
    ibutton_worker_emulate_start(instance->proto_worker, instance->key);
}

void hardware_worker_stop(HardwareWorker* instance) {
    ibutton_worker_stop(instance->proto_worker);
}

void hardware_worker_set_protocol_data(
    HardwareWorker* instance,
    uint8_t* payload,
    uint8_t payload_size) {
    ibutton_key_set_protocol_id(instance->key, instance->protocol_id);
    iButtonEditableData data;
    ibutton_protocols_get_editable_data(instance->protocols_items, instance->key, &data);

    furi_check(payload_size >= data.size);
    memcpy(data.ptr, payload, data.size);
}

void hardware_worker_get_protocol_data(
    HardwareWorker* instance,
    uint8_t* payload,
    uint8_t payload_size) {
    iButtonEditableData data;
    ibutton_protocols_get_editable_data(instance->protocols_items, instance->key, &data);
    furi_check(payload_size >= data.size);
    memcpy(payload, data.ptr, data.size);
}

static bool hardware_worker_protocol_is_valid(HardwareWorker* instance) {
    if(instance->protocol_id != iButtonProtocolIdInvalid) {
        return true;
    }
    return false;
}

bool hardware_worker_set_protocol_id_by_name(HardwareWorker* instance, const char* protocol_name) {
    instance->protocol_id =
        ibutton_protocols_get_id_by_name(instance->protocols_items, protocol_name);
    return (instance->protocol_id != iButtonProtocolIdInvalid);
}

HwProtocolID hardware_worker_get_protocol_id(HardwareWorker* instance) {
    if(hardware_worker_protocol_is_valid(instance)) {
        return instance->protocol_id;
    }
    return -1;
}

bool hardware_worker_load_key_from_file(HardwareWorker* instance, const char* filename) {
    bool res = false;

    if(!ibutton_protocols_load(instance->protocols_items, instance->key, filename)) {
        FURI_LOG_W(TAG, "Cant load file");
    } else {
        instance->protocol_id = ibutton_key_get_protocol_id(instance->key);
        res = true;
    }

    return res;
}

bool hardware_worker_save_key(HardwareWorker* instance, const char* path) {
    furi_assert(instance);
    bool res;

    res = ibutton_protocols_save(instance->protocols_items, instance->key, path);

    return res;
}
