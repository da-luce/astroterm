/* Test high level functions. This is the closest we will get to testing `main.c`.
 */

#include "bsc5.h"
#include "bsc5_constellations.h"
#include "bsc5_names.h"
#include "core.h"
#include "core_position.h"
#include "data/keplerian_elements.h"
#include "macros.h"
#include "unity.h"

#include <stdlib.h>
#include <string.h>

// Initialize data structs
static unsigned int num_stars, num_const;

static struct Entry *BSC5_entries;
static struct StarName *name_table;
static struct Star *star_table;
struct Constell *constell_table;
static int *num_by_mag;
struct Planet *planet_table;
struct Moon moon_object;

void setUp(void)
{
    parse_entries(bsc5, bsc5_len, &BSC5_entries, &num_stars);
    generate_name_table(bsc5_names, bsc5_names_len, &name_table, num_stars);
    generate_star_table(&star_table, BSC5_entries, name_table, num_stars);
    star_numbers_by_magnitude(&num_by_mag, star_table, num_stars);
    generate_constell_table(bsc5_constellations, bsc5_constellations_len, &constell_table, &num_const);
    generate_planet_table(&planet_table, planet_elements, planet_rates, planet_extras);
    generate_moon_object(&moon_object, &moon_elements, &moon_rates);
}

void tearDown(void)
{
    free_constells(constell_table, num_const);
    free_stars(star_table, num_stars);
    free_planets(planet_table, NUM_PLANETS);
    free_moon_object(moon_object);
    free_star_names(name_table, num_stars);
}

// Tolerance for positions in radians. Planets are slightly less accurate. The moon is even less inaccurate.
// However, differences of these scales are very hard to notice on even a large terminal display
#define S_EPSILON 0.01
#define P_EPSILON 0.02
#define M_EPSILON 0.06

// The xrpm and xdpm vary slightly between the binary bsc5 file provided at
// http://tdc-www.harvard.edu/catalogs/BSC5 and the binary file we generate if
// using the ASCII version (http://tdc-www.harvard.edu/catalogs/ybsc5.gz) as
// the original source.
#define SMOTION_EPSILON 1e-20

// Function to trim trailing '\r' characters
char *trim_string(const char *str)
{
    if (str == NULL)
        return NULL;

    // Duplicate the string to ensure we don't modify the original
    char *trimmed = strdup(str);
    if (trimmed == NULL)
        return NULL;

    // Find the end of the string
    char *end = trimmed + strlen(trimmed) - 1;

    // Trim trailing '\r' characters
    while (end >= trimmed && *end == '\r')
    {
        *end = '\0';
        end--;
    }

    return trimmed;
}

void test_generate_star_table(void)
{
    TEST_ASSERT_NOT_NULL(star_table);

    // Verify first star
    TEST_ASSERT_EQUAL(1, star_table[0].catalog_number);
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 0.023, star_table[0].right_ascension);
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 0.789, star_table[0].declination);
    TEST_ASSERT_DOUBLE_WITHIN(SMOTION_EPSILON, -0.00000005817764048288, star_table[0].ra_motion);
    TEST_ASSERT_DOUBLE_WITHIN(SMOTION_EPSILON, -0.00000008726646427704, star_table[0].dec_motion);
    TEST_ASSERT_FLOAT_WITHIN(S_EPSILON, 6.7, star_table[0].magnitude);

    // Verify start with name
    TEST_ASSERT_EQUAL(7001, star_table[7000].catalog_number);
    TEST_ASSERT_EQUAL_STRING("Vega", trim_string(star_table[7000].base.label));

    // Verify star with no data
    int no_data_index = 92 - 1;
    TEST_ASSERT_EQUAL(92, star_table[no_data_index].catalog_number);
    TEST_ASSERT_EQUAL(0.0, star_table[no_data_index].right_ascension);
    TEST_ASSERT_EQUAL(0.0, star_table[no_data_index].declination);
    TEST_ASSERT_EQUAL(0.0, star_table[no_data_index].ra_motion);
    TEST_ASSERT_EQUAL(0.0, star_table[no_data_index].dec_motion);
    TEST_ASSERT_EQUAL(0.0, star_table[no_data_index].magnitude);

    // Verify star within index
    int index = 2025 - 1;
    TEST_ASSERT_EQUAL(2025, star_table[index].catalog_number);
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 1.53876226281558, star_table[index].right_ascension);
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 0.690704355203134, star_table[index].declination);
    TEST_ASSERT_DOUBLE_WITHIN(SMOTION_EPSILON, -0.000000126051560300766, star_table[index].ra_motion);
    TEST_ASSERT_DOUBLE_WITHIN(SMOTION_EPSILON, -0.0000000775701920474603, star_table[index].dec_motion);
    TEST_ASSERT_FLOAT_WITHIN(S_EPSILON, 6.45, star_table[index].magnitude);

    // Verify last star
    int last_index = 9110 - 1;
    TEST_ASSERT_EQUAL(9110, star_table[last_index].catalog_number);
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 0.022267, star_table[last_index].right_ascension);
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 1.070134, star_table[last_index].declination);
    TEST_ASSERT_DOUBLE_WITHIN(SMOTION_EPSILON, 0.0000000727220523799588, star_table[last_index].ra_motion);
    TEST_ASSERT_DOUBLE_WITHIN(SMOTION_EPSILON, 0.0000000242406841266529, star_table[last_index].dec_motion);
    TEST_ASSERT_FLOAT_WITHIN(S_EPSILON, 5.8, star_table[last_index].magnitude);
}

void test_generate_name_table(void)
{
    // Trim carriage returns so passed on windows
    TEST_ASSERT_NOT_NULL(name_table);
    TEST_ASSERT_EQUAL_STRING("Acamar", trim_string(name_table[896].name));
    TEST_ASSERT_EQUAL_STRING("Vega", trim_string(name_table[7000].name));
    TEST_ASSERT_EQUAL_STRING("Wezen", trim_string(name_table[2692].name));
    TEST_ASSERT_EQUAL_STRING("Zubeneschamali", trim_string(name_table[5684].name));
}

void test_generate_constell_table(void)
{
    // As seen in bsc5_constellations.txt
    // FIXME: if the order of constellations in the text file changes, this will break.
    TEST_ASSERT_NOT_NULL(constell_table);

    // Aql constellation
    TEST_ASSERT_EQUAL_INT(8, constell_table[0].num_segments);

    // CVn constellation
    TEST_ASSERT_EQUAL_INT(1, constell_table[19].num_segments);
    int expected_star_numbers[] = {4785, 4915};
    TEST_ASSERT_EQUAL_INT_ARRAY(expected_star_numbers, constell_table[19].star_numbers, 2);
}

void test_star_numbers_by_magnitude(void)
{
    TEST_ASSERT_NOT_NULL(num_by_mag);

    // Least bright
    TEST_ASSERT_EQUAL(1894, num_by_mag[0]);
    TEST_ASSERT_EQUAL(365, num_by_mag[1]);
    TEST_ASSERT_EQUAL(3313, num_by_mag[2]);

    // Brightest
    // Access the last elements (brightest stars)
    unsigned int last_index = num_stars - 1; // Assuming num_stars is defined elsewhere

    TEST_ASSERT_EQUAL(2491, num_by_mag[last_index]);
    TEST_ASSERT_EQUAL(2326, num_by_mag[last_index - 1]);
    TEST_ASSERT_EQUAL(5340, num_by_mag[last_index - 2]);

    free(num_by_mag);
}

void test_update_star_positions(void)
{
    // REMEMBER:
    // * Get a Julian Date: https://aa.usno.navy.mil/data/JulianDate
    // * Convert to EST and use Stellarium to check
    // * Convert DMS to Radians: https://calculator.academy/dms-to-radians-calculator/

    double julian_date = 2459146.0; // 2020 October 23 12:00:00.0 UT1
    // Boston, MA in radians
    double latitude = 42.3601 * M_PI / 180;
    double longitude = -71.0589 * M_PI / 180;

    update_star_positions(star_table, num_stars, julian_date, latitude, longitude);

    // Verify Vega's position is correct
    // https://stellarium-web.org/skysource/Vega?fov=120.00&date=2020-10-23T12:00:00Z&lat=42.36&lng=-71.06&elev=0
    TEST_ASSERT_EQUAL(7001, star_table[7000].catalog_number);
    TEST_ASSERT_EQUAL_STRING("Vega", trim_string(star_table[7000].base.label));
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 0.547246, star_table[7000].base.azimuth);
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 0.0, star_table[7000].base.altitude);

    // Verify Arcturus's position is correct
    // https://stellarium-web.org/skysource/Arcturus?fov=120.00&date=2020-10-23T12:00:00Z&lat=42.36&lng=-71.06&elev=0
    TEST_ASSERT_EQUAL(5340, star_table[5339].catalog_number);
    TEST_ASSERT_EQUAL_STRING("Arcturus", trim_string(star_table[5339].base.label));
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 1.511414, star_table[5339].base.azimuth);
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 0.440355, star_table[5339].base.altitude);
}

void test_update_planet_positions(void)
{
    double julian_date = 2459146.0; // 2020 October 23 12:00:00.0 UT1
    // Boston, MA in radians
    double latitude = 42.3601 * M_PI / 180;
    double longitude = -71.0589 * M_PI / 180;

    update_planet_positions(planet_table, julian_date, latitude, longitude);

    // Verify Sun's position is correct
    // https://stellarium-web.org/skysource/Sun?fov=120.00&date=2020-10-23T12:00:00Z&lat=42.36&lng=-71.06&elev=0
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 1.993463, planet_table[SUN].base.azimuth);
    TEST_ASSERT_DOUBLE_WITHIN(S_EPSILON, 0.145643, planet_table[SUN].base.altitude);

    // Verify Mars's position is correct
    // https://stellarium-web.org/skysource/Mars?fov=120.00&date=2020-10-23T12:00:00Z&lat=42.36&lng=-71.06&elev=0
    TEST_ASSERT_DOUBLE_WITHIN(P_EPSILON, 5.1954878, planet_table[MARS].base.azimuth);
    TEST_ASSERT_DOUBLE_WITHIN(P_EPSILON, -0.341956, planet_table[MARS].base.altitude);

    // Verify Neptune's position is correct
    // Note that the outer planets have extra kep elements (position takes mroe calculation)
    // https://stellarium-web.org/skysource/Neptune?fov=120.00&date=2020-10-23T12:00:00Z&lat=42.36&lng=-71.06&elev=0
    TEST_ASSERT_DOUBLE_WITHIN(P_EPSILON, 5.5390816, planet_table[NEPTUNE].base.azimuth);
    TEST_ASSERT_DOUBLE_WITHIN(P_EPSILON, -0.779650, planet_table[NEPTUNE].base.altitude);
}

void test_update_moon_position(void)
{
    double julian_date = 2459146.0; // 2020 October 23 12:00:00.0 UT1
    // Boston, MA in radians
    double latitude = 42.3601 * M_PI / 180;
    double longitude = -71.0589 * M_PI / 180;

    update_moon_position(&moon_object, julian_date, latitude, longitude);

    // https://stellarium-web.org/skysource/Moon?fov=120.00&date=2020-10-23T12:00:00Z&lat=42.36&lng=-71.06&elev=0
    TEST_ASSERT_DOUBLE_WITHIN(M_EPSILON, 0.7817126, moon_object.base.azimuth);
    TEST_ASSERT_DOUBLE_WITHIN(M_EPSILON, -1.118899, moon_object.base.altitude);
}

void test_map_float_to_int_range(void)
{
    int result;

    result = map_float_to_int_range(0.0, 1.0, 0, 100, 0.5);
    TEST_ASSERT_EQUAL(50, result);

    result = map_float_to_int_range(-1.0, 1.0, 0, 10, 0.0);
    TEST_ASSERT_EQUAL(5, result);

    result = map_float_to_int_range(0.0, 10.0, 0, 100, 7.5);
    TEST_ASSERT_EQUAL(75, result);
}

void test_string_to_time(void)
{
    const char *time_string = "2025-01-01T12:34:56";
    struct tm time = {0};

    bool result = string_to_time(time_string, &time);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2025 - 1900, time.tm_year);
    TEST_ASSERT_EQUAL(0, time.tm_mon);
    TEST_ASSERT_EQUAL(1, time.tm_mday);
    TEST_ASSERT_EQUAL(12, time.tm_hour);
    TEST_ASSERT_EQUAL(34, time.tm_min);
    TEST_ASSERT_EQUAL(56, time.tm_sec);
}

void test_elapsed_time_to_components(void)
{
    double elapsed_days = 365.25 + 30 + (6.0 / 24.0) + (15.0 / 1440.0) + (30.0 / 86400.0);
    int years, days, hours, minutes, seconds;

    elapsed_time_to_components(elapsed_days, &years, &days, &hours, &minutes, &seconds);

    TEST_ASSERT_EQUAL(1, years);
    TEST_ASSERT_EQUAL(30, days);
    TEST_ASSERT_EQUAL(6, hours);
    TEST_ASSERT_EQUAL(15, minutes);
    TEST_ASSERT_EQUAL(30, seconds);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_generate_star_table);
    RUN_TEST(test_generate_name_table);
    RUN_TEST(test_generate_constell_table);
    RUN_TEST(test_star_numbers_by_magnitude);
    RUN_TEST(test_update_star_positions);
    RUN_TEST(test_update_planet_positions);
    RUN_TEST(test_update_moon_position);
    RUN_TEST(test_map_float_to_int_range);
    RUN_TEST(test_string_to_time);
    RUN_TEST(test_elapsed_time_to_components);

    return UNITY_END();
}
