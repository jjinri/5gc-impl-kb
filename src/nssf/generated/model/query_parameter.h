/*
 * query_parameter.h
 *
 * Contains the name and value of a query parameter
 */

#ifndef _query_parameter_H_
#define _query_parameter_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct query_parameter_t query_parameter_t;




typedef struct query_parameter_t {
    char *name; // string
    char *value; // string

} query_parameter_t;

query_parameter_t *query_parameter_create(
    char *name,
    char *value
);

void query_parameter_free(query_parameter_t *query_parameter);

query_parameter_t *query_parameter_parseFromJSON(cJSON *query_parameterJSON);

cJSON *query_parameter_convertToJSON(query_parameter_t *query_parameter);

#endif /* _query_parameter_H_ */

