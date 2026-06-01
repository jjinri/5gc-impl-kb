#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nssf_event_subscription_created_data.h"



nssf_event_subscription_created_data_t *nssf_event_subscription_created_data_create(
    char *subscription_id,
    char *expiry,
    list_t *authorized_nssai_availability_data,
    char *supported_features,
    list_t *accepted_events
    ) {
    nssf_event_subscription_created_data_t *nssf_event_subscription_created_data_local_var = malloc(sizeof(nssf_event_subscription_created_data_t));
    if (!nssf_event_subscription_created_data_local_var) {
        return NULL;
    }
    nssf_event_subscription_created_data_local_var->subscription_id = subscription_id;
    nssf_event_subscription_created_data_local_var->expiry = expiry;
    nssf_event_subscription_created_data_local_var->authorized_nssai_availability_data = authorized_nssai_availability_data;
    nssf_event_subscription_created_data_local_var->supported_features = supported_features;
    nssf_event_subscription_created_data_local_var->accepted_events = accepted_events;

    return nssf_event_subscription_created_data_local_var;
}


void nssf_event_subscription_created_data_free(nssf_event_subscription_created_data_t *nssf_event_subscription_created_data) {
    if(NULL == nssf_event_subscription_created_data){
        return ;
    }
    listEntry_t *listEntry;
    if (nssf_event_subscription_created_data->subscription_id) {
        free(nssf_event_subscription_created_data->subscription_id);
        nssf_event_subscription_created_data->subscription_id = NULL;
    }
    if (nssf_event_subscription_created_data->expiry) {
        free(nssf_event_subscription_created_data->expiry);
        nssf_event_subscription_created_data->expiry = NULL;
    }
    if (nssf_event_subscription_created_data->authorized_nssai_availability_data) {
        list_ForEach(listEntry, nssf_event_subscription_created_data->authorized_nssai_availability_data) {
            authorized_nssai_availability_data_free(listEntry->data);
        }
        list_freeList(nssf_event_subscription_created_data->authorized_nssai_availability_data);
        nssf_event_subscription_created_data->authorized_nssai_availability_data = NULL;
    }
    if (nssf_event_subscription_created_data->supported_features) {
        free(nssf_event_subscription_created_data->supported_features);
        nssf_event_subscription_created_data->supported_features = NULL;
    }
    if (nssf_event_subscription_created_data->accepted_events) {
        list_ForEach(listEntry, nssf_event_subscription_created_data->accepted_events) {
            nssf_event_type_free(listEntry->data);
        }
        list_freeList(nssf_event_subscription_created_data->accepted_events);
        nssf_event_subscription_created_data->accepted_events = NULL;
    }
    free(nssf_event_subscription_created_data);
}

cJSON *nssf_event_subscription_created_data_convertToJSON(nssf_event_subscription_created_data_t *nssf_event_subscription_created_data) {
    cJSON *item = cJSON_CreateObject();

    // nssf_event_subscription_created_data->subscription_id
    if (!nssf_event_subscription_created_data->subscription_id) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "subscriptionId", nssf_event_subscription_created_data->subscription_id) == NULL) {
    goto fail; //String
    }


    // nssf_event_subscription_created_data->expiry
    if(nssf_event_subscription_created_data->expiry) {
    if(cJSON_AddStringToObject(item, "expiry", nssf_event_subscription_created_data->expiry) == NULL) {
    goto fail; //Date-Time
    }
    }


    // nssf_event_subscription_created_data->authorized_nssai_availability_data
    if(nssf_event_subscription_created_data->authorized_nssai_availability_data) {
    cJSON *authorized_nssai_availability_data = cJSON_AddArrayToObject(item, "authorizedNssaiAvailabilityData");
    if(authorized_nssai_availability_data == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *authorized_nssai_availability_dataListEntry;
    if (nssf_event_subscription_created_data->authorized_nssai_availability_data) {
    list_ForEach(authorized_nssai_availability_dataListEntry, nssf_event_subscription_created_data->authorized_nssai_availability_data) {
    cJSON *itemLocal = authorized_nssai_availability_data_convertToJSON(authorized_nssai_availability_dataListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(authorized_nssai_availability_data, itemLocal);
    }
    }
    }


    // nssf_event_subscription_created_data->supported_features
    if(nssf_event_subscription_created_data->supported_features) {
    if(cJSON_AddStringToObject(item, "supportedFeatures", nssf_event_subscription_created_data->supported_features) == NULL) {
    goto fail; //String
    }
    }


    // nssf_event_subscription_created_data->accepted_events
    if(nssf_event_subscription_created_data->accepted_events) {
    cJSON *accepted_events = cJSON_AddArrayToObject(item, "acceptedEvents");
    if(accepted_events == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *accepted_eventsListEntry;
    if (nssf_event_subscription_created_data->accepted_events) {
    list_ForEach(accepted_eventsListEntry, nssf_event_subscription_created_data->accepted_events) {
    cJSON *itemLocal = nssf_event_type_convertToJSON(accepted_eventsListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(accepted_events, itemLocal);
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

nssf_event_subscription_created_data_t *nssf_event_subscription_created_data_parseFromJSON(cJSON *nssf_event_subscription_created_dataJSON){

    nssf_event_subscription_created_data_t *nssf_event_subscription_created_data_local_var = NULL;

    // define the local list for nssf_event_subscription_created_data->authorized_nssai_availability_data
    list_t *authorized_nssai_availability_dataList = NULL;

    // define the local list for nssf_event_subscription_created_data->accepted_events
    list_t *accepted_eventsList = NULL;

    // nssf_event_subscription_created_data->subscription_id
    cJSON *subscription_id = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_created_dataJSON, "subscriptionId");
    if (!subscription_id) {
        goto end;
    }

    
    if(!cJSON_IsString(subscription_id))
    {
    goto end; //String
    }

    // nssf_event_subscription_created_data->expiry
    cJSON *expiry = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_created_dataJSON, "expiry");
    if (expiry) { 
    if(!cJSON_IsString(expiry) && !cJSON_IsNull(expiry))
    {
    goto end; //DateTime
    }
    }

    // nssf_event_subscription_created_data->authorized_nssai_availability_data
    cJSON *authorized_nssai_availability_data = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_created_dataJSON, "authorizedNssaiAvailabilityData");
    if (authorized_nssai_availability_data) { 
    cJSON *authorized_nssai_availability_data_local_nonprimitive = NULL;
    if(!cJSON_IsArray(authorized_nssai_availability_data)){
        goto end; //nonprimitive container
    }

    authorized_nssai_availability_dataList = list_createList();

    cJSON_ArrayForEach(authorized_nssai_availability_data_local_nonprimitive,authorized_nssai_availability_data )
    {
        if(!cJSON_IsObject(authorized_nssai_availability_data_local_nonprimitive)){
            goto end;
        }
        authorized_nssai_availability_data_t *authorized_nssai_availability_dataItem = authorized_nssai_availability_data_parseFromJSON(authorized_nssai_availability_data_local_nonprimitive);

        list_addElement(authorized_nssai_availability_dataList, authorized_nssai_availability_dataItem);
    }
    }

    // nssf_event_subscription_created_data->supported_features
    cJSON *supported_features = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_created_dataJSON, "supportedFeatures");
    if (supported_features) { 
    if(!cJSON_IsString(supported_features) && !cJSON_IsNull(supported_features))
    {
    goto end; //String
    }
    }

    // nssf_event_subscription_created_data->accepted_events
    cJSON *accepted_events = cJSON_GetObjectItemCaseSensitive(nssf_event_subscription_created_dataJSON, "acceptedEvents");
    if (accepted_events) { 
    cJSON *accepted_events_local_nonprimitive = NULL;
    if(!cJSON_IsArray(accepted_events)){
        goto end; //nonprimitive container
    }

    accepted_eventsList = list_createList();

    cJSON_ArrayForEach(accepted_events_local_nonprimitive,accepted_events )
    {
        if(!cJSON_IsObject(accepted_events_local_nonprimitive)){
            goto end;
        }
        nssf_event_type_t *accepted_eventsItem = nssf_event_type_parseFromJSON(accepted_events_local_nonprimitive);

        list_addElement(accepted_eventsList, accepted_eventsItem);
    }
    }


    nssf_event_subscription_created_data_local_var = nssf_event_subscription_created_data_create (
        strdup(subscription_id->valuestring),
        expiry && !cJSON_IsNull(expiry) ? strdup(expiry->valuestring) : NULL,
        authorized_nssai_availability_data ? authorized_nssai_availability_dataList : NULL,
        supported_features && !cJSON_IsNull(supported_features) ? strdup(supported_features->valuestring) : NULL,
        accepted_events ? accepted_eventsList : NULL
        );

    return nssf_event_subscription_created_data_local_var;
end:
    if (authorized_nssai_availability_dataList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, authorized_nssai_availability_dataList) {
            authorized_nssai_availability_data_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(authorized_nssai_availability_dataList);
        authorized_nssai_availability_dataList = NULL;
    }
    if (accepted_eventsList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, accepted_eventsList) {
            nssf_event_type_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(accepted_eventsList);
        accepted_eventsList = NULL;
    }
    return NULL;

}
