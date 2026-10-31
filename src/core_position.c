#include "core_position.h"

#include "astro.h"
#include "coord.h"
#include "core.h"
#include "macros.h"

#include <math.h>

static void cache_object_projection(struct ObjectBase *base)
{
    double theta_sphere, phi_sphere;
    horizontal_to_spherical(base->azimuth, base->altitude, &theta_sphere, &phi_sphere);
    project_stereographic_north(1.0, theta_sphere, phi_sphere, &base->radius_polar, &base->theta_polar);
    base->proj_valid = true;
}

void update_star_positions_threshold(struct Star *star_table, int num_stars, double julian_date, double latitude,
                                     double longitude, float threshold)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);
    double local_sidereal_time = fmod(gmst + longitude, 2.0 * M_PI);
    double sin_lat = sin(latitude);
    double cos_lat = cos(latitude);

    const double J2000 = 2451545.0;
    const double days_per_year = 365.2425;
    double years_from_epoch = (julian_date - J2000) / days_per_year;

    for (int i = 0; i < num_stars; ++i)
    {
        struct Star *star = &star_table[i];

        if (star->magnitude > threshold)
        {
            star->base.proj_valid = false;
            continue;
        }

        double right_ascension = star->right_ascension + star->ra_motion * years_from_epoch;
        double declination = star->declination + star->dec_motion * years_from_epoch;

        double hour_angle = local_sidereal_time - right_ascension;
        if (hour_angle < 0.0)
        {
            hour_angle += 2.0 * M_PI;
        }
        if (hour_angle > M_PI)
        {
            hour_angle -= 2.0 * M_PI;
        }

        double sin_dec = sin(declination);
        double cos_dec = cos(declination);
        double sin_ha = sin(hour_angle);
        double cos_ha = cos(hour_angle);

        double sin_alt = sin_lat * sin_dec + cos_lat * cos_dec * cos_ha;
        if (sin_alt > 1.0)
        {
            sin_alt = 1.0;
        }
        if (sin_alt < -1.0)
        {
            sin_alt = -1.0;
        }
        double altitude = asin(sin_alt);
        double azimuth = atan2(sin_ha * cos_dec, cos_ha * sin_lat * cos_dec - sin_dec * cos_lat) - M_PI;
        if (azimuth < 0.0)
        {
            azimuth += 2.0 * M_PI;
        }

        star->base.azimuth = azimuth;
        star->base.altitude = altitude;
        cache_object_projection(&star->base);
    }
}

void update_star_positions(struct Star *star_table, int num_stars, double julian_date, double latitude, double longitude)
{
    update_star_positions_threshold(star_table, num_stars, julian_date, latitude, longitude, 100.0f);
}

void update_planet_positions(struct Planet *planet_table, double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    // Heliocentric coordinates of the Earth-Moon barycenter (computed once per frame)
    double xe, ye, ze;
    calc_planet_helio_ICRF(planet_table[EARTH].elements, planet_table[EARTH].rates, planet_table[EARTH].extras, julian_date,
                           &xe, &ye, &ze);

    int i;
    for (i = SUN; i < NUM_PLANETS; ++i)
    {
        // Geocentric rectangular equatorial coordinates
        double xg, yg, zg;

        if (i == SUN)
        {
            // Since the origin of the ICRF frame is the barycenter of the Solar
            // System, (for our purposes this is roughly the position of the
            // Sun) we obtain the geocentric coordinates of the Sun by negating
            // the heliocentric coordinates of the Earth
            xg = -xe;
            yg = -ye;
            zg = -ze;
        }
        else
        {
            calc_planet_helio_ICRF(planet_table[i].elements, planet_table[i].rates, planet_table[i].extras, julian_date, &xg,
                                   &yg, &zg);

            // Obtain geocentric coordinates by subtracting Earth's coordinates
            xg -= xe;
            yg -= ye;
            zg -= ze;
        }

        // Convert to spherical equatorial coordinates
        double right_ascension, declination;
        equatorial_rectangular_to_spherical(xg, yg, zg, &right_ascension, &declination);

        double azimuth, altitude;
        equatorial_to_horizontal(right_ascension, declination, gmst, latitude, longitude, &azimuth, &altitude);

        planet_table[i].base.azimuth = azimuth;
        planet_table[i].base.altitude = altitude;
        cache_object_projection(&planet_table[i].base);
    }
}

void update_moon_position(struct Moon *moon_object, double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    double xg, yg, zg;
    calc_moon_geo_ICRF(moon_object->elements, moon_object->rates, julian_date, &xg, &yg, &zg);

    // Convert to spherical equatorial coordinates
    double right_ascension, declination;
    equatorial_rectangular_to_spherical(xg, yg, zg, &right_ascension, &declination);

    double azimuth, altitude;
    equatorial_to_horizontal(right_ascension, declination, gmst, latitude, longitude, &azimuth, &altitude);

    moon_object->base.azimuth = azimuth;
    moon_object->base.altitude = altitude;
    cache_object_projection(&moon_object->base);

    return;
}

// FIXME: this does not render the correct phase and angle
void update_moon_phase(struct Moon *moon_object, double julian_date, double latitude)
{
    double age = calc_moon_age(julian_date);
    enum MoonPhase phase = moon_age_to_phase(age);
    moon_object->base.symbol_unicode = get_moon_phase_image(phase, latitude >= 0);

    return;
}
