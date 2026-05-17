#include "match.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static size_t min3(size_t a, size_t b, size_t c)
{
    return a < b ? (a < c ? a : c) : (b < c ? b : c);
}

size_t damerau_levenshtein(const char *a, const char *b)
{
    if (!a || !b)
        return 0;

    size_t m = strlen(a);
    size_t n = strlen(b);

    if (m == 0)
        return n;
    if (n == 0)
        return m;

    size_t rows = m + 1;
    size_t cols = n + 1;

    size_t *dp = malloc(rows * cols * sizeof *dp);
    if (!dp)
        return SIZE_MAX;

    for (size_t i = 0; i < rows; i++)
        dp[i] = i;
    for (size_t j = 0; j < cols; j++)
        dp[j * rows] = j;

    for (size_t i = 1; i < rows; i++) {
        for (size_t j = 1; j < cols; j++) {
            size_t cost = (a[i - 1] != b[j - 1]) ? 1 : 0;

            size_t insertion = dp[i + (j - 1) * rows] + 1;
            size_t deletion = dp[(i - 1) + j * rows] + 1;
            size_t substitution = dp[(i - 1) + (j - 1) * rows] + cost;

            dp[i + j * rows] = min3(insertion, deletion, substitution);

            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) {
                size_t transposition = dp[(i - 2) + (j - 2) * rows] + cost;
                if (transposition < dp[i + j * rows])
                    dp[i + j * rows] = transposition;
            }
        }
    }

    size_t result = dp[m + n * rows];
    free(dp);
    return result;
}
