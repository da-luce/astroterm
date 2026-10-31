#include "coord.h"
#include "macros.h"
#include "unity.h"

#include <math.h>

void setUp(void)
{
}
void tearDown(void)
{
}

// project_stereographic

void test_project_stereographic_top(void)
{
    // Center top of sphere
    double radius = 1.0;
    double center_theta = 0.0;
    double center_phi = 0.0;

    // Projection straight down should be at origin
    double radius_polar;
    double theta_polar;
    project_stereographic(radius, 0, 0, center_theta, center_phi, &radius_polar, &theta_polar);
    double expected_radius_polar = 0.0;
    double expected_theta_polar = 0.0;

    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_radius_polar, radius_polar);
    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_theta_polar, theta_polar);

    // Projection along equator should be at radius
    project_stereographic(radius, 0, M_PI / 2, center_theta, center_phi, &radius_polar, &theta_polar);
    expected_radius_polar = radius;
    expected_theta_polar = 0.0;

    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_radius_polar, radius_polar);
    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_theta_polar, theta_polar);

    // Projection along equator should be at radius
    project_stereographic(radius, M_PI, M_PI / 2, center_theta, center_phi, &radius_polar, &theta_polar);
    expected_radius_polar = radius;
    expected_theta_polar = M_PI;

    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_radius_polar, radius_polar);
    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_theta_polar, theta_polar);
}

// polar_to_win

void test_polar_to_win(void)
{
    int row, col;

    // Test case 1: Center of screen
    polar_to_win(0.0, 0.0, 100, 100, &row, &col);
    TEST_ASSERT_EQUAL_INT(50, row);
    TEST_ASSERT_EQUAL_INT(50, col);

    // Test case 2: Top of screen
    polar_to_win(1.0, M_PI / 2, 100, 100, &row, &col);
    TEST_ASSERT_EQUAL_INT(0, row);
    TEST_ASSERT_EQUAL_INT(50, col);

    // Test case 3: Bottom of screen
    polar_to_win(1.0, -M_PI / 2, 100, 100, &row, &col);
    TEST_ASSERT_EQUAL_INT(99, row);
    TEST_ASSERT_EQUAL_INT(50, col);
}

// equatorial_to_horizontal

void test_equatorial_to_horizontal_no_nan(void)
{
    double azimuth = 0.0;
    double altitude = 0.0;

    // Test case 1: Zenith transit in southern hemisphere (lat = dec = -29.94 deg, HA = 0)
    double lat1 = -29.94 * M_PI / 180.0;
    double dec1 = -29.94 * M_PI / 180.0;
    equatorial_to_horizontal(0.0, dec1, 0.0, lat1, 0.0, &azimuth, &altitude);
    TEST_ASSERT_FALSE(isnan(altitude));
    TEST_ASSERT_FALSE(isnan(azimuth));

    // Test case 2: Zenith transit in northern hemisphere (lat = dec = 86.4048 deg, HA = 0)
    double lat2 = 86.4048 * M_PI / 180.0;
    double dec2 = 86.4048 * M_PI / 180.0;
    equatorial_to_horizontal(0.0, dec2, 0.0, lat2, 0.0, &azimuth, &altitude);
    TEST_ASSERT_FALSE(isnan(altitude));
    TEST_ASSERT_FALSE(isnan(azimuth));

    // Test case 3: North Celestial Pole (dec = +M_PI / 2.0)
    double lat3 = 45.0 * M_PI / 180.0;
    double dec3 = M_PI / 2.0;
    equatorial_to_horizontal(0.0, dec3, 0.0, lat3, 0.0, &azimuth, &altitude);
    TEST_ASSERT_FALSE(isnan(altitude));
    TEST_ASSERT_FALSE(isnan(azimuth));

    // Test case 4: South Celestial Pole (dec = -M_PI / 2.0)
    double lat4 = -45.0 * M_PI / 180.0;
    double dec4 = -M_PI / 2.0;
    equatorial_to_horizontal(0.0, dec4, 0.0, lat4, 0.0, &azimuth, &altitude);
    TEST_ASSERT_FALSE(isnan(altitude));
    TEST_ASSERT_FALSE(isnan(azimuth));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_project_stereographic_top);
    RUN_TEST(test_polar_to_win);
    RUN_TEST(test_equatorial_to_horizontal_no_nan);

    return UNITY_END();
}
