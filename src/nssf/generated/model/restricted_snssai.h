/*
 * restricted_snssai.h
 *
 * This contains the restricted SNssai information per PLMN
 */

#ifndef _restricted_snssai_H_
#define _restricted_snssai_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct restricted_snssai_t restricted_snssai_t;

#include "ext_snssai.h"
#include "plmn_id.h"



typedef struct restricted_snssai_t {
    struct plmn_id_t *home_plmn_id; //model
    list_t *s_nssai_list; //nonprimitive container
    list_t *home_plmn_id_list; //nonprimitive container
    int roaming_restriction; //boolean

} restricted_snssai_t;

restricted_snssai_t *restricted_snssai_create(
    plmn_id_t *home_plmn_id,
    list_t *s_nssai_list,
    list_t *home_plmn_id_list,
    int roaming_restriction
);

void restricted_snssai_free(restricted_snssai_t *restricted_snssai);

restricted_snssai_t *restricted_snssai_parseFromJSON(cJSON *restricted_snssaiJSON);

cJSON *restricted_snssai_convertToJSON(restricted_snssai_t *restricted_snssai);

#endif /* _restricted_snssai_H_ */

