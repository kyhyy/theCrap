#include "rules.h"
#include "../match.h"

#include <stdlib.h>
#include <string.h>

/* Simple insertion sort */
static void sort_results(crp_rule_result_t **results, size_t *counts, size_t total,
                         const char *original_cmd)
{
    for (size_t i = 1; i < total; i++) {
        size_t j = i;
        double key_sim = (double)damerau_levenshtein(original_cmd, results[i]->suggestions[0])
                         / (double)strlen(original_cmd);
        crp_rule_result_t *key_res = results[i];
        size_t key_cnt = counts[i];

        while (j > 0) {
            double prev_sim = (double)damerau_levenshtein(original_cmd, results[j-1]->suggestions[0])
                              / (double)strlen(original_cmd);
            if (prev_sim <= key_sim)
                break;
            results[j] = results[j - 1];
            counts[j] = counts[j - 1];
            j--;
        }
        results[j] = key_res;
        counts[j] = key_cnt;
    }
}

/* Deduplicate suggestions: two suggestions are duplicates if their strings match */
static size_t dedup_results(crp_rule_result_t **results, size_t *counts, size_t total)
{
    if (total == 0) return 0;

    size_t out = 0;
    for (size_t i = 0; i < total; i++) {
        int is_dup = 0;
        for (size_t j = 0; j < out; j++) {
            if (strcmp(results[i]->suggestions[0], results[j]->suggestions[0]) == 0) {
                is_dup = 1;
                break;
            }
        }
        if (!is_dup) {
            if (out != i) {
                results[out] = results[i];
                counts[out] = counts[i];
            }
            out++;
        }
    }
    return out;
}

/* Collect all suggestions from an array of results into a flat array. */
static char **collect_all(crp_rule_result_t **results, size_t *counts, size_t total, size_t *total_count)
{
    size_t total_suggestions = 0;
    for (size_t i = 0; i < total; i++)
        total_suggestions += counts[i];

    char **all = calloc(total_suggestions + 1, sizeof *all);
    if (!all) return NULL;

    size_t idx = 0;
    for (size_t i = 0; i < total; i++) {
        for (size_t j = 0; j < counts[i]; j++) {
            all[idx++] = results[i]->suggestions[j];
        }
        free(results[i]);
    }
    free(results);
    *total_count = idx;
    return all;
}

void crp_result_free(crp_rule_result_t *result)
{
    if (!result) return;
    if (result->suggestions) {
        for (size_t i = 0; i < result->count; i++)
            free(result->suggestions[i]);
        free(result->suggestions);
    }
    free(result);
}

char **crp_find_fixes(const char *original_cmd, const char **tokens, size_t token_count,
                      const char *error_output, size_t *out_count)
{
    if (!original_cmd || !error_output || !out_count) return NULL;

    *out_count = 0;

    /* Collect results from all rules */
    size_t max_rules = crp_rules_count;
    crp_rule_result_t **all_results = calloc(max_rules, sizeof *all_results);
    size_t *all_counts = calloc(max_rules, sizeof *all_counts);
    if (!all_results || !all_counts) {
        free(all_results);
        free(all_counts);
        return NULL;
    }
    size_t result_count = 0;

    for (size_t i = 0; i < max_rules; i++) {
        crp_rule_result_t *res = crp_rules[i].fn(tokens, token_count, error_output);
        if (!res) continue;
        if (res->count == 0) {
            free(res);
            continue;
        }
        all_results[result_count] = res;
        all_counts[result_count] = res->count;
        result_count++;
    }

    /* Deduplicate */
    result_count = dedup_results(all_results, all_counts, result_count);

    /* Sort by similarity to original command */
    if (result_count > 1)
        sort_results(all_results, all_counts, result_count, original_cmd);

    /* Collect all suggestions into flat array */
    char **result = collect_all(all_results, all_counts, result_count, out_count);
    free(all_counts);

    return result;
}
