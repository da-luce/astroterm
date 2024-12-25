#include "core.h"
#include "core_position.h"
#include "core_render.h"

#include "data/keplerian_elements.h"
#include "parse_BSC5.h"
#include "stopwatch.h"
#include "term.h"

// Embedded data generated during build
#include "bsc5_constellations.h"
#include "bsc5_data.h"
#include "bsc5_names.h"

// Third part libraries
#include "argtable3.h"

#include <getopt.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>

static volatile bool perform_resize = false;

static void catch_winch(int sig);
static void handle_resize(WINDOW *win);
static void parse_options(int argc, char *argv[], struct conf *config);
static void convert_options(struct conf *config);
static void render_metadata(WINDOW *win, struct conf *config);

// Track current simulation time
// Default to current time in dt_string_utc is NULL
double julian_date = 0.0;
double julian_date_start = 0.0; // Note of when we started

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(int argc, char *argv[])
{
    // Default config
    struct conf config = {
        .longitude = -71.057083, // Boston, MA
        .latitude = 42.361145,
        .dt_string_utc = NULL,
        .threshold = 3.0f,
        .label_thresh = 0.5f,
        .fps = 24,
        .animation_mult = 1.0f,
        .ascii = true,
        .color = false,
        .grid = false,
        .constell = false,
        .meta = false,
    };

    // Parse command line args and convert to internal representations
    parse_options(argc, argv, &config);
    convert_options(&config);

    // Time for each frame in microseconds
    unsigned long dt = (unsigned long)(1.0 / config.fps * 1.0E6);

    // Initialize data structs
    unsigned int num_stars, num_const;

    struct entry *BSC5_entries;
    struct star_name *name_table;
    struct constell *constell_table;
    struct star *star_table;
    struct planet *planet_table;
    struct moon moon_object;
    int *num_by_mag;

    // Track success of functions
    bool s = true;

    // Generated BSC5 data during build in bsc5_xxx.h:
    //
    // uint8_t bsc5_xxx[];
    // size_t bsc5_xxx_len;

    s = s && parse_entries(bsc5_data, bsc5_data_len, &BSC5_entries, &num_stars);
    s = s && generate_name_table(bsc5_names, bsc5_names_len, &name_table, num_stars);
    s = s && generate_constell_table(bsc5_constellations, bsc5_constellations_len, &constell_table, &num_const);
    s = s && generate_star_table(&star_table, BSC5_entries, name_table, num_stars);
    s = s && generate_planet_table(&planet_table, planet_elements, planet_rates, planet_extras);
    s = s && generate_moon_object(&moon_object, &moon_elements, &moon_rates);
    s = s && star_numbers_by_magnitude(&num_by_mag, star_table, num_stars);

    if (!s)
    {
        // At least one of the above functions failed, abort
        abort();
    }

    // This memory is no longer needed
    free(BSC5_entries);
    free_star_names(name_table, num_stars);

    // Terminal/System settings
    setlocale(LC_ALL, "");         // Required for unicode rendering
    signal(SIGWINCH, catch_winch); // Capture window resizes

    // Ncurses initialization
    ncurses_init(config.color);

    // Add the main (projection) window
    WINDOW *main_win = newwin(0, 0, 0, 0);
    wtimeout(main_win, 0); // Non-blocking read for wgetch
    win_resize_square(main_win, get_cell_aspect_ratio());
    win_position_center(main_win);

    // Add a metadata window
    const int meta_rows = 6;                                   // Allows for 6 rows
    const int meta_cols = 48;                                  // Set to allow enough room for longest line (elapsed time)
    WINDOW *metadata_win = newwin(meta_rows, meta_cols, 0, 0); // Position at top right
    wtimeout(metadata_win, 0);                                 // Non-blocking read for wgetch

    // Render loop
    while (true)
    {
        struct sw_timestamp frame_begin;
        sw_gettime(&frame_begin);

        werase(main_win);

        if (perform_resize)
        {
            // Putting this after erasing the window reduces flickering
            handle_resize(main_win);
        }

        // Update object positions
        update_star_positions(star_table, num_stars, julian_date, config.latitude, config.longitude);
        update_planet_positions(planet_table, julian_date, config.latitude, config.longitude);
        update_moon_position(&moon_object, julian_date, config.latitude, config.longitude);
        update_moon_phase(&moon_object, julian_date, config.latitude);

        // Render
        render_stars_stereo(main_win, &config, star_table, num_stars, num_by_mag);
        if (config.constell)
        {
            render_constells(main_win, &config, &constell_table, num_const, star_table);
        }
        render_planets_stereo(main_win, &config, planet_table);
        render_moon_stereo(main_win, &config, moon_object);
        if (config.grid)
        {
            render_azimuthal_grid(main_win, &config);
        }
        else
        {
            render_cardinal_directions(main_win, &config);
        }

        if (config.meta)
        {
            werase(metadata_win);
        }

        if (config.meta)
        {
            render_metadata(metadata_win, &config);
        }

        // Exit if ESC or q is pressed
        int ch = getch();
        if (ch == 27 || ch == 'q')
        {
            break;
        }

        // Use double buffering to avoid flickering while updating
        wnoutrefresh(main_win);
        wnoutrefresh(metadata_win);
        doupdate();

        // TODO: this timing scheme *should* minimize any drift or divergence
        // between simulation time and realtime. Check this to make sure.

        // Increment "simulation" time
        const double microsec_per_day = 24.0 * 60.0 * 60.0 * 1.0E6;
        julian_date += (double)dt / microsec_per_day * config.animation_mult;

        // Determine time it took to update positions and render to screen
        struct sw_timestamp frame_end;
        sw_gettime(&frame_end);

        unsigned long long frame_time;
        sw_timediff_usec(frame_end, frame_begin, &frame_time);

        if (frame_time < dt)
        {
            sw_sleep(dt - frame_time);
        }
    }

    ncurses_kill();

    free_constells(constell_table, num_const);
    free_stars(star_table, num_stars);
    free_planets(planet_table, NUM_PLANETS);
    free_moon_object(moon_object);

    return 0;
}

void parse_options(int argc, char *argv[], struct conf *config)
{
    // Define Argtable3 option structures
    struct arg_dbl *latitude_arg = arg_dbl0("a", "latitude", "<degrees>", "Observer latitude [-90°, 90°] (default: 42.361145)");
    struct arg_dbl *longitude_arg =
        arg_dbl0("o", "longitude", "<degrees>", "Observer longitude [-180°, 180°] (default: -71.057083)");
    struct arg_str *datetime_arg = arg_str0("d", "datetime", "<yyyy-mm-ddThh:mm:ss>", "Observation datetime in UTC");
    struct arg_dbl *threshold_arg =
        arg_dbl0("t", "threshold", "<float>", "Only render stars brighter than this magnitude (default: 3.0)");
    struct arg_dbl *label_arg =
        arg_dbl0("l", "label-thresh", "<float>", "Label stars brighter than this magnitude (default: 0.5)");
    struct arg_int *fps_arg = arg_int0("f", "fps", "<int>", "Frames per second (default: 24)");
    struct arg_dbl *anim_arg = arg_dbl0("s", "speed", "<float>", "Animation speed multiplier (default: 1.0)");
    struct arg_lit *color_arg = arg_lit0(NULL, "color", "Enable terminal colors");
    struct arg_lit *constell_arg = arg_lit0(NULL, "constellations",
                                            "Draw constellations stick figures. Note: a constellation is only "
                                            "drawn if all stars in the figure are over the threshold");
    struct arg_lit *grid_arg = arg_lit0(NULL, "grid", "Draw an azimuthal grid");
    struct arg_lit *ascii_arg = arg_lit0(NULL, "ascii", "Only use ASCII characters");
    struct arg_lit *meta_arg = arg_lit0("m", "meta", "Display metadata");
    struct arg_lit *help_arg = arg_lit0("h", "help", "Print this help message");
    struct arg_end *end = arg_end(20);

    // Create argtable array
    void *argtable[] = {latitude_arg, longitude_arg, datetime_arg, threshold_arg, label_arg, fps_arg,  anim_arg,
                        color_arg,    constell_arg,  grid_arg,     ascii_arg,     meta_arg,  help_arg, end};

    // Parse the arguments
    int nerrors = arg_parse(argc, argv, argtable);

    if (help_arg->count > 0)
    {
        printf("View stars, planets, and more, right in your terminal! ✨🪐\n\n");
        printf("Usage: astroterm [OPTION]...\n\n");
        arg_print_glossary_gnu(stdout, argtable);
        exit(EXIT_SUCCESS);
    }

    if (nerrors > 0)
    {
        arg_print_errors(stderr, end, argv[0]);
        printf("Try '--help' for more information.\n");
        exit(EXIT_FAILURE);
    }

    // Assign parsed values to global variables
    if (latitude_arg->count > 0)
    {
        config->latitude = latitude_arg->dval[0];
        if (config->latitude < -90 || config->latitude > 90)
        {
            fprintf(stderr, "ERROR: Latitude out of range [-90°, 90°]\n");
            exit(EXIT_FAILURE);
        }
    }

    if (longitude_arg->count > 0)
    {
        config->longitude = longitude_arg->dval[0];
        if (config->longitude < -180 || config->longitude > 180)
        {
            fprintf(stderr, "ERROR: Longitude out of range [-180°, 180°]\n");
            exit(EXIT_FAILURE);
        }
    }

    if (datetime_arg->count > 0)
    {
        config->dt_string_utc = datetime_arg->sval[0];
    }

    if (threshold_arg->count > 0)
    {
        config->threshold = (float)threshold_arg->dval[0];
    }

    if (label_arg->count > 0)
    {
        config->label_thresh = (float)label_arg->dval[0];
    }

    if (fps_arg->count > 0)
    {
        config->fps = fps_arg->ival[0];
        if (config->fps < 1)
        {
            fprintf(stderr, "ERROR: FPS must be greater than or equal to 1\n");
            exit(EXIT_FAILURE);
        }
    }

    if (anim_arg->count > 0)
    {
        config->animation_mult = (float)anim_arg->dval[0];
    }

    if (color_arg->count > 0)
    {
        config->color = TRUE;
    }

    if (constell_arg->count > 0)
    {
        config->constell = TRUE;
    }

    if (meta_arg->count > 0)
    {
        config->meta = TRUE;
    }

    if (grid_arg->count > 0)
    {
        config->grid = TRUE;
    }

    if (ascii_arg->count > 0)
    {
        config->ascii = FALSE;
    }

    // Free Argtable resources
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
}

void convert_options(struct conf *config)
{
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

    // Convert longitude and latitude to radians
    config->longitude *= M_PI / 180.0;
    config->latitude *= M_PI / 180.0;

    // Convert Gregorian calendar date to Julian date
    if (config->dt_string_utc == NULL)
    {
        // Set julian date to current time
        julian_date_start = current_julian_date();
        julian_date = julian_date_start;
    }
    else
    {
        struct tm datetime;
        bool parse_success = string_to_time(config->dt_string_utc, &datetime);
        if (!parse_success)
        {
            printf("ERROR: Unable to parse datetime string '%s'\nDatetimes "
                   "must be in form <yyyy-mm-ddThh:mm:ss>",
                   config->dt_string_utc);
            exit(EXIT_FAILURE);
        }
        julian_date_start = datetime_to_julian_date(&datetime);
        julian_date = julian_date_start;
    }

    return;
}

void catch_winch(int sig)
{
    perform_resize = true;
}

void handle_resize(WINDOW *win)
{
    // Resize ncurses internal terminal
    int y;
    int x;
    term_size(&y, &x);
    resizeterm(y, x);

    // ???
    wclear(win);
    wrefresh(win);

    // Check cell ratio
    float aspect = get_cell_aspect_ratio();

    // Resize/position application window
    win_resize_square(win, aspect);
    win_position_center(win);

    perform_resize = false;
}

void render_metadata(WINDOW *win, struct conf *config)
{
    // Gregorian Date
    int year, month, day;
    julian_to_gregorian(julian_date, &year, &month, &day);
    mvwprintw(win, 0, 0, "Gregorian Date: %02d-%02d-%04d", day, month, year);

    // Zodiac
    const char *zodiac = get_zodiac_sign(day, month);
    mvwprintw(win, 1, 0, "Zodiac: \t%s", zodiac);

    // Lunar phase
    const char *lunar_phase = get_moon_phase_description(julian_date);
    mvwprintw(win, 2, 0, "Lunar phase: \t%s", lunar_phase);

    // Lat and Lon (convert back to degrees)
    mvwprintw(win, 3, 0, "Latitude: \t%.6f°", config->latitude * 180 / M_PI);
    mvwprintw(win, 4, 0, "Longitude: \t%.6f°", config->longitude * 180 / M_PI);

    // Elapsed time
    int eyears, edays, ehours, emins, esecs;
    elapsed_time_to_components(julian_date - julian_date_start, &eyears, &edays, &ehours, &emins, &esecs);
    mvwprintw(win, 5, 0, "Elapsed Time: \t%d years, %d days, %02d:%02d:%02d", eyears, edays, ehours, emins, esecs);

    return;
}
