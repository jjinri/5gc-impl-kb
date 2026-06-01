/*
 * nssf_event_notification.h
 *
 * This contains the notification for created event subscription
 */

#ifndef _nssf_event_notification_H_
#define _nssf_event_notification_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct nssf_event_notification_t nssf_event_notification_t;

#include "authorized_nssai_availability_data.h"
#include "object.h"
#include "snssai_replace_info.h"



typedef struct nssf_event_notification_t {
    char *subscription_id; // string
    list_t *authorized_nssai_availability_data; //nonprimitive container
    list_t *alt_nssai; //nonprimitive container
    list_t *unavailable_nsi_list; //primitive container
    list_t* nssai_validity_time_info; //map
    list_t* nssai_validity_time_info_list; //map

} nssf_event_notification_t;

nssf_event_notification_t *nssf_event_notification_create(
    char *subscription_id,
    list_t *authorized_nssai_availability_data,
    list_t *alt_nssai,
    list_t *unavailable_nsi_list,
    list_t* nssai_validity_time_info,
    list_t* nssai_validity_time_info_list
);

void nssf_event_notification_free(nssf_event_notification_t *nssf_event_notification);

nssf_event_notification_t *nssf_event_notification_parseFromJSON(cJSON *nssf_event_notificationJSON);

cJSON *nssf_event_notification_convertToJSON(nssf_event_notification_t *nssf_event_notification);

#endif /* _nssf_event_notification_H_ */

