/*
 * allowed_nssai.h
 *
 * Contains an array of allowed S-NSSAI that constitute the allowed NSSAI information for the authorized network slice information 
 */

#ifndef _allowed_nssai_H_
#define _allowed_nssai_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct allowed_nssai_t allowed_nssai_t;

#include "access_type.h"
#include "allowed_snssai.h"

// Enum  for allowed_nssai

typedef enum  { nssf_ns_selection_allowed_nssai__NULL = 0, nssf_ns_selection_allowed_nssai___3GPP_ACCESS, nssf_ns_selection_allowed_nssai__NON_3GPP_ACCESS } nssf_ns_selection_allowed_nssai__e;

char* allowed_nssai_access_type_ToString(nssf_ns_selection_allowed_nssai__e access_type);

nssf_ns_selection_allowed_nssai__e allowed_nssai_access_type_FromString(char* access_type);



typedef struct allowed_nssai_t {
    list_t *allowed_snssai_list; //nonprimitive container
    access_type_t *access_type; // custom

} allowed_nssai_t;

allowed_nssai_t *allowed_nssai_create(
    list_t *allowed_snssai_list,
    access_type_t *access_type
);

void allowed_nssai_free(allowed_nssai_t *allowed_nssai);

allowed_nssai_t *allowed_nssai_parseFromJSON(cJSON *allowed_nssaiJSON);

cJSON *allowed_nssai_convertToJSON(allowed_nssai_t *allowed_nssai);

#endif /* _allowed_nssai_H_ */

