/*
 * ext_snssai.h
 *
 * The sdRanges and wildcardSd attributes shall be exclusive from each other. If one of these attributes is present,  the sd attribute shall also be present and it shall contain one Slice Differentiator value within the range of SD  (if the sdRanges attribute is present) or with any value (if the wildcardSd attribute is present). 
 */

#ifndef _ext_snssai_H_
#define _ext_snssai_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct ext_snssai_t ext_snssai_t;

#include "sd_range.h"



typedef struct ext_snssai_t {
    int sst; //numeric
    char *sd; // string
    list_t *sd_ranges; //nonprimitive container
    int wildcard_sd; //boolean

} ext_snssai_t;

ext_snssai_t *ext_snssai_create(
    int sst,
    char *sd,
    list_t *sd_ranges,
    int wildcard_sd
);

void ext_snssai_free(ext_snssai_t *ext_snssai);

ext_snssai_t *ext_snssai_parseFromJSON(cJSON *ext_snssaiJSON);

cJSON *ext_snssai_convertToJSON(ext_snssai_t *ext_snssai);

#endif /* _ext_snssai_H_ */

