/*
 * subscription_post_handler.c — NSSAIAvailabilityPost inbound handler.
 *
 * Security-gate-first, then request-envelope checks, body parse + callbackUri
 * validation (via the shared dispatcher URL-policy gate), then
 * SubscriptionStore::create dispatch and result→HTTP mapping. The store performs
 * the ONE call-driven initial-snapshot dispatch internally — this handler never
 * loops. Transport-agnostic — same path drives tests, no socket.
 */

#include "subscription_post_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* strcasecmp — case-insensitive content-type/scheme match. */

#include "cJSON.h"
#include "nf_problem_details_wrapper.h"
#include "notification_dispatcher.h"  /* shared callbackUri URL-policy gate. */
#include "oauth2_scope_check.h"
#include "problem_details.h"

/* JSON body media type for the subscription create payload. */
#define SUBSCRIPTION_POST_CONTENT_TYPE "application/json"

/* Resource path the Location header points the client back to. */
#define SUBSCRIPTION_RESOURCE_PREFIX "/nssai-availability/subscriptions/"

static int emit_problem(nssf_subscription_post_response_t *out,
                        problem_details_t *pd,
                        int fallback_status)
{
    out->location[0] = '\0';  /* error responses carry no Location header. */
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
                          nssf_subscription_post_response_t *out)
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

int nssf_subscription_post_handle(
    const nssf_subscription_post_request_t *req,
    const nssf_subscription_post_deps_t *deps,
    nssf_subscription_post_response_t *out)
{
    if (req == NULL || deps == NULL || out == NULL) {
        if (out != NULL) {
            return emit_problem(out, nf_problem_details_make_500(NULL, NULL), 500);
        }
        return 500;
    }
    out->status = 0;
    out->location[0] = '\0';
    out->content_type = NULL;
    out->body = NULL;

    if (deps->jwks_cache == NULL || deps->store == NULL) {
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
    if (!req->has_content_length) {
        return emit_problem(out, nf_problem_details_make_411(NULL, req->request_target), 411);
    }
    if (req->max_body_len > 0 && req->content_length > req->max_body_len) {
        return emit_problem(out, nf_problem_details_make_413(NULL, req->request_target), 413);
    }
    if (req->content_type == NULL ||
        strcasecmp(req->content_type, SUBSCRIPTION_POST_CONTENT_TYPE) != 0) {
        return emit_problem(out, nf_problem_details_make_415_json(NULL, req->request_target), 415);
    }

    /* 3. Parse the create document. */
    if (req->body == NULL) {
        return emit_problem(out,
                            nf_problem_details_make_400_invalid_body(NULL, req->request_target),
                            400);
    }
    cJSON *doc = cJSON_Parse(req->body);
    if (doc == NULL) {
        return emit_problem(out,
                            nf_problem_details_make_400_invalid_body(NULL, req->request_target),
                            400);
    }

    /* Extract the callbackUri and validate it with the SHARED dispatcher URL
     * policy gate (https-only / no userinfo / no fragment / well-formed) before
     * the row is persisted — a callbackUri that could never receive a delivery is
     * a bad request, not a stored row. */
    const cJSON *cb = cJSON_GetObjectItemCaseSensitive(doc, "callbackUri");
    const char *callback_uri =
        (cJSON_IsString(cb) && cb->valuestring != NULL) ? cb->valuestring : NULL;
    if (callback_uri == NULL ||
        !nssf_notification_dispatcher_callback_url_allowed(callback_uri)) {
        cJSON_Delete(doc);
        return emit_problem(out,
                            nf_problem_details_make_400_invalid_param(
                                "callbackUri must be a well-formed https URL with no "
                                "userinfo or fragment",
                                req->request_target),
                            400);
    }

    /* The whole create document is persisted as the subscription's JSONB filter
     * (the store owns the column shape); the borrowed subtree is copied by the
     * store. expiry defaults to the G-10 lifetime — no validityTime delta is
     * derived here (No Spec Semantic Rediscovery). */
    const cJSON *filter =
        cJSON_GetObjectItemCaseSensitive(doc, "filter");  /* explicit filter, or */
    if (filter == NULL) {
        filter = doc;                                      /* the whole document. */
    }

    /* Subscriber identity (DOCX-GAP-001 self-notification suppression) — read the
     * NssfEventSubscriptionCreateData amfId/amfSetId from the document ROOT,
     * ALWAYS, independent of whether an explicit filter subtree is persisted.
     * These are the suppression truth, separate from the matching filter; either
     * may be absent (legacy / null-identity subscriber → never suppressed). */
    const cJSON *amf_id_j = cJSON_GetObjectItemCaseSensitive(doc, "amfId");
    const char *amf_id =
        (cJSON_IsString(amf_id_j) && amf_id_j->valuestring != NULL)
            ? amf_id_j->valuestring
            : NULL;
    const cJSON *amf_set_id_j = cJSON_GetObjectItemCaseSensitive(doc, "amfSetId");
    const char *amf_set_id =
        (cJSON_IsString(amf_set_id_j) && amf_set_id_j->valuestring != NULL)
            ? amf_set_id_j->valuestring
            : NULL;

    /* 4. SubscriptionStore::create() — persists, then performs the ONE
     * call-driven initial-snapshot dispatch via its snapshot seam (NO loop). */
    char id[37] = {0};
    nssf_sub_result_e result =
        nssf_subscription_store_create(deps->store, callback_uri, filter,
                                       amf_id, amf_set_id,
                                       NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS, id);
    cJSON_Delete(doc);

    /* 5. Result → HTTP. */
    if (result != NSSF_SUB_OK) {
        return emit_problem(out, nf_problem_details_make_500(NULL, req->request_target), 500);
    }

    int n = snprintf(out->location, sizeof(out->location),
                     SUBSCRIPTION_RESOURCE_PREFIX "%s", id);
    if (n < 0 || (size_t)n >= sizeof(out->location)) {
        out->location[0] = '\0';
        return emit_problem(out, nf_problem_details_make_500(NULL, req->request_target), 500);
    }
    out->status = 201;
    out->content_type = NULL;  /* 201 carries the Location header, no body. */
    out->body = NULL;
    return out->status;
}

void nssf_subscription_post_response_free(
    nssf_subscription_post_response_t *out)
{
    if (out == NULL) {
        return;
    }
    free(out->body);
    out->body = NULL;
}
