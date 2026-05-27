/*
 * problem_details.h
 *
 * Provides additional information in an error response.
 */

#ifndef _problem_details_H_
#define _problem_details_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct problem_details_t problem_details_t;

#include "access_token_err.h"
#include "access_token_req.h"
#include "invalid_param.h"
#include "no_profile_match_info.h"



typedef struct problem_details_t {
    char *type; // string
    char *title; // string
    int status; //numeric
    char *detail; // string
    char *instance; // string
    char *cause; // string
    list_t *invalid_params; //nonprimitive container
    char *supported_features; // string
    struct access_token_err_t *access_token_error; //model
    struct access_token_req_t *access_token_request; //model
    char *nrf_id; // string
    list_t *supported_api_versions; //primitive container
    struct no_profile_match_info_t *no_profile_match_info; //model

} problem_details_t;

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
);

void problem_details_free(problem_details_t *problem_details);

problem_details_t *problem_details_parseFromJSON(cJSON *problem_detailsJSON);

cJSON *problem_details_convertToJSON(problem_details_t *problem_details);

#endif /* _problem_details_H_ */

