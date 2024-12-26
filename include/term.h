/* Terminal, system, and ncurses common functions and utilities.
 */

#ifndef TERM_H
#define TERM_H

#include <ncurses.h>

/* Initialize ncurses.
 */
void ncurses_init(bool color);

/* Kill ncurses
 */
void ncurses_kill(void);

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

#endif // TERM_H
