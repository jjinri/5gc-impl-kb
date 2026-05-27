/*
 * roaming_indication.h
 *
 * Contains the indication on roaming
 */

#ifndef _roaming_indication_H_
#define _roaming_indication_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct roaming_indication_t roaming_indication_t;




typedef struct roaming_indication_t {

} roaming_indication_t;

roaming_indication_t *roaming_indication_create(
);

void roaming_indication_free(roaming_indication_t *roaming_indication);

roaming_indication_t *roaming_indication_parseFromJSON(cJSON *roaming_indicationJSON);

cJSON *roaming_indication_convertToJSON(roaming_indication_t *roaming_indication);

#endif /* _roaming_indication_H_ */

