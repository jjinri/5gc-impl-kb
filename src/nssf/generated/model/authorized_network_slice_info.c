#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "authorized_network_slice_info.h"



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
    ) {
    authorized_network_slice_info_t *authorized_network_slice_info_local_var = malloc(sizeof(authorized_network_slice_info_t));
    if (!authorized_network_slice_info_local_var) {
        return NULL;
    }
    authorized_network_slice_info_local_var->allowed_nssai_list = allowed_nssai_list;
    authorized_network_slice_info_local_var->configured_nssai = configured_nssai;
    authorized_network_slice_info_local_var->target_amf_set = target_amf_set;
    authorized_network_slice_info_local_var->candidate_amf_list = candidate_amf_list;
    authorized_network_slice_info_local_var->rejected_nssai_in_plmn = rejected_nssai_in_plmn;
    authorized_network_slice_info_local_var->rejected_nssai_in_ta = rejected_nssai_in_ta;
    authorized_network_slice_info_local_var->nsi_information = nsi_information;
    authorized_network_slice_info_local_var->supported_features = supported_features;
    authorized_network_slice_info_local_var->nrf_amf_set = nrf_amf_set;
    authorized_network_slice_info_local_var->nrf_amf_set_nf_mgt_uri = nrf_amf_set_nf_mgt_uri;
    authorized_network_slice_info_local_var->nrf_amf_set_access_token_uri = nrf_amf_set_access_token_uri;
    authorized_network_slice_info_local_var->nrf_oauth2_required = nrf_oauth2_required;
    authorized_network_slice_info_local_var->target_amf_service_set = target_amf_service_set;
    authorized_network_slice_info_local_var->target_nssai = target_nssai;
    authorized_network_slice_info_local_var->nsag_infos = nsag_infos;
    authorized_network_slice_info_local_var->mapping_of_nssai = mapping_of_nssai;
    authorized_network_slice_info_local_var->snssai_info_rsp_data = snssai_info_rsp_data;

    return authorized_network_slice_info_local_var;
}


void authorized_network_slice_info_free(authorized_network_slice_info_t *authorized_network_slice_info) {
    if(NULL == authorized_network_slice_info){
        return ;
    }
    listEntry_t *listEntry;
    if (authorized_network_slice_info->allowed_nssai_list) {
        list_ForEach(listEntry, authorized_network_slice_info->allowed_nssai_list) {
            allowed_nssai_free(listEntry->data);
        }
        list_freeList(authorized_network_slice_info->allowed_nssai_list);
        authorized_network_slice_info->allowed_nssai_list = NULL;
    }
    if (authorized_network_slice_info->configured_nssai) {
        list_ForEach(listEntry, authorized_network_slice_info->configured_nssai) {
            configured_snssai_free(listEntry->data);
        }
        list_freeList(authorized_network_slice_info->configured_nssai);
        authorized_network_slice_info->configured_nssai = NULL;
    }
    if (authorized_network_slice_info->target_amf_set) {
        free(authorized_network_slice_info->target_amf_set);
        authorized_network_slice_info->target_amf_set = NULL;
    }
    if (authorized_network_slice_info->candidate_amf_list) {
        list_ForEach(listEntry, authorized_network_slice_info->candidate_amf_list) {
            free(listEntry->data);
        }
        list_freeList(authorized_network_slice_info->candidate_amf_list);
        authorized_network_slice_info->candidate_amf_list = NULL;
    }
    if (authorized_network_slice_info->rejected_nssai_in_plmn) {
        list_ForEach(listEntry, authorized_network_slice_info->rejected_nssai_in_plmn) {
            snssai_free(listEntry->data);
        }
        list_freeList(authorized_network_slice_info->rejected_nssai_in_plmn);
        authorized_network_slice_info->rejected_nssai_in_plmn = NULL;
    }
    if (authorized_network_slice_info->rejected_nssai_in_ta) {
        list_ForEach(listEntry, authorized_network_slice_info->rejected_nssai_in_ta) {
            snssai_free(listEntry->data);
        }
        list_freeList(authorized_network_slice_info->rejected_nssai_in_ta);
        authorized_network_slice_info->rejected_nssai_in_ta = NULL;
    }
    if (authorized_network_slice_info->nsi_information) {
        nsi_information_free(authorized_network_slice_info->nsi_information);
        authorized_network_slice_info->nsi_information = NULL;
    }
    if (authorized_network_slice_info->supported_features) {
        free(authorized_network_slice_info->supported_features);
        authorized_network_slice_info->supported_features = NULL;
    }
    if (authorized_network_slice_info->nrf_amf_set) {
        free(authorized_network_slice_info->nrf_amf_set);
        authorized_network_slice_info->nrf_amf_set = NULL;
    }
    if (authorized_network_slice_info->nrf_amf_set_nf_mgt_uri) {
        free(authorized_network_slice_info->nrf_amf_set_nf_mgt_uri);
        authorized_network_slice_info->nrf_amf_set_nf_mgt_uri = NULL;
    }
    if (authorized_network_slice_info->nrf_amf_set_access_token_uri) {
        free(authorized_network_slice_info->nrf_amf_set_access_token_uri);
        authorized_network_slice_info->nrf_amf_set_access_token_uri = NULL;
    }
    if (authorized_network_slice_info->nrf_oauth2_required) {
        list_ForEach(listEntry, authorized_network_slice_info->nrf_oauth2_required) {
            keyValuePair_t *localKeyValue = (keyValuePair_t*) listEntry->data;
            free (localKeyValue->key);
            free (localKeyValue->value);
            keyValuePair_free(localKeyValue);
        }
        list_freeList(authorized_network_slice_info->nrf_oauth2_required);
        authorized_network_slice_info->nrf_oauth2_required = NULL;
    }
    if (authorized_network_slice_info->target_amf_service_set) {
        free(authorized_network_slice_info->target_amf_service_set);
        authorized_network_slice_info->target_amf_service_set = NULL;
    }
    if (authorized_network_slice_info->target_nssai) {
        list_ForEach(listEntry, authorized_network_slice_info->target_nssai) {
            snssai_free(listEntry->data);
        }
        list_freeList(authorized_network_slice_info->target_nssai);
        authorized_network_slice_info->target_nssai = NULL;
    }
    if (authorized_network_slice_info->nsag_infos) {
        list_ForEach(listEntry, authorized_network_slice_info->nsag_infos) {
            nsag_info_free(listEntry->data);
        }
        list_freeList(authorized_network_slice_info->nsag_infos);
        authorized_network_slice_info->nsag_infos = NULL;
    }
    if (authorized_network_slice_info->mapping_of_nssai) {
        list_ForEach(listEntry, authorized_network_slice_info->mapping_of_nssai) {
            mapping_of_snssai_free(listEntry->data);
        }
        list_freeList(authorized_network_slice_info->mapping_of_nssai);
        authorized_network_slice_info->mapping_of_nssai = NULL;
    }
    if (authorized_network_slice_info->snssai_info_rsp_data) {
        list_ForEach(listEntry, authorized_network_slice_info->snssai_info_rsp_data) {
            keyValuePair_t *localKeyValue = (keyValuePair_t*) listEntry->data;
            free (localKeyValue->key);
            free (localKeyValue->value);
            keyValuePair_free(localKeyValue);
        }
        list_freeList(authorized_network_slice_info->snssai_info_rsp_data);
        authorized_network_slice_info->snssai_info_rsp_data = NULL;
    }
    free(authorized_network_slice_info);
}

cJSON *authorized_network_slice_info_convertToJSON(authorized_network_slice_info_t *authorized_network_slice_info) {
    cJSON *item = cJSON_CreateObject();

    // authorized_network_slice_info->allowed_nssai_list
    if(authorized_network_slice_info->allowed_nssai_list) {
    cJSON *allowed_nssai_list = cJSON_AddArrayToObject(item, "allowedNssaiList");
    if(allowed_nssai_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *allowed_nssai_listListEntry;
    if (authorized_network_slice_info->allowed_nssai_list) {
    list_ForEach(allowed_nssai_listListEntry, authorized_network_slice_info->allowed_nssai_list) {
    cJSON *itemLocal = allowed_nssai_convertToJSON(allowed_nssai_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(allowed_nssai_list, itemLocal);
    }
    }
    }


    // authorized_network_slice_info->configured_nssai
    if(authorized_network_slice_info->configured_nssai) {
    cJSON *configured_nssai = cJSON_AddArrayToObject(item, "configuredNssai");
    if(configured_nssai == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *configured_nssaiListEntry;
    if (authorized_network_slice_info->configured_nssai) {
    list_ForEach(configured_nssaiListEntry, authorized_network_slice_info->configured_nssai) {
    cJSON *itemLocal = configured_snssai_convertToJSON(configured_nssaiListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(configured_nssai, itemLocal);
    }
    }
    }


    // authorized_network_slice_info->target_amf_set
    if(authorized_network_slice_info->target_amf_set) {
    if(cJSON_AddStringToObject(item, "targetAmfSet", authorized_network_slice_info->target_amf_set) == NULL) {
    goto fail; //String
    }
    }


    // authorized_network_slice_info->candidate_amf_list
    if(authorized_network_slice_info->candidate_amf_list) {
    cJSON *candidate_amf_list = cJSON_AddArrayToObject(item, "candidateAmfList");
    if(candidate_amf_list == NULL) {
        goto fail; //primitive container
    }

    listEntry_t *candidate_amf_listListEntry;
    list_ForEach(candidate_amf_listListEntry, authorized_network_slice_info->candidate_amf_list) {
    if(cJSON_AddStringToObject(candidate_amf_list, "", (char*)candidate_amf_listListEntry->data) == NULL)
    {
        goto fail;
    }
    }
    }


    // authorized_network_slice_info->rejected_nssai_in_plmn
    if(authorized_network_slice_info->rejected_nssai_in_plmn) {
    cJSON *rejected_nssai_in_plmn = cJSON_AddArrayToObject(item, "rejectedNssaiInPlmn");
    if(rejected_nssai_in_plmn == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *rejected_nssai_in_plmnListEntry;
    if (authorized_network_slice_info->rejected_nssai_in_plmn) {
    list_ForEach(rejected_nssai_in_plmnListEntry, authorized_network_slice_info->rejected_nssai_in_plmn) {
    cJSON *itemLocal = snssai_convertToJSON(rejected_nssai_in_plmnListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(rejected_nssai_in_plmn, itemLocal);
    }
    }
    }


    // authorized_network_slice_info->rejected_nssai_in_ta
    if(authorized_network_slice_info->rejected_nssai_in_ta) {
    cJSON *rejected_nssai_in_ta = cJSON_AddArrayToObject(item, "rejectedNssaiInTa");
    if(rejected_nssai_in_ta == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *rejected_nssai_in_taListEntry;
    if (authorized_network_slice_info->rejected_nssai_in_ta) {
    list_ForEach(rejected_nssai_in_taListEntry, authorized_network_slice_info->rejected_nssai_in_ta) {
    cJSON *itemLocal = snssai_convertToJSON(rejected_nssai_in_taListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(rejected_nssai_in_ta, itemLocal);
    }
    }
    }


    // authorized_network_slice_info->nsi_information
    if(authorized_network_slice_info->nsi_information) {
    cJSON *nsi_information_local_JSON = nsi_information_convertToJSON(authorized_network_slice_info->nsi_information);
    if(nsi_information_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "nsiInformation", nsi_information_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // authorized_network_slice_info->supported_features
    if(authorized_network_slice_info->supported_features) {
    if(cJSON_AddStringToObject(item, "supportedFeatures", authorized_network_slice_info->supported_features) == NULL) {
    goto fail; //String
    }
    }


    // authorized_network_slice_info->nrf_amf_set
    if(authorized_network_slice_info->nrf_amf_set) {
    if(cJSON_AddStringToObject(item, "nrfAmfSet", authorized_network_slice_info->nrf_amf_set) == NULL) {
    goto fail; //String
    }
    }


    // authorized_network_slice_info->nrf_amf_set_nf_mgt_uri
    if(authorized_network_slice_info->nrf_amf_set_nf_mgt_uri) {
    if(cJSON_AddStringToObject(item, "nrfAmfSetNfMgtUri", authorized_network_slice_info->nrf_amf_set_nf_mgt_uri) == NULL) {
    goto fail; //String
    }
    }


    // authorized_network_slice_info->nrf_amf_set_access_token_uri
    if(authorized_network_slice_info->nrf_amf_set_access_token_uri) {
    if(cJSON_AddStringToObject(item, "nrfAmfSetAccessTokenUri", authorized_network_slice_info->nrf_amf_set_access_token_uri) == NULL) {
    goto fail; //String
    }
    }


    // authorized_network_slice_info->nrf_oauth2_required
    if(authorized_network_slice_info->nrf_oauth2_required) {
    cJSON *nrf_oauth2_required = cJSON_AddObjectToObject(item, "nrfOauth2Required");
    if(nrf_oauth2_required == NULL) {
        goto fail; //primitive map container
    }
    cJSON *localMapObject = nrf_oauth2_required;
    listEntry_t *nrf_oauth2_requiredListEntry;
    if (authorized_network_slice_info->nrf_oauth2_required) {
    list_ForEach(nrf_oauth2_requiredListEntry, authorized_network_slice_info->nrf_oauth2_required) {
        keyValuePair_t *localKeyValue = (keyValuePair_t*)nrf_oauth2_requiredListEntry->data;
        if(cJSON_AddBoolToObject(localMapObject, localKeyValue->key, *(cJSON_bool *)localKeyValue->value) == NULL)
        {
            goto fail;
        }
    }
    }
    }


    // authorized_network_slice_info->target_amf_service_set
    if(authorized_network_slice_info->target_amf_service_set) {
    if(cJSON_AddStringToObject(item, "targetAmfServiceSet", authorized_network_slice_info->target_amf_service_set) == NULL) {
    goto fail; //String
    }
    }


    // authorized_network_slice_info->target_nssai
    if(authorized_network_slice_info->target_nssai) {
    cJSON *target_nssai = cJSON_AddArrayToObject(item, "targetNssai");
    if(target_nssai == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *target_nssaiListEntry;
    if (authorized_network_slice_info->target_nssai) {
    list_ForEach(target_nssaiListEntry, authorized_network_slice_info->target_nssai) {
    cJSON *itemLocal = snssai_convertToJSON(target_nssaiListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(target_nssai, itemLocal);
    }
    }
    }


    // authorized_network_slice_info->nsag_infos
    if(authorized_network_slice_info->nsag_infos) {
    cJSON *nsag_infos = cJSON_AddArrayToObject(item, "nsagInfos");
    if(nsag_infos == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *nsag_infosListEntry;
    if (authorized_network_slice_info->nsag_infos) {
    list_ForEach(nsag_infosListEntry, authorized_network_slice_info->nsag_infos) {
    cJSON *itemLocal = nsag_info_convertToJSON(nsag_infosListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(nsag_infos, itemLocal);
    }
    }
    }


    // authorized_network_slice_info->mapping_of_nssai
    if(authorized_network_slice_info->mapping_of_nssai) {
    cJSON *mapping_of_nssai = cJSON_AddArrayToObject(item, "mappingOfNssai");
    if(mapping_of_nssai == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *mapping_of_nssaiListEntry;
    if (authorized_network_slice_info->mapping_of_nssai) {
    list_ForEach(mapping_of_nssaiListEntry, authorized_network_slice_info->mapping_of_nssai) {
    cJSON *itemLocal = mapping_of_snssai_convertToJSON(mapping_of_nssaiListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(mapping_of_nssai, itemLocal);
    }
    }
    }


    // authorized_network_slice_info->snssai_info_rsp_data
    if(authorized_network_slice_info->snssai_info_rsp_data) {
    cJSON *snssai_info_rsp_data = cJSON_AddObjectToObject(item, "snssaiInfoRspData");
    if(snssai_info_rsp_data == NULL) {
        goto fail; //primitive map container
    }
    cJSON *localMapObject = snssai_info_rsp_data;
    listEntry_t *snssai_info_rsp_dataListEntry;
    if (authorized_network_slice_info->snssai_info_rsp_data) {
    list_ForEach(snssai_info_rsp_dataListEntry, authorized_network_slice_info->snssai_info_rsp_data) {
        keyValuePair_t *localKeyValue = (keyValuePair_t*)snssai_info_rsp_dataListEntry->data;
    }
    }
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

authorized_network_slice_info_t *authorized_network_slice_info_parseFromJSON(cJSON *authorized_network_slice_infoJSON){

    authorized_network_slice_info_t *authorized_network_slice_info_local_var = NULL;

    // define the local list for authorized_network_slice_info->allowed_nssai_list
    list_t *allowed_nssai_listList = NULL;

    // define the local list for authorized_network_slice_info->configured_nssai
    list_t *configured_nssaiList = NULL;

    // define the local list for authorized_network_slice_info->candidate_amf_list
    list_t *candidate_amf_listList = NULL;

    // define the local list for authorized_network_slice_info->rejected_nssai_in_plmn
    list_t *rejected_nssai_in_plmnList = NULL;

    // define the local list for authorized_network_slice_info->rejected_nssai_in_ta
    list_t *rejected_nssai_in_taList = NULL;

    // define the local variable for authorized_network_slice_info->nsi_information
    nsi_information_t *nsi_information_local_nonprim = NULL;

    // define the local map for authorized_network_slice_info->nrf_oauth2_required
    list_t *nrf_oauth2_requiredList = NULL;

    // define the local list for authorized_network_slice_info->target_nssai
    list_t *target_nssaiList = NULL;

    // define the local list for authorized_network_slice_info->nsag_infos
    list_t *nsag_infosList = NULL;

    // define the local list for authorized_network_slice_info->mapping_of_nssai
    list_t *mapping_of_nssaiList = NULL;

    // define the local map for authorized_network_slice_info->snssai_info_rsp_data
    list_t *snssai_info_rsp_dataList = NULL;

    // authorized_network_slice_info->allowed_nssai_list
    cJSON *allowed_nssai_list = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "allowedNssaiList");
    if (allowed_nssai_list) { 
    cJSON *allowed_nssai_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(allowed_nssai_list)){
        goto end; //nonprimitive container
    }

    allowed_nssai_listList = list_createList();

    cJSON_ArrayForEach(allowed_nssai_list_local_nonprimitive,allowed_nssai_list )
    {
        if(!cJSON_IsObject(allowed_nssai_list_local_nonprimitive)){
            goto end;
        }
        allowed_nssai_t *allowed_nssai_listItem = allowed_nssai_parseFromJSON(allowed_nssai_list_local_nonprimitive);

        list_addElement(allowed_nssai_listList, allowed_nssai_listItem);
    }
    }

    // authorized_network_slice_info->configured_nssai
    cJSON *configured_nssai = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "configuredNssai");
    if (configured_nssai) { 
    cJSON *configured_nssai_local_nonprimitive = NULL;
    if(!cJSON_IsArray(configured_nssai)){
        goto end; //nonprimitive container
    }

    configured_nssaiList = list_createList();

    cJSON_ArrayForEach(configured_nssai_local_nonprimitive,configured_nssai )
    {
        if(!cJSON_IsObject(configured_nssai_local_nonprimitive)){
            goto end;
        }
        configured_snssai_t *configured_nssaiItem = configured_snssai_parseFromJSON(configured_nssai_local_nonprimitive);

        list_addElement(configured_nssaiList, configured_nssaiItem);
    }
    }

    // authorized_network_slice_info->target_amf_set
    cJSON *target_amf_set = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "targetAmfSet");
    if (target_amf_set) { 
    if(!cJSON_IsString(target_amf_set) && !cJSON_IsNull(target_amf_set))
    {
    goto end; //String
    }
    }

    // authorized_network_slice_info->candidate_amf_list
    cJSON *candidate_amf_list = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "candidateAmfList");
    if (candidate_amf_list) { 
    cJSON *candidate_amf_list_local = NULL;
    if(!cJSON_IsArray(candidate_amf_list)) {
        goto end;//primitive container
    }
    candidate_amf_listList = list_createList();

    cJSON_ArrayForEach(candidate_amf_list_local, candidate_amf_list)
    {
        if(!cJSON_IsString(candidate_amf_list_local))
        {
            goto end;
        }
        list_addElement(candidate_amf_listList , strdup(candidate_amf_list_local->valuestring));
    }
    }

    // authorized_network_slice_info->rejected_nssai_in_plmn
    cJSON *rejected_nssai_in_plmn = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "rejectedNssaiInPlmn");
    if (rejected_nssai_in_plmn) { 
    cJSON *rejected_nssai_in_plmn_local_nonprimitive = NULL;
    if(!cJSON_IsArray(rejected_nssai_in_plmn)){
        goto end; //nonprimitive container
    }

    rejected_nssai_in_plmnList = list_createList();

    cJSON_ArrayForEach(rejected_nssai_in_plmn_local_nonprimitive,rejected_nssai_in_plmn )
    {
        if(!cJSON_IsObject(rejected_nssai_in_plmn_local_nonprimitive)){
            goto end;
        }
        snssai_t *rejected_nssai_in_plmnItem = snssai_parseFromJSON(rejected_nssai_in_plmn_local_nonprimitive);

        list_addElement(rejected_nssai_in_plmnList, rejected_nssai_in_plmnItem);
    }
    }

    // authorized_network_slice_info->rejected_nssai_in_ta
    cJSON *rejected_nssai_in_ta = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "rejectedNssaiInTa");
    if (rejected_nssai_in_ta) { 
    cJSON *rejected_nssai_in_ta_local_nonprimitive = NULL;
    if(!cJSON_IsArray(rejected_nssai_in_ta)){
        goto end; //nonprimitive container
    }

    rejected_nssai_in_taList = list_createList();

    cJSON_ArrayForEach(rejected_nssai_in_ta_local_nonprimitive,rejected_nssai_in_ta )
    {
        if(!cJSON_IsObject(rejected_nssai_in_ta_local_nonprimitive)){
            goto end;
        }
        snssai_t *rejected_nssai_in_taItem = snssai_parseFromJSON(rejected_nssai_in_ta_local_nonprimitive);

        list_addElement(rejected_nssai_in_taList, rejected_nssai_in_taItem);
    }
    }

    // authorized_network_slice_info->nsi_information
    cJSON *nsi_information = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "nsiInformation");
    if (nsi_information) { 
    nsi_information_local_nonprim = nsi_information_parseFromJSON(nsi_information); //nonprimitive
    }

    // authorized_network_slice_info->supported_features
    cJSON *supported_features = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "supportedFeatures");
    if (supported_features) { 
    if(!cJSON_IsString(supported_features) && !cJSON_IsNull(supported_features))
    {
    goto end; //String
    }
    }

    // authorized_network_slice_info->nrf_amf_set
    cJSON *nrf_amf_set = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "nrfAmfSet");
    if (nrf_amf_set) { 
    if(!cJSON_IsString(nrf_amf_set) && !cJSON_IsNull(nrf_amf_set))
    {
    goto end; //String
    }
    }

    // authorized_network_slice_info->nrf_amf_set_nf_mgt_uri
    cJSON *nrf_amf_set_nf_mgt_uri = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "nrfAmfSetNfMgtUri");
    if (nrf_amf_set_nf_mgt_uri) { 
    if(!cJSON_IsString(nrf_amf_set_nf_mgt_uri) && !cJSON_IsNull(nrf_amf_set_nf_mgt_uri))
    {
    goto end; //String
    }
    }

    // authorized_network_slice_info->nrf_amf_set_access_token_uri
    cJSON *nrf_amf_set_access_token_uri = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "nrfAmfSetAccessTokenUri");
    if (nrf_amf_set_access_token_uri) { 
    if(!cJSON_IsString(nrf_amf_set_access_token_uri) && !cJSON_IsNull(nrf_amf_set_access_token_uri))
    {
    goto end; //String
    }
    }

    // authorized_network_slice_info->nrf_oauth2_required
    cJSON *nrf_oauth2_required = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "nrfOauth2Required");
    if (nrf_oauth2_required) { 
    cJSON *nrf_oauth2_required_local_map = NULL;
    if(!cJSON_IsObject(nrf_oauth2_required) && !cJSON_IsNull(nrf_oauth2_required))
    {
        goto end;//primitive map container
    }
    if(cJSON_IsObject(nrf_oauth2_required))
    {
        nrf_oauth2_requiredList = list_createList();
        keyValuePair_t *localMapKeyPair;
        cJSON_ArrayForEach(nrf_oauth2_required_local_map, nrf_oauth2_required)
        {
            cJSON *localMapObject = nrf_oauth2_required_local_map;
            if(!cJSON_IsBool(localMapObject))
            {
                goto end;
            }
            localMapKeyPair = keyValuePair_create(strdup(localMapObject->string), &localMapObject->valueint);
            list_addElement(nrf_oauth2_requiredList , localMapKeyPair);
        }
    }
    }

    // authorized_network_slice_info->target_amf_service_set
    cJSON *target_amf_service_set = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "targetAmfServiceSet");
    if (target_amf_service_set) { 
    if(!cJSON_IsString(target_amf_service_set) && !cJSON_IsNull(target_amf_service_set))
    {
    goto end; //String
    }
    }

    // authorized_network_slice_info->target_nssai
    cJSON *target_nssai = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "targetNssai");
    if (target_nssai) { 
    cJSON *target_nssai_local_nonprimitive = NULL;
    if(!cJSON_IsArray(target_nssai)){
        goto end; //nonprimitive container
    }

    target_nssaiList = list_createList();

    cJSON_ArrayForEach(target_nssai_local_nonprimitive,target_nssai )
    {
        if(!cJSON_IsObject(target_nssai_local_nonprimitive)){
            goto end;
        }
        snssai_t *target_nssaiItem = snssai_parseFromJSON(target_nssai_local_nonprimitive);

        list_addElement(target_nssaiList, target_nssaiItem);
    }
    }

    // authorized_network_slice_info->nsag_infos
    cJSON *nsag_infos = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "nsagInfos");
    if (nsag_infos) { 
    cJSON *nsag_infos_local_nonprimitive = NULL;
    if(!cJSON_IsArray(nsag_infos)){
        goto end; //nonprimitive container
    }

    nsag_infosList = list_createList();

    cJSON_ArrayForEach(nsag_infos_local_nonprimitive,nsag_infos )
    {
        if(!cJSON_IsObject(nsag_infos_local_nonprimitive)){
            goto end;
        }
        nsag_info_t *nsag_infosItem = nsag_info_parseFromJSON(nsag_infos_local_nonprimitive);

        list_addElement(nsag_infosList, nsag_infosItem);
    }
    }

    // authorized_network_slice_info->mapping_of_nssai
    cJSON *mapping_of_nssai = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "mappingOfNssai");
    if (mapping_of_nssai) { 
    cJSON *mapping_of_nssai_local_nonprimitive = NULL;
    if(!cJSON_IsArray(mapping_of_nssai)){
        goto end; //nonprimitive container
    }

    mapping_of_nssaiList = list_createList();

    cJSON_ArrayForEach(mapping_of_nssai_local_nonprimitive,mapping_of_nssai )
    {
        if(!cJSON_IsObject(mapping_of_nssai_local_nonprimitive)){
            goto end;
        }
        mapping_of_snssai_t *mapping_of_nssaiItem = mapping_of_snssai_parseFromJSON(mapping_of_nssai_local_nonprimitive);

        list_addElement(mapping_of_nssaiList, mapping_of_nssaiItem);
    }
    }

    // authorized_network_slice_info->snssai_info_rsp_data
    cJSON *snssai_info_rsp_data = cJSON_GetObjectItemCaseSensitive(authorized_network_slice_infoJSON, "snssaiInfoRspData");
    if (snssai_info_rsp_data) { 

    // The data type of the elements in authorized_network_slice_info->snssai_info_rsp_data is currently not supported.

    }


    authorized_network_slice_info_local_var = authorized_network_slice_info_create (
        allowed_nssai_list ? allowed_nssai_listList : NULL,
        configured_nssai ? configured_nssaiList : NULL,
        target_amf_set && !cJSON_IsNull(target_amf_set) ? strdup(target_amf_set->valuestring) : NULL,
        candidate_amf_list ? candidate_amf_listList : NULL,
        rejected_nssai_in_plmn ? rejected_nssai_in_plmnList : NULL,
        rejected_nssai_in_ta ? rejected_nssai_in_taList : NULL,
        nsi_information ? nsi_information_local_nonprim : NULL,
        supported_features && !cJSON_IsNull(supported_features) ? strdup(supported_features->valuestring) : NULL,
        nrf_amf_set && !cJSON_IsNull(nrf_amf_set) ? strdup(nrf_amf_set->valuestring) : NULL,
        nrf_amf_set_nf_mgt_uri && !cJSON_IsNull(nrf_amf_set_nf_mgt_uri) ? strdup(nrf_amf_set_nf_mgt_uri->valuestring) : NULL,
        nrf_amf_set_access_token_uri && !cJSON_IsNull(nrf_amf_set_access_token_uri) ? strdup(nrf_amf_set_access_token_uri->valuestring) : NULL,
        nrf_oauth2_required ? nrf_oauth2_requiredList : NULL,
        target_amf_service_set && !cJSON_IsNull(target_amf_service_set) ? strdup(target_amf_service_set->valuestring) : NULL,
        target_nssai ? target_nssaiList : NULL,
        nsag_infos ? nsag_infosList : NULL,
        mapping_of_nssai ? mapping_of_nssaiList : NULL,
        snssai_info_rsp_data ? snssai_info_rsp_dataList : NULL
        );

    return authorized_network_slice_info_local_var;
end:
    if (allowed_nssai_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, allowed_nssai_listList) {
            allowed_nssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(allowed_nssai_listList);
        allowed_nssai_listList = NULL;
    }
    if (configured_nssaiList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, configured_nssaiList) {
            configured_snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(configured_nssaiList);
        configured_nssaiList = NULL;
    }
    if (candidate_amf_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, candidate_amf_listList) {
            free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(candidate_amf_listList);
        candidate_amf_listList = NULL;
    }
    if (rejected_nssai_in_plmnList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, rejected_nssai_in_plmnList) {
            snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(rejected_nssai_in_plmnList);
        rejected_nssai_in_plmnList = NULL;
    }
    if (rejected_nssai_in_taList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, rejected_nssai_in_taList) {
            snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(rejected_nssai_in_taList);
        rejected_nssai_in_taList = NULL;
    }
    if (nsi_information_local_nonprim) {
        nsi_information_free(nsi_information_local_nonprim);
        nsi_information_local_nonprim = NULL;
    }
    if (nrf_oauth2_requiredList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, nrf_oauth2_requiredList) {
            keyValuePair_t *localKeyValue = (keyValuePair_t*) listEntry->data;
            free(localKeyValue->key);
            localKeyValue->key = NULL;
            keyValuePair_free(localKeyValue);
            localKeyValue = NULL;
        }
        list_freeList(nrf_oauth2_requiredList);
        nrf_oauth2_requiredList = NULL;
    }
    if (target_nssaiList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, target_nssaiList) {
            snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(target_nssaiList);
        target_nssaiList = NULL;
    }
    if (nsag_infosList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, nsag_infosList) {
            nsag_info_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(nsag_infosList);
        nsag_infosList = NULL;
    }
    if (mapping_of_nssaiList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, mapping_of_nssaiList) {
            mapping_of_snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(mapping_of_nssaiList);
        mapping_of_nssaiList = NULL;
    }

    // The data type of the elements in authorized_network_slice_info->snssai_info_rsp_data is currently not supported.

    return NULL;

}
