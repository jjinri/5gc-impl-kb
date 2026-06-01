/*
 * nssf_event_type.h
 *
 * This contains the event for the subscription
 */

#ifndef _nssf_event_type_H_
#define _nssf_event_type_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct nssf_event_type_t nssf_event_type_t;




typedef struct nssf_event_type_t {

} nssf_event_type_t;

nssf_event_type_t *nssf_event_type_create(
);

void nssf_event_type_free(nssf_event_type_t *nssf_event_type);

nssf_event_type_t *nssf_event_type_parseFromJSON(cJSON *nssf_event_typeJSON);

cJSON *nssf_event_type_convertToJSON(nssf_event_type_t *nssf_event_type);

#endif /* _nssf_event_type_H_ */

