/* Coordinate system utilities.
 *
 * Coordinate systems definitions
 *  - All angles in radians unless otherwise noted
 *  - Using common mathematical conventions as opposed to physics
 *  - https://en.wikipedia.org/wiki/Spherical_coordinate_system
 *  - FIXME: this is not the ISO standard, maybe we should change it.
 *
 * DIRECTIONS
 * - North: corresponds with positive y-axis on cartesian grid          N
 * - East:  corresponds with positive x-axis on cartesian grid      W       E
 * - South: corresponds with negative y-axis on cartesian grid          S
 * - West:  corresponds with negative x-axis on cartesian grid
 *
 * GEOGRAPHIC (longitude, latitude)
 * - Longitude      : positive East & negative West of the Prime Meridian   [-π,
 * π]
 * - Latitude       : positive North & negative South of the Equator [-π/2, π/2]
 *
 * POLAR (r, θ)
 *  - Radius    (r)
 *  - Theta     (θ) : measured North of East
 *
 * SPHERICAL (ρ, θ, Φ)
 *  - Rho       (ρ)
 *  - Theta     (θ) : measured North of East
 *  - Phi       (Φ) : angular distance from positive z-axis
 *
 * HORIZONTAL (azimuth, altitude)
 * - Azimuth        : measured East of North
 * - Altitude       : measured from equator to the zenith (complement of
 * spherical phi)
 *
 * EQUATORIAL-SPHERICAL (right ascension, declination)
 * - Right ascension    : measured East of the Vernal Equinox along the
 * Celestial Equator
 * - Declination        : measured North of the Celestial Equator, along the
 * hour circle passing through the point in question
 *
 * See: https://en.wikipedia.org/wiki/Equatorial_coordinate_system
 */

#ifndef COORD_H
#define COORD_H

// CONVERSIONS

/* Converts equatorial coordinates (global) to horizontal coordinates (local)
 *
 * Reference:
 * https://jonvoisey.net/blog/2018/07/data-converting-alt-az-to-ra-dec-derivation/
 *              https://astrogreg.com/convert_ra_dec_to_alt_az.html
 */
void equatorial_to_horizontal(double right_ascension, double declination, double gmst, double latitude, double longitude,
                              double *azimuth, double *altitude);

/* Converts rectangular equatorial coordinates to spherical rectangular
 * coordinates
 */
void equatorial_rectangular_to_spherical(double xeq, double yeq, double zeq, double *right_ascension, double *declination);

/* Converts horizontal coordinates to spherical coordinates
 */
void horizontal_to_spherical(double azimuth, double altitude, double *theta_sphere, double *phi_sphere);

// MAP PROJECTIONS

/* Generalized stereographic projection centered on a generic focus point
 *
 * (R, θ, Φ) -> (r, θ)
 *
 * Maps a point on a sphere with radius R to the plane containing the center of
 * the sphere and orthogonal to the radius to the center point.
 *
 * Reference:
 * https://www.atractor.pt/mat/loxodromica/saber_estereografica1-_en.html
 *              https://en.wikipedia.org/wiki/Stereographic_projection
 *              https://pubs.usgs.gov/pp/1395/report.pdf
 */
void project_stereographic(double sphere_radius, double point_theta, double point_phi, double center_theta, double center_phi,
                           double *radius_polar, double *theta_polar);

/* Stereographic projection centered on the south pole
 *
 * (R, θ, Φ) -> (r, θ)
 *
 * Maps a point on a sphere with radius R to the plane containing the equator
 * of the sphere. Since the focus point is the north pole the point will only
 * lie within the "equatorial orthodrome" (circle with radius R) if
 * π/2 < Φ < 3π/2.
 */
void project_stereographic_south(double radius_sphere, double theta_sphere, double phi_sphere, double *r_polar,
                                 double *theta_polar);

/* Stereographic projection centered on the north pole
 *
 * (R, θ, Φ) -> (r, θ)
 *
 * Maps a point  on a sphere with radius R to the plane containing the equator
 * of the sphere. Since the focus point is the south pole the point will only
 * lie within the "equatorial orthodrome" (circle with radius R) if
 * -π/2 < Φ < π/2.
 */
void project_stereographic_north(double radius_sphere, double theta_sphere, double phi_sphere, double *r_polar,
                                 double *theta_polar);

// SCREEN SPACE MAPPING

/* Maps point a point (r, θ) on the unit circle to screen space
 */
void polar_to_win(double r, double theta, int win_height, int win_width, int *row, int *col);

/* Maps a "partial spherical frustum" defined by the angle of view(s) and the
 * perspective angle to screen space
 */
void perspective_to_win(double aov_phi, double aov_theta, double perspective_phi, double perspective_theta, double object_phi,
                        double object_theta, int win_height, int win_width, int *row, int *col);

#endif // COORD_H
