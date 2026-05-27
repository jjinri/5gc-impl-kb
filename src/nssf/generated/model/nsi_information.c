#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nsi_information.h"



nsi_information_t *nsi_information_create(
    char *nrf_id,
    char *nsi_id,
    char *nrf_nf_mgt_uri,
    char *nrf_access_token_uri,
    list_t* nrf_oauth2_required
    ) {
    nsi_information_t *nsi_information_local_var = malloc(sizeof(nsi_information_t));
    if (!nsi_information_local_var) {
        return NULL;
    }
    nsi_information_local_var->nrf_id = nrf_id;
    nsi_information_local_var->nsi_id = nsi_id;
    nsi_information_local_var->nrf_nf_mgt_uri = nrf_nf_mgt_uri;
    nsi_information_local_var->nrf_access_token_uri = nrf_access_token_uri;
    nsi_information_local_var->nrf_oauth2_required = nrf_oauth2_required;

    return nsi_information_local_var;
}


void nsi_information_free(nsi_information_t *nsi_information) {
    if(NULL == nsi_information){
        return ;
    }
    listEntry_t *listEntry;
    if (nsi_information->nrf_id) {
        free(nsi_information->nrf_id);
        nsi_information->nrf_id = NULL;
    }
    if (nsi_information->nsi_id) {
        free(nsi_information->nsi_id);
        nsi_information->nsi_id = NULL;
    }
    if (nsi_information->nrf_nf_mgt_uri) {
        free(nsi_information->nrf_nf_mgt_uri);
        nsi_information->nrf_nf_mgt_uri = NULL;
    }
    if (nsi_information->nrf_access_token_uri) {
        free(nsi_information->nrf_access_token_uri);
        nsi_information->nrf_access_token_uri = NULL;
    }
    if (nsi_information->nrf_oauth2_required) {
        list_ForEach(listEntry, nsi_information->nrf_oauth2_required) {
            keyValuePair_t *localKeyValue = (keyValuePair_t*) listEntry->data;
            free (localKeyValue->key);
            free (localKeyValue->value);
            keyValuePair_free(localKeyValue);
        }
        list_freeList(nsi_information->nrf_oauth2_required);
        nsi_information->nrf_oauth2_required = NULL;
    }
    free(nsi_information);
}

cJSON *nsi_information_convertToJSON(nsi_information_t *nsi_information) {
    cJSON *item = cJSON_CreateObject();

    // nsi_information->nrf_id
    if (!nsi_information->nrf_id) {
        goto fail;
    }
    if(cJSON_AddStringToObject(item, "nrfId", nsi_information->nrf_id) == NULL) {
    goto fail; //String
    }


    // nsi_information->nsi_id
    if(nsi_information->nsi_id) {
    if(cJSON_AddStringToObject(item, "nsiId", nsi_information->nsi_id) == NULL) {
    goto fail; //String
    }
    }


    // nsi_information->nrf_nf_mgt_uri
    if(nsi_information->nrf_nf_mgt_uri) {
    if(cJSON_AddStringToObject(item, "nrfNfMgtUri", nsi_information->nrf_nf_mgt_uri) == NULL) {
    goto fail; //String
    }
    }


    // nsi_information->nrf_access_token_uri
    if(nsi_information->nrf_access_token_uri) {
    if(cJSON_AddStringToObject(item, "nrfAccessTokenUri", nsi_information->nrf_access_token_uri) == NULL) {
    goto fail; //String
    }
    }


    // nsi_information->nrf_oauth2_required
    if(nsi_information->nrf_oauth2_required) {
    cJSON *nrf_oauth2_required = cJSON_AddObjectToObject(item, "nrfOauth2Required");
    if(nrf_oauth2_required == NULL) {
        goto fail; //primitive map container
    }
    cJSON *localMapObject = nrf_oauth2_required;
    listEntry_t *nrf_oauth2_requiredListEntry;
    if (nsi_information->nrf_oauth2_required) {
    list_ForEach(nrf_oauth2_requiredListEntry, nsi_information->nrf_oauth2_required) {
        keyValuePair_t *localKeyValue = (keyValuePair_t*)nrf_oauth2_requiredListEntry->data;
        if(cJSON_AddBoolToObject(localMapObject, localKeyValue->key, *(cJSON_bool *)localKeyValue->value) == NULL)
        {
            goto fail;
        }
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

nsi_information_t *nsi_information_parseFromJSON(cJSON *nsi_informationJSON){

    nsi_information_t *nsi_information_local_var = NULL;

    // define the local map for nsi_information->nrf_oauth2_required
    list_t *nrf_oauth2_requiredList = NULL;

    // nsi_information->nrf_id
    cJSON *nrf_id = cJSON_GetObjectItemCaseSensitive(nsi_informationJSON, "nrfId");
    if (!nrf_id) {
        goto end;
    }

    
    if(!cJSON_IsString(nrf_id))
    {
    goto end; //String
    }

    // nsi_information->nsi_id
    cJSON *nsi_id = cJSON_GetObjectItemCaseSensitive(nsi_informationJSON, "nsiId");
    if (nsi_id) { 
    if(!cJSON_IsString(nsi_id) && !cJSON_IsNull(nsi_id))
    {
    goto end; //String
    }
    }

    // nsi_information->nrf_nf_mgt_uri
    cJSON *nrf_nf_mgt_uri = cJSON_GetObjectItemCaseSensitive(nsi_informationJSON, "nrfNfMgtUri");
    if (nrf_nf_mgt_uri) { 
    if(!cJSON_IsString(nrf_nf_mgt_uri) && !cJSON_IsNull(nrf_nf_mgt_uri))
    {
    goto end; //String
    }
    }

    // nsi_information->nrf_access_token_uri
    cJSON *nrf_access_token_uri = cJSON_GetObjectItemCaseSensitive(nsi_informationJSON, "nrfAccessTokenUri");
    if (nrf_access_token_uri) { 
    if(!cJSON_IsString(nrf_access_token_uri) && !cJSON_IsNull(nrf_access_token_uri))
    {
    goto end; //String
    }
    }

    // nsi_information->nrf_oauth2_required
    cJSON *nrf_oauth2_required = cJSON_GetObjectItemCaseSensitive(nsi_informationJSON, "nrfOauth2Required");
    if (nrf_oauth2_required) { 
    cJSON *nrf_oauth2_required_local_map = NULL;
    if(!cJSON_IsObject(nrf_oauth2_required) && !cJSON_IsNull(nrf_oauth2_required))
    {
        goto end;//primitive map container
    }
    if(cJSON_IsObject(nrf_oauth2_required))
    {
        nrf_oauth2_requiredList = list_createList();
        keyValuePair_t *localMapKeyPair;
        cJSON_ArrayForEach(nrf_oauth2_required_local_map, nrf_oauth2_required)
        {
            cJSON *localMapObject = nrf_oauth2_required_local_map;
            if(!cJSON_IsBool(localMapObject))
            {
                goto end;
            }
            localMapKeyPair = keyValuePair_create(strdup(localMapObject->string), &localMapObject->valueint);
            list_addElement(nrf_oauth2_requiredList , localMapKeyPair);
        }
    }
    }


    nsi_information_local_var = nsi_information_create (
        strdup(nrf_id->valuestring),
        nsi_id && !cJSON_IsNull(nsi_id) ? strdup(nsi_id->valuestring) : NULL,
        nrf_nf_mgt_uri && !cJSON_IsNull(nrf_nf_mgt_uri) ? strdup(nrf_nf_mgt_uri->valuestring) : NULL,
        nrf_access_token_uri && !cJSON_IsNull(nrf_access_token_uri) ? strdup(nrf_access_token_uri->valuestring) : NULL,
        nrf_oauth2_required ? nrf_oauth2_requiredList : NULL
        );

    return nsi_information_local_var;
end:
    if (nrf_oauth2_requiredList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, nrf_oauth2_requiredList) {
            keyValuePair_t *localKeyValue = (keyValuePair_t*) listEntry->data;
            free(localKeyValue->key);
            localKeyValue->key = NULL;
            keyValuePair_free(localKeyValue);
            localKeyValue = NULL;
        }
        list_freeList(nrf_oauth2_requiredList);
        nrf_oauth2_requiredList = NULL;
    }
    return NULL;

}
