/*
 * ml_model_inter_ind.h
 *
 * ML Model Interoperability Indicator per Analytics Id
 */

#ifndef _ml_model_inter_ind_H_
#define _ml_model_inter_ind_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct ml_model_inter_ind_t ml_model_inter_ind_t;

#include "object.h"



typedef struct ml_model_inter_ind_t {
    object_t *analytics_id; // custom
    list_t *vendor_list; //nonprimitive container

} ml_model_inter_ind_t;

ml_model_inter_ind_t *ml_model_inter_ind_create(
    object_t *analytics_id,
    list_t *vendor_list
);

void ml_model_inter_ind_free(ml_model_inter_ind_t *ml_model_inter_ind);

ml_model_inter_ind_t *ml_model_inter_ind_parseFromJSON(cJSON *ml_model_inter_indJSON);

cJSON *ml_model_inter_ind_convertToJSON(ml_model_inter_ind_t *ml_model_inter_ind);

#endif /* _ml_model_inter_ind_H_ */

