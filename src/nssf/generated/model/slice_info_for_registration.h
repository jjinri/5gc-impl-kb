/*
 * slice_info_for_registration.h
 *
 * Contains the slice information requested during a Registration procedure
 */

#ifndef _slice_info_for_registration_H_
#define _slice_info_for_registration_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct slice_info_for_registration_t slice_info_for_registration_t;

#include "allowed_nssai.h"
#include "mapping_of_snssai.h"
#include "snssai.h"
#include "subscribed_snssai.h"



typedef struct slice_info_for_registration_t {
    list_t *subscribed_nssai; //nonprimitive container
    struct allowed_nssai_t *allowed_nssai_current_access; //model
    struct allowed_nssai_t *allowed_nssai_other_access; //model
    list_t *s_nssai_for_mapping; //nonprimitive container
    list_t *requested_nssai; //nonprimitive container
    int default_configured_snssai_ind; //boolean
    list_t *mapping_of_nssai; //nonprimitive container
    int request_mapping; //boolean
    int ue_sup_nssrg_ind; //boolean
    int suppress_nssrg_ind; //boolean
    int nsag_supported; //boolean

} slice_info_for_registration_t;

slice_info_for_registration_t *slice_info_for_registration_create(
    list_t *subscribed_nssai,
    allowed_nssai_t *allowed_nssai_current_access,
    allowed_nssai_t *allowed_nssai_other_access,
    list_t *s_nssai_for_mapping,
    list_t *requested_nssai,
    int default_configured_snssai_ind,
    list_t *mapping_of_nssai,
    int request_mapping,
    int ue_sup_nssrg_ind,
    int suppress_nssrg_ind,
    int nsag_supported
);

void slice_info_for_registration_free(slice_info_for_registration_t *slice_info_for_registration);

slice_info_for_registration_t *slice_info_for_registration_parseFromJSON(cJSON *slice_info_for_registrationJSON);

cJSON *slice_info_for_registration_convertToJSON(slice_info_for_registration_t *slice_info_for_registration);

#endif /* _slice_info_for_registration_H_ */

