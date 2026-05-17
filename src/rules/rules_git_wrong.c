#define _POSIX_C_SOURCE 200809L
#include "rules.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

crp_rule_result_t *crp_git_wrong_command(const char **tokens, size_t token_count, const char *error)
{
    if (!tokens || token_count == 0) return NULL;
    if (strcmp(tokens[0], "git") != 0) return NULL;
    if (strstr(error, "not a git command") == NULL) return NULL;

    /* Try both "similar command is" and "similar commands are" formats */
    const char *similar = strstr(error, "similar command is");
    size_t offset = 18;
    if (!similar) {
        similar = strstr(error, "similar commands are");
        offset = 20;
    }
    if (!similar) return NULL;

    similar += offset;
    while (*similar == ' ' || *similar == '\n' || *similar == '\t')
        similar++;

    char corr[64] = {0};
    int i = 0;
    while (*similar && *similar != '\n' && *similar != ' ' && i < 63)
        corr[i++] = *similar++;
    corr[i] = '\0';

    if (corr[0] == '\0') return NULL;

    /* Reconstruct command: git <correction> <rest of args> */
    size_t len = 256;
    char *buf = malloc(len);
    if (!buf) return NULL;
    char *p = buf;

    p += sprintf(p, "git");
    p += sprintf(p, " %s", corr);
    for (size_t j = 2; j < token_count; j++)
        p += sprintf(p, " %s", tokens[j]);
    *p = '\0';

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
