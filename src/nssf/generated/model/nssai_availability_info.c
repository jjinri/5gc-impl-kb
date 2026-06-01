#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nssai_availability_info.h"



nssai_availability_info_t *nssai_availability_info_create(
    list_t *supported_nssai_availability_data,
    char *supported_features,
    char *amf_set_id
    ) {
    nssai_availability_info_t *nssai_availability_info_local_var = malloc(sizeof(nssai_availability_info_t));
    if (!nssai_availability_info_local_var) {
        return NULL;
    }
    nssai_availability_info_local_var->supported_nssai_availability_data = supported_nssai_availability_data;
    nssai_availability_info_local_var->supported_features = supported_features;
    nssai_availability_info_local_var->amf_set_id = amf_set_id;

    return nssai_availability_info_local_var;
}


void nssai_availability_info_free(nssai_availability_info_t *nssai_availability_info) {
    if(NULL == nssai_availability_info){
        return ;
    }
    listEntry_t *listEntry;
    if (nssai_availability_info->supported_nssai_availability_data) {
        list_ForEach(listEntry, nssai_availability_info->supported_nssai_availability_data) {
            supported_nssai_availability_data_free(listEntry->data);
        }
        list_freeList(nssai_availability_info->supported_nssai_availability_data);
        nssai_availability_info->supported_nssai_availability_data = NULL;
    }
    if (nssai_availability_info->supported_features) {
        free(nssai_availability_info->supported_features);
        nssai_availability_info->supported_features = NULL;
    }
    if (nssai_availability_info->amf_set_id) {
        free(nssai_availability_info->amf_set_id);
        nssai_availability_info->amf_set_id = NULL;
    }
    free(nssai_availability_info);
}

cJSON *nssai_availability_info_convertToJSON(nssai_availability_info_t *nssai_availability_info) {
    cJSON *item = cJSON_CreateObject();

    // nssai_availability_info->supported_nssai_availability_data
    if (!nssai_availability_info->supported_nssai_availability_data) {
        goto fail;
    }
    cJSON *supported_nssai_availability_data = cJSON_AddArrayToObject(item, "supportedNssaiAvailabilityData");
    if(supported_nssai_availability_data == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *supported_nssai_availability_dataListEntry;
    if (nssai_availability_info->supported_nssai_availability_data) {
    list_ForEach(supported_nssai_availability_dataListEntry, nssai_availability_info->supported_nssai_availability_data) {
    cJSON *itemLocal = supported_nssai_availability_data_convertToJSON(supported_nssai_availability_dataListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(supported_nssai_availability_data, itemLocal);
    }
    }


    // nssai_availability_info->supported_features
    if(nssai_availability_info->supported_features) {
    if(cJSON_AddStringToObject(item, "supportedFeatures", nssai_availability_info->supported_features) == NULL) {
    goto fail; //String
    }
    }


    // nssai_availability_info->amf_set_id
    if(nssai_availability_info->amf_set_id) {
    if(cJSON_AddStringToObject(item, "amfSetId", nssai_availability_info->amf_set_id) == NULL) {
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

nssai_availability_info_t *nssai_availability_info_parseFromJSON(cJSON *nssai_availability_infoJSON){

    nssai_availability_info_t *nssai_availability_info_local_var = NULL;

    // define the local list for nssai_availability_info->supported_nssai_availability_data
    list_t *supported_nssai_availability_dataList = NULL;

    // nssai_availability_info->supported_nssai_availability_data
    cJSON *supported_nssai_availability_data = cJSON_GetObjectItemCaseSensitive(nssai_availability_infoJSON, "supportedNssaiAvailabilityData");
    if (!supported_nssai_availability_data) {
        goto end;
    }

    
    cJSON *supported_nssai_availability_data_local_nonprimitive = NULL;
    if(!cJSON_IsArray(supported_nssai_availability_data)){
        goto end; //nonprimitive container
    }

    supported_nssai_availability_dataList = list_createList();

    cJSON_ArrayForEach(supported_nssai_availability_data_local_nonprimitive,supported_nssai_availability_data )
    {
        if(!cJSON_IsObject(supported_nssai_availability_data_local_nonprimitive)){
            goto end;
        }
        supported_nssai_availability_data_t *supported_nssai_availability_dataItem = supported_nssai_availability_data_parseFromJSON(supported_nssai_availability_data_local_nonprimitive);

        list_addElement(supported_nssai_availability_dataList, supported_nssai_availability_dataItem);
    }

    // nssai_availability_info->supported_features
    cJSON *supported_features = cJSON_GetObjectItemCaseSensitive(nssai_availability_infoJSON, "supportedFeatures");
    if (supported_features) { 
    if(!cJSON_IsString(supported_features) && !cJSON_IsNull(supported_features))
    {
    goto end; //String
    }
    }

    // nssai_availability_info->amf_set_id
    cJSON *amf_set_id = cJSON_GetObjectItemCaseSensitive(nssai_availability_infoJSON, "amfSetId");
    if (amf_set_id) { 
    if(!cJSON_IsString(amf_set_id) && !cJSON_IsNull(amf_set_id))
    {
    goto end; //String
    }
    }


    nssai_availability_info_local_var = nssai_availability_info_create (
        supported_nssai_availability_dataList,
        supported_features && !cJSON_IsNull(supported_features) ? strdup(supported_features->valuestring) : NULL,
        amf_set_id && !cJSON_IsNull(amf_set_id) ? strdup(amf_set_id->valuestring) : NULL
        );

    return nssai_availability_info_local_var;
end:
    if (supported_nssai_availability_dataList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, supported_nssai_availability_dataList) {
            supported_nssai_availability_data_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(supported_nssai_availability_dataList);
        supported_nssai_availability_dataList = NULL;
    }
    return NULL;

}
