#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "redirect_response.h"



redirect_response_t *redirect_response_create(
    char *cause,
    char *target_scp,
    char *target_sepp
    ) {
    redirect_response_t *redirect_response_local_var = malloc(sizeof(redirect_response_t));
    if (!redirect_response_local_var) {
        return NULL;
    }
    redirect_response_local_var->cause = cause;
    redirect_response_local_var->target_scp = target_scp;
    redirect_response_local_var->target_sepp = target_sepp;

    return redirect_response_local_var;
}


void redirect_response_free(redirect_response_t *redirect_response) {
    if(NULL == redirect_response){
        return ;
    }
    listEntry_t *listEntry;
    if (redirect_response->cause) {
        free(redirect_response->cause);
        redirect_response->cause = NULL;
    }
    if (redirect_response->target_scp) {
        free(redirect_response->target_scp);
        redirect_response->target_scp = NULL;
    }
    if (redirect_response->target_sepp) {
        free(redirect_response->target_sepp);
        redirect_response->target_sepp = NULL;
    }
    free(redirect_response);
}

cJSON *redirect_response_convertToJSON(redirect_response_t *redirect_response) {
    cJSON *item = cJSON_CreateObject();

    // redirect_response->cause
    if(redirect_response->cause) {
    if(cJSON_AddStringToObject(item, "cause", redirect_response->cause) == NULL) {
    goto fail; //String
    }
    }


    // redirect_response->target_scp
    if(redirect_response->target_scp) {
    if(cJSON_AddStringToObject(item, "targetScp", redirect_response->target_scp) == NULL) {
    goto fail; //String
    }
    }


    // redirect_response->target_sepp
    if(redirect_response->target_sepp) {
    if(cJSON_AddStringToObject(item, "targetSepp", redirect_response->target_sepp) == NULL) {
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

redirect_response_t *redirect_response_parseFromJSON(cJSON *redirect_responseJSON){

    redirect_response_t *redirect_response_local_var = NULL;

    // redirect_response->cause
    cJSON *cause = cJSON_GetObjectItemCaseSensitive(redirect_responseJSON, "cause");
    if (cause) { 
    if(!cJSON_IsString(cause) && !cJSON_IsNull(cause))
    {
    goto end; //String
    }
    }

    // redirect_response->target_scp
    cJSON *target_scp = cJSON_GetObjectItemCaseSensitive(redirect_responseJSON, "targetScp");
    if (target_scp) { 
    if(!cJSON_IsString(target_scp) && !cJSON_IsNull(target_scp))
    {
    goto end; //String
    }
    }

    // redirect_response->target_sepp
    cJSON *target_sepp = cJSON_GetObjectItemCaseSensitive(redirect_responseJSON, "targetSepp");
    if (target_sepp) { 
    if(!cJSON_IsString(target_sepp) && !cJSON_IsNull(target_sepp))
    {
    goto end; //String
    }
    }


    redirect_response_local_var = redirect_response_create (
        cause && !cJSON_IsNull(cause) ? strdup(cause->valuestring) : NULL,
        target_scp && !cJSON_IsNull(target_scp) ? strdup(target_scp->valuestring) : NULL,
        target_sepp && !cJSON_IsNull(target_sepp) ? strdup(target_sepp->valuestring) : NULL
        );

    return redirect_response_local_var;
end:
    return NULL;

}
