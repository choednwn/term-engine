#include "termengine.h"

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define DEFAULT_CORE_BORDER 0
#define DEFAULT_CORE_TARGET_FPS 12
#define DEFAULT_CORE_FRAME_COUNT 0
#define DEFAULT_CORE_COLOR 0
#define DEFAULT_CORE_PREV_CLOCK_TIME clock()
#define DEFAULT_CORE_INPUT_ENABLED 0
#define DEFAULT_CORE_DEBUG_ENABLED 0
#define DEFAULT_CORE_DEBUG_HEIGHT 3

//======================================================
// Variables
//======================================================
CoreData CORE;  // system variable. accessible to user but not recommended tinkering with

//======================================================
// System Functions (Not accessable to user)
//======================================================

// usleep() on a separate thread
void *utimesleep(void *args) {
    int *time = (int *)args;
    usleep(*time);
    return NULL;
}

// Check if viewport is big enough to render
void checkViewport() {
    int full_height = CORE.height;  // rendered full (viewport + debug) height
    CORE.border_padding = 0;        // border padding
    CORE.border_padding_amt = 0;    // border padding amount

    if (CORE.border) {
        CORE.border_padding = 1;
        CORE.border_padding_amt = 2;

        if (CORE.debug_enabled) {
            CORE.border_padding_amt += 2;
        }
    }

    getmaxyx(stdscr, CORE.win_height, CORE.win_width);

    // Full viewport height depending if debug menu is enabled
    if (CORE.debug_enabled) {
        full_height += CORE.debug_height;
    }

    // Exits if viewport is smaller than needed area
    if ((CORE.win_height <= full_height + (CORE.border_padding * CORE.border_padding_amt)) ||
        (CORE.win_width <= CORE.width * 2 + (CORE.border_padding * CORE.border_padding_amt))) {
        deinitEngine();
        printf("Exited: Window is smaller than viewport size!");
        exit(0);
    }
}

//======================================================
// Initialization
//======================================================

// Initialize Engine
void initEngine() {
    // ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    // defaults
    CORE.border = DEFAULT_CORE_BORDER;
    CORE.target_fps = DEFAULT_CORE_TARGET_FPS;
    CORE.frame_count = DEFAULT_CORE_FRAME_COUNT;
    CORE.color_enabled = DEFAULT_CORE_COLOR;
    CORE.debug_enabled = DEFAULT_CORE_DEBUG_ENABLED;
    CORE.debug_height = DEFAULT_CORE_DEBUG_HEIGHT;
}

// Deinitialize Engine
void deinitEngine() {
    curs_set(1);
    endwin();
}

//======================================================
// Viewport
//======================================================

/**
 * Set viewport parameters
 * @param width   Width of viewport
 * @param height  Height of viewport
 * @param fc      Fill character
 */
void setViewport(int width, int height) {
    CORE.width = width;
    CORE.height = height;
    CORE.viewport = newwin(CORE.height, CORE.width * 2, 0, 0);

    CORE.viewport_data = (Viewport *)malloc((CORE.width * 2) * CORE.height * sizeof(Viewport));
    for (int i = 0; i < (CORE.width * 2) * CORE.height; i++) {
        CORE.viewport_data[i].ch = 0;
        CORE.viewport_data[i].color = 0;
    }

    checkViewport();
}

// Enable color rendering
void setColor() {
    if (has_colors()) {
        CORE.color_enabled = 1;

        start_color();
        init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
        init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
    } else {
        CORE.color_enabled = 0;
    }
}

// Enable border
void setBorder() {
    CORE.border = 1;
    wresize(CORE.viewport, CORE.height + 2, (CORE.width * 2) + 2);
    checkViewport();  // check again to make sure border is drawable
}

// Render viewport to terminal
void renderViewport() {
    CORE.prev_clock = clock();

    // Check if window is resized
    int nwin_width, nwin_height;
    getmaxyx(stdscr, nwin_width, nwin_height);
    if (CORE.win_width != nwin_width || CORE.win_height != nwin_height) {
        checkViewport();
    }

    // Render border if border is enabled
    if (CORE.border) {
        box(CORE.viewport, 0, 0);
        if (CORE.debug_enabled) {
            box(CORE.debug_menu, 0, 0);
        }
    }

    // Render viewport
    for (int i = 0; i < (CORE.width * 2) * CORE.height; i++) {
        if (CORE.viewport_data[i].ch != 0) {
            if (CORE.color_enabled) {
                wattron(CORE.viewport, COLOR_PAIR(CORE.viewport_data[i].color));
            }

            mvwaddch(CORE.viewport, i / (CORE.width * 2) + CORE.border_padding,
                     i % (CORE.width * 2) + CORE.border_padding, CORE.viewport_data[i].ch);

            if (CORE.color_enabled) {
                wattroff(CORE.viewport, COLOR_PAIR(CORE.viewport_data[i].color));
            }
        }
    }
    wrefresh(CORE.viewport);

    // Render debug
    if (CORE.debug_enabled) {
        for (int i = 0; i < CORE.debug_height; i++) {
            if (CORE.debug_data[i].title != 0) {
                mvwprintw(CORE.debug_menu, i + CORE.border_padding, 0 + CORE.border_padding,
                          "%s: %s", CORE.debug_data[i].title, CORE.debug_data[i].value);
            }
        }
        wrefresh(CORE.debug_menu);
    }

    // Set frame count for next frame
    CORE.frame_count++;
    if (CORE.frame_count == 4000000001) {
        CORE.frame_count = 0;
    }

    // Delay clock
    CORE.curr_clock = clock();
    if ((CLOCKS_PER_SEC / CORE.target_fps) - (CORE.curr_clock - CORE.prev_clock) > 0) {
        CORE.sleep_time = (CLOCKS_PER_SEC / CORE.target_fps) - (CORE.curr_clock - CORE.prev_clock);
        pthread_create(&CORE.sleep_id, NULL, utimesleep, (void *)&CORE.sleep_time);
        pthread_join(CORE.sleep_id, NULL);
    }
}

// Clear viewport
void clearViewport() {
    werase(CORE.viewport);
    if (CORE.debug_enabled) {
        werase(CORE.debug_menu);
    }
    for (int i = 0; i < (CORE.width * 2) * CORE.height; i++) {
        CORE.viewport_data[i].ch = 0;
        CORE.viewport_data[i].color = 0;
    }
}

//======================================================
// Time
//======================================================

/**
 * Set target refresh rate
 * @param fps FPS
 */
void setTargetFPS(int fps) {
    CORE.target_fps = abs(fps);
}

unsigned long getFrameCount() {
    return CORE.frame_count;
}

//======================================================
//                         Draw
//======================================================

/**
 * Draw pixel "#"
 * @param px        Precise x position
 * @param py        Presice y poisiton
 * @param ch        Character
 * @param color     Foreground color
 */
void drawPixel(int px, int py, char ch, int color) {
    if ((px >= 0) && (px < (CORE.width * 2)) && (py >= 0) && (py < CORE.height)) {
        CORE.viewport_data[py * (CORE.width * 2) + px].ch = ch;
        CORE.viewport_data[py * (CORE.width * 2) + px].color = color;
    }
}

/**
 * Draw point "##"
 * @param x     X position
 * @param y     Y poisiton
 * @param ch    Character
 * @param color Foreground color
 */
void drawPoint(int x, int y, char ch, int color) {
    if ((x >= 0) && (x < CORE.width) && (y >= 0) && (y < CORE.height)) {
        CORE.viewport_data[y * (CORE.width * 2) + (x * 2)].ch = ch;
        CORE.viewport_data[y * (CORE.width * 2) + (x * 2)].color = color;
        CORE.viewport_data[y * (CORE.width * 2) + (x * 2 + 1)].ch = ch;
        CORE.viewport_data[y * (CORE.width * 2) + (x * 2 + 1)].color = color;
    }
}

//======================================================
// Debugging
//======================================================

// Show debug menu
void setDebug() {
    CORE.debug_enabled = 1;

    int border_padding = 0;
    if (CORE.border) {
        border_padding = 2;
    }

    CORE.debug_menu = newwin(CORE.debug_height + border_padding, (CORE.width * 2) + border_padding,
                             CORE.height + border_padding, 0);
    CORE.debug_data = (Debug *)malloc(CORE.debug_height * sizeof(Debug));
}

// Update/Add debug attribute
void addDebugAttrib(int line_num, char *title, char *value) {
    // Reset max height so empty space can be removed
    int max_height = DEFAULT_CORE_DEBUG_HEIGHT;
    for (int i = 0; i < CORE.debug_height; i++) {
        if (CORE.debug_data[i].line_num > max_height) {
            max_height = CORE.debug_data[i].line_num;
        }
    }
    if (line_num >= CORE.debug_height) {
        CORE.debug_height = line_num;
    }
    // Resize debug_data for new data
    CORE.debug_data = (Debug *)realloc(CORE.debug_data, CORE.debug_height * sizeof(Debug));

    CORE.debug_data[line_num].line_num = line_num;
    CORE.debug_data[line_num].title = title;
    CORE.debug_data[line_num].value = value;
}