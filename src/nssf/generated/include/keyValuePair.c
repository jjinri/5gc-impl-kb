/*
 * keyValuePair.c — generated/include/keyValuePair.h 의 runtime 구현.
 *
 * PR #92 (WI-codegen-bootstrap) 가 keyValuePair.h 만 emit 하고 .c 구현은
 * 누락 (openapi-generator C template 의 supportingFiles 미활성 — list.h 와
 * 동일 패턴). keyValuePair 의 첫 실사용자 (nssf_event_notification 모델의
 * snapshot payload) 가 본 slice (subscription-store) 라 본 scope 에서
 * list.c 와 동일하게 보충한다.
 *
 * 향후 regenerate.sh / openapi-generator config 가 keyValuePair.c 를 정상
 * emit 하도록 갱신되면 본 파일은 codegen 산출과 교체된다 (sibling list.c /
 * codegen_shim.c 와 동일 — generator 가 emit 해야 할 산출의 임시 보충).
 */

#define _POSIX_C_SOURCE 200809L

#include "keyValuePair.h"

#include <stdlib.h>

keyValuePair_t *keyValuePair_create(char *key, void *value)
{
    keyValuePair_t *keyValuePair =
        (keyValuePair_t *)malloc(sizeof(keyValuePair_t));
    if (keyValuePair == NULL) {
        return NULL;
    }
    keyValuePair->key = key;
    keyValuePair->value = value;
    return keyValuePair;
}

keyValuePair_t *keyValuePair_create_allocate(char *key, double value)
{
    /* value 는 double 을 heap box 로 보관 — generator 의 표준 의미. 호출
     * 측이 free 책임을 갖는 value 포인터를 keyValuePair 가 소유한다. */
    double *valuePointer = (double *)malloc(sizeof(double));
    if (valuePointer == NULL) {
        return NULL;
    }
    *valuePointer = value;
    keyValuePair_t *keyValuePair = keyValuePair_create(key, valuePointer);
    if (keyValuePair == NULL) {
        free(valuePointer);
        return NULL;
    }
    return keyValuePair;
}

void keyValuePair_free(keyValuePair_t *keyValuePair)
{
    if (keyValuePair == NULL) {
        return;
    }
    free(keyValuePair);
}
