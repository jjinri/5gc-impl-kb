/*
 * sd_range.h
 *
 * A range of SDs (Slice Differentiators)
 */

#ifndef _sd_range_H_
#define _sd_range_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct sd_range_t sd_range_t;




typedef struct sd_range_t {
    char *start; // string
    char *end; // string

} sd_range_t;

sd_range_t *sd_range_create(
    char *start,
    char *end
);

void sd_range_free(sd_range_t *sd_range);

sd_range_t *sd_range_parseFromJSON(cJSON *sd_rangeJSON);

cJSON *sd_range_convertToJSON(sd_range_t *sd_range);

#endif /* _sd_range_H_ */

