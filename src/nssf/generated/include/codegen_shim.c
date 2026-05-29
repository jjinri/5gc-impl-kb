/*
 * codegen_shim.c — PR #92 (WI-codegen-bootstrap) 생성기 출력 후속 보충.
 *
 * 발견된 generator 버그.
 *   1) access_token_err.c — `access_token_err_convertToJSON` 가 호출하는
 *      `erroraccess_token_err_ToString` 가 정의되지 않음. 같은 파일에
 *      *대칭 이름* (`access_token_err_error_ToString`) 으로 emit. 호출/정의
 *      양쪽 모두 generator template 산출이라 직접 수정하지 않고 본 shim 이
 *      *호출 측 이름* 으로 alias 한다.
 *   2) access_token_req.c 의 `grant_typeaccess_token_req_ToString` 도 동일
 *      패턴.
 *
 * 향후 openapi-generator template / config 가 호출 측 이름을 정의 측과
 * 일치시키면 본 shim 은 자동 dead code 가 되며 제거된다.
 */

#define _POSIX_C_SOURCE 200809L

#include "../model/access_token_err.h"
#include "../model/access_token_req.h"

char *erroraccess_token_err_ToString(
    nssf_ns_selection_access_token_err_ERROR_e error)
{
    return access_token_err_error_ToString(error);
}

char *grant_typeaccess_token_req_ToString(
    nssf_ns_selection_access_token_req_GRANTTYPE_e grant_type)
{
    return access_token_req_grant_type_ToString(grant_type);
}
