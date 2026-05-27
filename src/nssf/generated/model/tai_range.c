#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tai_range.h"



tai_range_t *tai_range_create(
    plmn_id_t *plmn_id,
    list_t *tac_range_list,
    char *nid
    ) {
    tai_range_t *tai_range_local_var = malloc(sizeof(tai_range_t));
    if (!tai_range_local_var) {
        return NULL;
    }
    tai_range_local_var->plmn_id = plmn_id;
    tai_range_local_var->tac_range_list = tac_range_list;
    tai_range_local_var->nid = nid;

    return tai_range_local_var;
}


void tai_range_free(tai_range_t *tai_range) {
    if(NULL == tai_range){
        return ;
    }
    listEntry_t *listEntry;
    if (tai_range->plmn_id) {
        plmn_id_free(tai_range->plmn_id);
        tai_range->plmn_id = NULL;
    }
    if (tai_range->tac_range_list) {
        list_ForEach(listEntry, tai_range->tac_range_list) {
            tac_range_free(listEntry->data);
        }
        list_freeList(tai_range->tac_range_list);
        tai_range->tac_range_list = NULL;
    }
    if (tai_range->nid) {
        free(tai_range->nid);
        tai_range->nid = NULL;
    }
    free(tai_range);
}

cJSON *tai_range_convertToJSON(tai_range_t *tai_range) {
    cJSON *item = cJSON_CreateObject();

    // tai_range->plmn_id
    if (!tai_range->plmn_id) {
        goto fail;
    }
    cJSON *plmn_id_local_JSON = plmn_id_convertToJSON(tai_range->plmn_id);
    if(plmn_id_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "plmnId", plmn_id_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }


    // tai_range->tac_range_list
    if (!tai_range->tac_range_list) {
        goto fail;
    }
    cJSON *tac_range_list = cJSON_AddArrayToObject(item, "tacRangeList");
    if(tac_range_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *tac_range_listListEntry;
    if (tai_range->tac_range_list) {
    list_ForEach(tac_range_listListEntry, tai_range->tac_range_list) {
    cJSON *itemLocal = tac_range_convertToJSON(tac_range_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(tac_range_list, itemLocal);
    }
    }


    // tai_range->nid
    if(tai_range->nid) {
    if(cJSON_AddStringToObject(item, "nid", tai_range->nid) == NULL) {
    goto fail; //String
    }
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

tai_range_t *tai_range_parseFromJSON(cJSON *tai_rangeJSON){

    tai_range_t *tai_range_local_var = NULL;

    // define the local variable for tai_range->plmn_id
    plmn_id_t *plmn_id_local_nonprim = NULL;

    // define the local list for tai_range->tac_range_list
    list_t *tac_range_listList = NULL;

    // tai_range->plmn_id
    cJSON *plmn_id = cJSON_GetObjectItemCaseSensitive(tai_rangeJSON, "plmnId");
    if (!plmn_id) {
        goto end;
    }

    
    plmn_id_local_nonprim = plmn_id_parseFromJSON(plmn_id); //nonprimitive

    // tai_range->tac_range_list
    cJSON *tac_range_list = cJSON_GetObjectItemCaseSensitive(tai_rangeJSON, "tacRangeList");
    if (!tac_range_list) {
        goto end;
    }

    
    cJSON *tac_range_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(tac_range_list)){
        goto end; //nonprimitive container
    }

    tac_range_listList = list_createList();

    cJSON_ArrayForEach(tac_range_list_local_nonprimitive,tac_range_list )
    {
        if(!cJSON_IsObject(tac_range_list_local_nonprimitive)){
            goto end;
        }
        tac_range_t *tac_range_listItem = tac_range_parseFromJSON(tac_range_list_local_nonprimitive);

        list_addElement(tac_range_listList, tac_range_listItem);
    }

    // tai_range->nid
    cJSON *nid = cJSON_GetObjectItemCaseSensitive(tai_rangeJSON, "nid");
    if (nid) { 
    if(!cJSON_IsString(nid) && !cJSON_IsNull(nid))
    {
    goto end; //String
    }
    }


    tai_range_local_var = tai_range_create (
        plmn_id_local_nonprim,
        tac_range_listList,
        nid && !cJSON_IsNull(nid) ? strdup(nid->valuestring) : NULL
        );

    return tai_range_local_var;
end:
    if (plmn_id_local_nonprim) {
        plmn_id_free(plmn_id_local_nonprim);
        plmn_id_local_nonprim = NULL;
    }
    if (tac_range_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, tac_range_listList) {
            tac_range_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(tac_range_listList);
        tac_range_listList = NULL;
    }
    return NULL;

}
