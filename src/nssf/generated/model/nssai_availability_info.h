/*
 * nssai_availability_info.h
 *
 * This contains the Nssai availability information requested by the AMF
 */

#ifndef _nssai_availability_info_H_
#define _nssai_availability_info_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct nssai_availability_info_t nssai_availability_info_t;

#include "supported_nssai_availability_data.h"



typedef struct nssai_availability_info_t {
    list_t *supported_nssai_availability_data; //nonprimitive container
    char *supported_features; // string
    char *amf_set_id; // string

} nssai_availability_info_t;

nssai_availability_info_t *nssai_availability_info_create(
    list_t *supported_nssai_availability_data,
    char *supported_features,
    char *amf_set_id
);

void nssai_availability_info_free(nssai_availability_info_t *nssai_availability_info);

nssai_availability_info_t *nssai_availability_info_parseFromJSON(cJSON *nssai_availability_infoJSON);

cJSON *nssai_availability_info_convertToJSON(nssai_availability_info_t *nssai_availability_info);

#endif /* _nssai_availability_info_H_ */

