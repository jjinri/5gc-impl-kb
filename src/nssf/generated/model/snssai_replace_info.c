#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "snssai_replace_info.h"



snssai_replace_info_t *snssai_replace_info_create(
    object_t *snssai,
    object_t *status,
    object_t *alt_snssai,
    object_t *ns_repl_termin_ind,
    object_t *plmn_id,
    object_t *mitigation_info
    ) {
    snssai_replace_info_t *snssai_replace_info_local_var = malloc(sizeof(snssai_replace_info_t));
    if (!snssai_replace_info_local_var) {
        return NULL;
    }
    snssai_replace_info_local_var->snssai = snssai;
    snssai_replace_info_local_var->status = status;
    snssai_replace_info_local_var->alt_snssai = alt_snssai;
    snssai_replace_info_local_var->ns_repl_termin_ind = ns_repl_termin_ind;
    snssai_replace_info_local_var->plmn_id = plmn_id;
    snssai_replace_info_local_var->mitigation_info = mitigation_info;

    return snssai_replace_info_local_var;
}


void snssai_replace_info_free(snssai_replace_info_t *snssai_replace_info) {
    if(NULL == snssai_replace_info){
        return ;
    }
    listEntry_t *listEntry;
    if (snssai_replace_info->snssai) {
        object_free(snssai_replace_info->snssai);
        snssai_replace_info->snssai = NULL;
    }
    if (snssai_replace_info->status) {
        object_free(snssai_replace_info->status);
        snssai_replace_info->status = NULL;
    }
    if (snssai_replace_info->alt_snssai) {
        object_free(snssai_replace_info->alt_snssai);
        snssai_replace_info->alt_snssai = NULL;
    }
    if (snssai_replace_info->ns_repl_termin_ind) {
        object_free(snssai_replace_info->ns_repl_termin_ind);
        snssai_replace_info->ns_repl_termin_ind = NULL;
    }
    if (snssai_replace_info->plmn_id) {
        object_free(snssai_replace_info->plmn_id);
        snssai_replace_info->plmn_id = NULL;
    }
    if (snssai_replace_info->mitigation_info) {
        object_free(snssai_replace_info->mitigation_info);
        snssai_replace_info->mitigation_info = NULL;
    }
    free(snssai_replace_info);
}

cJSON *snssai_replace_info_convertToJSON(snssai_replace_info_t *snssai_replace_info) {
    cJSON *item = cJSON_CreateObject();

    // snssai_replace_info->snssai
    if (!snssai_replace_info->snssai) {
        goto fail;
    }
    cJSON *snssai_local_JSON = object_convertToJSON(snssai_replace_info->snssai);
    if(snssai_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "snssai", snssai_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }


    // snssai_replace_info->status
    if(snssai_replace_info->status) {
    cJSON *status_local_JSON = object_convertToJSON(snssai_replace_info->status);
    if(status_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "status", status_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }
    }


    // snssai_replace_info->alt_snssai
    if(snssai_replace_info->alt_snssai) {
    cJSON *alt_snssai_local_JSON = object_convertToJSON(snssai_replace_info->alt_snssai);
    if(alt_snssai_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "altSnssai", alt_snssai_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }
    }


    // snssai_replace_info->ns_repl_termin_ind
    if(snssai_replace_info->ns_repl_termin_ind) {
    cJSON *ns_repl_termin_ind_local_JSON = object_convertToJSON(snssai_replace_info->ns_repl_termin_ind);
    if(ns_repl_termin_ind_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "nsReplTerminInd", ns_repl_termin_ind_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }
    }


    // snssai_replace_info->plmn_id
    if(snssai_replace_info->plmn_id) {
    cJSON *plmn_id_local_JSON = object_convertToJSON(snssai_replace_info->plmn_id);
    if(plmn_id_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "plmnId", plmn_id_local_JSON);
    if(item->child == NULL) {
        goto fail;
    }
    }


    // snssai_replace_info->mitigation_info
    if(snssai_replace_info->mitigation_info) {
    cJSON *mitigation_info_local_JSON = object_convertToJSON(snssai_replace_info->mitigation_info);
    if(mitigation_info_local_JSON == NULL) {
        goto fail; // custom
    }
    cJSON_AddItemToObject(item, "mitigationInfo", mitigation_info_local_JSON);
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

snssai_replace_info_t *snssai_replace_info_parseFromJSON(cJSON *snssai_replace_infoJSON){

    snssai_replace_info_t *snssai_replace_info_local_var = NULL;

    // define the local variable for snssai_replace_info->snssai
    object_t *snssai_local_nonprim = NULL;

    // define the local variable for snssai_replace_info->status
    object_t *status_local_nonprim = NULL;

    // define the local variable for snssai_replace_info->alt_snssai
    object_t *alt_snssai_local_nonprim = NULL;

    // define the local variable for snssai_replace_info->ns_repl_termin_ind
    object_t *ns_repl_termin_ind_local_nonprim = NULL;

    // define the local variable for snssai_replace_info->plmn_id
    object_t *plmn_id_local_nonprim = NULL;

    // define the local variable for snssai_replace_info->mitigation_info
    object_t *mitigation_info_local_nonprim = NULL;

    // snssai_replace_info->snssai
    cJSON *snssai = cJSON_GetObjectItemCaseSensitive(snssai_replace_infoJSON, "snssai");
    if (!snssai) {
        goto end;
    }

    
    snssai_local_nonprim = object_parseFromJSON(snssai); //custom

    // snssai_replace_info->status
    cJSON *status = cJSON_GetObjectItemCaseSensitive(snssai_replace_infoJSON, "status");
    if (status) { 
    status_local_nonprim = object_parseFromJSON(status); //custom
    }

    // snssai_replace_info->alt_snssai
    cJSON *alt_snssai = cJSON_GetObjectItemCaseSensitive(snssai_replace_infoJSON, "altSnssai");
    if (alt_snssai) { 
    alt_snssai_local_nonprim = object_parseFromJSON(alt_snssai); //custom
    }

    // snssai_replace_info->ns_repl_termin_ind
    cJSON *ns_repl_termin_ind = cJSON_GetObjectItemCaseSensitive(snssai_replace_infoJSON, "nsReplTerminInd");
    if (ns_repl_termin_ind) { 
    ns_repl_termin_ind_local_nonprim = object_parseFromJSON(ns_repl_termin_ind); //custom
    }

    // snssai_replace_info->plmn_id
    cJSON *plmn_id = cJSON_GetObjectItemCaseSensitive(snssai_replace_infoJSON, "plmnId");
    if (plmn_id) { 
    plmn_id_local_nonprim = object_parseFromJSON(plmn_id); //custom
    }

    // snssai_replace_info->mitigation_info
    cJSON *mitigation_info = cJSON_GetObjectItemCaseSensitive(snssai_replace_infoJSON, "mitigationInfo");
    if (mitigation_info) { 
    mitigation_info_local_nonprim = object_parseFromJSON(mitigation_info); //custom
    }


    snssai_replace_info_local_var = snssai_replace_info_create (
        snssai_local_nonprim,
        status ? status_local_nonprim : NULL,
        alt_snssai ? alt_snssai_local_nonprim : NULL,
        ns_repl_termin_ind ? ns_repl_termin_ind_local_nonprim : NULL,
        plmn_id ? plmn_id_local_nonprim : NULL,
        mitigation_info ? mitigation_info_local_nonprim : NULL
        );

    return snssai_replace_info_local_var;
end:
    if (snssai_local_nonprim) {
        object_free(snssai_local_nonprim);
        snssai_local_nonprim = NULL;
    }
    if (status_local_nonprim) {
        object_free(status_local_nonprim);
        status_local_nonprim = NULL;
    }
    if (alt_snssai_local_nonprim) {
        object_free(alt_snssai_local_nonprim);
        alt_snssai_local_nonprim = NULL;
    }
    if (ns_repl_termin_ind_local_nonprim) {
        object_free(ns_repl_termin_ind_local_nonprim);
        ns_repl_termin_ind_local_nonprim = NULL;
    }
    if (plmn_id_local_nonprim) {
        object_free(plmn_id_local_nonprim);
        plmn_id_local_nonprim = NULL;
    }
    if (mitigation_info_local_nonprim) {
        object_free(mitigation_info_local_nonprim);
        mitigation_info_local_nonprim = NULL;
    }
    return NULL;

}
