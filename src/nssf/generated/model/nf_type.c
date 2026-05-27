#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nf_type.h"



nf_type_t *nf_type_create(
    ) {
    nf_type_t *nf_type_local_var = malloc(sizeof(nf_type_t));
    if (!nf_type_local_var) {
        return NULL;
    }

    return nf_type_local_var;
}


void nf_type_free(nf_type_t *nf_type) {
    if(NULL == nf_type){
        return ;
    }
    listEntry_t *listEntry;
    free(nf_type);
}

cJSON *nf_type_convertToJSON(nf_type_t *nf_type) {
    cJSON *item = cJSON_CreateObject();
    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

nf_type_t *nf_type_parseFromJSON(cJSON *nf_typeJSON){

    nf_type_t *nf_type_local_var = NULL;


    nf_type_local_var = nf_type_create (
        );

    return nf_type_local_var;
end:
    return NULL;

}
