#ifndef MATCH_H
#define MATCH_H

#include <stddef.h>

/**
 * Compute the Damerau-Levenshtein distance between two strings.
 *
 * This is the minimum number of single-character edits (insertions, deletions,
 * substitutions, or adjacent transpositions) needed to change one string into
 * the other.
 *
 * @param a  first string (must not be NULL)
 * @param b  second string (must not be NULL)
 * @return     the edit distance
 */
size_t damerau_levenshtein(const char *a, const char *b);

#endif /* MATCH_H */
