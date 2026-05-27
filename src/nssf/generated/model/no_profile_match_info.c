#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "no_profile_match_info.h"



no_profile_match_info_t *no_profile_match_info_create(
    object_t *reason,
    list_t *query_param_combination_list
    ) {
    no_profile_match_info_t *no_profile_match_info_local_var = malloc(sizeof(no_profile_match_info_t));
    if (!no_profile_match_info_local_var) {
        return NULL;
    }
    no_profile_match_info_local_var->reason = reason;
    no_profile_match_info_local_var->query_param_combination_list = query_param_combination_list;

    return no_profile_match_info_local_var;
}


void no_profile_match_info_free(no_profile_match_info_t *no_profile_match_info) {
    if(NULL == no_profile_match_info){
        return ;
    }
    listEntry_t *listEntry;
    if (no_profile_match_info->reason) {
        object_free(no_profile_match_info->reason);
        no_profile_match_info->reason = NULL;
    }
    if (no_profile_match_info->query_param_combination_list) {
        list_ForEach(listEntry, no_profile_match_info->query_param_combination_list) {
            query_param_combination_free(listEntry->data);
        }
        list_freeList(no_profile_match_info->query_param_combination_list);
        no_profile_match_info->query_param_combination_list = NULL;
    }
    free(no_profile_match_info);
}

cJSON *no_profile_match_info_convertToJSON(no_profile_match_info_t *no_profile_match_info) {
    cJSON *item = cJSON_CreateObject();

    // no_profile_match_info->reason
    if (!no_profile_match_info->reason) {
        goto fail;
    }
    cJSON *reason_local_JSON = object_convertToJSON(no_profile_match_info->reason);
    if(reason_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "reason", reason_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }


    // no_profile_match_info->query_param_combination_list
    if(no_profile_match_info->query_param_combination_list) {
    cJSON *query_param_combination_list = cJSON_AddArrayToObject(item, "queryParamCombinationList");
    if(query_param_combination_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *query_param_combination_listListEntry;
    if (no_profile_match_info->query_param_combination_list) {
    list_ForEach(query_param_combination_listListEntry, no_profile_match_info->query_param_combination_list) {
    cJSON *itemLocal = query_param_combination_convertToJSON(query_param_combination_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(query_param_combination_list, itemLocal);
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

no_profile_match_info_t *no_profile_match_info_parseFromJSON(cJSON *no_profile_match_infoJSON){

    no_profile_match_info_t *no_profile_match_info_local_var = NULL;

    // define the local variable for no_profile_match_info->reason
    object_t *reason_local_nonprim = NULL;

    // define the local list for no_profile_match_info->query_param_combination_list
    list_t *query_param_combination_listList = NULL;

    // no_profile_match_info->reason
    cJSON *reason = cJSON_GetObjectItemCaseSensitive(no_profile_match_infoJSON, "reason");
    if (!reason) {
        goto end;
    }

    
    reason_local_nonprim = object_parseFromJSON(reason); //custom

    // no_profile_match_info->query_param_combination_list
    cJSON *query_param_combination_list = cJSON_GetObjectItemCaseSensitive(no_profile_match_infoJSON, "queryParamCombinationList");
    if (query_param_combination_list) { 
    cJSON *query_param_combination_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(query_param_combination_list)){
        goto end; //nonprimitive container
    }

    query_param_combination_listList = list_createList();

    cJSON_ArrayForEach(query_param_combination_list_local_nonprimitive,query_param_combination_list )
    {
        if(!cJSON_IsObject(query_param_combination_list_local_nonprimitive)){
            goto end;
        }
        query_param_combination_t *query_param_combination_listItem = query_param_combination_parseFromJSON(query_param_combination_list_local_nonprimitive);

        list_addElement(query_param_combination_listList, query_param_combination_listItem);
    }
    }


    no_profile_match_info_local_var = no_profile_match_info_create (
        reason_local_nonprim,
        query_param_combination_list ? query_param_combination_listList : NULL
        );

    return no_profile_match_info_local_var;
end:
    if (reason_local_nonprim) {
        object_free(reason_local_nonprim);
        reason_local_nonprim = NULL;
    }
    if (query_param_combination_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, query_param_combination_listList) {
            query_param_combination_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(query_param_combination_listList);
        query_param_combination_listList = NULL;
    }
    return NULL;

}
