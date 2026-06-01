#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "authorized_nssai_availability_info.h"



authorized_nssai_availability_info_t *authorized_nssai_availability_info_create(
    list_t *authorized_nssai_availability_data,
    char *supported_features
    ) {
    authorized_nssai_availability_info_t *authorized_nssai_availability_info_local_var = malloc(sizeof(authorized_nssai_availability_info_t));
    if (!authorized_nssai_availability_info_local_var) {
        return NULL;
    }
    authorized_nssai_availability_info_local_var->authorized_nssai_availability_data = authorized_nssai_availability_data;
    authorized_nssai_availability_info_local_var->supported_features = supported_features;

    return authorized_nssai_availability_info_local_var;
}


void authorized_nssai_availability_info_free(authorized_nssai_availability_info_t *authorized_nssai_availability_info) {
    if(NULL == authorized_nssai_availability_info){
        return ;
    }
    listEntry_t *listEntry;
    if (authorized_nssai_availability_info->authorized_nssai_availability_data) {
        list_ForEach(listEntry, authorized_nssai_availability_info->authorized_nssai_availability_data) {
            authorized_nssai_availability_data_free(listEntry->data);
        }
        list_freeList(authorized_nssai_availability_info->authorized_nssai_availability_data);
        authorized_nssai_availability_info->authorized_nssai_availability_data = NULL;
    }
    if (authorized_nssai_availability_info->supported_features) {
        free(authorized_nssai_availability_info->supported_features);
        authorized_nssai_availability_info->supported_features = NULL;
    }
    free(authorized_nssai_availability_info);
}

cJSON *authorized_nssai_availability_info_convertToJSON(authorized_nssai_availability_info_t *authorized_nssai_availability_info) {
    cJSON *item = cJSON_CreateObject();

    // authorized_nssai_availability_info->authorized_nssai_availability_data
    if (!authorized_nssai_availability_info->authorized_nssai_availability_data) {
        goto fail;
    }
    cJSON *authorized_nssai_availability_data = cJSON_AddArrayToObject(item, "authorizedNssaiAvailabilityData");
    if(authorized_nssai_availability_data == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *authorized_nssai_availability_dataListEntry;
    if (authorized_nssai_availability_info->authorized_nssai_availability_data) {
    list_ForEach(authorized_nssai_availability_dataListEntry, authorized_nssai_availability_info->authorized_nssai_availability_data) {
    cJSON *itemLocal = authorized_nssai_availability_data_convertToJSON(authorized_nssai_availability_dataListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(authorized_nssai_availability_data, itemLocal);
    }
    }


    // authorized_nssai_availability_info->supported_features
    if(authorized_nssai_availability_info->supported_features) {
    if(cJSON_AddStringToObject(item, "supportedFeatures", authorized_nssai_availability_info->supported_features) == NULL) {
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

authorized_nssai_availability_info_t *authorized_nssai_availability_info_parseFromJSON(cJSON *authorized_nssai_availability_infoJSON){

    authorized_nssai_availability_info_t *authorized_nssai_availability_info_local_var = NULL;

    // define the local list for authorized_nssai_availability_info->authorized_nssai_availability_data
    list_t *authorized_nssai_availability_dataList = NULL;

    // authorized_nssai_availability_info->authorized_nssai_availability_data
    cJSON *authorized_nssai_availability_data = cJSON_GetObjectItemCaseSensitive(authorized_nssai_availability_infoJSON, "authorizedNssaiAvailabilityData");
    if (!authorized_nssai_availability_data) {
        goto end;
    }

    
    cJSON *authorized_nssai_availability_data_local_nonprimitive = NULL;
    if(!cJSON_IsArray(authorized_nssai_availability_data)){
        goto end; //nonprimitive container
    }

    authorized_nssai_availability_dataList = list_createList();

    cJSON_ArrayForEach(authorized_nssai_availability_data_local_nonprimitive,authorized_nssai_availability_data )
    {
        if(!cJSON_IsObject(authorized_nssai_availability_data_local_nonprimitive)){
            goto end;
        }
        authorized_nssai_availability_data_t *authorized_nssai_availability_dataItem = authorized_nssai_availability_data_parseFromJSON(authorized_nssai_availability_data_local_nonprimitive);

        list_addElement(authorized_nssai_availability_dataList, authorized_nssai_availability_dataItem);
    }

    // authorized_nssai_availability_info->supported_features
    cJSON *supported_features = cJSON_GetObjectItemCaseSensitive(authorized_nssai_availability_infoJSON, "supportedFeatures");
    if (supported_features) { 
    if(!cJSON_IsString(supported_features) && !cJSON_IsNull(supported_features))
    {
    goto end; //String
    }
    }


    authorized_nssai_availability_info_local_var = authorized_nssai_availability_info_create (
        authorized_nssai_availability_dataList,
        supported_features && !cJSON_IsNull(supported_features) ? strdup(supported_features->valuestring) : NULL
        );

    return authorized_nssai_availability_info_local_var;
end:
    if (authorized_nssai_availability_dataList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, authorized_nssai_availability_dataList) {
            authorized_nssai_availability_data_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(authorized_nssai_availability_dataList);
        authorized_nssai_availability_dataList = NULL;
    }
    return NULL;

}
