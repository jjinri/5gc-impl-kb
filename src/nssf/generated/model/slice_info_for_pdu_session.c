#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "slice_info_for_pdu_session.h"



slice_info_for_pdu_session_t *slice_info_for_pdu_session_create(
    snssai_t *s_nssai,
    roaming_indication_t *roaming_indication,
    snssai_t *home_snssai
    ) {
    slice_info_for_pdu_session_t *slice_info_for_pdu_session_local_var = malloc(sizeof(slice_info_for_pdu_session_t));
    if (!slice_info_for_pdu_session_local_var) {
        return NULL;
    }
    slice_info_for_pdu_session_local_var->s_nssai = s_nssai;
    slice_info_for_pdu_session_local_var->roaming_indication = roaming_indication;
    slice_info_for_pdu_session_local_var->home_snssai = home_snssai;

    return slice_info_for_pdu_session_local_var;
}


void slice_info_for_pdu_session_free(slice_info_for_pdu_session_t *slice_info_for_pdu_session) {
    if(NULL == slice_info_for_pdu_session){
        return ;
    }
    listEntry_t *listEntry;
    if (slice_info_for_pdu_session->s_nssai) {
        snssai_free(slice_info_for_pdu_session->s_nssai);
        slice_info_for_pdu_session->s_nssai = NULL;
    }
    if (slice_info_for_pdu_session->roaming_indication) {
        roaming_indication_free(slice_info_for_pdu_session->roaming_indication);
        slice_info_for_pdu_session->roaming_indication = NULL;
    }
    if (slice_info_for_pdu_session->home_snssai) {
        snssai_free(slice_info_for_pdu_session->home_snssai);
        slice_info_for_pdu_session->home_snssai = NULL;
    }
    free(slice_info_for_pdu_session);
}

cJSON *slice_info_for_pdu_session_convertToJSON(slice_info_for_pdu_session_t *slice_info_for_pdu_session) {
    cJSON *item = cJSON_CreateObject();

    // slice_info_for_pdu_session->s_nssai
    if (!slice_info_for_pdu_session->s_nssai) {
        goto fail;
    }
    cJSON *s_nssai_local_JSON = snssai_convertToJSON(slice_info_for_pdu_session->s_nssai);
    if(s_nssai_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "sNssai", s_nssai_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }


    // slice_info_for_pdu_session->roaming_indication
    if (!slice_info_for_pdu_session->roaming_indication) {
        goto fail;
    }
    cJSON *roaming_indication_local_JSON = roaming_indication_convertToJSON(slice_info_for_pdu_session->roaming_indication);
    if(roaming_indication_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "roamingIndication", roaming_indication_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }


    // slice_info_for_pdu_session->home_snssai
    if(slice_info_for_pdu_session->home_snssai) {
    cJSON *home_snssai_local_JSON = snssai_convertToJSON(slice_info_for_pdu_session->home_snssai);
    if(home_snssai_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "homeSnssai", home_snssai_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

slice_info_for_pdu_session_t *slice_info_for_pdu_session_parseFromJSON(cJSON *slice_info_for_pdu_sessionJSON){

    slice_info_for_pdu_session_t *slice_info_for_pdu_session_local_var = NULL;

    // define the local variable for slice_info_for_pdu_session->s_nssai
    snssai_t *s_nssai_local_nonprim = NULL;

    // define the local variable for slice_info_for_pdu_session->roaming_indication
    roaming_indication_t *roaming_indication_local_nonprim = NULL;

    // define the local variable for slice_info_for_pdu_session->home_snssai
    snssai_t *home_snssai_local_nonprim = NULL;

    // slice_info_for_pdu_session->s_nssai
    cJSON *s_nssai = cJSON_GetObjectItemCaseSensitive(slice_info_for_pdu_sessionJSON, "sNssai");
    if (!s_nssai) {
        goto end;
    }

    
    s_nssai_local_nonprim = snssai_parseFromJSON(s_nssai); //nonprimitive

    // slice_info_for_pdu_session->roaming_indication
    cJSON *roaming_indication = cJSON_GetObjectItemCaseSensitive(slice_info_for_pdu_sessionJSON, "roamingIndication");
    if (!roaming_indication) {
        goto end;
    }

    
    roaming_indication_local_nonprim = roaming_indication_parseFromJSON(roaming_indication); //nonprimitive

    // slice_info_for_pdu_session->home_snssai
    cJSON *home_snssai = cJSON_GetObjectItemCaseSensitive(slice_info_for_pdu_sessionJSON, "homeSnssai");
    if (home_snssai) { 
    home_snssai_local_nonprim = snssai_parseFromJSON(home_snssai); //nonprimitive
    }


    slice_info_for_pdu_session_local_var = slice_info_for_pdu_session_create (
        s_nssai_local_nonprim,
        roaming_indication_local_nonprim,
        home_snssai ? home_snssai_local_nonprim : NULL
        );

    return slice_info_for_pdu_session_local_var;
end:
    if (s_nssai_local_nonprim) {
        snssai_free(s_nssai_local_nonprim);
        s_nssai_local_nonprim = NULL;
    }
    if (roaming_indication_local_nonprim) {
        roaming_indication_free(roaming_indication_local_nonprim);
        roaming_indication_local_nonprim = NULL;
    }
    if (home_snssai_local_nonprim) {
        snssai_free(home_snssai_local_nonprim);
        home_snssai_local_nonprim = NULL;
    }
    return NULL;

}
