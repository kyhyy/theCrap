#define _POSIX_C_SOURCE 200809L
#include "rules.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Join tokens starting from offset (skip first `offset` tokens) */
static char *join_tokens_from(const char **tokens, size_t offset, size_t count)
{
    size_t len = 1;
    for (size_t i = 0; i < count; i++)
        len += strlen(tokens[offset + i]) + 1;

    char *buf = malloc(len);
    if (!buf) return NULL;
    char *p = buf;
    for (size_t i = 0; i < count; i++) {
        if (i > 0) *p++ = ' ';
        p += sprintf(p, "%s", tokens[offset + i]);
    }
    *p = '\0';
    return buf;
}

crp_rule_result_t *crp_mkdir_missing_parent(const char **tokens, size_t token_count, const char *error)
{
    if (!tokens || token_count == 0) return NULL;
    if (strcmp(tokens[0], "mkdir") != 0) return NULL;
    if (strstr(error, "No such file or directory") == NULL) return NULL;

    /* Join everything AFTER the command name */
    char *args = join_tokens_from(tokens, 1, token_count - 1);
    if (!args) return NULL;

    size_t new_len = 9 + strlen(args) + 1; /* "mkdir -p " + args + \0 */
    char *buf = malloc(new_len);
    if (!buf) { free(args); return NULL; }
    sprintf(buf, "mkdir -p %s", args);
    free(args);

    crp_rule_result_t *res = calloc(1, sizeof *res);
    if (!res) { free(buf); return NULL; }
    res->suggestions = calloc(1, sizeof *res->suggestions);
    if (!res->suggestions) { free(res); free(buf); return NULL; }
    res->suggestions[0] = strdup(buf);
    free(buf);
    if (!res->suggestions[0]) { free(res->suggestions); free(res); return NULL; }
    res->count = 1;
    return res;
}

crp_rule_result_t *crp_rm_dir(const char **tokens, size_t token_count, const char *error)
{
    if (!tokens || token_count == 0) return NULL;
    if (strcmp(tokens[0], "rm") != 0) return NULL;
    if (strstr(error, "Is a directory") == NULL) return NULL;

    char *args = join_tokens_from(tokens, 1, token_count - 1);
    if (!args) return NULL;

    size_t new_len = 6 + strlen(args) + 1; /* "rm -r " + args + \0 */
    char *buf = malloc(new_len);
    if (!buf) { free(args); return NULL; }
    sprintf(buf, "rm -r %s", args);
    free(args);

    crp_rule_result_t *res = calloc(1, sizeof *res);
    if (!res) { free(buf); return NULL; }
    res->suggestions = calloc(1, sizeof *res->suggestions);
    if (!res->suggestions) { free(res); free(buf); return NULL; }
    res->suggestions[0] = strdup(buf);
    free(buf);
    if (!res->suggestions[0]) { free(res->suggestions); free(res); return NULL; }
    res->count = 1;
    return res;
}

crp_rule_result_t *crp_cp_dir(const char **tokens, size_t token_count, const char *error)
{
    if (!tokens || token_count < 2) return NULL;
    if (strcmp(tokens[0], "cp") != 0) return NULL;
    /* Linux cp: "-r not specified; omitting directory"
     * macOS cp: "Is a directory" */
    if (strstr(error, "omitting directory") == NULL
     && strstr(error, "Is a directory") == NULL
     && strstr(error, "-r not specified") == NULL)
        return NULL;

    /* Skip cp flags and source, join destination args */
    char *args = join_tokens_from(tokens, 1, token_count - 1);
    if (!args) return NULL;

    size_t new_len = 7 + strlen(args) + 1; /* "cp -R " + args + \0 */
    char *buf = malloc(new_len);
    if (!buf) { free(args); return NULL; }
    sprintf(buf, "cp -R %s", args);
    free(args);

    crp_rule_result_t *res = calloc(1, sizeof *res);
    if (!res) { free(buf); return NULL; }
    res->suggestions = calloc(1, sizeof *res->suggestions);
    if (!res->suggestions) { free(res); free(buf); return NULL; }
    res->suggestions[0] = strdup(buf);
    free(buf);
    if (!res->suggestions[0]) { free(res->suggestions); free(res); return NULL; }
    res->count = 1;
    return res;
}
