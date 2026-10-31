#include "kitty_render.h"

#include "astro.h"
#include "core.h"
#include "core_render.h"
#include "macros.h"
#include "term.h"

#include <curses.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

#define KITTY_CHUNK_SIZE 4096

static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t kitty_encode_base64(const uint8_t *src, size_t src_len, char *dst)
{
    size_t i = 0;
    size_t j = 0;

    while (i + 2 < src_len)
    {
        uint32_t triple = ((uint32_t)src[i] << 16) | ((uint32_t)src[i + 1] << 8) | (uint32_t)src[i + 2];
        dst[j++] = b64_table[(triple >> 18) & 0x3F];
        dst[j++] = b64_table[(triple >> 12) & 0x3F];
        dst[j++] = b64_table[(triple >> 6) & 0x3F];
        dst[j++] = b64_table[triple & 0x3F];
        i += 3;
    }

    size_t rem = src_len - i;
    if (rem == 1)
    {
        uint32_t triple = (uint32_t)src[i] << 16;
        dst[j++] = b64_table[(triple >> 18) & 0x3F];
        dst[j++] = b64_table[(triple >> 12) & 0x3F];
        dst[j++] = '=';
        dst[j++] = '=';
    }
    else if (rem == 2)
    {
        uint32_t triple = ((uint32_t)src[i] << 16) | ((uint32_t)src[i + 1] << 8);
        dst[j++] = b64_table[(triple >> 18) & 0x3F];
        dst[j++] = b64_table[(triple >> 12) & 0x3F];
        dst[j++] = b64_table[(triple >> 6) & 0x3F];
        dst[j++] = '=';
    }

    dst[j] = '\0';
    return j;
}

bool kitty_canvas_init(struct KittyCanvas *canvas, int width, int height)
{
    if (canvas == NULL || width <= 0 || height <= 0)
    {
        return false;
    }

    size_t byte_len = (size_t)width * (size_t)height * 4;
    size_t b64_cap = ((byte_len + 2) / 3) * 4 + 1;

    uint8_t *pixels = calloc(byte_len, sizeof(uint8_t));
    if (pixels == NULL)
    {
        return false;
    }

    uint8_t *prev_pixels = calloc(byte_len, sizeof(uint8_t));
    if (prev_pixels == NULL)
    {
        free(pixels);
        return false;
    }

    char *b64_buf = malloc(b64_cap);
    if (b64_buf == NULL)
    {
        free(prev_pixels);
        free(pixels);
        return false;
    }

    memset(canvas, 0, sizeof(*canvas));
    canvas->width = width;
    canvas->height = height;
    canvas->pixels = pixels;
    canvas->prev_pixels = prev_pixels;
    canvas->b64_buf = b64_buf;
    canvas->b64_cap = b64_cap;
    return true;
}

void kitty_canvas_free(struct KittyCanvas *canvas)
{
    if (canvas == NULL)
    {
        return;
    }
#ifndef _WIN32
    for (int i = 0; i < KITTY_FILE_SLOTS; ++i)
    {
        if (canvas->temp_paths[i][0] != '\0')
        {
            unlink(canvas->temp_paths[i]);
            canvas->temp_paths[i][0] = '\0';
        }
    }
#endif
    free(canvas->pixels);
    free(canvas->prev_pixels);
    free(canvas->b64_buf);
    memset(canvas, 0, sizeof(*canvas));
}

void kitty_canvas_clear(struct KittyCanvas *canvas)
{
    if (canvas == NULL || canvas->pixels == NULL)
    {
        return;
    }
    memset(canvas->pixels, 0, (size_t)canvas->width * (size_t)canvas->height * 4);
    canvas->scene_valid = false;
    canvas->has_transmitted = false;
}

bool kitty_canvas_resize(struct KittyCanvas *canvas, int width, int height)
{
    if (canvas == NULL || width <= 0 || height <= 0)
    {
        return false;
    }
    if (canvas->pixels != NULL && canvas->width == width && canvas->height == height)
    {
        canvas->scene_valid = false;
        canvas->has_transmitted = false;
        return true;
    }
    struct KittyCanvas new_canvas;
    if (kitty_canvas_init(&new_canvas, width, height))
    {
        kitty_canvas_free(canvas);
        *canvas = new_canvas;
        return true;
    }
    return false;
}

void kitty_compute_canvas_size(WINDOW *win, int *width_out, int *height_out)
{
    if (width_out == NULL || height_out == NULL)
    {
        return;
    }

    int win_h = 40;
    int win_w = 80;
    if (win != NULL)
    {
        getmaxyx(win, win_h, win_w);
    }

    int term_rows = LINES > 0 ? LINES : 40;
    int term_cols = COLS > 0 ? COLS : 80;
    int ypixel = 0;
    int xpixel = 0;

    int dim;
    if (term_pixel_size(&ypixel, &xpixel) && term_rows > 0 && term_cols > 0)
    {
        int win_px_h = (ypixel * win_h) / term_rows;
        int win_px_w = (xpixel * win_w) / term_cols;
        dim = MIN(win_px_h, win_px_w);
    }
    else
    {
        dim = win_h * 10;
    }

    if (dim < KITTY_MIN_CANVAS_DIM)
    {
        dim = KITTY_MIN_CANVAS_DIM;
    }
    if (dim > KITTY_MAX_CANVAS_DIM)
    {
        dim = KITTY_MAX_CANVAS_DIM;
    }

    *width_out = dim;
    *height_out = dim;
}

void kitty_blend_pixel(struct KittyCanvas *canvas, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (canvas == NULL || canvas->pixels == NULL || a == 0)
    {
        return;
    }
    if (x < 0 || x >= canvas->width || y < 0 || y >= canvas->height)
    {
        return;
    }

    uint8_t *px = &canvas->pixels[((size_t)y * (size_t)canvas->width + (size_t)x) * 4];
    if (a == 255 || px[3] == 0)
    {
        px[0] = r;
        px[1] = g;
        px[2] = b;
        px[3] = a;
        return;
    }

    // Standard Porter-Duff source-over compositing
    uint32_t src_a = a;
    uint32_t inv_a = 255 - src_a;
    uint32_t dst_a = px[3];
    uint32_t out_a = src_a + (dst_a * inv_a) / 255;

    if (out_a > 0)
    {
        px[0] = (uint8_t)MIN(255U, (r * src_a + (px[0] * dst_a * inv_a) / 255) / out_a);
        px[1] = (uint8_t)MIN(255U, (g * src_a + (px[1] * dst_a * inv_a) / 255) / out_a);
        px[2] = (uint8_t)MIN(255U, (b * src_a + (px[2] * dst_a * inv_a) / 255) / out_a);
        px[3] = (uint8_t)MIN(255U, out_a);
    }
}

static float fpart_f(float x)
{
    return x - floorf(x);
}

static float rfpart_f(float x)
{
    return 1.0f - fpart_f(x);
}

void kitty_draw_line_aa(struct KittyCanvas *canvas, float x0, float y0, float x1, float y1, uint8_t r, uint8_t g, uint8_t b,
                        uint8_t a)
{
    bool steep = fabsf(y1 - y0) > fabsf(x1 - x0);
    if (steep)
    {
        float tmp = x0;
        x0 = y0;
        y0 = tmp;
        tmp = x1;
        x1 = y1;
        y1 = tmp;
    }
    if (x0 > x1)
    {
        float tmp = x0;
        x0 = x1;
        x1 = tmp;
        tmp = y0;
        y0 = y1;
        y1 = tmp;
    }

    float dx = x1 - x0;
    float dy = y1 - y0;
    float gradient = (dx == 0.0f) ? 1.0f : (dy / dx);

    float xend = roundf(x0);
    float yend = y0 + gradient * (xend - x0);
    float xgap = rfpart_f(x0 + 0.5f);
    int xpxl1 = (int)xend;
    int ypxl1 = (int)floorf(yend);

    if (steep)
    {
        kitty_blend_pixel(canvas, ypxl1, xpxl1, r, g, b, (uint8_t)(a * rfpart_f(yend) * xgap));
        kitty_blend_pixel(canvas, ypxl1 + 1, xpxl1, r, g, b, (uint8_t)(a * fpart_f(yend) * xgap));
    }
    else
    {
        kitty_blend_pixel(canvas, xpxl1, ypxl1, r, g, b, (uint8_t)(a * rfpart_f(yend) * xgap));
        kitty_blend_pixel(canvas, xpxl1, ypxl1 + 1, r, g, b, (uint8_t)(a * fpart_f(yend) * xgap));
    }
    float intery = yend + gradient;

    xend = roundf(x1);
    yend = y1 + gradient * (xend - x1);
    xgap = fpart_f(x1 + 0.5f);
    int xpxl2 = (int)xend;
    int ypxl2 = (int)floorf(yend);

    if (steep)
    {
        kitty_blend_pixel(canvas, ypxl2, xpxl2, r, g, b, (uint8_t)(a * rfpart_f(yend) * xgap));
        kitty_blend_pixel(canvas, ypxl2 + 1, xpxl2, r, g, b, (uint8_t)(a * fpart_f(yend) * xgap));
        for (int x = xpxl1 + 1; x < xpxl2; ++x)
        {
            int iy = (int)floorf(intery);
            kitty_blend_pixel(canvas, iy, x, r, g, b, (uint8_t)(a * rfpart_f(intery)));
            kitty_blend_pixel(canvas, iy + 1, x, r, g, b, (uint8_t)(a * fpart_f(intery)));
            intery += gradient;
        }
    }
    else
    {
        kitty_blend_pixel(canvas, xpxl2, ypxl2, r, g, b, (uint8_t)(a * rfpart_f(yend) * xgap));
        kitty_blend_pixel(canvas, xpxl2, ypxl2 + 1, r, g, b, (uint8_t)(a * fpart_f(yend) * xgap));
        for (int x = xpxl1 + 1; x < xpxl2; ++x)
        {
            int iy = (int)floorf(intery);
            kitty_blend_pixel(canvas, x, iy, r, g, b, (uint8_t)(a * rfpart_f(intery)));
            kitty_blend_pixel(canvas, x, iy + 1, r, g, b, (uint8_t)(a * fpart_f(intery)));
            intery += gradient;
        }
    }
}

void kitty_draw_circle_aa(struct KittyCanvas *canvas, float cx, float cy, float radius, uint8_t r, uint8_t g, uint8_t b,
                          uint8_t a)
{
    if (canvas == NULL || radius <= 0.0f)
    {
        return;
    }

    int steps = (int)(2.0f * (float)M_PI * radius * 1.5f);
    if (steps < 64)
    {
        steps = 64;
    }

    float prev_x = cx + radius;
    float prev_y = cy;
    for (int i = 1; i <= steps; ++i)
    {
        float angle = (float)i * (2.0f * (float)M_PI) / (float)steps;
        float cur_x = cx + radius * cosf(angle);
        float cur_y = cy + radius * sinf(angle);
        kitty_draw_line_aa(canvas, prev_x, prev_y, cur_x, cur_y, r, g, b, a);
        prev_x = cur_x;
        prev_y = cur_y;
    }
}

void kitty_draw_star_glow(struct KittyCanvas *canvas, float cx, float cy, float radius, uint8_t r, uint8_t g, uint8_t b,
                          uint8_t a)
{
    if (canvas == NULL || radius <= 0.0f)
    {
        return;
    }

    float outer = radius + 1.25f;
    int min_x = (int)floorf(cx - outer);
    int max_x = (int)ceilf(cx + outer);
    int min_y = (int)floorf(cy - outer);
    int max_y = (int)ceilf(cy + outer);

    for (int y = min_y; y <= max_y; ++y)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > outer)
            {
                continue;
            }

            float coverage;
            if (dist <= radius * 0.55f)
            {
                coverage = 1.0f;
            }
            else
            {
                float t = (dist - radius * 0.55f) / (outer - radius * 0.55f);
                coverage = (1.0f - t) * (1.0f - t);
            }

            uint8_t px_a = (uint8_t)roundf((float)a * coverage);
            // Bright white hot-core for bright stars/planets
            uint8_t px_r = r;
            uint8_t px_g = g;
            uint8_t px_b = b;
            if (dist <= radius * 0.4f && radius >= 2.0f)
            {
                px_r = (uint8_t)MIN(255, (int)r + 45);
                px_g = (uint8_t)MIN(255, (int)g + 45);
                px_b = (uint8_t)MIN(255, (int)b + 45);
            }
            kitty_blend_pixel(canvas, x, y, px_r, px_g, px_b, px_a);
        }
    }
}

void kitty_draw_moon_phase(struct KittyCanvas *canvas, float cx, float cy, float radius, double moon_age, bool northern)
{
    if (canvas == NULL || radius <= 0.0f)
    {
        return;
    }

    // Moon phase angle in [0, 2pi): 0 = new moon, pi = full moon
    double phase_angle = moon_age * 2.0 * M_PI;
    double cos_phase = cos(phase_angle);
    bool waxing = (moon_age <= 0.5);
    if (!northern)
    {
        waxing = !waxing;
    }

    float outer = radius + 1.0f;
    int min_x = (int)floorf(cx - outer);
    int max_x = (int)ceilf(cx + outer);
    int min_y = (int)floorf(cy - outer);
    int max_y = (int)ceilf(cy + outer);

    for (int y = min_y; y <= max_y; ++y)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > outer)
            {
                continue;
            }

            float edge_alpha = 1.0f;
            if (dist > radius - 0.5f)
            {
                edge_alpha = (outer - dist) / (outer - (radius - 0.5f));
                if (edge_alpha < 0.0f)
                {
                    edge_alpha = 0.0f;
                }
            }

            // Normalized coordinates on unit sphere
            float ny = dy / radius;
            float nx = dx / radius;
            float max_nx = sqrtf(fmaxf(0.0f, 1.0f - ny * ny));
            float terminator_x = (float)(cos_phase)*max_nx;

            bool lit;
            if (cos_phase >= 1.0 - 1e-6)
            {
                lit = false;
            }
            else if (cos_phase <= -1.0 + 1e-6)
            {
                lit = true;
            }
            else
            {
                lit = waxing ? (nx >= terminator_x) : (nx <= -terminator_x);
            }
            uint8_t r = lit ? 245 : 55;
            uint8_t g = lit ? 242 : 60;
            uint8_t b = lit ? 215 : 75;
            uint8_t a = (uint8_t)roundf(245.0f * edge_alpha);
            kitty_blend_pixel(canvas, x, y, r, g, b, a);
        }
    }
}

static void polar_to_canvas(double r, double theta, int height, int width, float *py, float *px)
{
    float rad_y = (height - 1) / 2.0f;
    float rad_x = (width - 1) / 2.0f;
    *py = (float)(r * -rad_y * sin(theta) + rad_y);
    *px = (float)(r * rad_x * cos(theta) + rad_x);
}

void render_kitty_scene(struct KittyCanvas *canvas, const struct Conf *config, struct Star *star_table, int num_stars,
                        const int *num_by_mag, const struct Constell *constell_table, int num_const,
                        const struct Planet *planet_table, struct Moon moon_object)
{
    if (canvas == NULL || canvas->pixels == NULL || config == NULL)
    {
        return;
    }

    int h = canvas->height;
    int w = canvas->width;
    float cx = (w - 1) / 2.0f;
    float cy = (h - 1) / 2.0f;
    float radius = MIN(cx, cy) - 1.0f;

    // Skip re-rasterizing when the sky has rotated by less than 0.05 pixels
    // since the last rendered frame and configuration parameters are unchanged.
    if (canvas->scene_valid)
    {
        double pixel_shift = fabs(config->julian_date - canvas->last_julian_date) * (2.0 * M_PI * (double)radius);
        if (pixel_shift < 0.05 && canvas->last_latitude == config->latitude && canvas->last_longitude == config->longitude &&
            canvas->last_threshold == config->threshold && canvas->last_color == config->color &&
            canvas->last_constell == config->constell && canvas->last_grid == config->grid)
        {
            return;
        }
    }

    kitty_canvas_clear(canvas);

    // 1. Subtle horizon ring
    kitty_draw_circle_aa(canvas, cx, cy, radius, 65, 85, 120, 110);

    // 2. Azimuthal grid if enabled
    if (config->grid)
    {
        kitty_draw_circle_aa(canvas, cx, cy, radius * 0.333f, 55, 70, 100, 85);
        kitty_draw_circle_aa(canvas, cx, cy, radius * 0.667f, 55, 70, 100, 85);
        for (int angle = 0; angle < 360; angle += 30)
        {
            float rad = (float)angle * (float)TO_RAD;
            float ex = cx + radius * cosf(rad);
            float ey = cy - radius * sinf(rad);
            kitty_draw_line_aa(canvas, cx, cy, ex, ey, 55, 70, 100, 85);
        }
    }

    // 3. Constellation stick figures if enabled
    if (config->constell && constell_table != NULL)
    {
        for (int c = 0; c < num_const; ++c)
        {
            const struct Constell *constell = &constell_table[c];
            unsigned int num_seg = constell->num_segments;

            bool all_visible = true;
            for (unsigned int i = 0; i < num_seg * 2; ++i)
            {
                int idx = constell->star_numbers[i] - 1;
                if (star_table[idx].magnitude > config->threshold)
                {
                    all_visible = false;
                    break;
                }
            }
            if (!all_visible)
            {
                continue;
            }

            for (unsigned int i = 0; i < num_seg * 2; i += 2)
            {
                const struct Star *sa = &star_table[constell->star_numbers[i] - 1];
                const struct Star *sb = &star_table[constell->star_numbers[i + 1] - 1];

                double ra, ta, rb, tb;
                get_object_polar(&sa->base, &ra, &ta);
                get_object_polar(&sb->base, &rb, &tb);

                if (fabs(ra) > 1.0 && fabs(rb) > 1.0)
                {
                    continue;
                }
                if (fabs(ra) > 1.0)
                {
                    ra = 1.0;
                }
                else if (fabs(rb) > 1.0)
                {
                    rb = 1.0;
                }

                float ya, xa, yb, xb;
                polar_to_canvas(ra, ta, h, w, &ya, &xa);
                polar_to_canvas(rb, tb, h, w, &yb, &xb);

                uint8_t lr = config->color ? 90 : 140;
                uint8_t lg = config->color ? 135 : 140;
                uint8_t lb = config->color ? 205 : 140;
                kitty_draw_line_aa(canvas, xa, ya, xb, yb, lr, lg, lb, 150);
            }
        }
    }

    // 4. Stars sorted from faintest to brightest
    for (int i = 0; i < num_stars; ++i)
    {
        int idx = num_by_mag[i] - 1;
        const struct Star *star = &star_table[idx];
        if (star->magnitude > config->threshold)
        {
            continue;
        }

        double r_pol, t_pol;
        get_object_polar(&star->base, &r_pol, &t_pol);
        if (fabs(r_pol) > 1.0)
        {
            continue;
        }

        float py, px;
        polar_to_canvas(r_pol, t_pol, h, w, &py, &px);

        uint8_t sr = 240, sg = 240, sb = 255;
        if (config->color)
        {
            get_star_rgb(star->spectral_type, star->magnitude, config->threshold, &sr, &sg, &sb);
        }

        float max_mag = (config->threshold > 0.5f) ? config->threshold : 5.0f;
        float norm = (max_mag - star->magnitude) / (max_mag - (-1.46f));
        if (norm < 0.0f)
        {
            norm = 0.0f;
        }
        if (norm > 1.0f)
        {
            norm = 1.0f;
        }

        float star_radius = 0.65f + 2.4f * (norm * norm);
        uint8_t alpha = (uint8_t)roundf(110.0f + 145.0f * norm);
        kitty_draw_star_glow(canvas, px, py, star_radius, sr, sg, sb, alpha);
    }

    // 5. Planets & Sun
    if (planet_table != NULL)
    {
        for (int i = NUM_PLANETS - 1; i >= 0; --i)
        {
            if (i == EARTH)
            {
                continue;
            }
            double r_pol, t_pol;
            get_object_polar(&planet_table[i].base, &r_pol, &t_pol);
            if (fabs(r_pol) > 1.0)
            {
                continue;
            }

            float py, px;
            polar_to_canvas(r_pol, t_pol, h, w, &py, &px);

            uint8_t pr = 255, pg = 255, pb = 255;
            if (config->color)
            {
                get_planet_rgb(i, &pr, &pg, &pb);
            }
            float p_radius = (i == SUN) ? 4.2f : 2.6f;
            kitty_draw_star_glow(canvas, px, py, p_radius, pr, pg, pb, 255);
        }
    }

    // 6. Moon with realistic phase shading
    {
        double r_pol, t_pol;
        get_object_polar(&moon_object.base, &r_pol, &t_pol);
        if (fabs(r_pol) <= 1.0)
        {
            float py, px;
            polar_to_canvas(r_pol, t_pol, h, w, &py, &px);
            double age = calc_moon_age(config->julian_date);
            kitty_draw_moon_phase(canvas, px, py, 3.8f, age, config->latitude >= 0.0);
        }
    }

    canvas->scene_valid = true;
    canvas->last_julian_date = config->julian_date;
    canvas->last_latitude = config->latitude;
    canvas->last_longitude = config->longitude;
    canvas->last_threshold = config->threshold;
    canvas->last_color = config->color;
    canvas->last_constell = config->constell;
    canvas->last_grid = config->grid;
}

void kitty_transmit_canvas(struct KittyCanvas *canvas, WINDOW *win)
{
    if (canvas == NULL || canvas->pixels == NULL || canvas->prev_pixels == NULL || canvas->b64_buf == NULL || win == NULL)
    {
        return;
    }

    int begy = 0, begx = 0;
    int height = 0, width = 0;
    getbegyx(win, begy, begx);
    getmaxyx(win, height, width);

    size_t raw_len = (size_t)canvas->width * (size_t)canvas->height * 4;

    // Keep the texture resident in Kitty's GPU VRAM when neither the pixel
    // buffer nor the target window placement has changed since the last upload.
    if (canvas->has_transmitted && canvas->last_begy == begy && canvas->last_begx == begx && canvas->last_win_h == height &&
        canvas->last_win_w == width && memcmp(canvas->pixels, canvas->prev_pixels, raw_len) == 0)
    {
        return;
    }

    bool sent_via_file = false;
#ifndef _WIN32
    if (getenv("SSH_CONNECTION") == NULL && getenv("SSH_CLIENT") == NULL && getenv("SSH_TTY") == NULL)
    {
        unsigned int slot = canvas->file_slot % KITTY_FILE_SLOTS;
        if (canvas->temp_paths[slot][0] == '\0')
        {
            snprintf(canvas->temp_paths[slot], sizeof(canvas->temp_paths[slot]),
                     "/tmp/tty-graphics-protocol-astroterm-%d-%u.raw", (int)getpid(), slot);
        }
        const char *path = canvas->temp_paths[slot];
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd >= 0)
        {
            size_t written = 0;
            while (written < raw_len)
            {
                ssize_t n = write(fd, canvas->pixels + written, raw_len - written);
                if (n <= 0)
                {
                    break;
                }
                written += (size_t)n;
            }
            close(fd);
            if (written == raw_len)
            {
                char path_b64[512];
                kitty_encode_base64((const uint8_t *)path, strlen(path), path_b64);
                fprintf(stdout,
                        "\x1b"
                        "7\x1b[%d;%dH\x1b_Ga=T,t=f,f=32,i=1,p=1,s=%d,v=%d,c=%d,r=%d,z=-1,C=1,q=2;%s\x1b\\\x1b"
                        "8",
                        begy + 1, begx + 1, canvas->width, canvas->height, width, height, path_b64);
                fflush(stdout);
                canvas->file_slot++;
                sent_via_file = true;
            }
        }
    }
#endif

    if (!sent_via_file)
    {
        size_t b64_len = kitty_encode_base64(canvas->pixels, raw_len, canvas->b64_buf);

        // Save cursor, move to top-left of projection window (1-based ANSI row/col)
        fprintf(stdout,
                "\x1b"
                "7\x1b[%d;%dH",
                begy + 1, begx + 1);

        size_t offset = 0;
        bool first = true;
        while (offset < b64_len)
        {
            size_t chunk = b64_len - offset;
            if (chunk > KITTY_CHUNK_SIZE)
            {
                chunk = KITTY_CHUNK_SIZE;
            }
            int more = (offset + chunk < b64_len) ? 1 : 0;

            if (first)
            {
                fprintf(stdout, "\x1b_Ga=T,f=32,i=1,p=1,s=%d,v=%d,c=%d,r=%d,z=-1,C=1,q=2,m=%d;", canvas->width, canvas->height,
                        width, height, more);
                first = false;
            }
            else
            {
                fprintf(stdout, "\x1b_Gm=%d;", more);
            }

            fwrite(canvas->b64_buf + offset, 1, chunk, stdout);
            fputs("\x1b\\", stdout);
            offset += chunk;
        }

        // Restore cursor
        fputs("\x1b"
              "8",
              stdout);
        fflush(stdout);
    }

    memcpy(canvas->prev_pixels, canvas->pixels, raw_len);
    canvas->has_transmitted = true;
    canvas->last_begy = begy;
    canvas->last_begx = begx;
    canvas->last_win_h = height;
    canvas->last_win_w = width;
}

void kitty_clear_images(void)
{
    fputs("\x1b_Ga=d,d=A,q=2;\x1b\\", stdout);
    fflush(stdout);
}
