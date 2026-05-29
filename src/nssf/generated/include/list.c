/*
 * list.c — generated/include/list.h 의 runtime 구현.
 *
 * PR #92 (WI-codegen-bootstrap) 가 list.h 만 emit 하고 .c 구현은 누락
 * (openapi-generator C template 의 supportingFiles 미활성). PR #92 의 첫
 * 실사용자 (problem_details_t 의 invalid_params / supported_api_versions
 * 등) 가 wave 1 wrapper PR 인 본 PR 이라 본 PR scope 에서 보충한다.
 *
 * 향후 regenerate.sh / openapi-generator config 가 list.c 를 정상 emit
 * 하도록 갱신되면 본 파일은 codegen 산출과 교체된다 (Pane 2 권고 — generated
 * code 직접 수정 금지의 spirit 유지: 본 파일은 *generator 가 emit 해야 할*
 * 산출의 임시 보충일 뿐).
 */

#define _POSIX_C_SOURCE 200809L

#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

list_t *list_createList(void)
{
    list_t *list = (list_t *)malloc(sizeof(list_t));
    if (list == NULL) {
        return NULL;
    }
    list->firstEntry = NULL;
    list->lastEntry = NULL;
    list->count = 0;
    return list;
}

static listEntry_t *make_entry(void *data)
{
    listEntry_t *entry = (listEntry_t *)malloc(sizeof(listEntry_t));
    if (entry == NULL) {
        return NULL;
    }
    entry->nextListEntry = NULL;
    entry->prevListEntry = NULL;
    entry->data = data;
    return entry;
}

void list_addElement(list_t *list, void *dataToAddInList)
{
    if (list == NULL) {
        return;
    }
    listEntry_t *entry = make_entry(dataToAddInList);
    if (entry == NULL) {
        return;
    }
    if (list->lastEntry == NULL) {
        list->firstEntry = entry;
        list->lastEntry = entry;
    } else {
        entry->prevListEntry = list->lastEntry;
        list->lastEntry->nextListEntry = entry;
        list->lastEntry = entry;
    }
    list->count++;
}

listEntry_t *list_getElementAt(list_t *list, long indexOfElement)
{
    if (list == NULL || indexOfElement < 0 || indexOfElement >= list->count) {
        return NULL;
    }
    listEntry_t *current = list->firstEntry;
    for (long i = 0; i < indexOfElement && current != NULL; ++i) {
        current = current->nextListEntry;
    }
    return current;
}

listEntry_t *list_getWithIndex(list_t *list, int index)
{
    return list_getElementAt(list, (long)index);
}

void list_removeElement(list_t *list, listEntry_t *elementToRemove)
{
    if (list == NULL || elementToRemove == NULL) {
        return;
    }
    if (elementToRemove->prevListEntry != NULL) {
        elementToRemove->prevListEntry->nextListEntry =
            elementToRemove->nextListEntry;
    } else {
        list->firstEntry = elementToRemove->nextListEntry;
    }
    if (elementToRemove->nextListEntry != NULL) {
        elementToRemove->nextListEntry->prevListEntry =
            elementToRemove->prevListEntry;
    } else {
        list->lastEntry = elementToRemove->prevListEntry;
    }
    free(elementToRemove);
    if (list->count > 0) {
        list->count--;
    }
}

void list_freeList(list_t *listToFree)
{
    if (listToFree == NULL) {
        return;
    }
    listEntry_t *current = listToFree->firstEntry;
    while (current != NULL) {
        listEntry_t *next = current->nextListEntry;
        free(current);
        current = next;
    }
    free(listToFree);
}

void list_iterateThroughListForward(
    list_t *list,
    void (*operationToPerform)(listEntry_t *, void *),
    void *additionalDataNeededForCallbackFunction)
{
    if (list == NULL || operationToPerform == NULL) {
        return;
    }
    listEntry_t *current = list->firstEntry;
    while (current != NULL) {
        operationToPerform(current, additionalDataNeededForCallbackFunction);
        current = current->nextListEntry;
    }
}

void list_iterateThroughListBackward(
    list_t *list,
    void (*operationToPerform)(listEntry_t *, void *),
    void *additionalDataNeededForCallbackFunction)
{
    if (list == NULL || operationToPerform == NULL) {
        return;
    }
    listEntry_t *current = list->lastEntry;
    while (current != NULL) {
        operationToPerform(current, additionalDataNeededForCallbackFunction);
        current = current->prevListEntry;
    }
}

void listEntry_printAsInt(listEntry_t *listEntry, void *additionalData)
{
    (void)additionalData;
    if (listEntry == NULL || listEntry->data == NULL) {
        return;
    }
    printf("%d", *(int *)listEntry->data);
}

void listEntry_free(listEntry_t *listEntry, void *additionalData)
{
    (void)additionalData;
    if (listEntry == NULL) {
        return;
    }
    free(listEntry->data);
    listEntry->data = NULL;
}

char *findStrInStrList(list_t *strList, const char *str)
{
    if (strList == NULL || str == NULL) {
        return NULL;
    }
    listEntry_t *current = strList->firstEntry;
    while (current != NULL) {
        if (current->data != NULL &&
            strcmp((char *)current->data, str) == 0) {
            return (char *)current->data;
        }
        current = current->nextListEntry;
    }
    return NULL;
}

void clear_and_free_string_list(list_t *list)
{
    if (list == NULL) {
        return;
    }
    listEntry_t *current = list->firstEntry;
    while (current != NULL) {
        listEntry_t *next = current->nextListEntry;
        free(current->data);
        free(current);
        current = next;
    }
    free(list);
}
