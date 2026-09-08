#include "tpms_pack.h"

#include <furi.h>
#include "schrader_gg4.h"
#include "tpms_ford.h"
#include "tpms_citroen.h"
#include "tpms_pmv107j.h"
#include "tpms_renault.h"

#define TAG "TpmsPack"

void tpms_pack(const char* protocol_name, TPMSBlockGeneric* generic) {
    furi_assert(generic);
    if(!protocol_name) return;

    if(!strcmp(protocol_name, TPMS_PROTOCOL_SCHRADER_GG4_NAME)) {
        tpms_protocol_schrader_gg4_pack(generic);
    } else if(!strcmp(protocol_name, TPMS_PROTOCOL_FORD_NAME)) {
        tpms_protocol_ford_pack(generic);
    } else if(!strcmp(protocol_name, TPMS_PROTOCOL_CITROEN_NAME)) {
        tpms_protocol_citroen_pack(generic);
    } else if(!strcmp(protocol_name, TPMS_PROTOCOL_PMV107J_NAME)) {
        tpms_protocol_pmv107j_pack(generic);
    } else if(!strcmp(protocol_name, TPMS_PROTOCOL_RENAULT_NAME)) {
        tpms_protocol_renault_pack(generic);
    } else {
        FURI_LOG_E(TAG, "No pack() for protocol '%s'", protocol_name);
    }
}
