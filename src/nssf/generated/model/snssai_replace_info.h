/*
 * snssai_replace_info.h
 *
 * Indicates the status of an S-NSSAI and an alternative S-NSSAI optionally.
 */

#ifndef _snssai_replace_info_H_
#define _snssai_replace_info_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct snssai_replace_info_t snssai_replace_info_t;

#include "object.h"



typedef struct snssai_replace_info_t {
    object_t *snssai; // custom
    object_t *status; // custom
    object_t *alt_snssai; // custom
    object_t *ns_repl_termin_ind; // custom
    object_t *plmn_id; // custom
    object_t *mitigation_info; // custom

} snssai_replace_info_t;

snssai_replace_info_t *snssai_replace_info_create(
    object_t *snssai,
    object_t *status,
    object_t *alt_snssai,
    object_t *ns_repl_termin_ind,
    object_t *plmn_id,
    object_t *mitigation_info
);

void snssai_replace_info_free(snssai_replace_info_t *snssai_replace_info);

snssai_replace_info_t *snssai_replace_info_parseFromJSON(cJSON *snssai_replace_infoJSON);

cJSON *snssai_replace_info_convertToJSON(snssai_replace_info_t *snssai_replace_info);

#endif /* _snssai_replace_info_H_ */

