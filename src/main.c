#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include "src/cli.h"
#include "src/shlex.h"
#include "src/rules/rules.h"
#include "src/ui.h"

int main(int argc, const char **argv)
{
    crap_cmd cmd;
    crap_fix_cmd fix;
    crap_init_cmd init;

    int ret = crap_parse(argc, argv, &cmd, &fix, &init);
    if (ret != 0)
        return 1;

    switch (cmd) {
    case CRP_CMD_FIX:
        {
        /* Tokenize the command */
        size_t token_count;
        char **tokens = shlex(fix.cmd, &token_count);
        if (!tokens) {
            fprintf(stderr, "error: failed to tokenize command\n");
            return 1;
        }

        /* Re-run via sh -c to get a proper shell error message
         * (e.g. "gti: command not found" instead of execvp failing silently) */
        int pipefd[2];
        if (pipe(pipefd) != 0) {
            fprintf(stderr, "error: pipe() failed\n");
            shlex_free(tokens, token_count);
            return 1;
        }

        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "error: fork() failed\n");
            shlex_free(tokens, token_count);
            return 1;
        }

        if (pid == 0) {
            /* Child: redirect both stdout and stderr into the pipe */
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);

            /* Use sh -c so we get proper "command not found" messages */
            execl("/bin/sh", "sh", "-c", fix.cmd, (char *)NULL);
            perror("execl");
            _exit(127);
        }

        /* Parent: read combined output */
        close(pipefd[1]);
        char stderr_buf[4096];
        size_t stderr_len = 0;
        ssize_t n;
        while (stderr_len < sizeof(stderr_buf) - 1 &&
               (n = read(pipefd[0], stderr_buf + stderr_len,
                         sizeof(stderr_buf) - stderr_len - 1)) > 0) {
            stderr_len += (size_t)n;
        }
        stderr_buf[stderr_len] = '\0';
        close(pipefd[0]);

        /* Wait for child */
        int status;
        waitpid(pid, &status, 0);

        int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;

        if (exit_code == 0) {
            /* Command succeeded — nothing to fix */
            shlex_free(tokens, token_count);
            return 0;
        }

        /* Find fixes */
        size_t fix_count;
        char **fixes = crp_find_fixes(fix.cmd, (const char **)tokens, token_count,
                                       stderr_buf, &fix_count);
        shlex_free(tokens, token_count);

        if (!fixes || fix_count == 0) {
            return 1;  /* exit 1 = no fixes, fish function checks $status */
        }

        /* Check if interactive selection is enabled */
        const char *select_env = getenv("CRP_SELECT");
        int ui_enabled = !(select_env && strcmp(select_env, "0") == 0);

        if (ui_enabled) {
            /* Interactive selection UI */
            int selected = ui_select((const char **)fixes, fix_count, fix.page_size);

            if (selected >= 0 && (size_t)selected < fix_count) {
                /* User selected a suggestion */
                printf("%s\n", fixes[selected]);
                for (size_t i = 0; i < fix_count; i++)
                    free(fixes[i]);
                free(fixes);
                return 0;
            }

            /* User cancelled — fall back to non-interactive mode */
            size_t shown = 0;
            for (size_t i = 0; i < fix_count; i++) {
                if (shown >= (size_t)fix.page_size) break;
                printf("%s\n", fixes[i]);
                shown++;
            }
            for (size_t i = 0; i < fix_count; i++)
                free(fixes[i]);
            free(fixes);
            return 0;
        }

        /* Non-interactive: print suggestions */
        size_t shown = 0;
        for (size_t i = 0; i < fix_count; i++) {
            if (shown >= (size_t)fix.page_size) break;
            printf("%s\n", fixes[i]);
            shown++;
        }

        /* Free fixes */
        for (size_t i = 0; i < fix_count; i++)
            free(fixes[i]);
        free(fixes);
        }
        break;

    case CRP_CMD_INIT:
        return crap_init(&init);

    case CRP_CMD_NONE:
        return 1;
    }

    return 0;
}
