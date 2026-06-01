/*
 * nssf_event_subscription_created_data.h
 *
 * This contains the information for created event subscription
 */

#ifndef _nssf_event_subscription_created_data_H_
#define _nssf_event_subscription_created_data_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct nssf_event_subscription_created_data_t nssf_event_subscription_created_data_t;

#include "authorized_nssai_availability_data.h"
#include "nssf_event_type.h"



typedef struct nssf_event_subscription_created_data_t {
    char *subscription_id; // string
    char *expiry; //date time
    list_t *authorized_nssai_availability_data; //nonprimitive container
    char *supported_features; // string
    list_t *accepted_events; //nonprimitive container

} nssf_event_subscription_created_data_t;

nssf_event_subscription_created_data_t *nssf_event_subscription_created_data_create(
    char *subscription_id,
    char *expiry,
    list_t *authorized_nssai_availability_data,
    char *supported_features,
    list_t *accepted_events
);

void nssf_event_subscription_created_data_free(nssf_event_subscription_created_data_t *nssf_event_subscription_created_data);

nssf_event_subscription_created_data_t *nssf_event_subscription_created_data_parseFromJSON(cJSON *nssf_event_subscription_created_dataJSON);

cJSON *nssf_event_subscription_created_data_convertToJSON(nssf_event_subscription_created_data_t *nssf_event_subscription_created_data);

#endif /* _nssf_event_subscription_created_data_H_ */

