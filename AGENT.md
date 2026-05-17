# thecrap — a terminal utility that fixes mistakes in your commands

A C rewrite of thefuck/fixit — a command-line utility that fixes mistakes in your previous command. Inspired by thefuck and fixit.

When you type a failed command, `thecrap` can suggest a corrected version. For example:

    $ gti push
    error: unknown command 'gti'

    $ crap
    > git push
    # press Enter and it runs automatically

# Current Status

**Status: Core infrastructure complete, rules engine working.**

| Component | Status | Tests |
|-----------|--------|-------|
| shlex (tokenizer) | ✅ Complete | 26/26 |
| match (Damerau-Levenshtein) | ✅ Complete | 13/13 |
| CLI parser (crap_parse) | ✅ Complete | 13/13 |
| Init (shell templates) | ✅ bash/zsh/fish | - |
| Rules engine | ✅ Complete | - |
| 5 working rules | ✅ sudo, git_wrong, mkdir -p, rm -r, cp -R | - |
| stderr capture | ✅ fork + pipe | - |
| get_text (terminal APIs) | ⏳ Not yet implemented | - |
| Selection UI | ⏳ Not yet implemented | - |

**Total: 52 tests passing.**

# Naming Convention

- **`thecrap`** — binary name
- **`crap`** — default alias name (bash/zsh/fish), configurable via `--name`
- **`crp`** — short prefix for internal types (e.g., `crp_cmd`)
- **`CRP_`** — uppercase prefix for enums/constants (e.g., `CRP_CMD_FIX`)
- **`crp_*`** — lowercase prefix for functions (e.g., `crp_parse`)
- **`CRP`** — env var prefix (e.g., `CRP_PAGE_SIZE`)
- **`crp_*`** — prefix for rule module functions (e.g., `crp_sudo`)

## File Structure

```
src/main.c              - Entry point, fork+pipe to capture stderr, dispatches to fix/init
src/cli.h / cli.c       - CLI parsing (fix, init, --name, CRP_* env vars)
src/shlex.h / shlex.c   - Shell command tokenizer (quotes, escapes)
src/match.h / match.c   - Damerau-Levenshtein edit distance
src/init.c              - Shell alias generation (bash/zsh/fish)
src/rules/
    rules.h             - Rule type definitions and registry
    rules.c             - Rule execution (crp_find_fixes, deduplication, sorting)
    rules_registry.c    - CRP_RULES[] registration and crp_rules_count
    rules_sudo.c        - Permission denied -> sudo
    rules_git_wrong.c   - Git typo corrections
    rules_misc.c        - mkdir_missing_parent, rm_dir, cp_dir
src/get_text/           - Stubbed (placeholder files only)
templates/
    alias.sh            - Bash alias template
    alias.zsh           - Zsh alias template
    alias.fish          - Fish alias template
tests/
    test_shlex.c        - Shell lexer tests
    test_match.c        - Edit distance tests
    test_cli.c          - CLI parser tests
Makefile                - Build system
README.md               - Project documentation
AGENT.md                - This file
.gitignore              - Build artifacts and OS files
install.sh              - Installer script
```

# Build System

This is a pure C project. Build with `make` or manual `gcc` commands.

```bash
cd /home/khajduk/Projects/thecrap

# Build everything (all sources)
gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -I. \
    -o build/thecrap \
    src/main.c src/shlex.c src/cli.c src/match.c src/init.c

# Run tests individually
gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -I. -o build/test_shlex tests/test_shlex.c src/shlex.c && ./build/test_shlex
gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -I. -o build/test_match tests/test_match.c src/match.c && ./build/test_match
gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -I. -o build/test_cli tests/test_cli.c src/cli.c src/shlex.c && ./build/test_cli

# Verify binary works
./build/thecrap init bash | head -2
./build/thecrap fix "gti push"
```

**Build rules:**
- No external dependencies. Only standard C library (stdlib, string, stdio, stddef).
- Use `-std=c11` with `-Wall -Wextra -Wpedantic`.
- The `build/` directory must exist before linking (`mkdir -p build`).
- All source files live under `src/` with headers co-located (`foo.h` / `foo.c`).

# How It Works

The system has two operational paths:

## Init Path (shell alias setup)

```
thecrap init bash   -->   generates a bash function alias for stdout
thecrap init zsh    -->   generates a zsh function alias for stdout
thecrap init fish   -->   generates a fish function alias for stdout
```

The generated alias should be sourced into the user's shell config. When invoked (e.g., `crap`), it:
1. Captures the last command and its error output
2. Calls `thecrap fix "<command>"`
3. If a fix is found, echoes it and evaluates it

## Fix Path (the actual logic)

```
thecrap fix "<broken command>"
```

Flow:

```
1. Parse command args (fix <cmd>, --name, env vars)
2. Tokenize command with shlex()  -->  ["git", "push"]
3. Get command output:
   a. Quick path: query terminal emulator via API (kitty, tmux, WezTerm, etc.)
   b. Fallback: re-run command capturing stderr
4. Run rules against (tokenized_cmd, error_output):
   - Each rule: fn(cmd_tokens, error_text) --> Option<Vec<String>>
   - Rules run in parallel (pthread in C)
5. Deduplicate suggestions, sort by similarity
6. Print selected fix to stdout
```

# CRITICAL Implementation Details

## shlex (shell lexer)

- Handles single quotes `'...'`, double quotes `"..."`, and backslash escapes `\`.
- Backslash escapes work both inside AND outside quotes.
- Returns `char **` via malloc (caller must `shlex_free()` each token then the array).
- Empty string returns `NULL, count=0`.
- Whitespace splitting includes `\t`, `\n`, `\r`.
- **Bug note:** tokens must be independent copies (use `strsave()`), not pointers into the shared word buffer.

## match (Damerau-Levenshtein distance)

- Computes edit distance between two strings.
- Supports insertions, deletions, substitutions, and adjacent transpositions.
- Returns `size_t` (returns `SIZE_MAX` on malloc failure).
- NULL input returns `0`.
- Space complexity: `O(m*n)` for the DP table. For very long strings, consider row-optimization later.

## CLI parser

- Subcommands: `fix <cmd...>`, `init <shell> [--name N]`
- Environment variables:
  - `CRP_PAGE_SIZE` — suggestions per page (default: 5)
  - `CRP_QUICK_ENABLE` — enable terminal quick search (default: 1)
  - `CRP_QUICK_SEARCH_DEPTH` — scrollback lines to scan (default: 1000)
- `fix` joins all args after `fix` into a single string (space-separated).
- `init` default alias is `"crap"` (unless overridden by `--name`).
- Returns 0 on success, -1 on error. Prints usage to stderr.

## Rules system

- A rule has type: `crp_rule_result_t *fn(const char **tokens, size_t token_count, const char *error)`
- Rules are registered in `rules_registry.c` as `CRP_RULES[]`.
- `crp_find_fixes()` in `rules.c`:
  1. Iterates all rules sequentially (pthread for parallelism planned)
  2. For each rule, tries each error line
  3. Deduplicates via string comparison
  4. Sorts by Damerau-Levenshtein similarity (lower = more similar)
- **Never add a rule that could silently execute destructive commands** (e.g., `rm` without confirmation).

## Implemented rules (5)

| Rule | Function | Triggers |
|------|---------|---------|
| sudo | `crp_sudo()` | "permission denied", "operation not permitted", "access denied" |
| git_wrong_command | `crp_git_wrong_command()` | "not a git command" + "similar command is" |
| mkdir_missing_parent | `crp_mkdir_missing_parent()` | "No such file or directory" on mkdir |
| rm_dir | `crp_rm_dir()` | "Is a directory" on rm |
| cp_dir | `crp_cp_dir()` | "Is a directory" or "omitting directory" on cp |

## get_text (quick search)

Priority order (first match wins):
1. tmux (capture-pane)
2. kitty (remote control text)
3. zellij (get-text)
4. wezterm (get-text)
5. iterm (OSC escape sequences)
6. macos_terminal (OSC escape sequences)
7. Fallback: re-run command and capture output (currently only this works)

# Rules to Implement (priority order)

## MUST HAVE (core rules from original)

1. **sudo** — ✅ detect "permission denied" / "operation not permitted", prepend `sudo`
2. **git_wrong_command** — ✅ detect "is not a git command", suggest similar command via regex
3. **command_not_found** — search `$PATH` for typos (which/whereis alternatives)
4. **git_branch_exists** — detect "A branch named 'X' already exists" in checkout -b
5. **mkdir_missing_parent** — ✅ detect "No such file or directory", suggest `mkdir -p`
6. **rm_dir** — ✅ detect "Is a directory", suggest `rm -r`
7. **cp_dir** — ✅ detect "Is a directory" or "omitting directory", suggest `cp -R`
8. **cp_cwd** — detect single-argument cp, suggest adding `/current/dir`

## NICE TO HAVE

9. **cargo_wrong_command** — similar to git_wrong_command for cargo
10. **brew_update_upgrade** — suggest `brew upgrade` instead of `brew update`
11. **git_no_upstream** — suggest `git push -u origin branch`
12. **taskfile_no_task** — suggest task names from Taskfile.yml
13. **git_commit_no_changes** — suggest `git commit -a`

# Test Commands

All existing tests should pass:

```bash
# shlex tests (quotes, escapes, empty, NULL)
./build/test_shlex   # expect: 26/26 pass

# match tests (edit distance)
./build/test_match   # expect: 13/13 pass

# CLI parser tests (fix, init, --name, env vars)
./build/test_cli     # expect: 13/13 pass
```

Expected total: **52 tests passing**.

When adding new rule modules, create corresponding test files in `tests/`. Each rule module should have inline `#if 0 ... #endif` test blocks or a dedicated test file.

# Development Rules

- **Always test before committing.** Run `./build/test_*` for all existing test suites.
- **Keep dependencies zero.** No external libraries beyond the C standard library.
- **Use pthreads sparingly.** Only for parallel rule evaluation. No thread pools, no async.
- **Error handling:** return -1 for errors, set errno or use stderr for messages. Avoid custom error types.
- **Memory safety:** every `malloc`/`realloc` must have a matching `free`. Use RAII-style cleanup (goto bail pattern) in complex functions.
- **No dead code.** If something is unused, remove it. The `shell_name()` warning in cli.c was already removed.
- **Keep the init templates minimal.** Bash/zsh/fish support is the priority. PowerShell can be stubbed out.
- **Follow the naming convention strictly.** Never use `cnt`, `CNT`, `cunt`, `thecunt`, `fixit`, `fixit_*`.
- **When in doubt, match the original fixit behavior.** The Rust implementation is the reference.

# Original Reference Implementation

The original is at: https://github.com/eugene-babichenko/fixit
and: https://github.com/nvbn/thefuck

# Debugging Notes

- Reusable debug scripts belong in `./debug`.
- One-off throwaway debugging tools should be created in `/tmp` and discarded.
- The `build/` directory is ephemeral — recreate with `mkdir -p build` if deleted.
- Use `gcc -g -O0` for debugging builds when needed.

# Known Pitfalls

1. **Terminal creation:** `mkdir -p build` before linking, or `ld` will fail with "cannot open output file."
2. **Include paths:** use `-I.` for header resolution, otherwise `"src/shlex.h"` style paths fail.
3. **Unicode escapes:** never use `\uXXXX` in C source. The C compiler does not treat these as universal character names until C23. Write raw `<`, `>`, `&` characters instead.
4. **shlex token memory:** never store pointers into the shared `word` buffer. Always make independent copies. This caused a double-free bug early on.
5. **shlex backslash escapes:** must work inside quotes too. The original Rust implementation escapes globally, not just outside quotes.
6. **Init template --name:** the `--name` flag must be detected and the function name substituted in all output. Don't forget `complete`/`compdef` lines at the bottom.
7. **Default alias:** always `"crap"` unless `--name` overrides. Never use hardcoded other names.
8. **Damerau-Levenshtein expected values:** transpositions count as 1 edit (not 2). Double-check test expectations against the algorithm's actual behavior, not intuition.
9. **pthread safety:** only parallelize rule evaluation. Each rule receives a copy of the command tokens. Never share mutable state between rule threads.
