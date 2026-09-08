#include "subghz_view_protocol_groups.h"
#include "../protocols/protocol_groups.h"
#include "../helpers/subghz_elements_compat.h"
#include <assets_icons.h>
#include <gui/elements.h>
#include <gui/icon.h>
#include <furi.h>
#include <stdio.h>

/* Full-width double-row boxes, 2 visible per page, one per protocol group -
 * same box geometry as fox_update_downloader/view_menu.c. Up/Down only move
 * the cursor highlight (a filled box) between rows; pressing OK on a row
 * makes IT the active RX group, marked with a filled-in 7x7 OK icon on the
 * left. Cursor and active group are independent - the cursor can be moved
 * around to browse without switching what Read is actually listening for. */
#define BOX_X          4
#define BOX_W          120
#define BOX_H          28
#define BOX_R          4
#define GROUPS_VISIBLE 2

#define ICON_GAP 3
#define TEXT_PAD 3

#define SCROLL_TIMER_PERIOD_MS 333

static const uint8_t k_slot_y[GROUPS_VISIBLE] = {2, 34};

typedef struct {
    uint8_t cursor;
    uint8_t active;
    size_t scroll_counter;
} SubGhzProtocolGroupsModel;

struct SubGhzProtocolGroups {
    View* view;
    FuriTimer* scroll_timer;
    bool scroll_running;
    SubGhzProtocolGroupsCallback callback;
    void* context;
};

static uint8_t protocol_groups_scroll_top(uint8_t cursor) {
    uint8_t max_top = SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT - GROUPS_VISIBLE;
    return (cursor > max_top) ? max_top : cursor;
}

static void protocol_groups_draw_cb(Canvas* canvas, void* model_ptr) {
    SubGhzProtocolGroupsModel* m = model_ptr;
    canvas_clear(canvas);

    uint8_t top = protocol_groups_scroll_top(m->cursor);
    char line2_buf[128];

    for(uint8_t slot = 0; slot < GROUPS_VISIBLE; slot++) {
        uint8_t idx = top + slot;
        if(idx >= SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT) break;
        bool at_cursor = (idx == m->cursor);
        bool is_active = (idx == m->active);
        uint8_t y = k_slot_y[slot];

        canvas_set_color(canvas, ColorBlack);
        if(at_cursor) {
            canvas_draw_rbox(canvas, BOX_X, y, BOX_W, BOX_H, BOX_R);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_draw_rframe(canvas, BOX_X, y, BOX_W, BOX_H, BOX_R);
        }

        uint8_t icon_x = BOX_X + TEXT_PAD;
        uint8_t icon_y = y + (BOX_H - icon_get_height(&I_ButtonCenter_7x7)) / 2;
        uint8_t text_x = icon_x + icon_get_width(&I_ButtonCenter_7x7) + ICON_GAP;
        uint8_t text_w = BOX_X + BOX_W - text_x - TEXT_PAD;

        /* Every row shows a radio-button-style indicator: filled OK icon
         * (a filled circle) for the active RX group, a matching hollow
         * circle outline otherwise - so it's clear every row is
         * selectable, not just the active one. */
        if(is_active) {
            canvas_draw_icon(canvas, icon_x, icon_y, &I_ButtonCenter_7x7);
        } else {
            canvas_draw_circle(
                canvas,
                icon_x + icon_get_width(&I_ButtonCenter_7x7) / 2,
                icon_y + icon_get_height(&I_ButtonCenter_7x7) / 2,
                icon_get_width(&I_ButtonCenter_7x7) / 2);
        }

        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, text_x, y + 11, subghz_garage_protocol_group_names[idx]);

        canvas_set_font(canvas, FontSecondary);
        snprintf(
            line2_buf, sizeof(line2_buf), "Protocols: %s",
            subghz_garage_protocol_group_members[idx]);
        /* Only the cursor row's text scrolls - a static row scrolling
         * unread text underneath it is just noise. */
        subghz_garage_scrollable_text_line_str(
            canvas, text_x, y + 23, text_w, line2_buf,
            at_cursor ? m->scroll_counter : 0, false, false);

        canvas_set_color(canvas, ColorBlack);
    }
}

static void protocol_groups_notify_active(SubGhzProtocolGroups* instance, uint8_t idx) {
    if(instance->callback) {
        instance->callback(instance->context, idx);
    }
}

static bool protocol_groups_input_cb(InputEvent* event, void* context) {
    SubGhzProtocolGroups* instance = context;
    if(event->type != InputTypeShort) return false;

    bool consumed = false;
    bool activated = false;
    uint8_t new_active = 0;

    with_view_model(
        instance->view,
        SubGhzProtocolGroupsModel* m,
        {
            if(event->key == InputKeyUp) {
                m->cursor = (m->cursor == 0) ?
                                (SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT - 1) :
                                m->cursor - 1;
                m->scroll_counter = 0;
                consumed = true;
            } else if(event->key == InputKeyDown) {
                m->cursor = (uint8_t)((m->cursor + 1) % SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT);
                m->scroll_counter = 0;
                consumed = true;
            } else if(event->key == InputKeyOk) {
                if(m->active != m->cursor) {
                    m->active = m->cursor;
                    activated = true;
                }
                new_active = m->active;
                consumed = true;
            }
        },
        consumed);

    if(activated) {
        protocol_groups_notify_active(instance, new_active);
    }

    return consumed;
}

static void protocol_groups_scroll_timer_cb(void* context) {
    SubGhzProtocolGroups* instance = context;
    with_view_model(
        instance->view, SubGhzProtocolGroupsModel* m, { m->scroll_counter++; }, true);
}

SubGhzProtocolGroups* subghz_protocol_groups_alloc(void) {
    SubGhzProtocolGroups* instance = malloc(sizeof(SubGhzProtocolGroups));
    instance->view = view_alloc();
    instance->callback = NULL;
    instance->context = NULL;
    view_set_context(instance->view, instance);
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(SubGhzProtocolGroupsModel));
    view_set_draw_callback(instance->view, protocol_groups_draw_cb);
    view_set_input_callback(instance->view, protocol_groups_input_cb);

    with_view_model(
        instance->view,
        SubGhzProtocolGroupsModel* m,
        {
            m->cursor = 0;
            m->active = 0;
            m->scroll_counter = 0;
        },
        false);

    /* Not started here - this view is allocated once and kept alive for
     * the rest of the app's life (see subghz_ensure_protocol_groups()), so
     * starting the timer at alloc time would leave it ticking in the
     * background - calling view_port_update() on a view that's no longer
     * the one on screen - for every scene visited afterward. This was the
     * cause of a ViewPort lockup warning firing on the way out of the
     * Protocol List screen. Started/stopped instead from the scene's
     * on_enter/on_exit via resume/pause below. */
    instance->scroll_timer = furi_timer_alloc(
        protocol_groups_scroll_timer_cb, FuriTimerTypePeriodic, instance);
    instance->scroll_running = false;

    return instance;
}

void subghz_protocol_groups_free(SubGhzProtocolGroups* instance) {
    furi_assert(instance);
    furi_timer_stop(instance->scroll_timer);
    furi_timer_free(instance->scroll_timer);
    view_free(instance->view);
    free(instance);
}

void subghz_protocol_groups_resume_scroll(SubGhzProtocolGroups* instance) {
    furi_assert(instance);
    if(instance->scroll_running) return;
    instance->scroll_running = true;
    furi_timer_start(instance->scroll_timer, SCROLL_TIMER_PERIOD_MS);
}

void subghz_protocol_groups_pause_scroll(SubGhzProtocolGroups* instance) {
    furi_assert(instance);
    if(!instance->scroll_running) return;
    instance->scroll_running = false;
    furi_timer_stop(instance->scroll_timer);
}

View* subghz_protocol_groups_get_view(SubGhzProtocolGroups* instance) {
    furi_assert(instance);
    return instance->view;
}

void subghz_protocol_groups_set_callback(
    SubGhzProtocolGroups* instance,
    SubGhzProtocolGroupsCallback callback,
    void* context) {
    furi_assert(instance);
    instance->callback = callback;
    instance->context = context;
}

void subghz_protocol_groups_set_selected(SubGhzProtocolGroups* instance, uint8_t group_index) {
    furi_assert(instance);
    if(group_index >= SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT) return;
    with_view_model(
        instance->view,
        SubGhzProtocolGroupsModel* m,
        {
            m->cursor = group_index;
            m->active = group_index;
            m->scroll_counter = 0;
        },
        false);
}
