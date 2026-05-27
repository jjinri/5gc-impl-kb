/*
 * tac_range.h
 *
 * Range of TACs (Tracking Area Codes)
 */

#ifndef _tac_range_H_
#define _tac_range_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct tac_range_t tac_range_t;




typedef struct tac_range_t {
    char *start; // string
    char *end; // string
    char *pattern; // string

} tac_range_t;

tac_range_t *tac_range_create(
    char *start,
    char *end,
    char *pattern
);

void tac_range_free(tac_range_t *tac_range);

tac_range_t *tac_range_parseFromJSON(cJSON *tac_rangeJSON);

cJSON *tac_range_convertToJSON(tac_range_t *tac_range);

#endif /* _tac_range_H_ */

