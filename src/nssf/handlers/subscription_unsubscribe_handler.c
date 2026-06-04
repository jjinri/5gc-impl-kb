/*
 * subscription_unsubscribe_handler.c — NSSAIAvailabilityUnsubscribe inbound
 * handler.
 *
 * Security-gate-first, then SubscriptionStore::delete dispatch, then result→HTTP
 * mapping. No request body. The store delete is idempotent — a missing row is
 * success, not 404. Transport-agnostic — same path drives tests, no socket.
 */

#include "subscription_unsubscribe_handler.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* strncasecmp — case-insensitive "Bearer " scheme match. */

#include "cJSON.h"
#include "nf_problem_details_wrapper.h"
#include "oauth2_scope_check.h"
#include "problem_details.h"

static int emit_problem(nssf_subscription_unsubscribe_response_t *out,
                        problem_details_t *pd,
                        int fallback_status)
{
    if (pd == NULL) {
        out->status = fallback_status;
        out->content_type = NSSF_CT_PROBLEM_JSON;
        out->body = NULL;
        return out->status;
    }
    out->status = pd->status;
    out->content_type = NSSF_CT_PROBLEM_JSON;
    cJSON *json = problem_details_convertToJSON(pd);
    out->body = (json != NULL) ? cJSON_PrintUnformatted(json) : NULL;
    cJSON_Delete(json);
    problem_details_free(pd);
    return out->status;
}

static bool security_gate(const char *authorization,
                          const char *instance,
                          nssf_jwks_cache_t *jwks_cache,
                          nssf_subscription_unsubscribe_response_t *out)
{
    if (authorization == NULL || authorization[0] == '\0') {
        emit_problem(out, nf_problem_details_make_401_token_missing(NULL, instance), 401);
        return false;
    }

    const char *bearer = authorization;
    const char prefix[] = "Bearer ";
    if (strncasecmp(authorization, prefix, sizeof(prefix) - 1) == 0) {
        bearer = authorization + (sizeof(prefix) - 1);
    } else {
        emit_problem(out, nf_problem_details_make_401_token_missing(NULL, instance), 401);
        return false;
    }

    char *scope_claim = NULL;
    char errbuf[256] = {0};
    if (nssf_oauth2_validate_bearer(jwks_cache, bearer, &scope_claim,
                                    errbuf, sizeof(errbuf)) != 0) {
        free(scope_claim);
        emit_problem(out,
                     nf_problem_details_make_401_token_invalid(errbuf[0] ? errbuf : NULL,
                                                               instance),
                     401);
        return false;
    }

    bool scoped = nssf_oauth2_scope_match(scope_claim, NSSF_SCOPE_NSSAIAVAILABILITY);
    free(scope_claim);
    if (!scoped) {
        emit_problem(out,
                     nf_problem_details_make_403("required scope "
                                                 NSSF_SCOPE_NSSAIAVAILABILITY
                                                 " not present in access token",
                                                 instance),
                     403);
        return false;
    }
    return true;
}

int nssf_subscription_unsubscribe_handle(
    const nssf_subscription_unsubscribe_request_t *req,
    const nssf_subscription_unsubscribe_deps_t *deps,
    nssf_subscription_unsubscribe_response_t *out)
{
    if (req == NULL || deps == NULL || out == NULL) {
        if (out != NULL) {
            return emit_problem(out, nf_problem_details_make_500(NULL, NULL), 500);
        }
        return 500;
    }
    out->status = 0;
    out->content_type = NULL;
    out->body = NULL;

    if (deps->jwks_cache == NULL || deps->store == NULL) {
        return emit_problem(out, nf_problem_details_make_500(NULL, req->request_target), 500);
    }

    /* 1. Security gate BEFORE business logic. */
    if (!security_gate(req->authorization, req->request_target, deps->jwks_cache, out)) {
        return out->status;
    }

    /* 2. Request-target length cap → 414. */
    if (req->max_uri_len > 0 && req->request_target != NULL &&
        strlen(req->request_target) > req->max_uri_len) {
        return emit_problem(out, nf_problem_details_make_414(NULL, req->request_target), 414);
    }

    /* 3. SubscriptionStore::delete() — idempotent (absent row is success). */
    nssf_sub_result_e result =
        nssf_subscription_store_delete(deps->store, req->subscription_id);

    /* 4. Result → HTTP. A successful or no-op delete is 204. */
    if (result == NSSF_SUB_OK) {
        out->status = 204;
        out->content_type = NSSF_CT_JSON;
        out->body = NULL;
        return out->status;
    }
    return emit_problem(out, nf_problem_details_make_500(NULL, req->request_target), 500);
}

void nssf_subscription_unsubscribe_response_free(
    nssf_subscription_unsubscribe_response_t *out)
{
    if (out == NULL) {
        return;
    }
    free(out->body);
    out->body = NULL;
}
