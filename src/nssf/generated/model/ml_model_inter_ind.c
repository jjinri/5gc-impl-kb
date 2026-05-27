#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ml_model_inter_ind.h"



ml_model_inter_ind_t *ml_model_inter_ind_create(
    object_t *analytics_id,
    list_t *vendor_list
    ) {
    ml_model_inter_ind_t *ml_model_inter_ind_local_var = malloc(sizeof(ml_model_inter_ind_t));
    if (!ml_model_inter_ind_local_var) {
        return NULL;
    }
    ml_model_inter_ind_local_var->analytics_id = analytics_id;
    ml_model_inter_ind_local_var->vendor_list = vendor_list;

    return ml_model_inter_ind_local_var;
}


void ml_model_inter_ind_free(ml_model_inter_ind_t *ml_model_inter_ind) {
    if(NULL == ml_model_inter_ind){
        return ;
    }
    listEntry_t *listEntry;
    if (ml_model_inter_ind->analytics_id) {
        object_free(ml_model_inter_ind->analytics_id);
        ml_model_inter_ind->analytics_id = NULL;
    }
    if (ml_model_inter_ind->vendor_list) {
        list_ForEach(listEntry, ml_model_inter_ind->vendor_list) {
            object_free(listEntry->data);
        }
        list_freeList(ml_model_inter_ind->vendor_list);
        ml_model_inter_ind->vendor_list = NULL;
    }
    free(ml_model_inter_ind);
}

cJSON *ml_model_inter_ind_convertToJSON(ml_model_inter_ind_t *ml_model_inter_ind) {
    cJSON *item = cJSON_CreateObject();

    // ml_model_inter_ind->analytics_id
    if (!ml_model_inter_ind->analytics_id) {
        goto fail;
    }
    cJSON *analytics_id_local_JSON = object_convertToJSON(ml_model_inter_ind->analytics_id);
    if(analytics_id_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "analyticsId", analytics_id_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }


    // ml_model_inter_ind->vendor_list
    if (!ml_model_inter_ind->vendor_list) {
        goto fail;
    }
    cJSON *vendor_list = cJSON_AddArrayToObject(item, "vendorList");
    if(vendor_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *vendor_listListEntry;
    if (ml_model_inter_ind->vendor_list) {
    list_ForEach(vendor_listListEntry, ml_model_inter_ind->vendor_list) {
    cJSON *itemLocal = object_convertToJSON(vendor_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(vendor_list, itemLocal);
    }
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

ml_model_inter_ind_t *ml_model_inter_ind_parseFromJSON(cJSON *ml_model_inter_indJSON){

    ml_model_inter_ind_t *ml_model_inter_ind_local_var = NULL;

    // define the local variable for ml_model_inter_ind->analytics_id
    object_t *analytics_id_local_nonprim = NULL;

    // define the local list for ml_model_inter_ind->vendor_list
    list_t *vendor_listList = NULL;

    // ml_model_inter_ind->analytics_id
    cJSON *analytics_id = cJSON_GetObjectItemCaseSensitive(ml_model_inter_indJSON, "analyticsId");
    if (!analytics_id) {
        goto end;
    }

    
    analytics_id_local_nonprim = object_parseFromJSON(analytics_id); //custom

    // ml_model_inter_ind->vendor_list
    cJSON *vendor_list = cJSON_GetObjectItemCaseSensitive(ml_model_inter_indJSON, "vendorList");
    if (!vendor_list) {
        goto end;
    }

    
    cJSON *vendor_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(vendor_list)){
        goto end; //nonprimitive container
    }

    vendor_listList = list_createList();

    cJSON_ArrayForEach(vendor_list_local_nonprimitive,vendor_list )
    {
        if(!cJSON_IsObject(vendor_list_local_nonprimitive)){
            goto end;
        }
        object_t *vendor_listItem = object_parseFromJSON(vendor_list_local_nonprimitive);

        list_addElement(vendor_listList, vendor_listItem);
    }


    ml_model_inter_ind_local_var = ml_model_inter_ind_create (
        analytics_id_local_nonprim,
        vendor_listList
        );

    return ml_model_inter_ind_local_var;
end:
    if (analytics_id_local_nonprim) {
        object_free(analytics_id_local_nonprim);
        analytics_id_local_nonprim = NULL;
    }
    if (vendor_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, vendor_listList) {
            object_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(vendor_listList);
        vendor_listList = NULL;
    }
    return NULL;

}
