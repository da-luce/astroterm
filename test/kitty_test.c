#include "bsc5.h"
#include "bsc5_constellations.h"
#include "bsc5_names.h"
#include "core.h"
#include "core_position.h"
#include "data/keplerian_elements.h"
#include "kitty_render.h"
#include "macros.h"
#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_kitty_base64_encoding(void)
{
    char out[64];

    // Edge case: 0-length input
    size_t len = kitty_encode_base64((const uint8_t *)"", 0, out);
    TEST_ASSERT_EQUAL_UINT(0, len);
    TEST_ASSERT_EQUAL_STRING("", out);

    // NULL input with 0 length
    len = kitty_encode_base64(NULL, 0, out);
    TEST_ASSERT_EQUAL_UINT(0, len);
    TEST_ASSERT_EQUAL_STRING("", out);

    len = kitty_encode_base64((const uint8_t *)"M", 1, out);
    TEST_ASSERT_EQUAL_UINT(4, len);
    TEST_ASSERT_EQUAL_STRING("TQ==", out);

    len = kitty_encode_base64((const uint8_t *)"Ma", 2, out);
    TEST_ASSERT_EQUAL_UINT(4, len);
    TEST_ASSERT_EQUAL_STRING("TWE=", out);

    len = kitty_encode_base64((const uint8_t *)"Man", 3, out);
    TEST_ASSERT_EQUAL_UINT(4, len);
    TEST_ASSERT_EQUAL_STRING("TWFu", out);

    const uint8_t bin[4] = {0x00, 0xFF, 0x00, 0xFF};
    len = kitty_encode_base64(bin, 4, out);
    TEST_ASSERT_EQUAL_UINT(8, len);
    TEST_ASSERT_EQUAL_STRING("AP8A/w==", out);

    len = kitty_encode_base64((const uint8_t *)"astroterm", 9, out);
    TEST_ASSERT_EQUAL_UINT(12, len);
    TEST_ASSERT_EQUAL_STRING("YXN0cm90ZXJt", out);
}

void test_kitty_canvas_lifecycle_and_resize(void)
{
    struct KittyCanvas canvas = {0};

    TEST_ASSERT_FALSE(kitty_canvas_init(NULL, 64, 64));
    TEST_ASSERT_FALSE(kitty_canvas_init(&canvas, 0, 64));

    TEST_ASSERT_TRUE(kitty_canvas_init(&canvas, 64, 64));
    TEST_ASSERT_EQUAL_INT(64, canvas.width);
    TEST_ASSERT_EQUAL_INT(64, canvas.height);
    TEST_ASSERT_NOT_NULL(canvas.pixels);
    TEST_ASSERT_NOT_NULL(canvas.b64_buf);

    kitty_blend_pixel(&canvas, 10, 10, 200, 150, 100, 255);
    size_t idx = ((size_t)10 * 64 + 10) * 4;
    TEST_ASSERT_EQUAL_UINT8(200, canvas.pixels[idx + 0]);
    TEST_ASSERT_EQUAL_UINT8(150, canvas.pixels[idx + 1]);
    TEST_ASSERT_EQUAL_UINT8(100, canvas.pixels[idx + 2]);
    TEST_ASSERT_EQUAL_UINT8(255, canvas.pixels[idx + 3]);

    // Alpha blend on top
    kitty_blend_pixel(&canvas, 10, 10, 100, 250, 200, 128);
    TEST_ASSERT_EQUAL_UINT8(255, canvas.pixels[idx + 3]);
    TEST_ASSERT_TRUE(canvas.pixels[idx + 0] < 200);
    TEST_ASSERT_TRUE(canvas.pixels[idx + 1] > 150);

    // Out-of-bounds blend should be a safe no-op
    kitty_blend_pixel(&canvas, -5, -5, 255, 255, 255, 255);
    kitty_blend_pixel(&canvas, 100, 100, 255, 255, 255, 255);

    kitty_canvas_clear(&canvas);
    TEST_ASSERT_EQUAL_UINT8(0, canvas.pixels[idx + 3]);

    TEST_ASSERT_TRUE(kitty_canvas_resize(&canvas, 128, 128));
    TEST_ASSERT_EQUAL_INT(128, canvas.width);
    TEST_ASSERT_EQUAL_INT(128, canvas.height);

    // Memory safe resize failure tests: invalid dimensions should return false without corrupting canvas
    TEST_ASSERT_FALSE(kitty_canvas_resize(&canvas, 0, 128));
    TEST_ASSERT_FALSE(kitty_canvas_resize(&canvas, 128, -1));
    TEST_ASSERT_FALSE(kitty_canvas_resize(NULL, 128, 128));
    TEST_ASSERT_EQUAL_INT(128, canvas.width);
    TEST_ASSERT_EQUAL_INT(128, canvas.height);
    TEST_ASSERT_NOT_NULL(canvas.pixels);

    kitty_canvas_free(&canvas);
    TEST_ASSERT_NULL(canvas.pixels);
    TEST_ASSERT_NULL(canvas.b64_buf);
}

void test_kitty_drawing_primitives_and_scene(void)
{
    struct KittyCanvas canvas = {0};
    TEST_ASSERT_TRUE(kitty_canvas_init(&canvas, 160, 160));

    kitty_draw_line_aa(&canvas, 10.0f, 10.0f, 150.0f, 150.0f, 100, 150, 255, 200);
    kitty_draw_circle_aa(&canvas, 80.0f, 80.0f, 40.0f, 200, 200, 200, 180);
    kitty_draw_star_glow(&canvas, 80.0f, 80.0f, 3.0f, 255, 220, 180, 255);
    kitty_draw_moon_phase(&canvas, 40.0f, 40.0f, 5.0f, 0.25, true);

    // Center pixel at (80, 80) should be non-transparent from star glow
    size_t center_idx = ((size_t)80 * 160 + 80) * 4;
    TEST_ASSERT_GREATER_THAN(0, canvas.pixels[center_idx + 3]);

    // Test full scene render
    unsigned int num_stars = 0, num_const = 0;
    struct Entry *entries = NULL;
    struct StarName *names = NULL;
    struct Star *stars = NULL;
    struct Constell *constells = NULL;
    struct Planet *planets = NULL;
    struct Moon moon;
    int *num_by_mag = NULL;

    parse_entries(bsc5, bsc5_len, &entries, &num_stars);
    generate_name_table(bsc5_names, bsc5_names_len, &names, (int)num_stars);
    generate_star_table(&stars, entries, names, num_stars);
    star_numbers_by_magnitude(&num_by_mag, stars, num_stars);
    generate_constell_table(bsc5_constellations, bsc5_constellations_len, &constells, &num_const);
    generate_planet_table(&planets, planet_elements, planet_rates, planet_extras);
    generate_moon_object(&moon, &moon_elements, &moon_rates);
    free(entries);

    struct Conf config = {
        .longitude = -71.0589 * M_PI / 180.0,
        .latitude = 42.3601 * M_PI / 180.0,
        .threshold = 5.0f,
        .label_thresh = 0.25f,
        .julian_date = 2459146.0,
        .color = true,
        .grid = true,
        .constell = true,
        .kitty = true,
    };

    update_star_positions(stars, (int)num_stars, config.julian_date, config.latitude, config.longitude);
    update_planet_positions(planets, config.julian_date, config.latitude, config.longitude);
    update_moon_position(&moon, config.julian_date, config.latitude, config.longitude);

    render_kitty_scene(&canvas, &config, stars, (int)num_stars, num_by_mag, constells, (int)num_const, planets, moon);
    TEST_ASSERT_TRUE(canvas.scene_valid);

    // Verify non-empty rendered scene
    int non_zero_pixels = 0;
    for (int i = 0; i < canvas.width * canvas.height; ++i)
    {
        if (canvas.pixels[i * 4 + 3] > 0)
        {
            non_zero_pixels++;
        }
    }
    TEST_ASSERT_GREATER_THAN(100, non_zero_pixels);

    // Sub-pixel time step (< 0.15 px shift) should reuse cached scene without clearing
    canvas.pixels[0] = 42;
    config.julian_date += 1.0e-7;
    render_kitty_scene(&canvas, &config, stars, (int)num_stars, num_by_mag, constells, (int)num_const, planets, moon);
    TEST_ASSERT_EQUAL_UINT8(42, canvas.pixels[0]);

    // Larger time step should re-rasterize the scene
    config.julian_date += 0.01;
    update_star_positions(stars, (int)num_stars, config.julian_date, config.latitude, config.longitude);
    render_kitty_scene(&canvas, &config, stars, (int)num_stars, num_by_mag, constells, (int)num_const, planets, moon);
    TEST_ASSERT_EQUAL_UINT8(0, canvas.pixels[0]);

    free_constells(constells, num_const);
    free_stars(stars, num_stars);
    free_planets(planets, NUM_PLANETS);
    free_moon_object(moon);
    free_star_names(names, num_stars);
    free(num_by_mag);
    kitty_canvas_free(&canvas);
}

static void count_moon_pixels(const struct KittyCanvas *canvas, int *total_disk_pixels, int *lit_pixels)
{
    *total_disk_pixels = 0;
    *lit_pixels = 0;
    for (int y = 0; y < canvas->height; ++y)
    {
        for (int x = 0; x < canvas->width; ++x)
        {
            size_t idx = ((size_t)y * canvas->width + x) * 4;
            uint8_t a = canvas->pixels[idx + 3];
            if (a > 50) // Inside the moon disk
            {
                (*total_disk_pixels)++;
                uint8_t r = canvas->pixels[idx + 0];
                if (r > 150) // Lit pixel (lit color is r=245, unlit is r=55)
                {
                    (*lit_pixels)++;
                }
            }
        }
    }
}

void test_kitty_moon_phase_terminator(void)
{
    struct KittyCanvas canvas = {0};
    TEST_ASSERT_TRUE(kitty_canvas_init(&canvas, 100, 100));

    int total, lit;

    // 1. New Moon (moon_age = 0.0) -> 0% lit (all unlit pixels)
    kitty_canvas_clear(&canvas);
    kitty_draw_moon_phase(&canvas, 50.0f, 50.0f, 20.0f, 0.0, true);
    count_moon_pixels(&canvas, &total, &lit);
    TEST_ASSERT_GREATER_THAN(500, total); // Moon disk should have substantial pixels
    TEST_ASSERT_EQUAL_INT(0, lit);        // New moon must have 0 lit pixels

    // 2. Full Moon (moon_age = 0.5) -> 100% lit
    kitty_canvas_clear(&canvas);
    kitty_draw_moon_phase(&canvas, 50.0f, 50.0f, 20.0f, 0.5, true);
    count_moon_pixels(&canvas, &total, &lit);
    TEST_ASSERT_GREATER_THAN(500, total);
    TEST_ASSERT_EQUAL_INT(total, lit); // Full moon must be 100% lit

    // 3. First Quarter (moon_age = 0.25, waxing) -> ~50% lit
    kitty_canvas_clear(&canvas);
    kitty_draw_moon_phase(&canvas, 50.0f, 50.0f, 20.0f, 0.25, true);
    count_moon_pixels(&canvas, &total, &lit);
    TEST_ASSERT_GREATER_THAN(500, total);
    float ratio_q1 = (float)lit / (float)total;
    TEST_ASSERT_FLOAT_WITHIN(0.10f, 0.50f, ratio_q1);

    // 4. Third Quarter (moon_age = 0.75, waning) -> ~50% lit
    kitty_canvas_clear(&canvas);
    kitty_draw_moon_phase(&canvas, 50.0f, 50.0f, 20.0f, 0.75, false);
    count_moon_pixels(&canvas, &total, &lit);
    TEST_ASSERT_GREATER_THAN(500, total);
    float ratio_q3 = (float)lit / (float)total;
    TEST_ASSERT_FLOAT_WITHIN(0.10f, 0.50f, ratio_q3);

    kitty_canvas_free(&canvas);
}

void test_kitty_compute_canvas_size(void)
{
    int w = 0, h = 0;

    // NULL pointers should be handled safely
    kitty_compute_canvas_size(NULL, NULL, NULL);
    kitty_compute_canvas_size(NULL, &w, NULL);
    kitty_compute_canvas_size(NULL, NULL, &h);

    // With win == NULL, should populate default canvas size bounded by KITTY_MIN_CANVAS_DIM and KITTY_MAX_CANVAS_DIM
    kitty_compute_canvas_size(NULL, &w, &h);
    TEST_ASSERT_TRUE(w >= KITTY_MIN_CANVAS_DIM);
    TEST_ASSERT_TRUE(w <= KITTY_MAX_CANVAS_DIM);
    TEST_ASSERT_TRUE(h >= KITTY_MIN_CANVAS_DIM);
    TEST_ASSERT_TRUE(h <= KITTY_MAX_CANVAS_DIM);
    TEST_ASSERT_EQUAL_INT(w, h); // Kitty canvas is square
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_kitty_base64_encoding);
    RUN_TEST(test_kitty_canvas_lifecycle_and_resize);
    RUN_TEST(test_kitty_drawing_primitives_and_scene);
    RUN_TEST(test_kitty_moon_phase_terminator);
    RUN_TEST(test_kitty_compute_canvas_size);

    return UNITY_END();
}
