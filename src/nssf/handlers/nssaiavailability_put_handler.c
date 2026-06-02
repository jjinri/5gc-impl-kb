/*
 * nssaiavailability_put_handler.c — NSSAIAvailabilityPut inbound handler.
 *
 * Security-gate-first, then request-envelope checks, body parse, and
 * AvailabilityEngine::replace dispatch, then result→HTTP mapping.
 * Transport-agnostic: a server-wiring step fills the request struct; the same
 * path drives tests without a socket.
 */

#include "nssaiavailability_put_handler.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* strncasecmp — case-insensitive "Bearer " scheme match. */

#include "cJSON.h"
#include "nf_problem_details_wrapper.h"
#include "oauth2_scope_check.h"
#include "problem_details.h"

/* JSON body media type for the NssaiAvailabilityInfo PUT payload. */
#define NSSAIAVAIL_PUT_CONTENT_TYPE "application/json"

/* Serialize a ProblemDetails into the response (application/problem+json). */
static int emit_problem(nssf_nssaiavailability_put_response_t *out,
                        problem_details_t *pd,
                        int fallback_status)
{
    if (pd == NULL) {
        /* OOM building the ProblemDetails — answer the status with empty body. */
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

/*
 * Security gate — runs before any business logic. Returns true when the request
 * is authenticated and scoped for NSSAIAvailability; on a deny writes the 401/403
 * ProblemDetails into `out` and returns false.
 */
static bool security_gate(const char *authorization,
                          const char *instance,
                          nssf_jwks_cache_t *jwks_cache,
                          nssf_nssaiavailability_put_response_t *out)
{
    if (authorization == NULL || authorization[0] == '\0') {
        emit_problem(out, nf_problem_details_make_401_token_missing(NULL, instance), 401);
        return false;
    }

    /* Strip the "Bearer " scheme prefix (case-insensitive) before validation. */
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

int nssf_nssaiavailability_put_handle(
    const nssf_nssaiavailability_put_request_t *req,
    const nssf_nssaiavailability_put_deps_t *deps,
    nssf_nssaiavailability_put_response_t *out)
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

    /* A server with no validator or engine cannot serve securely → 500. */
    if (deps->jwks_cache == NULL || deps->engine == NULL) {
        return emit_problem(out, nf_problem_details_make_500(NULL, req->request_target), 500);
    }

    /* 1. Security gate BEFORE business logic. */
    if (!security_gate(req->authorization, req->request_target, deps->jwks_cache, out)) {
        return out->status;
    }

    /* 2. Request-envelope checks. */
    if (req->max_uri_len > 0 && req->request_target != NULL &&
        strlen(req->request_target) > req->max_uri_len) {
        return emit_problem(out, nf_problem_details_make_414(NULL, req->request_target), 414);
    }
    /* A PUT body is required — no Content-Length where a body is required → 411. */
    if (!req->has_content_length) {
        return emit_problem(out, nf_problem_details_make_411(NULL, req->request_target), 411);
    }
    if (req->max_body_len > 0 && req->content_length > req->max_body_len) {
        return emit_problem(out, nf_problem_details_make_413(NULL, req->request_target), 413);
    }
    /* PUT body media type is application/json (NssaiAvailabilityInfo). */
    if (req->content_type == NULL ||
        strcasecmp(req->content_type, NSSAIAVAIL_PUT_CONTENT_TYPE) != 0) {
        return emit_problem(out, nf_problem_details_make_415_json(NULL, req->request_target), 415);
    }

    /* 3. Parse the body as NssaiAvailabilityInfo cJSON. */
    if (req->body == NULL) {
        return emit_problem(out,
                            nf_problem_details_make_400_invalid_body(NULL, req->request_target),
                            400);
    }
    cJSON *info = cJSON_Parse(req->body);
    if (info == NULL) {
        return emit_problem(out,
                            nf_problem_details_make_400_invalid_body(NULL, req->request_target),
                            400);
    }

    /* 4. AvailabilityEngine::replace(). The deferred change-event publish (the
     * "retry_queue enqueue" intent) fires inside the engine via its injected
     * seam — a no-op in phase2. */
    cJSON *authorized = NULL;
    char errbuf[256] = {0};
    nssf_availability_result_e result =
        nssf_availability_engine_replace(deps->engine, req->nf_id, info,
                                         &authorized, errbuf, sizeof(errbuf));
    cJSON_Delete(info);

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
    case NSSF_AVAIL_ERR_BAD_REQUEST:
        cJSON_Delete(authorized);
        return emit_problem(out,
                            nf_problem_details_make_400_invalid_body(errbuf[0] ? errbuf : NULL,
                                                                     req->request_target),
                            400);
    case NSSF_AVAIL_ERR_UNSUPPORTED_MEDIA_TYPE:
        cJSON_Delete(authorized);
        return emit_problem(out,
                            nf_problem_details_make_415_json(errbuf[0] ? errbuf : NULL,
                                                             req->request_target),
                            415);
    case NSSF_AVAIL_ERR_NOT_FOUND:
        cJSON_Delete(authorized);
        return emit_problem(out,
                            nf_problem_details_make_404_no_availability(errbuf[0] ? errbuf : NULL,
                                                                        req->request_target),
                            404);
    case NSSF_AVAIL_ERR_INTERNAL:
    default:
        cJSON_Delete(authorized);
        /* A repository/DB failure the client cannot fix → 503 (db unavailable). */
        return emit_problem(out,
                            nf_problem_details_make_503_db(errbuf[0] ? errbuf : NULL,
                                                           req->request_target),
                            503);
    }
}

void nssf_nssaiavailability_put_response_free(
    nssf_nssaiavailability_put_response_t *out)
{
    if (out == NULL) {
        return;
    }
    free(out->body);
    out->body = NULL;
}
