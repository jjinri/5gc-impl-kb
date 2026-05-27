#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "query_parameter.h"



query_parameter_t *query_parameter_create(
    char *name,
    char *value
    ) {
    query_parameter_t *query_parameter_local_var = malloc(sizeof(query_parameter_t));
    if (!query_parameter_local_var) {
        return NULL;
    }
    query_parameter_local_var->name = name;
    query_parameter_local_var->value = value;

    return query_parameter_local_var;
}


void query_parameter_free(query_parameter_t *query_parameter) {
    if(NULL == query_parameter){
        return ;
    }
    listEntry_t *listEntry;
    if (query_parameter->name) {
        free(query_parameter->name);
        query_parameter->name = NULL;
    }
    if (query_parameter->value) {
        free(query_parameter->value);
        query_parameter->value = NULL;
    }
    free(query_parameter);
}

cJSON *query_parameter_convertToJSON(query_parameter_t *query_parameter) {
    cJSON *item = cJSON_CreateObject();

    // query_parameter->name
    if (!query_parameter->name) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "name", query_parameter->name) == NULL) {
    goto fail; //String
    }


    // query_parameter->value
    if (!query_parameter->value) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "value", query_parameter->value) == NULL) {
    goto fail; //String
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

query_parameter_t *query_parameter_parseFromJSON(cJSON *query_parameterJSON){

    query_parameter_t *query_parameter_local_var = NULL;

    // query_parameter->name
    cJSON *name = cJSON_GetObjectItemCaseSensitive(query_parameterJSON, "name");
    if (!name) {
        goto end;
    }

    
    if(!cJSON_IsString(name))
    {
    goto end; //String
    }

    // query_parameter->value
    cJSON *value = cJSON_GetObjectItemCaseSensitive(query_parameterJSON, "value");
    if (!value) {
        goto end;
    }

    
    if(!cJSON_IsString(value))
    {
    goto end; //String
    }


    query_parameter_local_var = query_parameter_create (
        strdup(name->valuestring),
        strdup(value->valuestring)
        );

    return query_parameter_local_var;
end:
    return NULL;

}
