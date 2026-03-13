#include "drawing.h"

#include <curses.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// The difference in logic between drawing an ASCII and unicode line differs
// enough that having two different functions is warranted

void draw_line_ASCII(WINDOW *win, int ya, int xa, int yb, int xb)
{
    // The logic here is not particularly elegant or efficient

    int dy = yb - ya;
    int dx = xb - xa;

    // "Joint"/junction character
    char slope;

    // No intelligence... just choose based on case
    if (dx > 0)
    {
        slope = dy > 0 ? '\\' : '/';
    }
    else
    {
        slope = dy > 0 ? '/' : '\\';
    }

    if (abs(dy) >= abs(dx))
    {
        int y = 0;
        double x = 0.0;

        // Step size
        int sy = (dy > 0) ? 1 : -1;
        double sx = (double)dx / abs(dy);

        while (abs(y) <= abs(dy))
        {
            int curr_y = ya + y;
            int curr_x = xa + (int)round(x);

            int next_y = ya + y + sy;
            int next_x = xa + (int)round(x + sx);

            mvwaddch(win, curr_y, curr_x, '|');

            // Draw slope if we jump a column
            if (next_x != curr_x)
            {
                mvwaddch(win, curr_y, curr_x, slope);
            }

            y += sy;
            x += sx;
        }
    }
    else
    {
        double y = 0.0;
        int x = 0;

        // Step size
        double sy = (double)dy / abs(dx);
        int sx = (dx > 0) ? 1 : -1;

        while (abs(x) <= abs(dx))
        {
            int curr_y = ya + (int)round(y);
            int curr_x = xa + x;

            int next_y = ya + (int)round(y + sy);
            int next_x = xa + x + sx;

            // Edge case where we draw a horizontal line
            char horizontal = ya == yb ? '-' : '_';

            mvwaddch(win, curr_y, curr_x, horizontal);

            // This bit requires a little more logic: drawing '-' characters
            // isn't as smooth as '_' characters. Thus, to draw a good lookin'
            // line, the slope characters must be drawn in a particular way...
            // (remember we're in screen space coordinates and the y-axis is
            // "flipped")

            // Draw slope if we jump a row
            if (next_y != curr_y)
            {
                if (dy > 0)
                {
                    // We're moving "down": add the slope to the next position
                    // Make sure we're not on the last cell first
                    if (curr_y != yb)
                    {
                        mvwaddch(win, next_y, next_x, slope);

                        // Skip drawing the next position the next iteration
                        y += sy;
                        x += sx;
                    }
                }
                else
                {
                    // We're moving "up": just add the slope to the current cell
                    mvwaddch(win, curr_y, curr_x, slope);
                }
            }

            y += sy;
            x += sx;
        }
    }

    // Could add asterisks at beginning and end of segment to "prettify",
    // but not for this application
    // mvwaddch(win, ya, xa, '*');
    // mvwaddch(win, yb, xb, '*');
}

void draw_line_smooth(WINDOW *win, int ya, int xa, int yb, int xb)
{
    // The logic here is not particularly elegant or efficient

    int dy = yb - ya;
    int dx = xb - xa;

    // "Joint"/junction characters
    char *joint_a;
    char *joint_b;

    if (abs(dy) > abs(dx))
    {
        // No intelligence... just choose based on case
        if (dx > 0)
        {
            joint_a = dy > 0 ? "╰" : "╭";
            joint_b = dy > 0 ? "╮" : "╯";
        }
        else
        {
            joint_a = dy > 0 ? "╯" : "╮";
            joint_b = dy > 0 ? "╭" : "╰";
        }

        int y = 0;
        double x = 0.0;

        // Step size
        int sy = (dy > 0) ? 1 : -1;
        double sx = (double)dx / abs(dy);

        while (abs(y) <= abs(dy))
        {
            int curr_y = ya + y;
            int curr_x = xa + (int)round(x);

            int next_y = ya + y + sy;
            int next_x = xa + (int)round(x + sx);

            mvwaddstr(win, curr_y, curr_x, "│");

            // Draw joint if we jump a column && we're not on the last cell
            if (curr_x != next_x && curr_x != xb)
            {
                mvwaddstr(win, curr_y, curr_x, joint_a);
                mvwaddstr(win, curr_y, next_x, joint_b);
            }

            y += sy;
            x += sx;
        }
    }
    else
    {
        // No intelligence... just choose based on case
        if (dy > 0)
        {
            joint_a = dx > 0 ? "╮" : "╭";
            joint_b = dx > 0 ? "╰" : "╯";
        }
        else
        {
            joint_b = dx > 0 ? "╭" : "╮";
            joint_a = dx > 0 ? "╯" : "╰";
        }

        double y = 0.0;
        int x = 0;

        // Step size
        double sy = (double)dy / abs(dx);
        int sx = (dx > 0) ? 1 : -1;

        while (abs(x) <= abs(dx))
        {
            int curr_y = ya + (int)round(y);
            int curr_x = xa + x;

            int next_y = ya + (int)round(y + sy);
            int next_x = xa + x + sx;

            mvwaddstr(win, curr_y, curr_x, "─");

            // Draw joint if we jump a row && we're not on the last cell
            if (curr_y != next_y && curr_y != yb)
            {
                mvwaddstr(win, curr_y, curr_x, joint_a);
                mvwaddstr(win, next_y, curr_x, joint_b);
            }

            y += sy;
            x += sx;
        }
    }
}

void draw_line_dotted(WINDOW *win, int ya, int xa, int yb, int xb)
{
    // The logic here is not particularly elegant or efficient

    int dy = yb - ya;
    int dx = xb - xa;

    char *fill = "•";

    if (abs(dy) >= abs(dx))
    {
        int y = 0;
        double x = 0.0;

        // Step size
        int sy = (dy > 0) ? 1 : -1;
        double sx = (double)dx / abs(dy);

        while (abs(y) <= abs(dy))
        {
            int curr_y = ya + y;
            int curr_x = xa + (int)round(x);

            mvwaddstr(win, curr_y, curr_x, fill);

            y += sy;
            x += sx;
        }
    }
    else
    {
        double y = 0.0;
        int x = 0;

        // Step size
        double sy = (double)dy / abs(dx);
        int sx = (dx > 0) ? 1 : -1;

        while (abs(x) <= abs(dx))
        {
            int curr_y = ya + (int)round(y);
            int curr_x = xa + x;

            mvwaddstr(win, curr_y, curr_x, fill);

            y += sy;
            x += sx;
        }
    }
}

#define MAX_COLS 1024
#define MAX_ROWS 1024

static unsigned char braille_layer[MAX_ROWS][MAX_COLS];

void clear_braille_lines(void)
{
    memset(braille_layer, 0, sizeof(braille_layer));
}

void draw_braille_cell(WINDOW *win, int y, int x, unsigned char mask)
{
    if (mask == 0) return;

    mask |= braille_layer[y][x];
    braille_layer[y][x] = mask;

    unsigned char utf8[4];
    utf8[0] = 0xE2;
    utf8[1] = 0xA0 | (mask >> 6);   // Top 2 bits of mask
    utf8[2] = 0x80 | (mask & 0x3F); // Bottom 6 bits of mask
    utf8[3] = '\0';

    mvwaddstr(win, y, x, (char *)utf8);
}

void draw_line_braille(WINDOW *win, int ya, int xa, int yb, int xb)
{
    // ncurses coordinates
    int curs_xa = xa;
    int curs_ya = ya;

    // braille coordinates
    if (xa < xb) {
        xa = xa * 2 + 1;
        xb = xb * 2;
    }
    else if (xa > xb) {
        xa = xa * 2;
        xb = xb * 2 + 1;
    }
    else {
        xa = xa * 2;
        xb = xb * 2;
    }

    if (ya < yb) {
        ya = ya * 4 + 2;
        yb = yb * 4 + 1;
    }
    else if (ya > yb) {
        ya = ya * 4 + 1;
        yb = yb * 4 + 2;
    }
    else {
        ya = ya * 4 + 1;
        yb = yb * 4 + 1;
    }

    int dx = abs(xb - xa);
    int dy = abs(yb - ya);

    int incx = (xa < xb) ? 1 : -1;
    int incy = (ya < yb) ? 1 : -1;

    int err = ((dx > dy) ? dx : -dy) / 2;
    int e2 = 0;

    unsigned char braille_mask = 0;

    for(;;)
    {
        int curs_x = xa / 2;
        int curs_y = ya / 4;

        if (curs_x != curs_xa || curs_y != curs_ya)
        {
            draw_braille_cell(win, curs_ya, curs_xa, braille_mask);
            braille_mask = 0;
            curs_xa = curs_x;
            curs_ya = curs_y;
        }

        int dot_x = xa % 2;
        int dot_y = ya % 4;

        static const unsigned char dot_map[4][2] = {
            {0x01, 0x08}, {0x02, 0x10}, {0x04, 0x20}, {0x40, 0x80}
        };

        braille_mask |= dot_map[dot_y][dot_x];

        if (xa == xb && ya == yb) {
            break;
        }

        e2 = err;
        if (e2 > -dx)
        {
            err -= dy;
            xa += incx;
        }
        if (e2 < dy)
        {
            err += dx;
            ya += incy;
        }
    }

    draw_braille_cell(win, curs_ya, curs_xa, braille_mask);
}

enum FillType
{
    HORIZONTAL,
    VERTICAL,
    CORNER,
};

// Reference: https://dai.fmph.uniba.sk/upload/0/01/Ellipse.pdf

void print_chars_ellipse_ASCII(WINDOW *win, int center_y, int center_x, int y, int x, int fill)
{
    switch (fill)
    {
    case CORNER:
        mvwaddch(win, center_y - y, center_x + x, '\\'); // Quad I
        mvwaddch(win, center_y - y, center_x - x, '/');  // Quad II
        mvwaddch(win, center_y + y, center_x - x, '\\'); // Quad III
        mvwaddch(win, center_y + y, center_x + x, '/');  // Quad IV
        break;

    case VERTICAL:
        mvwaddch(win, center_y - y, center_x + x, '|');
        mvwaddch(win, center_y - y, center_x - x, '|');
        mvwaddch(win, center_y + y, center_x - x, '|');
        mvwaddch(win, center_y + y, center_x + x, '|');
        break;

    case HORIZONTAL:
        mvwaddch(win, center_y - y, center_x + x, '-');
        mvwaddch(win, center_y - y, center_x - x, '-');
        mvwaddch(win, center_y + y, center_x - x, '-');
        mvwaddch(win, center_y + y, center_x + x, '-');
        break;
    }
}

void print_chars_ellipse_unicode(WINDOW *win, int center_y, int center_x, int y, int x, int fill)
{
    // TODO: def not correct
    switch (fill)
    {
    case CORNER:
        // Quad I
        mvwaddstr(win, center_y - y - 1, center_x + x, "╮");
        mvwaddstr(win, center_y - y, center_x + x, "╰");
        // Quad II
        mvwaddstr(win, center_y - y - 1, center_x - x, "╭");
        mvwaddstr(win, center_y - y, center_x - x, "╯");
        // Quad III
        mvwaddstr(win, center_y + y - 1, center_x - x, "╮");
        mvwaddstr(win, center_y + y, center_x - x, "╰");
        // Quad IV
        mvwaddstr(win, center_y + y - 1, center_x + x, "╭");
        mvwaddstr(win, center_y + y, center_x + x, "╯");
        break;

    case VERTICAL:
        mvwaddstr(win, center_y - y, center_x + x, "│");
        mvwaddstr(win, center_y - y, center_x - x, "│");
        mvwaddstr(win, center_y + y, center_x - x, "│");
        mvwaddstr(win, center_y + y, center_x + x, "│");
        break;

    case HORIZONTAL:
        mvwaddstr(win, center_y - y, center_x + x, "─");
        mvwaddstr(win, center_y - y, center_x - x, "─");
        mvwaddstr(win, center_y + y, center_x - x, "─");
        mvwaddstr(win, center_y + y, center_x + x, "─");
        break;
    }

    return;
}

int ellipse_error(int y, int x, int rad_y, int rad_x)
{
    return (rad_x * rad_x + x * x) + (rad_y * rad_y + y * y) - (rad_x * rad_x * rad_y * rad_y);
}

void draw_ellipse(WINDOW *win, int center_y, int center_x, int rad_y, int rad_x, bool no_unicode)
{
    int y = 0;
    int x = rad_x;

    int y_next = y;
    int x_next = x;

    // Point where slope = -1
    int magicY = (int)sqrt(pow((double)rad_y, 4.0) / (rad_x * rad_x + rad_y * rad_y));

    // Print first part of first quadrant: slope is > -1
    while (y_next > magicY)
    {
        // If outside ellipse, move inward
        y_next = y + 1;
        x_next = (ellipse_error(y_next, x_next, rad_y, rad_x) > 0) ? x - 1 : x;

        bool corner = y_next > y && x_next < x;
        bool vertical = y_next > y && x_next == x;

        char fill = corner ? CORNER : vertical ? VERTICAL : HORIZONTAL;

        if (no_unicode)
        {
            print_chars_ellipse_ASCII(win, center_y, center_x, y, x, fill);
        }
        else
        {
            print_chars_ellipse_unicode(win, center_y, center_x, y, x, fill);
        }

        y = y_next;
        x = x_next;
    }

    // Print second part of first quadrant: slope is < -1
    while (x_next > 0)
    {
        // If inside ellipse, move outward
        y_next = (ellipse_error(y_next, x_next, rad_y, rad_x) < 0) ? y + 1 : y;
        x_next = x - 1;

        bool corner = y_next > y && x_next < x;
        bool vertical = y_next > y && x_next == x;

        char fill = corner ? CORNER : vertical ? VERTICAL : HORIZONTAL;

        if (no_unicode)
        {
            print_chars_ellipse_ASCII(win, center_y, center_x, y, x, fill);
        }
        else
        {
            print_chars_ellipse_unicode(win, center_y, center_x, y, x, fill);
        }

        y = y_next;
        x = x_next;
    }

    return;
}
