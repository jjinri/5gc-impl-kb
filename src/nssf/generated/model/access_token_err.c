#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "access_token_err.h"


char* access_token_err_error_ToString(nssf_ns_selection_access_token_err_ERROR_e error) {
    char* errorArray[] =  { "NULL", "invalid_request", "invalid_client", "invalid_grant", "unauthorized_client", "unsupported_grant_type", "invalid_scope" };
    return errorArray[error];
}

nssf_ns_selection_access_token_err_ERROR_e access_token_err_error_FromString(char* error){
    int stringToReturn = 0;
    char *errorArray[] =  { "NULL", "invalid_request", "invalid_client", "invalid_grant", "unauthorized_client", "unsupported_grant_type", "invalid_scope" };
    size_t sizeofArray = sizeof(errorArray) / sizeof(errorArray[0]);
    while(stringToReturn < sizeofArray) {
        if(strcmp(error, errorArray[stringToReturn]) == 0) {
            return stringToReturn;
        }
        stringToReturn++;
    }
    return 0;
}

access_token_err_t *access_token_err_create(
    nssf_ns_selection_access_token_err_ERROR_e error,
    char *error_description,
    char *error_uri
    ) {
    access_token_err_t *access_token_err_local_var = malloc(sizeof(access_token_err_t));
    if (!access_token_err_local_var) {
        return NULL;
    }
    access_token_err_local_var->error = error;
    access_token_err_local_var->error_description = error_description;
    access_token_err_local_var->error_uri = error_uri;

    return access_token_err_local_var;
}


void access_token_err_free(access_token_err_t *access_token_err) {
    if(NULL == access_token_err){
        return ;
    }
    listEntry_t *listEntry;
    if (access_token_err->error_description) {
        free(access_token_err->error_description);
        access_token_err->error_description = NULL;
    }
    if (access_token_err->error_uri) {
        free(access_token_err->error_uri);
        access_token_err->error_uri = NULL;
    }
    free(access_token_err);
}

cJSON *access_token_err_convertToJSON(access_token_err_t *access_token_err) {
    cJSON *item = cJSON_CreateObject();

    // access_token_err->error
    if (nssf_ns_selection_access_token_err_ERROR_NULL == access_token_err->error) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "error", erroraccess_token_err_ToString(access_token_err->error)) == NULL)
    {
    goto fail; //Enum
    }


    // access_token_err->error_description
    if(access_token_err->error_description) {
    if(cJSON_AddStringToObject(item, "error_description", access_token_err->error_description) == NULL) {
    goto fail; //String
    }
    }


    // access_token_err->error_uri
    if(access_token_err->error_uri) {
    if(cJSON_AddStringToObject(item, "error_uri", access_token_err->error_uri) == NULL) {
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

access_token_err_t *access_token_err_parseFromJSON(cJSON *access_token_errJSON){

    access_token_err_t *access_token_err_local_var = NULL;

    // access_token_err->error
    cJSON *error = cJSON_GetObjectItemCaseSensitive(access_token_errJSON, "error");
    if (!error) {
        goto end;
    }

    nssf_ns_selection_access_token_err_ERROR_e errorVariable;
    
    if(!cJSON_IsString(error))
    {
    goto end; //Enum
    }
    errorVariable = access_token_err_error_FromString(error->valuestring);

    // access_token_err->error_description
    cJSON *error_description = cJSON_GetObjectItemCaseSensitive(access_token_errJSON, "error_description");
    if (error_description) { 
    if(!cJSON_IsString(error_description) && !cJSON_IsNull(error_description))
    {
    goto end; //String
    }
    }

    // access_token_err->error_uri
    cJSON *error_uri = cJSON_GetObjectItemCaseSensitive(access_token_errJSON, "error_uri");
    if (error_uri) { 
    if(!cJSON_IsString(error_uri) && !cJSON_IsNull(error_uri))
    {
    goto end; //String
    }
    }


    access_token_err_local_var = access_token_err_create (
        errorVariable,
        error_description && !cJSON_IsNull(error_description) ? strdup(error_description->valuestring) : NULL,
        error_uri && !cJSON_IsNull(error_uri) ? strdup(error_uri->valuestring) : NULL
        );

    return access_token_err_local_var;
end:
    return NULL;

}
