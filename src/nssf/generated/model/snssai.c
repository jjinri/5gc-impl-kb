#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "snssai.h"



snssai_t *snssai_create(
    int sst,
    char *sd
    ) {
    snssai_t *snssai_local_var = malloc(sizeof(snssai_t));
    if (!snssai_local_var) {
        return NULL;
    }
    snssai_local_var->sst = sst;
    snssai_local_var->sd = sd;

    return snssai_local_var;
}


void snssai_free(snssai_t *snssai) {
    if(NULL == snssai){
        return ;
    }
    listEntry_t *listEntry;
    if (snssai->sd) {
        free(snssai->sd);
        snssai->sd = NULL;
    }
    free(snssai);
}

cJSON *snssai_convertToJSON(snssai_t *snssai) {
    cJSON *item = cJSON_CreateObject();

    // snssai->sst
    if (!snssai->sst) {
        goto fail;
    }
    if(cJSON_AddNumberToObject(item, "sst", snssai->sst) == NULL) {
    goto fail; //Numeric
    }


    // snssai->sd
    if(snssai->sd) {
    if(cJSON_AddStringToObject(item, "sd", snssai->sd) == NULL) {
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

snssai_t *snssai_parseFromJSON(cJSON *snssaiJSON){

    snssai_t *snssai_local_var = NULL;

    // snssai->sst
    cJSON *sst = cJSON_GetObjectItemCaseSensitive(snssaiJSON, "sst");
    if (!sst) {
        goto end;
    }

    
    if(!cJSON_IsNumber(sst))
    {
    goto end; //Numeric
    }

    // snssai->sd
    cJSON *sd = cJSON_GetObjectItemCaseSensitive(snssaiJSON, "sd");
    if (sd) { 
    if(!cJSON_IsString(sd) && !cJSON_IsNull(sd))
    {
    goto end; //String
    }
    }


    snssai_local_var = snssai_create (
        sst->valuedouble,
        sd && !cJSON_IsNull(sd) ? strdup(sd->valuestring) : NULL
        );

    return snssai_local_var;
end:
    return NULL;

}
