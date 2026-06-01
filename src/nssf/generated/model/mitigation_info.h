/*
 * mitigation_info.h
 *
 * include the congestion mitigation information.
 */

#ifndef _mitigation_info_H_
#define _mitigation_info_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct mitigation_info_t mitigation_info_t;




typedef struct mitigation_info_t {
    int perc_value_num_ues; //numeric
    int new_ues_ind; //boolean

} mitigation_info_t;

mitigation_info_t *mitigation_info_create(
    int perc_value_num_ues,
    int new_ues_ind
);

void mitigation_info_free(mitigation_info_t *mitigation_info);

mitigation_info_t *mitigation_info_parseFromJSON(cJSON *mitigation_infoJSON);

cJSON *mitigation_info_convertToJSON(mitigation_info_t *mitigation_info);

#endif /* _mitigation_info_H_ */

