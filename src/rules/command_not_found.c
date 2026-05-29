#define _POSIX_C_SOURCE 200809L
#include "rules.h"
#include "../match.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>

#define MAX_DISTANCE 3

static int extract_typo(const char *error, char *out, size_t out_size)
{
    const char *p;
    size_t i;

    /* bash/sh: "gti: command not found" or "gti: not found" */
    p = strstr(error, ": command not found");
    if (!p) p = strstr(error, ": not found");
    if (p) {
        const char *end = p;
        const char *start = end;
        while (start > error && *(start-1) != ' ' && *(start-1) != ':')
            start--;
        size_t len = (size_t)(end - start);
        if (len > 0 && len < out_size) {
            memcpy(out, start, len);
            out[len] = '\0';
            return 1;
        }
    }

    /* zsh: "command not found: gti" */
    p = strstr(error, "command not found: ");
    if (p) {
        p += 19;
        i = 0;
        while (p[i] && p[i] != '\n' && p[i] != ' ' && i < out_size - 1) {
            out[i] = p[i];
            i++;
        }
        out[i] = '\0';
        if (i > 0) return 1;
    }

    /* fish (lowercased): "unknown command: gti" */
    p = strstr(error, "unknown command: ");
    if (p) {
        p += 17;
        i = 0;
        while (p[i] && p[i] != '\n' && p[i] != ' ' && i < out_size - 1) {
            out[i] = p[i];
            i++;
        }
        out[i] = '\0';
        if (i > 0) return 1;
    }

    /* ubuntu: "command 'gti' not found" */
    p = strstr(error, "command '");
    if (p) {
        p += 9;
        i = 0;
        while (p[i] && p[i] != '\'' && i < out_size - 1) {
            out[i] = p[i];
            i++;
        }
        out[i] = '\0';
        if (i > 0) return 1;
    }

    return 0;
}

/* Returns 1 if b is a pure transposition of a (same chars, same length, one swap) */
static int is_transposition_only(const char *a, const char *b)
{
    size_t la = strlen(a);
    size_t lb = strlen(b);
    if (la != lb) return 0;

    int diffs = 0;
    int diff_pos[2] = {-1, -1};
    for (size_t i = 0; i < la; i++) {
        if (a[i] != b[i]) {
            if (diffs >= 2) return 0;
            diff_pos[diffs++] = (int)i;
        }
    }
    if (diffs != 2) return 0;
    return a[diff_pos[0]] == b[diff_pos[1]] && a[diff_pos[1]] == b[diff_pos[0]];
}

crp_rule_result_t *crp_command_not_found(const char **tokens, size_t token_count, const char *error)
{
    if (!tokens || token_count == 0) return NULL;

    char typo[256] = {0};
    if (!extract_typo(error, typo, sizeof(typo)))
        return NULL;

    if (strcmp(tokens[0], typo) != 0)
        return NULL;

    const char *path_env = getenv("PATH");
    if (!path_env) return NULL;

    char *path_copy = strdup(path_env);
    if (!path_copy) return NULL;

    char *best = NULL;
    size_t best_dist = MAX_DISTANCE + 1;
    int best_is_transposition = 0;

    char *saveptr = NULL;
    char *dir = strtok_r(path_copy, ":", &saveptr);
    while (dir) {
        DIR *dp = opendir(dir);
        if (dp) {
            struct dirent *ent;
            while ((ent = readdir(dp)) != NULL) {
                if (ent->d_name[0] == '.') continue;

                size_t dist = damerau_levenshtein(typo, ent->d_name);
                if (dist == 0 || dist > MAX_DISTANCE) continue;

                int is_trans = is_transposition_only(typo, ent->d_name);

                /* Prefer: lower distance first.
                 * On a tie: prefer transposition over substitution. */
                int better = 0;
                if (dist < best_dist) {
                    better = 1;
                } else if (dist == best_dist && is_trans && !best_is_transposition) {
                    better = 1;
                }

                if (better) {
                    free(best);
                    best = strdup(ent->d_name);
                    if (!best) { closedir(dp); free(path_copy); return NULL; }
                    best_dist = dist;
                    best_is_transposition = is_trans;
                }
            }
            closedir(dp);
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }
    free(path_copy);

    if (!best) return NULL;

    /* Reconstruct command with corrected first token */
    size_t len = strlen(best) + 1;
    for (size_t i = 1; i < token_count; i++)
        len += strlen(tokens[i]) + 1;

    char *buf = malloc(len);
    if (!buf) { free(best); return NULL; }

    char *q = buf;
    q += sprintf(q, "%s", best);
    for (size_t i = 1; i < token_count; i++)
        q += sprintf(q, " %s", tokens[i]);
    *q = '\0';
    free(best);

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
