#define _POSIX_C_SOURCE 200809L
#include "rules.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static crp_rule_result_t *make_result(const char *suggestion)
{
    crp_rule_result_t *res = calloc(1, sizeof *res);
    if (!res) return NULL;
    res->suggestions = calloc(1, sizeof *res->suggestions);
    if (!res->suggestions) { free(res); return NULL; }
    res->suggestions[0] = strdup(suggestion);
    if (!res->suggestions[0]) { free(res->suggestions); free(res); return NULL; }
    res->count = 1;
    return res;
}

crp_rule_result_t *crp_sudo(const char **tokens, size_t token_count, const char *error)
{
    if (!tokens || token_count == 0) return NULL;
    if (strcmp(tokens[0], "sudo") == 0) return NULL;

    if (strstr(error, "permission denied") == NULL
     && strstr(error, "operation not permitted") == NULL
     && strstr(error, "access denied") == NULL
     && strstr(error, "authentication failure") == NULL
     && strstr(error, "eacces") == NULL
     && strstr(error, "epERM") == NULL
     && strstr(error, "ePERM") == NULL)
        return NULL;

    size_t len = 5;
    for (size_t i = 0; i < token_count; i++)
        len += strlen(tokens[i]) + 1;

    char *buf = malloc(len);
    if (!buf) return NULL;

    char *p = buf;
    p += sprintf(p, "sudo");
    for (size_t i = 0; i < token_count; i++) {
        *p++ = ' ';
        p += sprintf(p, "%s", tokens[i]);
    }
    *p = '\0';

    crp_rule_result_t *res = make_result(buf);
    free(buf);
    return res;
}
