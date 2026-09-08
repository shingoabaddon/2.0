#pragma once
#include <toolbox/protocols/protocol.h>
#include "t5577.h"
#include "em4305.h"
#include "hitagmicro.h"

// Private copy of applications/main/lfrfid's protocol registry, trimmed to
// just the 3 protocols picopass_device.c uses to export a PACS credential as
// an lfrfid-compatible file. lfrfid_dict_file_save()/lfrfid_protocols[] used
// to be resolvable via the firmware SDK; they no longer are now that
// lib/lfrfid lives privately inside lfrfid.fap, so picopass needs its own
// copy instead.

typedef enum {
    LFRFIDFeatureASK = 1 << 0, /** ASK Demodulation */
    LFRFIDFeaturePSK = 1 << 1, /** PSK Demodulation */
} LFRFIDFeature;

typedef enum {
    PPLfrfidProtocolH10301,
    PPLfrfidProtocolHidGeneric,
    PPLfrfidProtocolHidExGeneric,

    PPLfrfidProtocolMax,
} PPLfrfidProtocol;

extern const ProtocolBase* const pp_lfrfid_protocols[];

typedef enum {
    LFRFIDWriteTypeT5577,
    LFRFIDWriteTypeEM4305,
    LFRFIDWriteTypeHitagMicro,

    LFRFIDWriteTypeMax,
} LFRFIDWriteType;

typedef struct {
    LFRFIDWriteType write_type;
    union {
        LFRFIDT5577 t5577;
        LFRFIDEM4305 em4305;
        LFRFIDHitagMicro hitagmicro;
    };
} LFRFIDWriteRequest;
