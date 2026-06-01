#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sd_range.h"



sd_range_t *sd_range_create(
    char *start,
    char *end
    ) {
    sd_range_t *sd_range_local_var = malloc(sizeof(sd_range_t));
    if (!sd_range_local_var) {
        return NULL;
    }
    sd_range_local_var->start = start;
    sd_range_local_var->end = end;

    return sd_range_local_var;
}


void sd_range_free(sd_range_t *sd_range) {
    if(NULL == sd_range){
        return ;
    }
    listEntry_t *listEntry;
    if (sd_range->start) {
        free(sd_range->start);
        sd_range->start = NULL;
    }
    if (sd_range->end) {
        free(sd_range->end);
        sd_range->end = NULL;
    }
    free(sd_range);
}

cJSON *sd_range_convertToJSON(sd_range_t *sd_range) {
    cJSON *item = cJSON_CreateObject();

    // sd_range->start
    if(sd_range->start) {
    if(cJSON_AddStringToObject(item, "start", sd_range->start) == NULL) {
    goto fail; //String
    }
    }


    // sd_range->end
    if(sd_range->end) {
    if(cJSON_AddStringToObject(item, "end", sd_range->end) == NULL) {
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

sd_range_t *sd_range_parseFromJSON(cJSON *sd_rangeJSON){

    sd_range_t *sd_range_local_var = NULL;

    // sd_range->start
    cJSON *start = cJSON_GetObjectItemCaseSensitive(sd_rangeJSON, "start");
    if (start) { 
    if(!cJSON_IsString(start) && !cJSON_IsNull(start))
    {
    goto end; //String
    }
    }

    // sd_range->end
    cJSON *end = cJSON_GetObjectItemCaseSensitive(sd_rangeJSON, "end");
    if (end) { 
    if(!cJSON_IsString(end) && !cJSON_IsNull(end))
    {
    goto end; //String
    }
    }


    sd_range_local_var = sd_range_create (
        start && !cJSON_IsNull(start) ? strdup(start->valuestring) : NULL,
        end && !cJSON_IsNull(end) ? strdup(end->valuestring) : NULL
        );

    return sd_range_local_var;
end:
    return NULL;

}
