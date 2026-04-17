#include "coord.h"
#include "macros.h"
#include "unity.h"

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

void test_ecliptic_to_equatorial(void)
{
    double right_ascension;
    double declination;
    double obliquity = 84381.448 / (60.0 * 60.0) * TO_RAD;

    ecliptic_to_equatorial(0.0, 0.0, &right_ascension, &declination);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 0.0, right_ascension);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 0.0, declination);

    ecliptic_to_equatorial(M_PI / 2.0, 0.0, &right_ascension, &declination);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, M_PI / 2.0, right_ascension);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, obliquity, declination);

    ecliptic_to_equatorial(M_PI, 0.0, &right_ascension, &declination);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, M_PI, right_ascension);
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 0.0, declination);
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

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_project_stereographic_top);
    RUN_TEST(test_ecliptic_to_equatorial);
    RUN_TEST(test_polar_to_win);

    return UNITY_END();
}
