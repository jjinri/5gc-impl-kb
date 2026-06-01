#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nsi_unavailability_subscribe_info.h"



nsi_unavailability_subscribe_info_t *nsi_unavailability_subscribe_info_create(
    list_t *nsi_to_subscribe,
    list_t *snssai_to_subscribe
    ) {
    nsi_unavailability_subscribe_info_t *nsi_unavailability_subscribe_info_local_var = malloc(sizeof(nsi_unavailability_subscribe_info_t));
    if (!nsi_unavailability_subscribe_info_local_var) {
        return NULL;
    }
    nsi_unavailability_subscribe_info_local_var->nsi_to_subscribe = nsi_to_subscribe;
    nsi_unavailability_subscribe_info_local_var->snssai_to_subscribe = snssai_to_subscribe;

    return nsi_unavailability_subscribe_info_local_var;
}


void nsi_unavailability_subscribe_info_free(nsi_unavailability_subscribe_info_t *nsi_unavailability_subscribe_info) {
    if(NULL == nsi_unavailability_subscribe_info){
        return ;
    }
    listEntry_t *listEntry;
    if (nsi_unavailability_subscribe_info->nsi_to_subscribe) {
        list_ForEach(listEntry, nsi_unavailability_subscribe_info->nsi_to_subscribe) {
            object_free(listEntry->data);
        }
        list_freeList(nsi_unavailability_subscribe_info->nsi_to_subscribe);
        nsi_unavailability_subscribe_info->nsi_to_subscribe = NULL;
    }
    if (nsi_unavailability_subscribe_info->snssai_to_subscribe) {
        list_ForEach(listEntry, nsi_unavailability_subscribe_info->snssai_to_subscribe) {
            object_free(listEntry->data);
        }
        list_freeList(nsi_unavailability_subscribe_info->snssai_to_subscribe);
        nsi_unavailability_subscribe_info->snssai_to_subscribe = NULL;
    }
    free(nsi_unavailability_subscribe_info);
}

cJSON *nsi_unavailability_subscribe_info_convertToJSON(nsi_unavailability_subscribe_info_t *nsi_unavailability_subscribe_info) {
    cJSON *item = cJSON_CreateObject();

    // nsi_unavailability_subscribe_info->nsi_to_subscribe
    if(nsi_unavailability_subscribe_info->nsi_to_subscribe) {
    cJSON *nsi_to_subscribe = cJSON_AddArrayToObject(item, "nsiToSubscribe");
    if(nsi_to_subscribe == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *nsi_to_subscribeListEntry;
    if (nsi_unavailability_subscribe_info->nsi_to_subscribe) {
    list_ForEach(nsi_to_subscribeListEntry, nsi_unavailability_subscribe_info->nsi_to_subscribe) {
    cJSON *itemLocal = object_convertToJSON(nsi_to_subscribeListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(nsi_to_subscribe, itemLocal);
    }
    }
    }


    // nsi_unavailability_subscribe_info->snssai_to_subscribe
    if(nsi_unavailability_subscribe_info->snssai_to_subscribe) {
    cJSON *snssai_to_subscribe = cJSON_AddArrayToObject(item, "snssaiToSubscribe");
    if(snssai_to_subscribe == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *snssai_to_subscribeListEntry;
    if (nsi_unavailability_subscribe_info->snssai_to_subscribe) {
    list_ForEach(snssai_to_subscribeListEntry, nsi_unavailability_subscribe_info->snssai_to_subscribe) {
    cJSON *itemLocal = object_convertToJSON(snssai_to_subscribeListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(snssai_to_subscribe, itemLocal);
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

nsi_unavailability_subscribe_info_t *nsi_unavailability_subscribe_info_parseFromJSON(cJSON *nsi_unavailability_subscribe_infoJSON){

    nsi_unavailability_subscribe_info_t *nsi_unavailability_subscribe_info_local_var = NULL;

    // define the local list for nsi_unavailability_subscribe_info->nsi_to_subscribe
    list_t *nsi_to_subscribeList = NULL;

    // define the local list for nsi_unavailability_subscribe_info->snssai_to_subscribe
    list_t *snssai_to_subscribeList = NULL;

    // nsi_unavailability_subscribe_info->nsi_to_subscribe
    cJSON *nsi_to_subscribe = cJSON_GetObjectItemCaseSensitive(nsi_unavailability_subscribe_infoJSON, "nsiToSubscribe");
    if (nsi_to_subscribe) { 
    cJSON *nsi_to_subscribe_local_nonprimitive = NULL;
    if(!cJSON_IsArray(nsi_to_subscribe)){
        goto end; //nonprimitive container
    }

    nsi_to_subscribeList = list_createList();

    cJSON_ArrayForEach(nsi_to_subscribe_local_nonprimitive,nsi_to_subscribe )
    {
        if(!cJSON_IsObject(nsi_to_subscribe_local_nonprimitive)){
            goto end;
        }
        object_t *nsi_to_subscribeItem = object_parseFromJSON(nsi_to_subscribe_local_nonprimitive);

        list_addElement(nsi_to_subscribeList, nsi_to_subscribeItem);
    }
    }

    // nsi_unavailability_subscribe_info->snssai_to_subscribe
    cJSON *snssai_to_subscribe = cJSON_GetObjectItemCaseSensitive(nsi_unavailability_subscribe_infoJSON, "snssaiToSubscribe");
    if (snssai_to_subscribe) { 
    cJSON *snssai_to_subscribe_local_nonprimitive = NULL;
    if(!cJSON_IsArray(snssai_to_subscribe)){
        goto end; //nonprimitive container
    }

    snssai_to_subscribeList = list_createList();

    cJSON_ArrayForEach(snssai_to_subscribe_local_nonprimitive,snssai_to_subscribe )
    {
        if(!cJSON_IsObject(snssai_to_subscribe_local_nonprimitive)){
            goto end;
        }
        object_t *snssai_to_subscribeItem = object_parseFromJSON(snssai_to_subscribe_local_nonprimitive);

        list_addElement(snssai_to_subscribeList, snssai_to_subscribeItem);
    }
    }


    nsi_unavailability_subscribe_info_local_var = nsi_unavailability_subscribe_info_create (
        nsi_to_subscribe ? nsi_to_subscribeList : NULL,
        snssai_to_subscribe ? snssai_to_subscribeList : NULL
        );

    return nsi_unavailability_subscribe_info_local_var;
end:
    if (nsi_to_subscribeList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, nsi_to_subscribeList) {
            object_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(nsi_to_subscribeList);
        nsi_to_subscribeList = NULL;
    }
    if (snssai_to_subscribeList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, snssai_to_subscribeList) {
            object_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(snssai_to_subscribeList);
        snssai_to_subscribeList = NULL;
    }
    return NULL;

}
