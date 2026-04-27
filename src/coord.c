#include "coord.h"
#include "macros.h"

#include <math.h>

// Conversions

void equatorial_rectangular_to_spherical(double xeq, double yeq, double zeq, double *right_ascension, double *declination)
{
    *right_ascension = atan2(yeq, xeq);
    *declination = atan2(zeq, sqrt(xeq * xeq + yeq * yeq));
}

void ecliptic_to_equatorial(double ecliptic_longitude, double ecliptic_latitude, double *right_ascension, double *declination)
{
    const double obliquity = 84381.448 / (60.0 * 60.0) * TO_RAD;

    *right_ascension =
        atan2(sin(ecliptic_longitude) * cos(obliquity) - tan(ecliptic_latitude) * sin(obliquity), cos(ecliptic_longitude));
    if (*right_ascension < 0.0)
    {
        *right_ascension += 2.0 * M_PI;
    }

    *declination =
        asin(sin(ecliptic_latitude) * cos(obliquity) + cos(ecliptic_latitude) * sin(obliquity) * sin(ecliptic_longitude));
}

void equatorial_to_horizontal(double right_ascension, double declination, double gmst, double latitude, double longitude,
                              double *azimuth, double *altitude)
{
    // Astronomical Algorithms, Jean Meeus, eq. 13.5 & 13.6*
    // *modified so West longitudes are negative and Azimuth 0 is North
    // as done by Greg Miller (http://www.celestialprogramming.com/)

    // Compute the approximate hour angle (not corrected for nutation)
    double local_sidereal_time = fmod(gmst + longitude, 2.0 * M_PI);
    double hour_angle = local_sidereal_time - right_ascension;

    // Normalize hour angle to [0, π]...?
    if (hour_angle < 0.0)
    {
        hour_angle += 2.0 * M_PI;
    }
    if (hour_angle > M_PI)
    {
        hour_angle -= 2.0 * M_PI;
    }

    *altitude = asin(sin(latitude) * sin(declination) + cos(latitude) * cos(declination) * cos(hour_angle));

    *azimuth = atan2(sin(hour_angle), cos(hour_angle) * sin(latitude) - tan(declination) * cos(latitude));

    // Make Azimuth 0 at North
    *azimuth -= M_PI;
    if (*azimuth < 0.0)
    {
        *azimuth += 2.0 * M_PI;
    }

    return;
}

void horizontal_to_spherical(double azimuth, double altitude, double *point_theta, double *point_phi)
{
    *point_theta = M_PI / 2 - azimuth;
    *point_phi = M_PI / 2 - altitude;
}

// Projections

void project_stereographic(double sphere_radius, double point_theta, double point_phi, double center_theta, double center_phi,
                           double *radius_polar, double *theta_polar)
{
    // TODO: Check this math (theta is likely incorrect)

    // Map Projections - A Working Manual By John P.Snyder

    double c = fabs(center_phi - point_phi);      // Angular separation between center & point
    *radius_polar = sphere_radius * tan(c / 2.0); // eq (21 - 1) - dividing by 2 gives projection
                                                  // onto plane containing equator
    *theta_polar = point_theta - center_theta;    // eq (20 - 2) - FIXME: I believe this is incorrect
}

void project_stereographic_north(double sphere_radius, double theta_point, double point_phi, double *radius_polar,
                                 double *theta_polar)
{
    // Map Projections - A Working Manual By John P.Snyder

    double c = fabs(0.0 - point_phi);             // Angular separation between center
                                                  // (Φ_north_pole = 0) & point
    *radius_polar = sphere_radius * tan(c / 2.0); // eq (21 - 1) Dividing by 2 gives projection onto
                                                  // plane containing equator
    *theta_polar = M_PI - theta_point;            // eq (20 - 2) This is a little weird, but doing so makes
                                                  // the "North"
                                                  //             horizon is at the "top" of the projection
}

void project_stereographic_south(double sphere_radius, double point_theta, double point_phi, double *radius_polar,
                                 double *theta_polar)
{
    // Map Projections - A Working Manual By John P.Snyder

    double c = fabs(M_PI - point_phi);            // Angular separation between center
                                                  // (Φ_south_pole = π) & point
    *radius_polar = sphere_radius * tan(c / 2.0); // eq (21 - 1) Dividing by 2 gives projection onto
                                                  // plane containing equator
    *theta_polar = M_PI - point_theta;            // eq (20 - 2) This is a little weird, but doing so makes
                                                  // the "North"
                                                  //             horizon is at the "top" of the projection
}

// Screen space mapping

void polar_to_win(double r, double theta, int win_height, int win_width, int *row, int *col)
{
    int maxy = win_height - 1;
    int maxx = win_width - 1;

    double rad_y = maxy / 2.0;
    double rad_x = maxx / 2.0;

    // rad_y is flipped because y-axis is "flipped" in screen coordinates
    double row_d = r * -rad_y * sin(theta) + rad_y;
    double col_d = r * rad_x * cos(theta) + rad_x;

    *row = (int)round(row_d);
    *col = (int)round(col_d);
    return;
}

void perspective_to_win(double aov_phi, double aov_theta, double perspective_phi, double perspective_theta, double object_phi,
                        double object_theta, int win_height, int win_width, int *row, int *col)
{
    // Treat the view window as a "partial" frustum of a sphere
    // Map object coordinates as a "percentage" of this frustum

    double start_phi = perspective_phi - aov_phi / 2.0;
    double start_theta = perspective_theta - aov_theta / 2.0;

    double row_d = (start_phi - object_phi) / aov_phi * win_height;
    double col_d = (object_theta - start_theta) / aov_theta * win_width;

    *row = (int)round(row_d);
    *col = (int)round(col_d);
    return;
}
