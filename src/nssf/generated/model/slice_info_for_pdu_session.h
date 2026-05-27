/*
 * slice_info_for_pdu_session.h
 *
 * Contains the slice information requested during PDU Session establishment procedure 
 */

#ifndef _slice_info_for_pdu_session_H_
#define _slice_info_for_pdu_session_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct slice_info_for_pdu_session_t slice_info_for_pdu_session_t;

#include "roaming_indication.h"
#include "snssai.h"



typedef struct slice_info_for_pdu_session_t {
    struct snssai_t *s_nssai; //model
    struct roaming_indication_t *roaming_indication; //model
    struct snssai_t *home_snssai; //model

} slice_info_for_pdu_session_t;

slice_info_for_pdu_session_t *slice_info_for_pdu_session_create(
    snssai_t *s_nssai,
    roaming_indication_t *roaming_indication,
    snssai_t *home_snssai
);

void slice_info_for_pdu_session_free(slice_info_for_pdu_session_t *slice_info_for_pdu_session);

slice_info_for_pdu_session_t *slice_info_for_pdu_session_parseFromJSON(cJSON *slice_info_for_pdu_sessionJSON);

cJSON *slice_info_for_pdu_session_convertToJSON(slice_info_for_pdu_session_t *slice_info_for_pdu_session);

#endif /* _slice_info_for_pdu_session_H_ */

