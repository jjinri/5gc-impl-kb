#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "allowed_snssai.h"



allowed_snssai_t *allowed_snssai_create(
    snssai_t *allowed_snssai,
    list_t *nsi_information_list,
    snssai_t *mapped_home_snssai
    ) {
    allowed_snssai_t *allowed_snssai_local_var = malloc(sizeof(allowed_snssai_t));
    if (!allowed_snssai_local_var) {
        return NULL;
    }
    allowed_snssai_local_var->allowed_snssai = allowed_snssai;
    allowed_snssai_local_var->nsi_information_list = nsi_information_list;
    allowed_snssai_local_var->mapped_home_snssai = mapped_home_snssai;

    return allowed_snssai_local_var;
}


void allowed_snssai_free(allowed_snssai_t *allowed_snssai) {
    if(NULL == allowed_snssai){
        return ;
    }
    listEntry_t *listEntry;
    if (allowed_snssai->allowed_snssai) {
        snssai_free(allowed_snssai->allowed_snssai);
        allowed_snssai->allowed_snssai = NULL;
    }
    if (allowed_snssai->nsi_information_list) {
        list_ForEach(listEntry, allowed_snssai->nsi_information_list) {
            nsi_information_free(listEntry->data);
        }
        list_freeList(allowed_snssai->nsi_information_list);
        allowed_snssai->nsi_information_list = NULL;
    }
    if (allowed_snssai->mapped_home_snssai) {
        snssai_free(allowed_snssai->mapped_home_snssai);
        allowed_snssai->mapped_home_snssai = NULL;
    }
    free(allowed_snssai);
}

cJSON *allowed_snssai_convertToJSON(allowed_snssai_t *allowed_snssai) {
    cJSON *item = cJSON_CreateObject();

    // allowed_snssai->allowed_snssai
    if (!allowed_snssai->allowed_snssai) {
        goto fail;
    }
    cJSON *allowed_snssai_local_JSON = snssai_convertToJSON(allowed_snssai->allowed_snssai);
    if(allowed_snssai_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "allowedSnssai", allowed_snssai_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }


    // allowed_snssai->nsi_information_list
    if(allowed_snssai->nsi_information_list) {
    cJSON *nsi_information_list = cJSON_AddArrayToObject(item, "nsiInformationList");
    if(nsi_information_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *nsi_information_listListEntry;
    if (allowed_snssai->nsi_information_list) {
    list_ForEach(nsi_information_listListEntry, allowed_snssai->nsi_information_list) {
    cJSON *itemLocal = nsi_information_convertToJSON(nsi_information_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(nsi_information_list, itemLocal);
    }
    }
    }


    // allowed_snssai->mapped_home_snssai
    if(allowed_snssai->mapped_home_snssai) {
    cJSON *mapped_home_snssai_local_JSON = snssai_convertToJSON(allowed_snssai->mapped_home_snssai);
    if(mapped_home_snssai_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "mappedHomeSnssai", mapped_home_snssai_local_JSON);
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

allowed_snssai_t *allowed_snssai_parseFromJSON(cJSON *allowed_snssaiJSON){

    allowed_snssai_t *allowed_snssai_local_var = NULL;

    // define the local variable for allowed_snssai->allowed_snssai
    snssai_t *allowed_snssai_local_nonprim = NULL;

    // define the local list for allowed_snssai->nsi_information_list
    list_t *nsi_information_listList = NULL;

    // define the local variable for allowed_snssai->mapped_home_snssai
    snssai_t *mapped_home_snssai_local_nonprim = NULL;

    // allowed_snssai->allowed_snssai
    cJSON *allowed_snssai = cJSON_GetObjectItemCaseSensitive(allowed_snssaiJSON, "allowedSnssai");
    if (!allowed_snssai) {
        goto end;
    }

    
    allowed_snssai_local_nonprim = snssai_parseFromJSON(allowed_snssai); //nonprimitive

    // allowed_snssai->nsi_information_list
    cJSON *nsi_information_list = cJSON_GetObjectItemCaseSensitive(allowed_snssaiJSON, "nsiInformationList");
    if (nsi_information_list) { 
    cJSON *nsi_information_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(nsi_information_list)){
        goto end; //nonprimitive container
    }

    nsi_information_listList = list_createList();

    cJSON_ArrayForEach(nsi_information_list_local_nonprimitive,nsi_information_list )
    {
        if(!cJSON_IsObject(nsi_information_list_local_nonprimitive)){
            goto end;
        }
        nsi_information_t *nsi_information_listItem = nsi_information_parseFromJSON(nsi_information_list_local_nonprimitive);

        list_addElement(nsi_information_listList, nsi_information_listItem);
    }
    }

    // allowed_snssai->mapped_home_snssai
    cJSON *mapped_home_snssai = cJSON_GetObjectItemCaseSensitive(allowed_snssaiJSON, "mappedHomeSnssai");
    if (mapped_home_snssai) { 
    mapped_home_snssai_local_nonprim = snssai_parseFromJSON(mapped_home_snssai); //nonprimitive
    }


    allowed_snssai_local_var = allowed_snssai_create (
        allowed_snssai_local_nonprim,
        nsi_information_list ? nsi_information_listList : NULL,
        mapped_home_snssai ? mapped_home_snssai_local_nonprim : NULL
        );

    return allowed_snssai_local_var;
end:
    if (allowed_snssai_local_nonprim) {
        snssai_free(allowed_snssai_local_nonprim);
        allowed_snssai_local_nonprim = NULL;
    }
    if (nsi_information_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, nsi_information_listList) {
            nsi_information_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(nsi_information_listList);
        nsi_information_listList = NULL;
    }
    if (mapped_home_snssai_local_nonprim) {
        snssai_free(mapped_home_snssai_local_nonprim);
        mapped_home_snssai_local_nonprim = NULL;
    }
    return NULL;

}
