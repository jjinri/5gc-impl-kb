/*
 * oauth2_scope_check.h — per-operation OAuth2 scope check for NSSF.
 *
 * Pure, table-driven, libc-only. Maps NSSF API operations to the two required
 * NSSF scopes and tests a space-delimited OAuth2 scope claim for membership.
 */

#ifndef NSSF_RUNTIME_OAUTH2_SCOPE_CHECK_H
#define NSSF_RUNTIME_OAUTH2_SCOPE_CHECK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The two NSSF OAuth2 scopes (authoritative per config contract). */
#define NSSF_SCOPE_NSSELECTION        "nnssf-nsselection"
#define NSSF_SCOPE_NSSAIAVAILABILITY  "nnssf-nssaiavailability"

/*
 * True iff required_scope appears as a whole, space-delimited token within
 * token_scope_claim. NULL inputs and empty required_scope yield false.
 */
bool nssf_oauth2_scope_match(const char *token_scope_claim,
                             const char *required_scope);

/*
 * Map an NSSF operation id or API name to its required scope. Returns a static
 * string (NSSF_SCOPE_*) or NULL when the operation is unknown — caller treats
 * NULL as a deny.
 */
const char *nssf_oauth2_scope_for_operation(const char *operation_id_or_api);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_RUNTIME_OAUTH2_SCOPE_CHECK_H */
