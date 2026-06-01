/*
 * termination_indication.h
 *
 * Indicates the termination of Network Slice Replacement.
 */

#ifndef _termination_indication_H_
#define _termination_indication_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct termination_indication_t termination_indication_t;




typedef struct termination_indication_t {

} termination_indication_t;

termination_indication_t *termination_indication_create(
);

void termination_indication_free(termination_indication_t *termination_indication);

termination_indication_t *termination_indication_parseFromJSON(cJSON *termination_indicationJSON);

cJSON *termination_indication_convertToJSON(termination_indication_t *termination_indication);

#endif /* _termination_indication_H_ */

