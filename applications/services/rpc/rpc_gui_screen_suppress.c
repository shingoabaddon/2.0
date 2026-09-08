#include "rpc_gui_screen_suppress.h"

#include <furi.h>
#include <string.h>
#include <u8g2.h>
#include <toolbox/compress.h>
#include <gui/icon.h>
#include <gui/canvas_i.h>
#include <assets_icons.h>

#define TAG "RpcGuiScreenSuppress"

/* 128x64, 1bpp, u8g2's own vertical-top-LSB tile packing - matches
 * gui_get_framebuffer_size()/canvas_get_buffer_size() for this display.
 * A hardcoded size is fine here (this display size is a hardware
 * constant on this platform, same as GUI_DISPLAY_WIDTH/HEIGHT in
 * gui_i.h) - get_placeholder_frame() below still checks the caller's
 * buffer_size against it rather than assuming. */
#define PLACEHOLDER_BUFFER_WIDTH  128u
#define PLACEHOLDER_BUFFER_HEIGHT 64u
#define PLACEHOLDER_BUFFER_SIZE   (PLACEHOLDER_BUFFER_WIDTH * PLACEHOLDER_BUFFER_HEIGHT / 8u)
#define PLACEHOLDER_TILE_HEIGHT   (PLACEHOLDER_BUFFER_HEIGHT / 8u)

/* Not exposed via u8g2_glue.h (only the combined u8g2_Setup_st756x_
 * flipper() wrapper is) - declared here so this module can drive its own
 * private, non-hardware u8g2_t instance directly. Never actually invokes
 * any hardware transaction (see build_placeholder_frame()'s comment) -
 * this is purely to get the same buffer geometry stock canvas.c uses. */
extern uint8_t u8x8_d_st756x_flipper(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

static bool s_suppressed = false;
static uint16_t s_active_stream_count = 0;

static bool s_placeholder_ready = false;
static uint8_t s_placeholder_buf[PLACEHOLDER_BUFFER_SIZE];

void rpc_gui_screen_stream_set_suppressed(bool suppressed) {
    s_suppressed = suppressed;
}

bool rpc_gui_screen_stream_is_suppressed(void) {
    return s_suppressed;
}

void rpc_gui_screen_stream_mark_active(bool active) {
    if(active) {
        s_active_stream_count++;
    } else if(s_active_stream_count > 0) {
        s_active_stream_count--;
    }
}

bool rpc_gui_screen_stream_is_active(void) {
    return s_active_stream_count > 0;
}

/* Renders the placeholder into a private, own-buffer u8g2_t - NOT the
 * real display's shared tile buffer (u8g2_Setup_st756x_flipper() itself
 * always points at that single shared buffer via u8g2_m_16_8_f(), so it
 * can't be reused for a second instance without corrupting the real
 * screen). u8g2_SetupDisplay()/u8x8_Setup() only populates geometry from
 * the display driver's own U8X8_MSG_DISPLAY_SETUP_MEMORY handler - pure
 * memory setup, no communication - confirmed via u8x8_setup.c's own doc
 * comment: "This setup will not communicate with the display itself."
 * Nothing here ever calls u8g2_InitDisplay()/u8g2_SendBuffer(), which are
 * the only calls that would actually touch SPI/GPIO - byte_cb/gpio_cb
 * are passed as u8x8_dummy_cb specifically so there's no path to real
 * hardware even if that assumption is ever wrong. */
static bool build_placeholder_frame(void) {
    memset(s_placeholder_buf, 0, sizeof(s_placeholder_buf));

    u8g2_t u8g2;
    memset(&u8g2, 0, sizeof(u8g2));
    u8g2_SetupDisplay(&u8g2, u8x8_d_st756x_flipper, u8x8_cad_001, u8x8_dummy_cb, u8x8_dummy_cb);
    u8g2_SetupBuffer(
        &u8g2,
        s_placeholder_buf,
        (uint8_t)PLACEHOLDER_TILE_HEIGHT,
        u8g2_ll_hvline_vertical_top_lsb,
        U8G2_R0);

    CompressIcon* compress_icon = compress_icon_alloc(ICON_DECOMPRESSOR_BUFFER_SIZE);
    if(compress_icon) {
        uint8_t* icon_data = NULL;
        compress_icon_decode(compress_icon, icon_get_frame_data(&I_fox_32x32, 0), &icon_data);
        if(icon_data) {
            canvas_draw_u8g2_bitmap(
                &u8g2,
                4,
                16,
                icon_get_width(&I_fox_32x32),
                icon_get_height(&I_fox_32x32),
                icon_data,
                IconRotation0);
        }
        compress_icon_free(compress_icon);
    } else {
        FURI_LOG_W(TAG, "build_placeholder_frame: icon decompressor alloc failed, text only");
    }

    u8g2_SetFontDirection(&u8g2, 0);
    u8g2_SetFontMode(&u8g2, 1);
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);

    /* Short, large message, centered both horizontally and vertically in
     * the space to the right of the icon (icon spans x=4..36) - same
     * AlignCenter/AlignCenter math canvas_draw_str_aligned() uses, applied
     * manually since this is a private, non-Canvas u8g2_t instance,
     * generalized to however many lines are in the array below. */
    const char* lines[] = {"Sub-GHz", "Operation", "in", "Progress!"};
    const size_t line_count = COUNT_OF(lines);
    int32_t text_area_center_x = (40 + PLACEHOLDER_BUFFER_WIDTH) / 2;
    int32_t line_gap = 16;
    int32_t center_y = PLACEHOLDER_BUFFER_HEIGHT / 2;
    int32_t first_y = center_y - (((int32_t)line_count - 1) * line_gap) / 2 +
                       (u8g2_GetAscent(&u8g2) / 2);

    for(size_t i = 0; i < line_count; i++) {
        int32_t y = first_y + (int32_t)i * line_gap;
        u8g2_DrawStr(
            &u8g2, text_area_center_x - (u8g2_GetStrWidth(&u8g2, lines[i]) / 2), y, lines[i]);
    }

    return true;
}

bool rpc_gui_screen_stream_get_placeholder_frame(uint8_t* out_buffer, size_t buffer_size) {
    furi_check(out_buffer);
    if(buffer_size != PLACEHOLDER_BUFFER_SIZE) {
        FURI_LOG_E(
            TAG,
            "get_placeholder_frame: buffer_size %zu != expected %u",
            buffer_size,
            (unsigned)PLACEHOLDER_BUFFER_SIZE);
        return false;
    }

    if(!s_placeholder_ready) {
        s_placeholder_ready = build_placeholder_frame();
        if(!s_placeholder_ready) return false;
    }

    memcpy(out_buffer, s_placeholder_buf, buffer_size);
    return true;
}
