#define _POSIX_C_SOURCE 200809L
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <sys/ioctl.h>

/* ---------- ANSI escape sequences ---------- */

#define ESC "\033"

#define ANSI_CLEAR ESC "[2J" ESC "[H"
#define ANSI_RESET ESC "[0m"
#define ANSI_BOLD  ESC "[1m"
#define ANSI_GREEN ESC "[32m"
#define ANSI_CYAN  ESC "[36m"
#define ANSI_DIM   ESC "[2m"
#define ANSI       ESC "["

/* ---------- Terminal save/restore ---------- */

static struct termios orig_termios;
static int termios_saved = 0;

static int save_terminal(void)
{
    if (tcgetattr(STDIN_FILENO, &orig_termios) != 0)
        return -1;
    termios_saved = 1;
    return 0;
}

static void enter_raw(void)
{
    struct termios raw;
    if (!termios_saved) return;

    raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;     /* non-blocking */
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

static void exit_raw(void)
{
    if (!termios_saved) return;
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    termios_saved = 0;
}

/* ---------- Helper: clamp val to [lo, hi] ---------- */

static int clamp(int val, int lo, int hi)
{
    return val < lo ? lo : (val > hi ? hi : val);
}

/* ---------- Menu display ---------- */

static void display_menu(const char **suggestions, size_t count, int page_size,
                         int page, int selected)
{
    /* Total items on this page */
    int start = page * page_size;
    int end = clamp((page + 1) * page_size, 0, (int)count);
    int visible = end - start;

    /* How many pages */
    int total_pages = (int)count > 0
        ? (int)((count + page_size - 1) / page_size)
        : 1;

    /* Screen dimensions (unused — could be used for wrapping) */
    (void)ioctl(STDERR_FILENO, TIOCGWINSZ, &(struct winsize){0});

    /* Header. All menu chrome goes to stderr (the controlling terminal) so
     * that stdout carries only the chosen fix. */
    fprintf(stderr, ANSI_CLEAR
            ANSI_CYAN "Suggestions (%zu total):" ANSI_RESET "\n", count);

    /* Items */
    for (int i = 0; i < visible; i++) {
        int idx = start + i;
        int is_selected = (i == selected);

        /* Item number (right-aligned, width 2) */
        if (is_selected)
            fprintf(stderr, ANSI_BOLD ANSI_GREEN);
        fprintf(stderr, "%2d  ", idx + 1);
        if (is_selected)
            fprintf(stderr, ANSI_RESET);

        /* Item text */
        if (is_selected)
            fprintf(stderr, ANSI_CYAN ANSI_BOLD);
        fprintf(stderr, "%s" ANSI_RESET "\n", suggestions[idx]);

        /* Cursor position for redisplay */
        fprintf(stderr, ANSI "%d;%dH", 3 + i * 2, 1);
    }

    /* Separator + instructions */
    fprintf(stderr, "\n");
    fprintf(stderr, ANSI_DIM
            "up/down navigate   "
            "1-%d select   "
            "PageUp/PageDown pages   "
            "q/Esc cancel" ANSI_RESET "\n",
            page_size);
    if (total_pages > 1)
        fprintf(stderr, ANSI_DIM "Page %d/%d" ANSI_RESET "\n", page + 1, total_pages);

    /* Move cursor below menu */
    fprintf(stderr, ANSI "%d;1H", 3 + visible * 2);

    /* Flush */
    fflush(stderr);
}

/* ---------- Arrow key detection ---------- */

/* Read next byte with a tiny poll timeout. Returns the byte, or -1 on timeout. */
static int peek_char(void)
{
    char buf;
    struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };

    if (poll(&pfd, 1, 0) <= 0)
        return -1;

    ssize_t n = read(STDIN_FILENO, &buf, 1);
    return n > 0 ? (int)(unsigned char)buf : -1;
}

static int read_arrow_key(void)
{
    /* Expect: ESC [ X */
    int b1 = peek_char();
    if (b1 != '[') return 0; /* not an arrow key */

    int b2 = peek_char();
    switch (b2) {
    case 'A': return -1;   /* Up */
    case 'B': return  1;   /* Down */
    case 'H': return -2;   /* Home */
    case 'F': return  2;   /* End */
    default:  return  0;   /* not a directional key */
    }
}

/* ---------- Public API ---------- */

int ui_select(const char **suggestions, size_t count, int page_size)
{
    if (count == 0) return -1; /* nothing to select */

    /* Clamp page_size to a reasonable range */
    if (page_size <= 0 || page_size > 20)
        page_size = 10;

    /* Save and enter raw terminal mode */
    if (save_terminal() != 0)
        return -2;
    enter_raw();

    /* Clear screen and show initial menu */
    fprintf(stderr, ANSI_CLEAR);
    fflush(stderr);

    int page       = 0;
    int selected   = 0;
    int result     = -1; /* -1 = cancelled */

    display_menu(suggestions, count, page_size, page, selected);

    /* Main input loop */
    for (;;) {
        char buf[6] = {0};
        int nbytes = 0;

        struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };
        int poll_ret = poll(&pfd, 1, 50); /* 50ms timeout for responsiveness */

        /* Nothing ready (timeout) or poll error — loop again without
         * touching buf (it would otherwise be read uninitialized). */
        if (poll_ret <= 0)
            continue;

        nbytes = read(STDIN_FILENO, buf, sizeof(buf));
        if (nbytes <= 0) continue;

        /* Process the first byte */
        unsigned char c = (unsigned char)buf[0];

        /* Escape sequence — read more bytes */
        if (c == 0x1B && nbytes >= 2) {
            int arrow = read_arrow_key();
            if (arrow != 0) {
                if (arrow == -1) { /* Up */
                    if (selected > 0)
                        selected--;
                    else if (page > 0) {
                        page--;
                        selected = page_size - 1;
                        if ((int)count - page * page_size < selected)
                            selected = (int)count - page * page_size - 1;
                    }
                } else if (arrow == 1) { /* Down */
                    int max_sel = clamp((int)count - page * page_size - 1, 0, page_size - 1);
                    if (selected < max_sel)
                        selected++;
                    else if (page < ((int)count + page_size - 1) / page_size - 1)
                        page++;
                } else if (arrow == -2) { /* Home → first */
                    page = 0;
                    selected = 0;
                } else if (arrow == 2) { /* End → last */
                    page = ((int)count + page_size - 1) / page_size - 1;
                    selected = (int)count - page * page_size - 1;
                    selected = clamp(selected, 0, page_size - 1);
                }
                display_menu(suggestions, count, page_size, page, selected);
                continue;
            }

            /* Check for Page Up (ESC [ 5 ~) / Page Down (ESC [ 6 ~) */
            if (nbytes >= 3) {
                if (buf[1] == '[') {
                    if (buf[2] == '5' && nbytes >= 4 && buf[3] == '~') {
                        if (page > 0) {
                            page--;
                            selected = page_size - 1;
                            if ((int)count - page * page_size <= selected)
                                selected = (int)count - page * page_size - 1;
                            if (selected < 0) selected = 0;
                            display_menu(suggestions, count, page_size, page, selected);
                        }
                        continue;
                    }
                    if (buf[2] == '6' && nbytes >= 4 && buf[3] == '~') {
                        int total_pages = (int)count > 0
                            ? ((int)count + page_size - 1) / page_size
                            : 1;
                        if (page < total_pages - 1) {
                            page++;
                            selected = 0;
                            display_menu(suggestions, count, page_size, page, selected);
                        }
                        continue;
                    }
                }
            }
            /* Unknown escape sequence — ignore */
            continue;
        }

        /* Regular character */
        if (c == '\n' || c == '\r') {
            /* Select current item */
            result = page * page_size + selected;
            break;
        }

        if (c == 'q' || c == 'Q' || c == 27) {
            result = -1; /* cancelled */
            break;
        }

        if (c >= '1' && c <= '9') {
            int target = c - '1';
            int start_idx = page * page_size;
            if (target >= 0 && start_idx + target < (int)count) {
                selected = target;
                /* Snap to the right page */
                int wanted_page = target / page_size;
                if (wanted_page != page) {
                    page = wanted_page;
                }
                display_menu(suggestions, count, page_size, page, selected);
            }
            continue;
        }
    }

    exit_raw();
    fprintf(stderr, ANSI_RESET);
    fflush(stderr);
    return result;
}
