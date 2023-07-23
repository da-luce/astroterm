#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <locale.h>
#include <time.h>
#include <stdbool.h>

#include "astro.h"
#include "coord.h"
// #include "drawing.c"
#include "misc.h"
#include "term.h"
#include "celestial_bodies.h"
#include "parse_BSC5.h"

// flag for resize signal handler
static volatile bool perform_resize = false;

// star magnitude mapping
static const char *mag_map_unicode_round[10]    = {"⬤", "⚫︎", "●", "⦁", "•", "🞄", "∙", "⋅", "⋅", "⋅"};
static const char *mag_map_unicode_diamond[10]  = {"⯁", "◇", "⬥", "⬦", "⬩", "🞘", "🞗", "🞗", "🞗", "🞗"};
static const char *mag_map_unicode_open[10]     = {"✩", "✧", "⋄", "⭒", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
static const char *mag_map_unicode_filled[10]   = {"★", "✦", "⬩", "⭑", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
static const char mag_map_round_ASCII[10]       = {'0', '0', 'O', 'O', 'o', 'o', '.', '.', '.', '.'};

static const float min_magnitude = -1.46;
static const float max_magnitude = 7.96;

void update_star_positions(struct star stars[], int num_stars,
                           double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);
    
    for (int i = 0; i < num_stars; ++i)
    {
        struct star *star = &stars[i];

        double altitude, azimuth;
        equatorial_to_horizontal(star -> declination, star -> right_ascension,
                                 gmst, latitude, longitude,
                                 &altitude, &azimuth);

        star -> altitude = altitude;
        star -> azimuth = azimuth;
    }

    return;
}

void populate_special_chars(const char *special_symbols[], int num_stars)
{
    // TODO: is there a better way to do this?

    // Initialize all strings pointers to NULL
    // Add 1 since catalog_num = array_index + 1
    for (int i = 0; i < num_stars + 1; ++i)
    {
        special_symbols[i] = NULL;
    }
    special_symbols[424]  = "✦"; // Polaris
    special_symbols[4301] = "D"; // Dubhe
    special_symbols[2061] = "B"; // Betelgeuse
    special_symbols[7001] = "V"; // Vega
    return;
}

void render_stereo(struct star stars[], int num_stars,
                   const char *special_symbols[],
                   bool no_unicode, float threshold,
                   WINDOW *win)
{

    for (int i = 0; i < num_stars; ++i)
    {
        struct star* star = &stars[i];

        // filter stars by magnitude
        if (star -> magnitude > threshold)
        {
            continue;
        }

        // map stars using stereographic projection

        double theta_sphere, phi_sphere;
        horizontal_to_spherical(star -> azimuth, star -> altitude,
                                &theta_sphere, &phi_sphere);

        double radius_polar, theta_polar;
        project_stereographic_north(1.0, theta_sphere, phi_sphere,
                                    &radius_polar, &theta_polar);

        // if outside projection, ignore
        if (fabs(radius_polar) > 1)
        {
            continue;
        }

        int row, col;
        polar_to_win(radius_polar, theta_polar,
                   win->_maxy, win->_maxx,
                   &row, &col);

        int symbol_index = map_float_to_int_range(min_magnitude, max_magnitude, 0, 9, star->magnitude);

        // draw star
        if (no_unicode)
        {
            mvwaddch(win, row, col, mag_map_round_ASCII[symbol_index]);
        }
        else
        {
            mvwaddstr(win, row, col, mag_map_unicode_diamond[symbol_index]);
        }

        // special stars used for debugging
        if (special_symbols[(int) star->catalog_number] != NULL)
        {
            mvwaddstr(win, row, col, special_symbols[(int)star->catalog_number]);
        }
    }

    return;
}

void render_azimuthal_grid(WINDOW *win, bool no_unicode)
{
    if (no_unicode)
    {
        mvwaddch(win, win->_maxy / 2, win->_maxx / 2, '+');
    }
    else
    {
        mvwaddstr(win, win->_maxy / 2, win->_maxx / 2, "＋");
    }
}

void catch_winch(int sig)
{
    perform_resize = true;
}

void handle_resize(WINDOW *win)
{
    // resize ncurses internal terminal
    int y;
    int x;
    term_size(&y, &x);
    resizeterm(y, x);

    // ???
    wclear(win);
    wrefresh(win);

    // check cell ratio
    float aspect = get_cell_aspect_ratio();

    // resize/position application window
    win_resize_square(win, aspect);
    win_position_center(win);

    perform_resize = false;
}

bool parse_options(int argc, char *argv[],
                   double *latitude,
                   double *longitude,
                   double *julian_date,
                   float *threshold,
                   int *fps,
                   float *animation_mult,
                   int *no_unicode,
                   int *color,
                   int *grid);

int main(int argc, char *argv[])
{
    // get current time
    time_t t = time(NULL);
    struct tm lt = *localtime(&t);
    double current_jd = datetime_to_julian_date(&lt);

    // defaults
    double latitude         = 0.73934145516;    // Boston, MA
    double longitude        = 5.04300525197;    // Boston, MA
    double julian_date      = current_jd;       // Jan 1, 2000 00:00:00.0
    float threshold         = 3.0f;
    int fps                 = 24;
    float animation_mult    = 1.0f;             // real time animation speed mult (e.g. 2 is 2x real time)

    int no_unicode;
    int color;
    int grid;

    bool input_error = parse_options(argc, argv,
                                    &latitude,
                                    &longitude,
                                    &julian_date,
                                    &threshold,
                                    &fps,
                                    &animation_mult,
                                    &no_unicode,
                                    &color,
                                    &grid);

    if (input_error)
    {
        return 1;
    }

    int total_stars;
    struct star *stars = parse_stars("data/BSC5", &total_stars);

    // sort stars by magnitude so "larger" stars are always rendered on top
    // reduces "flickering" when rendering many stars
    qsort(stars, total_stars, sizeof(struct star), star_magnitude_comparator);

    // initialize special character map for debugging stars
    const char  *special_symbols[total_stars];
    populate_special_chars(special_symbols, total_stars);

    setlocale(LC_ALL, ""); // required for unicode rendering

    signal(SIGWINCH, catch_winch); // Capture window resizes

    ncurses_init();

    WINDOW *win = newwin(0, 0, 0, 0);
    wtimeout(win, 0); // non-blocking read for wgetch
    win_resize_square(win, get_cell_aspect_ratio());
    win_position_center(win);

    int c;
    while (true)
    {
        
        // wait until ESC is pressed
        if ((c = wgetch(win)) == 27)
        {
            break;
        }

        if (perform_resize)
        {
            handle_resize(win);
        }

        // ncurses erase should occur before rendering?
        // https://stackoverflow.com/questions/68706290/how-to-reduce-flickering-lag-on-curses

        // FIXME: rendered frames only show up starting on second frame
        werase(win);

        update_star_positions(stars, total_stars, julian_date, latitude, longitude);
        render_stereo(stars, total_stars, special_symbols, no_unicode, threshold, win);
        if (grid)
        {
            render_azimuthal_grid(win, no_unicode);
        }

        julian_date += (1.0 / fps) / (24 * 60 * 60) * animation_mult;

        usleep(1.0 / fps * 1000000);
    }
    
    ncurses_kill();

    free(stars);

    return 0;
}

bool parse_options(int argc, char *argv[],
                   double   *latitude,
                   double   *longitude,
                   double   *julian_date,
                   float    *threshold,
                   int      *fps,
                   float    *animation_mult,
                   int      *no_unicode,
                   int      *color,
                   int      *grid)
{
    // flags (TODO: this is weird way around needing const flags in struct?)
    static int no_unicode_flag;
    static int color_flag;
    static int grid_flag;

    // https://azrael.digipen.edu/~mmead/www/mg/getopt/index.html
    int c;
    bool input_error = false;

    while (1)
    {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"latitude",        required_argument,  NULL,               'a'},
            {"longitude",       required_argument,  NULL,               'o'},
            {"julian-date",     required_argument,  NULL,               'j'},
            {"threshold",       required_argument,  NULL,               't'},
            {"fps",             required_argument,  NULL,               'f'},
            {"animation-mult",  required_argument,  NULL,               'm'},
            {"no-unicode",      no_argument,        &no_unicode_flag,   1},
            {"color",           no_argument,        &color_flag,        1},
            {"grid",            no_argument,        &grid_flag,         1},
            {NULL,              0,                  NULL,               0}
        };

        c = getopt_long(argc, argv, ":a:l:j:f:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 0:
            break;

        case 1:
            break;

        case 'a':
            *latitude = atof(optarg);
            break;

        case 'l':
            *longitude = atof(optarg);
            break;

        case 'j':
            *julian_date = atof(optarg);
            break;

        case 't':
            *threshold = atof(optarg);
            break;

        case 'f':
            *fps = atoi(optarg);
            break;

        case 'm':
            *animation_mult = atof(optarg);
            break;

        case '?':
            if (optopt == 0)
            {
                printf("Unrecognized long option\n");
            }
            else
            {
                printf("Unrecognized option '%c'\n", optopt);
            }
            input_error = true;
            break;

        case ':':
            printf("Missing option for '%c'\n", optopt);
            input_error = true;
            break;

        default:
            printf("?? getopt returned character code 0%o ??\n", c);
            input_error = true;
            break;
        }
    }

    // TODO: weird stuff
    *no_unicode = no_unicode_flag;
    *color = color_flag;
    *grid = grid_flag;

    return input_error;
}