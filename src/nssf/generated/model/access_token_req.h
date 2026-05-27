/*
 * access_token_req.h
 *
 * Contains information related to the access token request
 */

#ifndef _access_token_req_H_
#define _access_token_req_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct access_token_req_t access_token_req_t;

#include "ml_model_inter_ind.h"
#include "nf_type.h"
#include "object.h"
#include "plmn_id.h"
#include "plmn_id_nid.h"
#include "snssai.h"

// Enum GRANTTYPE for access_token_req

typedef enum  { nssf_ns_selection_access_token_req_GRANTTYPE_NULL = 0, nssf_ns_selection_access_token_req_GRANTTYPE_client_credentials } nssf_ns_selection_access_token_req_GRANTTYPE_e;

char* access_token_req_grant_type_ToString(nssf_ns_selection_access_token_req_GRANTTYPE_e grant_type);

nssf_ns_selection_access_token_req_GRANTTYPE_e access_token_req_grant_type_FromString(char* grant_type);



typedef struct access_token_req_t {
    nssf_ns_selection_access_token_req_GRANTTYPE_e grant_type; //enum
    char *nf_instance_id; // string
    struct nf_type_t *nf_type; //model
    struct nf_type_t *target_nf_type; //model
    char *scope; // string
    char *target_nf_instance_id; // string
    struct plmn_id_t *requester_plmn; //model
    list_t *requester_plmn_list; //nonprimitive container
    list_t *requester_snssai_list; //nonprimitive container
    char *requester_fqdn; // string
    list_t *requester_snpn_list; //nonprimitive container
    struct plmn_id_t *target_plmn; //model
    struct plmn_id_nid_t *target_snpn; //model
    list_t *target_snssai_list; //nonprimitive container
    list_t *target_nsi_list; //primitive container
    char *target_nf_set_id; // string
    char *target_nf_service_set_id; // string
    char *hnrf_access_token_uri; // string
    char *source_nf_instance_id; // string
    char *vendor_id; // string
    list_t *analytics_ids; //nonprimitive container
    list_t *requester_inter_ind_list; //nonprimitive container
    object_t *source_vendor_id; // custom
    char *af_id; // string

} access_token_req_t;

access_token_req_t *access_token_req_create(
    nssf_ns_selection_access_token_req_GRANTTYPE_e grant_type,
    char *nf_instance_id,
    nf_type_t *nf_type,
    nf_type_t *target_nf_type,
    char *scope,
    char *target_nf_instance_id,
    plmn_id_t *requester_plmn,
    list_t *requester_plmn_list,
    list_t *requester_snssai_list,
    char *requester_fqdn,
    list_t *requester_snpn_list,
    plmn_id_t *target_plmn,
    plmn_id_nid_t *target_snpn,
    list_t *target_snssai_list,
    list_t *target_nsi_list,
    char *target_nf_set_id,
    char *target_nf_service_set_id,
    char *hnrf_access_token_uri,
    char *source_nf_instance_id,
    char *vendor_id,
    list_t *analytics_ids,
    list_t *requester_inter_ind_list,
    object_t *source_vendor_id,
    char *af_id
);

void access_token_req_free(access_token_req_t *access_token_req);

access_token_req_t *access_token_req_parseFromJSON(cJSON *access_token_reqJSON);

cJSON *access_token_req_convertToJSON(access_token_req_t *access_token_req);

#endif /* _access_token_req_H_ */

