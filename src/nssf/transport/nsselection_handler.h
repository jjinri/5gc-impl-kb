#ifndef NSSF_TRANSPORT_NSSELECTION_HANDLER_H
#define NSSF_TRANSPORT_NSSELECTION_HANDLER_H

#include <stddef.h>

/*
 * Phase 1 tracer-bullet handler for NSSelectionGet.
 *
 * 501 Not Implemented (application/problem+json) 응답을 hardcoded JSON
 * string 으로 반환한다. cJSON encoding 은 Phase 3 진입.
 *
 * out_body  — 호출자 소유 버퍼.
 * out_size  — 버퍼 크기 (bytes).
 * 반환값    — HTTP status code (Phase 1 항상 501).
 *
 * out_body 가 NULL 이거나 out_size 가 응답 길이 미만이면 반환값은 동일하게
 * 501 이지만 body 는 truncate 또는 미기록될 수 있다. Phase 1 contract 검증은
 * 충분한 버퍼를 가정한다.
 */
int nsselection_get_handle(void *unused_request,
                           char *out_body,
                           size_t out_size);

#endif /* NSSF_TRANSPORT_NSSELECTION_HANDLER_H */
