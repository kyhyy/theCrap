#include "rules.h"

/* Forward declarations for each rule */
crp_rule_result_t *crp_sudo(const char **tokens, size_t token_count, const char *error);
crp_rule_result_t *crp_git_wrong_command(const char **tokens, size_t token_count, const char *error);
crp_rule_result_t *crp_mkdir_missing_parent(const char **tokens, size_t token_count, const char *error);
crp_rule_result_t *crp_rm_dir(const char **tokens, size_t token_count, const char *error);
crp_rule_result_t *crp_cp_dir(const char **tokens, size_t token_count, const char *error);
crp_rule_result_t *crp_command_not_found(const char **tokens, size_t token_count, const char *error);

/* Registry of all rules — order doesn't matter (dedup + sort handles it) */
const crp_rule_entry_t crp_rules[] = {
    { "sudo",               crp_sudo },
    { "git_wrong_command",  crp_git_wrong_command },
    { "mkdir_missing_parent", crp_mkdir_missing_parent },
    { "rm_dir",             crp_rm_dir },
    { "cp_dir",             crp_cp_dir },
    { "command_not_found", crp_command_not_found },
};

size_t crp_rules_count = sizeof(crp_rules) / sizeof(crp_rules[0]);
