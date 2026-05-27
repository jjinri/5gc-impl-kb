/*
 * subscribed_snssai.h
 *
 * Contains the subscribed S-NSSAI
 */

#ifndef _subscribed_snssai_H_
#define _subscribed_snssai_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct subscribed_snssai_t subscribed_snssai_t;

#include "snssai.h"



typedef struct subscribed_snssai_t {
    struct snssai_t *subscribed_snssai; //model
    int default_indication; //boolean
    list_t *subscribed_ns_srg_list; //primitive container

} subscribed_snssai_t;

subscribed_snssai_t *subscribed_snssai_create(
    snssai_t *subscribed_snssai,
    int default_indication,
    list_t *subscribed_ns_srg_list
);

void subscribed_snssai_free(subscribed_snssai_t *subscribed_snssai);

subscribed_snssai_t *subscribed_snssai_parseFromJSON(cJSON *subscribed_snssaiJSON);

cJSON *subscribed_snssai_convertToJSON(subscribed_snssai_t *subscribed_snssai);

#endif /* _subscribed_snssai_H_ */

