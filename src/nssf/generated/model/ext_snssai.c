#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ext_snssai.h"



ext_snssai_t *ext_snssai_create(
    int sst,
    char *sd,
    list_t *sd_ranges,
    int wildcard_sd
    ) {
    ext_snssai_t *ext_snssai_local_var = malloc(sizeof(ext_snssai_t));
    if (!ext_snssai_local_var) {
        return NULL;
    }
    ext_snssai_local_var->sst = sst;
    ext_snssai_local_var->sd = sd;
    ext_snssai_local_var->sd_ranges = sd_ranges;
    ext_snssai_local_var->wildcard_sd = wildcard_sd;

    return ext_snssai_local_var;
}


void ext_snssai_free(ext_snssai_t *ext_snssai) {
    if(NULL == ext_snssai){
        return ;
    }
    listEntry_t *listEntry;
    if (ext_snssai->sd) {
        free(ext_snssai->sd);
        ext_snssai->sd = NULL;
    }
    if (ext_snssai->sd_ranges) {
        list_ForEach(listEntry, ext_snssai->sd_ranges) {
            sd_range_free(listEntry->data);
        }
        list_freeList(ext_snssai->sd_ranges);
        ext_snssai->sd_ranges = NULL;
    }
    free(ext_snssai);
}

cJSON *ext_snssai_convertToJSON(ext_snssai_t *ext_snssai) {
    cJSON *item = cJSON_CreateObject();

    // ext_snssai->sst
    if (!ext_snssai->sst) {
        goto fail;
    }
    if(cJSON_AddNumberToObject(item, "sst", ext_snssai->sst) == NULL) {
    goto fail; //Numeric
    }


    // ext_snssai->sd
    if(ext_snssai->sd) {
    if(cJSON_AddStringToObject(item, "sd", ext_snssai->sd) == NULL) {
    goto fail; //String
    }
    }


    // ext_snssai->sd_ranges
    if(ext_snssai->sd_ranges) {
    cJSON *sd_ranges = cJSON_AddArrayToObject(item, "sdRanges");
    if(sd_ranges == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *sd_rangesListEntry;
    if (ext_snssai->sd_ranges) {
    list_ForEach(sd_rangesListEntry, ext_snssai->sd_ranges) {
    cJSON *itemLocal = sd_range_convertToJSON(sd_rangesListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(sd_ranges, itemLocal);
    }
    }
    }


    // ext_snssai->wildcard_sd
    if(ext_snssai->wildcard_sd) {
    if(cJSON_AddBoolToObject(item, "wildcardSd", ext_snssai->wildcard_sd) == NULL) {
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

ext_snssai_t *ext_snssai_parseFromJSON(cJSON *ext_snssaiJSON){

    ext_snssai_t *ext_snssai_local_var = NULL;

    // define the local list for ext_snssai->sd_ranges
    list_t *sd_rangesList = NULL;

    // ext_snssai->sst
    cJSON *sst = cJSON_GetObjectItemCaseSensitive(ext_snssaiJSON, "sst");
    if (!sst) {
        goto end;
    }

    
    if(!cJSON_IsNumber(sst))
    {
    goto end; //Numeric
    }

    // ext_snssai->sd
    cJSON *sd = cJSON_GetObjectItemCaseSensitive(ext_snssaiJSON, "sd");
    if (sd) { 
    if(!cJSON_IsString(sd) && !cJSON_IsNull(sd))
    {
    goto end; //String
    }
    }

    // ext_snssai->sd_ranges
    cJSON *sd_ranges = cJSON_GetObjectItemCaseSensitive(ext_snssaiJSON, "sdRanges");
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

    // ext_snssai->wildcard_sd
    cJSON *wildcard_sd = cJSON_GetObjectItemCaseSensitive(ext_snssaiJSON, "wildcardSd");
    if (wildcard_sd) { 
    if(!cJSON_IsBool(wildcard_sd))
    {
    goto end; //Bool
    }
    }


    ext_snssai_local_var = ext_snssai_create (
        sst->valuedouble,
        sd && !cJSON_IsNull(sd) ? strdup(sd->valuestring) : NULL,
        sd_ranges ? sd_rangesList : NULL,
        wildcard_sd ? wildcard_sd->valueint : 0
        );

    return ext_snssai_local_var;
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
