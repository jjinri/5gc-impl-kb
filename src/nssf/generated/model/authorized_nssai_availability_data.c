#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "authorized_nssai_availability_data.h"



authorized_nssai_availability_data_t *authorized_nssai_availability_data_create(
    tai_t *tai,
    list_t *supported_snssai_list,
    list_t *restricted_snssai_list,
    list_t *tai_list,
    list_t *tai_range_list,
    list_t *nsag_infos
    ) {
    authorized_nssai_availability_data_t *authorized_nssai_availability_data_local_var = malloc(sizeof(authorized_nssai_availability_data_t));
    if (!authorized_nssai_availability_data_local_var) {
        return NULL;
    }
    authorized_nssai_availability_data_local_var->tai = tai;
    authorized_nssai_availability_data_local_var->supported_snssai_list = supported_snssai_list;
    authorized_nssai_availability_data_local_var->restricted_snssai_list = restricted_snssai_list;
    authorized_nssai_availability_data_local_var->tai_list = tai_list;
    authorized_nssai_availability_data_local_var->tai_range_list = tai_range_list;
    authorized_nssai_availability_data_local_var->nsag_infos = nsag_infos;

    return authorized_nssai_availability_data_local_var;
}


void authorized_nssai_availability_data_free(authorized_nssai_availability_data_t *authorized_nssai_availability_data) {
    if(NULL == authorized_nssai_availability_data){
        return ;
    }
    listEntry_t *listEntry;
    if (authorized_nssai_availability_data->tai) {
        tai_free(authorized_nssai_availability_data->tai);
        authorized_nssai_availability_data->tai = NULL;
    }
    if (authorized_nssai_availability_data->supported_snssai_list) {
        list_ForEach(listEntry, authorized_nssai_availability_data->supported_snssai_list) {
            ext_snssai_free(listEntry->data);
        }
        list_freeList(authorized_nssai_availability_data->supported_snssai_list);
        authorized_nssai_availability_data->supported_snssai_list = NULL;
    }
    if (authorized_nssai_availability_data->restricted_snssai_list) {
        list_ForEach(listEntry, authorized_nssai_availability_data->restricted_snssai_list) {
            restricted_snssai_free(listEntry->data);
        }
        list_freeList(authorized_nssai_availability_data->restricted_snssai_list);
        authorized_nssai_availability_data->restricted_snssai_list = NULL;
    }
    if (authorized_nssai_availability_data->tai_list) {
        list_ForEach(listEntry, authorized_nssai_availability_data->tai_list) {
            tai_free(listEntry->data);
        }
        list_freeList(authorized_nssai_availability_data->tai_list);
        authorized_nssai_availability_data->tai_list = NULL;
    }
    if (authorized_nssai_availability_data->tai_range_list) {
        list_ForEach(listEntry, authorized_nssai_availability_data->tai_range_list) {
            tai_range_free(listEntry->data);
        }
        list_freeList(authorized_nssai_availability_data->tai_range_list);
        authorized_nssai_availability_data->tai_range_list = NULL;
    }
    if (authorized_nssai_availability_data->nsag_infos) {
        list_ForEach(listEntry, authorized_nssai_availability_data->nsag_infos) {
            nsag_info_free(listEntry->data);
        }
        list_freeList(authorized_nssai_availability_data->nsag_infos);
        authorized_nssai_availability_data->nsag_infos = NULL;
    }
    free(authorized_nssai_availability_data);
}

cJSON *authorized_nssai_availability_data_convertToJSON(authorized_nssai_availability_data_t *authorized_nssai_availability_data) {
    cJSON *item = cJSON_CreateObject();

    // authorized_nssai_availability_data->tai
    if (!authorized_nssai_availability_data->tai) {
        goto fail;
    }
    cJSON *tai_local_JSON = tai_convertToJSON(authorized_nssai_availability_data->tai);
    if(tai_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "tai", tai_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }


    // authorized_nssai_availability_data->supported_snssai_list
    if (!authorized_nssai_availability_data->supported_snssai_list) {
        goto fail;
    }
    cJSON *supported_snssai_list = cJSON_AddArrayToObject(item, "supportedSnssaiList");
    if(supported_snssai_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *supported_snssai_listListEntry;
    if (authorized_nssai_availability_data->supported_snssai_list) {
    list_ForEach(supported_snssai_listListEntry, authorized_nssai_availability_data->supported_snssai_list) {
    cJSON *itemLocal = ext_snssai_convertToJSON(supported_snssai_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(supported_snssai_list, itemLocal);
    }
    }


    // authorized_nssai_availability_data->restricted_snssai_list
    if(authorized_nssai_availability_data->restricted_snssai_list) {
    cJSON *restricted_snssai_list = cJSON_AddArrayToObject(item, "restrictedSnssaiList");
    if(restricted_snssai_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *restricted_snssai_listListEntry;
    if (authorized_nssai_availability_data->restricted_snssai_list) {
    list_ForEach(restricted_snssai_listListEntry, authorized_nssai_availability_data->restricted_snssai_list) {
    cJSON *itemLocal = restricted_snssai_convertToJSON(restricted_snssai_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(restricted_snssai_list, itemLocal);
    }
    }
    }


    // authorized_nssai_availability_data->tai_list
    if(authorized_nssai_availability_data->tai_list) {
    cJSON *tai_list = cJSON_AddArrayToObject(item, "taiList");
    if(tai_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *tai_listListEntry;
    if (authorized_nssai_availability_data->tai_list) {
    list_ForEach(tai_listListEntry, authorized_nssai_availability_data->tai_list) {
    cJSON *itemLocal = tai_convertToJSON(tai_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(tai_list, itemLocal);
    }
    }
    }


    // authorized_nssai_availability_data->tai_range_list
    if(authorized_nssai_availability_data->tai_range_list) {
    cJSON *tai_range_list = cJSON_AddArrayToObject(item, "taiRangeList");
    if(tai_range_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *tai_range_listListEntry;
    if (authorized_nssai_availability_data->tai_range_list) {
    list_ForEach(tai_range_listListEntry, authorized_nssai_availability_data->tai_range_list) {
    cJSON *itemLocal = tai_range_convertToJSON(tai_range_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(tai_range_list, itemLocal);
    }
    }
    }


    // authorized_nssai_availability_data->nsag_infos
    if(authorized_nssai_availability_data->nsag_infos) {
    cJSON *nsag_infos = cJSON_AddArrayToObject(item, "nsagInfos");
    if(nsag_infos == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *nsag_infosListEntry;
    if (authorized_nssai_availability_data->nsag_infos) {
    list_ForEach(nsag_infosListEntry, authorized_nssai_availability_data->nsag_infos) {
    cJSON *itemLocal = nsag_info_convertToJSON(nsag_infosListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(nsag_infos, itemLocal);
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

authorized_nssai_availability_data_t *authorized_nssai_availability_data_parseFromJSON(cJSON *authorized_nssai_availability_dataJSON){

    authorized_nssai_availability_data_t *authorized_nssai_availability_data_local_var = NULL;

    // define the local variable for authorized_nssai_availability_data->tai
    tai_t *tai_local_nonprim = NULL;

    // define the local list for authorized_nssai_availability_data->supported_snssai_list
    list_t *supported_snssai_listList = NULL;

    // define the local list for authorized_nssai_availability_data->restricted_snssai_list
    list_t *restricted_snssai_listList = NULL;

    // define the local list for authorized_nssai_availability_data->tai_list
    list_t *tai_listList = NULL;

    // define the local list for authorized_nssai_availability_data->tai_range_list
    list_t *tai_range_listList = NULL;

    // define the local list for authorized_nssai_availability_data->nsag_infos
    list_t *nsag_infosList = NULL;

    // authorized_nssai_availability_data->tai
    cJSON *tai = cJSON_GetObjectItemCaseSensitive(authorized_nssai_availability_dataJSON, "tai");
    if (!tai) {
        goto end;
    }

    
    tai_local_nonprim = tai_parseFromJSON(tai); //nonprimitive

    // authorized_nssai_availability_data->supported_snssai_list
    cJSON *supported_snssai_list = cJSON_GetObjectItemCaseSensitive(authorized_nssai_availability_dataJSON, "supportedSnssaiList");
    if (!supported_snssai_list) {
        goto end;
    }

    
    cJSON *supported_snssai_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(supported_snssai_list)){
        goto end; //nonprimitive container
    }

    supported_snssai_listList = list_createList();

    cJSON_ArrayForEach(supported_snssai_list_local_nonprimitive,supported_snssai_list )
    {
        if(!cJSON_IsObject(supported_snssai_list_local_nonprimitive)){
            goto end;
        }
        ext_snssai_t *supported_snssai_listItem = ext_snssai_parseFromJSON(supported_snssai_list_local_nonprimitive);

        list_addElement(supported_snssai_listList, supported_snssai_listItem);
    }

    // authorized_nssai_availability_data->restricted_snssai_list
    cJSON *restricted_snssai_list = cJSON_GetObjectItemCaseSensitive(authorized_nssai_availability_dataJSON, "restrictedSnssaiList");
    if (restricted_snssai_list) { 
    cJSON *restricted_snssai_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(restricted_snssai_list)){
        goto end; //nonprimitive container
    }

    restricted_snssai_listList = list_createList();

    cJSON_ArrayForEach(restricted_snssai_list_local_nonprimitive,restricted_snssai_list )
    {
        if(!cJSON_IsObject(restricted_snssai_list_local_nonprimitive)){
            goto end;
        }
        restricted_snssai_t *restricted_snssai_listItem = restricted_snssai_parseFromJSON(restricted_snssai_list_local_nonprimitive);

        list_addElement(restricted_snssai_listList, restricted_snssai_listItem);
    }
    }

    // authorized_nssai_availability_data->tai_list
    cJSON *tai_list = cJSON_GetObjectItemCaseSensitive(authorized_nssai_availability_dataJSON, "taiList");
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

    // authorized_nssai_availability_data->tai_range_list
    cJSON *tai_range_list = cJSON_GetObjectItemCaseSensitive(authorized_nssai_availability_dataJSON, "taiRangeList");
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

    // authorized_nssai_availability_data->nsag_infos
    cJSON *nsag_infos = cJSON_GetObjectItemCaseSensitive(authorized_nssai_availability_dataJSON, "nsagInfos");
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


    authorized_nssai_availability_data_local_var = authorized_nssai_availability_data_create (
        tai_local_nonprim,
        supported_snssai_listList,
        restricted_snssai_list ? restricted_snssai_listList : NULL,
        tai_list ? tai_listList : NULL,
        tai_range_list ? tai_range_listList : NULL,
        nsag_infos ? nsag_infosList : NULL
        );

    return authorized_nssai_availability_data_local_var;
end:
    if (tai_local_nonprim) {
        tai_free(tai_local_nonprim);
        tai_local_nonprim = NULL;
    }
    if (supported_snssai_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, supported_snssai_listList) {
            ext_snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(supported_snssai_listList);
        supported_snssai_listList = NULL;
    }
    if (restricted_snssai_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, restricted_snssai_listList) {
            restricted_snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(restricted_snssai_listList);
        restricted_snssai_listList = NULL;
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
    if (nsag_infosList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, nsag_infosList) {
            nsag_info_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(nsag_infosList);
        nsag_infosList = NULL;
    }
    return NULL;

}
