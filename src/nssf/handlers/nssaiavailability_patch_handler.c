/*
 * nssaiavailability_patch_handler.c — NSSAIAvailabilityPatch inbound handler.
 *
 * Security-gate-first, then request-envelope checks, body+tai parse, and
 * AvailabilityEngine::patch dispatch, then result→HTTP mapping. The engine owns
 * the 415 content-type classification (G-04) so the handler forwards
 * content_type verbatim. Transport-agnostic — same path drives tests, no socket.
 */

#include "nssaiavailability_patch_handler.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* strncasecmp — case-insensitive "Bearer " scheme match. */

#include "cJSON.h"
#include "nf_problem_details_wrapper.h"
#include "oauth2_scope_check.h"
#include "problem_details.h"

static int emit_problem(nssf_nssaiavailability_patch_response_t *out,
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
                          nssf_nssaiavailability_patch_response_t *out)
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

int nssf_nssaiavailability_patch_handle(
    const nssf_nssaiavailability_patch_request_t *req,
    const nssf_nssaiavailability_patch_deps_t *deps,
    nssf_nssaiavailability_patch_response_t *out)
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

    if (deps->jwks_cache == NULL || deps->engine == NULL) {
        return emit_problem(out, nf_problem_details_make_500(NULL, req->request_target), 500);
    }

    /* 1. Security gate BEFORE business logic. */
    if (!security_gate(req->authorization, req->request_target, deps->jwks_cache, out)) {
        return out->status;
    }

    /* 2. Request-envelope checks. The 415 media-type decision is the engine's
     * (it classifies non-json-patch+json distinctly), so it is NOT pre-checked
     * here — only length/uri envelope checks the handler must perform. */
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

    /* 3. Parse the routing tai and the RFC 6902 patch array. A parse failure is
     * a malformed request → 400 (the engine would reject the same way). */
    cJSON *tai = (req->tai != NULL) ? cJSON_Parse(req->tai) : NULL;
    cJSON *patch = (req->body != NULL) ? cJSON_Parse(req->body) : NULL;

    /* 4. AvailabilityEngine::patch() — engine owns 415 (content-type),
     * 400 (op/tai validation), 404 (unknown nfId@tai). The deferred change-event
     * publish fires inside the engine (no-op in phase2). */
    cJSON *authorized = NULL;
    char errbuf[256] = {0};
    nssf_availability_result_e result =
        nssf_availability_engine_patch(deps->engine, req->nf_id, req->content_type,
                                       tai, patch, &authorized, errbuf, sizeof(errbuf));
    cJSON_Delete(tai);
    cJSON_Delete(patch);

    /* 5. Result → HTTP. */
    switch (result) {
    case NSSF_AVAIL_OK: {
        out->status = 200;
        out->content_type = NSSF_CT_JSON;
        out->body = cJSON_PrintUnformatted(authorized);
        cJSON_Delete(authorized);
        if (out->body == NULL) {
            return emit_problem(out, nf_problem_details_make_500(NULL, req->request_target), 500);
        }
        return out->status;
    }
    case NSSF_AVAIL_NO_CONTENT:
        cJSON_Delete(authorized);
        out->status = 204;
        out->content_type = NSSF_CT_JSON;
        out->body = NULL;
        return out->status;
    case NSSF_AVAIL_ERR_UNSUPPORTED_MEDIA_TYPE:
        cJSON_Delete(authorized);
        return emit_problem(out,
                            nf_problem_details_make_415_json_patch(errbuf[0] ? errbuf : NULL,
                                                                   req->request_target),
                            415);
    case NSSF_AVAIL_ERR_NOT_FOUND:
        cJSON_Delete(authorized);
        return emit_problem(out,
                            nf_problem_details_make_404_no_availability(errbuf[0] ? errbuf : NULL,
                                                                        req->request_target),
                            404);
    case NSSF_AVAIL_ERR_BAD_REQUEST:
        cJSON_Delete(authorized);
        return emit_problem(out,
                            nf_problem_details_make_400_invalid_body(errbuf[0] ? errbuf : NULL,
                                                                     req->request_target),
                            400);
    case NSSF_AVAIL_ERR_INTERNAL:
    default:
        cJSON_Delete(authorized);
        return emit_problem(out,
                            nf_problem_details_make_503_db(errbuf[0] ? errbuf : NULL,
                                                           req->request_target),
                            503);
    }
}

void nssf_nssaiavailability_patch_response_free(
    nssf_nssaiavailability_patch_response_t *out)
{
    if (out == NULL) {
        return;
    }
    free(out->body);
    out->body = NULL;
}
