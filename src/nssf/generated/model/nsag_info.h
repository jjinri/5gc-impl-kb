/*
 * nsag_info.h
 *
 * Contains the association of NSAGs and S-NSSAI(s) along with the TA(s) within which the association is valid. 
 */

#ifndef _nsag_info_H_
#define _nsag_info_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct nsag_info_t nsag_info_t;

#include "snssai.h"
#include "tai.h"
#include "tai_range.h"



typedef struct nsag_info_t {
    list_t *nsag_ids; //primitive container
    list_t *snssai_list; //nonprimitive container
    list_t *tai_list; //nonprimitive container
    list_t *tai_range_list; //nonprimitive container

} nsag_info_t;

nsag_info_t *nsag_info_create(
    list_t *nsag_ids,
    list_t *snssai_list,
    list_t *tai_list,
    list_t *tai_range_list
);

void nsag_info_free(nsag_info_t *nsag_info);

nsag_info_t *nsag_info_parseFromJSON(cJSON *nsag_infoJSON);

cJSON *nsag_info_convertToJSON(nsag_info_t *nsag_info);

#endif /* _nsag_info_H_ */

