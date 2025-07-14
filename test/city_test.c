#include "city.h"
#include "unity.h"

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

    // Test for a city with mixed case and spaces (regression test for PR #79)
    city = get_city("Rio de Janeiro");
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Rio de Janeiro", city->city_name);
    TEST_ASSERT_EQUAL_FLOAT(-22.90642, city->latitude);
    TEST_ASSERT_EQUAL_FLOAT(-43.18223, city->longitude);
    free_city(city);

    // Test for a city that does not exist
    city = get_city("NonexistentCity");
    TEST_ASSERT_NULL(city);

    // Test for a null input
    city = get_city(NULL);
    TEST_ASSERT_NULL(city);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_get_city);

    return UNITY_END();
}
