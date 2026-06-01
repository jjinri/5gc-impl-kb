#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "snssai_status.h"



snssai_status_t *snssai_status_create(
    ) {
    snssai_status_t *snssai_status_local_var = malloc(sizeof(snssai_status_t));
    if (!snssai_status_local_var) {
        return NULL;
    }

    return snssai_status_local_var;
}


void snssai_status_free(snssai_status_t *snssai_status) {
    if(NULL == snssai_status){
        return ;
    }
    listEntry_t *listEntry;
    free(snssai_status);
}

cJSON *snssai_status_convertToJSON(snssai_status_t *snssai_status) {
    cJSON *item = cJSON_CreateObject();
    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

snssai_status_t *snssai_status_parseFromJSON(cJSON *snssai_statusJSON){

    snssai_status_t *snssai_status_local_var = NULL;


    snssai_status_local_var = snssai_status_create (
        );

    return snssai_status_local_var;
end:
    return NULL;

}
