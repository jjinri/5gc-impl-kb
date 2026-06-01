#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "snssai_replacement_subscribe_info.h"



snssai_replacement_subscribe_info_t *snssai_replacement_subscribe_info_create(
    list_t *snssai_to_subscribe,
    object_t *nf_type,
    object_t *nf_id,
    object_t *plmn_id
    ) {
    snssai_replacement_subscribe_info_t *snssai_replacement_subscribe_info_local_var = malloc(sizeof(snssai_replacement_subscribe_info_t));
    if (!snssai_replacement_subscribe_info_local_var) {
        return NULL;
    }
    snssai_replacement_subscribe_info_local_var->snssai_to_subscribe = snssai_to_subscribe;
    snssai_replacement_subscribe_info_local_var->nf_type = nf_type;
    snssai_replacement_subscribe_info_local_var->nf_id = nf_id;
    snssai_replacement_subscribe_info_local_var->plmn_id = plmn_id;

    return snssai_replacement_subscribe_info_local_var;
}


void snssai_replacement_subscribe_info_free(snssai_replacement_subscribe_info_t *snssai_replacement_subscribe_info) {
    if(NULL == snssai_replacement_subscribe_info){
        return ;
    }
    listEntry_t *listEntry;
    if (snssai_replacement_subscribe_info->snssai_to_subscribe) {
        list_ForEach(listEntry, snssai_replacement_subscribe_info->snssai_to_subscribe) {
            object_free(listEntry->data);
        }
        list_freeList(snssai_replacement_subscribe_info->snssai_to_subscribe);
        snssai_replacement_subscribe_info->snssai_to_subscribe = NULL;
    }
    if (snssai_replacement_subscribe_info->nf_type) {
        object_free(snssai_replacement_subscribe_info->nf_type);
        snssai_replacement_subscribe_info->nf_type = NULL;
    }
    if (snssai_replacement_subscribe_info->nf_id) {
        object_free(snssai_replacement_subscribe_info->nf_id);
        snssai_replacement_subscribe_info->nf_id = NULL;
    }
    if (snssai_replacement_subscribe_info->plmn_id) {
        object_free(snssai_replacement_subscribe_info->plmn_id);
        snssai_replacement_subscribe_info->plmn_id = NULL;
    }
    free(snssai_replacement_subscribe_info);
}

cJSON *snssai_replacement_subscribe_info_convertToJSON(snssai_replacement_subscribe_info_t *snssai_replacement_subscribe_info) {
    cJSON *item = cJSON_CreateObject();

    // snssai_replacement_subscribe_info->snssai_to_subscribe
    if (!snssai_replacement_subscribe_info->snssai_to_subscribe) {
        goto fail;
    }
    cJSON *snssai_to_subscribe = cJSON_AddArrayToObject(item, "snssaiToSubscribe");
    if(snssai_to_subscribe == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *snssai_to_subscribeListEntry;
    if (snssai_replacement_subscribe_info->snssai_to_subscribe) {
    list_ForEach(snssai_to_subscribeListEntry, snssai_replacement_subscribe_info->snssai_to_subscribe) {
    cJSON *itemLocal = object_convertToJSON(snssai_to_subscribeListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(snssai_to_subscribe, itemLocal);
    }
    }


    // snssai_replacement_subscribe_info->nf_type
    if (!snssai_replacement_subscribe_info->nf_type) {
        goto fail;
    }
    cJSON *nf_type_local_JSON = object_convertToJSON(snssai_replacement_subscribe_info->nf_type);
    if(nf_type_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "nfType", nf_type_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }


    // snssai_replacement_subscribe_info->nf_id
    if (!snssai_replacement_subscribe_info->nf_id) {
        goto fail;
    }
    cJSON *nf_id_local_JSON = object_convertToJSON(snssai_replacement_subscribe_info->nf_id);
    if(nf_id_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "nfId", nf_id_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }


    // snssai_replacement_subscribe_info->plmn_id
    if(snssai_replacement_subscribe_info->plmn_id) {
    cJSON *plmn_id_local_JSON = object_convertToJSON(snssai_replacement_subscribe_info->plmn_id);
    if(plmn_id_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "plmnId", plmn_id_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

snssai_replacement_subscribe_info_t *snssai_replacement_subscribe_info_parseFromJSON(cJSON *snssai_replacement_subscribe_infoJSON){

    snssai_replacement_subscribe_info_t *snssai_replacement_subscribe_info_local_var = NULL;

    // define the local list for snssai_replacement_subscribe_info->snssai_to_subscribe
    list_t *snssai_to_subscribeList = NULL;

    // define the local variable for snssai_replacement_subscribe_info->nf_type
    object_t *nf_type_local_nonprim = NULL;

    // define the local variable for snssai_replacement_subscribe_info->nf_id
    object_t *nf_id_local_nonprim = NULL;

    // define the local variable for snssai_replacement_subscribe_info->plmn_id
    object_t *plmn_id_local_nonprim = NULL;

    // snssai_replacement_subscribe_info->snssai_to_subscribe
    cJSON *snssai_to_subscribe = cJSON_GetObjectItemCaseSensitive(snssai_replacement_subscribe_infoJSON, "snssaiToSubscribe");
    if (!snssai_to_subscribe) {
        goto end;
    }

    
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

    // snssai_replacement_subscribe_info->nf_type
    cJSON *nf_type = cJSON_GetObjectItemCaseSensitive(snssai_replacement_subscribe_infoJSON, "nfType");
    if (!nf_type) {
        goto end;
    }

    
    nf_type_local_nonprim = object_parseFromJSON(nf_type); //custom

    // snssai_replacement_subscribe_info->nf_id
    cJSON *nf_id = cJSON_GetObjectItemCaseSensitive(snssai_replacement_subscribe_infoJSON, "nfId");
    if (!nf_id) {
        goto end;
    }

    
    nf_id_local_nonprim = object_parseFromJSON(nf_id); //custom

    // snssai_replacement_subscribe_info->plmn_id
    cJSON *plmn_id = cJSON_GetObjectItemCaseSensitive(snssai_replacement_subscribe_infoJSON, "plmnId");
    if (plmn_id) { 
    plmn_id_local_nonprim = object_parseFromJSON(plmn_id); //custom
    }


    snssai_replacement_subscribe_info_local_var = snssai_replacement_subscribe_info_create (
        snssai_to_subscribeList,
        nf_type_local_nonprim,
        nf_id_local_nonprim,
        plmn_id ? plmn_id_local_nonprim : NULL
        );

    return snssai_replacement_subscribe_info_local_var;
end:
    if (snssai_to_subscribeList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, snssai_to_subscribeList) {
            object_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(snssai_to_subscribeList);
        snssai_to_subscribeList = NULL;
    }
    if (nf_type_local_nonprim) {
        object_free(nf_type_local_nonprim);
        nf_type_local_nonprim = NULL;
    }
    if (nf_id_local_nonprim) {
        object_free(nf_id_local_nonprim);
        nf_id_local_nonprim = NULL;
    }
    if (plmn_id_local_nonprim) {
        object_free(plmn_id_local_nonprim);
        plmn_id_local_nonprim = NULL;
    }
    return NULL;

}
