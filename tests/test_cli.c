#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/cli.h"

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

static void test_fix_cmd(void)
{
    printf("\n--- fix subcommand ---\n");

    const char *argv[] = {"thecrap", "fix", "gti", "push"};
    crap_cmd cmd;
    crap_fix_cmd fix;
    crap_init_cmd init;
    int ret = crap_parse(4, argv, &cmd, &fix, &init);

    check("fix parses OK", ret == 0);
    check("cmd is FIX", cmd == CRP_CMD_FIX);
    check("cmd string == 'gti push'", strcmp(fix.cmd, "gti push") == 0);
    check("page_size default 5", fix.page_size == 5);
}

static void test_init_cmd(void)
{
    printf("\n--- init subcommand ---\n");

    const char *argv[] = {"thecrap", "init", "bash"};
    crap_cmd cmd;
    crap_fix_cmd fix;
    crap_init_cmd init;
    int ret = crap_parse(3, argv, &cmd, &fix, &init);

    check("init parses OK", ret == 0);
    check("cmd is INIT", cmd == CRP_CMD_INIT);
    check("shell is BASH", init.shell == CRP_SHELL_BASH);
    check("alias default 'crap'", strcmp(init.alias_name, "crap") == 0);
}

static void test_init_custom_name(void)
{
    printf("\n--- init --name ---\n");

    const char *argv[] = {"thecrap", "init", "zsh", "--name", "f"};
    crap_cmd cmd;
    crap_fix_cmd fix;
    crap_init_cmd init;
    int ret = crap_parse(5, argv, &cmd, &fix, &init);

    check("--name parses OK", ret == 0);
    check("shell is ZSH", init.shell == CRP_SHELL_ZSH);
    check("alias is 'f'", strcmp(init.alias_name, "f") == 0);
}

static void test_no_args(void)
{
    printf("\n--- no args ---\n");

    const char *argv[] = {"thecrap"};
    crap_cmd cmd;
    crap_fix_cmd fix;
    crap_init_cmd init;
    int ret = crap_parse(1, argv, &cmd, &fix, &init);
    check("no args returns -1", ret == -1);
}

static void test_unknown_subcmd(void)
{
    printf("\n--- unknown subcommand ---\n");

    const char *argv[] = {"thecrap", "bleh"};
    crap_cmd cmd;
    crap_fix_cmd fix;
    crap_init_cmd init;
    int ret = crap_parse(2, argv, &cmd, &fix, &init);
    check("unknown subcmd returns -1", ret == -1);
}

int main(void)
{
    printf("=== CLI parser tests ===\n");

    test_fix_cmd();
    test_init_cmd();
    test_init_custom_name();
    test_no_args();
    test_unknown_subcmd();

    printf("\n=== Results ===\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
