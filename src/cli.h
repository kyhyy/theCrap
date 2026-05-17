#ifndef CLI_H
#define CLI_H

#include <stddef.h>

/* Possible top-level subcommands */
typedef enum {
    CRP_CMD_NONE,
    CRP_CMD_FIX,
    CRP_CMD_INIT,
} crap_cmd;

/* Shell targets for the init command */
typedef enum {
    CRP_SHELL_NONE,
    CRP_SHELL_BASH,
    CRP_SHELL_ZSH,
    CRP_SHELL_FISH,
    CRP_SHELL_PWSH,
} crap_shell;

/* A parsed "fix" command */
typedef struct {
    const char *cmd;        /* the broken command string */
    int page_size;          /* from CRP_PAGE_SIZE, default 5 */
    int quick_enabled;      /* from CRP_QUICK_ENABLE, default 1 (true) */
    int search_depth;       /* from CRP_QUICK_SEARCH_DEPTH, default 1000 */
} crap_fix_cmd;

/* A parsed "init" command */
typedef struct {
    crap_shell shell;       /* bash, zsh, fish, or pwsh */
    const char *alias_name; /* default "crap" */
} crap_init_cmd;

/**
 * Parse argc/argv into the appropriate subcommand.
 *
 * Returns CRP_CMD_NONE if no valid subcommand was found (or --help/-h was
 * requested).
 *
 * On success, fills in one of the output pointers (fix_cmd or init_cmd).
 * The caller must not free any string pointers returned through fix_cmd->cmd
 * (they point into argv). init_cmd->alias_name may point into argv or be a
 * static default.
 *
 * Returns 0 on success, -1 on error (e.g. --name used without init).
 */
int crap_parse(int argc, const char **argv,
               crap_cmd *out_cmd,
               crap_fix_cmd *out_fix,
               crap_init_cmd *out_init);

/**
 * Print a usage/help message to stderr.
 */
void crap_usage(const char *progname);

/**
 * Execute the init subcommand. Generates shell alias script.
 *
 * Returns 0 on success, -1 on error.
 */
int crap_init(crap_init_cmd *init);

#endif /* CLI_H */
