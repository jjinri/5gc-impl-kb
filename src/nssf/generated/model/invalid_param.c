#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "invalid_param.h"



invalid_param_t *invalid_param_create(
    char *param,
    char *reason
    ) {
    invalid_param_t *invalid_param_local_var = malloc(sizeof(invalid_param_t));
    if (!invalid_param_local_var) {
        return NULL;
    }
    invalid_param_local_var->param = param;
    invalid_param_local_var->reason = reason;

    return invalid_param_local_var;
}


void invalid_param_free(invalid_param_t *invalid_param) {
    if(NULL == invalid_param){
        return ;
    }
    listEntry_t *listEntry;
    if (invalid_param->param) {
        free(invalid_param->param);
        invalid_param->param = NULL;
    }
    if (invalid_param->reason) {
        free(invalid_param->reason);
        invalid_param->reason = NULL;
    }
    free(invalid_param);
}

cJSON *invalid_param_convertToJSON(invalid_param_t *invalid_param) {
    cJSON *item = cJSON_CreateObject();

    // invalid_param->param
    if (!invalid_param->param) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "param", invalid_param->param) == NULL) {
    goto fail; //String
    }


    // invalid_param->reason
    if(invalid_param->reason) {
    if(cJSON_AddStringToObject(item, "reason", invalid_param->reason) == NULL) {
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

invalid_param_t *invalid_param_parseFromJSON(cJSON *invalid_paramJSON){

    invalid_param_t *invalid_param_local_var = NULL;

    // invalid_param->param
    cJSON *param = cJSON_GetObjectItemCaseSensitive(invalid_paramJSON, "param");
    if (!param) {
        goto end;
    }

    
    if(!cJSON_IsString(param))
    {
    goto end; //String
    }

    // invalid_param->reason
    cJSON *reason = cJSON_GetObjectItemCaseSensitive(invalid_paramJSON, "reason");
    if (reason) { 
    if(!cJSON_IsString(reason) && !cJSON_IsNull(reason))
    {
    goto end; //String
    }
    }


    invalid_param_local_var = invalid_param_create (
        strdup(param->valuestring),
        reason && !cJSON_IsNull(reason) ? strdup(reason->valuestring) : NULL
        );

    return invalid_param_local_var;
end:
    return NULL;

}
