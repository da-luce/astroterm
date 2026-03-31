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
    CityData *city = get_city("Tunis");
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Tunis", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(36.81897, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(10.16579, city->longitude);
    free_city(city);

    // Test for another city that exists
    city = get_city("Boston");
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Boston", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(42.35843, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(-71.05977, city->longitude);
    free_city(city);

    // Test for cities with duplicate names. The larger should maintain the same
    // name
    city = get_city("London");
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("London", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(51.50853, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(-0.12574, city->longitude);
    free_city(city);
    // TODO: test other "London"

    // Test for yet another city that exists
    city = get_city("Lisbon");
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Lisbon", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(38.72509, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(-9.1498, city->longitude);
    free_city(city);

    // Test for a city with mixed case and spaces (regression test for PR #79)
    city = get_city("Rio de Janeiro");
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Rio de Janeiro", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(-22.90642, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(-43.18223, city->longitude);
    free_city(city);

    // Test for a city with non-ASCII characters
    city = get_city("Thủ Dầu Một");
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Thủ Dầu Một", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(10.9804, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(106.6519, city->longitude);
    free_city(city);

    // Test for a city that does not exist
    city = get_city("NonexistentCity");
    TEST_ASSERT_NULL(city);

    // Ensure small population locations are not present
    city = get_city("Nantucket");
    TEST_ASSERT_NULL(city);

    // Test for a null input
    city = get_city(NULL);
    TEST_ASSERT_NULL(city);
}

// -----------------------------------------------------------------------------
// Callbacks for iter_cities tests
// -----------------------------------------------------------------------------

// Callback 1: Simply counts the number of cities iterated over
static void count_cities_cb(const CityData *city, void *data)
{
    int *count = (int *)data;
    (*count)++;
}

// Callback 2: Looks for "Boston" and records its coordinates in a float array
// data expects a float array: [0] = found flag, [1] = latitude, [2] = longitude
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

    iter_cities(count_cities_cb, &city_count);

    // We expect the count to be greater than 0 if data/cities.csv is loaded properly
    TEST_ASSERT_GREATER_THAN(0, city_count);
}

void test_iter_cities_should_parse_data_correctly(void)
{
    // results array: [found_flag, latitude, longitude]
    float results[3] = {0.0f, 0.0f, 0.0f};

    iter_cities(find_boston_cb, results);

    // Verify the callback found the city and parsed the floats correctly
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, results[0], "Boston was not found during iteration");
    TEST_ASSERT_EQUAL_FLOAT(42.35843, results[1]);
    TEST_ASSERT_EQUAL_FLOAT(-71.05977, results[2]);
}

void test_iter_cities_null_callback_should_not_crash(void)
{
    // Passing NULL as the callback should safely return immediately
    iter_cities(NULL, NULL);

    // Dummy assertion to ensure the test passes if we get here without crashing
    TEST_ASSERT_TRUE(1);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_get_city);

    RUN_TEST(test_iter_cities_should_iterate_all_rows);
    RUN_TEST(test_iter_cities_should_parse_data_correctly);
    RUN_TEST(test_iter_cities_null_callback_should_not_crash);

    return UNITY_END();
}
