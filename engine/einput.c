#include "termengine.h"

// Get pressed key (ncurses)
int getKey() {
    return getch();
}

// Flush input buffer (ncurses)
void flushInputBuf() {
    flushinp();
}