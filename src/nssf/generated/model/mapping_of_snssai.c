#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mapping_of_snssai.h"



mapping_of_snssai_t *mapping_of_snssai_create(
    snssai_t *serving_snssai,
    snssai_t *home_snssai
    ) {
    mapping_of_snssai_t *mapping_of_snssai_local_var = malloc(sizeof(mapping_of_snssai_t));
    if (!mapping_of_snssai_local_var) {
        return NULL;
    }
    mapping_of_snssai_local_var->serving_snssai = serving_snssai;
    mapping_of_snssai_local_var->home_snssai = home_snssai;

    return mapping_of_snssai_local_var;
}


void mapping_of_snssai_free(mapping_of_snssai_t *mapping_of_snssai) {
    if(NULL == mapping_of_snssai){
        return ;
    }
    listEntry_t *listEntry;
    if (mapping_of_snssai->serving_snssai) {
        snssai_free(mapping_of_snssai->serving_snssai);
        mapping_of_snssai->serving_snssai = NULL;
    }
    if (mapping_of_snssai->home_snssai) {
        snssai_free(mapping_of_snssai->home_snssai);
        mapping_of_snssai->home_snssai = NULL;
    }
    free(mapping_of_snssai);
}

cJSON *mapping_of_snssai_convertToJSON(mapping_of_snssai_t *mapping_of_snssai) {
    cJSON *item = cJSON_CreateObject();

    // mapping_of_snssai->serving_snssai
    if (!mapping_of_snssai->serving_snssai) {
        goto fail;
    }
    cJSON *serving_snssai_local_JSON = snssai_convertToJSON(mapping_of_snssai->serving_snssai);
    if(serving_snssai_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "servingSnssai", serving_snssai_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }


    // mapping_of_snssai->home_snssai
    if (!mapping_of_snssai->home_snssai) {
        goto fail;
    }
    cJSON *home_snssai_local_JSON = snssai_convertToJSON(mapping_of_snssai->home_snssai);
    if(home_snssai_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "homeSnssai", home_snssai_local_JSON);
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

mapping_of_snssai_t *mapping_of_snssai_parseFromJSON(cJSON *mapping_of_snssaiJSON){

    mapping_of_snssai_t *mapping_of_snssai_local_var = NULL;

    // define the local variable for mapping_of_snssai->serving_snssai
    snssai_t *serving_snssai_local_nonprim = NULL;

    // define the local variable for mapping_of_snssai->home_snssai
    snssai_t *home_snssai_local_nonprim = NULL;

    // mapping_of_snssai->serving_snssai
    cJSON *serving_snssai = cJSON_GetObjectItemCaseSensitive(mapping_of_snssaiJSON, "servingSnssai");
    if (!serving_snssai) {
        goto end;
    }

    
    serving_snssai_local_nonprim = snssai_parseFromJSON(serving_snssai); //nonprimitive

    // mapping_of_snssai->home_snssai
    cJSON *home_snssai = cJSON_GetObjectItemCaseSensitive(mapping_of_snssaiJSON, "homeSnssai");
    if (!home_snssai) {
        goto end;
    }

    
    home_snssai_local_nonprim = snssai_parseFromJSON(home_snssai); //nonprimitive


    mapping_of_snssai_local_var = mapping_of_snssai_create (
        serving_snssai_local_nonprim,
        home_snssai_local_nonprim
        );

    return mapping_of_snssai_local_var;
end:
    if (serving_snssai_local_nonprim) {
        snssai_free(serving_snssai_local_nonprim);
        serving_snssai_local_nonprim = NULL;
    }
    if (home_snssai_local_nonprim) {
        snssai_free(home_snssai_local_nonprim);
        home_snssai_local_nonprim = NULL;
    }
    return NULL;

}
