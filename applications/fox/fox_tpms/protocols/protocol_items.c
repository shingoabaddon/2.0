#include "protocol_items.h"

const SubGhzProtocol* tpms_protocol_registry_items[] = {
    &tpms_protocol_schrader_gg4,
    &tpms_protocol_ford,
    &tpms_protocol_renault,
    &tpms_protocol_citroen,
    &tpms_protocol_pmv107j,
};

const SubGhzProtocolRegistry tpms_protocol_registry = {
    .items = tpms_protocol_registry_items,
    .size = COUNT_OF(tpms_protocol_registry_items)};
