/*
 * snssai_info.h
 *
 * Contains the slice information in the response from NSSF
 */

#ifndef _snssai_info_H_
#define _snssai_info_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct snssai_info_t snssai_info_t;




typedef struct snssai_info_t {
    list_t *nsi_ids; //primitive container

} snssai_info_t;

snssai_info_t *snssai_info_create(
    list_t *nsi_ids
);

void snssai_info_free(snssai_info_t *snssai_info);

snssai_info_t *snssai_info_parseFromJSON(cJSON *snssai_infoJSON);

cJSON *snssai_info_convertToJSON(snssai_info_t *snssai_info);

#endif /* _snssai_info_H_ */

