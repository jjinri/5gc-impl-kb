#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "access_token_req.h"


char* access_token_req_grant_type_ToString(nssf_ns_selection_access_token_req_GRANTTYPE_e grant_type) {
    char* grant_typeArray[] =  { "NULL", "client_credentials" };
    return grant_typeArray[grant_type];
}

nssf_ns_selection_access_token_req_GRANTTYPE_e access_token_req_grant_type_FromString(char* grant_type){
    int stringToReturn = 0;
    char *grant_typeArray[] =  { "NULL", "client_credentials" };
    size_t sizeofArray = sizeof(grant_typeArray) / sizeof(grant_typeArray[0]);
    while(stringToReturn < sizeofArray) {
        if(strcmp(grant_type, grant_typeArray[stringToReturn]) == 0) {
            return stringToReturn;
        }
        stringToReturn++;
    }
    return 0;
}

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
    ) {
    access_token_req_t *access_token_req_local_var = malloc(sizeof(access_token_req_t));
    if (!access_token_req_local_var) {
        return NULL;
    }
    access_token_req_local_var->grant_type = grant_type;
    access_token_req_local_var->nf_instance_id = nf_instance_id;
    access_token_req_local_var->nf_type = nf_type;
    access_token_req_local_var->target_nf_type = target_nf_type;
    access_token_req_local_var->scope = scope;
    access_token_req_local_var->target_nf_instance_id = target_nf_instance_id;
    access_token_req_local_var->requester_plmn = requester_plmn;
    access_token_req_local_var->requester_plmn_list = requester_plmn_list;
    access_token_req_local_var->requester_snssai_list = requester_snssai_list;
    access_token_req_local_var->requester_fqdn = requester_fqdn;
    access_token_req_local_var->requester_snpn_list = requester_snpn_list;
    access_token_req_local_var->target_plmn = target_plmn;
    access_token_req_local_var->target_snpn = target_snpn;
    access_token_req_local_var->target_snssai_list = target_snssai_list;
    access_token_req_local_var->target_nsi_list = target_nsi_list;
    access_token_req_local_var->target_nf_set_id = target_nf_set_id;
    access_token_req_local_var->target_nf_service_set_id = target_nf_service_set_id;
    access_token_req_local_var->hnrf_access_token_uri = hnrf_access_token_uri;
    access_token_req_local_var->source_nf_instance_id = source_nf_instance_id;
    access_token_req_local_var->vendor_id = vendor_id;
    access_token_req_local_var->analytics_ids = analytics_ids;
    access_token_req_local_var->requester_inter_ind_list = requester_inter_ind_list;
    access_token_req_local_var->source_vendor_id = source_vendor_id;
    access_token_req_local_var->af_id = af_id;

    return access_token_req_local_var;
}


void access_token_req_free(access_token_req_t *access_token_req) {
    if(NULL == access_token_req){
        return ;
    }
    listEntry_t *listEntry;
    if (access_token_req->nf_instance_id) {
        free(access_token_req->nf_instance_id);
        access_token_req->nf_instance_id = NULL;
    }
    if (access_token_req->nf_type) {
        nf_type_free(access_token_req->nf_type);
        access_token_req->nf_type = NULL;
    }
    if (access_token_req->target_nf_type) {
        nf_type_free(access_token_req->target_nf_type);
        access_token_req->target_nf_type = NULL;
    }
    if (access_token_req->scope) {
        free(access_token_req->scope);
        access_token_req->scope = NULL;
    }
    if (access_token_req->target_nf_instance_id) {
        free(access_token_req->target_nf_instance_id);
        access_token_req->target_nf_instance_id = NULL;
    }
    if (access_token_req->requester_plmn) {
        plmn_id_free(access_token_req->requester_plmn);
        access_token_req->requester_plmn = NULL;
    }
    if (access_token_req->requester_plmn_list) {
        list_ForEach(listEntry, access_token_req->requester_plmn_list) {
            plmn_id_free(listEntry->data);
        }
        list_freeList(access_token_req->requester_plmn_list);
        access_token_req->requester_plmn_list = NULL;
    }
    if (access_token_req->requester_snssai_list) {
        list_ForEach(listEntry, access_token_req->requester_snssai_list) {
            snssai_free(listEntry->data);
        }
        list_freeList(access_token_req->requester_snssai_list);
        access_token_req->requester_snssai_list = NULL;
    }
    if (access_token_req->requester_fqdn) {
        free(access_token_req->requester_fqdn);
        access_token_req->requester_fqdn = NULL;
    }
    if (access_token_req->requester_snpn_list) {
        list_ForEach(listEntry, access_token_req->requester_snpn_list) {
            plmn_id_nid_free(listEntry->data);
        }
        list_freeList(access_token_req->requester_snpn_list);
        access_token_req->requester_snpn_list = NULL;
    }
    if (access_token_req->target_plmn) {
        plmn_id_free(access_token_req->target_plmn);
        access_token_req->target_plmn = NULL;
    }
    if (access_token_req->target_snpn) {
        plmn_id_nid_free(access_token_req->target_snpn);
        access_token_req->target_snpn = NULL;
    }
    if (access_token_req->target_snssai_list) {
        list_ForEach(listEntry, access_token_req->target_snssai_list) {
            snssai_free(listEntry->data);
        }
        list_freeList(access_token_req->target_snssai_list);
        access_token_req->target_snssai_list = NULL;
    }
    if (access_token_req->target_nsi_list) {
        list_ForEach(listEntry, access_token_req->target_nsi_list) {
            free(listEntry->data);
        }
        list_freeList(access_token_req->target_nsi_list);
        access_token_req->target_nsi_list = NULL;
    }
    if (access_token_req->target_nf_set_id) {
        free(access_token_req->target_nf_set_id);
        access_token_req->target_nf_set_id = NULL;
    }
    if (access_token_req->target_nf_service_set_id) {
        free(access_token_req->target_nf_service_set_id);
        access_token_req->target_nf_service_set_id = NULL;
    }
    if (access_token_req->hnrf_access_token_uri) {
        free(access_token_req->hnrf_access_token_uri);
        access_token_req->hnrf_access_token_uri = NULL;
    }
    if (access_token_req->source_nf_instance_id) {
        free(access_token_req->source_nf_instance_id);
        access_token_req->source_nf_instance_id = NULL;
    }
    if (access_token_req->vendor_id) {
        free(access_token_req->vendor_id);
        access_token_req->vendor_id = NULL;
    }
    if (access_token_req->analytics_ids) {
        list_ForEach(listEntry, access_token_req->analytics_ids) {
            object_free(listEntry->data);
        }
        list_freeList(access_token_req->analytics_ids);
        access_token_req->analytics_ids = NULL;
    }
    if (access_token_req->requester_inter_ind_list) {
        list_ForEach(listEntry, access_token_req->requester_inter_ind_list) {
            ml_model_inter_ind_free(listEntry->data);
        }
        list_freeList(access_token_req->requester_inter_ind_list);
        access_token_req->requester_inter_ind_list = NULL;
    }
    if (access_token_req->source_vendor_id) {
        object_free(access_token_req->source_vendor_id);
        access_token_req->source_vendor_id = NULL;
    }
    if (access_token_req->af_id) {
        free(access_token_req->af_id);
        access_token_req->af_id = NULL;
    }
    free(access_token_req);
}

cJSON *access_token_req_convertToJSON(access_token_req_t *access_token_req) {
    cJSON *item = cJSON_CreateObject();

    // access_token_req->grant_type
    if (nssf_ns_selection_access_token_req_GRANTTYPE_NULL == access_token_req->grant_type) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "grant_type", grant_typeaccess_token_req_ToString(access_token_req->grant_type)) == NULL)
    {
    goto fail; //Enum
    }


    // access_token_req->nf_instance_id
    if (!access_token_req->nf_instance_id) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "nfInstanceId", access_token_req->nf_instance_id) == NULL) {
    goto fail; //String
    }


    // access_token_req->nf_type
    if(access_token_req->nf_type) {
    cJSON *nf_type_local_JSON = nf_type_convertToJSON(access_token_req->nf_type);
    if(nf_type_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "nfType", nf_type_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // access_token_req->target_nf_type
    if(access_token_req->target_nf_type) {
    cJSON *target_nf_type_local_JSON = nf_type_convertToJSON(access_token_req->target_nf_type);
    if(target_nf_type_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "targetNfType", target_nf_type_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // access_token_req->scope
    if (!access_token_req->scope) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "scope", access_token_req->scope) == NULL) {
    goto fail; //String
    }


    // access_token_req->target_nf_instance_id
    if(access_token_req->target_nf_instance_id) {
    if(cJSON_AddStringToObject(item, "targetNfInstanceId", access_token_req->target_nf_instance_id) == NULL) {
    goto fail; //String
    }
    }


    // access_token_req->requester_plmn
    if(access_token_req->requester_plmn) {
    cJSON *requester_plmn_local_JSON = plmn_id_convertToJSON(access_token_req->requester_plmn);
    if(requester_plmn_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "requesterPlmn", requester_plmn_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // access_token_req->requester_plmn_list
    if(access_token_req->requester_plmn_list) {
    cJSON *requester_plmn_list = cJSON_AddArrayToObject(item, "requesterPlmnList");
    if(requester_plmn_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *requester_plmn_listListEntry;
    if (access_token_req->requester_plmn_list) {
    list_ForEach(requester_plmn_listListEntry, access_token_req->requester_plmn_list) {
    cJSON *itemLocal = plmn_id_convertToJSON(requester_plmn_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(requester_plmn_list, itemLocal);
    }
    }
    }


    // access_token_req->requester_snssai_list
    if(access_token_req->requester_snssai_list) {
    cJSON *requester_snssai_list = cJSON_AddArrayToObject(item, "requesterSnssaiList");
    if(requester_snssai_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *requester_snssai_listListEntry;
    if (access_token_req->requester_snssai_list) {
    list_ForEach(requester_snssai_listListEntry, access_token_req->requester_snssai_list) {
    cJSON *itemLocal = snssai_convertToJSON(requester_snssai_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(requester_snssai_list, itemLocal);
    }
    }
    }


    // access_token_req->requester_fqdn
    if(access_token_req->requester_fqdn) {
    if(cJSON_AddStringToObject(item, "requesterFqdn", access_token_req->requester_fqdn) == NULL) {
    goto fail; //String
    }
    }


    // access_token_req->requester_snpn_list
    if(access_token_req->requester_snpn_list) {
    cJSON *requester_snpn_list = cJSON_AddArrayToObject(item, "requesterSnpnList");
    if(requester_snpn_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *requester_snpn_listListEntry;
    if (access_token_req->requester_snpn_list) {
    list_ForEach(requester_snpn_listListEntry, access_token_req->requester_snpn_list) {
    cJSON *itemLocal = plmn_id_nid_convertToJSON(requester_snpn_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(requester_snpn_list, itemLocal);
    }
    }
    }


    // access_token_req->target_plmn
    if(access_token_req->target_plmn) {
    cJSON *target_plmn_local_JSON = plmn_id_convertToJSON(access_token_req->target_plmn);
    if(target_plmn_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "targetPlmn", target_plmn_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // access_token_req->target_snpn
    if(access_token_req->target_snpn) {
    cJSON *target_snpn_local_JSON = plmn_id_nid_convertToJSON(access_token_req->target_snpn);
    if(target_snpn_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "targetSnpn", target_snpn_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // access_token_req->target_snssai_list
    if(access_token_req->target_snssai_list) {
    cJSON *target_snssai_list = cJSON_AddArrayToObject(item, "targetSnssaiList");
    if(target_snssai_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *target_snssai_listListEntry;
    if (access_token_req->target_snssai_list) {
    list_ForEach(target_snssai_listListEntry, access_token_req->target_snssai_list) {
    cJSON *itemLocal = snssai_convertToJSON(target_snssai_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(target_snssai_list, itemLocal);
    }
    }
    }


    // access_token_req->target_nsi_list
    if(access_token_req->target_nsi_list) {
    cJSON *target_nsi_list = cJSON_AddArrayToObject(item, "targetNsiList");
    if(target_nsi_list == NULL) {
        goto fail; //primitive container
    }

    listEntry_t *target_nsi_listListEntry;
    list_ForEach(target_nsi_listListEntry, access_token_req->target_nsi_list) {
    if(cJSON_AddStringToObject(target_nsi_list, "", (char*)target_nsi_listListEntry->data) == NULL)
    {
        goto fail;
    }
    }
    }


    // access_token_req->target_nf_set_id
    if(access_token_req->target_nf_set_id) {
    if(cJSON_AddStringToObject(item, "targetNfSetId", access_token_req->target_nf_set_id) == NULL) {
    goto fail; //String
    }
    }


    // access_token_req->target_nf_service_set_id
    if(access_token_req->target_nf_service_set_id) {
    if(cJSON_AddStringToObject(item, "targetNfServiceSetId", access_token_req->target_nf_service_set_id) == NULL) {
    goto fail; //String
    }
    }


    // access_token_req->hnrf_access_token_uri
    if(access_token_req->hnrf_access_token_uri) {
    if(cJSON_AddStringToObject(item, "hnrfAccessTokenUri", access_token_req->hnrf_access_token_uri) == NULL) {
    goto fail; //String
    }
    }


    // access_token_req->source_nf_instance_id
    if(access_token_req->source_nf_instance_id) {
    if(cJSON_AddStringToObject(item, "sourceNfInstanceId", access_token_req->source_nf_instance_id) == NULL) {
    goto fail; //String
    }
    }


    // access_token_req->vendor_id
    if(access_token_req->vendor_id) {
    if(cJSON_AddStringToObject(item, "vendorId", access_token_req->vendor_id) == NULL) {
    goto fail; //String
    }
    }


    // access_token_req->analytics_ids
    if(access_token_req->analytics_ids) {
    cJSON *analytics_ids = cJSON_AddArrayToObject(item, "analyticsIds");
    if(analytics_ids == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *analytics_idsListEntry;
    if (access_token_req->analytics_ids) {
    list_ForEach(analytics_idsListEntry, access_token_req->analytics_ids) {
    cJSON *itemLocal = object_convertToJSON(analytics_idsListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(analytics_ids, itemLocal);
    }
    }
    }


    // access_token_req->requester_inter_ind_list
    if(access_token_req->requester_inter_ind_list) {
    cJSON *requester_inter_ind_list = cJSON_AddArrayToObject(item, "requesterInterIndList");
    if(requester_inter_ind_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *requester_inter_ind_listListEntry;
    if (access_token_req->requester_inter_ind_list) {
    list_ForEach(requester_inter_ind_listListEntry, access_token_req->requester_inter_ind_list) {
    cJSON *itemLocal = ml_model_inter_ind_convertToJSON(requester_inter_ind_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(requester_inter_ind_list, itemLocal);
    }
    }
    }


    // access_token_req->source_vendor_id
    if(access_token_req->source_vendor_id) {
    cJSON *source_vendor_id_local_JSON = object_convertToJSON(access_token_req->source_vendor_id);
    if(source_vendor_id_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "sourceVendorId", source_vendor_id_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }
    }


    // access_token_req->af_id
    if(access_token_req->af_id) {
    if(cJSON_AddStringToObject(item, "afId", access_token_req->af_id) == NULL) {
    goto fail; //String
    }
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

access_token_req_t *access_token_req_parseFromJSON(cJSON *access_token_reqJSON){

    access_token_req_t *access_token_req_local_var = NULL;

    // define the local variable for access_token_req->nf_type
    nf_type_t *nf_type_local_nonprim = NULL;

    // define the local variable for access_token_req->target_nf_type
    nf_type_t *target_nf_type_local_nonprim = NULL;

    // define the local variable for access_token_req->requester_plmn
    plmn_id_t *requester_plmn_local_nonprim = NULL;

    // define the local list for access_token_req->requester_plmn_list
    list_t *requester_plmn_listList = NULL;

    // define the local list for access_token_req->requester_snssai_list
    list_t *requester_snssai_listList = NULL;

    // define the local list for access_token_req->requester_snpn_list
    list_t *requester_snpn_listList = NULL;

    // define the local variable for access_token_req->target_plmn
    plmn_id_t *target_plmn_local_nonprim = NULL;

    // define the local variable for access_token_req->target_snpn
    plmn_id_nid_t *target_snpn_local_nonprim = NULL;

    // define the local list for access_token_req->target_snssai_list
    list_t *target_snssai_listList = NULL;

    // define the local list for access_token_req->target_nsi_list
    list_t *target_nsi_listList = NULL;

    // define the local list for access_token_req->analytics_ids
    list_t *analytics_idsList = NULL;

    // define the local list for access_token_req->requester_inter_ind_list
    list_t *requester_inter_ind_listList = NULL;

    // define the local variable for access_token_req->source_vendor_id
    object_t *source_vendor_id_local_nonprim = NULL;

    // access_token_req->grant_type
    cJSON *grant_type = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "grant_type");
    if (!grant_type) {
        goto end;
    }

    nssf_ns_selection_access_token_req_GRANTTYPE_e grant_typeVariable;
    
    if(!cJSON_IsString(grant_type))
    {
    goto end; //Enum
    }
    grant_typeVariable = access_token_req_grant_type_FromString(grant_type->valuestring);

    // access_token_req->nf_instance_id
    cJSON *nf_instance_id = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "nfInstanceId");
    if (!nf_instance_id) {
        goto end;
    }

    
    if(!cJSON_IsString(nf_instance_id))
    {
    goto end; //String
    }

    // access_token_req->nf_type
    cJSON *nf_type = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "nfType");
    if (nf_type) { 
    nf_type_local_nonprim = nf_type_parseFromJSON(nf_type); //nonprimitive
    }

    // access_token_req->target_nf_type
    cJSON *target_nf_type = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "targetNfType");
    if (target_nf_type) { 
    target_nf_type_local_nonprim = nf_type_parseFromJSON(target_nf_type); //nonprimitive
    }

    // access_token_req->scope
    cJSON *scope = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "scope");
    if (!scope) {
        goto end;
    }

    
    if(!cJSON_IsString(scope))
    {
    goto end; //String
    }

    // access_token_req->target_nf_instance_id
    cJSON *target_nf_instance_id = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "targetNfInstanceId");
    if (target_nf_instance_id) { 
    if(!cJSON_IsString(target_nf_instance_id) && !cJSON_IsNull(target_nf_instance_id))
    {
    goto end; //String
    }
    }

    // access_token_req->requester_plmn
    cJSON *requester_plmn = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "requesterPlmn");
    if (requester_plmn) { 
    requester_plmn_local_nonprim = plmn_id_parseFromJSON(requester_plmn); //nonprimitive
    }

    // access_token_req->requester_plmn_list
    cJSON *requester_plmn_list = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "requesterPlmnList");
    if (requester_plmn_list) { 
    cJSON *requester_plmn_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(requester_plmn_list)){
        goto end; //nonprimitive container
    }

    requester_plmn_listList = list_createList();

    cJSON_ArrayForEach(requester_plmn_list_local_nonprimitive,requester_plmn_list )
    {
        if(!cJSON_IsObject(requester_plmn_list_local_nonprimitive)){
            goto end;
        }
        plmn_id_t *requester_plmn_listItem = plmn_id_parseFromJSON(requester_plmn_list_local_nonprimitive);

        list_addElement(requester_plmn_listList, requester_plmn_listItem);
    }
    }

    // access_token_req->requester_snssai_list
    cJSON *requester_snssai_list = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "requesterSnssaiList");
    if (requester_snssai_list) { 
    cJSON *requester_snssai_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(requester_snssai_list)){
        goto end; //nonprimitive container
    }

    requester_snssai_listList = list_createList();

    cJSON_ArrayForEach(requester_snssai_list_local_nonprimitive,requester_snssai_list )
    {
        if(!cJSON_IsObject(requester_snssai_list_local_nonprimitive)){
            goto end;
        }
        snssai_t *requester_snssai_listItem = snssai_parseFromJSON(requester_snssai_list_local_nonprimitive);

        list_addElement(requester_snssai_listList, requester_snssai_listItem);
    }
    }

    // access_token_req->requester_fqdn
    cJSON *requester_fqdn = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "requesterFqdn");
    if (requester_fqdn) { 
    if(!cJSON_IsString(requester_fqdn) && !cJSON_IsNull(requester_fqdn))
    {
    goto end; //String
    }
    }

    // access_token_req->requester_snpn_list
    cJSON *requester_snpn_list = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "requesterSnpnList");
    if (requester_snpn_list) { 
    cJSON *requester_snpn_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(requester_snpn_list)){
        goto end; //nonprimitive container
    }

    requester_snpn_listList = list_createList();

    cJSON_ArrayForEach(requester_snpn_list_local_nonprimitive,requester_snpn_list )
    {
        if(!cJSON_IsObject(requester_snpn_list_local_nonprimitive)){
            goto end;
        }
        plmn_id_nid_t *requester_snpn_listItem = plmn_id_nid_parseFromJSON(requester_snpn_list_local_nonprimitive);

        list_addElement(requester_snpn_listList, requester_snpn_listItem);
    }
    }

    // access_token_req->target_plmn
    cJSON *target_plmn = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "targetPlmn");
    if (target_plmn) { 
    target_plmn_local_nonprim = plmn_id_parseFromJSON(target_plmn); //nonprimitive
    }

    // access_token_req->target_snpn
    cJSON *target_snpn = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "targetSnpn");
    if (target_snpn) { 
    target_snpn_local_nonprim = plmn_id_nid_parseFromJSON(target_snpn); //nonprimitive
    }

    // access_token_req->target_snssai_list
    cJSON *target_snssai_list = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "targetSnssaiList");
    if (target_snssai_list) { 
    cJSON *target_snssai_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(target_snssai_list)){
        goto end; //nonprimitive container
    }

    target_snssai_listList = list_createList();

    cJSON_ArrayForEach(target_snssai_list_local_nonprimitive,target_snssai_list )
    {
        if(!cJSON_IsObject(target_snssai_list_local_nonprimitive)){
            goto end;
        }
        snssai_t *target_snssai_listItem = snssai_parseFromJSON(target_snssai_list_local_nonprimitive);

        list_addElement(target_snssai_listList, target_snssai_listItem);
    }
    }

    // access_token_req->target_nsi_list
    cJSON *target_nsi_list = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "targetNsiList");
    if (target_nsi_list) { 
    cJSON *target_nsi_list_local = NULL;
    if(!cJSON_IsArray(target_nsi_list)) {
        goto end;//primitive container
    }
    target_nsi_listList = list_createList();

    cJSON_ArrayForEach(target_nsi_list_local, target_nsi_list)
    {
        if(!cJSON_IsString(target_nsi_list_local))
        {
            goto end;
        }
        list_addElement(target_nsi_listList , strdup(target_nsi_list_local->valuestring));
    }
    }

    // access_token_req->target_nf_set_id
    cJSON *target_nf_set_id = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "targetNfSetId");
    if (target_nf_set_id) { 
    if(!cJSON_IsString(target_nf_set_id) && !cJSON_IsNull(target_nf_set_id))
    {
    goto end; //String
    }
    }

    // access_token_req->target_nf_service_set_id
    cJSON *target_nf_service_set_id = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "targetNfServiceSetId");
    if (target_nf_service_set_id) { 
    if(!cJSON_IsString(target_nf_service_set_id) && !cJSON_IsNull(target_nf_service_set_id))
    {
    goto end; //String
    }
    }

    // access_token_req->hnrf_access_token_uri
    cJSON *hnrf_access_token_uri = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "hnrfAccessTokenUri");
    if (hnrf_access_token_uri) { 
    if(!cJSON_IsString(hnrf_access_token_uri) && !cJSON_IsNull(hnrf_access_token_uri))
    {
    goto end; //String
    }
    }

    // access_token_req->source_nf_instance_id
    cJSON *source_nf_instance_id = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "sourceNfInstanceId");
    if (source_nf_instance_id) { 
    if(!cJSON_IsString(source_nf_instance_id) && !cJSON_IsNull(source_nf_instance_id))
    {
    goto end; //String
    }
    }

    // access_token_req->vendor_id
    cJSON *vendor_id = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "vendorId");
    if (vendor_id) { 
    if(!cJSON_IsString(vendor_id) && !cJSON_IsNull(vendor_id))
    {
    goto end; //String
    }
    }

    // access_token_req->analytics_ids
    cJSON *analytics_ids = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "analyticsIds");
    if (analytics_ids) { 
    cJSON *analytics_ids_local_nonprimitive = NULL;
    if(!cJSON_IsArray(analytics_ids)){
        goto end; //nonprimitive container
    }

    analytics_idsList = list_createList();

    cJSON_ArrayForEach(analytics_ids_local_nonprimitive,analytics_ids )
    {
        if(!cJSON_IsObject(analytics_ids_local_nonprimitive)){
            goto end;
        }
        object_t *analytics_idsItem = object_parseFromJSON(analytics_ids_local_nonprimitive);

        list_addElement(analytics_idsList, analytics_idsItem);
    }
    }

    // access_token_req->requester_inter_ind_list
    cJSON *requester_inter_ind_list = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "requesterInterIndList");
    if (requester_inter_ind_list) { 
    cJSON *requester_inter_ind_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(requester_inter_ind_list)){
        goto end; //nonprimitive container
    }

    requester_inter_ind_listList = list_createList();

    cJSON_ArrayForEach(requester_inter_ind_list_local_nonprimitive,requester_inter_ind_list )
    {
        if(!cJSON_IsObject(requester_inter_ind_list_local_nonprimitive)){
            goto end;
        }
        ml_model_inter_ind_t *requester_inter_ind_listItem = ml_model_inter_ind_parseFromJSON(requester_inter_ind_list_local_nonprimitive);

        list_addElement(requester_inter_ind_listList, requester_inter_ind_listItem);
    }
    }

    // access_token_req->source_vendor_id
    cJSON *source_vendor_id = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "sourceVendorId");
    if (source_vendor_id) { 
    source_vendor_id_local_nonprim = object_parseFromJSON(source_vendor_id); //custom
    }

    // access_token_req->af_id
    cJSON *af_id = cJSON_GetObjectItemCaseSensitive(access_token_reqJSON, "afId");
    if (af_id) { 
    if(!cJSON_IsString(af_id) && !cJSON_IsNull(af_id))
    {
    goto end; //String
    }
    }


    access_token_req_local_var = access_token_req_create (
        grant_typeVariable,
        strdup(nf_instance_id->valuestring),
        nf_type ? nf_type_local_nonprim : NULL,
        target_nf_type ? target_nf_type_local_nonprim : NULL,
        strdup(scope->valuestring),
        target_nf_instance_id && !cJSON_IsNull(target_nf_instance_id) ? strdup(target_nf_instance_id->valuestring) : NULL,
        requester_plmn ? requester_plmn_local_nonprim : NULL,
        requester_plmn_list ? requester_plmn_listList : NULL,
        requester_snssai_list ? requester_snssai_listList : NULL,
        requester_fqdn && !cJSON_IsNull(requester_fqdn) ? strdup(requester_fqdn->valuestring) : NULL,
        requester_snpn_list ? requester_snpn_listList : NULL,
        target_plmn ? target_plmn_local_nonprim : NULL,
        target_snpn ? target_snpn_local_nonprim : NULL,
        target_snssai_list ? target_snssai_listList : NULL,
        target_nsi_list ? target_nsi_listList : NULL,
        target_nf_set_id && !cJSON_IsNull(target_nf_set_id) ? strdup(target_nf_set_id->valuestring) : NULL,
        target_nf_service_set_id && !cJSON_IsNull(target_nf_service_set_id) ? strdup(target_nf_service_set_id->valuestring) : NULL,
        hnrf_access_token_uri && !cJSON_IsNull(hnrf_access_token_uri) ? strdup(hnrf_access_token_uri->valuestring) : NULL,
        source_nf_instance_id && !cJSON_IsNull(source_nf_instance_id) ? strdup(source_nf_instance_id->valuestring) : NULL,
        vendor_id && !cJSON_IsNull(vendor_id) ? strdup(vendor_id->valuestring) : NULL,
        analytics_ids ? analytics_idsList : NULL,
        requester_inter_ind_list ? requester_inter_ind_listList : NULL,
        source_vendor_id ? source_vendor_id_local_nonprim : NULL,
        af_id && !cJSON_IsNull(af_id) ? strdup(af_id->valuestring) : NULL
        );

    return access_token_req_local_var;
end:
    if (nf_type_local_nonprim) {
        nf_type_free(nf_type_local_nonprim);
        nf_type_local_nonprim = NULL;
    }
    if (target_nf_type_local_nonprim) {
        nf_type_free(target_nf_type_local_nonprim);
        target_nf_type_local_nonprim = NULL;
    }
    if (requester_plmn_local_nonprim) {
        plmn_id_free(requester_plmn_local_nonprim);
        requester_plmn_local_nonprim = NULL;
    }
    if (requester_plmn_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, requester_plmn_listList) {
            plmn_id_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(requester_plmn_listList);
        requester_plmn_listList = NULL;
    }
    if (requester_snssai_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, requester_snssai_listList) {
            snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(requester_snssai_listList);
        requester_snssai_listList = NULL;
    }
    if (requester_snpn_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, requester_snpn_listList) {
            plmn_id_nid_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(requester_snpn_listList);
        requester_snpn_listList = NULL;
    }
    if (target_plmn_local_nonprim) {
        plmn_id_free(target_plmn_local_nonprim);
        target_plmn_local_nonprim = NULL;
    }
    if (target_snpn_local_nonprim) {
        plmn_id_nid_free(target_snpn_local_nonprim);
        target_snpn_local_nonprim = NULL;
    }
    if (target_snssai_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, target_snssai_listList) {
            snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(target_snssai_listList);
        target_snssai_listList = NULL;
    }
    if (target_nsi_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, target_nsi_listList) {
            free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(target_nsi_listList);
        target_nsi_listList = NULL;
    }
    if (analytics_idsList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, analytics_idsList) {
            object_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(analytics_idsList);
        analytics_idsList = NULL;
    }
    if (requester_inter_ind_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, requester_inter_ind_listList) {
            ml_model_inter_ind_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(requester_inter_ind_listList);
        requester_inter_ind_listList = NULL;
    }
    if (source_vendor_id_local_nonprim) {
        object_free(source_vendor_id_local_nonprim);
        source_vendor_id_local_nonprim = NULL;
    }
    return NULL;

}
