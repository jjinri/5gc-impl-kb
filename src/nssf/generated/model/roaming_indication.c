#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "roaming_indication.h"



roaming_indication_t *roaming_indication_create(
    ) {
    roaming_indication_t *roaming_indication_local_var = malloc(sizeof(roaming_indication_t));
    if (!roaming_indication_local_var) {
        return NULL;
    }

    return roaming_indication_local_var;
}


void roaming_indication_free(roaming_indication_t *roaming_indication) {
    if(NULL == roaming_indication){
        return ;
    }
    listEntry_t *listEntry;
    free(roaming_indication);
}

cJSON *roaming_indication_convertToJSON(roaming_indication_t *roaming_indication) {
    cJSON *item = cJSON_CreateObject();
    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

roaming_indication_t *roaming_indication_parseFromJSON(cJSON *roaming_indicationJSON){

    roaming_indication_t *roaming_indication_local_var = NULL;


    roaming_indication_local_var = roaming_indication_create (
        );

    return roaming_indication_local_var;
end:
    return NULL;

}
