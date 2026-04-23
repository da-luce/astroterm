#include "cities.h"
#include "city.h"
#include "unity.h"
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_get_city(void)
{
    // Test for a city that exists
    CityData *city = get_city("Tunis", cities, cities_len);
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Tunis", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(36.81897, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(10.16579, city->longitude);
    free_city(city);

    // Test for another city that exists
    city = get_city("Boston", cities, cities_len);
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Boston", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(42.35843, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(-71.05977, city->longitude);
    free_city(city);

    // Test for cities with duplicate names. The larger should maintain the same
    // name
    city = get_city("London", cities, cities_len);
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("London", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(51.50853, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(-0.12574, city->longitude);
    free_city(city);

    // Test for yet another city that exists
    city = get_city("Lisbon", cities, cities_len);
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Lisbon", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(38.72509, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(-9.1498, city->longitude);
    free_city(city);

    // Test for a city with mixed case and spaces (regression test for PR #79)
    city = get_city("Rio de Janeiro", cities, cities_len);
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Rio de Janeiro", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(-22.90642, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(-43.18223, city->longitude);
    free_city(city);

    // Test for a city with non-ASCII characters
    city = get_city("Thủ Dầu Một", cities, cities_len);
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Thủ Dầu Một", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(10.9804, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(106.6519, city->longitude);
    free_city(city);

    // Test for a city that does not exist
    city = get_city("NonexistentCity", cities, cities_len);
    TEST_ASSERT_NULL(city);

    // Ensure small population locations are not present
    city = get_city("Nantucket", cities, cities_len);
    TEST_ASSERT_NULL(city);

    // Test for a null input
    city = get_city(NULL, cities, cities_len);
    TEST_ASSERT_NULL(city);

    // Test for null data
    city = get_city("Boston", NULL, 0);
    TEST_ASSERT_NULL(city);
}

// Verify that when a name is duplicated in the input CSV, the row with the
// highest population is returned regardless of the order the rows appear in.
// Before PR #79 a binary-search approach could satisfy the original order but
// not its reverse, so we test both orderings here.
void test_get_city_duplicate_names(void)
{
    // CSV schema: city_name,population,country_code,timezone,latitude,longitude
    // Two "London" rows with different populations; the ~8.9M entry must win.
    const unsigned char small_first[] =
        "city_name,population,country_code,timezone,latitude,longitude\n"
        "London,1000,CA,America/Toronto,42.98339,-81.23304\n"
        "London,8900000,GB,Europe/London,51.50853,-0.12574\n";

    CityData *city = get_city("London", small_first, (unsigned int)(sizeof(small_first) - 1));
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("London", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(51.50853, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(-0.12574, city->longitude);
    free_city(city);

    const unsigned char big_first[] =
        "city_name,population,country_code,timezone,latitude,longitude\n"
        "London,8900000,GB,Europe/London,51.50853,-0.12574\n"
        "London,1000,CA,America/Toronto,42.98339,-81.23304\n";

    city = get_city("London", big_first, (unsigned int)(sizeof(big_first) - 1));
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("London", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(51.50853, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(-0.12574, city->longitude);
    free_city(city);
}

// -----------------------------------------------------------------------------
// Callbacks for iter_cities tests
// -----------------------------------------------------------------------------

// Counts the number of cities iterated over
static void count_cities_cb(const CityData *city, void *data)
{
    int *count = (int *)data;
    (*count)++;
}

// Looks for "Boston" and records its coordinates in a float array
// [0] = found flag, [1] = latitude, [2] = longitude
static void find_boston_cb(const CityData *city, void *data)
{
    float *results = (float *)data;

    if (strcmp(city->city_name, "Boston") == 0)
    {
        results[0] = 1.0f;            // Mark as found
        results[1] = city->latitude;  // Record latitude
        results[2] = city->longitude; // Record longitude
    }
}

//------------------------------------------------------------------------------
// Tests for iter_cities
//------------------------------------------------------------------------------

void test_iter_cities_should_iterate_all_rows(void)
{
    int city_count = 0;

    iter_cities(count_cities_cb, &city_count, cities, cities_len);

    // We expect the count to be greater than 0 if data/cities.csv is loaded
    // properly
    TEST_ASSERT_GREATER_THAN(0, city_count);
}

void test_iter_cities_should_parse_data_correctly(void)
{
    // results array: [found_flag, latitude, longitude]
    float results[3] = {0.0f, 0.0f, 0.0f};

    iter_cities(find_boston_cb, results, cities, cities_len);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, results[0], "Boston was not found during iteration");
    TEST_ASSERT_EQUAL_FLOAT(42.35843, results[1]);
    TEST_ASSERT_EQUAL_FLOAT(-71.05977, results[2]);
}

void test_iter_cities_null_callback_should_not_crash(void)
{
    // Passing NULL as the callback should return immediately
    iter_cities(NULL, NULL, cities, cities_len);

    // Sentinel assertion to ensure test passes if we get here without crashing
    TEST_ASSERT_TRUE(1);
}

void test_iter_cities_null_data_should_not_crash(void)
{
    int city_count = 0;
    iter_cities(count_cities_cb, &city_count, NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, city_count);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_get_city);
    RUN_TEST(test_get_city_duplicate_names);

    RUN_TEST(test_iter_cities_should_iterate_all_rows);
    RUN_TEST(test_iter_cities_should_parse_data_correctly);
    RUN_TEST(test_iter_cities_null_callback_should_not_crash);
    RUN_TEST(test_iter_cities_null_data_should_not_crash);

    return UNITY_END();
}
