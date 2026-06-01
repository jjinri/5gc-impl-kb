/*
 * availability_repository.h — AvailabilityRepository interface (WI-availability-repo).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten persistence module.
 *
 * Abstract repository over the `availability` table (infra/nssf/schema.sql,
 * schema_version=1). Two backends share one vtable behind an opaque handle —
 * a libpq/PostgreSQL backend (production) and an in-memory backend (test seam,
 * unit/module-integration only). AvailabilityEngine (later slice) and
 * NSSelectionGet consume this interface; column truth = schema.sql.
 *
 * Design intent — design/nssf/architecture/state-persistence.md (repository
 * interface abstraction + PG impl + in-memory test seam) and
 * design/nssf/module-decomposition/AvailabilityEngine.md.
 */
#ifndef NSSF_AVAILABILITY_REPOSITORY_H
#define NSSF_AVAILABILITY_REPOSITORY_H

#include <stdbool.h>
#include <stddef.h>

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * One row of the `availability` table.
 *
 * Ownership — the record OWNS its cJSON trees (plmn_id/tai/snssais) and the
 * amf_set_id heap string. nssf_availability_record_clear() frees them. A record
 * the caller fills for upsert() is the caller's to clear; a record returned in
 * a result list is owned by that list and freed by
 * nssf_availability_record_list_free(). `id` is a fixed 36-char UUID text + NUL;
 * amf_set_id is NULL when the column is SQL NULL.
 */
typedef struct {
    char id[37];
    cJSON *plmn_id;
    cJSON *tai;
    cJSON *snssais;
    char *amf_set_id;
    bool reallocation_indication;
} nssf_availability_record_t;

/* Owned result set returned by find_for_plmn_tai(). */
typedef struct {
    nssf_availability_record_t *records;
    size_t count;
} nssf_availability_record_list_t;

/* Opaque handle — backend (libpq or in-memory) hidden behind the vtable. */
typedef struct nssf_availability_repo nssf_availability_repo_t;

/*
 * Construct a libpq-backed repository. `conninfo` is a libpq connection string
 * sourced from operator config (never hardcoded). Returns NULL on connect or
 * setup failure and writes a NUL-terminated diagnostic into errbuf (when
 * errbuf != NULL && errlen > 0).
 */
nssf_availability_repo_t *nssf_availability_repo_new_pg(const char *conninfo,
                                                        char *errbuf,
                                                        size_t errlen);

/* Construct the in-memory test-seam repository. Returns NULL only on OOM. */
nssf_availability_repo_t *nssf_availability_repo_new_inmemory(void);

/* Tear down a repository of either backend (idempotent on NULL). */
void nssf_availability_repo_free(nssf_availability_repo_t *repo);

/*
 * Read rows whose plmn_id and tai match the given JSON values exactly. On
 * success returns 0 and fills *out (caller frees with
 * nssf_availability_record_list_free); on error returns -1 and *out is zeroed.
 * `plmn_id` and `tai` are borrowed (not retained).
 */
int nssf_availability_repo_find_for_plmn_tai(nssf_availability_repo_t *repo,
                                             const cJSON *plmn_id,
                                             const cJSON *tai,
                                             nssf_availability_record_list_t *out);

/*
 * INSERT ... ON CONFLICT (id) DO UPDATE inside a transaction (BEGIN/COMMIT,
 * ROLLBACK on error). `rec` is borrowed — the repository deep-copies what it
 * keeps. Returns 0 on commit, -1 on error.
 */
int nssf_availability_repo_upsert(nssf_availability_repo_t *repo,
                                  const nssf_availability_record_t *rec);

/*
 * Partial update by id. Any of plmn_id/tai/snssais/amf_set_id may be NULL to
 * leave that column unchanged; set_reallocation toggles whether
 * reallocation_indication is written. Borrowed args. Returns 0 when a row was
 * updated, 1 when no row matched id, -1 on error.
 */
int nssf_availability_repo_patch(nssf_availability_repo_t *repo,
                                 const char *id,
                                 const cJSON *plmn_id,
                                 const cJSON *tai,
                                 const cJSON *snssais,
                                 const char *amf_set_id,
                                 bool set_reallocation,
                                 bool reallocation_indication);

/*
 * Delete by id. Returns 0 when a row was deleted, 1 when no row matched id,
 * -1 on error.
 */
int nssf_availability_repo_delete(nssf_availability_repo_t *repo, const char *id);

/* Free the cJSON trees + amf_set_id of a record and zero it. */
void nssf_availability_record_clear(nssf_availability_record_t *rec);

/* Free a result list and all records it owns, then zero the list. */
void nssf_availability_record_list_free(nssf_availability_record_list_t *list);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_AVAILABILITY_REPOSITORY_H */
