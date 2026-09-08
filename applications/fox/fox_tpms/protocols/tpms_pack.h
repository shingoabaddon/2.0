#pragma once

#include "tpms_generic.h"

/** Re-pack generic's data word (and checksum/CRC) for protocol_name after
 * an edit, so the raw data matches the edited fields. Logs and no-ops if
 * protocol_name isn't a known TPMS protocol. */
void tpms_pack(const char* protocol_name, TPMSBlockGeneric* generic);
