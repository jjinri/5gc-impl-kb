#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "configured_snssai.h"



configured_snssai_t *configured_snssai_create(
    snssai_t *configured_snssai,
    snssai_t *mapped_home_snssai
    ) {
    configured_snssai_t *configured_snssai_local_var = malloc(sizeof(configured_snssai_t));
    if (!configured_snssai_local_var) {
        return NULL;
    }
    configured_snssai_local_var->configured_snssai = configured_snssai;
    configured_snssai_local_var->mapped_home_snssai = mapped_home_snssai;

    return configured_snssai_local_var;
}


void configured_snssai_free(configured_snssai_t *configured_snssai) {
    if(NULL == configured_snssai){
        return ;
    }
    listEntry_t *listEntry;
    if (configured_snssai->configured_snssai) {
        snssai_free(configured_snssai->configured_snssai);
        configured_snssai->configured_snssai = NULL;
    }
    if (configured_snssai->mapped_home_snssai) {
        snssai_free(configured_snssai->mapped_home_snssai);
        configured_snssai->mapped_home_snssai = NULL;
    }
    free(configured_snssai);
}

cJSON *configured_snssai_convertToJSON(configured_snssai_t *configured_snssai) {
    cJSON *item = cJSON_CreateObject();

    // configured_snssai->configured_snssai
    if (!configured_snssai->configured_snssai) {
        goto fail;
    }
    cJSON *configured_snssai_local_JSON = snssai_convertToJSON(configured_snssai->configured_snssai);
    if(configured_snssai_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "configuredSnssai", configured_snssai_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }


    // configured_snssai->mapped_home_snssai
    if(configured_snssai->mapped_home_snssai) {
    cJSON *mapped_home_snssai_local_JSON = snssai_convertToJSON(configured_snssai->mapped_home_snssai);
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

configured_snssai_t *configured_snssai_parseFromJSON(cJSON *configured_snssaiJSON){

    configured_snssai_t *configured_snssai_local_var = NULL;

    // define the local variable for configured_snssai->configured_snssai
    snssai_t *configured_snssai_local_nonprim = NULL;

    // define the local variable for configured_snssai->mapped_home_snssai
    snssai_t *mapped_home_snssai_local_nonprim = NULL;

    // configured_snssai->configured_snssai
    cJSON *configured_snssai = cJSON_GetObjectItemCaseSensitive(configured_snssaiJSON, "configuredSnssai");
    if (!configured_snssai) {
        goto end;
    }

    
    configured_snssai_local_nonprim = snssai_parseFromJSON(configured_snssai); //nonprimitive

    // configured_snssai->mapped_home_snssai
    cJSON *mapped_home_snssai = cJSON_GetObjectItemCaseSensitive(configured_snssaiJSON, "mappedHomeSnssai");
    if (mapped_home_snssai) { 
    mapped_home_snssai_local_nonprim = snssai_parseFromJSON(mapped_home_snssai); //nonprimitive
    }


    configured_snssai_local_var = configured_snssai_create (
        configured_snssai_local_nonprim,
        mapped_home_snssai ? mapped_home_snssai_local_nonprim : NULL
        );

    return configured_snssai_local_var;
end:
    if (configured_snssai_local_nonprim) {
        snssai_free(configured_snssai_local_nonprim);
        configured_snssai_local_nonprim = NULL;
    }
    if (mapped_home_snssai_local_nonprim) {
        snssai_free(mapped_home_snssai_local_nonprim);
        mapped_home_snssai_local_nonprim = NULL;
    }
    return NULL;

}
