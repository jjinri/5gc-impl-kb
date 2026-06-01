/*
 * snssai_replacement_subscribe_info.h
 *
 * Present if the NF service consumer subscribes to events related to Network Slice Replacement. 
 */

#ifndef _snssai_replacement_subscribe_info_H_
#define _snssai_replacement_subscribe_info_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct snssai_replacement_subscribe_info_t snssai_replacement_subscribe_info_t;

#include "object.h"



typedef struct snssai_replacement_subscribe_info_t {
    list_t *snssai_to_subscribe; //nonprimitive container
    object_t *nf_type; // custom
    object_t *nf_id; // custom
    object_t *plmn_id; // custom

} snssai_replacement_subscribe_info_t;

snssai_replacement_subscribe_info_t *snssai_replacement_subscribe_info_create(
    list_t *snssai_to_subscribe,
    object_t *nf_type,
    object_t *nf_id,
    object_t *plmn_id
);

void snssai_replacement_subscribe_info_free(snssai_replacement_subscribe_info_t *snssai_replacement_subscribe_info);

snssai_replacement_subscribe_info_t *snssai_replacement_subscribe_info_parseFromJSON(cJSON *snssai_replacement_subscribe_infoJSON);

cJSON *snssai_replacement_subscribe_info_convertToJSON(snssai_replacement_subscribe_info_t *snssai_replacement_subscribe_info);

#endif /* _snssai_replacement_subscribe_info_H_ */

