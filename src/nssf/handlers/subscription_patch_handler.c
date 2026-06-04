/*
 * subscription_patch_handler.c — NSSAIAvailabilitySubModifyPatch inbound handler.
 *
 * Security-gate-first, then request-envelope checks, body parse, and
 * SubscriptionStore::patch dispatch, then result→HTTP mapping. The store owns the
 * 415 content-type classification (it rejects non-json-patch+json before the body
 * is touched), so the handler forwards content_type verbatim. Transport-agnostic
 * — same path drives tests, no socket.
 */

#include "subscription_patch_handler.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* strncasecmp — case-insensitive "Bearer " scheme match. */

#include "cJSON.h"
#include "nf_problem_details_wrapper.h"
#include "oauth2_scope_check.h"
#include "problem_details.h"

static int emit_problem(nssf_subscription_patch_response_t *out,
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
                          nssf_subscription_patch_response_t *out)
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

int nssf_subscription_patch_handle(
    const nssf_subscription_patch_request_t *req,
    const nssf_subscription_patch_deps_t *deps,
    nssf_subscription_patch_response_t *out)
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

    /* 2. Request-envelope checks. The 415 media-type decision is the store's (it
     * rejects non-json-patch+json before touching the body), so it is NOT
     * pre-checked here — only the length/uri envelope checks. */
    if (req->max_uri_len > 0 && req->request_target != NULL &&
        strlen(req->request_target) > req->max_uri_len) {
        return emit_problem(out, nf_problem_details_make_414(NULL, req->request_target), 414);
    }
    if (!req->has_content_length) {
        return emit_problem(out, nf_problem_details_make_411(NULL, req->request_target), 411);
    }
    if (req->max_body_len > 0 && req->content_length > req->max_body_len) {
        return emit_problem(out, nf_problem_details_make_413(NULL, req->request_target), 413);
    }

    /* 3. Parse the RFC 6902 patch array. A parse failure is a malformed request
     * → 400 (the store would reject it as a bad patch the same way). */
    cJSON *patch = (req->body != NULL) ? cJSON_Parse(req->body) : NULL;

    /* 4. SubscriptionStore::patch() — store owns 415 (content-type, checked
     * before the body), 400 (invalid op/path), 404 (absent id), atomic apply. */
    nssf_sub_result_e result =
        nssf_subscription_store_patch(deps->store, req->subscription_id,
                                      req->content_type, patch);
    cJSON_Delete(patch);

    /* 5. Result → HTTP. */
    switch (result) {
    case NSSF_SUB_OK:
        out->status = 200;
        out->content_type = NSSF_CT_JSON;
        out->body = NULL;
        return out->status;
    case NSSF_SUB_UNSUPPORTED:
        return emit_problem(out,
                            nf_problem_details_make_415_json_patch(NULL, req->request_target),
                            415);
    case NSSF_SUB_NOT_FOUND:
        return emit_problem(out,
                            nf_problem_details_make_404_subscription(NULL, req->request_target),
                            404);
    case NSSF_SUB_BAD_PATCH:
        return emit_problem(out,
                            nf_problem_details_make_400_invalid_body(NULL, req->request_target),
                            400);
    case NSSF_SUB_ERROR:
    default:
        return emit_problem(out, nf_problem_details_make_500(NULL, req->request_target), 500);
    }
}

void nssf_subscription_patch_response_free(
    nssf_subscription_patch_response_t *out)
{
    if (out == NULL) {
        return;
    }
    free(out->body);
    out->body = NULL;
}
