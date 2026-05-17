#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/shlex.h"

static int tests_passed = 0;
static int tests_failed = 0;

static void check(const char *name, int cond)
{
    if (cond) {
        printf("  PASS: %s\n", name);
        tests_passed++;
    } else {
        printf("  FAIL: %s\n", name);
        tests_failed++;
    }
}

static void check_str_array(const char *name, const char *expected[], size_t exp_n, char **actual, size_t act_n)
{
    char msg[64];
    snprintf(msg, sizeof msg, "%s (count: %zu == %zu)", name, act_n, exp_n);
    check(msg, act_n == exp_n);

    for (size_t i = 0; i < (act_n < exp_n ? act_n : exp_n); i++) {
        snprintf(msg, sizeof msg, "%s[%zu] == %s", name, i, expected[i]);
        check(msg, actual && actual[i] && strcmp(actual[i], expected[i]) == 0);
    }
}

int main(void)
{
    printf("=== shlex tests ===\n\n");

    /* Test 1: empty string */
    {
        size_t count = 0;
        char **tokens = shlex("", &count);
        check_str_array("empty", (const char *[]){"git"}, 0, tokens, count);
        shlex_free(tokens, count);
    }

    /* Test 2: simple command */
    {
        size_t count = 0;
        char **tokens = shlex("git push origin master", &count);
        const char *expected[] = {"git", "push", "origin", "master"};
        check_str_array("simple", expected, 4, tokens, count);
        shlex_free(tokens, count);
    }

    /* Test 3: double quotes */
    {
        size_t count = 0;
        char **tokens = shlex("git commit -m \"initial commit\" --amend", &count);
        const char *expected[] = {"git", "commit", "-m", "\"initial commit\"", "--amend"};
        check_str_array("double quotes", expected, 5, tokens, count);
        shlex_free(tokens, count);
    }

    /* Test 4: single quotes */
    {
        size_t count = 0;
        char **tokens = shlex("echo 'hello world'", &count);
        const char *expected[] = {"echo", "'hello world'"};
        check_str_array("single quotes", expected, 2, tokens, count);
        shlex_free(tokens, count);
    }

    /* Test 5: escaped chars inside quotes */
    {
        size_t count = 0;
        char **tokens = shlex("git commit -m \"test \\\"escaping\\\"\" --amend", &count);
        const char *expected[] = {"git", "commit", "-m", "\"test \\\"escaping\\\"\"", "--amend"};
        check_str_array("escaped quotes", expected, 5, tokens, count);
        shlex_free(tokens, count);
    }

    /* Test 6: NULL input */
    {
        size_t count = 0;
        char **tokens = shlex(NULL, &count);
        check("NULL input returns NULL", tokens == NULL);
        check("NULL input count is 0", count == 0);
    }

    /* Test 7: count=NULL (out param not needed) */
    {
        size_t count = 0;
        char **tokens = shlex("ls -la", &count);
        check("no count param: count == 2", count == 2);
        if (tokens) {
            check("no count param: ls", strcmp(tokens[0], "ls") == 0);
            check("no count param: -la", strcmp(tokens[1], "-la") == 0);
            shlex_free(tokens, count);
        }
    }

    /* Summary */
    printf("\n=== Results ===\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
