/* Terminal, system, and ncurses common functions and utilities.
 */

#ifndef TERM_H
#define TERM_H

#include <curses.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#endif

// Base color pairs (1-8) match standard ANSI colors.
// Extended star color pairs start at STAR_COLOR_PAIR_BASE:
// 7 spectral classes (O, B, A, F, G, K, M) x 4 brightness tiers = 28 pairs (10..37)
#define STAR_COLOR_PAIR_BASE 10
#define NUM_SPECTRAL_CLASSES 7
#define NUM_BRIGHTNESS_TIERS 4
#define CONSTELL_COLOR_PAIR 38
#define GRID_COLOR_PAIR 39

/* Initialize ncurses.h
 */
void ncurses_init(bool color);

/* Kill ncurses
 */
void ncurses_kill(void);

/* Check whether the current terminal supports DECSET 2026 synchronized output
 */
bool term_supports_sync_output(void);

/* Begin a synchronized terminal update batch (DECSET 2026)
 */
void term_sync_begin(void);

/* End a synchronized terminal update batch (DECRST 2026) and flush stdout
 */
void term_sync_end(void);

/* Check whether the current terminal is Kitty or supports the Kitty graphics protocol
 */
bool term_is_kitty(void);

/* Get the pixel dimensions of the terminal window if reported by the OS/PTY.
 * Returns true if non-zero pixel dimensions are available, false otherwise.
 */
bool term_pixel_size(int *ypixel, int *xpixel);

void wrectangle(WINDOW *win, int ya, int xa, int yb, int xb);

/* Resize window to square with largest possible area
 * aspect: cell aspect ratio (font height to width)
 */
void win_resize_square(WINDOW *win, float aspect);

/* Resize a window to full screen
 */
void win_resize_full(WINDOW *win);

/* Center window vertically and horizontally
 */
void win_position_center(WINDOW *win);

/* Get the number of rows and columns in the terminal buffer
 */
void term_size(int *y, int *x);

/* attempt to get the cell aspect ratio: cell height to width
 * i.e. "how many columns form the apparent height of a row"
 *
 * This may be influenced by many factors, including the aspect ratio of the font face, and the line height of the terminal.
 * Detection does not work in all environments, including:
 * - Docker images
 * TODO: find a predictable way to determine weather we can detect cell aspect ratio.
 */
float get_cell_aspect_ratio(void);

/* Add string via mvwaddstr, but truncate text that does not fit in the window,
 * instead of having it wrap
 */
void mvwaddstr_truncate(WINDOW *win, int y, int x, const char *str);

/* Check for window resizing on windows
 */
#ifdef _WIN32
int check_console_window_resize_event(COORD *info);
#endif

#endif // TERM_H
