#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "allowed_nssai.h"


char* allowed_nssai_access_type_ToString(nssf_ns_selection_allowed_nssai__e access_type) {
    char* access_typeArray[] =  { "NULL", "3GPP_ACCESS", "NON_3GPP_ACCESS" };
    return access_typeArray[access_type];
}

nssf_ns_selection_allowed_nssai__e allowed_nssai_access_type_FromString(char* access_type){
    int stringToReturn = 0;
    char *access_typeArray[] =  { "NULL", "3GPP_ACCESS", "NON_3GPP_ACCESS" };
    size_t sizeofArray = sizeof(access_typeArray) / sizeof(access_typeArray[0]);
    while(stringToReturn < sizeofArray) {
        if(strcmp(access_type, access_typeArray[stringToReturn]) == 0) {
            return stringToReturn;
        }
        stringToReturn++;
    }
    return 0;
}

allowed_nssai_t *allowed_nssai_create(
    list_t *allowed_snssai_list,
    access_type_t *access_type
    ) {
    allowed_nssai_t *allowed_nssai_local_var = malloc(sizeof(allowed_nssai_t));
    if (!allowed_nssai_local_var) {
        return NULL;
    }
    allowed_nssai_local_var->allowed_snssai_list = allowed_snssai_list;
    allowed_nssai_local_var->access_type = access_type;

    return allowed_nssai_local_var;
}


void allowed_nssai_free(allowed_nssai_t *allowed_nssai) {
    if(NULL == allowed_nssai){
        return ;
    }
    listEntry_t *listEntry;
    if (allowed_nssai->allowed_snssai_list) {
        list_ForEach(listEntry, allowed_nssai->allowed_snssai_list) {
            allowed_snssai_free(listEntry->data);
        }
        list_freeList(allowed_nssai->allowed_snssai_list);
        allowed_nssai->allowed_snssai_list = NULL;
    }
    if (allowed_nssai->access_type) {
        access_type_free(allowed_nssai->access_type);
        allowed_nssai->access_type = NULL;
    }
    free(allowed_nssai);
}

cJSON *allowed_nssai_convertToJSON(allowed_nssai_t *allowed_nssai) {
    cJSON *item = cJSON_CreateObject();

    // allowed_nssai->allowed_snssai_list
    if (!allowed_nssai->allowed_snssai_list) {
        goto fail;
    }
    cJSON *allowed_snssai_list = cJSON_AddArrayToObject(item, "allowedSnssaiList");
    if(allowed_snssai_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *allowed_snssai_listListEntry;
    if (allowed_nssai->allowed_snssai_list) {
    list_ForEach(allowed_snssai_listListEntry, allowed_nssai->allowed_snssai_list) {
    cJSON *itemLocal = allowed_snssai_convertToJSON(allowed_snssai_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(allowed_snssai_list, itemLocal);
    }
    }


    // allowed_nssai->access_type
    if (nssf_ns_selection_allowed_nssai__NULL == allowed_nssai->access_type) {
        goto fail;
    }
    cJSON *access_type_local_JSON = access_type_convertToJSON(allowed_nssai->access_type);
    if(access_type_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "accessType", access_type_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

allowed_nssai_t *allowed_nssai_parseFromJSON(cJSON *allowed_nssaiJSON){

    allowed_nssai_t *allowed_nssai_local_var = NULL;

    // define the local list for allowed_nssai->allowed_snssai_list
    list_t *allowed_snssai_listList = NULL;

    // define the local variable for allowed_nssai->access_type
    access_type_t *access_type_local_nonprim = NULL;

    // allowed_nssai->allowed_snssai_list
    cJSON *allowed_snssai_list = cJSON_GetObjectItemCaseSensitive(allowed_nssaiJSON, "allowedSnssaiList");
    if (!allowed_snssai_list) {
        goto end;
    }

    
    cJSON *allowed_snssai_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(allowed_snssai_list)){
        goto end; //nonprimitive container
    }

    allowed_snssai_listList = list_createList();

    cJSON_ArrayForEach(allowed_snssai_list_local_nonprimitive,allowed_snssai_list )
    {
        if(!cJSON_IsObject(allowed_snssai_list_local_nonprimitive)){
            goto end;
        }
        allowed_snssai_t *allowed_snssai_listItem = allowed_snssai_parseFromJSON(allowed_snssai_list_local_nonprimitive);

        list_addElement(allowed_snssai_listList, allowed_snssai_listItem);
    }

    // allowed_nssai->access_type
    cJSON *access_type = cJSON_GetObjectItemCaseSensitive(allowed_nssaiJSON, "accessType");
    if (!access_type) {
        goto end;
    }

    
    access_type_local_nonprim = access_type_parseFromJSON(access_type); //custom


    allowed_nssai_local_var = allowed_nssai_create (
        allowed_snssai_listList,
        access_type_local_nonprim
        );

    return allowed_nssai_local_var;
end:
    if (allowed_snssai_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, allowed_snssai_listList) {
            allowed_snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(allowed_snssai_listList);
        allowed_snssai_listList = NULL;
    }
    if (access_type_local_nonprim) {
        access_type_free(access_type_local_nonprim);
        access_type_local_nonprim = NULL;
    }
    return NULL;

}
