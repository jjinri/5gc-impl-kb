/*
 * slice_info_for_ue_configuration_update.h
 *
 * Contains the slice information requested during UE configuration update procedure 
 */

#ifndef _slice_info_for_ue_configuration_update_H_
#define _slice_info_for_ue_configuration_update_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct slice_info_for_ue_configuration_update_t slice_info_for_ue_configuration_update_t;

#include "allowed_nssai.h"
#include "mapping_of_snssai.h"
#include "snssai.h"
#include "subscribed_snssai.h"



typedef struct slice_info_for_ue_configuration_update_t {
    list_t *subscribed_nssai; //nonprimitive container
    struct allowed_nssai_t *allowed_nssai_current_access; //model
    struct allowed_nssai_t *allowed_nssai_other_access; //model
    int default_configured_snssai_ind; //boolean
    list_t *requested_nssai; //nonprimitive container
    list_t *mapping_of_nssai; //nonprimitive container
    int ue_sup_nssrg_ind; //boolean
    int suppress_nssrg_ind; //boolean
    list_t *rejected_nssai_ra; //nonprimitive container
    int nsag_supported; //boolean

} slice_info_for_ue_configuration_update_t;

slice_info_for_ue_configuration_update_t *slice_info_for_ue_configuration_update_create(
    list_t *subscribed_nssai,
    allowed_nssai_t *allowed_nssai_current_access,
    allowed_nssai_t *allowed_nssai_other_access,
    int default_configured_snssai_ind,
    list_t *requested_nssai,
    list_t *mapping_of_nssai,
    int ue_sup_nssrg_ind,
    int suppress_nssrg_ind,
    list_t *rejected_nssai_ra,
    int nsag_supported
);

void slice_info_for_ue_configuration_update_free(slice_info_for_ue_configuration_update_t *slice_info_for_ue_configuration_update);

slice_info_for_ue_configuration_update_t *slice_info_for_ue_configuration_update_parseFromJSON(cJSON *slice_info_for_ue_configuration_updateJSON);

cJSON *slice_info_for_ue_configuration_update_convertToJSON(slice_info_for_ue_configuration_update_t *slice_info_for_ue_configuration_update);

#endif /* _slice_info_for_ue_configuration_update_H_ */

