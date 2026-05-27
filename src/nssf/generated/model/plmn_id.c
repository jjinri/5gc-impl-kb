#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "plmn_id.h"



plmn_id_t *plmn_id_create(
    char *mcc,
    char *mnc
    ) {
    plmn_id_t *plmn_id_local_var = malloc(sizeof(plmn_id_t));
    if (!plmn_id_local_var) {
        return NULL;
    }
    plmn_id_local_var->mcc = mcc;
    plmn_id_local_var->mnc = mnc;

    return plmn_id_local_var;
}


void plmn_id_free(plmn_id_t *plmn_id) {
    if(NULL == plmn_id){
        return ;
    }
    listEntry_t *listEntry;
    if (plmn_id->mcc) {
        free(plmn_id->mcc);
        plmn_id->mcc = NULL;
    }
    if (plmn_id->mnc) {
        free(plmn_id->mnc);
        plmn_id->mnc = NULL;
    }
    free(plmn_id);
}

cJSON *plmn_id_convertToJSON(plmn_id_t *plmn_id) {
    cJSON *item = cJSON_CreateObject();

    // plmn_id->mcc
    if (!plmn_id->mcc) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "mcc", plmn_id->mcc) == NULL) {
    goto fail; //String
    }


    // plmn_id->mnc
    if (!plmn_id->mnc) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "mnc", plmn_id->mnc) == NULL) {
    goto fail; //String
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

plmn_id_t *plmn_id_parseFromJSON(cJSON *plmn_idJSON){

    plmn_id_t *plmn_id_local_var = NULL;

    // plmn_id->mcc
    cJSON *mcc = cJSON_GetObjectItemCaseSensitive(plmn_idJSON, "mcc");
    if (!mcc) {
        goto end;
    }

    
    if(!cJSON_IsString(mcc))
    {
    goto end; //String
    }

    // plmn_id->mnc
    cJSON *mnc = cJSON_GetObjectItemCaseSensitive(plmn_idJSON, "mnc");
    if (!mnc) {
        goto end;
    }

    
    if(!cJSON_IsString(mnc))
    {
    goto end; //String
    }


    plmn_id_local_var = plmn_id_create (
        strdup(mcc->valuestring),
        strdup(mnc->valuestring)
        );

    return plmn_id_local_var;
end:
    return NULL;

}
