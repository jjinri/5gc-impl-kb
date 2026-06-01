#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "termination_indication.h"



termination_indication_t *termination_indication_create(
    ) {
    termination_indication_t *termination_indication_local_var = malloc(sizeof(termination_indication_t));
    if (!termination_indication_local_var) {
        return NULL;
    }

    return termination_indication_local_var;
}


void termination_indication_free(termination_indication_t *termination_indication) {
    if(NULL == termination_indication){
        return ;
    }
    listEntry_t *listEntry;
    free(termination_indication);
}

cJSON *termination_indication_convertToJSON(termination_indication_t *termination_indication) {
    cJSON *item = cJSON_CreateObject();
    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

termination_indication_t *termination_indication_parseFromJSON(cJSON *termination_indicationJSON){

    termination_indication_t *termination_indication_local_var = NULL;


    termination_indication_local_var = termination_indication_create (
        );

    return termination_indication_local_var;
end:
    return NULL;

}
