#ifndef RULES_H
#define RULES_H

#include <stddef.h>

/* Result returned by a rule — caller must free. */
typedef struct {
    char **suggestions;
    size_t count;
} crp_rule_result_t;

/* A rule function: takes tokenized command and error output, returns suggestions.
 * Returns NULL when the rule doesn't match.
 * The caller must free crp_rule_result_t via crp_result_free(). */
typedef crp_rule_result_t * (*crp_rule_fn_t)(const char **tokens, size_t token_count,
                                                const char *error);

/* Rule registry entry */
typedef struct {
    const char *name;
    crp_rule_fn_t fn;
} crp_rule_entry_t;

/* Free a rule result */
void crp_result_free(crp_rule_result_t *result);

/* Run all registered rules and return deduplicated, sorted suggestions.
 * Returns a char** array via malloc (caller frees each string then the array).
 * *out_count stores the number of suggestions.
 * Returns NULL on allocation failure. */
char **crp_find_fixes(const char *original_cmd, const char **tokens, size_t token_count,
                      const char *error_output, size_t *out_count);

/* All registered rules. */
extern const crp_rule_entry_t crp_rules[];
extern size_t crp_rules_count;

#endif /* RULES_H */
