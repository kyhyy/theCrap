#ifndef UI_H
#define UI_H

#include <stddef.h>

/**
 * Display an interactive selection menu over a list of suggestions.
 *
 * Features:
 *   - Arrow up/down to navigate
 *   - Number keys (1..9) jump directly to that option
 *   - Enter selects the current option
 *   - Ctrl-C / q / Esc cancels
 *
 * ANSI colors and cursor positioning are used (no ncurses dependency).
 * Terminal must support VT100/ANSI escape sequences.
 *
 * @param suggestions  array of suggestion strings
 * @param count        number of suggestions
 * @param page_size    max suggestions shown per "page"
 * @return             index of selected suggestion (0..page_size-1 within page),
 *                     or -1 if cancelled, -2 if error.
 */
int ui_select(const char **suggestions, size_t count, int page_size);

#endif /* UI_H */
