#include "split_lines.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

static void free_split_lines(char **lines, int count)
{
    if (lines != NULL)
    {
        for (int i = 0; i < count; i++)
        {
            free(lines[i]);
        }
        free(lines);
    }
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_split_lines_normal_input(void)
{
    char data[] = "Line 1\nLine 2\nLine 3";
    int count = 0;

    char **lines = split_lines(data, &count);

    TEST_ASSERT_NOT_NULL(lines);
    TEST_ASSERT_EQUAL_INT(3, count);
    TEST_ASSERT_EQUAL_STRING("Line 1", lines[0]);
    TEST_ASSERT_EQUAL_STRING("Line 2", lines[1]);
    TEST_ASSERT_EQUAL_STRING("Line 3", lines[2]);

    free_split_lines(lines, count);
}

void test_split_lines_single_line(void)
{
    char data[] = "Just one line here";
    int count = 0;

    char **lines = split_lines(data, &count);

    TEST_ASSERT_NOT_NULL(lines);
    TEST_ASSERT_EQUAL_INT(1, count);
    TEST_ASSERT_EQUAL_STRING("Just one line here", lines[0]);

    free_split_lines(lines, count);
}

void test_split_lines_empty_string(void)
{
    char data[] = "";
    int count = -1; // Initialize to -1 to ensure it gets updated to 0

    char **lines = split_lines(data, &count);

    // strtok on an empty string returns NULL immediately,
    // so our function returns NULL and sets count to 0.
    TEST_ASSERT_NULL(lines);
    TEST_ASSERT_EQUAL_INT(0, count);
}

void test_split_lines_consecutive_newlines(void)
{
    // strtok treats consecutive delimiters as a single delimiter.
    // It skips empty tokens. This test verifies that expected behavior.
    char data[] = "First\n\n\nSecond\nThird\n";
    int count = 0;

    char **lines = split_lines(data, &count);

    TEST_ASSERT_NOT_NULL(lines);
    TEST_ASSERT_EQUAL_INT(3, count);
    TEST_ASSERT_EQUAL_STRING("First", lines[0]);
    TEST_ASSERT_EQUAL_STRING("Second", lines[1]);
    TEST_ASSERT_EQUAL_STRING("Third", lines[2]);

    free_split_lines(lines, count);
}

void test_split_lines_null_count_pointer(void)
{
    char data[] = "Line A\nLine B";

    char **lines = split_lines(data, NULL);

    TEST_ASSERT_NOT_NULL(lines);
    TEST_ASSERT_EQUAL_STRING("Line A", lines[0]);
    TEST_ASSERT_EQUAL_STRING("Line B", lines[1]);

    // We have to hardcode the count to 2 here to free it properly
    free_split_lines(lines, 2);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_split_lines_normal_input);
    RUN_TEST(test_split_lines_single_line);
    RUN_TEST(test_split_lines_empty_string);
    RUN_TEST(test_split_lines_consecutive_newlines);
    RUN_TEST(test_split_lines_null_count_pointer);

    return UNITY_END();
}