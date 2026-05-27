#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "slice_info_for_ue_configuration_update.h"



slice_info_for_ue_configuration_update_t *slice_info_for_ue_configuration_update_create(
    list_t *subscribed_nssai,
    allowed_nssai_t *allowed_nssai_current_access,
    allowed_nssai_t *allowed_nssai_other_access,
    int default_configured_snssai_ind,
    list_t *requested_nssai,
    list_t *mapping_of_nssai,
    int ue_sup_nssrg_ind,
    int suppress_nssrg_ind,
    list_t *rejected_nssai_ra,
    int nsag_supported
    ) {
    slice_info_for_ue_configuration_update_t *slice_info_for_ue_configuration_update_local_var = malloc(sizeof(slice_info_for_ue_configuration_update_t));
    if (!slice_info_for_ue_configuration_update_local_var) {
        return NULL;
    }
    slice_info_for_ue_configuration_update_local_var->subscribed_nssai = subscribed_nssai;
    slice_info_for_ue_configuration_update_local_var->allowed_nssai_current_access = allowed_nssai_current_access;
    slice_info_for_ue_configuration_update_local_var->allowed_nssai_other_access = allowed_nssai_other_access;
    slice_info_for_ue_configuration_update_local_var->default_configured_snssai_ind = default_configured_snssai_ind;
    slice_info_for_ue_configuration_update_local_var->requested_nssai = requested_nssai;
    slice_info_for_ue_configuration_update_local_var->mapping_of_nssai = mapping_of_nssai;
    slice_info_for_ue_configuration_update_local_var->ue_sup_nssrg_ind = ue_sup_nssrg_ind;
    slice_info_for_ue_configuration_update_local_var->suppress_nssrg_ind = suppress_nssrg_ind;
    slice_info_for_ue_configuration_update_local_var->rejected_nssai_ra = rejected_nssai_ra;
    slice_info_for_ue_configuration_update_local_var->nsag_supported = nsag_supported;

    return slice_info_for_ue_configuration_update_local_var;
}


void slice_info_for_ue_configuration_update_free(slice_info_for_ue_configuration_update_t *slice_info_for_ue_configuration_update) {
    if(NULL == slice_info_for_ue_configuration_update){
        return ;
    }
    listEntry_t *listEntry;
    if (slice_info_for_ue_configuration_update->subscribed_nssai) {
        list_ForEach(listEntry, slice_info_for_ue_configuration_update->subscribed_nssai) {
            subscribed_snssai_free(listEntry->data);
        }
        list_freeList(slice_info_for_ue_configuration_update->subscribed_nssai);
        slice_info_for_ue_configuration_update->subscribed_nssai = NULL;
    }
    if (slice_info_for_ue_configuration_update->allowed_nssai_current_access) {
        allowed_nssai_free(slice_info_for_ue_configuration_update->allowed_nssai_current_access);
        slice_info_for_ue_configuration_update->allowed_nssai_current_access = NULL;
    }
    if (slice_info_for_ue_configuration_update->allowed_nssai_other_access) {
        allowed_nssai_free(slice_info_for_ue_configuration_update->allowed_nssai_other_access);
        slice_info_for_ue_configuration_update->allowed_nssai_other_access = NULL;
    }
    if (slice_info_for_ue_configuration_update->requested_nssai) {
        list_ForEach(listEntry, slice_info_for_ue_configuration_update->requested_nssai) {
            snssai_free(listEntry->data);
        }
        list_freeList(slice_info_for_ue_configuration_update->requested_nssai);
        slice_info_for_ue_configuration_update->requested_nssai = NULL;
    }
    if (slice_info_for_ue_configuration_update->mapping_of_nssai) {
        list_ForEach(listEntry, slice_info_for_ue_configuration_update->mapping_of_nssai) {
            mapping_of_snssai_free(listEntry->data);
        }
        list_freeList(slice_info_for_ue_configuration_update->mapping_of_nssai);
        slice_info_for_ue_configuration_update->mapping_of_nssai = NULL;
    }
    if (slice_info_for_ue_configuration_update->rejected_nssai_ra) {
        list_ForEach(listEntry, slice_info_for_ue_configuration_update->rejected_nssai_ra) {
            snssai_free(listEntry->data);
        }
        list_freeList(slice_info_for_ue_configuration_update->rejected_nssai_ra);
        slice_info_for_ue_configuration_update->rejected_nssai_ra = NULL;
    }
    free(slice_info_for_ue_configuration_update);
}

cJSON *slice_info_for_ue_configuration_update_convertToJSON(slice_info_for_ue_configuration_update_t *slice_info_for_ue_configuration_update) {
    cJSON *item = cJSON_CreateObject();

    // slice_info_for_ue_configuration_update->subscribed_nssai
    if(slice_info_for_ue_configuration_update->subscribed_nssai) {
    cJSON *subscribed_nssai = cJSON_AddArrayToObject(item, "subscribedNssai");
    if(subscribed_nssai == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *subscribed_nssaiListEntry;
    if (slice_info_for_ue_configuration_update->subscribed_nssai) {
    list_ForEach(subscribed_nssaiListEntry, slice_info_for_ue_configuration_update->subscribed_nssai) {
    cJSON *itemLocal = subscribed_snssai_convertToJSON(subscribed_nssaiListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(subscribed_nssai, itemLocal);
    }
    }
    }


    // slice_info_for_ue_configuration_update->allowed_nssai_current_access
    if(slice_info_for_ue_configuration_update->allowed_nssai_current_access) {
    cJSON *allowed_nssai_current_access_local_JSON = allowed_nssai_convertToJSON(slice_info_for_ue_configuration_update->allowed_nssai_current_access);
    if(allowed_nssai_current_access_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "allowedNssaiCurrentAccess", allowed_nssai_current_access_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // slice_info_for_ue_configuration_update->allowed_nssai_other_access
    if(slice_info_for_ue_configuration_update->allowed_nssai_other_access) {
    cJSON *allowed_nssai_other_access_local_JSON = allowed_nssai_convertToJSON(slice_info_for_ue_configuration_update->allowed_nssai_other_access);
    if(allowed_nssai_other_access_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "allowedNssaiOtherAccess", allowed_nssai_other_access_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // slice_info_for_ue_configuration_update->default_configured_snssai_ind
    if(slice_info_for_ue_configuration_update->default_configured_snssai_ind) {
    if(cJSON_AddBoolToObject(item, "defaultConfiguredSnssaiInd", slice_info_for_ue_configuration_update->default_configured_snssai_ind) == NULL) {
    goto fail; //Bool
    }
    }


    // slice_info_for_ue_configuration_update->requested_nssai
    if(slice_info_for_ue_configuration_update->requested_nssai) {
    cJSON *requested_nssai = cJSON_AddArrayToObject(item, "requestedNssai");
    if(requested_nssai == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *requested_nssaiListEntry;
    if (slice_info_for_ue_configuration_update->requested_nssai) {
    list_ForEach(requested_nssaiListEntry, slice_info_for_ue_configuration_update->requested_nssai) {
    cJSON *itemLocal = snssai_convertToJSON(requested_nssaiListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(requested_nssai, itemLocal);
    }
    }
    }


    // slice_info_for_ue_configuration_update->mapping_of_nssai
    if(slice_info_for_ue_configuration_update->mapping_of_nssai) {
    cJSON *mapping_of_nssai = cJSON_AddArrayToObject(item, "mappingOfNssai");
    if(mapping_of_nssai == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *mapping_of_nssaiListEntry;
    if (slice_info_for_ue_configuration_update->mapping_of_nssai) {
    list_ForEach(mapping_of_nssaiListEntry, slice_info_for_ue_configuration_update->mapping_of_nssai) {
    cJSON *itemLocal = mapping_of_snssai_convertToJSON(mapping_of_nssaiListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(mapping_of_nssai, itemLocal);
    }
    }
    }


    // slice_info_for_ue_configuration_update->ue_sup_nssrg_ind
    if(slice_info_for_ue_configuration_update->ue_sup_nssrg_ind) {
    if(cJSON_AddBoolToObject(item, "ueSupNssrgInd", slice_info_for_ue_configuration_update->ue_sup_nssrg_ind) == NULL) {
    goto fail; //Bool
    }
    }


    // slice_info_for_ue_configuration_update->suppress_nssrg_ind
    if(slice_info_for_ue_configuration_update->suppress_nssrg_ind) {
    if(cJSON_AddBoolToObject(item, "suppressNssrgInd", slice_info_for_ue_configuration_update->suppress_nssrg_ind) == NULL) {
    goto fail; //Bool
    }
    }


    // slice_info_for_ue_configuration_update->rejected_nssai_ra
    if(slice_info_for_ue_configuration_update->rejected_nssai_ra) {
    cJSON *rejected_nssai_ra = cJSON_AddArrayToObject(item, "rejectedNssaiRa");
    if(rejected_nssai_ra == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *rejected_nssai_raListEntry;
    if (slice_info_for_ue_configuration_update->rejected_nssai_ra) {
    list_ForEach(rejected_nssai_raListEntry, slice_info_for_ue_configuration_update->rejected_nssai_ra) {
    cJSON *itemLocal = snssai_convertToJSON(rejected_nssai_raListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(rejected_nssai_ra, itemLocal);
    }
    }
    }


    // slice_info_for_ue_configuration_update->nsag_supported
    if(slice_info_for_ue_configuration_update->nsag_supported) {
    if(cJSON_AddBoolToObject(item, "nsagSupported", slice_info_for_ue_configuration_update->nsag_supported) == NULL) {
    goto fail; //Bool
    }
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

slice_info_for_ue_configuration_update_t *slice_info_for_ue_configuration_update_parseFromJSON(cJSON *slice_info_for_ue_configuration_updateJSON){

    slice_info_for_ue_configuration_update_t *slice_info_for_ue_configuration_update_local_var = NULL;

    // define the local list for slice_info_for_ue_configuration_update->subscribed_nssai
    list_t *subscribed_nssaiList = NULL;

    // define the local variable for slice_info_for_ue_configuration_update->allowed_nssai_current_access
    allowed_nssai_t *allowed_nssai_current_access_local_nonprim = NULL;

    // define the local variable for slice_info_for_ue_configuration_update->allowed_nssai_other_access
    allowed_nssai_t *allowed_nssai_other_access_local_nonprim = NULL;

    // define the local list for slice_info_for_ue_configuration_update->requested_nssai
    list_t *requested_nssaiList = NULL;

    // define the local list for slice_info_for_ue_configuration_update->mapping_of_nssai
    list_t *mapping_of_nssaiList = NULL;

    // define the local list for slice_info_for_ue_configuration_update->rejected_nssai_ra
    list_t *rejected_nssai_raList = NULL;

    // slice_info_for_ue_configuration_update->subscribed_nssai
    cJSON *subscribed_nssai = cJSON_GetObjectItemCaseSensitive(slice_info_for_ue_configuration_updateJSON, "subscribedNssai");
    if (subscribed_nssai) { 
    cJSON *subscribed_nssai_local_nonprimitive = NULL;
    if(!cJSON_IsArray(subscribed_nssai)){
        goto end; //nonprimitive container
    }

    subscribed_nssaiList = list_createList();

    cJSON_ArrayForEach(subscribed_nssai_local_nonprimitive,subscribed_nssai )
    {
        if(!cJSON_IsObject(subscribed_nssai_local_nonprimitive)){
            goto end;
        }
        subscribed_snssai_t *subscribed_nssaiItem = subscribed_snssai_parseFromJSON(subscribed_nssai_local_nonprimitive);

        list_addElement(subscribed_nssaiList, subscribed_nssaiItem);
    }
    }

    // slice_info_for_ue_configuration_update->allowed_nssai_current_access
    cJSON *allowed_nssai_current_access = cJSON_GetObjectItemCaseSensitive(slice_info_for_ue_configuration_updateJSON, "allowedNssaiCurrentAccess");
    if (allowed_nssai_current_access) { 
    allowed_nssai_current_access_local_nonprim = allowed_nssai_parseFromJSON(allowed_nssai_current_access); //nonprimitive
    }

    // slice_info_for_ue_configuration_update->allowed_nssai_other_access
    cJSON *allowed_nssai_other_access = cJSON_GetObjectItemCaseSensitive(slice_info_for_ue_configuration_updateJSON, "allowedNssaiOtherAccess");
    if (allowed_nssai_other_access) { 
    allowed_nssai_other_access_local_nonprim = allowed_nssai_parseFromJSON(allowed_nssai_other_access); //nonprimitive
    }

    // slice_info_for_ue_configuration_update->default_configured_snssai_ind
    cJSON *default_configured_snssai_ind = cJSON_GetObjectItemCaseSensitive(slice_info_for_ue_configuration_updateJSON, "defaultConfiguredSnssaiInd");
    if (default_configured_snssai_ind) { 
    if(!cJSON_IsBool(default_configured_snssai_ind))
    {
    goto end; //Bool
    }
    }

    // slice_info_for_ue_configuration_update->requested_nssai
    cJSON *requested_nssai = cJSON_GetObjectItemCaseSensitive(slice_info_for_ue_configuration_updateJSON, "requestedNssai");
    if (requested_nssai) { 
    cJSON *requested_nssai_local_nonprimitive = NULL;
    if(!cJSON_IsArray(requested_nssai)){
        goto end; //nonprimitive container
    }

    requested_nssaiList = list_createList();

    cJSON_ArrayForEach(requested_nssai_local_nonprimitive,requested_nssai )
    {
        if(!cJSON_IsObject(requested_nssai_local_nonprimitive)){
            goto end;
        }
        snssai_t *requested_nssaiItem = snssai_parseFromJSON(requested_nssai_local_nonprimitive);

        list_addElement(requested_nssaiList, requested_nssaiItem);
    }
    }

    // slice_info_for_ue_configuration_update->mapping_of_nssai
    cJSON *mapping_of_nssai = cJSON_GetObjectItemCaseSensitive(slice_info_for_ue_configuration_updateJSON, "mappingOfNssai");
    if (mapping_of_nssai) { 
    cJSON *mapping_of_nssai_local_nonprimitive = NULL;
    if(!cJSON_IsArray(mapping_of_nssai)){
        goto end; //nonprimitive container
    }

    mapping_of_nssaiList = list_createList();

    cJSON_ArrayForEach(mapping_of_nssai_local_nonprimitive,mapping_of_nssai )
    {
        if(!cJSON_IsObject(mapping_of_nssai_local_nonprimitive)){
            goto end;
        }
        mapping_of_snssai_t *mapping_of_nssaiItem = mapping_of_snssai_parseFromJSON(mapping_of_nssai_local_nonprimitive);

        list_addElement(mapping_of_nssaiList, mapping_of_nssaiItem);
    }
    }

    // slice_info_for_ue_configuration_update->ue_sup_nssrg_ind
    cJSON *ue_sup_nssrg_ind = cJSON_GetObjectItemCaseSensitive(slice_info_for_ue_configuration_updateJSON, "ueSupNssrgInd");
    if (ue_sup_nssrg_ind) { 
    if(!cJSON_IsBool(ue_sup_nssrg_ind))
    {
    goto end; //Bool
    }
    }

    // slice_info_for_ue_configuration_update->suppress_nssrg_ind
    cJSON *suppress_nssrg_ind = cJSON_GetObjectItemCaseSensitive(slice_info_for_ue_configuration_updateJSON, "suppressNssrgInd");
    if (suppress_nssrg_ind) { 
    if(!cJSON_IsBool(suppress_nssrg_ind))
    {
    goto end; //Bool
    }
    }

    // slice_info_for_ue_configuration_update->rejected_nssai_ra
    cJSON *rejected_nssai_ra = cJSON_GetObjectItemCaseSensitive(slice_info_for_ue_configuration_updateJSON, "rejectedNssaiRa");
    if (rejected_nssai_ra) { 
    cJSON *rejected_nssai_ra_local_nonprimitive = NULL;
    if(!cJSON_IsArray(rejected_nssai_ra)){
        goto end; //nonprimitive container
    }

    rejected_nssai_raList = list_createList();

    cJSON_ArrayForEach(rejected_nssai_ra_local_nonprimitive,rejected_nssai_ra )
    {
        if(!cJSON_IsObject(rejected_nssai_ra_local_nonprimitive)){
            goto end;
        }
        snssai_t *rejected_nssai_raItem = snssai_parseFromJSON(rejected_nssai_ra_local_nonprimitive);

        list_addElement(rejected_nssai_raList, rejected_nssai_raItem);
    }
    }

    // slice_info_for_ue_configuration_update->nsag_supported
    cJSON *nsag_supported = cJSON_GetObjectItemCaseSensitive(slice_info_for_ue_configuration_updateJSON, "nsagSupported");
    if (nsag_supported) { 
    if(!cJSON_IsBool(nsag_supported))
    {
    goto end; //Bool
    }
    }


    slice_info_for_ue_configuration_update_local_var = slice_info_for_ue_configuration_update_create (
        subscribed_nssai ? subscribed_nssaiList : NULL,
        allowed_nssai_current_access ? allowed_nssai_current_access_local_nonprim : NULL,
        allowed_nssai_other_access ? allowed_nssai_other_access_local_nonprim : NULL,
        default_configured_snssai_ind ? default_configured_snssai_ind->valueint : 0,
        requested_nssai ? requested_nssaiList : NULL,
        mapping_of_nssai ? mapping_of_nssaiList : NULL,
        ue_sup_nssrg_ind ? ue_sup_nssrg_ind->valueint : 0,
        suppress_nssrg_ind ? suppress_nssrg_ind->valueint : 0,
        rejected_nssai_ra ? rejected_nssai_raList : NULL,
        nsag_supported ? nsag_supported->valueint : 0
        );

    return slice_info_for_ue_configuration_update_local_var;
end:
    if (subscribed_nssaiList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, subscribed_nssaiList) {
            subscribed_snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(subscribed_nssaiList);
        subscribed_nssaiList = NULL;
    }
    if (allowed_nssai_current_access_local_nonprim) {
        allowed_nssai_free(allowed_nssai_current_access_local_nonprim);
        allowed_nssai_current_access_local_nonprim = NULL;
    }
    if (allowed_nssai_other_access_local_nonprim) {
        allowed_nssai_free(allowed_nssai_other_access_local_nonprim);
        allowed_nssai_other_access_local_nonprim = NULL;
    }
    if (requested_nssaiList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, requested_nssaiList) {
            snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(requested_nssaiList);
        requested_nssaiList = NULL;
    }
    if (mapping_of_nssaiList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, mapping_of_nssaiList) {
            mapping_of_snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(mapping_of_nssaiList);
        mapping_of_nssaiList = NULL;
    }
    if (rejected_nssai_raList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, rejected_nssai_raList) {
            snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(rejected_nssai_raList);
        rejected_nssai_raList = NULL;
    }
    return NULL;

}
