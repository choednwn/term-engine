#include "termengine.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

//======================================================
// Shapes
//======================================================

/**
 * Draw text
 * @param px    Precise x position
 * @param py    Precise y position
 * @param text  Text (string)
 * @param wrap  Wrap text
 * @param color Foreground color
 */
void drawText(int px, int py, char *text, int wrap, int color) {
    int text_len = strlen(text);
    for (int i = 0; i < text_len; i++) {
        if (py < CORE.height) {
            if (text[i] != ' ') {
                drawPixel(px, py, text[i], color);
            }

            px++;

            if (px == CORE.width * 2) {
                if (wrap) {
                    px = 0;
                    py++;
                } else {
                    break;
                }
            }
        } else {
            break;
        }
    }
}

/**
 * Draw line
 * (Bresenham's line algorithm)
 * @param x1    Start x position
 * @param y1    Start y poisiton
 * @param x2    End x position
 * @param ch    Character
 * @param color Foreground color
 */
void drawLine(int x1, int y1, int x2, int y2, char ch, int color) {
    int dx = abs(x2 - x1);
    int sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1);
    int sy = y1 < y2 ? 1 : -1;
    int error = dx + dy;

    while (1) {
        drawPoint(x1, y1, ch, color);
        if ((x1 == x2) && (y1 == y2)) {
            break;
        }
        int e2 = 2 * error;
        if (e2 >= dy) {
            if (x1 == x2) {
                break;
            }
            error += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            if (y1 == y2) {
                break;
            }
            error += dx;
            y1 += sy;
        }
    }
}

/**
 * Draw circle
 * (Midpoint circle algorithm)
 * @param x         X position
 * @param y         Y position
 * @param r         Radius
 * @param fill      Fill
 * @param ch        Character
 * @param color     Foreground color
 */
void drawCircle(int x, int y, int r, int fill, char ch, int color) {
    int cx = r;
    int cy = 0;
    int err = 0;

    while (cx >= cy) {
        if (fill == 0) {
            drawPoint(x + cx, y + cy, ch, color);
            drawPoint(x + cy, y + cx, ch, color);
            drawPoint(x - cy, y + cx, ch, color);
            drawPoint(x - cx, y + cy, ch, color);
            drawPoint(x - cx, y - cy, ch, color);
            drawPoint(x - cy, y - cx, ch, color);
            drawPoint(x + cy, y - cx, ch, color);
            drawPoint(x + cx, y - cy, ch, color);
        } else {
            drawLine(x - cy, y + cx, x + cy, y + cx, ch, color);
            drawLine(x - cx, y + cy, x + cx, y + cy, ch, color);
            drawLine(x - cx, y - cy, x + cx, y - cy, ch, color);
            drawLine(x - cy, y - cx, x + cy, y - cx, ch, color);
        }

        if (err <= 0) {
            cy += 1;
            err += 2 * cy + 1;
        } else {
            cx -= 1;
            err -= 2 * cx + 1;
        }
    }
}

/**
 * Draw circle (w/ Circle type)
 * (Midpoint circle algorithm)
 * @param circle    Circle
 * @param fill      Fill
 * @param ch        Character
 * @param color     Foreground color
 */
void drawCircleT(Circle circ, int fill, char ch, int color) {
    drawCircle(circ.x, circ.y, circ.radius, fill, ch, color);
}

/**
 * Draw rectangle
 * @param x         X position
 * @param y         Y position
 * @param w         Width
 * @param h         Height
 * @param fill      Fill
 * @param ch        Character
 * @param color     Foreground color
 */
void drawRectangle(int x, int y, int w, int h, int fill, char ch, int color) {
    if (fill == 0) {
        drawLine(x, y, x + w - 1, y, ch, color);
        drawLine(x, y + h - 1, x, y, ch, color);
        drawLine(x + w - 1, y, x + w - 1, y + h - 1, ch, color);
        drawLine(x + w - 1, y + h - 1, x, y + h - 1, ch, color);
    } else {
        for (int i = 0; i < h; i++) {
            drawLine(x, y + i, x + w - 1, y + i, ch, color);
        }
    }
}

/**
 * Draw rectangle (w/ Rectangle type)
 * @param Rectangle Rectangle
 * @param fill      Fill
 * @param ch        Character
 * @param color     Foreground color
 */
void drawRectangleT(Rectangle rect, int fill, char ch, int color) {
    drawRectangle(rect.x, rect.y, rect.width, rect.height, fill, ch, color);
}

//======================================================
// Collision
//======================================================

/**
 * Check collision between point and rectangle
 * @param point     Point
 * @param rect      Rectangle
 */
int checkCollisionPointRect(Vector2 point, Rectangle rect) {
    if ((point.x >= rect.x) && (point.x <= rect.x + rect.width - 1) && (point.y >= rect.y) &&
        (point.y <= rect.y + rect.height - 1)) {
        return 1;
    }
    return 0;
}

/**
 * Check collision between point and circle
 * @param point     Point
 * @param circ      Circle
 */
int checkCollisionPointCirc(Vector2 point, Circle circ) {
    if (pow(abs(circ.x - point.x), 2) + pow(abs(circ.y - point.y), 2) <= pow(circ.radius, 2)) {
        return 1;
    }
    return 0;
}

/**
 * Check collision between two rectangles
 * @param rect1     Rectangle 1
 * @param rect2     Rectangle 2
 */
int checkCollisionRects(Rectangle rect1, Rectangle rect2) {
    if ((rect1.x <= (rect2.x + rect2.width - 1) && (rect1.x + rect1.width - 1) >= rect2.x) &&
        (rect1.y <= (rect2.y + rect2.height - 1) && (rect1.y + rect1.height - 1) >= rect2.y)) {
        return 1;
    }
    return 0;
}

/**
 * Check collision between two circles
 * @param circ1     Circle 1
 * @param circ2     Circle 2
 */
int checkCollisionCircs(Circle circ1, Circle circ2) {
    // todo: find better alg
    double points_distance = sqrt(pow(abs(circ1.x - circ2.x), 2) + pow(abs(circ1.y - circ2.y), 2));
    if (points_distance <= (circ1.radius + circ2.radius + 2)) {
        return 1;
    }
    return 0;
}