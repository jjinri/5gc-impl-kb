#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "problem_details.h"



problem_details_t *problem_details_create(
    char *type,
    char *title,
    int status,
    char *detail,
    char *instance,
    char *cause,
    list_t *invalid_params,
    char *supported_features,
    access_token_err_t *access_token_error,
    access_token_req_t *access_token_request,
    char *nrf_id,
    list_t *supported_api_versions,
    no_profile_match_info_t *no_profile_match_info
    ) {
    problem_details_t *problem_details_local_var = malloc(sizeof(problem_details_t));
    if (!problem_details_local_var) {
        return NULL;
    }
    problem_details_local_var->type = type;
    problem_details_local_var->title = title;
    problem_details_local_var->status = status;
    problem_details_local_var->detail = detail;
    problem_details_local_var->instance = instance;
    problem_details_local_var->cause = cause;
    problem_details_local_var->invalid_params = invalid_params;
    problem_details_local_var->supported_features = supported_features;
    problem_details_local_var->access_token_error = access_token_error;
    problem_details_local_var->access_token_request = access_token_request;
    problem_details_local_var->nrf_id = nrf_id;
    problem_details_local_var->supported_api_versions = supported_api_versions;
    problem_details_local_var->no_profile_match_info = no_profile_match_info;

    return problem_details_local_var;
}


void problem_details_free(problem_details_t *problem_details) {
    if(NULL == problem_details){
        return ;
    }
    listEntry_t *listEntry;
    if (problem_details->type) {
        free(problem_details->type);
        problem_details->type = NULL;
    }
    if (problem_details->title) {
        free(problem_details->title);
        problem_details->title = NULL;
    }
    if (problem_details->detail) {
        free(problem_details->detail);
        problem_details->detail = NULL;
    }
    if (problem_details->instance) {
        free(problem_details->instance);
        problem_details->instance = NULL;
    }
    if (problem_details->cause) {
        free(problem_details->cause);
        problem_details->cause = NULL;
    }
    if (problem_details->invalid_params) {
        list_ForEach(listEntry, problem_details->invalid_params) {
            invalid_param_free(listEntry->data);
        }
        list_freeList(problem_details->invalid_params);
        problem_details->invalid_params = NULL;
    }
    if (problem_details->supported_features) {
        free(problem_details->supported_features);
        problem_details->supported_features = NULL;
    }
    if (problem_details->access_token_error) {
        access_token_err_free(problem_details->access_token_error);
        problem_details->access_token_error = NULL;
    }
    if (problem_details->access_token_request) {
        access_token_req_free(problem_details->access_token_request);
        problem_details->access_token_request = NULL;
    }
    if (problem_details->nrf_id) {
        free(problem_details->nrf_id);
        problem_details->nrf_id = NULL;
    }
    if (problem_details->supported_api_versions) {
        list_ForEach(listEntry, problem_details->supported_api_versions) {
            free(listEntry->data);
        }
        list_freeList(problem_details->supported_api_versions);
        problem_details->supported_api_versions = NULL;
    }
    if (problem_details->no_profile_match_info) {
        no_profile_match_info_free(problem_details->no_profile_match_info);
        problem_details->no_profile_match_info = NULL;
    }
    free(problem_details);
}

cJSON *problem_details_convertToJSON(problem_details_t *problem_details) {
    cJSON *item = cJSON_CreateObject();

    // problem_details->type
    if(problem_details->type) {
    if(cJSON_AddStringToObject(item, "type", problem_details->type) == NULL) {
    goto fail; //String
    }
    }


    // problem_details->title
    if(problem_details->title) {
    if(cJSON_AddStringToObject(item, "title", problem_details->title) == NULL) {
    goto fail; //String
    }
    }


    // problem_details->status
    if(problem_details->status) {
    if(cJSON_AddNumberToObject(item, "status", problem_details->status) == NULL) {
    goto fail; //Numeric
    }
    }


    // problem_details->detail
    if(problem_details->detail) {
    if(cJSON_AddStringToObject(item, "detail", problem_details->detail) == NULL) {
    goto fail; //String
    }
    }


    // problem_details->instance
    if(problem_details->instance) {
    if(cJSON_AddStringToObject(item, "instance", problem_details->instance) == NULL) {
    goto fail; //String
    }
    }


    // problem_details->cause
    if(problem_details->cause) {
    if(cJSON_AddStringToObject(item, "cause", problem_details->cause) == NULL) {
    goto fail; //String
    }
    }


    // problem_details->invalid_params
    if(problem_details->invalid_params) {
    cJSON *invalid_params = cJSON_AddArrayToObject(item, "invalidParams");
    if(invalid_params == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *invalid_paramsListEntry;
    if (problem_details->invalid_params) {
    list_ForEach(invalid_paramsListEntry, problem_details->invalid_params) {
    cJSON *itemLocal = invalid_param_convertToJSON(invalid_paramsListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(invalid_params, itemLocal);
    }
    }
    }


    // problem_details->supported_features
    if(problem_details->supported_features) {
    if(cJSON_AddStringToObject(item, "supportedFeatures", problem_details->supported_features) == NULL) {
    goto fail; //String
    }
    }


    // problem_details->access_token_error
    if(problem_details->access_token_error) {
    cJSON *access_token_error_local_JSON = access_token_err_convertToJSON(problem_details->access_token_error);
    if(access_token_error_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "accessTokenError", access_token_error_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // problem_details->access_token_request
    if(problem_details->access_token_request) {
    cJSON *access_token_request_local_JSON = access_token_req_convertToJSON(problem_details->access_token_request);
    if(access_token_request_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "accessTokenRequest", access_token_request_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // problem_details->nrf_id
    if(problem_details->nrf_id) {
    if(cJSON_AddStringToObject(item, "nrfId", problem_details->nrf_id) == NULL) {
    goto fail; //String
    }
    }


    // problem_details->supported_api_versions
    if(problem_details->supported_api_versions) {
    cJSON *supported_api_versions = cJSON_AddArrayToObject(item, "supportedApiVersions");
    if(supported_api_versions == NULL) {
        goto fail; //primitive container
    }

    listEntry_t *supported_api_versionsListEntry;
    list_ForEach(supported_api_versionsListEntry, problem_details->supported_api_versions) {
    if(cJSON_AddStringToObject(supported_api_versions, "", (char*)supported_api_versionsListEntry->data) == NULL)
    {
        goto fail;
    }
    }
    }


    // problem_details->no_profile_match_info
    if(problem_details->no_profile_match_info) {
    cJSON *no_profile_match_info_local_JSON = no_profile_match_info_convertToJSON(problem_details->no_profile_match_info);
    if(no_profile_match_info_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "noProfileMatchInfo", no_profile_match_info_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

problem_details_t *problem_details_parseFromJSON(cJSON *problem_detailsJSON){

    problem_details_t *problem_details_local_var = NULL;

    // define the local list for problem_details->invalid_params
    list_t *invalid_paramsList = NULL;

    // define the local variable for problem_details->access_token_error
    access_token_err_t *access_token_error_local_nonprim = NULL;

    // define the local variable for problem_details->access_token_request
    access_token_req_t *access_token_request_local_nonprim = NULL;

    // define the local list for problem_details->supported_api_versions
    list_t *supported_api_versionsList = NULL;

    // define the local variable for problem_details->no_profile_match_info
    no_profile_match_info_t *no_profile_match_info_local_nonprim = NULL;

    // problem_details->type
    cJSON *type = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "type");
    if (type) { 
    if(!cJSON_IsString(type) && !cJSON_IsNull(type))
    {
    goto end; //String
    }
    }

    // problem_details->title
    cJSON *title = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "title");
    if (title) { 
    if(!cJSON_IsString(title) && !cJSON_IsNull(title))
    {
    goto end; //String
    }
    }

    // problem_details->status
    cJSON *status = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "status");
    if (status) { 
    if(!cJSON_IsNumber(status))
    {
    goto end; //Numeric
    }
    }

    // problem_details->detail
    cJSON *detail = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "detail");
    if (detail) { 
    if(!cJSON_IsString(detail) && !cJSON_IsNull(detail))
    {
    goto end; //String
    }
    }

    // problem_details->instance
    cJSON *instance = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "instance");
    if (instance) { 
    if(!cJSON_IsString(instance) && !cJSON_IsNull(instance))
    {
    goto end; //String
    }
    }

    // problem_details->cause
    cJSON *cause = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "cause");
    if (cause) { 
    if(!cJSON_IsString(cause) && !cJSON_IsNull(cause))
    {
    goto end; //String
    }
    }

    // problem_details->invalid_params
    cJSON *invalid_params = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "invalidParams");
    if (invalid_params) { 
    cJSON *invalid_params_local_nonprimitive = NULL;
    if(!cJSON_IsArray(invalid_params)){
        goto end; //nonprimitive container
    }

    invalid_paramsList = list_createList();

    cJSON_ArrayForEach(invalid_params_local_nonprimitive,invalid_params )
    {
        if(!cJSON_IsObject(invalid_params_local_nonprimitive)){
            goto end;
        }
        invalid_param_t *invalid_paramsItem = invalid_param_parseFromJSON(invalid_params_local_nonprimitive);

        list_addElement(invalid_paramsList, invalid_paramsItem);
    }
    }

    // problem_details->supported_features
    cJSON *supported_features = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "supportedFeatures");
    if (supported_features) { 
    if(!cJSON_IsString(supported_features) && !cJSON_IsNull(supported_features))
    {
    goto end; //String
    }
    }

    // problem_details->access_token_error
    cJSON *access_token_error = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "accessTokenError");
    if (access_token_error) { 
    access_token_error_local_nonprim = access_token_err_parseFromJSON(access_token_error); //nonprimitive
    }

    // problem_details->access_token_request
    cJSON *access_token_request = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "accessTokenRequest");
    if (access_token_request) { 
    access_token_request_local_nonprim = access_token_req_parseFromJSON(access_token_request); //nonprimitive
    }

    // problem_details->nrf_id
    cJSON *nrf_id = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "nrfId");
    if (nrf_id) { 
    if(!cJSON_IsString(nrf_id) && !cJSON_IsNull(nrf_id))
    {
    goto end; //String
    }
    }

    // problem_details->supported_api_versions
    cJSON *supported_api_versions = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "supportedApiVersions");
    if (supported_api_versions) { 
    cJSON *supported_api_versions_local = NULL;
    if(!cJSON_IsArray(supported_api_versions)) {
        goto end;//primitive container
    }
    supported_api_versionsList = list_createList();

    cJSON_ArrayForEach(supported_api_versions_local, supported_api_versions)
    {
        if(!cJSON_IsString(supported_api_versions_local))
        {
            goto end;
        }
        list_addElement(supported_api_versionsList , strdup(supported_api_versions_local->valuestring));
    }
    }

    // problem_details->no_profile_match_info
    cJSON *no_profile_match_info = cJSON_GetObjectItemCaseSensitive(problem_detailsJSON, "noProfileMatchInfo");
    if (no_profile_match_info) { 
    no_profile_match_info_local_nonprim = no_profile_match_info_parseFromJSON(no_profile_match_info); //nonprimitive
    }


    problem_details_local_var = problem_details_create (
        type && !cJSON_IsNull(type) ? strdup(type->valuestring) : NULL,
        title && !cJSON_IsNull(title) ? strdup(title->valuestring) : NULL,
        status ? status->valuedouble : 0,
        detail && !cJSON_IsNull(detail) ? strdup(detail->valuestring) : NULL,
        instance && !cJSON_IsNull(instance) ? strdup(instance->valuestring) : NULL,
        cause && !cJSON_IsNull(cause) ? strdup(cause->valuestring) : NULL,
        invalid_params ? invalid_paramsList : NULL,
        supported_features && !cJSON_IsNull(supported_features) ? strdup(supported_features->valuestring) : NULL,
        access_token_error ? access_token_error_local_nonprim : NULL,
        access_token_request ? access_token_request_local_nonprim : NULL,
        nrf_id && !cJSON_IsNull(nrf_id) ? strdup(nrf_id->valuestring) : NULL,
        supported_api_versions ? supported_api_versionsList : NULL,
        no_profile_match_info ? no_profile_match_info_local_nonprim : NULL
        );

    return problem_details_local_var;
end:
    if (invalid_paramsList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, invalid_paramsList) {
            invalid_param_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(invalid_paramsList);
        invalid_paramsList = NULL;
    }
    if (access_token_error_local_nonprim) {
        access_token_err_free(access_token_error_local_nonprim);
        access_token_error_local_nonprim = NULL;
    }
    if (access_token_request_local_nonprim) {
        access_token_req_free(access_token_request_local_nonprim);
        access_token_request_local_nonprim = NULL;
    }
    if (supported_api_versionsList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, supported_api_versionsList) {
            free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(supported_api_versionsList);
        supported_api_versionsList = NULL;
    }
    if (no_profile_match_info_local_nonprim) {
        no_profile_match_info_free(no_profile_match_info_local_nonprim);
        no_profile_match_info_local_nonprim = NULL;
    }
    return NULL;

}
