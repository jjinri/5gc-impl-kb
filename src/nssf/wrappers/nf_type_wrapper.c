/*
 * nf_type_wrapper.c — NFType anyOf passthrough wrapper.
 *
 * 67 known enum ↔ string table 단일 출처. probe 는 67 entry 의 linear strcmp
 * (lookup 빈도 낮음 — connection 수준 1회).
 */

#define _POSIX_C_SOURCE 200809L

#include "nf_type_wrapper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const NAMES[NSSF_NF_TYPE_KNOWN_COUNT] = {
    [NSSF_NF_TYPE_NRF]          = "NRF",
    [NSSF_NF_TYPE_UDM]          = "UDM",
    [NSSF_NF_TYPE_AMF]          = "AMF",
    [NSSF_NF_TYPE_SMF]          = "SMF",
    [NSSF_NF_TYPE_AUSF]         = "AUSF",
    [NSSF_NF_TYPE_NEF]          = "NEF",
    [NSSF_NF_TYPE_PCF]          = "PCF",
    [NSSF_NF_TYPE_SMSF]         = "SMSF",
    [NSSF_NF_TYPE_NSSF]         = "NSSF",
    [NSSF_NF_TYPE_UDR]          = "UDR",
    [NSSF_NF_TYPE_LMF]          = "LMF",
    [NSSF_NF_TYPE_GMLC]         = "GMLC",
    [NSSF_NF_TYPE_5G_EIR]       = "5G_EIR",
    [NSSF_NF_TYPE_SEPP]         = "SEPP",
    [NSSF_NF_TYPE_UPF]          = "UPF",
    [NSSF_NF_TYPE_N3IWF]        = "N3IWF",
    [NSSF_NF_TYPE_AF]           = "AF",
    [NSSF_NF_TYPE_UDSF]         = "UDSF",
    [NSSF_NF_TYPE_BSF]          = "BSF",
    [NSSF_NF_TYPE_CHF]          = "CHF",
    [NSSF_NF_TYPE_NWDAF]        = "NWDAF",
    [NSSF_NF_TYPE_PCSCF]        = "PCSCF",
    [NSSF_NF_TYPE_CBCF]         = "CBCF",
    [NSSF_NF_TYPE_HSS]          = "HSS",
    [NSSF_NF_TYPE_UCMF]         = "UCMF",
    [NSSF_NF_TYPE_SOR_AF]       = "SOR_AF",
    [NSSF_NF_TYPE_SPAF]         = "SPAF",
    [NSSF_NF_TYPE_MME]          = "MME",
    [NSSF_NF_TYPE_SCSAS]        = "SCSAS",
    [NSSF_NF_TYPE_SCEF]         = "SCEF",
    [NSSF_NF_TYPE_SCP]          = "SCP",
    [NSSF_NF_TYPE_NSSAAF]       = "NSSAAF",
    [NSSF_NF_TYPE_ICSCF]        = "ICSCF",
    [NSSF_NF_TYPE_SCSCF]        = "SCSCF",
    [NSSF_NF_TYPE_DRA]          = "DRA",
    [NSSF_NF_TYPE_IMS_AS]       = "IMS_AS",
    [NSSF_NF_TYPE_AANF]         = "AANF",
    [NSSF_NF_TYPE_5G_DDNMF]     = "5G_DDNMF",
    [NSSF_NF_TYPE_NSACF]        = "NSACF",
    [NSSF_NF_TYPE_MFAF]         = "MFAF",
    [NSSF_NF_TYPE_EASDF]        = "EASDF",
    [NSSF_NF_TYPE_DCCF]         = "DCCF",
    [NSSF_NF_TYPE_MB_SMF]       = "MB_SMF",
    [NSSF_NF_TYPE_TSCTSF]       = "TSCTSF",
    [NSSF_NF_TYPE_ADRF]         = "ADRF",
    [NSSF_NF_TYPE_GBA_BSF]      = "GBA_BSF",
    [NSSF_NF_TYPE_CEF]          = "CEF",
    [NSSF_NF_TYPE_MB_UPF]       = "MB_UPF",
    [NSSF_NF_TYPE_NSWOF]        = "NSWOF",
    [NSSF_NF_TYPE_PKMF]         = "PKMF",
    [NSSF_NF_TYPE_MNPF]         = "MNPF",
    [NSSF_NF_TYPE_SMS_GMSC]     = "SMS_GMSC",
    [NSSF_NF_TYPE_SMS_IWMSC]    = "SMS_IWMSC",
    [NSSF_NF_TYPE_MBSF]         = "MBSF",
    [NSSF_NF_TYPE_MBSTF]        = "MBSTF",
    [NSSF_NF_TYPE_PANF]         = "PANF",
    [NSSF_NF_TYPE_IP_SM_GW]     = "IP_SM_GW",
    [NSSF_NF_TYPE_SMS_ROUTER]   = "SMS_ROUTER",
    [NSSF_NF_TYPE_DCSF]         = "DCSF",
    [NSSF_NF_TYPE_MRF]          = "MRF",
    [NSSF_NF_TYPE_MRFP]         = "MRFP",
    [NSSF_NF_TYPE_MF]           = "MF",
    [NSSF_NF_TYPE_SLPKMF]       = "SLPKMF",
    [NSSF_NF_TYPE_RH]           = "RH",
    [NSSF_NF_TYPE_EIF]          = "EIF",
    [NSSF_NF_TYPE_AIOTF]        = "AIOTF",
    [NSSF_NF_TYPE_ADM]          = "ADM",
};

#define UNKNOWN_LABEL "UNKNOWN"

nssf_nf_type_e nssf_nf_type_probe(const char *input)
{
    if (input == NULL || input[0] == '\0') {
        return NSSF_NF_TYPE_UNKNOWN;
    }
    for (int i = 0; i < NSSF_NF_TYPE_KNOWN_COUNT; ++i) {
        if (strcmp(NAMES[i], input) == 0) {
            return (nssf_nf_type_e)i;
        }
    }
    return NSSF_NF_TYPE_UNKNOWN;
}

const char *nssf_nf_type_to_string(nssf_nf_type_e type)
{
    if (type == NSSF_NF_TYPE_UNKNOWN) {
        return UNKNOWN_LABEL;
    }
    if ((int)type < 0 || type >= NSSF_NF_TYPE_KNOWN_COUNT) {
        return UNKNOWN_LABEL;
    }
    return NAMES[type];
}

bool nssf_nf_type_is_known(nssf_nf_type_e type)
{
    return type != NSSF_NF_TYPE_UNKNOWN &&
           (int)type >= 0 &&
           type < NSSF_NF_TYPE_KNOWN_COUNT;
}

nssf_nf_type_t *nssf_nf_type_create_from_string(const char *input)
{
    if (input == NULL) {
        return NULL;
    }

    nssf_nf_type_t *wrapper = malloc(sizeof(*wrapper));
    if (wrapper == NULL) {
        return NULL;
    }

    wrapper->raw = strdup(input);
    if (wrapper->raw == NULL) {
        free(wrapper);
        return NULL;
    }

    wrapper->probed = nssf_nf_type_probe(input);

    if (wrapper->probed == NSSF_NF_TYPE_UNKNOWN) {
        /*
         * G-09 — unknown NFType 은 reject 하지 않고 passthrough + log warn.
         * logger 도입 (WI-tls-bootstrap 이후 또는 별도 logging PR) 시 zlog 로
         * 교체. 현재는 stderr emit 으로 ASan/UBSan 실행 중 가시성 확보.
         */
        fprintf(stderr,
                "[NSSF][WARN] unknown NFType passthrough — raw='%s'\n",
                wrapper->raw);
    }

    return wrapper;
}

void nssf_nf_type_free(nssf_nf_type_t *type)
{
    if (type == NULL) {
        return;
    }
    free(type->raw);
    free(type);
}
