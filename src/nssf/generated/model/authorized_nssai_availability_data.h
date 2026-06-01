/*
 * authorized_nssai_availability_data.h
 *
 * This contains the Nssai availability data information per TA authorized by the NSSF
 */

#ifndef _authorized_nssai_availability_data_H_
#define _authorized_nssai_availability_data_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct authorized_nssai_availability_data_t authorized_nssai_availability_data_t;

#include "ext_snssai.h"
#include "nsag_info.h"
#include "restricted_snssai.h"
#include "tai.h"
#include "tai_range.h"



typedef struct authorized_nssai_availability_data_t {
    struct tai_t *tai; //model
    list_t *supported_snssai_list; //nonprimitive container
    list_t *restricted_snssai_list; //nonprimitive container
    list_t *tai_list; //nonprimitive container
    list_t *tai_range_list; //nonprimitive container
    list_t *nsag_infos; //nonprimitive container

} authorized_nssai_availability_data_t;

authorized_nssai_availability_data_t *authorized_nssai_availability_data_create(
    tai_t *tai,
    list_t *supported_snssai_list,
    list_t *restricted_snssai_list,
    list_t *tai_list,
    list_t *tai_range_list,
    list_t *nsag_infos
);

void authorized_nssai_availability_data_free(authorized_nssai_availability_data_t *authorized_nssai_availability_data);

authorized_nssai_availability_data_t *authorized_nssai_availability_data_parseFromJSON(cJSON *authorized_nssai_availability_dataJSON);

cJSON *authorized_nssai_availability_data_convertToJSON(authorized_nssai_availability_data_t *authorized_nssai_availability_data);

#endif /* _authorized_nssai_availability_data_H_ */

