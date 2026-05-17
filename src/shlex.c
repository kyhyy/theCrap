#include "shlex.h"

#include <stdlib.h>
#include <string.h>

static char *strsave(const char *s, size_t n)
{
    char *d = malloc(n + 1);
    if (!d)
        return NULL;
    memcpy(d, s, n);
    d[n] = '\0';
    return d;
}

char **shlex(const char *cmd, size_t *count)
{
    if (cmd == NULL) {
        if (count)
            *count = 0;
        return NULL;
    }

    size_t cap = 8;
    size_t n = 0;

    char **tokens = malloc(cap * sizeof *tokens);
    if (!tokens) {
        if (count)
            *count = 0;
        return NULL;
    }

    size_t word_cap = 32;
    char *word = malloc(word_cap);
    if (!word) {
        free(tokens);
        if (count)
            *count = 0;
        return NULL;
    }
    size_t word_len = 0;

    const char *p = cmd;
    int in_quote = 0;   /* 0 = no quote, '"' = double, '\'' = single */
    int escaped = 0;

    while (*p) {
        if (escaped) {
            escaped = 0;
            if (word_len + 1 >= word_cap) {
                size_t new_cap = word_cap * 2;
                char *tmp = realloc(word, new_cap);
                if (!tmp) {
                    free(tokens);
                    free(word);
                    if (count)
                        *count = n;
                    return NULL;
                }
                word_cap = new_cap;
                word = tmp;
            }
            word[word_len++] = *p;
            p++;
            continue;
        }

        if (*p == '\\') {
            escaped = 1;
            if (word_len + 1 >= word_cap) {
                size_t new_cap = word_cap * 2;
                char *tmp = realloc(word, new_cap);
                if (!tmp) {
                    free(tokens);
                    free(word);
                    if (count)
                        *count = n;
                    return NULL;
                }
                word_cap = new_cap;
                word = tmp;
            }
            word[word_len++] = '\\';
            p++;
            continue;
        }

        if (!in_quote && (*p == '"' || *p == '\'')) {
            in_quote = *p;
            if (word_len + 1 >= word_cap) {
                size_t new_cap = word_cap * 2;
                char *tmp = realloc(word, new_cap);
                if (!tmp) {
                    free(tokens);
                    free(word);
                    if (count)
                        *count = n;
                    return NULL;
                }
                word_cap = new_cap;
                word = tmp;
            }
            word[word_len++] = *p;
            p++;
            continue;
        }

        if (in_quote) {
            if (*p == in_quote) {
                in_quote = 0;
            }
            if (word_len + 1 >= word_cap) {
                size_t new_cap = word_cap * 2;
                char *tmp = realloc(word, new_cap);
                if (!tmp) {
                    free(tokens);
                    free(word);
                    if (count)
                        *count = n;
                    return NULL;
                }
                word_cap = new_cap;
                word = tmp;
            }
            word[word_len++] = *p;
            p++;
            continue;
        }

        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            if (word_len > 0) {
                word[word_len] = '\0';
                /* Make an independent copy so we can reuse word */
                char *token = strsave(word, word_len);
                if (!token) {
                    free(word);
                    free(tokens);
                    if (count)
                        *count = n;
                    return NULL;
                }
                if (n + 1 > cap) {
                    size_t new_cap = cap * 2;
                    char **tmp = realloc(tokens, new_cap * sizeof *tokens);
                    if (!tmp) {
                        free(token);
                        free(word);
                        if (count)
                            *count = n;
                        return NULL;
                    }
                    cap = new_cap;
                    tokens = tmp;
                }
                tokens[n++] = token;
                word_cap = 32;
                free(word);
                word = malloc(word_cap);
                if (!word) {
                    if (count)
                        *count = n;
                    return NULL;
                }
                word_len = 0;
            }
            p++;
            continue;
        }

        if (word_len + 1 >= word_cap) {
            size_t new_cap = word_cap * 2;
            char *tmp = realloc(word, new_cap);
            if (!tmp) {
                free(tokens);
                free(word);
                if (count)
                    *count = n;
                return NULL;
            }
            word_cap = new_cap;
            word = tmp;
        }
        word[word_len++] = *p;
        p++;
    }

    /* Flush remaining word */
    if (word_len > 0) {
        word[word_len] = '\0';
        char *token = strsave(word, word_len);
        free(word);
        if (!token) {
            free(tokens);
            if (count)
                *count = n;
            return NULL;
        }
        if (n + 1 > cap) {
            size_t new_cap = cap * 2;
            char **tmp = realloc(tokens, new_cap * sizeof *tokens);
            if (!tmp) {
                free(token);
                if (count)
                    *count = n;
                return NULL;
            }
            cap = new_cap;
            tokens = tmp;
        }
        tokens[n++] = token;
    }

    if (count)
        *count = n;

    return tokens;
}

void shlex_free(char **tokens, size_t count)
{
    if (!tokens)
        return;
    for (size_t i = 0; i < count; i++)
        free(tokens[i]);
    free(tokens);
}
