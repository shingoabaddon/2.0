#include "lfrfid_protocols.h"
#include "protocol_h10301.h"
#include "protocol_hid_generic.h"
#include "protocol_hid_ex_generic.h"

const ProtocolBase* const pp_lfrfid_protocols[] = {
    [PPLfrfidProtocolH10301] = &protocol_h10301,
    [PPLfrfidProtocolHidGeneric] = &protocol_hid_generic,
    [PPLfrfidProtocolHidExGeneric] = &protocol_hid_ex_generic,
};
