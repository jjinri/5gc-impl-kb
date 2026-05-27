#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "query_param_combination.h"



query_param_combination_t *query_param_combination_create(
    list_t *query_params
    ) {
    query_param_combination_t *query_param_combination_local_var = malloc(sizeof(query_param_combination_t));
    if (!query_param_combination_local_var) {
        return NULL;
    }
    query_param_combination_local_var->query_params = query_params;

    return query_param_combination_local_var;
}


void query_param_combination_free(query_param_combination_t *query_param_combination) {
    if(NULL == query_param_combination){
        return ;
    }
    listEntry_t *listEntry;
    if (query_param_combination->query_params) {
        list_ForEach(listEntry, query_param_combination->query_params) {
            query_parameter_free(listEntry->data);
        }
        list_freeList(query_param_combination->query_params);
        query_param_combination->query_params = NULL;
    }
    free(query_param_combination);
}

cJSON *query_param_combination_convertToJSON(query_param_combination_t *query_param_combination) {
    cJSON *item = cJSON_CreateObject();

    // query_param_combination->query_params
    if (!query_param_combination->query_params) {
        goto fail;
    }
    cJSON *query_params = cJSON_AddArrayToObject(item, "queryParams");
    if(query_params == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *query_paramsListEntry;
    if (query_param_combination->query_params) {
    list_ForEach(query_paramsListEntry, query_param_combination->query_params) {
    cJSON *itemLocal = query_parameter_convertToJSON(query_paramsListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(query_params, itemLocal);
    }
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

query_param_combination_t *query_param_combination_parseFromJSON(cJSON *query_param_combinationJSON){

    query_param_combination_t *query_param_combination_local_var = NULL;

    // define the local list for query_param_combination->query_params
    list_t *query_paramsList = NULL;

    // query_param_combination->query_params
    cJSON *query_params = cJSON_GetObjectItemCaseSensitive(query_param_combinationJSON, "queryParams");
    if (!query_params) {
        goto end;
    }

    
    cJSON *query_params_local_nonprimitive = NULL;
    if(!cJSON_IsArray(query_params)){
        goto end; //nonprimitive container
    }

    query_paramsList = list_createList();

    cJSON_ArrayForEach(query_params_local_nonprimitive,query_params )
    {
        if(!cJSON_IsObject(query_params_local_nonprimitive)){
            goto end;
        }
        query_parameter_t *query_paramsItem = query_parameter_parseFromJSON(query_params_local_nonprimitive);

        list_addElement(query_paramsList, query_paramsItem);
    }


    query_param_combination_local_var = query_param_combination_create (
        query_paramsList
        );

    return query_param_combination_local_var;
end:
    if (query_paramsList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, query_paramsList) {
            query_parameter_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(query_paramsList);
        query_paramsList = NULL;
    }
    return NULL;

}
