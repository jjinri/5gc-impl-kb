#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "snssai_extension.h"



snssai_extension_t *snssai_extension_create(
    list_t *sd_ranges,
    int wildcard_sd
    ) {
    snssai_extension_t *snssai_extension_local_var = malloc(sizeof(snssai_extension_t));
    if (!snssai_extension_local_var) {
        return NULL;
    }
    snssai_extension_local_var->sd_ranges = sd_ranges;
    snssai_extension_local_var->wildcard_sd = wildcard_sd;

    return snssai_extension_local_var;
}


void snssai_extension_free(snssai_extension_t *snssai_extension) {
    if(NULL == snssai_extension){
        return ;
    }
    listEntry_t *listEntry;
    if (snssai_extension->sd_ranges) {
        list_ForEach(listEntry, snssai_extension->sd_ranges) {
            sd_range_free(listEntry->data);
        }
        list_freeList(snssai_extension->sd_ranges);
        snssai_extension->sd_ranges = NULL;
    }
    free(snssai_extension);
}

cJSON *snssai_extension_convertToJSON(snssai_extension_t *snssai_extension) {
    cJSON *item = cJSON_CreateObject();

    // snssai_extension->sd_ranges
    if(snssai_extension->sd_ranges) {
    cJSON *sd_ranges = cJSON_AddArrayToObject(item, "sdRanges");
    if(sd_ranges == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *sd_rangesListEntry;
    if (snssai_extension->sd_ranges) {
    list_ForEach(sd_rangesListEntry, snssai_extension->sd_ranges) {
    cJSON *itemLocal = sd_range_convertToJSON(sd_rangesListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(sd_ranges, itemLocal);
    }
    }
    }


    // snssai_extension->wildcard_sd
    if(snssai_extension->wildcard_sd) {
    if(cJSON_AddBoolToObject(item, "wildcardSd", snssai_extension->wildcard_sd) == NULL) {
    goto fail; //Bool
    }
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

snssai_extension_t *snssai_extension_parseFromJSON(cJSON *snssai_extensionJSON){

    snssai_extension_t *snssai_extension_local_var = NULL;

    // define the local list for snssai_extension->sd_ranges
    list_t *sd_rangesList = NULL;

    // snssai_extension->sd_ranges
    cJSON *sd_ranges = cJSON_GetObjectItemCaseSensitive(snssai_extensionJSON, "sdRanges");
    if (sd_ranges) { 
    cJSON *sd_ranges_local_nonprimitive = NULL;
    if(!cJSON_IsArray(sd_ranges)){
        goto end; //nonprimitive container
    }

    sd_rangesList = list_createList();

    cJSON_ArrayForEach(sd_ranges_local_nonprimitive,sd_ranges )
    {
        if(!cJSON_IsObject(sd_ranges_local_nonprimitive)){
            goto end;
        }
        sd_range_t *sd_rangesItem = sd_range_parseFromJSON(sd_ranges_local_nonprimitive);

        list_addElement(sd_rangesList, sd_rangesItem);
    }
    }

    // snssai_extension->wildcard_sd
    cJSON *wildcard_sd = cJSON_GetObjectItemCaseSensitive(snssai_extensionJSON, "wildcardSd");
    if (wildcard_sd) { 
    if(!cJSON_IsBool(wildcard_sd))
    {
    goto end; //Bool
    }
    }


    snssai_extension_local_var = snssai_extension_create (
        sd_ranges ? sd_rangesList : NULL,
        wildcard_sd ? wildcard_sd->valueint : 0
        );

    return snssai_extension_local_var;
end:
    if (sd_rangesList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, sd_rangesList) {
            sd_range_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(sd_rangesList);
        sd_rangesList = NULL;
    }
    return NULL;

}
