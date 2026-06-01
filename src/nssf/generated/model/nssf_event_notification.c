#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nssf_event_notification.h"



nssf_event_notification_t *nssf_event_notification_create(
    char *subscription_id,
    list_t *authorized_nssai_availability_data,
    list_t *alt_nssai,
    list_t *unavailable_nsi_list,
    list_t* nssai_validity_time_info,
    list_t* nssai_validity_time_info_list
    ) {
    nssf_event_notification_t *nssf_event_notification_local_var = malloc(sizeof(nssf_event_notification_t));
    if (!nssf_event_notification_local_var) {
        return NULL;
    }
    nssf_event_notification_local_var->subscription_id = subscription_id;
    nssf_event_notification_local_var->authorized_nssai_availability_data = authorized_nssai_availability_data;
    nssf_event_notification_local_var->alt_nssai = alt_nssai;
    nssf_event_notification_local_var->unavailable_nsi_list = unavailable_nsi_list;
    nssf_event_notification_local_var->nssai_validity_time_info = nssai_validity_time_info;
    nssf_event_notification_local_var->nssai_validity_time_info_list = nssai_validity_time_info_list;

    return nssf_event_notification_local_var;
}


void nssf_event_notification_free(nssf_event_notification_t *nssf_event_notification) {
    if(NULL == nssf_event_notification){
        return ;
    }
    listEntry_t *listEntry;
    if (nssf_event_notification->subscription_id) {
        free(nssf_event_notification->subscription_id);
        nssf_event_notification->subscription_id = NULL;
    }
    if (nssf_event_notification->authorized_nssai_availability_data) {
        list_ForEach(listEntry, nssf_event_notification->authorized_nssai_availability_data) {
            authorized_nssai_availability_data_free(listEntry->data);
        }
        list_freeList(nssf_event_notification->authorized_nssai_availability_data);
        nssf_event_notification->authorized_nssai_availability_data = NULL;
    }
    if (nssf_event_notification->alt_nssai) {
        list_ForEach(listEntry, nssf_event_notification->alt_nssai) {
            snssai_replace_info_free(listEntry->data);
        }
        list_freeList(nssf_event_notification->alt_nssai);
        nssf_event_notification->alt_nssai = NULL;
    }
    if (nssf_event_notification->unavailable_nsi_list) {
        list_ForEach(listEntry, nssf_event_notification->unavailable_nsi_list) {
            free(listEntry->data);
        }
        list_freeList(nssf_event_notification->unavailable_nsi_list);
        nssf_event_notification->unavailable_nsi_list = NULL;
    }
    if (nssf_event_notification->nssai_validity_time_info) {
        list_ForEach(listEntry, nssf_event_notification->nssai_validity_time_info) {
            keyValuePair_t *localKeyValue = (keyValuePair_t*) listEntry->data;
            free (localKeyValue->key);
            free (localKeyValue->value);
            keyValuePair_free(localKeyValue);
        }
        list_freeList(nssf_event_notification->nssai_validity_time_info);
        nssf_event_notification->nssai_validity_time_info = NULL;
    }
    if (nssf_event_notification->nssai_validity_time_info_list) {
        list_ForEach(listEntry, nssf_event_notification->nssai_validity_time_info_list) {
            keyValuePair_t *localKeyValue = (keyValuePair_t*) listEntry->data;
            free (localKeyValue->key);
            free (localKeyValue->value);
            keyValuePair_free(localKeyValue);
        }
        list_freeList(nssf_event_notification->nssai_validity_time_info_list);
        nssf_event_notification->nssai_validity_time_info_list = NULL;
    }
    free(nssf_event_notification);
}

cJSON *nssf_event_notification_convertToJSON(nssf_event_notification_t *nssf_event_notification) {
    cJSON *item = cJSON_CreateObject();

    // nssf_event_notification->subscription_id
    if (!nssf_event_notification->subscription_id) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "subscriptionId", nssf_event_notification->subscription_id) == NULL) {
    goto fail; //String
    }


    // nssf_event_notification->authorized_nssai_availability_data
    if(nssf_event_notification->authorized_nssai_availability_data) {
    cJSON *authorized_nssai_availability_data = cJSON_AddArrayToObject(item, "authorizedNssaiAvailabilityData");
    if(authorized_nssai_availability_data == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *authorized_nssai_availability_dataListEntry;
    if (nssf_event_notification->authorized_nssai_availability_data) {
    list_ForEach(authorized_nssai_availability_dataListEntry, nssf_event_notification->authorized_nssai_availability_data) {
    cJSON *itemLocal = authorized_nssai_availability_data_convertToJSON(authorized_nssai_availability_dataListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(authorized_nssai_availability_data, itemLocal);
    }
    }
    }


    // nssf_event_notification->alt_nssai
    if(nssf_event_notification->alt_nssai) {
    cJSON *alt_nssai = cJSON_AddArrayToObject(item, "altNssai");
    if(alt_nssai == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *alt_nssaiListEntry;
    if (nssf_event_notification->alt_nssai) {
    list_ForEach(alt_nssaiListEntry, nssf_event_notification->alt_nssai) {
    cJSON *itemLocal = snssai_replace_info_convertToJSON(alt_nssaiListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(alt_nssai, itemLocal);
    }
    }
    }


    // nssf_event_notification->unavailable_nsi_list
    if(nssf_event_notification->unavailable_nsi_list) {
    cJSON *unavailable_nsi_list = cJSON_AddArrayToObject(item, "unavailableNsiList");
    if(unavailable_nsi_list == NULL) {
        goto fail; //primitive container
    }

    listEntry_t *unavailable_nsi_listListEntry;
    list_ForEach(unavailable_nsi_listListEntry, nssf_event_notification->unavailable_nsi_list) {
    if(cJSON_AddStringToObject(unavailable_nsi_list, "", (char*)unavailable_nsi_listListEntry->data) == NULL)
    {
        goto fail;
    }
    }
    }


    // nssf_event_notification->nssai_validity_time_info
    if(nssf_event_notification->nssai_validity_time_info) {
    cJSON *nssai_validity_time_info = cJSON_AddObjectToObject(item, "nssaiValidityTimeInfo");
    if(nssai_validity_time_info == NULL) {
        goto fail; //primitive map container
    }
    cJSON *localMapObject = nssai_validity_time_info;
    listEntry_t *nssai_validity_time_infoListEntry;
    if (nssf_event_notification->nssai_validity_time_info) {
    list_ForEach(nssai_validity_time_infoListEntry, nssf_event_notification->nssai_validity_time_info) {
        keyValuePair_t *localKeyValue = (keyValuePair_t*)nssai_validity_time_infoListEntry->data;
    }
    }
    }


    // nssf_event_notification->nssai_validity_time_info_list
    if(nssf_event_notification->nssai_validity_time_info_list) {
    cJSON *nssai_validity_time_info_list = cJSON_AddObjectToObject(item, "nssaiValidityTimeInfoList");
    if(nssai_validity_time_info_list == NULL) {
        goto fail; //primitive map container
    }
    cJSON *localMapObject = nssai_validity_time_info_list;
    listEntry_t *nssai_validity_time_info_listListEntry;
    if (nssf_event_notification->nssai_validity_time_info_list) {
    list_ForEach(nssai_validity_time_info_listListEntry, nssf_event_notification->nssai_validity_time_info_list) {
        keyValuePair_t *localKeyValue = (keyValuePair_t*)nssai_validity_time_info_listListEntry->data;
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

nssf_event_notification_t *nssf_event_notification_parseFromJSON(cJSON *nssf_event_notificationJSON){

    nssf_event_notification_t *nssf_event_notification_local_var = NULL;

    // define the local list for nssf_event_notification->authorized_nssai_availability_data
    list_t *authorized_nssai_availability_dataList = NULL;

    // define the local list for nssf_event_notification->alt_nssai
    list_t *alt_nssaiList = NULL;

    // define the local list for nssf_event_notification->unavailable_nsi_list
    list_t *unavailable_nsi_listList = NULL;

    // define the local map for nssf_event_notification->nssai_validity_time_info
    list_t *nssai_validity_time_infoList = NULL;

    // define the local map for nssf_event_notification->nssai_validity_time_info_list
    list_t *nssai_validity_time_info_listList = NULL;

    // nssf_event_notification->subscription_id
    cJSON *subscription_id = cJSON_GetObjectItemCaseSensitive(nssf_event_notificationJSON, "subscriptionId");
    if (!subscription_id) {
        goto end;
    }

    
    if(!cJSON_IsString(subscription_id))
    {
    goto end; //String
    }

    // nssf_event_notification->authorized_nssai_availability_data
    cJSON *authorized_nssai_availability_data = cJSON_GetObjectItemCaseSensitive(nssf_event_notificationJSON, "authorizedNssaiAvailabilityData");
    if (authorized_nssai_availability_data) { 
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
    }

    // nssf_event_notification->alt_nssai
    cJSON *alt_nssai = cJSON_GetObjectItemCaseSensitive(nssf_event_notificationJSON, "altNssai");
    if (alt_nssai) { 
    cJSON *alt_nssai_local_nonprimitive = NULL;
    if(!cJSON_IsArray(alt_nssai)){
        goto end; //nonprimitive container
    }

    alt_nssaiList = list_createList();

    cJSON_ArrayForEach(alt_nssai_local_nonprimitive,alt_nssai )
    {
        if(!cJSON_IsObject(alt_nssai_local_nonprimitive)){
            goto end;
        }
        snssai_replace_info_t *alt_nssaiItem = snssai_replace_info_parseFromJSON(alt_nssai_local_nonprimitive);

        list_addElement(alt_nssaiList, alt_nssaiItem);
    }
    }

    // nssf_event_notification->unavailable_nsi_list
    cJSON *unavailable_nsi_list = cJSON_GetObjectItemCaseSensitive(nssf_event_notificationJSON, "unavailableNsiList");
    if (unavailable_nsi_list) { 
    cJSON *unavailable_nsi_list_local = NULL;
    if(!cJSON_IsArray(unavailable_nsi_list)) {
        goto end;//primitive container
    }
    unavailable_nsi_listList = list_createList();

    cJSON_ArrayForEach(unavailable_nsi_list_local, unavailable_nsi_list)
    {
        if(!cJSON_IsString(unavailable_nsi_list_local))
        {
            goto end;
        }
        list_addElement(unavailable_nsi_listList , strdup(unavailable_nsi_list_local->valuestring));
    }
    }

    // nssf_event_notification->nssai_validity_time_info
    cJSON *nssai_validity_time_info = cJSON_GetObjectItemCaseSensitive(nssf_event_notificationJSON, "nssaiValidityTimeInfo");
    if (nssai_validity_time_info) { 
    cJSON *nssai_validity_time_info_local_map = NULL;
    if(!cJSON_IsObject(nssai_validity_time_info) && !cJSON_IsNull(nssai_validity_time_info))
    {
        goto end;//primitive map container
    }
    if(cJSON_IsObject(nssai_validity_time_info))
    {
        nssai_validity_time_infoList = list_createList();
        keyValuePair_t *localMapKeyPair;
        cJSON_ArrayForEach(nssai_validity_time_info_local_map, nssai_validity_time_info)
        {
            cJSON *localMapObject = nssai_validity_time_info_local_map;
            list_addElement(nssai_validity_time_infoList , localMapKeyPair);
        }
    }
    }

    // nssf_event_notification->nssai_validity_time_info_list
    cJSON *nssai_validity_time_info_list = cJSON_GetObjectItemCaseSensitive(nssf_event_notificationJSON, "nssaiValidityTimeInfoList");
    if (nssai_validity_time_info_list) { 
    cJSON *nssai_validity_time_info_list_local_map = NULL;
    if(!cJSON_IsObject(nssai_validity_time_info_list) && !cJSON_IsNull(nssai_validity_time_info_list))
    {
        goto end;//primitive map container
    }
    if(cJSON_IsObject(nssai_validity_time_info_list))
    {
        nssai_validity_time_info_listList = list_createList();
        keyValuePair_t *localMapKeyPair;
        cJSON_ArrayForEach(nssai_validity_time_info_list_local_map, nssai_validity_time_info_list)
        {
            cJSON *localMapObject = nssai_validity_time_info_list_local_map;
            list_addElement(nssai_validity_time_info_listList , localMapKeyPair);
        }
    }
    }


    nssf_event_notification_local_var = nssf_event_notification_create (
        strdup(subscription_id->valuestring),
        authorized_nssai_availability_data ? authorized_nssai_availability_dataList : NULL,
        alt_nssai ? alt_nssaiList : NULL,
        unavailable_nsi_list ? unavailable_nsi_listList : NULL,
        nssai_validity_time_info ? nssai_validity_time_infoList : NULL,
        nssai_validity_time_info_list ? nssai_validity_time_info_listList : NULL
        );

    return nssf_event_notification_local_var;
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
    if (alt_nssaiList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, alt_nssaiList) {
            snssai_replace_info_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(alt_nssaiList);
        alt_nssaiList = NULL;
    }
    if (unavailable_nsi_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, unavailable_nsi_listList) {
            free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(unavailable_nsi_listList);
        unavailable_nsi_listList = NULL;
    }
    if (nssai_validity_time_infoList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, nssai_validity_time_infoList) {
            keyValuePair_t *localKeyValue = (keyValuePair_t*) listEntry->data;
            free(localKeyValue->key);
            localKeyValue->key = NULL;
            keyValuePair_free(localKeyValue);
            localKeyValue = NULL;
        }
        list_freeList(nssai_validity_time_infoList);
        nssai_validity_time_infoList = NULL;
    }
    if (nssai_validity_time_info_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, nssai_validity_time_info_listList) {
            keyValuePair_t *localKeyValue = (keyValuePair_t*) listEntry->data;
            free(localKeyValue->key);
            localKeyValue->key = NULL;
            keyValuePair_free(localKeyValue);
            localKeyValue = NULL;
        }
        list_freeList(nssai_validity_time_info_listList);
        nssai_validity_time_info_listList = NULL;
    }
    return NULL;

}
