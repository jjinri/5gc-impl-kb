/*
 * tai.h
 *
 * Contains the tracking area identity as described in 3GPP 23.003
 */

#ifndef _tai_H_
#define _tai_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct tai_t tai_t;

#include "plmn_id.h"



typedef struct tai_t {
    struct plmn_id_t *plmn_id; //model
    char *tac; // string
    char *nid; // string

} tai_t;

tai_t *tai_create(
    plmn_id_t *plmn_id,
    char *tac,
    char *nid
);

void tai_free(tai_t *tai);

tai_t *tai_parseFromJSON(cJSON *taiJSON);

cJSON *tai_convertToJSON(tai_t *tai);

#endif /* _tai_H_ */

