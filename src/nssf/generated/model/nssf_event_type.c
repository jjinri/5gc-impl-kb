#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nssf_event_type.h"



nssf_event_type_t *nssf_event_type_create(
    ) {
    nssf_event_type_t *nssf_event_type_local_var = malloc(sizeof(nssf_event_type_t));
    if (!nssf_event_type_local_var) {
        return NULL;
    }

    return nssf_event_type_local_var;
}


void nssf_event_type_free(nssf_event_type_t *nssf_event_type) {
    if(NULL == nssf_event_type){
        return ;
    }
    listEntry_t *listEntry;
    free(nssf_event_type);
}

cJSON *nssf_event_type_convertToJSON(nssf_event_type_t *nssf_event_type) {
    cJSON *item = cJSON_CreateObject();
    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

nssf_event_type_t *nssf_event_type_parseFromJSON(cJSON *nssf_event_typeJSON){

    nssf_event_type_t *nssf_event_type_local_var = NULL;


    nssf_event_type_local_var = nssf_event_type_create (
        );

    return nssf_event_type_local_var;
end:
    return NULL;

}
