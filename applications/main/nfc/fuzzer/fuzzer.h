#pragma once

#include <lib/nfc/nfc.h>

// nfc: Nfc instance already owned by the caller (e.g. the parent NFC app) -
// the fuzzer reuses it rather than acquiring the NFC hardware a second time.
void fuzzer_mifare_run(Nfc* nfc);
