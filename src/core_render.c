#include "core_render.h"
#include "macros.h"

#include "coord.h"
#include "core.h"
#include "drawing.h"
#include "term.h"

#include <curses.h>
#include <math.h>
#include <stdlib.h>

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
            draw_line_smooth(win, ya, xa, yb, xb);
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

int gcd(int a, int b)
{
    while (b != 0)
    {
        int temp = a % b;

        a = b;
        b = temp;
    }
    return a;
}

int compare_angles(const void *a, const void *b)
{
    int x = *(int *)a;
    int y = *(int *)b;
    return (90 / gcd(x, 90)) < (90 / gcd(y, 90));
}

void render_azimuthal_grid(WINDOW *win, const struct Conf *config)
{
    const double to_rad = M_PI / 180.0;

    int height, width;
    getmaxyx(win, height, width);
    int maxy = height - 1;
    int maxx = width - 1;

    int rad_vertical = round(maxy / 2.0);
    int rad_horizontal = round(maxx / 2.0);

    // Possible step sizes in degrees (multiples of 5 and factors of 90)
    int step_sizes[5] = {10, 15, 30, 45, 90};
    int length = sizeof(step_sizes) / sizeof(step_sizes[0]);

    // Minimum number of rows separating grid line (at end of window)
    int min_height = 10;

    // Set the step size to the smallest desirable increment
    int inc;
    for (int i = length - 1; i >= 0; --i)
    {
        inc = step_sizes[i];
        if (round(rad_vertical * sin(inc * to_rad)) < min_height)
        {
            inc = step_sizes[--i]; // Go back to previous increment
            break;
        }
    }

    // Sort grid angles in the first quadrant by rendering priority
    int number_angles = 90 / inc + 1;
    int *angles = malloc(number_angles * sizeof(int));

    for (int i = 0; i < number_angles; ++i)
    {
        angles[i] = inc * i;
    }
    qsort(angles, number_angles, sizeof(int), compare_angles);

    // Draw angles in all four quadrants
    int quad;
    for (quad = 0; quad < 4; ++quad)
    {
        for (int i = 0; i < number_angles; ++i)
        {
            int angle = angles[i] + 90 * quad;

            int y = rad_vertical - round(rad_vertical * sin(angle * to_rad));
            int x = rad_horizontal + round(rad_horizontal * cos(angle * to_rad));

            if (config->unicode)
            {
                draw_line_smooth(win, y, x, rad_vertical, rad_horizontal);
            }
            else
            {
                draw_line_ASCII(win, y, x, rad_vertical, rad_horizontal);
            }

            int str_len = snprintf(NULL, 0, "%d", angle);
            char *label = malloc(str_len + 1);

            snprintf(label, str_len + 1, "%d", angle);

            // Offset to avoid truncating string
            int x_off = (x < rad_horizontal) ? 0 : -(str_len - 1);

            mvwaddstr(win, y, x + x_off, label);

            free(label);
        }
    }

    // while (angle <= 90.0)
    // {
    //     int rad_x = rad_horizontal * angle / 90.0;
    //     int rad_x = rad_vertical * angle / 90.0;
    //     // draw_ellipse(win, win->_maxy/2, win->_maxx/2, 20, 20,
    //     ascii); angle += inc;
    // }
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
