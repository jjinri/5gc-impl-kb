/*
 * authorized_network_slice_info.h
 *
 * Contains the authorized network slice information
 */

#ifndef _authorized_network_slice_info_H_
#define _authorized_network_slice_info_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct authorized_network_slice_info_t authorized_network_slice_info_t;

#include "allowed_nssai.h"
#include "configured_snssai.h"
#include "mapping_of_snssai.h"
#include "nsag_info.h"
#include "nsi_information.h"
#include "snssai.h"
#include "snssai_info.h"



typedef struct authorized_network_slice_info_t {
    list_t *allowed_nssai_list; //nonprimitive container
    list_t *configured_nssai; //nonprimitive container
    char *target_amf_set; // string
    list_t *candidate_amf_list; //primitive container
    list_t *rejected_nssai_in_plmn; //nonprimitive container
    list_t *rejected_nssai_in_ta; //nonprimitive container
    struct nsi_information_t *nsi_information; //model
    char *supported_features; // string
    char *nrf_amf_set; // string
    char *nrf_amf_set_nf_mgt_uri; // string
    char *nrf_amf_set_access_token_uri; // string
    list_t* nrf_oauth2_required; //map
    char *target_amf_service_set; // string
    list_t *target_nssai; //nonprimitive container
    list_t *nsag_infos; //nonprimitive container
    list_t *mapping_of_nssai; //nonprimitive container
    list_t* snssai_info_rsp_data; //map

} authorized_network_slice_info_t;

authorized_network_slice_info_t *authorized_network_slice_info_create(
    list_t *allowed_nssai_list,
    list_t *configured_nssai,
    char *target_amf_set,
    list_t *candidate_amf_list,
    list_t *rejected_nssai_in_plmn,
    list_t *rejected_nssai_in_ta,
    nsi_information_t *nsi_information,
    char *supported_features,
    char *nrf_amf_set,
    char *nrf_amf_set_nf_mgt_uri,
    char *nrf_amf_set_access_token_uri,
    list_t* nrf_oauth2_required,
    char *target_amf_service_set,
    list_t *target_nssai,
    list_t *nsag_infos,
    list_t *mapping_of_nssai,
    list_t* snssai_info_rsp_data
);

void authorized_network_slice_info_free(authorized_network_slice_info_t *authorized_network_slice_info);

authorized_network_slice_info_t *authorized_network_slice_info_parseFromJSON(cJSON *authorized_network_slice_infoJSON);

cJSON *authorized_network_slice_info_convertToJSON(authorized_network_slice_info_t *authorized_network_slice_info);

#endif /* _authorized_network_slice_info_H_ */

