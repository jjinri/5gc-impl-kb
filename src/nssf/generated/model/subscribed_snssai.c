#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "subscribed_snssai.h"



subscribed_snssai_t *subscribed_snssai_create(
    snssai_t *subscribed_snssai,
    int default_indication,
    list_t *subscribed_ns_srg_list
    ) {
    subscribed_snssai_t *subscribed_snssai_local_var = malloc(sizeof(subscribed_snssai_t));
    if (!subscribed_snssai_local_var) {
        return NULL;
    }
    subscribed_snssai_local_var->subscribed_snssai = subscribed_snssai;
    subscribed_snssai_local_var->default_indication = default_indication;
    subscribed_snssai_local_var->subscribed_ns_srg_list = subscribed_ns_srg_list;

    return subscribed_snssai_local_var;
}


void subscribed_snssai_free(subscribed_snssai_t *subscribed_snssai) {
    if(NULL == subscribed_snssai){
        return ;
    }
    listEntry_t *listEntry;
    if (subscribed_snssai->subscribed_snssai) {
        snssai_free(subscribed_snssai->subscribed_snssai);
        subscribed_snssai->subscribed_snssai = NULL;
    }
    if (subscribed_snssai->subscribed_ns_srg_list) {
        list_ForEach(listEntry, subscribed_snssai->subscribed_ns_srg_list) {
            free(listEntry->data);
        }
        list_freeList(subscribed_snssai->subscribed_ns_srg_list);
        subscribed_snssai->subscribed_ns_srg_list = NULL;
    }
    free(subscribed_snssai);
}

cJSON *subscribed_snssai_convertToJSON(subscribed_snssai_t *subscribed_snssai) {
    cJSON *item = cJSON_CreateObject();

    // subscribed_snssai->subscribed_snssai
    if (!subscribed_snssai->subscribed_snssai) {
        goto fail;
    }
    cJSON *subscribed_snssai_local_JSON = snssai_convertToJSON(subscribed_snssai->subscribed_snssai);
    if(subscribed_snssai_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "subscribedSnssai", subscribed_snssai_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }


    // subscribed_snssai->default_indication
    if(subscribed_snssai->default_indication) {
    if(cJSON_AddBoolToObject(item, "defaultIndication", subscribed_snssai->default_indication) == NULL) {
    goto fail; //Bool
    }
    }


    // subscribed_snssai->subscribed_ns_srg_list
    if(subscribed_snssai->subscribed_ns_srg_list) {
    cJSON *subscribed_ns_srg_list = cJSON_AddArrayToObject(item, "subscribedNsSrgList");
    if(subscribed_ns_srg_list == NULL) {
        goto fail; //primitive container
    }

    listEntry_t *subscribed_ns_srg_listListEntry;
    list_ForEach(subscribed_ns_srg_listListEntry, subscribed_snssai->subscribed_ns_srg_list) {
    if(cJSON_AddStringToObject(subscribed_ns_srg_list, "", (char*)subscribed_ns_srg_listListEntry->data) == NULL)
    {
        goto fail;
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

subscribed_snssai_t *subscribed_snssai_parseFromJSON(cJSON *subscribed_snssaiJSON){

    subscribed_snssai_t *subscribed_snssai_local_var = NULL;

    // define the local variable for subscribed_snssai->subscribed_snssai
    snssai_t *subscribed_snssai_local_nonprim = NULL;

    // define the local list for subscribed_snssai->subscribed_ns_srg_list
    list_t *subscribed_ns_srg_listList = NULL;

    // subscribed_snssai->subscribed_snssai
    cJSON *subscribed_snssai = cJSON_GetObjectItemCaseSensitive(subscribed_snssaiJSON, "subscribedSnssai");
    if (!subscribed_snssai) {
        goto end;
    }

    
    subscribed_snssai_local_nonprim = snssai_parseFromJSON(subscribed_snssai); //nonprimitive

    // subscribed_snssai->default_indication
    cJSON *default_indication = cJSON_GetObjectItemCaseSensitive(subscribed_snssaiJSON, "defaultIndication");
    if (default_indication) { 
    if(!cJSON_IsBool(default_indication))
    {
    goto end; //Bool
    }
    }

    // subscribed_snssai->subscribed_ns_srg_list
    cJSON *subscribed_ns_srg_list = cJSON_GetObjectItemCaseSensitive(subscribed_snssaiJSON, "subscribedNsSrgList");
    if (subscribed_ns_srg_list) { 
    cJSON *subscribed_ns_srg_list_local = NULL;
    if(!cJSON_IsArray(subscribed_ns_srg_list)) {
        goto end;//primitive container
    }
    subscribed_ns_srg_listList = list_createList();

    cJSON_ArrayForEach(subscribed_ns_srg_list_local, subscribed_ns_srg_list)
    {
        if(!cJSON_IsString(subscribed_ns_srg_list_local))
        {
            goto end;
        }
        list_addElement(subscribed_ns_srg_listList , strdup(subscribed_ns_srg_list_local->valuestring));
    }
    }


    subscribed_snssai_local_var = subscribed_snssai_create (
        subscribed_snssai_local_nonprim,
        default_indication ? default_indication->valueint : 0,
        subscribed_ns_srg_list ? subscribed_ns_srg_listList : NULL
        );

    return subscribed_snssai_local_var;
end:
    if (subscribed_snssai_local_nonprim) {
        snssai_free(subscribed_snssai_local_nonprim);
        subscribed_snssai_local_nonprim = NULL;
    }
    if (subscribed_ns_srg_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, subscribed_ns_srg_listList) {
            free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(subscribed_ns_srg_listList);
        subscribed_ns_srg_listList = NULL;
    }
    return NULL;

}
