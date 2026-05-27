#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "no_profile_match_reason.h"



no_profile_match_reason_t *no_profile_match_reason_create(
    ) {
    no_profile_match_reason_t *no_profile_match_reason_local_var = malloc(sizeof(no_profile_match_reason_t));
    if (!no_profile_match_reason_local_var) {
        return NULL;
    }

    return no_profile_match_reason_local_var;
}


void no_profile_match_reason_free(no_profile_match_reason_t *no_profile_match_reason) {
    if(NULL == no_profile_match_reason){
        return ;
    }
    listEntry_t *listEntry;
    free(no_profile_match_reason);
}

cJSON *no_profile_match_reason_convertToJSON(no_profile_match_reason_t *no_profile_match_reason) {
    cJSON *item = cJSON_CreateObject();
    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

no_profile_match_reason_t *no_profile_match_reason_parseFromJSON(cJSON *no_profile_match_reasonJSON){

    no_profile_match_reason_t *no_profile_match_reason_local_var = NULL;


    no_profile_match_reason_local_var = no_profile_match_reason_create (
        );

    return no_profile_match_reason_local_var;
end:
    return NULL;

}
