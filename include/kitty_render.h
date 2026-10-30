/* High-resolution pixel rendering backend using the Kitty Graphics Protocol.
 *
 * Renders stars (with spectral color and magnitude-based radial glow),
 * anti-aliased constellation stick figures, azimuthal grids, planets, and
 * pixel-shaded lunar phases into an RGBA pixel buffer and transmits it to
 * GPU-accelerated terminals supporting the Kitty Graphics Protocol.
 */

#ifndef KITTY_RENDER_H
#define KITTY_RENDER_H

#include "core.h"

#include <curses.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct KittyCanvas
{
    int width;
    int height;
    uint8_t *pixels;
    char *b64_buf;
    size_t b64_cap;
};

/* Initialize an RGBA pixel canvas and preallocate its Base64 buffer.
 */
bool kitty_canvas_init(struct KittyCanvas *canvas, int width, int height);

/* Free all buffers owned by a KittyCanvas.
 */
void kitty_canvas_free(struct KittyCanvas *canvas);

/* Clear all pixels in the canvas to transparent black (0, 0, 0, 0).
 */
void kitty_canvas_clear(struct KittyCanvas *canvas);

/* Resize the canvas if dimensions changed. Returns false on allocation failure.
 */
bool kitty_canvas_resize(struct KittyCanvas *canvas, int width, int height);

/* Compute appropriate pixel dimensions for the canvas based on the ncurses
 * projection window and terminal pixel metrics.
 */
void kitty_compute_canvas_size(WINDOW *win, int *width_out, int *height_out);

/* Alpha-blend a single RGBA pixel onto the canvas at (x, y).
 */
void kitty_blend_pixel(struct KittyCanvas *canvas, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* Draw an anti-aliased line segment using Xiaolin Wu's line algorithm.
 */
void kitty_draw_line_aa(struct KittyCanvas *canvas, float x0, float y0, float x1, float y1, uint8_t r, uint8_t g, uint8_t b,
                        uint8_t a);

/* Draw an anti-aliased circle outline centered at (cx, cy).
 */
void kitty_draw_circle_aa(struct KittyCanvas *canvas, float cx, float cy, float radius, uint8_t r, uint8_t g, uint8_t b,
                          uint8_t a);

/* Draw a sub-pixel anti-aliased star/planet disc with soft radial glow.
 */
void kitty_draw_star_glow(struct KittyCanvas *canvas, float cx, float cy, float radius, uint8_t r, uint8_t g, uint8_t b,
                          uint8_t a);

/* Draw a pixel-shaded lunar disc showing the current illuminated phase.
 */
void kitty_draw_moon_phase(struct KittyCanvas *canvas, float cx, float cy, float radius, double moon_age, bool northern);

/* Encode `src_len` bytes into standard Base64 at `dst`. Returns encoded length.
 */
size_t kitty_encode_base64(const uint8_t *src, size_t src_len, char *dst);

/* Render the full celestial scene (grid, constellations, stars, planets, moon)
 * onto the RGBA pixel canvas.
 */
void render_kitty_scene(struct KittyCanvas *canvas, const struct Conf *config, struct Star *star_table, int num_stars,
                        const int *num_by_mag, const struct Constell *constell_table, int num_const,
                        const struct Planet *planet_table, struct Moon moon_object);

/* Transmit the RGBA canvas to the terminal using the Kitty Graphics Protocol
 * (z=-1 under text layer, anchored to `win`).
 */
void kitty_transmit_canvas(const struct KittyCanvas *canvas, WINDOW *win);

/* Delete all active Kitty graphics placements from the terminal screen.
 */
void kitty_clear_images(void);

#endif // KITTY_RENDER_H
