/*
 * snssai_extension.h
 *
 * Extensions to the Snssai data type, sdRanges and wildcardSd shall not be present simultaneously 
 */

#ifndef _snssai_extension_H_
#define _snssai_extension_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct snssai_extension_t snssai_extension_t;

#include "sd_range.h"



typedef struct snssai_extension_t {
    list_t *sd_ranges; //nonprimitive container
    int wildcard_sd; //boolean

} snssai_extension_t;

snssai_extension_t *snssai_extension_create(
    list_t *sd_ranges,
    int wildcard_sd
);

void snssai_extension_free(snssai_extension_t *snssai_extension);

snssai_extension_t *snssai_extension_parseFromJSON(cJSON *snssai_extensionJSON);

cJSON *snssai_extension_convertToJSON(snssai_extension_t *snssai_extension);

#endif /* _snssai_extension_H_ */

