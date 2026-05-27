#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "access_type.h"


char* access_type_access_type_ToString(nssf_ns_selection_access_type__e access_type) {
    char *access_typeArray[] =  { "NULL", "3GPP_ACCESS", "NON_3GPP_ACCESS" };
    return access_typeArray[access_type];
}

nssf_ns_selection_access_type__e access_type_access_type_FromString(char* access_type) {
    int stringToReturn = 0;
    char *access_typeArray[] =  { "NULL", "3GPP_ACCESS", "NON_3GPP_ACCESS" };
    size_t sizeofArray = sizeof(access_typeArray) / sizeof(access_typeArray[0]);
    while(stringToReturn < sizeofArray) {
        if(strcmp(access_type, access_typeArray[stringToReturn]) == 0) {
            return stringToReturn;
        }
        stringToReturn++;
    }
    return 0;
}

cJSON *access_type_access_type_convertToJSON(nssf_ns_selection_access_type__e access_type) {
    cJSON *item = cJSON_CreateObject();
    if(cJSON_AddStringToObject(item, "access_type", access_type_access_type_ToString(access_type)) == NULL) {
        goto fail;
    }
    return item;
fail:
    cJSON_Delete(item);
    return NULL;
}

nssf_ns_selection_access_type__e access_type_access_type_parseFromJSON(cJSON *access_typeJSON) {
    nssf_ns_selection_access_type__e *access_type = NULL;
    nssf_ns_selection_access_type__e access_typeVariable;
    cJSON *access_typeVar = cJSON_GetObjectItemCaseSensitive(access_typeJSON, "access_type");
    if(!cJSON_IsString(access_typeVar) || (access_typeVar->valuestring == NULL)){
        goto end;
    }
    access_typeVariable = access_type_access_type_FromString(access_typeVar->valuestring);
    return access_typeVariable;
end:
    return 0;
}
