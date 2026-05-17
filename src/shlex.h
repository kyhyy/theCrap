#ifndef SHLEX_H
#define SHLEX_H

#include <stddef.h>

/**
 * Tokenize a shell command string into individual tokens.
 * Handles single quotes, double quotes, and backslash escapes.
 *
 * Tokens are separated by whitespace. Quotes are preserved as part of
 * the token (so callers can decide how to handle them).
 *
 * @param cmd  null-terminated command string (must not be NULL)
 * @return     dynamically allocated array of token strings, or NULL on
 *             allocation failure. The caller must:
 *               1. free each string in the returned array
 *               2. free the array itself
 *             The caller must also check *count (see below).
 * @param[out] count  pointer to store the number of tokens (may be NULL)
 */
char **shlex(const char *cmd, size_t *count);

/**
 * Free the result of shlex().
 *
 * @param tokens  the array returned by shlex() (may be NULL)
 * @param count   the count returned by shlex()
 */
void shlex_free(char **tokens, size_t count);

#endif /* SHLEX_H */
