/*
 * nf_type.h
 *
 * NF types known to NRF
 */

#ifndef _nf_type_H_
#define _nf_type_H_

#include <string.h>
#include "../external/cJSON.h"
#include "../include/list.h"
#include "../include/keyValuePair.h"
#include "../include/binary.h"

typedef struct nf_type_t nf_type_t;




typedef struct nf_type_t {

} nf_type_t;

nf_type_t *nf_type_create(
);

void nf_type_free(nf_type_t *nf_type);

nf_type_t *nf_type_parseFromJSON(cJSON *nf_typeJSON);

cJSON *nf_type_convertToJSON(nf_type_t *nf_type);

#endif /* _nf_type_H_ */

