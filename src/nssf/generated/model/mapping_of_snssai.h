/*
 * mapping_of_snssai.h
 *
 * Contains the mapping of S-NSSAI in the serving network and the value of the home network 
 */

#ifndef _mapping_of_snssai_H_
#define _mapping_of_snssai_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct mapping_of_snssai_t mapping_of_snssai_t;

#include "snssai.h"



typedef struct mapping_of_snssai_t {
    struct snssai_t *serving_snssai; //model
    struct snssai_t *home_snssai; //model

} mapping_of_snssai_t;

mapping_of_snssai_t *mapping_of_snssai_create(
    snssai_t *serving_snssai,
    snssai_t *home_snssai
);

void mapping_of_snssai_free(mapping_of_snssai_t *mapping_of_snssai);

mapping_of_snssai_t *mapping_of_snssai_parseFromJSON(cJSON *mapping_of_snssaiJSON);

cJSON *mapping_of_snssai_convertToJSON(mapping_of_snssai_t *mapping_of_snssai);

#endif /* _mapping_of_snssai_H_ */

