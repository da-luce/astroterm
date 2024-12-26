/* ASCII and Unicode ncurses rendering functions. These functions aim to provide
 * a balance of performance, readability, and style of the resulting render,
 * with more emphasis placed on the latter two objectives. Here, we forgo many
 * of the micro-optimizations (e.g. precomputing frequently used values) of the
 * inspiring/underlying algorithms, as the runtime of these functions will
 * largely be dominated by slow nature of drawing characters to a terminal, as
 * opposed to CPU arithmetic.
 *
 * Functions receive integer coordinates representing rows and columns on the
 * terminal screen: any calculation needed to adjust for the aspect ratio of
 * cells should be done before hand. Within each function, cell coordinates are
 * translated to conform to a normal cartesian grid. Points on this grid are
 * represented as `y` and `x` and are only translated to their respective `row`
 * and `column` on the terminal when they are pushed to the screen buffer.
 *
 * IMPORTANT:   using Unicode-designated functions requires UTF-8 encoding
 *              for proper results
 *
 * TODO:        function naming argument: verb - noun - adjectives
 *              (postpositive adjectives)
 */

#ifndef DRAWING_H
#define DRAWING_H

#include <ncurses.h>
#include <stdbool.h>

/* Draw an ASCII line segment from (xa, ya) and (xb, yb) where y and x
 * are synonymous with row and column, respectively.
 */
void draw_line_ASCII(WINDOW *win, int ya, int xa, int yb, int xb);

/* Draw a smooth unicode line segment from (xa, ya) and (xb, yb) where y and x
 * are synonymous with row and column, respectively
 */
void draw_line_smooth(WINDOW *win, int ya, int xa, int yb, int xb);

/* Draw an dotted line segment from (xa, ya) and (xb, yb) where y and x
 * are synonymous with row and column, respectively.
 */
void draw_line_dotted(WINDOW *win, int ya, int xa, int yb, int xb);

/* Draw an ellipse. By taking advantage of knowing the cell aspect ratio,
 * this function can generate an "apparent" circle.
 */
void draw_ellipse(WINDOW *win, int centerRow, int centerCol, int radiusY, int radiusX, bool no_unicode);

#endif // DRAWING_H
