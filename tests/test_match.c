#include <stdio.h>
#include <string.h>
#include "src/match.h"

static int tests_passed = 0;
static int tests_failed = 0;

static void check_str(const char *name, size_t expected, size_t actual)
{
    if (expected == actual) {
        printf("  PASS: %s (got %zu)\n", name, actual);
        tests_passed++;
    } else {
        printf("  FAIL: %s (expected %zu, got %zu)\n", name, expected, actual);
        tests_failed++;
    }
}

int main(void)
{
    printf("=== match (Damerau-Levenshtein) tests ===\n\n");

    /* Identical strings */
    check_str("identical", 0, damerau_levenshtein("git", "git"));

    /* One empty string */
    check_str("empty_a", 3, damerau_levenshtein("", "git"));
    check_str("empty_b", 3, damerau_levenshtein("git", ""));

    /* Single substitution */
    check_str("substitution", 1, damerau_levenshtein("git", "pit"));

    /* Single insertion */
    check_str("insertion", 1, damerau_levenshtein("it", "git"));

    /* Single deletion */
    check_str("deletion", 1, damerau_levenshtein("git", "gt"));

    /* Adjacent transposition */
    check_str("transposition", 1, damerau_levenshtein("git", "gti"));

    /* Classic: "git" -> "gti" (the fixit test case) */
    check_str("fixit classic", 1, damerau_levenshtein("gti", "git"));

    /* git push -> git pusk (push->pusk = 1 substitution + 1 transposition = 2) */
    check_str("pusk", 1, damerau_levenshtein("git pusk", "git push"));

    /* Multiple changes */
    check_str("multiple", 4, damerau_levenshtein("hello", "world"));

    /* Completely different */
    check_str("different", 4, damerau_levenshtein("abcd", "xyzw"));

    /* NULL safety */
    printf("\n");
    check_str("NULL a", 0, damerau_levenshtein(NULL, "git"));
    check_str("NULL b", 0, damerau_levenshtein("git", NULL));

    /* Summary */
    printf("\n=== Results ===\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
