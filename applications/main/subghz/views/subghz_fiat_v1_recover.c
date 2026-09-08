#include "subghz_fiat_v1_recover.h"

#include <gui/elements.h>
#include <furi.h>
#include <lib/subghz/protocols/fiat_v1.h>

struct SubGhzViewFiatV1Recover {
    View* view;
    SubGhzViewFiatV1RecoverCallback callback;
    void* context;
};

typedef struct {
    uint8_t progress;
    uint32_t combos_tested;
    uint32_t combos_per_sec;
    uint32_t elapsed_sec;
    uint32_t eta_sec;
} SubGhzFiatV1RecoverModel;

static void subghz_view_fiat_v1_recover_format_count(char* buf, size_t len, uint32_t count) {
    if(count >= 1000000) {
        snprintf(buf, len, "%lu.%luM", count / 1000000, (count % 1000000) / 100000);
    } else if(count >= 1000) {
        snprintf(buf, len, "%luK", count / 1000);
    } else {
        snprintf(buf, len, "%lu", count);
    }
}

static void subghz_view_fiat_v1_recover_draw(Canvas* canvas, void* _model) {
    SubGhzFiatV1RecoverModel* model = (SubGhzFiatV1RecoverModel*)_model;

    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 2, AlignCenter, AlignTop, "Fiat V1 Recover");

    canvas_draw_rframe(canvas, 3, 15, 122, 12, 2);
    uint8_t fill = (uint8_t)((uint16_t)model->progress * 116 / 100);
    if(fill > 2) {
        canvas_draw_rbox(canvas, 5, 17, fill, 8, 1);
    } else if(fill > 0) {
        canvas_draw_box(canvas, 5, 17, fill, 8);
    }

    canvas_set_font(canvas, FontSecondary);

    char total_buf[12];
    subghz_view_fiat_v1_recover_format_count(
        total_buf, sizeof(total_buf), fiat_v1_hitag2_recovery_total());
    char tested_buf[12];
    subghz_view_fiat_v1_recover_format_count(tested_buf, sizeof(tested_buf), model->combos_tested);
    char combos_str[40];
    snprintf(
        combos_str,
        sizeof(combos_str),
        "%d%% - %s / %s combos",
        model->progress,
        tested_buf,
        total_buf);
    canvas_draw_str(canvas, 2, 38, combos_str);

    char speed_buf[12];
    subghz_view_fiat_v1_recover_format_count(
        speed_buf, sizeof(speed_buf), model->combos_per_sec);
    char speed_str[40];
    uint32_t eta_m = model->eta_sec / 60;
    uint32_t eta_s = model->eta_sec % 60;
    if(eta_m > 0) {
        snprintf(speed_str, sizeof(speed_str), "%s/sec  ETA %lum %lus", speed_buf, eta_m, eta_s);
    } else {
        snprintf(speed_str, sizeof(speed_str), "%s/sec  ETA %lus", speed_buf, eta_s);
    }
    canvas_draw_str(canvas, 2, 48, speed_str);

    char elapsed_str[24];
    uint32_t el_m = model->elapsed_sec / 60;
    uint32_t el_s = model->elapsed_sec % 60;
    if(el_m > 0) {
        snprintf(elapsed_str, sizeof(elapsed_str), "Elapsed: %lum %lus", el_m, el_s);
    } else {
        snprintf(elapsed_str, sizeof(elapsed_str), "Elapsed: %lus", el_s);
    }
    canvas_draw_str(canvas, 2, 58, elapsed_str);

    canvas_draw_str_aligned(canvas, 126, 64, AlignRight, AlignBottom, "Back: Cancel");
}

static bool subghz_view_fiat_v1_recover_input(InputEvent* event, void* context) {
    SubGhzViewFiatV1Recover* instance = (SubGhzViewFiatV1Recover*)context;

    if(event->key == InputKeyBack || event->key == InputKeyOk) {
        if(instance->callback) {
            instance->callback(SubGhzCustomEventViewTransmitterBack, instance->context);
        }
        return true;
    }
    return false;
}

SubGhzViewFiatV1Recover* subghz_view_fiat_v1_recover_alloc(void) {
    SubGhzViewFiatV1Recover* instance = malloc(sizeof(SubGhzViewFiatV1Recover));
    instance->view = view_alloc();
    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(SubGhzFiatV1RecoverModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, subghz_view_fiat_v1_recover_draw);
    view_set_input_callback(instance->view, subghz_view_fiat_v1_recover_input);

    with_view_model(
        instance->view,
        SubGhzFiatV1RecoverModel * model,
        {
            model->progress = 0;
            model->combos_tested = 0;
            model->combos_per_sec = 0;
            model->elapsed_sec = 0;
            model->eta_sec = 0;
        },
        false);

    return instance;
}

void subghz_view_fiat_v1_recover_free(SubGhzViewFiatV1Recover* instance) {
    furi_check(instance);
    view_free(instance->view);
    free(instance);
}

View* subghz_view_fiat_v1_recover_get_view(SubGhzViewFiatV1Recover* instance) {
    furi_check(instance);
    return instance->view;
}

void subghz_view_fiat_v1_recover_set_callback(
    SubGhzViewFiatV1Recover* instance,
    SubGhzViewFiatV1RecoverCallback callback,
    void* context) {
    furi_check(instance);
    instance->callback = callback;
    instance->context = context;
}

void subghz_view_fiat_v1_recover_update_stats(
    SubGhzViewFiatV1Recover* instance,
    uint8_t progress,
    uint32_t combos_tested,
    uint32_t combos_per_sec,
    uint32_t elapsed_sec,
    uint32_t eta_sec) {
    furi_check(instance);
    with_view_model(
        instance->view,
        SubGhzFiatV1RecoverModel * model,
        {
            model->progress = progress;
            model->combos_tested = combos_tested;
            model->combos_per_sec = combos_per_sec;
            model->elapsed_sec = elapsed_sec;
            model->eta_sec = eta_sec;
        },
        true);
}

void subghz_view_fiat_v1_recover_reset(SubGhzViewFiatV1Recover* instance) {
    furi_check(instance);
    with_view_model(
        instance->view,
        SubGhzFiatV1RecoverModel * model,
        {
            model->progress = 0;
            model->combos_tested = 0;
            model->combos_per_sec = 0;
            model->elapsed_sec = 0;
            model->eta_sec = 0;
        },
        false);
}
