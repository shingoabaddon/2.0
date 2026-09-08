#pragma once
/* Wraps cli_vcp_session_lock()/cli_vcp_session_unlock() - FoxFW2.0's own
 * addition to the stock applications/services/cli/cli_vcp.h service header
 * (present everywhere, so __has_include can't tell forks apart). Gated by
 * SUBGHZ_GARAGE_HAS_CLI_VCP_LOCK, a cdefine set only on FoxFW2.0's own
 * native build (see application.fam; stripped for the _COMPATIBLE
 * cross-fork variant by sync_apps_from_foxfw.ps1).
 *
 * IMPORTANT, not cosmetic: subghz.c's own comments (subghz_lock_cli_
 * sessions(), subghz_cli_soft_lock()) document that skipping this call and
 * relying on hard USB teardown alone was already tried and caused a real
 * OOM crash after repeated Read/Read RAW visits - the hard USB teardown
 * disconnects the interface, but leaves CLI VCP's own internal state
 * (is_connected, shell, pipes) stale, leaking a little more every cycle.
 * No fork without this hook has an equivalent, and the two previously-tried
 * alternatives (cli_vcp_disable(), raw furi_hal_usb calls) are separately
 * documented as corrupting CLI VCP state badly enough to need a reboot -
 * so there is no safe substitute to fall back to here. On forks without
 * SUBGHZ_GARAGE_HAS_CLI_VCP_LOCK, the hard USB teardown still runs (it's
 * plain furi_hal_usb, not gated by this), but this specific leak-over-
 * repeated-cycles risk is NOT mitigated - a real, known gap on those
 * forks, not just a reduced-fidelity one. */

#include <cli/cli_vcp.h>

#ifdef SUBGHZ_GARAGE_HAS_CLI_VCP_LOCK

static inline void subghz_garage_cli_vcp_session_lock(CliVcp* cli_vcp) {
    cli_vcp_session_lock(cli_vcp);
}

static inline void subghz_garage_cli_vcp_session_unlock(CliVcp* cli_vcp) {
    cli_vcp_session_unlock(cli_vcp);
}

#else

static inline void subghz_garage_cli_vcp_session_lock(CliVcp* cli_vcp) {
    UNUSED(cli_vcp);
}

static inline void subghz_garage_cli_vcp_session_unlock(CliVcp* cli_vcp) {
    UNUSED(cli_vcp);
}

#endif
