#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mitigation_info.h"



mitigation_info_t *mitigation_info_create(
    int perc_value_num_ues,
    int new_ues_ind
    ) {
    mitigation_info_t *mitigation_info_local_var = malloc(sizeof(mitigation_info_t));
    if (!mitigation_info_local_var) {
        return NULL;
    }
    mitigation_info_local_var->perc_value_num_ues = perc_value_num_ues;
    mitigation_info_local_var->new_ues_ind = new_ues_ind;

    return mitigation_info_local_var;
}


void mitigation_info_free(mitigation_info_t *mitigation_info) {
    if(NULL == mitigation_info){
        return ;
    }
    listEntry_t *listEntry;
    free(mitigation_info);
}

cJSON *mitigation_info_convertToJSON(mitigation_info_t *mitigation_info) {
    cJSON *item = cJSON_CreateObject();

    // mitigation_info->perc_value_num_ues
    if(mitigation_info->perc_value_num_ues) {
    if(cJSON_AddNumberToObject(item, "percValueNumUes", mitigation_info->perc_value_num_ues) == NULL) {
    goto fail; //Numeric
    }
    }


    // mitigation_info->new_ues_ind
    if(mitigation_info->new_ues_ind) {
    if(cJSON_AddBoolToObject(item, "newUesInd", mitigation_info->new_ues_ind) == NULL) {
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

mitigation_info_t *mitigation_info_parseFromJSON(cJSON *mitigation_infoJSON){

    mitigation_info_t *mitigation_info_local_var = NULL;

    // mitigation_info->perc_value_num_ues
    cJSON *perc_value_num_ues = cJSON_GetObjectItemCaseSensitive(mitigation_infoJSON, "percValueNumUes");
    if (perc_value_num_ues) { 
    if(!cJSON_IsNumber(perc_value_num_ues))
    {
    goto end; //Numeric
    }
    }

    // mitigation_info->new_ues_ind
    cJSON *new_ues_ind = cJSON_GetObjectItemCaseSensitive(mitigation_infoJSON, "newUesInd");
    if (new_ues_ind) { 
    if(!cJSON_IsBool(new_ues_ind))
    {
    goto end; //Bool
    }
    }


    mitigation_info_local_var = mitigation_info_create (
        perc_value_num_ues ? perc_value_num_ues->valuedouble : 0,
        new_ues_ind ? new_ues_ind->valueint : 0
        );

    return mitigation_info_local_var;
end:
    return NULL;

}
