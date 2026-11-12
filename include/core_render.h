/* Core functions for rendering
 */

#ifndef CORE_RENDER_H
#define CORE_RENDER_H

#include "core.h"

#include <curses.h>

/* Render stars to the screen using a stereographic projection
 */
void render_stars_stereo(WINDOW *win, const struct Conf *config, struct Star *star_table, int num_stars, const int *num_by_mag);

/* Render the Sun and planets to the screen using a stereographic projection
 */
void render_planets_stereo(WINDOW *win, const struct Conf *config, const struct Planet *planet_table);

/* Render the Moon to the screen using a stereographic projection
 */
void render_moon_stereo(WINDOW *win, const struct Conf *config, struct Moon moon_object);

/* Render constellations
 */
void render_constells(WINDOW *win, const struct Conf *config, struct Constell **constell_table, int num_const,
                      const struct Star *star_table);

/* Render the lines of a dashed alt-az grid (altitude rings and azimuth spokes)
 * on the stereographic projection. Intended to be drawn before sky objects so
 * that they appear on top of it
 */
void render_azimuthal_grid(WINDOW *win, const struct Conf *config);

/* Render the labels of the alt-az grid (compass azimuths and ring altitudes).
 * Intended to be drawn after sky objects so that labels stay legible
 */
void render_azimuthal_grid_labels(WINDOW *win, const struct Conf *config);

/* Render cardinal direction indicators for the Northern, Eastern, Southern, and
 * Western horizons
 */
void render_cardinal_directions(WINDOW *win, const struct Conf *config);

#endif // CORE_RENDER_H
