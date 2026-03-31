#include "bit.h"
#include "drawing.h"
#include "unity.h"

#include <curses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// These tests aren't meant to be gospel:
// - We only test layouts with an unanimous answer, e.g. there is only one "right" way to draw a vertical ASCII line (except
// maybe the unicode diagonals)
// - We just want to make sure the line drawing functions didn't get horribly messed up

#define MAX_WINDOW_HEIGHT 100
#define MAX_WINDOW_WIDTH 100

// -----------------------------------------------------------------------------
// ASCII Tests
// -----------------------------------------------------------------------------

// Function to read an ncurses window into a 2D array
void read_window_to_array(WINDOW *win, char array[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH], int height, int width)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            array[y][x] = mvwinch(win, y, x) & A_CHARTEXT; // Extract character
        }
        array[y][width] = '\0'; // Null-terminate the line
    }
}

// Function to compare two 2D arrays
int compare_arrays(const char array1[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH],
                   const char array2[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH], int height, int width)
{
    for (int y = 0; y < height; y++)
    {
        if (strncmp(array1[y], array2[y], width) != 0)
        {
            fprintf(stderr, "Mismatch at row %d: \"%s\" != \"%s\"\n", y, array1[y], array2[y]);
            return 0;
        }
    }
    return 1;
}

// -----------------------------------------------------------------------------
// ASCII Diagonal
// -----------------------------------------------------------------------------

// clang-format off
const char diagonal_ascii_10x10[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH] = {
    "\\         ",
    " \\        ",
    "  \\       ",
    "   \\      ",
    "    \\     ",
    "     \\    ",
    "      \\   ",
    "       \\  ",
    "        \\ ",
    "         \\",
};
// clang-format on

void test_diagonal_ascii_10x10(void)
{
    WINDOW *win = newwin(10, 10, 0, 0);
    char actual[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH];

    // Draw the line
    draw_line_ASCII(win, 0, 0, 9, 9);

    // Read window content into an array
    read_window_to_array(win, actual, 10, 10);

    const char(*const_actual)[MAX_WINDOW_WIDTH] = (const char(*)[MAX_WINDOW_WIDTH])actual;

    // Compare actual and expected output
    TEST_ASSERT_TRUE(compare_arrays(const_actual, diagonal_ascii_10x10, 10, 10));

    delwin(win);
}

// -----------------------------------------------------------------------------
// ASCII Diagonal Opposite
// -----------------------------------------------------------------------------

// clang-format off
const char diagonal_ascii_opposite_10x10[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH] = {
    "         /",
    "        / ",
    "       /  ",
    "      /   ",
    "     /    ",
    "    /     ",
    "   /      ",
    "  /       ",
    " /        ",
    "/         ",
};
// clang-format on

void test_diagonal_ascii_opposite_10x10(void)
{
    WINDOW *win = newwin(10, 10, 0, 0);
    char actual[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH];

    // Draw the line (opposite diagonal)
    draw_line_ASCII(win, 9, 0, 0, 9);

    // Read window content into an ASCII array
    read_window_to_array(win, actual, 10, 10);

    const char(*const_actual)[MAX_WINDOW_WIDTH] = (const char(*)[MAX_WINDOW_WIDTH])actual;

    // Compare actual and expected output
    TEST_ASSERT_TRUE(compare_arrays(const_actual, diagonal_ascii_opposite_10x10, 10, 10));

    delwin(win);
}

// -----------------------------------------------------------------------------
// ASCII Vertical
// -----------------------------------------------------------------------------

// clang-format off
const char vertical_ascii_11x11[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH] = {
    "     |     ",
    "     |     ",
    "     |     ",
    "     |     ",
    "     |     ",
    "     |     ",
    "     |     ",
    "     |     ",
    "     |     ",
    "     |     ",
    "     |     ",
};
// clang-format on

void test_vertical_ascii_11x11(void)
{
    WINDOW *win = newwin(11, 11, 0, 0);
    char actual[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH];

    // Draw the line
    draw_line_ASCII(win, 0, 5, 10, 5);

    // Read window content into an ASCII array
    read_window_to_array(win, actual, 11, 11);

    const char(*const_actual)[MAX_WINDOW_WIDTH] = (const char(*)[MAX_WINDOW_WIDTH])actual;

    // Compare actual and expected output
    TEST_ASSERT_TRUE(compare_arrays(const_actual, vertical_ascii_11x11, 11, 11));

    delwin(win);
}

// -----------------------------------------------------------------------------
// ASCII Horizontal
// -----------------------------------------------------------------------------

// clang-format off
const char horizontal_ascii_11x11[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH] = {
    "           ",
    "           ",
    "           ",
    "           ",
    "           ",
    "-----------",
    "           ",
    "           ",
    "           ",
    "           ",
    "           ",
};
// clang-format on

void test_horizontal_ascii_11x11(void)
{
    WINDOW *win = newwin(11, 11, 0, 0);
    char actual[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH];

    // Draw the line
    draw_line_ASCII(win, 5, 0, 5, 10);

    // Read window content into an ASCII array
    read_window_to_array(win, actual, 11, 11);

    const char(*const_actual)[MAX_WINDOW_WIDTH] = (const char(*)[MAX_WINDOW_WIDTH])actual;

    // Compare actual and expected output
    TEST_ASSERT_TRUE(compare_arrays(const_actual, horizontal_ascii_11x11, 11, 11));

    delwin(win);
}

// -----------------------------------------------------------------------------
// Unicode Tests
// -----------------------------------------------------------------------------

void read_window_to_wide_array(WINDOW *win, wchar_t array[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH], int height, int width)
{
    chtype wc;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            array[y][x] = mvwinch(win, y, x);
        }
        array[y][width] = L'\0'; // Null-terminate the line
    }
}

int compare_wide_arrays(const wchar_t array1[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH],
                        const wchar_t array2[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH], int height, int width)
{
    for (int y = 0; y < height; y++)
    {
        if (wcsncmp(array1[y], array2[y], width) != 0)
        {
            fwprintf(stderr, L"Mismatch at row %d: \"%ls\" != \"%ls\"\n", y, array1[y], array2[y]);
            fflush(stderr);
            return 0;
        }
    }
    return 1;
}

// -----------------------------------------------------------------------------
// Unicode Diagonal
// -----------------------------------------------------------------------------

// clang-format off
const wchar_t diagonal_smooth_10x10[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH] = {
    L"╮         ",
    L"╰╮        ",
    L" ╰╮       ",
    L"  ╰╮      ",
    L"   ╰╮     ",
    L"    ╰╮    ",
    L"     ╰╮   ",
    L"      ╰╮  ",
    L"       ╰╮ ",
    L"        ╰─",
};
// clang-format on

void test_diagonal_smooth_10x10(void)
{
    WINDOW *win = newwin(10, 10, 0, 0);
    wchar_t actual[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH];

    // Draw the line
    draw_line_smooth(win, 0, 0, 9, 9);

    // Read window content into a wide-character array
    read_window_to_wide_array(win, actual, 10, 10);

    const wchar_t(*const_actual)[MAX_WINDOW_WIDTH] = (const wchar_t(*)[MAX_WINDOW_WIDTH])actual;

    // Compare actual and expected output
    TEST_ASSERT_TRUE(compare_wide_arrays(const_actual, diagonal_smooth_10x10, 10, 10));

    delwin(win);
}

// -----------------------------------------------------------------------------
// Unicode Opposite Diagonal
// -----------------------------------------------------------------------------

// clang-format off
const wchar_t diagonal_smooth_opposite_10x10[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH] = {
    L"        ╭─",
    L"       ╭╯ ",
    L"      ╭╯  ",
    L"     ╭╯   ",
    L"    ╭╯    ",
    L"   ╭╯     ",
    L"  ╭╯      ",
    L" ╭╯       ",
    L"╭╯        ",
    L"╯         ",
};
// clang-format on

void test_diagonal_smooth_opposite_10x10(void)
{
    WINDOW *win = newwin(10, 10, 0, 0);
    wchar_t actual[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH];

    // Draw the line (opposite diagonal)
    draw_line_smooth(win, 9, 0, 0, 9);

    // Read window content into a wide-character array
    read_window_to_wide_array(win, actual, 10, 10);

    const wchar_t(*const_actual)[MAX_WINDOW_WIDTH] = (const wchar_t(*)[MAX_WINDOW_WIDTH])actual;

    // Compare actual and expected output
    TEST_ASSERT_TRUE(compare_wide_arrays(const_actual, diagonal_smooth_opposite_10x10, 10, 10));

    delwin(win);
}

// -----------------------------------------------------------------------------
// Unicode Vertical
// -----------------------------------------------------------------------------

// clang-format off
const wchar_t vertical_smooth_11x11[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH] = {
    L"     │     ",
    L"     │     ",
    L"     │     ",
    L"     │     ",
    L"     │     ",
    L"     │     ",
    L"     │     ",
    L"     │     ",
    L"     │     ",
    L"     │     ",
    L"     │     ",
};
// clang-format on

void test_vertical_smooth_11x11(void)
{
    WINDOW *win = newwin(11, 11, 0, 0);
    wchar_t actual[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH];

    // Draw the line
    draw_line_smooth(win, 0, 5, 10, 5);

    // Read window content into a wide-character array
    read_window_to_wide_array(win, actual, 11, 11);

    const wchar_t(*const_actual)[MAX_WINDOW_WIDTH] = (const wchar_t(*)[MAX_WINDOW_WIDTH])actual;

    // Compare actual and expected output
    TEST_ASSERT_TRUE(compare_wide_arrays(const_actual, vertical_smooth_11x11, 11, 11));

    delwin(win);
}

// -----------------------------------------------------------------------------
// Unicode Horizontal
// -----------------------------------------------------------------------------

// clang-format off
const wchar_t horizontal_smooth_11x11[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH] = {
    L"           ",
    L"           ",
    L"           ",
    L"           ",
    L"           ",
    L"───────────",
    L"           ",
    L"           ",
    L"           ",
    L"           ",
    L"           ",
};
// clang-format on

void test_horizontal_smooth_11x11(void)
{
    WINDOW *win = newwin(11, 11, 0, 0);
    wchar_t actual[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH];

    // Draw the line
    draw_line_smooth(win, 5, 0, 5, 10);

    // Read window content into a wide-character array
    read_window_to_wide_array(win, actual, 11, 11);

    const wchar_t(*const_actual)[MAX_WINDOW_WIDTH] = (const wchar_t(*)[MAX_WINDOW_WIDTH])actual;

    // Compare actual and expected output
    TEST_ASSERT_TRUE(compare_wide_arrays(const_actual, horizontal_smooth_11x11, 11, 11));

    delwin(win);
}

// -----------------------------------------------------------------------------
// Braille Vertical
// -----------------------------------------------------------------------------

// clang-format off
const wchar_t vertical_braille_11x11[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH] = {
    L"     ⡄     ",
    L"     ⡇     ",
    L"     ⡇     ",
    L"     ⡇     ",
    L"     ⡇     ",
    L"     ⡇     ",
    L"     ⡇     ",
    L"     ⡇     ",
    L"     ⡇     ",
    L"     ⡇     ",
    L"     ⠃     ",
};
// clang-format on

void test_vertical_braille_11x11(void)
{
    // Important: Clear the global braille layer state before drawing
    clear_braille_lines();

    WINDOW *win = newwin(11, 11, 0, 0);
    wchar_t actual[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH];

    // Draw the line
    draw_line_braille(win, 0, 5, 10, 5);

    // Read window content into a wide-character array
    read_window_to_wide_array(win, actual, 11, 11);

    const wchar_t(*const_actual)[MAX_WINDOW_WIDTH] = (const wchar_t(*)[MAX_WINDOW_WIDTH])actual;

    // Compare actual and expected output
    TEST_ASSERT_TRUE(compare_wide_arrays(const_actual, vertical_braille_11x11, 11, 11));

    delwin(win);
}

// -----------------------------------------------------------------------------
// Braille Horizontal
// -----------------------------------------------------------------------------

// clang-format off
const wchar_t horizontal_braille_11x11[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH] = {
    L"           ",
    L"           ",
    L"           ",
    L"           ",
    L"           ",
    L"⠐⠒⠒⠒⠒⠒⠒⠒⠒⠒⠂",
    L"           ",
    L"           ",
    L"           ",
    L"           ",
    L"           ",
};
// clang-format on

void test_horizontal_braille_11x11(void)
{
    // Important: Clear the global braille layer state before drawing
    clear_braille_lines();

    WINDOW *win = newwin(11, 11, 0, 0);
    wchar_t actual[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH];

    // Draw the line
    draw_line_braille(win, 5, 0, 5, 10);

    // Read window content into a wide-character array
    read_window_to_wide_array(win, actual, 11, 11);

    const wchar_t(*const_actual)[MAX_WINDOW_WIDTH] = (const wchar_t(*)[MAX_WINDOW_WIDTH])actual;

    // Compare actual and expected output
    TEST_ASSERT_TRUE(compare_wide_arrays(const_actual, horizontal_braille_11x11, 11, 11));

    delwin(win);
}

// -----------------------------------------------------------------------------
// Braille Diagonal
// -----------------------------------------------------------------------------

// clang-format off
const wchar_t diagonal_braille_6x11[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH] = {
    L"⠠⡀         ",
    L" ⠈⠢⡀       ",
    L"   ⠈⠢⡀     ",
    L"     ⠈⠢⡀   ",
    L"       ⠈⠢⡀ ",
    L"         ⠈⠂",
};
// clang-format on

void test_diagonal_braille_6x11(void)
{
    // Important: Clear the global braille layer state before drawing
    clear_braille_lines();

    WINDOW *win = newwin(6, 11, 0, 0);
    wchar_t actual[MAX_WINDOW_HEIGHT][MAX_WINDOW_WIDTH];

    // Draw the line (0,0 to 5,10)
    draw_line_braille(win, 0, 0, 5, 10);

    // Read window content into a wide-character array
    read_window_to_wide_array(win, actual, 6, 11);

    const wchar_t(*const_actual)[MAX_WINDOW_WIDTH] = (const wchar_t(*)[MAX_WINDOW_WIDTH])actual;

    // Compare actual and expected output
    TEST_ASSERT_TRUE(compare_wide_arrays(const_actual, diagonal_braille_6x11, 6, 11));

    delwin(win);
}

// -----------------------------------------------------------------------------
// Unity
// -----------------------------------------------------------------------------

FILE *output_file;
SCREEN *fake_screen;

void setUp(void)
{
    // Use a "fake" screen to bypass the need for a real terminal in CI/CD environments.
    // This redirects ncurses output to a file or buffer for testing in non-interactive setups.

    setlocale(LC_ALL, "");
    output_file = fopen("fake_terminal.txt", "w");
    if (!output_file)
    {
        perror("Failed to open file for fake terminal");
        exit(EXIT_FAILURE);
    }

    fake_screen = newterm("xterm", output_file, stdin);
    if (!fake_screen)
    {
        fprintf(stderr, "Failed to create fake terminal\n");
        fclose(output_file);
        exit(EXIT_FAILURE);
    }

    set_term(fake_screen);
    noecho();
    cbreak();
}

void tearDown(void)
{
    endwin();
    delscreen(fake_screen);
    fclose(output_file);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_diagonal_ascii_10x10);
    RUN_TEST(test_diagonal_ascii_opposite_10x10);
    RUN_TEST(test_vertical_ascii_11x11);
    RUN_TEST(test_horizontal_ascii_11x11);

    RUN_TEST(test_diagonal_smooth_10x10);
    RUN_TEST(test_diagonal_smooth_opposite_10x10);
    RUN_TEST(test_vertical_smooth_11x11);
    RUN_TEST(test_horizontal_smooth_11x11);

    RUN_TEST(test_vertical_braille_11x11);
    RUN_TEST(test_horizontal_braille_11x11);
    RUN_TEST(test_diagonal_braille_6x11);

    return UNITY_END();
}
