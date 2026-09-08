#pragma once

/* XBM <-> monochrome BMP conversion for Fox XBM Converter.
 *
 * Both formats are read/written through a shared internal convention: a
 * top-down, row-major, 1-bit-per-pixel buffer where a set bit (1) means a
 * BLACK pixel, packed LSB-first within each byte, each row starting a new
 * byte (row stride = ceil(width / 8)). This is exactly the layout Flipper's
 * own canvas_draw_xbm() expects, and exactly what FFB's screenshot XBM
 * parser (ffv_parse_xbm in fox_file_browser/ffb.c) produces - so an .xbm
 * this app writes can be redrawn with canvas_draw_xbm() unmodified, and an
 * .xbm it reads is interpreted the same way FFB's own image viewer does.
 *
 * XBM text parsing/writing and BMP reading/writing each convert to/from
 * that shared buffer independently, so every combination (including BMPs
 * with an inverted 2-color palette, or 4/8/24/32bpp source BMPs thresholded
 * by luminance) goes through the same well-defined middle format.
 */

#include <stdint.h>
#include <stdbool.h>
#include <storage/storage.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XbmBmpOk = 0,
    XbmBmpErrorOpenSource, /* couldn't open/read the source file */
    XbmBmpErrorOpenDest, /* couldn't create/write the destination file */
    XbmBmpErrorBadFormat, /* source file isn't a format we understand */
    XbmBmpErrorTooLarge, /* image exceeds our memory-safety size cap */
    XbmBmpErrorOutOfMemory, /* malloc() failed */
} XbmBmpResult;

/* Returns a short, user-facing description of a result code (never NULL). */
const char* xbm_bmp_result_text(XbmBmpResult result);

/* Called periodically during conversion with progress in [0, 100]. */
typedef void (*XbmBmpProgressCb)(uint8_t percent, void* context);

/* Reads the text-format XBM file at `source_path` and writes a 1-bit
 * (monochrome, 2-color-palette) BMP file to `dest_path`, overwriting it if
 * it already exists. Width/height are read from the file's
 * "#define ..._width"/"#define ..._height" lines when present, falling
 * back to 128x64 (FoxFW's screenshot size) when they're missing. */
XbmBmpResult xbm_to_bmp_convert(
    Storage* storage,
    const char* source_path,
    const char* dest_path,
    XbmBmpProgressCb progress_cb,
    void* progress_context);

/* Reads the BMP file at `source_path` (1/4/8/24/32 bits per pixel; 4/8-bit
 * uses the palette, 24/32-bit is thresholded by luminance) and writes a
 * text-format XBM file to `dest_path`, overwriting it if it already
 * exists. The array/#define names are derived from dest_path's basename. */
XbmBmpResult bmp_to_xbm_convert(
    Storage* storage,
    const char* source_path,
    const char* dest_path,
    XbmBmpProgressCb progress_cb,
    void* progress_context);

#ifdef __cplusplus
}
#endif
