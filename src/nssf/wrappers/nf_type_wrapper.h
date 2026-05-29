/*
 * nf_type_wrapper.h — NFType anyOf passthrough wrapper.
 *
 * 본 wrapper 는 design/nssf/contract/data-model/NFType.json 의 67 known enum
 * 과 *forward-compat passthrough* 정책 (open-gaps-and-assumptions G-09) 을
 * 구현한다. generated nf_type.h 는 anyOf 미지원으로 빈 struct 만 emit —
 * 실제 NFType lifecycle 은 본 wrapper 가 단일 책임.
 *
 * Lossless raw retain.
 *   wrapper 인스턴스는 *항상* 원본 문자열을 strdup 으로 보유한다. probed
 *   enum 이 UNKNOWN 이어도 원본 raw 는 다른 NF (예 NRF 등록 / Discovery 응답
 *   forward) 로 그대로 전달 가능. 3GPP forward-compat 정책 (G-09) 의 핵심
 *   불변 — 본 NF 가 모르는 신규 NFType 을 reject 하지 않는다.
 *
 * Memory ownership.
 *   nssf_nf_type_t 인스턴스는 nssf_nf_type_free 가 raw 포함 전부 해제.
 *   nssf_nf_type_to_string 은 정적 const 문자열 (UNKNOWN 포함) — caller 가
 *   복제·해제 책임 없음.
 */

#ifndef NSSF_NF_TYPE_WRAPPER_H_
#define NSSF_NF_TYPE_WRAPPER_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 67 known + UNKNOWN sentinel. order = NFType.json enum 순서. */
typedef enum {
    NSSF_NF_TYPE_NRF = 0,
    NSSF_NF_TYPE_UDM,
    NSSF_NF_TYPE_AMF,
    NSSF_NF_TYPE_SMF,
    NSSF_NF_TYPE_AUSF,
    NSSF_NF_TYPE_NEF,
    NSSF_NF_TYPE_PCF,
    NSSF_NF_TYPE_SMSF,
    NSSF_NF_TYPE_NSSF,
    NSSF_NF_TYPE_UDR,
    NSSF_NF_TYPE_LMF,
    NSSF_NF_TYPE_GMLC,
    NSSF_NF_TYPE_5G_EIR,
    NSSF_NF_TYPE_SEPP,
    NSSF_NF_TYPE_UPF,
    NSSF_NF_TYPE_N3IWF,
    NSSF_NF_TYPE_AF,
    NSSF_NF_TYPE_UDSF,
    NSSF_NF_TYPE_BSF,
    NSSF_NF_TYPE_CHF,
    NSSF_NF_TYPE_NWDAF,
    NSSF_NF_TYPE_PCSCF,
    NSSF_NF_TYPE_CBCF,
    NSSF_NF_TYPE_HSS,
    NSSF_NF_TYPE_UCMF,
    NSSF_NF_TYPE_SOR_AF,
    NSSF_NF_TYPE_SPAF,
    NSSF_NF_TYPE_MME,
    NSSF_NF_TYPE_SCSAS,
    NSSF_NF_TYPE_SCEF,
    NSSF_NF_TYPE_SCP,
    NSSF_NF_TYPE_NSSAAF,
    NSSF_NF_TYPE_ICSCF,
    NSSF_NF_TYPE_SCSCF,
    NSSF_NF_TYPE_DRA,
    NSSF_NF_TYPE_IMS_AS,
    NSSF_NF_TYPE_AANF,
    NSSF_NF_TYPE_5G_DDNMF,
    NSSF_NF_TYPE_NSACF,
    NSSF_NF_TYPE_MFAF,
    NSSF_NF_TYPE_EASDF,
    NSSF_NF_TYPE_DCCF,
    NSSF_NF_TYPE_MB_SMF,
    NSSF_NF_TYPE_TSCTSF,
    NSSF_NF_TYPE_ADRF,
    NSSF_NF_TYPE_GBA_BSF,
    NSSF_NF_TYPE_CEF,
    NSSF_NF_TYPE_MB_UPF,
    NSSF_NF_TYPE_NSWOF,
    NSSF_NF_TYPE_PKMF,
    NSSF_NF_TYPE_MNPF,
    NSSF_NF_TYPE_SMS_GMSC,
    NSSF_NF_TYPE_SMS_IWMSC,
    NSSF_NF_TYPE_MBSF,
    NSSF_NF_TYPE_MBSTF,
    NSSF_NF_TYPE_PANF,
    NSSF_NF_TYPE_IP_SM_GW,
    NSSF_NF_TYPE_SMS_ROUTER,
    NSSF_NF_TYPE_DCSF,
    NSSF_NF_TYPE_MRF,
    NSSF_NF_TYPE_MRFP,
    NSSF_NF_TYPE_MF,
    NSSF_NF_TYPE_SLPKMF,
    NSSF_NF_TYPE_RH,
    NSSF_NF_TYPE_EIF,
    NSSF_NF_TYPE_AIOTF,
    NSSF_NF_TYPE_ADM,
    NSSF_NF_TYPE_KNOWN_COUNT,        /* sentinel — 67 */
    NSSF_NF_TYPE_UNKNOWN = -1        /* G-09 passthrough */
} nssf_nf_type_e;

typedef struct {
    nssf_nf_type_e probed;  /* enum 또는 UNKNOWN */
    char *raw;              /* 원본 문자열 (lossless retain) */
} nssf_nf_type_t;

/* probe — input 문자열을 67 known enum 또는 UNKNOWN 으로 분류. NULL/empty 는 UNKNOWN. */
nssf_nf_type_e nssf_nf_type_probe(const char *input);

/* to_string — known enum 의 canonical 문자열 또는 "UNKNOWN". static const, caller 해제 금지. */
const char *nssf_nf_type_to_string(nssf_nf_type_e type);

/*
 * create_from_string — input 을 strdup 으로 retain + probe 결과 저장.
 *   input == NULL — NULL 반환 (G-09 적용 대상 외 — 호출자 입력 검증).
 *   OOM — NULL 반환.
 *   probed == UNKNOWN — stderr 경고 emit (logger 도입 후 zlog 로 교체).
 */
nssf_nf_type_t *nssf_nf_type_create_from_string(const char *input);

void nssf_nf_type_free(nssf_nf_type_t *type);

/* convenience predicate — UNKNOWN 외 모두 true. */
bool nssf_nf_type_is_known(nssf_nf_type_e type);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_NF_TYPE_WRAPPER_H_ */
