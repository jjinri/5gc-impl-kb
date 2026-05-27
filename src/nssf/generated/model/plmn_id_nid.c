#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "plmn_id_nid.h"



plmn_id_nid_t *plmn_id_nid_create(
    object_t *mcc,
    object_t *mnc,
    object_t *nid
    ) {
    plmn_id_nid_t *plmn_id_nid_local_var = malloc(sizeof(plmn_id_nid_t));
    if (!plmn_id_nid_local_var) {
        return NULL;
    }
    plmn_id_nid_local_var->mcc = mcc;
    plmn_id_nid_local_var->mnc = mnc;
    plmn_id_nid_local_var->nid = nid;

    return plmn_id_nid_local_var;
}


void plmn_id_nid_free(plmn_id_nid_t *plmn_id_nid) {
    if(NULL == plmn_id_nid){
        return ;
    }
    listEntry_t *listEntry;
    if (plmn_id_nid->mcc) {
        object_free(plmn_id_nid->mcc);
        plmn_id_nid->mcc = NULL;
    }
    if (plmn_id_nid->mnc) {
        object_free(plmn_id_nid->mnc);
        plmn_id_nid->mnc = NULL;
    }
    if (plmn_id_nid->nid) {
        object_free(plmn_id_nid->nid);
        plmn_id_nid->nid = NULL;
    }
    free(plmn_id_nid);
}

cJSON *plmn_id_nid_convertToJSON(plmn_id_nid_t *plmn_id_nid) {
    cJSON *item = cJSON_CreateObject();

    // plmn_id_nid->mcc
    if (!plmn_id_nid->mcc) {
        goto fail;
    }
    cJSON *mcc_local_JSON = object_convertToJSON(plmn_id_nid->mcc);
    if(mcc_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "mcc", mcc_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }


    // plmn_id_nid->mnc
    if (!plmn_id_nid->mnc) {
        goto fail;
    }
    cJSON *mnc_local_JSON = object_convertToJSON(plmn_id_nid->mnc);
    if(mnc_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "mnc", mnc_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }


    // plmn_id_nid->nid
    if(plmn_id_nid->nid) {
    cJSON *nid_local_JSON = object_convertToJSON(plmn_id_nid->nid);
    if(nid_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "nid", nid_local_JSON);
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

plmn_id_nid_t *plmn_id_nid_parseFromJSON(cJSON *plmn_id_nidJSON){

    plmn_id_nid_t *plmn_id_nid_local_var = NULL;

    // define the local variable for plmn_id_nid->mcc
    object_t *mcc_local_nonprim = NULL;

    // define the local variable for plmn_id_nid->mnc
    object_t *mnc_local_nonprim = NULL;

    // define the local variable for plmn_id_nid->nid
    object_t *nid_local_nonprim = NULL;

    // plmn_id_nid->mcc
    cJSON *mcc = cJSON_GetObjectItemCaseSensitive(plmn_id_nidJSON, "mcc");
    if (!mcc) {
        goto end;
    }

    
    mcc_local_nonprim = object_parseFromJSON(mcc); //custom

    // plmn_id_nid->mnc
    cJSON *mnc = cJSON_GetObjectItemCaseSensitive(plmn_id_nidJSON, "mnc");
    if (!mnc) {
        goto end;
    }

    
    mnc_local_nonprim = object_parseFromJSON(mnc); //custom

    // plmn_id_nid->nid
    cJSON *nid = cJSON_GetObjectItemCaseSensitive(plmn_id_nidJSON, "nid");
    if (nid) { 
    nid_local_nonprim = object_parseFromJSON(nid); //custom
    }


    plmn_id_nid_local_var = plmn_id_nid_create (
        mcc_local_nonprim,
        mnc_local_nonprim,
        nid ? nid_local_nonprim : NULL
        );

    return plmn_id_nid_local_var;
end:
    if (mcc_local_nonprim) {
        object_free(mcc_local_nonprim);
        mcc_local_nonprim = NULL;
    }
    if (mnc_local_nonprim) {
        object_free(mnc_local_nonprim);
        mnc_local_nonprim = NULL;
    }
    if (nid_local_nonprim) {
        object_free(nid_local_nonprim);
        nid_local_nonprim = NULL;
    }
    return NULL;

}
