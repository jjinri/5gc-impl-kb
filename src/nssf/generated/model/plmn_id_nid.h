/*
 * plmn_id_nid.h
 *
 * Contains the serving core network operator PLMN ID and, for an SNPN, the NID that together with the PLMN ID identifies the SNPN. 
 */

#ifndef _plmn_id_nid_H_
#define _plmn_id_nid_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct plmn_id_nid_t plmn_id_nid_t;

#include "object.h"



typedef struct plmn_id_nid_t {
    object_t *mcc; // custom
    object_t *mnc; // custom
    object_t *nid; // custom

} plmn_id_nid_t;

plmn_id_nid_t *plmn_id_nid_create(
    object_t *mcc,
    object_t *mnc,
    object_t *nid
);

void plmn_id_nid_free(plmn_id_nid_t *plmn_id_nid);

plmn_id_nid_t *plmn_id_nid_parseFromJSON(cJSON *plmn_id_nidJSON);

cJSON *plmn_id_nid_convertToJSON(plmn_id_nid_t *plmn_id_nid);

#endif /* _plmn_id_nid_H_ */

