/*
 * nf_problem_details_wrapper.c — error-cause-catalog.yaml 의 18 production
 * cause metadata 테이블 + factory 구현. Pane 2 권고 "generated code 직접
 * 수정 금지" 준수 — wrapper layer 가 generated problem_details_t 만 채운다.
 */

#define _POSIX_C_SOURCE 200809L

#include "nf_problem_details_wrapper.h"

#include <stdlib.h>
#include <string.h>

/* ---------- static cause metadata table ------------------------------- */

static const char *const HDR_WWW_AUTH[] = {
    "WWW-Authenticate: Bearer realm=\"nssf\", error=\"invalid_token\"",
};
static const char *const HDR_ALLOW[] = {
    "Allow: {allowed methods}",
};
static const char *const HDR_RETRY_AFTER[] = {
    "Retry-After: {seconds}",
};
static const char *const HDR_ACCEPT_PATCH[] = {
    "Accept-Patch: application/json-patch+json",
};

#define HDR_LIST(arr) (arr), (sizeof(arr) / sizeof((arr)[0]))
#define HDR_NONE      NULL, 0

static const nssf_pd_metadata_t TABLE[NSSF_PD_CAUSE_COUNT] = {
    [NSSF_PD_BAD_REQUEST_INVALID_BODY] = {
        NSSF_PD_BAD_REQUEST_INVALID_BODY, 400, "Bad Request",
        "urn:5gc:problem:bad-request",
        "Request body invalid: {reason}",
        HDR_NONE,
    },
    [NSSF_PD_BAD_REQUEST_INVALID_PARAM] = {
        NSSF_PD_BAD_REQUEST_INVALID_PARAM, 400, "Bad Request",
        "urn:5gc:problem:bad-request",
        "Query parameter invalid: {param}",
        HDR_NONE,
    },
    [NSSF_PD_UNAUTHORIZED_TOKEN_MISSING] = {
        NSSF_PD_UNAUTHORIZED_TOKEN_MISSING, 401, "Unauthorized",
        "urn:5gc:problem:unauthorized",
        "OAuth2 bearer token missing",
        HDR_LIST(HDR_WWW_AUTH),
    },
    [NSSF_PD_UNAUTHORIZED_TOKEN_INVALID] = {
        NSSF_PD_UNAUTHORIZED_TOKEN_INVALID, 401, "Unauthorized",
        "urn:5gc:problem:unauthorized",
        "OAuth2 bearer token invalid: {reason}",
        HDR_LIST(HDR_WWW_AUTH),
    },
    [NSSF_PD_FORBIDDEN_SCOPE_MISMATCH] = {
        NSSF_PD_FORBIDDEN_SCOPE_MISMATCH, 403, "Forbidden",
        "urn:5gc:problem:forbidden",
        "OAuth2 scope mismatch: required={required} actual={actual}",
        HDR_NONE,
    },
    [NSSF_PD_NOT_FOUND_NO_AVAILABILITY] = {
        NSSF_PD_NOT_FOUND_NO_AVAILABILITY, 404, "Not Found",
        "urn:5gc:problem:not-found",
        "No availability for plmn={plmn} tai={tai}",
        HDR_NONE,
    },
    [NSSF_PD_NOT_FOUND_SUBSCRIPTION] = {
        NSSF_PD_NOT_FOUND_SUBSCRIPTION, 404, "Not Found",
        "urn:5gc:problem:not-found",
        "Subscription not found: id={id}",
        HDR_NONE,
    },
    [NSSF_PD_METHOD_NOT_ALLOWED] = {
        NSSF_PD_METHOD_NOT_ALLOWED, 405, "Method Not Allowed",
        "urn:5gc:problem:method-not-allowed",
        "Method {method} not allowed on {uri}",
        HDR_LIST(HDR_ALLOW),
    },
    [NSSF_PD_REQUEST_TIMEOUT] = {
        NSSF_PD_REQUEST_TIMEOUT, 408, "Request Timeout",
        "urn:5gc:problem:request-timeout",
        "Request did not complete within {timeout}s",
        HDR_NONE,
    },
    [NSSF_PD_PAYLOAD_LENGTH_REQUIRED] = {
        NSSF_PD_PAYLOAD_LENGTH_REQUIRED, 411, "Length Required",
        "urn:5gc:problem:length-required",
        "Content-Length header required",
        HDR_NONE,
    },
    [NSSF_PD_PAYLOAD_TOO_LARGE] = {
        NSSF_PD_PAYLOAD_TOO_LARGE, 413, "Payload Too Large",
        "urn:5gc:problem:payload-too-large",
        "Request body exceeds {max} bytes",
        HDR_NONE,
    },
    [NSSF_PD_URI_TOO_LONG] = {
        NSSF_PD_URI_TOO_LONG, 414, "URI Too Long",
        "urn:5gc:problem:uri-too-long",
        "Request URI exceeds {max} bytes",
        HDR_NONE,
    },
    [NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON] = {
        NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON, 415, "Unsupported Media Type",
        "urn:5gc:problem:unsupported-media-type",
        "Content-Type must be application/json (got: {actual})",
        HDR_NONE,
    },
    [NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON_PATCH] = {
        NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON_PATCH, 415, "Unsupported Media Type",
        "urn:5gc:problem:unsupported-media-type",
        "Content-Type must be application/json-patch+json (got: {actual})",
        HDR_LIST(HDR_ACCEPT_PATCH),
    },
    [NSSF_PD_TOO_MANY_REQUESTS] = {
        NSSF_PD_TOO_MANY_REQUESTS, 429, "Too Many Requests",
        "urn:5gc:problem:too-many-requests",
        "Rate limit exceeded",
        HDR_LIST(HDR_RETRY_AFTER),
    },
    [NSSF_PD_SERVICE_UNAVAILABLE_DB] = {
        NSSF_PD_SERVICE_UNAVAILABLE_DB, 503, "Service Unavailable",
        "urn:5gc:problem:service-unavailable",
        "Database backend unavailable: {reason}",
        HDR_LIST(HDR_RETRY_AFTER),
    },
    [NSSF_PD_SERVICE_UNAVAILABLE_UPSTREAM] = {
        NSSF_PD_SERVICE_UNAVAILABLE_UPSTREAM, 503, "Service Unavailable",
        "urn:5gc:problem:service-unavailable",
        "Upstream NF unavailable: {nf}",
        HDR_LIST(HDR_RETRY_AFTER),
    },
    [NSSF_PD_INTERNAL_SERVER_ERROR] = {
        NSSF_PD_INTERNAL_SERVER_ERROR, 500, "Internal Server Error",
        "urn:5gc:problem:internal-server-error",
        "Internal error: {request_id}",
        HDR_NONE,
    },
};

static const char *const NAMES[NSSF_PD_CAUSE_COUNT] = {
    [NSSF_PD_BAD_REQUEST_INVALID_BODY]          = "BAD_REQUEST_INVALID_BODY",
    [NSSF_PD_BAD_REQUEST_INVALID_PARAM]         = "BAD_REQUEST_INVALID_PARAM",
    [NSSF_PD_UNAUTHORIZED_TOKEN_MISSING]        = "UNAUTHORIZED_TOKEN_MISSING",
    [NSSF_PD_UNAUTHORIZED_TOKEN_INVALID]        = "UNAUTHORIZED_TOKEN_INVALID",
    [NSSF_PD_FORBIDDEN_SCOPE_MISMATCH]          = "FORBIDDEN_SCOPE_MISMATCH",
    [NSSF_PD_NOT_FOUND_NO_AVAILABILITY]         = "NOT_FOUND_NO_AVAILABILITY",
    [NSSF_PD_NOT_FOUND_SUBSCRIPTION]            = "NOT_FOUND_SUBSCRIPTION",
    [NSSF_PD_METHOD_NOT_ALLOWED]                = "METHOD_NOT_ALLOWED",
    [NSSF_PD_REQUEST_TIMEOUT]                   = "REQUEST_TIMEOUT",
    [NSSF_PD_PAYLOAD_LENGTH_REQUIRED]           = "PAYLOAD_LENGTH_REQUIRED",
    [NSSF_PD_PAYLOAD_TOO_LARGE]                 = "PAYLOAD_TOO_LARGE",
    [NSSF_PD_URI_TOO_LONG]                      = "URI_TOO_LONG",
    [NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON]       = "UNSUPPORTED_MEDIA_TYPE_JSON",
    [NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON_PATCH] = "UNSUPPORTED_MEDIA_TYPE_JSON_PATCH",
    [NSSF_PD_TOO_MANY_REQUESTS]                 = "TOO_MANY_REQUESTS",
    [NSSF_PD_SERVICE_UNAVAILABLE_DB]            = "SERVICE_UNAVAILABLE_DB",
    [NSSF_PD_SERVICE_UNAVAILABLE_UPSTREAM]      = "SERVICE_UNAVAILABLE_UPSTREAM",
    [NSSF_PD_INTERNAL_SERVER_ERROR]             = "INTERNAL_SERVER_ERROR",
};

/* ---------- accessors ------------------------------------------------- */

const nssf_pd_metadata_t *nssf_pd_metadata_for(nssf_pd_cause_e cause)
{
    if ((int)cause < 0 || cause >= NSSF_PD_CAUSE_COUNT) {
        return NULL;
    }
    return &TABLE[cause];
}

const char *nssf_pd_cause_name(nssf_pd_cause_e cause)
{
    if ((int)cause < 0 || cause >= NSSF_PD_CAUSE_COUNT) {
        return NULL;
    }
    return NAMES[cause];
}

/* ---------- generic factory ------------------------------------------- */

problem_details_t *nf_problem_details_make_by_cause(nssf_pd_cause_e cause,
                                                    const char *detail,
                                                    const char *instance)
{
    const nssf_pd_metadata_t *meta = nssf_pd_metadata_for(cause);
    if (meta == NULL) {
        return NULL;
    }

    /*
     * 3GPP TS 29.500 §5.2.7.2 의 application error cause — catalog id 의
     * UPPERCASE_UNDERSCORE 표기 (e.g. "NOT_FOUND_NO_AVAILABILITY") 를 그대로
     * 3GPP cause 로 사용한다. NAMES[] 가 valid enum 에 대해 non-NULL 보장.
     */
    const char *cause_str = NAMES[cause];
    const char *detail_src = (detail != NULL) ? detail : meta->detail_template;

    char *type_dup = strdup(meta->type_urn);
    char *title_dup = strdup(meta->title);
    char *detail_dup = strdup(detail_src);
    char *cause_dup = strdup(cause_str);
    char *instance_dup = NULL;
    if (instance != NULL) {
        instance_dup = strdup(instance);
    }

    int oom = (type_dup == NULL || title_dup == NULL || detail_dup == NULL ||
               cause_dup == NULL || (instance != NULL && instance_dup == NULL));
    if (oom) {
        free(type_dup);
        free(title_dup);
        free(detail_dup);
        free(cause_dup);
        free(instance_dup);
        return NULL;
    }

    /*
     * problem_details_create() takes ownership of all passed pointers and
     * frees them via problem_details_free(). Pass NULL for optional
     * NRF/OAuth fields — caller can mutate the returned struct if needed
     * (e.g. invalid_params for 400).
     */
    problem_details_t *pd = problem_details_create(
        type_dup,
        title_dup,
        meta->status,
        detail_dup,
        instance_dup,
        cause_dup,
        NULL,   /* invalid_params */
        NULL,   /* supported_features */
        NULL,   /* access_token_error */
        NULL,   /* access_token_request */
        NULL,   /* nrf_id */
        NULL,   /* supported_api_versions */
        NULL    /* no_profile_match_info */
    );

    if (pd == NULL) {
        free(type_dup);
        free(title_dup);
        free(detail_dup);
        free(cause_dup);
        free(instance_dup);
        return NULL;
    }
    return pd;
}

/* ---------- variant factories ----------------------------------------- */

#define DEFINE_FACTORY(suffix, cause)                                    \
    problem_details_t *nf_problem_details_make_##suffix(const char *detail,         \
                                             const char *instance)       \
    {                                                                    \
        return nf_problem_details_make_by_cause((cause), detail, instance); \
    }

DEFINE_FACTORY(400_invalid_body,    NSSF_PD_BAD_REQUEST_INVALID_BODY)
DEFINE_FACTORY(400_invalid_param,   NSSF_PD_BAD_REQUEST_INVALID_PARAM)
DEFINE_FACTORY(401_token_missing,   NSSF_PD_UNAUTHORIZED_TOKEN_MISSING)
DEFINE_FACTORY(401_token_invalid,   NSSF_PD_UNAUTHORIZED_TOKEN_INVALID)
DEFINE_FACTORY(403,                 NSSF_PD_FORBIDDEN_SCOPE_MISMATCH)
DEFINE_FACTORY(404_no_availability, NSSF_PD_NOT_FOUND_NO_AVAILABILITY)
DEFINE_FACTORY(404_subscription,    NSSF_PD_NOT_FOUND_SUBSCRIPTION)
DEFINE_FACTORY(405,                 NSSF_PD_METHOD_NOT_ALLOWED)
DEFINE_FACTORY(408,                 NSSF_PD_REQUEST_TIMEOUT)
DEFINE_FACTORY(411,                 NSSF_PD_PAYLOAD_LENGTH_REQUIRED)
DEFINE_FACTORY(413,                 NSSF_PD_PAYLOAD_TOO_LARGE)
DEFINE_FACTORY(414,                 NSSF_PD_URI_TOO_LONG)
DEFINE_FACTORY(415_json,            NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON)
DEFINE_FACTORY(415_json_patch,      NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON_PATCH)
DEFINE_FACTORY(429,                 NSSF_PD_TOO_MANY_REQUESTS)
DEFINE_FACTORY(500,                 NSSF_PD_INTERNAL_SERVER_ERROR)
DEFINE_FACTORY(503_db,              NSSF_PD_SERVICE_UNAVAILABLE_DB)
DEFINE_FACTORY(503_upstream,        NSSF_PD_SERVICE_UNAVAILABLE_UPSTREAM)
