/* Core functions for updating object positions
 */

#ifndef CORE_POSITION_H
#define CORE_POSITION_H

#include "core.h"

/* Update apparent star positions for a given observation time and location by
 * setting the azimuth and altitude of each star struct in an array of star
 * structs
 */
void update_star_positions(struct Star *star_table, int num_stars, double julian_date, double latitude, double longitude);

/* Update apparent star positions only for stars brighter than or equal to
 * `threshold`, precomputing shared trigonometric terms and caching stereographic
 * polar coordinates for rendering.
 */
void update_star_positions_threshold(struct Star *star_table, int num_stars, double julian_date, double latitude,
                                     double longitude, float threshold);

/* Update apparent Sun & planet positions for a given observation time and
 * location by setting the azimuth and altitude of each planet struct in an
 * array of planet structs
 */
void update_planet_positions(struct Planet *planet_table, double julian_date, double latitude, double longitude);

/* Update apparent Moon positions for a given observation time and
 * location by setting the azimuth and altitude of a moon struct
 */
void update_moon_position(struct Moon *moon_object, double julian_date, double latitude, double longitude);

/* Update the phase of the Moon at a given time by setting the unicode symbol
 * for a moon struct
 */
void update_moon_phase(struct Moon *moon_object, double julian_date, double latitude);

#endif // CORE_POSITION_H
