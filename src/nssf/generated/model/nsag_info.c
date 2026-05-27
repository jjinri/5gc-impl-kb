#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nsag_info.h"



nsag_info_t *nsag_info_create(
    list_t *nsag_ids,
    list_t *snssai_list,
    list_t *tai_list,
    list_t *tai_range_list
    ) {
    nsag_info_t *nsag_info_local_var = malloc(sizeof(nsag_info_t));
    if (!nsag_info_local_var) {
        return NULL;
    }
    nsag_info_local_var->nsag_ids = nsag_ids;
    nsag_info_local_var->snssai_list = snssai_list;
    nsag_info_local_var->tai_list = tai_list;
    nsag_info_local_var->tai_range_list = tai_range_list;

    return nsag_info_local_var;
}


void nsag_info_free(nsag_info_t *nsag_info) {
    if(NULL == nsag_info){
        return ;
    }
    listEntry_t *listEntry;
    if (nsag_info->nsag_ids) {
        list_ForEach(listEntry, nsag_info->nsag_ids) {
            free(listEntry->data);
        }
        list_freeList(nsag_info->nsag_ids);
        nsag_info->nsag_ids = NULL;
    }
    if (nsag_info->snssai_list) {
        list_ForEach(listEntry, nsag_info->snssai_list) {
            snssai_free(listEntry->data);
        }
        list_freeList(nsag_info->snssai_list);
        nsag_info->snssai_list = NULL;
    }
    if (nsag_info->tai_list) {
        list_ForEach(listEntry, nsag_info->tai_list) {
            tai_free(listEntry->data);
        }
        list_freeList(nsag_info->tai_list);
        nsag_info->tai_list = NULL;
    }
    if (nsag_info->tai_range_list) {
        list_ForEach(listEntry, nsag_info->tai_range_list) {
            tai_range_free(listEntry->data);
        }
        list_freeList(nsag_info->tai_range_list);
        nsag_info->tai_range_list = NULL;
    }
    free(nsag_info);
}

cJSON *nsag_info_convertToJSON(nsag_info_t *nsag_info) {
    cJSON *item = cJSON_CreateObject();

    // nsag_info->nsag_ids
    if (!nsag_info->nsag_ids) {
        goto fail;
    }
    cJSON *nsag_ids = cJSON_AddArrayToObject(item, "nsagIds");
    if(nsag_ids == NULL) {
        goto fail; //primitive container
    }

    listEntry_t *nsag_idsListEntry;
    list_ForEach(nsag_idsListEntry, nsag_info->nsag_ids) {
    if(cJSON_AddNumberToObject(nsag_ids, "", *(double *)nsag_idsListEntry->data) == NULL)
    {
        goto fail;
    }
    }


    // nsag_info->snssai_list
    if (!nsag_info->snssai_list) {
        goto fail;
    }
    cJSON *snssai_list = cJSON_AddArrayToObject(item, "snssaiList");
    if(snssai_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *snssai_listListEntry;
    if (nsag_info->snssai_list) {
    list_ForEach(snssai_listListEntry, nsag_info->snssai_list) {
    cJSON *itemLocal = snssai_convertToJSON(snssai_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(snssai_list, itemLocal);
    }
    }


    // nsag_info->tai_list
    if(nsag_info->tai_list) {
    cJSON *tai_list = cJSON_AddArrayToObject(item, "taiList");
    if(tai_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *tai_listListEntry;
    if (nsag_info->tai_list) {
    list_ForEach(tai_listListEntry, nsag_info->tai_list) {
    cJSON *itemLocal = tai_convertToJSON(tai_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(tai_list, itemLocal);
    }
    }
    }


    // nsag_info->tai_range_list
    if(nsag_info->tai_range_list) {
    cJSON *tai_range_list = cJSON_AddArrayToObject(item, "taiRangeList");
    if(tai_range_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *tai_range_listListEntry;
    if (nsag_info->tai_range_list) {
    list_ForEach(tai_range_listListEntry, nsag_info->tai_range_list) {
    cJSON *itemLocal = tai_range_convertToJSON(tai_range_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(tai_range_list, itemLocal);
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

nsag_info_t *nsag_info_parseFromJSON(cJSON *nsag_infoJSON){

    nsag_info_t *nsag_info_local_var = NULL;

    // define the local list for nsag_info->nsag_ids
    list_t *nsag_idsList = NULL;

    // define the local list for nsag_info->snssai_list
    list_t *snssai_listList = NULL;

    // define the local list for nsag_info->tai_list
    list_t *tai_listList = NULL;

    // define the local list for nsag_info->tai_range_list
    list_t *tai_range_listList = NULL;

    // nsag_info->nsag_ids
    cJSON *nsag_ids = cJSON_GetObjectItemCaseSensitive(nsag_infoJSON, "nsagIds");
    if (!nsag_ids) {
        goto end;
    }

    
    cJSON *nsag_ids_local = NULL;
    if(!cJSON_IsArray(nsag_ids)) {
        goto end;//primitive container
    }
    nsag_idsList = list_createList();

    cJSON_ArrayForEach(nsag_ids_local, nsag_ids)
    {
        if(!cJSON_IsNumber(nsag_ids_local))
        {
            goto end;
        }
        double *nsag_ids_local_value = (double *)calloc(1, sizeof(double));
        if(!nsag_ids_local_value)
        {
            goto end;
        }
        *nsag_ids_local_value = nsag_ids_local->valuedouble;
        list_addElement(nsag_idsList , nsag_ids_local_value);
    }

    // nsag_info->snssai_list
    cJSON *snssai_list = cJSON_GetObjectItemCaseSensitive(nsag_infoJSON, "snssaiList");
    if (!snssai_list) {
        goto end;
    }

    
    cJSON *snssai_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(snssai_list)){
        goto end; //nonprimitive container
    }

    snssai_listList = list_createList();

    cJSON_ArrayForEach(snssai_list_local_nonprimitive,snssai_list )
    {
        if(!cJSON_IsObject(snssai_list_local_nonprimitive)){
            goto end;
        }
        snssai_t *snssai_listItem = snssai_parseFromJSON(snssai_list_local_nonprimitive);

        list_addElement(snssai_listList, snssai_listItem);
    }

    // nsag_info->tai_list
    cJSON *tai_list = cJSON_GetObjectItemCaseSensitive(nsag_infoJSON, "taiList");
    if (tai_list) { 
    cJSON *tai_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(tai_list)){
        goto end; //nonprimitive container
    }

    tai_listList = list_createList();

    cJSON_ArrayForEach(tai_list_local_nonprimitive,tai_list )
    {
        if(!cJSON_IsObject(tai_list_local_nonprimitive)){
            goto end;
        }
        tai_t *tai_listItem = tai_parseFromJSON(tai_list_local_nonprimitive);

        list_addElement(tai_listList, tai_listItem);
    }
    }

    // nsag_info->tai_range_list
    cJSON *tai_range_list = cJSON_GetObjectItemCaseSensitive(nsag_infoJSON, "taiRangeList");
    if (tai_range_list) { 
    cJSON *tai_range_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(tai_range_list)){
        goto end; //nonprimitive container
    }

    tai_range_listList = list_createList();

    cJSON_ArrayForEach(tai_range_list_local_nonprimitive,tai_range_list )
    {
        if(!cJSON_IsObject(tai_range_list_local_nonprimitive)){
            goto end;
        }
        tai_range_t *tai_range_listItem = tai_range_parseFromJSON(tai_range_list_local_nonprimitive);

        list_addElement(tai_range_listList, tai_range_listItem);
    }
    }


    nsag_info_local_var = nsag_info_create (
        nsag_idsList,
        snssai_listList,
        tai_list ? tai_listList : NULL,
        tai_range_list ? tai_range_listList : NULL
        );

    return nsag_info_local_var;
end:
    if (nsag_idsList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, nsag_idsList) {
            free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(nsag_idsList);
        nsag_idsList = NULL;
    }
    if (snssai_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, snssai_listList) {
            snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(snssai_listList);
        snssai_listList = NULL;
    }
    if (tai_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, tai_listList) {
            tai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(tai_listList);
        tai_listList = NULL;
    }
    if (tai_range_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, tai_range_listList) {
            tai_range_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(tai_range_listList);
        tai_range_listList = NULL;
    }
    return NULL;

}
