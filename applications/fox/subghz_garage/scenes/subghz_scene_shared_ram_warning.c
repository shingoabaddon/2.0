/**
 * @file subghz_scene_shared_ram_warning.c
 * @brief Shown once, the first time subghz_scene_start_on_enter() runs in
 *        this app session, if free heap is already below SUBGHZ_LOW_RAM_
 *        FREE_HEAP or qFlipper's screen-stream is already active (see
 *        subghz_scene_start.c) - a heads-up before the user ever reaches
 *        Read, rather than only discovering the low-RAM warning mid-Read.
 *        Internal scene/symbol names still say "shared_ram" - only the
 *        on-screen heading is "Flipper RAM".
 *
 * Heading "Flipper RAM", scrollable body text (fixed wording, one hand-
 * placed line per \n, each prefixed with the widget_element_text_scroll
 * control symbol "\ec" so it renders horizontally centered - see that
 * element's own source for the control-symbol mechanism), single OK-
 * button-styled "Continue" (GuiButtonTypeCenter - elements_button_center()
 * draws this with the standard 7x7 OK icon already, matching this
 * codebase's fixed 1-button info-page convention). Purely informational
 * itself - doesn't lock or suppress anything here at Start, so qFlipper's
 * screen-mirror keeps working normally everywhere outside Read/Read
 * RAW/Decode RAW. Inside those three, subghz_low_ram_mitigate() (subghz_
 * i.h) unconditionally disconnects CLI/RPC the moment the user commits to
 * the heavy operation (Start/Rec press, or equivalent) - deterministic,
 * not RAM-gated, so this screen's job is just to set that expectation up
 * front rather than let it be a surprise mid-Read. Back exits the whole
 * app straight to Desktop (same as subghz_scene_protocol_load_error.c),
 * not just this screen - there's no previous screen worth returning to
 * once the user has seen this.
 */

#include "../subghz_i.h"
#include "../helpers/subghz_custom_event.h"
#include <furi/core/memmgr.h>

static void subghz_scene_shared_ram_warning_widget_cb(
    GuiButtonType result,
    InputType type,
    void* context) {
    SubGhz* subghz = context;
    if(type != InputTypeShort) return;

    if(result == GuiButtonTypeCenter) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSharedRamWarningContinue);
    }
}

void subghz_scene_shared_ram_warning_on_enter(void* context) {
    SubGhz* subghz = context;

    subghz_ensure_widget(subghz);
    Widget* widget = subghz->widget;

    widget_add_string_element(
        widget, 64, 2, AlignCenter, AlignTop, FontPrimary, "Flipper RAM");

    const char* body = "\ecSub-GHz Garage App\n"
                        "\ecrequires the use of\n"
                        "\ecadditional RAM.\n"
                        "\ecFox will Disconnect\n"
                        "\ecqFlipper during Read and\n"
                        "\ecDecode and Reconnect\n"
                        "\ecautomatically on its own.";
    widget_add_text_scroll_element(widget, 0, 14, 128, 34, body);

    widget_add_button_element(
        widget,
        GuiButtonTypeCenter,
        "Continue",
        subghz_scene_shared_ram_warning_widget_cb,
        subghz);

    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewIdWidget);
}

bool subghz_scene_shared_ram_warning_on_event(void* context, SceneManagerEvent event) {
    SubGhz* subghz = context;

    if(event.type == SceneManagerEventTypeBack) {
        /* Straight to Desktop, not the previous scene - see file header. */
        scene_manager_stop(subghz->scene_manager);
        view_dispatcher_stop(subghz->view_dispatcher);
        return true;
    }

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubGhzCustomEventSharedRamWarningContinue) {
            scene_manager_previous_scene(subghz->scene_manager);
            return true;
        }
    }

    return false;
}

void subghz_scene_shared_ram_warning_on_exit(void* context) {
    SubGhz* subghz = context;
    if(subghz->widget) {
        widget_reset(subghz->widget);
    }
}
