#include "astro.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef AU
#define AU 149597870.691
#endif

/* Normalize a radian angle to [0, 2π]
 */
static double norm_rad(double rad)
{
    double rem = fmod(rad, 2 * M_PI);
    rem += rem < 0 ? 2 * M_PI : 0;
    return rem;
}

void calc_star_position(double right_ascension, double ra_motion, double declination, double dec_motion, double julian_date,
                        double *ITRF_right_ascension, double *ITRF_declination)
{
    double J2000 = 2451545.0;        // J2000 epoch in julian days
    double days_per_year = 365.2425; // Average number of days per year
    double years_from_epoch = (julian_date - J2000) / days_per_year;

    *ITRF_right_ascension = right_ascension + ra_motion * years_from_epoch;
    *ITRF_declination = declination + dec_motion * years_from_epoch;
}

double earth_rotation_angle_rad(double jd)
{
    // IERS Technical Note No. 32: 5.4.4 eq. 14

    double t = jd - 2451545.0;
    double d = jd - floor(jd);

    double theta = 2.0 * M_PI * (d + 0.7790572732640 + 0.00273781191135448 * t);
    theta = norm_rad(theta);

    return theta;
}

double greenwich_mean_sidereal_time_rad(double jd)
{
    // "Expressions for IAU 2000 precession quantities,"
    // N.Capitaine, P.T.Wallace, and J.Chapront, eq. 42

    // Calculate Julian centuries after J2000
    double t = (jd - 2451545.0) / 36525.0;

    // This isn't explicitly stated, but I believe this gives the accumulated
    // precession as described in https://en.wikipedia.org/wiki/Sidereal_time
    double acc_precession_sec = -0.014506 - 4612.156534 * t - 1.3915817 * pow(t, 2) + 0.00000044 * pow(t, 3) +
                                0.000029956 * pow(t, 4) + 0.0000000368 * pow(t, 5);

    // Convert to degrees then radians
    double acc_precession_rad = acc_precession_sec / 3600.0 * M_PI / 180.0;

    double gmst = earth_rotation_angle_rad(jd) - acc_precession_rad;
    gmst = norm_rad(gmst);

    return gmst;
}

double datetime_to_julian_date(struct tm *time)
{
    // Convert ISO C tm struct to Gregorian datetime format
    int second = time->tm_sec;
    int minute = time->tm_min;
    int hour = time->tm_hour;
    int day = time->tm_mday;
    int month = time->tm_mon + 1;
    int year = time->tm_year + 1900;

    // https://orbital-mechanics.space/reference/julian-date.html
    int a = (month - 14) / 12;          // eq 436
    int b = 1461 * (year + 4800 + a);   // eq 436
    int c = 367 * (month - 2 - 12 * a); // eq 436
    int e = (year + 4900 + a) / 100;    // eq 436

    int julian_day_num = b / 4 + c / 12 - (3 * e) / 4 + day - 32075; // eq 437

    // Determine fraction of seconds that have passed in one day
    double julian_day_frac = (hour - 12) / 24.0 + minute / (24.0 * 60.0) + second / (24.0 * 60.0 * 60.0);

    return julian_day_num + julian_day_frac;
}

struct tm julian_date_to_datetime(double julian_date)
{
    // https://orbital-mechanics.space/reference/julian-date.html
    int julian_day_num = (int)julian_date;

    int l = julian_day_num + 68569;
    int n = 4 * l / 146097;
    l = l - (146097 * n + 3) / 4;
    int i = 4000 * (l + 1) / 1461001;
    l = l - 1461 * i / 4 + 31;
    int j = 80 * l / 2447;

    int day = l - 2447 * j / 80;
    l = j / 11;
    int month = j + 2 - 12 * l;
    int year = 100 * (n - 49) + i + l;

    double julian_day_frac = julian_date - julian_day_num;

    double hour_d = julian_day_frac * 24.0 + 12; // add twelve because of weird offset
    double minute_d = (hour_d - floor(hour_d)) * 60.0;
    double second_d = (minute_d - floor(minute_d)) * 60.0;

    // Convert Gregorian datetime format to ISO C tm struct
    struct tm time = {
        .tm_sec = floor(second_d),
        .tm_min = floor(minute_d),
        .tm_hour = floor(hour_d),
        .tm_mday = day,
        .tm_mon = month - 1,
        .tm_year = year - 1900,
    };

    // Adjust all fields to usual range
    mktime(&time);

    return time;
}

double current_julian_date(void)
{
    time_t t = time(NULL);
    struct tm lt = *gmtime(&t); // UTC
    return datetime_to_julian_date(&lt);
}

static double solve_kepler(double M, double e, double E)
{
    const double to_rad = M_PI / 180.0;

    double dM = M - (E - e / to_rad * sin(E * to_rad));
    double dE = dM / (1.0 - e * cos(E * to_rad));

    return dE;
}

/* Calculate the heliocentric ICRF position of a planet in rectangular
 * equatorial coordinates
 */
void calc_planet_helio_ICRF(const struct kep_elems *elements, const struct kep_rates *rates, const struct kep_extra *extras,
                            double julian_date, double *xh, double *yh, double *zh)
{
    // Explanatory Supplement to the Astronomical Almanac: Chapter 8,  Page 340

    const double to_rad = M_PI / 180.0;

    // 1.

    // Calculate number of centuries past J2000
    double t = (julian_date - 2451545.0) / 36525.0;

    double a = elements->a + rates->da * t;
    double e = elements->e + rates->de * t;
    double I = elements->I + rates->dI * t;
    double M = elements->M + rates->dM * t;
    double w = elements->w + rates->dw * t;
    double O = elements->O + rates->dO * t;

    double L = M + w + O; // Mean longitude
    double w_bar = w + O; // Longitude of perihelion

    // 2.
    if (extras != NULL)
    {
        double b = extras->b;
        double c = extras->c;
        double s = extras->s;
        double f = extras->f;
        M = L - w_bar + b * t * t + c * cos(f * t * to_rad) + s * sin(f * t * to_rad);
    }

    // 3.

    while (M > 180.0)
    {
        M -= 360.0;
    }

    double e_star = 180.0 / M_PI * e;
    double E = M + e_star * sin(M * to_rad);

    double dE = 1.0;
    int n = 0;
    while (fabs(dE) > 1E-6 && n < 10)
    {
        dE = solve_kepler(M, e, E);
        E += dE;
        n++;
    }

    // 4.

    const double xp = a * (cos(E * to_rad) - e);
    const double yp = a * sqrt(1.0 - e * e) * sin(E * to_rad);
    const double zp = 0.0;

    // 5.

    a *= to_rad;
    e *= to_rad;
    I *= to_rad;
    L *= to_rad;
    w *= to_rad;
    O *= to_rad;
    double xecl = (cos(w) * cos(O) - sin(w) * sin(O) * cos(I)) * xp + (-sin(w) * cos(O) - cos(w) * sin(O) * cos(I)) * yp;
    double yecl = (cos(w) * sin(O) + sin(w) * cos(O) * cos(I)) * xp + (-sin(w) * sin(O) + cos(w) * cos(O) * cos(I)) * yp;
    double zecl = (sin(w) * sin(I)) * xp + (cos(w) * sin(I)) * yp;

    // 6.

    // Obliquity at J2000 in radians
    double eps = 84381.448 / (60.0 * 60.0) * to_rad;

    *xh = xecl;
    *yh = cos(eps) * yecl - sin(eps) * zecl;
    *zh = sin(eps) * yecl + cos(eps) * zecl;

    return;
}

/* Correct ICRF for polar motion, precession, nutation, frame bias & earth
 * rotation
 *
 * According to ASCOM, *true* apparent coordinates would also correct for other
 * elements but those are beyond the scope of this project
 * https://ascom-standards.org/Help/Developer/html/72A95B28-BBE2-4C7D-BC03-2D6AB324B6F7.htm
 */
void ICRF_to_ITRF(double *x, double *y, double *z)
{
    // TODO: implement concise CIO/CEO based transformations
    *x = *x;
    *y = *y;
    *z = *z;
}

void calc_planet_geo_ICRF(double xe, double ye, double ze, const struct kep_elems *planet_elements,
                          const struct kep_rates *planet_rates, const struct kep_extra *planet_extras, double julian_date,
                          double *xg, double *yg, double *zg)
{
    // Coordinates of desired planet
    double xh, yh, zh;
    calc_planet_helio_ICRF(planet_elements, planet_rates, planet_extras, julian_date, &xh, &yh, &zh);

    // Obtain geocentric coordinates by subtracting Earth's coordinates
    *xg = xh - xe;
    *yg = yh - ye;
    *zg = zh - ze;

    return;
}

void calc_moon_geo_ICRF(const struct kep_elems *moon_elements, const struct kep_rates *moon_rates, double julian_date,
                        double *xg, double *yg, double *zg)
{
    // Algorithm taken from Paul Schlyter's page "How to compute planetary
    // positions" https://stjarnhimlen.se/comp/ppcomp.html#6 (modified)

    // https: //
    // astronomy.stackexchange.com/questions/29522/moon-equatorial-coordinates

    const double to_rad = M_PI / 180.0;

    // When using Paul Schlyter's elements
    double d = julian_date - 2451543.5; // weird stuff here

    // When using NASA JPL elements (currently not working)
    // Calculate number of centuries past J2000
    // double t = (julian_date - 2451544.5) / 36525.0;

    double a = moon_elements->a + moon_rates->da * d;
    double e = moon_elements->e + moon_rates->de * d;
    double I = moon_elements->I + moon_rates->dI * d;
    double M = moon_elements->M + moon_rates->dM * d;
    double w = moon_elements->w + moon_rates->dw * d;
    double O = moon_elements->O + moon_rates->dO * d;

    while (M > 180.0)
    {
        M -= 360.0;
    }

    // Compute the eccentric anomaly, E
    double e_star = 180.0 / M_PI * e;
    double E = M + e_star * sin(M * to_rad) * (1.0 + e * cos(M * to_rad));

    double dE = 1.0;
    int n = 0;
    while (fabs(dE) > 1E-6 && n < 10)
    {
        dE = solve_kepler(M, e, E);
        E += dE;
        n++;
    }

    // Compute moon's geocentric  coordinates in its orbital plane
    double xp = a * (cos(E * to_rad) - e);
    double yp = a * sqrt(1.0 - e * e) * sin(E * to_rad);
    double zp = 0.0;

    // Compute the moon's position in 3-dimensional space in ecliptic coords
    I *= to_rad;
    w *= to_rad;
    O *= to_rad;
    M *= to_rad;
    double xecl = (cos(w) * cos(O) - sin(w) * sin(O) * cos(I)) * xp + (-sin(w) * cos(O) - cos(w) * sin(O) * cos(I)) * yp;
    double yecl = (cos(w) * sin(O) + sin(w) * cos(O) * cos(I)) * xp + (-sin(w) * sin(O) + cos(w) * cos(O) * cos(I)) * yp;
    double zecl = (sin(w) * sin(I)) * xp + (cos(w) * sin(I)) * yp;

    // Obliquity at J2000 in radians
    double eps = 84381.448 / (60.0 * 60.0) * to_rad;

    // Convert to equatorial coords
    *xg = xecl;
    *yg = cos(eps) * yecl - sin(eps) * zecl;
    *zg = sin(eps) * yecl + cos(eps) * zecl;

    return;
}

double calc_moon_phase(double julian_date)
{
    // A crude calculation for the phase of the moon
    // https://en.wikipedia.org/wiki/Lunar_phase

    double synodic_month = 29.53059;
    double age = (julian_date - 2451550.1) / synodic_month;
    return age - floor(age);
}

void julian_to_gregorian(double jd, int *year, int *month, int *day)
{
    // https://quasar.as.utexas.edu/BillInfo/JulianDatesG.html
    int J = (int)(jd + 0.5);
    int j_alpha = (int)(((J - 1867216.25) / 36524.25));
    int b = J + 1 + j_alpha - (int)(j_alpha / 4);
    int c = b + 1524;
    int d = (int)((c - 122.1) / 365.25);
    int e = (int)(365.25 * d);
    int g = (int)((c - e) / 30.6001);

    *day = c - e - (int)(30.6001 * g);
    *month = (g < 13.5) ? (g - 1) : (g - 13);
    *year = (*month > 2) ? (d - 4716) : (d - 4715);
}

const char *get_zodiac_sign(int day, int month)
{
    // Define Zodiac signs and date ranges
    static const char *zodiac_signs[] = {"Capricorn", "Aquarius", "Pisces", "Aries",   "Taurus",      "Gemini",   "Cancer",
                                         "Leo",       "Virgo",    "Libra",  "Scorpio", "Sagittarius", "Capricorn"};

    static const char *zodiac_symbols[] = {"♑", "♒", "♓", "♈", "♉", "♊", "♋", "♌", "♍", "♎", "♏", "♐", "♑"};

    static const int zodiac_start_days[] = {20, 19, 21, 21, 21, 21, 23, 23, 23, 23, 23, 22, 31};

    int index = (day < zodiac_start_days[month - 1]) ? (month - 1) : month;

    // Return the sign combined with its symbol
    static char result[50];
    snprintf(result, sizeof(result), "%s %s", zodiac_symbols[index], zodiac_signs[index]);
    return result;
}

const char *get_moon_phase_description(double julian_date)
{
    double phase = calc_moon_phase(julian_date);

    if (phase < 0.03 || phase > 0.97)
    {
        return "New Moon";
    }
    else if (phase < 0.25)
    {
        return "Waxing Crescent";
    }
    else if (phase < 0.27)
    {
        return "First Quarter";
    }
    else if (phase < 0.50)
    {
        return "Waxing Gibbous";
    }
    else if (phase < 0.53)
    {
        return "Full Moon";
    }
    else if (phase < 0.75)
    {
        return "Waning Gibbous";
    }
    else if (phase < 0.77)
    {
        return "Last Quarter";
    }
    else
    {
        return "Waning Crescent";
    }
}

void decimal_to_dms(double decimal_value, int *degrees, int *minutes, double *seconds)
{
    *degrees = (int)decimal_value;
    double fractional_part = fabs(decimal_value - *degrees);
    double total_minutes = fractional_part * 60;
    *minutes = (int)total_minutes;
    *seconds = (total_minutes - *minutes) * 60;

    if (decimal_value < 0)
    {
        *degrees = *degrees < 0 ? *degrees : -*degrees;
    }
}
