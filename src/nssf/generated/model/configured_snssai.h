/*
 * configured_snssai.h
 *
 * Contains the configured S-NSSAI authorized by the NSSF in the serving PLMN and optional mapped home S-NSSAI 
 */

#ifndef _configured_snssai_H_
#define _configured_snssai_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct configured_snssai_t configured_snssai_t;

#include "snssai.h"



typedef struct configured_snssai_t {
    struct snssai_t *configured_snssai; //model
    struct snssai_t *mapped_home_snssai; //model

} configured_snssai_t;

configured_snssai_t *configured_snssai_create(
    snssai_t *configured_snssai,
    snssai_t *mapped_home_snssai
);

void configured_snssai_free(configured_snssai_t *configured_snssai);

configured_snssai_t *configured_snssai_parseFromJSON(cJSON *configured_snssaiJSON);

cJSON *configured_snssai_convertToJSON(configured_snssai_t *configured_snssai);

#endif /* _configured_snssai_H_ */

