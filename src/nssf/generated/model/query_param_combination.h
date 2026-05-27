/*
 * query_param_combination.h
 *
 * Contains a list of Query Parameters
 */

#ifndef _query_param_combination_H_
#define _query_param_combination_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct query_param_combination_t query_param_combination_t;

#include "query_parameter.h"



typedef struct query_param_combination_t {
    list_t *query_params; //nonprimitive container

} query_param_combination_t;

query_param_combination_t *query_param_combination_create(
    list_t *query_params
);

void query_param_combination_free(query_param_combination_t *query_param_combination);

query_param_combination_t *query_param_combination_parseFromJSON(cJSON *query_param_combinationJSON);

cJSON *query_param_combination_convertToJSON(query_param_combination_t *query_param_combination);

#endif /* _query_param_combination_H_ */

