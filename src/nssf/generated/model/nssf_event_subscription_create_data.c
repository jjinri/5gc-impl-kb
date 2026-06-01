#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nssf_event_subscription_create_data.h"



nssf_event_subscription_create_data_t *nssf_event_subscription_create_data_create(
    char *nf_nssai_availability_uri,
    list_t *tai_list,
    nssf_event_type_t *event,
    list_t *additional_events,
    char *expiry,
    char *amf_set_id,
    list_t *tai_range_list,
    char *amf_id,
    char *supported_features,
    int all_amf_set_tai_ind,
    snssai_replacement_subscribe_info_t *nsrp_subscribe_info,
    nsi_unavailability_subscribe_info_t *nsiun_subscribe_info,
    list_t *validity_time_sub_list
    ) {
    nssf_event_subscription_create_data_t *nssf_event_subscription_create_data_local_var = malloc(sizeof(nssf_event_subscription_create_data_t));
    if (!nssf_event_subscription_create_data_local_var) {
        return NULL;
    }
    nssf_event_subscription_create_data_local_var->nf_nssai_availability_uri = nf_nssai_availability_uri;
    nssf_event_subscription_create_data_local_var->tai_list = tai_list;
    nssf_event_subscription_create_data_local_var->event = event;
    nssf_event_subscription_create_data_local_var->additional_events = additional_events;
    nssf_event_subscription_create_data_local_var->expiry = expiry;
    nssf_event_subscription_create_data_local_var->amf_set_id = amf_set_id;
    nssf_event_subscription_create_data_local_var->tai_range_list = tai_range_list;
    nssf_event_subscription_create_data_local_var->amf_id = amf_id;
    nssf_event_subscription_create_data_local_var->supported_features = supported_features;
    nssf_event_subscription_create_data_local_var->all_amf_set_tai_ind = all_amf_set_tai_ind;
    nssf_event_subscription_create_data_local_var->nsrp_subscribe_info = nsrp_subscribe_info;
    nssf_event_subscription_create_data_local_var->nsiun_subscribe_info = nsiun_subscribe_info;
    nssf_event_subscription_create_data_local_var->validity_time_sub_list = validity_time_sub_list;

    return nssf_event_subscription_create_data_local_var;
}


void nssf_event_subscription_create_data_free(nssf_event_subscription_create_data_t *nssf_event_subscription_create_data) {
    if(NULL == nssf_event_subscription_create_data){
        return ;
    }
    listEntry_t *listEntry;
    if (nssf_event_subscription_create_data->nf_nssai_availability_uri) {
        free(nssf_event_subscription_create_data->nf_nssai_availability_uri);
        nssf_event_subscription_create_data->nf_nssai_availability_uri = NULL;
    }
    if (nssf_event_subscription_create_data->tai_list) {
        list_ForEach(listEntry, nssf_event_subscription_create_data->tai_list) {
            tai_free(listEntry->data);
        }
        list_freeList(nssf_event_subscription_create_data->tai_list);
        nssf_event_subscription_create_data->tai_list = NULL;
    }
    if (nssf_event_subscription_create_data->event) {
        nssf_event_type_free(nssf_event_subscription_create_data->event);
        nssf_event_subscription_create_data->event = NULL;
    }
    if (nssf_event_subscription_create_data->additional_events) {
        list_ForEach(listEntry, nssf_event_subscription_create_data->additional_events) {
            nssf_event_type_free(listEntry->data);
        }
        list_freeList(nssf_event_subscription_create_data->additional_events);
        nssf_event_subscription_create_data->additional_events = NULL;
    }
    if (nssf_event_subscription_create_data->expiry) {
        free(nssf_event_subscription_create_data->expiry);
        nssf_event_subscription_create_data->expiry = NULL;
    }
    if (nssf_event_subscription_create_data->amf_set_id) {
        free(nssf_event_subscription_create_data->amf_set_id);
        nssf_event_subscription_create_data->amf_set_id = NULL;
    }
    if (nssf_event_subscription_create_data->tai_range_list) {
        list_ForEach(listEntry, nssf_event_subscription_create_data->tai_range_list) {
            tai_range_free(listEntry->data);
        }
        list_freeList(nssf_event_subscription_create_data->tai_range_list);
        nssf_event_subscription_create_data->tai_range_list = NULL;
    }
    if (nssf_event_subscription_create_data->amf_id) {
        free(nssf_event_subscription_create_data->amf_id);
        nssf_event_subscription_create_data->amf_id = NULL;
    }
    if (nssf_event_subscription_create_data->supported_features) {
        free(nssf_event_subscription_create_data->supported_features);
        nssf_event_subscription_create_data->supported_features = NULL;
    }
    if (nssf_event_subscription_create_data->nsrp_subscribe_info) {
        snssai_replacement_subscribe_info_free(nssf_event_subscription_create_data->nsrp_subscribe_info);
        nssf_event_subscription_create_data->nsrp_subscribe_info = NULL;
    }
    if (nssf_event_subscription_create_data->nsiun_subscribe_info) {
        nsi_unavailability_subscribe_info_free(nssf_event_subscription_create_data->nsiun_subscribe_info);
        nssf_event_subscription_create_data->nsiun_subscribe_info = NULL;
    }
    if (nssf_event_subscription_create_data->validity_time_sub_list) {
        list_ForEach(listEntry, nssf_event_subscription_create_data->validity_time_sub_list) {
            snssai_free(listEntry->data);
        }
        list_freeList(nssf_event_subscription_create_data->validity_time_sub_list);
        nssf_event_subscription_create_data->validity_time_sub_list = NULL;
    }
    free(nssf_event_subscription_create_data);
}

cJSON *nssf_event_subscription_create_data_convertToJSON(nssf_event_subscription_create_data_t *nssf_event_subscription_create_data) {
    cJSON *item = cJSON_CreateObject();

    // nssf_event_subscription_create_data->nf_nssai_availability_uri
    if (!nssf_event_subscription_create_data->nf_nssai_availability_uri) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "nfNssaiAvailabilityUri", nssf_event_subscription_create_data->nf_nssai_availability_uri) == NULL) {
    goto fail; //String
    }


    // nssf_event_subscription_create_data->tai_list
    if(nssf_event_subscription_create_data->tai_list) {
    cJSON *tai_list = cJSON_AddArrayToObject(item, "taiList");
    if(tai_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *tai_listListEntry;
    if (nssf_event_subscription_create_data->tai_list) {
    list_ForEach(tai_listListEntry, nssf_event_subscription_create_data->tai_list) {
    cJSON *itemLocal = tai_convertToJSON(tai_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(tai_list, itemLocal);
    }
    }
    }


    // nssf_event_subscription_create_data->event
    if (!nssf_event_subscription_create_data->event) {
        goto fail;
    }
    cJSON *event_local_JSON = nssf_event_type_convertToJSON(nssf_event_subscription_create_data->event);
    if(event_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "event", event_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }


    // nssf_event_subscription_create_data->additional_events
    if(nssf_event_subscription_create_data->additional_events) {
    cJSON *additional_events = cJSON_AddArrayToObject(item, "additionalEvents");
    if(additional_events == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *additional_eventsListEntry;
    if (nssf_event_subscription_create_data->additional_events) {
    list_ForEach(additional_eventsListEntry, nssf_event_subscription_create_data->additional_events) {
    cJSON *itemLocal = nssf_event_type_convertToJSON(additional_eventsListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(additional_events, itemLocal);
    }
    }
    }


    // nssf_event_subscription_create_data->expiry
    if(nssf_event_subscription_create_data->expiry) {
    if(cJSON_AddStringToObject(item, "expiry", nssf_event_subscription_create_data->expiry) == NULL) {
    goto fail; //Date-Time
    }
    }


    // nssf_event_subscription_create_data->amf_set_id
    if(nssf_event_subscription_create_data->amf_set_id) {
    if(cJSON_AddStringToObject(item, "amfSetId", nssf_event_subscription_create_data->amf_set_id) == NULL) {
    goto fail; //String
    }
    }


    // nssf_event_subscription_create_data->tai_range_list
    if(nssf_event_subscription_create_data->tai_range_list) {
    cJSON *tai_range_list = cJSON_AddArrayToObject(item, "taiRangeList");
    if(tai_range_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *tai_range_listListEntry;
    if (nssf_event_subscription_create_data->tai_range_list) {
    list_ForEach(tai_range_listListEntry, nssf_event_subscription_create_data->tai_range_list) {
    cJSON *itemLocal = tai_range_convertToJSON(tai_range_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(tai_range_list, itemLocal);
    }
    }
    }


    // nssf_event_subscription_create_data->amf_id
    if(nssf_event_subscription_create_data->amf_id) {
    if(cJSON_AddStringToObject(item, "amfId", nssf_event_subscription_create_data->amf_id) == NULL) {
    goto fail; //String
    }
    }


    // nssf_event_subscription_create_data->supported_features
    if(nssf_event_subscription_create_data->supported_features) {
    if(cJSON_AddStringToObject(item, "supportedFeatures", nssf_event_subscription_create_data->supported_features) == NULL) {
    goto fail; //String
    }
    }


    // nssf_event_subscription_create_data->all_amf_set_tai_ind
    if(nssf_event_subscription_create_data->all_amf_set_tai_ind) {
    if(cJSON_AddBoolToObject(item, "allAmfSetTaiInd", nssf_event_subscription_create_data->all_amf_set_tai_ind) == NULL) {
    goto fail; //Bool
    }
    }


    // nssf_event_subscription_create_data->nsrp_subscribe_info
    if(nssf_event_subscription_create_data->nsrp_subscribe_info) {
    cJSON *nsrp_subscribe_info_local_JSON = snssai_replacement_subscribe_info_convertToJSON(nssf_event_subscription_create_data->nsrp_subscribe_info);
    if(nsrp_subscribe_info_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "nsrpSubscribeInfo", nsrp_subscribe_info_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // nssf_event_subscription_create_data->nsiun_subscribe_info
    if(nssf_event_subscription_create_data->nsiun_subscribe_info) {
    cJSON *nsiun_subscribe_info_local_JSON = nsi_unavailability_subscribe_info_convertToJSON(nssf_event_subscription_create_data->nsiun_subscribe_info);
    if(nsiun_subscribe_info_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "nsiunSubscribeInfo", nsiun_subscribe_info_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }
    }


    // nssf_event_subscription_create_data->validity_time_sub_list
    if(nssf_event_subscription_create_data->validity_time_sub_list) {
    cJSON *validity_time_sub_list = cJSON_AddArrayToObject(item, "validityTimeSubList");
    if(validity_time_sub_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *validity_time_sub_listListEntry;
    if (nssf_event_subscription_create_data->validity_time_sub_list) {
    list_ForEach(validity_time_sub_listListEntry, nssf_event_subscription_create_data->validity_time_sub_list) {
    cJSON *itemLocal = snssai_convertToJSON(validity_time_sub_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(validity_time_sub_list, itemLocal);
    }
    }
    }

    return item;
fail:
    if (item) {
        cJSON_Delete(item);
    }
    return NULL;
}

nssf_event_subscription_create_data_t *nssf_event_subscription_create_data_parseFromJSON(cJSON *nssf_event_subscription_create_dataJSON){

    nssf_event_subscription_create_data_t *nssf_event_subscription_create_data_local_var = NULL;

    // define the local list for nssf_event_subscription_create_data->tai_list
    list_t *tai_listList = NULL;

    // define the local variable for nssf_event_subscription_create_data->event
    nssf_event_type_t *event_local_nonprim = NULL;

    // define the local list for nssf_event_subscription_create_data->additional_events
    list_t *additional_eventsList = NULL;

    // define the local list for nssf_event_subscription_create_data->tai_range_list
    list_t *tai_range_listList = NULL;

    // define the local variable for nssf_event_subscription_create_data->nsrp_subscribe_info
    snssai_replacement_subscribe_info_t *nsrp_subscribe_info_local_nonprim = NULL;

    // define the local variable for nssf_event_subscription_create_data->nsiun_subscribe_info
    nsi_unavailability_subscribe_info_t *nsiun_subscribe_info_local_nonprim = NULL;

    // define the local list for nssf_event_subscription_create_data->validity_time_sub_list
    list_t *validity_time_sub_listList = NULL;

    // nssf_event_subscription_create_data->nf_nssai_availability_uri
    cJSON *nf_nssai_availability_uri = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "nfNssaiAvailabilityUri");
    if (!nf_nssai_availability_uri) {
        goto end;
    }

    
    if(!cJSON_IsString(nf_nssai_availability_uri))
    {
    goto end; //String
    }

    // nssf_event_subscription_create_data->tai_list
    cJSON *tai_list = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "taiList");
    if (tai_list) { 
    cJSON *tai_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(tai_list)){
        goto end; //nonprimitive container
    }

    tai_listList = list_createList();

    cJSON_ArrayForEach(tai_list_local_nonprimitive,tai_list )
    {
        if(!cJSON_IsObject(tai_list_local_nonprimitive)){
            goto end;
        }
        tai_t *tai_listItem = tai_parseFromJSON(tai_list_local_nonprimitive);

        list_addElement(tai_listList, tai_listItem);
    }
    }

    // nssf_event_subscription_create_data->event
    cJSON *event = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "event");
    if (!event) {
        goto end;
    }

    
    event_local_nonprim = nssf_event_type_parseFromJSON(event); //nonprimitive

    // nssf_event_subscription_create_data->additional_events
    cJSON *additional_events = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "additionalEvents");
    if (additional_events) { 
    cJSON *additional_events_local_nonprimitive = NULL;
    if(!cJSON_IsArray(additional_events)){
        goto end; //nonprimitive container
    }

    additional_eventsList = list_createList();

    cJSON_ArrayForEach(additional_events_local_nonprimitive,additional_events )
    {
        if(!cJSON_IsObject(additional_events_local_nonprimitive)){
            goto end;
        }
        nssf_event_type_t *additional_eventsItem = nssf_event_type_parseFromJSON(additional_events_local_nonprimitive);

        list_addElement(additional_eventsList, additional_eventsItem);
    }
    }

    // nssf_event_subscription_create_data->expiry
    cJSON *expiry = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "expiry");
    if (expiry) { 
    if(!cJSON_IsString(expiry) && !cJSON_IsNull(expiry))
    {
    goto end; //DateTime
    }
    }

    // nssf_event_subscription_create_data->amf_set_id
    cJSON *amf_set_id = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "amfSetId");
    if (amf_set_id) { 
    if(!cJSON_IsString(amf_set_id) && !cJSON_IsNull(amf_set_id))
    {
    goto end; //String
    }
    }

    // nssf_event_subscription_create_data->tai_range_list
    cJSON *tai_range_list = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "taiRangeList");
    if (tai_range_list) { 
    cJSON *tai_range_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(tai_range_list)){
        goto end; //nonprimitive container
    }

    tai_range_listList = list_createList();

    cJSON_ArrayForEach(tai_range_list_local_nonprimitive,tai_range_list )
    {
        if(!cJSON_IsObject(tai_range_list_local_nonprimitive)){
            goto end;
        }
        tai_range_t *tai_range_listItem = tai_range_parseFromJSON(tai_range_list_local_nonprimitive);

        list_addElement(tai_range_listList, tai_range_listItem);
    }
    }

    // nssf_event_subscription_create_data->amf_id
    cJSON *amf_id = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "amfId");
    if (amf_id) { 
    if(!cJSON_IsString(amf_id) && !cJSON_IsNull(amf_id))
    {
    goto end; //String
    }
    }

    // nssf_event_subscription_create_data->supported_features
    cJSON *supported_features = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "supportedFeatures");
    if (supported_features) { 
    if(!cJSON_IsString(supported_features) && !cJSON_IsNull(supported_features))
    {
    goto end; //String
    }
    }

    // nssf_event_subscription_create_data->all_amf_set_tai_ind
    cJSON *all_amf_set_tai_ind = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "allAmfSetTaiInd");
    if (all_amf_set_tai_ind) { 
    if(!cJSON_IsBool(all_amf_set_tai_ind))
    {
    goto end; //Bool
    }
    }

    // nssf_event_subscription_create_data->nsrp_subscribe_info
    cJSON *nsrp_subscribe_info = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "nsrpSubscribeInfo");
    if (nsrp_subscribe_info) { 
    nsrp_subscribe_info_local_nonprim = snssai_replacement_subscribe_info_parseFromJSON(nsrp_subscribe_info); //nonprimitive
    }

    // nssf_event_subscription_create_data->nsiun_subscribe_info
    cJSON *nsiun_subscribe_info = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "nsiunSubscribeInfo");
    if (nsiun_subscribe_info) { 
    nsiun_subscribe_info_local_nonprim = nsi_unavailability_subscribe_info_parseFromJSON(nsiun_subscribe_info); //nonprimitive
    }

    // nssf_event_subscription_create_data->validity_time_sub_list
    cJSON *validity_time_sub_list = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_create_dataJSON, "validityTimeSubList");
    if (validity_time_sub_list) { 
    cJSON *validity_time_sub_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(validity_time_sub_list)){
        goto end; //nonprimitive container
    }

    validity_time_sub_listList = list_createList();

    cJSON_ArrayForEach(validity_time_sub_list_local_nonprimitive,validity_time_sub_list )
    {
        if(!cJSON_IsObject(validity_time_sub_list_local_nonprimitive)){
            goto end;
        }
        snssai_t *validity_time_sub_listItem = snssai_parseFromJSON(validity_time_sub_list_local_nonprimitive);

        list_addElement(validity_time_sub_listList, validity_time_sub_listItem);
    }
    }


    nssf_event_subscription_create_data_local_var = nssf_event_subscription_create_data_create (
        strdup(nf_nssai_availability_uri->valuestring),
        tai_list ? tai_listList : NULL,
        event_local_nonprim,
        additional_events ? additional_eventsList : NULL,
        expiry && !cJSON_IsNull(expiry) ? strdup(expiry->valuestring) : NULL,
        amf_set_id && !cJSON_IsNull(amf_set_id) ? strdup(amf_set_id->valuestring) : NULL,
        tai_range_list ? tai_range_listList : NULL,
        amf_id && !cJSON_IsNull(amf_id) ? strdup(amf_id->valuestring) : NULL,
        supported_features && !cJSON_IsNull(supported_features) ? strdup(supported_features->valuestring) : NULL,
        all_amf_set_tai_ind ? all_amf_set_tai_ind->valueint : 0,
        nsrp_subscribe_info ? nsrp_subscribe_info_local_nonprim : NULL,
        nsiun_subscribe_info ? nsiun_subscribe_info_local_nonprim : NULL,
        validity_time_sub_list ? validity_time_sub_listList : NULL
        );

    return nssf_event_subscription_create_data_local_var;
end:
    if (tai_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, tai_listList) {
            tai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(tai_listList);
        tai_listList = NULL;
    }
    if (event_local_nonprim) {
        nssf_event_type_free(event_local_nonprim);
        event_local_nonprim = NULL;
    }
    if (additional_eventsList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, additional_eventsList) {
            nssf_event_type_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(additional_eventsList);
        additional_eventsList = NULL;
    }
    if (tai_range_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, tai_range_listList) {
            tai_range_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(tai_range_listList);
        tai_range_listList = NULL;
    }
    if (nsrp_subscribe_info_local_nonprim) {
        snssai_replacement_subscribe_info_free(nsrp_subscribe_info_local_nonprim);
        nsrp_subscribe_info_local_nonprim = NULL;
    }
    if (nsiun_subscribe_info_local_nonprim) {
        nsi_unavailability_subscribe_info_free(nsiun_subscribe_info_local_nonprim);
        nsiun_subscribe_info_local_nonprim = NULL;
    }
    if (validity_time_sub_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, validity_time_sub_listList) {
            snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(validity_time_sub_listList);
        validity_time_sub_listList = NULL;
    }
    return NULL;

}
