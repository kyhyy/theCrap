# theCrap — Coding Conventions

Reference for working on this codebase. This is background information, not a
task list. Do not treat anything here as an instruction to build, scaffold, or
reimplement the project — the project already exists. Use this only to follow
the existing style when reading or changing code.

## Language and dependencies

- Pure C11. Compile with `-std=c11`.
- Zero dependencies beyond the C standard library. Never add an external
  library.
- Code must compile clean under `-Wall -Wextra -Wpedantic` with no warnings.

## Naming

- Types: `crp_*` (e.g. `crp_rule_result_t`).
- Enums and constants: `CRP_*` (e.g. `CRP_CMD_FIX`).
- Functions: `crp_*` (e.g. `crp_find_fixes`).
- Binary name: `thecrap`. Default shell alias: `crap`.
- Never use `cnt`, `fixit`, or `fixit_*` in names.

## Memory safety

- Every `malloc` / `realloc` has a matching `free`.
- Use the goto-bail cleanup pattern in functions with multiple allocations.
- Tokens and strings stored in arrays must be independent copies, never
  pointers into a shared buffer.

## Source layout

- Sources live under `src/`, headers co-located (`foo.h` / `foo.c`).
- Rules live in `src/rules/`. A new rule is implemented there, declared in
  `src/rules/rules_registry.c`, and added to the `CRP_RULES[]` array.
- A rule function has the signature:
  `crp_rule_result_t *fn(const char **tokens, size_t token_count, const char *error)`
  and returns `NULL` when it does not match.

## Building and testing

- Create the build dir first: `mkdir -p build`.
- Compile with `-I.` so `"src/..."` include paths resolve.
- After any change, rebuild and run all test suites. They must all pass:
  - `./build/test_shlex`
  - `./build/test_match`
  - `./build/test_cli`
- When adding a rule, add a matching test file under `tests/`.

## Safety

- Never write a rule that could silently execute a destructive command (for
  example `rm` without confirmation).

## Source notes

- Do not use `\uXXXX` escapes in C source; write raw characters instead.
- No dead code: remove anything unused rather than leaving it in place.
