/*
 * nsi_unavailability_subscribe_info.h
 *
 * Present if the NF service consumer subscribes to events related to Network Slice Instance Replacement. 
 */

#ifndef _nsi_unavailability_subscribe_info_H_
#define _nsi_unavailability_subscribe_info_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct nsi_unavailability_subscribe_info_t nsi_unavailability_subscribe_info_t;

#include "object.h"



typedef struct nsi_unavailability_subscribe_info_t {
    list_t *nsi_to_subscribe; //nonprimitive container
    list_t *snssai_to_subscribe; //nonprimitive container

} nsi_unavailability_subscribe_info_t;

nsi_unavailability_subscribe_info_t *nsi_unavailability_subscribe_info_create(
    list_t *nsi_to_subscribe,
    list_t *snssai_to_subscribe
);

void nsi_unavailability_subscribe_info_free(nsi_unavailability_subscribe_info_t *nsi_unavailability_subscribe_info);

nsi_unavailability_subscribe_info_t *nsi_unavailability_subscribe_info_parseFromJSON(cJSON *nsi_unavailability_subscribe_infoJSON);

cJSON *nsi_unavailability_subscribe_info_convertToJSON(nsi_unavailability_subscribe_info_t *nsi_unavailability_subscribe_info);

#endif /* _nsi_unavailability_subscribe_info_H_ */

