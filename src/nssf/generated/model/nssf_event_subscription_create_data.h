/*
 * nssf_event_subscription_create_data.h
 *
 * This contains the information for event subscription
 */

#ifndef _nssf_event_subscription_create_data_H_
#define _nssf_event_subscription_create_data_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct nssf_event_subscription_create_data_t nssf_event_subscription_create_data_t;

#include "nsi_unavailability_subscribe_info.h"
#include "nssf_event_type.h"
#include "snssai.h"
#include "snssai_replacement_subscribe_info.h"
#include "tai.h"
#include "tai_range.h"



typedef struct nssf_event_subscription_create_data_t {
    char *nf_nssai_availability_uri; // string
    list_t *tai_list; //nonprimitive container
    struct nssf_event_type_t *event; //model
    list_t *additional_events; //nonprimitive container
    char *expiry; //date time
    char *amf_set_id; // string
    list_t *tai_range_list; //nonprimitive container
    char *amf_id; // string
    char *supported_features; // string
    int all_amf_set_tai_ind; //boolean
    struct snssai_replacement_subscribe_info_t *nsrp_subscribe_info; //model
    struct nsi_unavailability_subscribe_info_t *nsiun_subscribe_info; //model
    list_t *validity_time_sub_list; //nonprimitive container

} nssf_event_subscription_create_data_t;

nssf_event_subscription_create_data_t *nssf_event_subscription_create_data_create(
    char *nf_nssai_availability_uri,
    list_t *tai_list,
    nssf_event_type_t *event,
    list_t *additional_events,
    char *expiry,
    char *amf_set_id,
    list_t *tai_range_list,
    char *amf_id,
    char *supported_features,
    int all_amf_set_tai_ind,
    snssai_replacement_subscribe_info_t *nsrp_subscribe_info,
    nsi_unavailability_subscribe_info_t *nsiun_subscribe_info,
    list_t *validity_time_sub_list
);

void nssf_event_subscription_create_data_free(nssf_event_subscription_create_data_t *nssf_event_subscription_create_data);

nssf_event_subscription_create_data_t *nssf_event_subscription_create_data_parseFromJSON(cJSON *nssf_event_subscription_create_dataJSON);

cJSON *nssf_event_subscription_create_data_convertToJSON(nssf_event_subscription_create_data_t *nssf_event_subscription_create_data);

#endif /* _nssf_event_subscription_create_data_H_ */

