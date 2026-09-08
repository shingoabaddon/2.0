#pragma once

#if __has_include("rpc/rpc_gui_screen_suppress.h")
#include "rpc/rpc_gui_screen_suppress.h"
#else

/* FoxFW2.0's shared qFlipper screen-stream-suppression service (see the
 * real header's doc comment) isn't available on this firmware - fall back
 * to no-ops. Screen-stream suppression during RAM-sensitive Read/Decode
 * RAW simply doesn't happen here; the RAM-safety guard logic itself
 * (RSSI-based backoff, storage checks, Low RAM Warning, etc.) is fully
 * unaffected either way. */

#include <stdbool.h>

static inline void rpc_gui_screen_stream_set_suppressed(bool suppressed) {
    (void)suppressed;
}

static inline bool rpc_gui_screen_stream_is_suppressed(void) {
    return false;
}

static inline bool rpc_gui_screen_stream_is_active(void) {
    return false;
}

#endif
