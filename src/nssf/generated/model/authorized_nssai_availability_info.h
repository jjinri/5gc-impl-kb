/*
 * authorized_nssai_availability_info.h
 *
 * This contains the Nssai availability data information authorized by the NSSF
 */

#ifndef _authorized_nssai_availability_info_H_
#define _authorized_nssai_availability_info_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct authorized_nssai_availability_info_t authorized_nssai_availability_info_t;

#include "authorized_nssai_availability_data.h"



typedef struct authorized_nssai_availability_info_t {
    list_t *authorized_nssai_availability_data; //nonprimitive container
    char *supported_features; // string

} authorized_nssai_availability_info_t;

authorized_nssai_availability_info_t *authorized_nssai_availability_info_create(
    list_t *authorized_nssai_availability_data,
    char *supported_features
);

void authorized_nssai_availability_info_free(authorized_nssai_availability_info_t *authorized_nssai_availability_info);

authorized_nssai_availability_info_t *authorized_nssai_availability_info_parseFromJSON(cJSON *authorized_nssai_availability_infoJSON);

cJSON *authorized_nssai_availability_info_convertToJSON(authorized_nssai_availability_info_t *authorized_nssai_availability_info);

#endif /* _authorized_nssai_availability_info_H_ */

