/*
 * oauth2_scope_check.c — per-operation OAuth2 scope check for NSSF.
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten runtime module.
 *
 * Pure libc. The operation→scope table is the single source of truth for the
 * two NSSF service scopes; an unknown operation maps to NULL (deny by default).
 */

#define _POSIX_C_SOURCE 200809L

#include "oauth2_scope_check.h"

#include <stddef.h>
#include <string.h>

/*
 * Operation/API → scope table. Keys cover both the generated operationId style
 * and the service-name prefix so a caller can pass either. Substring of the
 * service maps to one of the two NSSF scopes.
 */
typedef struct {
    const char *key;
    const char *scope;
} scope_entry_t;

static const scope_entry_t SCOPE_TABLE[] = {
    /* Nnssf_NSSelection service. */
    {"NSSelectionGet", NSSF_SCOPE_NSSELECTION},
    {"Nnssf_NSSelection", NSSF_SCOPE_NSSELECTION},
    {"nnssf-nsselection", NSSF_SCOPE_NSSELECTION},
    /* Nnssf_NSSAIAvailability service. */
    {"NSSAIAvailabilityPut", NSSF_SCOPE_NSSAIAVAILABILITY},
    {"NSSAIAvailabilityPatch", NSSF_SCOPE_NSSAIAVAILABILITY},
    {"NSSAIAvailabilityDelete", NSSF_SCOPE_NSSAIAVAILABILITY},
    {"NSSAIAvailabilityUnsubscribe", NSSF_SCOPE_NSSAIAVAILABILITY},
    {"NSSAIAvailabilityPostSubscriptions", NSSF_SCOPE_NSSAIAVAILABILITY},
    {"NSSAIAvailabilitySubModifyPatch", NSSF_SCOPE_NSSAIAVAILABILITY},
    {"Nnssf_NSSAIAvailability", NSSF_SCOPE_NSSAIAVAILABILITY},
    {"nnssf-nssaiavailability", NSSF_SCOPE_NSSAIAVAILABILITY},
};

static const size_t SCOPE_TABLE_LEN =
    sizeof(SCOPE_TABLE) / sizeof(SCOPE_TABLE[0]);

bool nssf_oauth2_scope_match(const char *token_scope_claim,
                             const char *required_scope)
{
    if (token_scope_claim == NULL || required_scope == NULL) {
        return false;
    }

    size_t req_len = strlen(required_scope);
    if (req_len == 0) {
        return false;
    }

    /* Walk space-delimited tokens; require a whole-token match (no substring). */
    const char *cursor = token_scope_claim;
    while (*cursor != '\0') {
        while (*cursor == ' ') {
            cursor++;
        }
        const char *start = cursor;
        while (*cursor != '\0' && *cursor != ' ') {
            cursor++;
        }
        size_t tok_len = (size_t)(cursor - start);
        if (tok_len == req_len && memcmp(start, required_scope, req_len) == 0) {
            return true;
        }
    }
    return false;
}

const char *nssf_oauth2_scope_for_operation(const char *operation_id_or_api)
{
    if (operation_id_or_api == NULL) {
        return NULL;
    }

    /* Exact match first — deterministic for known operationIds/service names. */
    for (size_t i = 0; i < SCOPE_TABLE_LEN; i++) {
        if (strcmp(operation_id_or_api, SCOPE_TABLE[i].key) == 0) {
            return SCOPE_TABLE[i].scope;
        }
    }

    /*
     * Fallback: callers may pass a fuller path/operationId that contains a
     * known service token (e.g. an NSSAIAvailability variant we did not list
     * verbatim). Substring containment keeps the two-scope partition robust.
     */
    if (strstr(operation_id_or_api, "NSSAIAvailability") != NULL) {
        return NSSF_SCOPE_NSSAIAVAILABILITY;
    }
    if (strstr(operation_id_or_api, "NSSelection") != NULL) {
        return NSSF_SCOPE_NSSELECTION;
    }

    return NULL;
}
