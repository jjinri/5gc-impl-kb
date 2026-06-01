/*
 * snssai_status.h
 *
 * Indicates the S-NSSAI availability.
 */

#ifndef _snssai_status_H_
#define _snssai_status_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct snssai_status_t snssai_status_t;




typedef struct snssai_status_t {

} snssai_status_t;

snssai_status_t *snssai_status_create(
);

void snssai_status_free(snssai_status_t *snssai_status);

snssai_status_t *snssai_status_parseFromJSON(cJSON *snssai_statusJSON);

cJSON *snssai_status_convertToJSON(snssai_status_t *snssai_status);

#endif /* _snssai_status_H_ */

