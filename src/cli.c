#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_int_env(const char *var, int default_val)
{
    const char *val = getenv(var);
    if (!val || *val == '\0')
        return default_val;
    char *end;
    long lv = strtol(val, &end, 10);
    if (*end != '\0' || lv < 1)
        return default_val;
    return (int)lv;
}

static crap_shell shell_from_name(const char *name)
{
    if (strcmp(name, "bash") == 0)  return CRP_SHELL_BASH;
    if (strcmp(name, "zsh") == 0)   return CRP_SHELL_ZSH;
    if (strcmp(name, "fish") == 0)  return CRP_SHELL_FISH;
    if (strcmp(name, "powershell") == 0 || strcmp(name, "pwsh") == 0)
                                      return CRP_SHELL_PWSH;
    return CRP_SHELL_NONE;
}

void crap_usage(const char *progname)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s fix <command>           Fix a failed command\n"
        "  %s init <shell> [--name N]  Generate shell alias\n"
        "\n"
        "Subcommands:\n"
        "  fix         Pass the broken command string as the argument\n"
        "  init        One of: bash, zsh, fish, powershell\n"
        "\n"
        "Environment variables:\n"
        "  CRP_PAGE_SIZE              Suggestions per page (default: 5)\n"
        "  CRP_QUICK_ENABLE           Use terminal quick search (default: 1)\n"
        "  CRP_QUICK_SEARCH_DEPTH     Scrollback lines to scan (default: 1000)\n"
        "\n"
        "Examples:\n"
        "  %s fix \"gti push\"\n"
        "  %s init bash --name crap\n",
        progname, progname, progname, progname);
}

int crap_parse(int argc, const char **argv,
               crap_cmd *out_cmd,
               crap_fix_cmd *out_fix,
               crap_init_cmd *out_init)
{
    if (!out_cmd || !out_fix || !out_init)
        return -1;

    memset(out_cmd, 0, sizeof *out_cmd);
    memset(out_fix, 0, sizeof *out_fix);
    memset(out_init, 0, sizeof *out_init);

    if (argc < 2) {
        crap_usage(argv[0]);
        return -1;
    }

    const char *sub = argv[1];

    /* ---- fix subcommand ---- */
    if (strcmp(sub, "fix") == 0) {
        if (argc < 3) {
            fprintf(stderr, "error: 'fix' requires a command argument\n");
            return -1;
        }

        /* Collect everything after "fix" as the command string */
        const char **start = &argv[2];
        int n = argc - 2;

        /* Reconstruct the command: "gti push" from argv["gti", "push"] */
        size_t len = 0;
        for (int i = 0; i < n; i++) {
            len += strlen(start[i]) + 1; /* +1 for space */
        }

        char *cmd = malloc(len);
        if (!cmd)
            return -1;

        char *p = cmd;
        for (int i = 0; i < n; i++) {
            size_t sl = strlen(start[i]);
            memcpy(p, start[i], sl);
            p += sl;
            if (i < n - 1) {
                *p++ = ' ';
            }
        }
        *p = '\0';

        out_fix->cmd = cmd;
        out_fix->page_size = parse_int_env("CRP_PAGE_SIZE", 5);
        out_fix->quick_enabled = parse_int_env("CRP_QUICK_ENABLE", 1);
        out_fix->search_depth = parse_int_env("CRP_QUICK_SEARCH_DEPTH", 1000);

        *out_cmd = CRP_CMD_FIX;
        return 0;
    }

    /* ---- init subcommand ---- */
    if (strcmp(sub, "init") == 0) {
        if (argc < 3) {
            fprintf(stderr, "error: 'init' requires a shell argument\n");
            return -1;
        }

        crap_shell shell = shell_from_name(argv[2]);
        if (shell == CRP_SHELL_NONE) {
            fprintf(stderr, "error: unknown shell '%s'\n", argv[2]);
            fprintf(stderr, "supported: bash, zsh, fish, powershell\n");
            return -1;
        }

        out_init->shell = shell;
        out_init->alias_name = "crap"; /* canonical default */

        /* Scan for --name */
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--name") == 0) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "error: --name requires a value\n");
                    return -1;
                }
                out_init->alias_name = argv[i + 1];
                break;
            }
        }

        *out_cmd = CRP_CMD_INIT;
        return 0;
    }

    /* Unknown subcommand */
    fprintf(stderr, "error: unknown subcommand '%s'\n", sub);
    fprintf(stderr, "supported: fix, init\n");
    return -1;
}
