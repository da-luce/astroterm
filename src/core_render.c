#include "core_render.h"
#include "macros.h"

#include "coord.h"
#include "core.h"
#include "drawing.h"
#include "term.h"

#include <curses.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void horizontal_to_polar(double azimuth, double altitude, double *radius, double *theta)
{
    double theta_sphere, phi_sphere;
    horizontal_to_spherical(azimuth, altitude, &theta_sphere, &phi_sphere);

    project_stereographic_north(1.0, theta_sphere, phi_sphere, radius, theta);

    return;
}

void render_object_stereo(WINDOW *win, struct ObjectBase *object, const struct Conf *config)
{
    double radius_polar, theta_polar;
    horizontal_to_polar(object->azimuth, object->altitude, &radius_polar, &theta_polar);

    int y, x;
    int height, width;
    getmaxyx(win, height, width);
    polar_to_win(radius_polar, theta_polar, height, width, &y, &x);

    // If outside projection, ignore
    if (fabs(radius_polar) > 1)
    {
        return;
    }

    bool use_color = config->color && object->color_pair != 0;

    if (use_color)
    {
        wattron(win, COLOR_PAIR(object->color_pair));
    }

    // Draw object
    if (config->unicode)
    {
        mvwaddstr(win, y, x, object->symbol_unicode);
    }
    else
    {
        mvwaddch(win, y, x, object->symbol_ASCII);
    }

    // Draw label
    if (object->label != NULL)
    {
        mvwaddstr_truncate(win, y - 1, x + 1, object->label);
    }

    if (use_color)
    {
        wattroff(win, COLOR_PAIR(object->color_pair));
    }

    return;
}

void render_stars_stereo(WINDOW *win, const struct Conf *config, struct Star *star_table, int num_stars, const int *num_by_mag)
{
    int i;
    for (i = 0; i < num_stars; ++i)
    {
        int catalog_num = num_by_mag[i];
        int table_index = catalog_num - 1;

        struct Star *star = &star_table[table_index];

        if (star->magnitude > config->threshold)
        {
            continue;
        }

        // FIXME: this is hacky
        if (star->magnitude > config->label_thresh)
        {
            star->base.label = NULL;
        }

        render_object_stereo(win, &star->base, config);
    }

    return;
}

void render_constellation(WINDOW *win, const struct Conf *config, struct Constell *constellation, const struct Star *star_table)
{
    unsigned int num_segments = constellation->num_segments;

    // Only render if all stars are visible
    for (unsigned int i = 0; i < num_segments * 2; i += 1)
    {
        int catalog_num = constellation->star_numbers[i];
        int table_index = catalog_num - 1;
        struct Star star = star_table[table_index];
        if (star.magnitude > config->threshold)
        {
            return;
        }
    }

    for (unsigned int i = 0; i < num_segments * 2; i += 2)
    {
        int catalog_num_a = constellation->star_numbers[i];
        int catalog_num_b = constellation->star_numbers[i + 1];

        int table_index_a = catalog_num_a - 1;
        int table_index_b = catalog_num_b - 1;

        struct Star star_a = star_table[table_index_a];
        struct Star star_b = star_table[table_index_b];

        // TODO: Same code as in render_object_stereo... perhaps refactor this
        // or cache coordinates
        double radius_a, theta_a;
        double radius_b, theta_b;
        horizontal_to_polar(star_a.base.azimuth, star_a.base.altitude, &radius_a, &theta_a);
        horizontal_to_polar(star_b.base.azimuth, star_b.base.altitude, &radius_b, &theta_b);

        // Clip to edge of screen
        if (fabs(radius_a) > 1 && fabs(radius_b) > 1)
        {
            // Segment lies outside of screen
            continue;
        }

        bool a_clipped = false;
        bool b_clipped = false;

        // Clip the segment
        if (fabs(radius_a) > 1)
        {
            a_clipped = true;
            radius_a = 1.0;
        }
        else if (fabs(radius_b) > 1)
        {
            b_clipped = true;
            radius_b = 1.0;
        }

        int height, width;
        getmaxyx(win, height, width);

        int ya, xa;
        int yb, xb;
        polar_to_win(radius_a, theta_a, height, width, &ya, &xa);

        polar_to_win(radius_b, theta_b, height, width, &yb, &xb);

        // TODO: In old version, constrained line length for some reason... not
        // sure why?
        // FIXME: this logic is super verbose/long (any way to cut it down?)
        // FIXME: this clipping doesn't seem to work or no-unicode for some reason?
        if (config->unicode)
        {
            if (config->braille)
            {
                draw_line_braille(win, ya, xa, yb, xb);
            }
            else
            {
                draw_line_smooth(win, ya, xa, yb, xb);
            }
            if (!a_clipped)
            {
                mvwaddstr(win, ya, xa, "\u25CB"); // Unicode circle symbol
            }
            if (!b_clipped)
            {
                mvwaddstr(win, yb, xb, "\u25CB");
            }
        }
        else
        {
            draw_line_ASCII(win, ya, xa, yb, xb);
            if (!a_clipped)
            {
                mvwaddch(win, ya, xa, '+');
            }
            if (!b_clipped)
            {
                mvwaddch(win, yb, xb, '+');
            }
        }
    }
}

void render_constells(WINDOW *win, const struct Conf *config, struct Constell **constell_table, int num_const,
                      const struct Star *star_table)
{
    clear_braille_lines();
    for (int i = 0; i < num_const; ++i)
    {
        struct Constell *constellation = &((*constell_table)[i]);
        render_constellation(win, config, constellation, star_table);
    }
}

void render_planets_stereo(WINDOW *win, const struct Conf *config, const struct Planet *planet_table)
{
    // Render planets so that closest are drawn on top
    int i;
    for (i = NUM_PLANETS - 1; i >= 0; --i)
    {
        // Skip rendering the Earth--we're on the Earth! The geocentric
        // coordinates of the Earth are (0.0, 0.0, 0.0) and plotting the "Earth"
        // simply traces along the ecliptic at the approximate hour angle
        if (i == EARTH)
        {
            continue;
        }

        struct Planet planet_data = planet_table[i];
        render_object_stereo(win, &planet_data.base, config);
    }

    return;
}

void render_moon_stereo(WINDOW *win, const struct Conf *config, struct Moon moon_object)
{
    render_object_stereo(win, &moon_object.base, config);

    return;
}

// ---------------------------------------------------------------------------
// Azimuthal (alt-az) grid
// ---------------------------------------------------------------------------
//
// The grid is a light, dashed graticule drawn *behind* all sky objects:
//
//  - altitude rings (30 and 60 deg, more on large windows)
//  - azimuth spokes (every 30/45/90 deg, depending on window size) running
//    from the 60 deg ring out to the horizon, so they never pile up at the
//    zenith
//  - compass azimuth labels at the outer end of each spoke, and altitude labels
//    set into each ring
//
// Every grid point is mapped with horizontal_to_polar() + polar_to_win(), the
// same functions used for stars. So the grid lines up with the sky exactly,
// including orientation (N up, E left, as seen looking up at the sky).
//
// Dashes take their glyph from the local direction of the line ('-', '/', ':'
// or '\' in ASCII; '─' and '│' replace '-' and ':' in Unicode). None of
// these are used as star symbols, so dashes can't be confused with faint stars.

#define GRID_MAX_RINGS 5
#define GRID_MAX_LABELS 32

// Color pair used for the grid (same blue as the cardinal directions)
#define GRID_COLOR_PAIR 5

struct GridLabel
{
    int y;
    int x0;
    int x1;
    bool dim;
    char text[12];
};

struct GridState
{
    WINDOW *win;
    const struct Conf *config;
    int height;
    int width;
    // Layout
    int spoke_step;
    int spoke_start;
    int num_rings;
    int rings[GRID_MAX_RINGS];
    // Placed labels
    struct GridLabel labels[GRID_MAX_LABELS];
    int num_labels;
};

/* Map a horizontal coordinate (degrees) to a continuous, normalized screen
 * position where the horizon is the unit circle, +x is screen right and +y is
 * screen up
 */
static void grid_normalized(double az_deg, double alt_deg, double *nx, double *ny)
{
    double r, theta;
    horizontal_to_polar(az_deg * TO_RAD, alt_deg * TO_RAD, &r, &theta);
    *nx = r * cos(theta);
    *ny = r * sin(theta);
}

/* Map a horizontal coordinate (degrees) to a window cell. Same mapping as
 * polar_to_win(), but values that land exactly halfway between two cells
 * (the center line of even-sized windows) always round the same way, up, as the
 * N/E/S/W letters do. Otherwise floating point noise in sin/cos (e.g.
 * cos(90deg) = +6e-17, cos(270deg) = -2e-16) splits opposite spokes onto
 * neighboring columns/rows
 */
static void grid_cell(const struct GridState *g, double az_deg, double alt_deg, int *row, int *col)
{
    double r, theta;
    horizontal_to_polar(az_deg * TO_RAD, alt_deg * TO_RAD, &r, &theta);

    double rad_y = (g->height - 1) / 2.0;
    double rad_x = (g->width - 1) / 2.0;

    double row_d = r * -rad_y * sin(theta) + rad_y;
    double col_d = r * rad_x * cos(theta) + rad_x;

    const double eps = 1e-9;
    *row = (int)floor(row_d + 0.5 + eps);
    *col = (int)floor(col_d + 0.5 + eps);
}

static bool grid_in_bounds(const struct GridState *g, int row, int col)
{
    return row >= 0 && row < g->height && col >= 0 && col < g->width;
}

/* Choose a dash glyph from the on-screen direction (dx, dy) of the line, with
 * +y pointing up. The window is visually square, so normalized coordinates
 * give the true visual angle.
 */
static void grid_put_dash(const struct GridState *g, int row, int col, double dx, double dy)
{
    double angle = atan2(dy, dx) / TO_RAD; // (-180, 180]
    if (angle < 0)
    {
        angle += 180.0; // Direction, not orientation: fold to [0, 180)
    }

    bool unicode = g->config->unicode;

    if (angle < 22.5 || angle >= 157.5)
    {
        if (unicode)
        {
            mvwaddstr(g->win, row, col, "─");
        }
        else
        {
            mvwaddch(g->win, row, col, '-');
        }
    }
    else if (angle < 67.5)
    {
        mvwaddch(g->win, row, col, '/');
    }
    else if (angle < 112.5)
    {
        if (unicode)
        {
            mvwaddstr(g->win, row, col, "│");
        }
        else
        {
            mvwaddch(g->win, row, col, ':');
        }
    }
    else
    {
        mvwaddch(g->win, row, col, '\\');
    }
}

/* True if the cell belongs to a label (plus one cell of padding on each side),
 * so lines leave a clean gap for the label text
 */
static bool grid_reserved(const struct GridState *g, int row, int col)
{
    for (int i = 0; i < g->num_labels; ++i)
    {
        const struct GridLabel *l = &g->labels[i];
        if (l->y == row && col >= l->x0 - 1 && col <= l->x1 + 1)
        {
            return true;
        }
    }
    return false;
}

/* Trace a path of horizontal coordinates from (az0, alt0) to (az1, alt1),
 * interpolated linearly in (az, alt), as a "stitched" line: one glyph every
 * `GRID_STITCH` rows of on-screen length, oriented along the path. Even
 * spacing by on-screen length keeps the pattern regular in every direction
 */
#define GRID_STITCH 1.5

static void grid_trace(const struct GridState *g, double az0, double alt0, double az1, double alt1)
{
    double radius_rows = (g->height - 1) / 2.0;

    // Sample finely enough that stitch positions are accurate to a cell
    double span_deg = fmax(fabs(az1 - az0), fabs(alt1 - alt0));
    int cells = MAX(g->width, g->height);
    int samples = MAX(8, (int)ceil(span_deg * TO_RAD * cells * 2.0));

    double prev_x = 0.0, prev_y = 0.0;
    double length = 0.0; // Distance travelled along the path, in rows
    double next_stitch = 0.0;
    int last_row = INT_MIN, last_col = INT_MIN;

    for (int i = 0; i <= samples; ++i)
    {
        double t = (double)i / samples;
        double az = az0 + (az1 - az0) * t;
        double alt = alt0 + (alt1 - alt0) * t;

        double x, y;
        grid_normalized(az, alt, &x, &y);
        double dx = x - prev_x;
        double dy = y - prev_y;
        if (i > 0)
        {
            // The window is visually square: normalized units scale equally
            length += hypot(dx, dy) * radius_rows;
        }
        prev_x = x;
        prev_y = y;

        if (length < next_stitch)
        {
            continue;
        }
        next_stitch += GRID_STITCH;

        int row, col;
        grid_cell(g, az, alt, &row, &col);
        if ((row == last_row && col == last_col) || !grid_in_bounds(g, row, col) || grid_reserved(g, row, col))
        {
            continue;
        }
        last_row = row;
        last_col = col;

        // Local direction of the path, from a short step forward
        double t2 = fmin(t + 1e-3, 1.0);
        double t1 = t2 - 1e-3;
        double ax, ay, bx, by;
        grid_normalized(az0 + (az1 - az0) * t1, alt0 + (alt1 - alt0) * t1, &ax, &ay);
        grid_normalized(az0 + (az1 - az0) * t2, alt0 + (alt1 - alt0) * t2, &bx, &by);

        grid_put_dash(g, row, col, bx - ax, by - ay);
    }
}

/* Reserve a label centered on (row, col), clamped inside the window. Labels
 * overlapping a previously placed label are skipped
 */
static void grid_place_label(struct GridState *g, int row, int col, const char *text, bool dim)
{
    // Display width: count UTF-8 lead bytes only (labels may contain '°')
    int len = 0;
    for (const char *c = text; *c != '\0'; ++c)
    {
        len += ((*c & 0xC0) != 0x80);
    }

    if (row < 0 || row >= g->height || len > g->width || g->num_labels >= GRID_MAX_LABELS)
    {
        return;
    }

    int x0 = col - len / 2;
    x0 = MAX(0, MIN(x0, g->width - len));
    int x1 = x0 + len - 1;

    for (int i = 0; i < g->num_labels; ++i)
    {
        const struct GridLabel *l = &g->labels[i];
        // One cell of padding around each label
        if (l->y == row && x0 <= l->x1 + 1 && x1 >= l->x0 - 1)
        {
            return;
        }
    }

    struct GridLabel *l = &g->labels[g->num_labels++];
    l->y = row;
    l->x0 = x0;
    l->x1 = x1;
    l->dim = dim;
    snprintf(l->text, sizeof(l->text), "%s", text);
}

/* Compute the grid layout (density, rings, spokes, and label positions) for
 * the current window size. Returns false if the window is too small for a grid
 */
static bool grid_setup(struct GridState *g, WINDOW *win, const struct Conf *config)
{
    g->win = win;
    g->config = config;
    g->num_labels = 0;
    getmaxyx(win, g->height, g->width);

    if (g->height < 5 || g->width < 5)
    {
        return false; // Too small for a meaningful grid
    }

    // Choose density from the vertical radius of the projection (in rows)
    double radius_rows = (g->height - 1) / 2.0;

    if (radius_rows < 8)
    {
        g->spoke_step = 90;
        g->num_rings = 1;
        g->rings[0] = 45;
    }
    else if (radius_rows < 16)
    {
        g->spoke_step = 45;
        g->num_rings = 2;
        g->rings[0] = 30;
        g->rings[1] = 60;
    }
    else if (radius_rows < 35)
    {
        g->spoke_step = 30;
        g->num_rings = 2;
        g->rings[0] = 30;
        g->rings[1] = 60;
    }
    else
    {
        g->spoke_step = 30;
        g->num_rings = 5;
        for (int i = 0; i < g->num_rings; ++i)
        {
            g->rings[i] = 15 * (i + 1);
        }
    }

    // Spokes start at the highest ring at or below 60 deg so they stay clear of
    // the zenith
    g->spoke_start = g->rings[0];
    for (int i = 0; i < g->num_rings; ++i)
    {
        if (g->rings[i] <= 60)
        {
            g->spoke_start = g->rings[i];
        }
    }

    char buf[12];

    // Compass azimuth at the outer end of each spoke, ~1.5 rows inside the
    // horizon. Cardinal directions are skipped: N/E/S/W letters mark those
    double label_alt = 90.0 - 2.0 * atan(1.0 - 1.5 / radius_rows) / TO_RAD;
    for (int az = 0; az < 360; az += g->spoke_step)
    {
        if (az % 90 == 0)
        {
            continue;
        }
        int row, col;
        grid_cell(g, az, label_alt, &row, &col);
        snprintf(buf, sizeof(buf), config->unicode ? "%d°" : "%d", az);
        grid_place_label(g, row, col, buf, false);
    }

    // Altitude labels set into each ring, halfway between the North spoke and
    // the next one. Dimmed like the rings so they read as part of them, and
    // stay distinct from the azimuth labels
    double ring_label_az = g->spoke_step / 2.0;
    for (int i = 0; i < g->num_rings; ++i)
    {
        int row, col;
        grid_cell(g, ring_label_az, g->rings[i], &row, &col);
        snprintf(buf, sizeof(buf), "%d", g->rings[i]);
        grid_place_label(g, row, col, buf, true);
    }

    return true;
}

void render_azimuthal_grid(WINDOW *win, const struct Conf *config)
{
    struct GridState g;
    if (!grid_setup(&g, win, config))
    {
        return;
    }

    // Lines: dim, so they recede behind the sky. Cells reserved for labels are
    // left blank (see grid_trace)
    wattron(win, A_DIM);
    if (config->color)
    {
        wattron(win, COLOR_PAIR(GRID_COLOR_PAIR));
    }

    for (int i = 0; i < g.num_rings; ++i)
    {
        grid_trace(&g, 0.0, g.rings[i], 360.0, g.rings[i]);
    }

    for (int az = 0; az < 360; az += g.spoke_step)
    {
        grid_trace(&g, az, g.spoke_start, az, 0.0);
    }

    if (config->color)
    {
        wattroff(win, COLOR_PAIR(GRID_COLOR_PAIR));
    }
    wattroff(win, A_DIM);
}

void render_azimuthal_grid_labels(WINDOW *win, const struct Conf *config)
{
    struct GridState g;
    if (!grid_setup(&g, win, config))
    {
        return;
    }

    if (config->color)
    {
        wattron(win, COLOR_PAIR(GRID_COLOR_PAIR));
    }

    for (int i = 0; i < g.num_labels; ++i)
    {
        const struct GridLabel *l = &g.labels[i];
        if (l->dim)
        {
            wattron(win, A_DIM);
        }
        mvwaddstr(win, l->y, l->x0, l->text);
        if (l->dim)
        {
            wattroff(win, A_DIM);
        }
    }

    if (config->color)
    {
        wattroff(win, COLOR_PAIR(GRID_COLOR_PAIR));
    }
}

void render_cardinal_directions(WINDOW *win, const struct Conf *config)
{
    // Render horizon directions

    if (config->color)
    {
        wattron(win, COLOR_PAIR(5));
    }

    int height, width;
    getmaxyx(win, height, width);
    int maxy = height - 1;
    int maxx = width - 1;

    int half_maxy = round(maxy / 2.0);
    int half_maxx = round(maxx / 2.0);

    mvwaddch(win, 0, half_maxx, 'N');
    mvwaddch(win, half_maxy, width - 1, 'W');
    mvwaddch(win, height - 1, half_maxx, 'S');
    mvwaddch(win, half_maxy, 0, 'E');

    if (config->color)
    {
        wattroff(win, COLOR_PAIR(5));
    }
}
