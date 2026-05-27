#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "snssai_info.h"



snssai_info_t *snssai_info_create(
    list_t *nsi_ids
    ) {
    snssai_info_t *snssai_info_local_var = malloc(sizeof(snssai_info_t));
    if (!snssai_info_local_var) {
        return NULL;
    }
    snssai_info_local_var->nsi_ids = nsi_ids;

    return snssai_info_local_var;
}


void snssai_info_free(snssai_info_t *snssai_info) {
    if(NULL == snssai_info){
        return ;
    }
    listEntry_t *listEntry;
    if (snssai_info->nsi_ids) {
        list_ForEach(listEntry, snssai_info->nsi_ids) {
            free(listEntry->data);
        }
        list_freeList(snssai_info->nsi_ids);
        snssai_info->nsi_ids = NULL;
    }
    free(snssai_info);
}

cJSON *snssai_info_convertToJSON(snssai_info_t *snssai_info) {
    cJSON *item = cJSON_CreateObject();

    // snssai_info->nsi_ids
    if(snssai_info->nsi_ids) {
    cJSON *nsi_ids = cJSON_AddArrayToObject(item, "nsiIds");
    if(nsi_ids == NULL) {
        goto fail; //primitive container
    }

    listEntry_t *nsi_idsListEntry;
    list_ForEach(nsi_idsListEntry, snssai_info->nsi_ids) {
    if(cJSON_AddStringToObject(nsi_ids, "", (char*)nsi_idsListEntry->data) == NULL)
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

snssai_info_t *snssai_info_parseFromJSON(cJSON *snssai_infoJSON){

    snssai_info_t *snssai_info_local_var = NULL;

    // define the local list for snssai_info->nsi_ids
    list_t *nsi_idsList = NULL;

    // snssai_info->nsi_ids
    cJSON *nsi_ids = cJSON_GetObjectItemCaseSensitive(snssai_infoJSON, "nsiIds");
    if (nsi_ids) { 
    cJSON *nsi_ids_local = NULL;
    if(!cJSON_IsArray(nsi_ids)) {
        goto end;//primitive container
    }
    nsi_idsList = list_createList();

    cJSON_ArrayForEach(nsi_ids_local, nsi_ids)
    {
        if(!cJSON_IsString(nsi_ids_local))
        {
            goto end;
        }
        list_addElement(nsi_idsList , strdup(nsi_ids_local->valuestring));
    }
    }


    snssai_info_local_var = snssai_info_create (
        nsi_ids ? nsi_idsList : NULL
        );

    return snssai_info_local_var;
end:
    if (nsi_idsList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, nsi_idsList) {
            free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(nsi_idsList);
        nsi_idsList = NULL;
    }
    return NULL;

}
