/*
 * nf_problem_details_wrapper.h — NSSF 의 RFC 7807 ProblemDetails factory.
 *
 * 본 wrapper 는 dev/nssf/error-cause-catalog.yaml 의 18 production cause 와
 * 1:1 매칭되는 factory 함수를 노출한다. 각 factory 는 generated
 * problem_details_t 인스턴스를 채우고 caller 가 problem_details_free() 로
 * 해제한다. TRACER_BULLET_STUB (501, wave 0 only) 은 본 wrapper 의 책임 외 —
 * Phase 1 wave 1 종료 후 nsselection_handler 가 실제 응답으로 대체할 때까지
 * src/nssf/transport/nsselection_handler.c 가 hardcoded 501 body 를 유지한다.
 *
 * Memory ownership.
 *   - factory 가 반환한 problem_details_t* 는 caller 가 problem_details_free()
 *     로 해제. 내부 문자열 (type/title/detail/instance) 은 strdup 으로 복제.
 *   - mandatory_headers 는 정적 const char* 배열 — caller 가 복제 후 송신.
 */

#ifndef NSSF_NF_PROBLEM_DETAILS_WRAPPER_H_
#define NSSF_NF_PROBLEM_DETAILS_WRAPPER_H_

#include <stddef.h>

#include "problem_details.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 18 production cause — error-cause-catalog.yaml 와 1:1. */
typedef enum {
    NSSF_PD_BAD_REQUEST_INVALID_BODY = 0,
    NSSF_PD_BAD_REQUEST_INVALID_PARAM,
    NSSF_PD_UNAUTHORIZED_TOKEN_MISSING,
    NSSF_PD_UNAUTHORIZED_TOKEN_INVALID,
    NSSF_PD_FORBIDDEN_SCOPE_MISMATCH,
    NSSF_PD_NOT_FOUND_NO_AVAILABILITY,
    NSSF_PD_NOT_FOUND_SUBSCRIPTION,
    NSSF_PD_METHOD_NOT_ALLOWED,
    NSSF_PD_REQUEST_TIMEOUT,
    NSSF_PD_PAYLOAD_LENGTH_REQUIRED,
    NSSF_PD_PAYLOAD_TOO_LARGE,
    NSSF_PD_URI_TOO_LONG,
    NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON,
    NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON_PATCH,
    NSSF_PD_TOO_MANY_REQUESTS,
    NSSF_PD_SERVICE_UNAVAILABLE_DB,
    NSSF_PD_SERVICE_UNAVAILABLE_UPSTREAM,
    NSSF_PD_INTERNAL_SERVER_ERROR,
    NSSF_PD_CAUSE_COUNT  /* sentinel — 18 */
} nssf_pd_cause_e;

/* metadata view — catalog 의 readonly snapshot. */
typedef struct {
    nssf_pd_cause_e cause;
    int status;
    const char *title;
    const char *type_urn;
    const char *detail_template;
    const char *const *mandatory_headers;  /* NULL-terminated. */
    size_t mandatory_header_count;
} nssf_pd_metadata_t;

/* metadata accessors. */
const nssf_pd_metadata_t *nssf_pd_metadata_for(nssf_pd_cause_e cause);
const char *nssf_pd_cause_name(nssf_pd_cause_e cause);

/*
 * Generic factory.
 *   detail : NULL 이면 catalog detail_template 그대로 사용. 비-NULL 이면 detail
 *            인자 그대로 본문에 기록 (caller 가 {placeholder} 치환 후 전달).
 *   instance : NULL 가능 (RFC 7807 optional).
 *   반환 : problem_details_t* (caller 가 problem_details_free 호출).
 *          OOM 시 NULL.
 */
problem_details_t *nf_problem_details_make_by_cause(nssf_pd_cause_e cause,
                                                    const char *detail,
                                                    const char *instance);

/*
 * Status-code/cause variant 별 편의 factory — error-cause-catalog.yaml
 * factory_naming_convention 준수. catalog 의 cause variant 가 2+ 인 status
 * (400, 401, 404, 415, 503) 는 short suffix 부착, 단일 cause 인 status 는
 * suffix 생략.
 */
problem_details_t *nf_problem_details_make_400_invalid_body(const char *detail,
                                                 const char *instance);
problem_details_t *nf_problem_details_make_400_invalid_param(const char *detail,
                                                  const char *instance);
problem_details_t *nf_problem_details_make_401_token_missing(const char *detail,
                                                  const char *instance);
problem_details_t *nf_problem_details_make_401_token_invalid(const char *detail,
                                                  const char *instance);
problem_details_t *nf_problem_details_make_403(const char *detail, const char *instance);
problem_details_t *nf_problem_details_make_404_no_availability(const char *detail,
                                                    const char *instance);
problem_details_t *nf_problem_details_make_404_subscription(const char *detail,
                                                 const char *instance);
problem_details_t *nf_problem_details_make_405(const char *detail, const char *instance);
problem_details_t *nf_problem_details_make_408(const char *detail, const char *instance);
problem_details_t *nf_problem_details_make_411(const char *detail, const char *instance);
problem_details_t *nf_problem_details_make_413(const char *detail, const char *instance);
problem_details_t *nf_problem_details_make_414(const char *detail, const char *instance);
problem_details_t *nf_problem_details_make_415_json(const char *detail,
                                         const char *instance);
problem_details_t *nf_problem_details_make_415_json_patch(const char *detail,
                                               const char *instance);
problem_details_t *nf_problem_details_make_429(const char *detail, const char *instance);
problem_details_t *nf_problem_details_make_500(const char *detail, const char *instance);
problem_details_t *nf_problem_details_make_503_db(const char *detail,
                                       const char *instance);
problem_details_t *nf_problem_details_make_503_upstream(const char *detail,
                                             const char *instance);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_NF_PROBLEM_DETAILS_WRAPPER_H_ */
