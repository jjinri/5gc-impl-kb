#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "restricted_snssai.h"



restricted_snssai_t *restricted_snssai_create(
    plmn_id_t *home_plmn_id,
    list_t *s_nssai_list,
    list_t *home_plmn_id_list,
    int roaming_restriction
    ) {
    restricted_snssai_t *restricted_snssai_local_var = malloc(sizeof(restricted_snssai_t));
    if (!restricted_snssai_local_var) {
        return NULL;
    }
    restricted_snssai_local_var->home_plmn_id = home_plmn_id;
    restricted_snssai_local_var->s_nssai_list = s_nssai_list;
    restricted_snssai_local_var->home_plmn_id_list = home_plmn_id_list;
    restricted_snssai_local_var->roaming_restriction = roaming_restriction;

    return restricted_snssai_local_var;
}


void restricted_snssai_free(restricted_snssai_t *restricted_snssai) {
    if(NULL == restricted_snssai){
        return ;
    }
    listEntry_t *listEntry;
    if (restricted_snssai->home_plmn_id) {
        plmn_id_free(restricted_snssai->home_plmn_id);
        restricted_snssai->home_plmn_id = NULL;
    }
    if (restricted_snssai->s_nssai_list) {
        list_ForEach(listEntry, restricted_snssai->s_nssai_list) {
            ext_snssai_free(listEntry->data);
        }
        list_freeList(restricted_snssai->s_nssai_list);
        restricted_snssai->s_nssai_list = NULL;
    }
    if (restricted_snssai->home_plmn_id_list) {
        list_ForEach(listEntry, restricted_snssai->home_plmn_id_list) {
            plmn_id_free(listEntry->data);
        }
        list_freeList(restricted_snssai->home_plmn_id_list);
        restricted_snssai->home_plmn_id_list = NULL;
    }
    free(restricted_snssai);
}

cJSON *restricted_snssai_convertToJSON(restricted_snssai_t *restricted_snssai) {
    cJSON *item = cJSON_CreateObject();

    // restricted_snssai->home_plmn_id
    if (!restricted_snssai->home_plmn_id) {
        goto fail;
    }
    cJSON *home_plmn_id_local_JSON = plmn_id_convertToJSON(restricted_snssai->home_plmn_id);
    if(home_plmn_id_local_JSON == NULL) {
    goto fail; //model
    }
    cJSON_AddItemToObject(item, "homePlmnId", home_plmn_id_local_JSON);
    if(item->child == NULL) {
    goto fail;
    }


    // restricted_snssai->s_nssai_list
    if (!restricted_snssai->s_nssai_list) {
        goto fail;
    }
    cJSON *s_nssai_list = cJSON_AddArrayToObject(item, "sNssaiList");
    if(s_nssai_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *s_nssai_listListEntry;
    if (restricted_snssai->s_nssai_list) {
    list_ForEach(s_nssai_listListEntry, restricted_snssai->s_nssai_list) {
    cJSON *itemLocal = ext_snssai_convertToJSON(s_nssai_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(s_nssai_list, itemLocal);
    }
    }


    // restricted_snssai->home_plmn_id_list
    if(restricted_snssai->home_plmn_id_list) {
    cJSON *home_plmn_id_list = cJSON_AddArrayToObject(item, "homePlmnIdList");
    if(home_plmn_id_list == NULL) {
    goto fail; //nonprimitive container
    }

    listEntry_t *home_plmn_id_listListEntry;
    if (restricted_snssai->home_plmn_id_list) {
    list_ForEach(home_plmn_id_listListEntry, restricted_snssai->home_plmn_id_list) {
    cJSON *itemLocal = plmn_id_convertToJSON(home_plmn_id_listListEntry->data);
    if(itemLocal == NULL) {
    goto fail;
    }
    cJSON_AddItemToArray(home_plmn_id_list, itemLocal);
    }
    }
    }


    // restricted_snssai->roaming_restriction
    if(restricted_snssai->roaming_restriction) {
    if(cJSON_AddBoolToObject(item, "roamingRestriction", restricted_snssai->roaming_restriction) == NULL) {
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

restricted_snssai_t *restricted_snssai_parseFromJSON(cJSON *restricted_snssaiJSON){

    restricted_snssai_t *restricted_snssai_local_var = NULL;

    // define the local variable for restricted_snssai->home_plmn_id
    plmn_id_t *home_plmn_id_local_nonprim = NULL;

    // define the local list for restricted_snssai->s_nssai_list
    list_t *s_nssai_listList = NULL;

    // define the local list for restricted_snssai->home_plmn_id_list
    list_t *home_plmn_id_listList = NULL;

    // restricted_snssai->home_plmn_id
    cJSON *home_plmn_id = cJSON_GetObjectItemCaseSensitive(restricted_snssaiJSON, "homePlmnId");
    if (!home_plmn_id) {
        goto end;
    }

    
    home_plmn_id_local_nonprim = plmn_id_parseFromJSON(home_plmn_id); //nonprimitive

    // restricted_snssai->s_nssai_list
    cJSON *s_nssai_list = cJSON_GetObjectItemCaseSensitive(restricted_snssaiJSON, "sNssaiList");
    if (!s_nssai_list) {
        goto end;
    }

    
    cJSON *s_nssai_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(s_nssai_list)){
        goto end; //nonprimitive container
    }

    s_nssai_listList = list_createList();

    cJSON_ArrayForEach(s_nssai_list_local_nonprimitive,s_nssai_list )
    {
        if(!cJSON_IsObject(s_nssai_list_local_nonprimitive)){
            goto end;
        }
        ext_snssai_t *s_nssai_listItem = ext_snssai_parseFromJSON(s_nssai_list_local_nonprimitive);

        list_addElement(s_nssai_listList, s_nssai_listItem);
    }

    // restricted_snssai->home_plmn_id_list
    cJSON *home_plmn_id_list = cJSON_GetObjectItemCaseSensitive(restricted_snssaiJSON, "homePlmnIdList");
    if (home_plmn_id_list) { 
    cJSON *home_plmn_id_list_local_nonprimitive = NULL;
    if(!cJSON_IsArray(home_plmn_id_list)){
        goto end; //nonprimitive container
    }

    home_plmn_id_listList = list_createList();

    cJSON_ArrayForEach(home_plmn_id_list_local_nonprimitive,home_plmn_id_list )
    {
        if(!cJSON_IsObject(home_plmn_id_list_local_nonprimitive)){
            goto end;
        }
        plmn_id_t *home_plmn_id_listItem = plmn_id_parseFromJSON(home_plmn_id_list_local_nonprimitive);

        list_addElement(home_plmn_id_listList, home_plmn_id_listItem);
    }
    }

    // restricted_snssai->roaming_restriction
    cJSON *roaming_restriction = cJSON_GetObjectItemCaseSensitive(restricted_snssaiJSON, "roamingRestriction");
    if (roaming_restriction) { 
    if(!cJSON_IsBool(roaming_restriction))
    {
    goto end; //Bool
    }
    }


    restricted_snssai_local_var = restricted_snssai_create (
        home_plmn_id_local_nonprim,
        s_nssai_listList,
        home_plmn_id_list ? home_plmn_id_listList : NULL,
        roaming_restriction ? roaming_restriction->valueint : 0
        );

    return restricted_snssai_local_var;
end:
    if (home_plmn_id_local_nonprim) {
        plmn_id_free(home_plmn_id_local_nonprim);
        home_plmn_id_local_nonprim = NULL;
    }
    if (s_nssai_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, s_nssai_listList) {
            ext_snssai_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(s_nssai_listList);
        s_nssai_listList = NULL;
    }
    if (home_plmn_id_listList) {
        listEntry_t *listEntry = NULL;
        list_ForEach(listEntry, home_plmn_id_listList) {
            plmn_id_free(listEntry->data);
            listEntry->data = NULL;
        }
        list_freeList(home_plmn_id_listList);
        home_plmn_id_listList = NULL;
    }
    return NULL;

}
