/*
 * availability_repository.c — libpq + in-memory backends for the `availability`
 * table (WI-availability-repo).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten persistence module.
 *
 * SQL safety — every value reaches the server through PQexecParams positional
 * parameters ($1,$2,...); JSONB columns are bound as text and cast `$N::jsonb`
 * server-side. No value is ever concatenated into a SQL string.
 */
#include "availability_repository.h"

#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

/* ── vtable ───────────────────────────────────────────────────────────── */

typedef struct {
    int (*find_for_plmn_tai)(nssf_availability_repo_t *,
                             const cJSON *, const cJSON *,
                             nssf_availability_record_list_t *);
    int (*upsert)(nssf_availability_repo_t *,
                  const nssf_availability_record_t *);
    int (*patch)(nssf_availability_repo_t *, const char *,
                 const cJSON *, const cJSON *, const cJSON *,
                 const char *, bool, bool);
    int (*del)(nssf_availability_repo_t *, const char *);
    void (*destroy)(nssf_availability_repo_t *);
} nssf_availability_vtable_t;

struct nssf_availability_repo {
    const nssf_availability_vtable_t *vt;
};

/* ── shared record helpers ────────────────────────────────────────────── */

void nssf_availability_record_clear(nssf_availability_record_t *rec)
{
    if (rec == NULL) {
        return;
    }
    cJSON_Delete(rec->plmn_id);
    cJSON_Delete(rec->tai);
    cJSON_Delete(rec->snssais);
    free(rec->amf_set_id);
    memset(rec, 0, sizeof(*rec));
}

void nssf_availability_record_list_free(nssf_availability_record_list_t *list)
{
    if (list == NULL) {
        return;
    }
    for (size_t i = 0; i < list->count; ++i) {
        nssf_availability_record_clear(&list->records[i]);
    }
    free(list->records);
    list->records = NULL;
    list->count = 0;
}

/*
 * Deep-copy src into dst (dst must be zeroed). Returns 0 on success; on failure
 * dst is cleared and -1 is returned. amf_set_id NULL stays NULL.
 */
static int record_copy(nssf_availability_record_t *dst,
                       const nssf_availability_record_t *src)
{
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->id, src->id, sizeof(dst->id));
    dst->id[sizeof(dst->id) - 1] = '\0';
    dst->reallocation_indication = src->reallocation_indication;

    if (src->plmn_id != NULL) {
        dst->plmn_id = cJSON_Duplicate(src->plmn_id, 1);
        if (dst->plmn_id == NULL) {
            goto fail;
        }
    }
    if (src->tai != NULL) {
        dst->tai = cJSON_Duplicate(src->tai, 1);
        if (dst->tai == NULL) {
            goto fail;
        }
    }
    if (src->snssais != NULL) {
        dst->snssais = cJSON_Duplicate(src->snssais, 1);
        if (dst->snssais == NULL) {
            goto fail;
        }
    }
    if (src->amf_set_id != NULL) {
        size_t n = strlen(src->amf_set_id) + 1;
        dst->amf_set_id = malloc(n);
        if (dst->amf_set_id == NULL) {
            goto fail;
        }
        memcpy(dst->amf_set_id, src->amf_set_id, n);
    }
    return 0;

fail:
    nssf_availability_record_clear(dst);
    return -1;
}

/* ════════════════════════════════════════════════════════════════════════
 * libpq backend
 * ════════════════════════════════════════════════════════════════════════ */

typedef struct {
    nssf_availability_repo_t base;
    PGconn *conn;
} pg_repo_t;

static void copy_err(char *errbuf, size_t errlen, const char *msg)
{
    if (errbuf == NULL || errlen == 0) {
        return;
    }
    /* libpq messages carry a trailing newline — keep it terse for the caller. */
    size_t n = strlen(msg);
    if (n >= errlen) {
        n = errlen - 1;
    }
    memcpy(errbuf, msg, n);
    while (n > 0 && (errbuf[n - 1] == '\n' || errbuf[n - 1] == '\r')) {
        --n;
    }
    errbuf[n] = '\0';
}

/*
 * Serialize a cJSON tree to a heap string for binding as a `$N::jsonb` text
 * param. Caller frees with cJSON_free (== free here). Returns NULL on OOM.
 */
static char *jsonb_text(const cJSON *v)
{
    if (v == NULL) {
        return NULL;
    }
    return cJSON_PrintUnformatted(v);
}

/* Parse one returned row (by column name) into rec (zeroed). 0 ok / -1 fail. */
static int pg_row_to_record(PGresult *res, int row,
                            nssf_availability_record_t *rec)
{
    memset(rec, 0, sizeof(*rec));

    int c_id = PQfnumber(res, "id");
    int c_plmn = PQfnumber(res, "plmn_id");
    int c_tai = PQfnumber(res, "tai");
    int c_snssais = PQfnumber(res, "snssais");
    int c_amf = PQfnumber(res, "amf_set_id");
    int c_realloc = PQfnumber(res, "reallocation_indication");
    if (c_id < 0 || c_plmn < 0 || c_tai < 0 || c_snssais < 0 ||
        c_amf < 0 || c_realloc < 0) {
        return -1;
    }

    const char *id = PQgetvalue(res, row, c_id);
    strncpy(rec->id, id, sizeof(rec->id) - 1);
    rec->id[sizeof(rec->id) - 1] = '\0';

    rec->plmn_id = cJSON_Parse(PQgetvalue(res, row, c_plmn));
    rec->tai = cJSON_Parse(PQgetvalue(res, row, c_tai));
    rec->snssais = cJSON_Parse(PQgetvalue(res, row, c_snssais));
    if (rec->plmn_id == NULL || rec->tai == NULL || rec->snssais == NULL) {
        goto fail;
    }

    if (!PQgetisnull(res, row, c_amf)) {
        const char *amf = PQgetvalue(res, row, c_amf);
        size_t n = strlen(amf) + 1;
        rec->amf_set_id = malloc(n);
        if (rec->amf_set_id == NULL) {
            goto fail;
        }
        memcpy(rec->amf_set_id, amf, n);
    }

    /* libpq renders boolean as "t"/"f" in text format. */
    const char *rb = PQgetvalue(res, row, c_realloc);
    rec->reallocation_indication = (rb[0] == 't');
    return 0;

fail:
    nssf_availability_record_clear(rec);
    return -1;
}

static int pg_find_for_plmn_tai(nssf_availability_repo_t *r,
                                const cJSON *plmn_id, const cJSON *tai,
                                nssf_availability_record_list_t *out)
{
    pg_repo_t *self = (pg_repo_t *)r;
    memset(out, 0, sizeof(*out));

    char *plmn_txt = jsonb_text(plmn_id);
    char *tai_txt = jsonb_text(tai);
    if (plmn_txt == NULL || tai_txt == NULL) {
        cJSON_free(plmn_txt);
        cJSON_free(tai_txt);
        return -1;
    }

    const char *sql =
        "SELECT id, plmn_id, tai, snssais, amf_set_id, reallocation_indication "
        "FROM availability "
        "WHERE plmn_id = $1::jsonb AND tai = $2::jsonb";
    const char *params[2] = {plmn_txt, tai_txt};

    PGresult *res = PQexecParams(self->conn, sql, 2, NULL, params,
                                 NULL, NULL, 0);
    cJSON_free(plmn_txt);
    cJSON_free(tai_txt);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return -1;
    }

    int n = PQntuples(res);
    int rc = 0;
    nssf_availability_record_t *recs = NULL;
    if (n > 0) {
        recs = calloc((size_t)n, sizeof(*recs));
        if (recs == NULL) {
            PQclear(res);
            return -1;
        }
        for (int i = 0; i < n; ++i) {
            if (pg_row_to_record(res, i, &recs[i]) != 0) {
                rc = -1;
                break;
            }
        }
    }
    PQclear(res);

    if (rc != 0) {
        for (int i = 0; i < n; ++i) {
            nssf_availability_record_clear(&recs[i]);
        }
        free(recs);
        return -1;
    }
    out->records = recs;
    out->count = (size_t)n;
    return 0;
}

/* Run a no-result command (BEGIN/COMMIT/ROLLBACK). 0 ok / -1 fail. */
static int pg_cmd(PGconn *conn, const char *sql)
{
    PGresult *res = PQexec(conn, sql);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return ok ? 0 : -1;
}

static int pg_upsert(nssf_availability_repo_t *r,
                     const nssf_availability_record_t *rec)
{
    pg_repo_t *self = (pg_repo_t *)r;

    char *plmn_txt = jsonb_text(rec->plmn_id);
    char *tai_txt = jsonb_text(rec->tai);
    char *snssais_txt = jsonb_text(rec->snssais);
    if (plmn_txt == NULL || tai_txt == NULL || snssais_txt == NULL) {
        cJSON_free(plmn_txt);
        cJSON_free(tai_txt);
        cJSON_free(snssais_txt);
        return -1;
    }

    const char realloc_param[2] = {rec->reallocation_indication ? 't' : 'f', '\0'};
    /* amf_set_id NULL → bind SQL NULL via NULL param pointer. */
    const char *params[6] = {
        rec->id, plmn_txt, tai_txt, snssais_txt,
        rec->amf_set_id, realloc_param,
    };

    const char *sql =
        "INSERT INTO availability "
        "(id, plmn_id, tai, snssais, amf_set_id, reallocation_indication, "
        " updated_at) "
        "VALUES ($1::uuid, $2::jsonb, $3::jsonb, $4::jsonb, $5, $6::boolean, "
        " NOW()) "
        "ON CONFLICT (id) DO UPDATE SET "
        "plmn_id = EXCLUDED.plmn_id, tai = EXCLUDED.tai, "
        "snssais = EXCLUDED.snssais, amf_set_id = EXCLUDED.amf_set_id, "
        "reallocation_indication = EXCLUDED.reallocation_indication, "
        "updated_at = NOW()";

    int rc = -1;
    if (pg_cmd(self->conn, "BEGIN") != 0) {
        goto cleanup;
    }

    PGresult *res = PQexecParams(self->conn, sql, 6, NULL, params,
                                 NULL, NULL, 0);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);

    if (!ok) {
        (void)pg_cmd(self->conn, "ROLLBACK");
        goto cleanup;
    }
    if (pg_cmd(self->conn, "COMMIT") != 0) {
        (void)pg_cmd(self->conn, "ROLLBACK");
        goto cleanup;
    }
    rc = 0;

cleanup:
    cJSON_free(plmn_txt);
    cJSON_free(tai_txt);
    cJSON_free(snssais_txt);
    return rc;
}

static int pg_patch(nssf_availability_repo_t *r, const char *id,
                    const cJSON *plmn_id, const cJSON *tai,
                    const cJSON *snssais, const char *amf_set_id,
                    bool set_reallocation, bool reallocation_indication)
{
    pg_repo_t *self = (pg_repo_t *)r;

    /*
     * COALESCE keeps the existing column when the param is SQL NULL. amf_set_id
     * is intentionally addressable to NULL, so it is gated by a separate
     * boolean param ($6) rather than COALESCE.
     */
    char *plmn_txt = jsonb_text(plmn_id);   /* NULL allowed */
    char *tai_txt = jsonb_text(tai);        /* NULL allowed */
    char *snssais_txt = jsonb_text(snssais);/* NULL allowed */
    if ((plmn_id != NULL && plmn_txt == NULL) ||
        (tai != NULL && tai_txt == NULL) ||
        (snssais != NULL && snssais_txt == NULL)) {
        cJSON_free(plmn_txt);
        cJSON_free(tai_txt);
        cJSON_free(snssais_txt);
        return -1;
    }

    const char set_amf_param[2] = {amf_set_id != NULL ? 't' : 'f', '\0'};
    const char set_realloc_param[2] = {set_reallocation ? 't' : 'f', '\0'};
    const char realloc_param[2] = {reallocation_indication ? 't' : 'f', '\0'};

    const char *params[8] = {
        id,
        plmn_txt,
        tai_txt,
        snssais_txt,
        amf_set_id,        /* value when $5_set true; NULL otherwise */
        set_amf_param,
        set_realloc_param,
        realloc_param,
    };

    const char *sql =
        "UPDATE availability SET "
        "plmn_id = COALESCE($2::jsonb, plmn_id), "
        "tai = COALESCE($3::jsonb, tai), "
        "snssais = COALESCE($4::jsonb, snssais), "
        "amf_set_id = CASE WHEN $6::boolean THEN $5 ELSE amf_set_id END, "
        "reallocation_indication = CASE WHEN $7::boolean THEN $8::boolean "
        "    ELSE reallocation_indication END, "
        "updated_at = NOW() "
        "WHERE id = $1::uuid";

    int rc = -1;
    if (pg_cmd(self->conn, "BEGIN") != 0) {
        goto cleanup;
    }

    PGresult *res = PQexecParams(self->conn, sql, 8, NULL, params,
                                 NULL, NULL, 0);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    int affected = ok ? atoi(PQcmdTuples(res)) : 0;
    PQclear(res);

    if (!ok) {
        (void)pg_cmd(self->conn, "ROLLBACK");
        goto cleanup;
    }
    if (pg_cmd(self->conn, "COMMIT") != 0) {
        (void)pg_cmd(self->conn, "ROLLBACK");
        goto cleanup;
    }
    rc = (affected > 0) ? 0 : 1;

cleanup:
    cJSON_free(plmn_txt);
    cJSON_free(tai_txt);
    cJSON_free(snssais_txt);
    return rc;
}

static int pg_delete(nssf_availability_repo_t *r, const char *id)
{
    pg_repo_t *self = (pg_repo_t *)r;
    const char *params[1] = {id};

    int rc = -1;
    if (pg_cmd(self->conn, "BEGIN") != 0) {
        return -1;
    }

    PGresult *res = PQexecParams(self->conn,
                                 "DELETE FROM availability WHERE id = $1::uuid",
                                 1, NULL, params, NULL, NULL, 0);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    int affected = ok ? atoi(PQcmdTuples(res)) : 0;
    PQclear(res);

    if (!ok) {
        (void)pg_cmd(self->conn, "ROLLBACK");
        return -1;
    }
    if (pg_cmd(self->conn, "COMMIT") != 0) {
        (void)pg_cmd(self->conn, "ROLLBACK");
        return -1;
    }
    rc = (affected > 0) ? 0 : 1;
    return rc;
}

static void pg_destroy(nssf_availability_repo_t *r)
{
    pg_repo_t *self = (pg_repo_t *)r;
    if (self->conn != NULL) {
        PQfinish(self->conn);
    }
    free(self);
}

static const nssf_availability_vtable_t PG_VTABLE = {
    .find_for_plmn_tai = pg_find_for_plmn_tai,
    .upsert = pg_upsert,
    .patch = pg_patch,
    .del = pg_delete,
    .destroy = pg_destroy,
};

nssf_availability_repo_t *nssf_availability_repo_new_pg(const char *conninfo,
                                                        char *errbuf,
                                                        size_t errlen)
{
    pg_repo_t *self = calloc(1, sizeof(*self));
    if (self == NULL) {
        copy_err(errbuf, errlen, "out of memory");
        return NULL;
    }
    self->base.vt = &PG_VTABLE;

    self->conn = PQconnectdb(conninfo != NULL ? conninfo : "");
    if (self->conn == NULL) {
        copy_err(errbuf, errlen, "PQconnectdb returned NULL");
        free(self);
        return NULL;
    }
    if (PQstatus(self->conn) != CONNECTION_OK) {
        copy_err(errbuf, errlen, PQerrorMessage(self->conn));
        PQfinish(self->conn);
        free(self);
        return NULL;
    }
    return &self->base;
}

/* ════════════════════════════════════════════════════════════════════════
 * in-memory backend — self-contained dynamic array (test seam)
 * ════════════════════════════════════════════════════════════════════════ */

typedef struct {
    nssf_availability_repo_t base;
    nssf_availability_record_t *items;
    size_t count;
    size_t cap;
} mem_repo_t;

/* Grow to hold at least one more item. 0 ok / -1 OOM. */
static int mem_reserve_one(mem_repo_t *self)
{
    if (self->count < self->cap) {
        return 0;
    }
    size_t new_cap = (self->cap == 0) ? 4 : self->cap * 2;
    nssf_availability_record_t *grown =
        realloc(self->items, new_cap * sizeof(*grown));
    if (grown == NULL) {
        return -1;
    }
    self->items = grown;
    self->cap = new_cap;
    return 0;
}

/* Index of the record with matching id, or -1. */
static long mem_index_of(const mem_repo_t *self, const char *id)
{
    for (size_t i = 0; i < self->count; ++i) {
        if (strcmp(self->items[i].id, id) == 0) {
            return (long)i;
        }
    }
    return -1;
}

/*
 * Compare two cJSON trees by canonical serialization. Mirrors the PG backend's
 * exact-match semantics closely enough for the test seam. 1 = equal.
 */
static int json_equal(const cJSON *a, const cJSON *b)
{
    if (a == NULL || b == NULL) {
        return a == b;
    }
    char *sa = cJSON_PrintUnformatted(a);
    char *sb = cJSON_PrintUnformatted(b);
    int eq = (sa != NULL && sb != NULL && strcmp(sa, sb) == 0);
    cJSON_free(sa);
    cJSON_free(sb);
    return eq;
}

static int mem_find_for_plmn_tai(nssf_availability_repo_t *r,
                                 const cJSON *plmn_id, const cJSON *tai,
                                 nssf_availability_record_list_t *out)
{
    mem_repo_t *self = (mem_repo_t *)r;
    memset(out, 0, sizeof(*out));

    size_t matched = 0;
    for (size_t i = 0; i < self->count; ++i) {
        if (json_equal(self->items[i].plmn_id, plmn_id) &&
            json_equal(self->items[i].tai, tai)) {
            ++matched;
        }
    }
    if (matched == 0) {
        return 0;
    }

    nssf_availability_record_t *recs = calloc(matched, sizeof(*recs));
    if (recs == NULL) {
        return -1;
    }
    size_t w = 0;
    for (size_t i = 0; i < self->count; ++i) {
        if (json_equal(self->items[i].plmn_id, plmn_id) &&
            json_equal(self->items[i].tai, tai)) {
            if (record_copy(&recs[w], &self->items[i]) != 0) {
                for (size_t k = 0; k < w; ++k) {
                    nssf_availability_record_clear(&recs[k]);
                }
                free(recs);
                return -1;
            }
            ++w;
        }
    }
    out->records = recs;
    out->count = matched;
    return 0;
}

static int mem_upsert(nssf_availability_repo_t *r,
                      const nssf_availability_record_t *rec)
{
    mem_repo_t *self = (mem_repo_t *)r;

    long idx = mem_index_of(self, rec->id);
    nssf_availability_record_t copy;
    if (record_copy(&copy, rec) != 0) {
        return -1;
    }

    if (idx >= 0) {
        nssf_availability_record_clear(&self->items[idx]);
        self->items[idx] = copy;
        return 0;
    }
    if (mem_reserve_one(self) != 0) {
        nssf_availability_record_clear(&copy);
        return -1;
    }
    self->items[self->count++] = copy;
    return 0;
}

static int mem_patch(nssf_availability_repo_t *r, const char *id,
                     const cJSON *plmn_id, const cJSON *tai,
                     const cJSON *snssais, const char *amf_set_id,
                     bool set_reallocation, bool reallocation_indication)
{
    mem_repo_t *self = (mem_repo_t *)r;
    long idx = mem_index_of(self, id);
    if (idx < 0) {
        return 1;
    }
    nssf_availability_record_t *cur = &self->items[idx];

    /* Stage all duplicates first so a mid-way OOM leaves cur untouched. */
    cJSON *new_plmn = NULL, *new_tai = NULL, *new_snssais = NULL;
    char *new_amf = NULL;
    bool amf_change = (amf_set_id != NULL);

    if (plmn_id != NULL && (new_plmn = cJSON_Duplicate(plmn_id, 1)) == NULL) {
        goto oom;
    }
    if (tai != NULL && (new_tai = cJSON_Duplicate(tai, 1)) == NULL) {
        goto oom;
    }
    if (snssais != NULL && (new_snssais = cJSON_Duplicate(snssais, 1)) == NULL) {
        goto oom;
    }
    if (amf_change) {
        size_t n = strlen(amf_set_id) + 1;
        new_amf = malloc(n);
        if (new_amf == NULL) {
            goto oom;
        }
        memcpy(new_amf, amf_set_id, n);
    }

    if (new_plmn != NULL) {
        cJSON_Delete(cur->plmn_id);
        cur->plmn_id = new_plmn;
    }
    if (new_tai != NULL) {
        cJSON_Delete(cur->tai);
        cur->tai = new_tai;
    }
    if (new_snssais != NULL) {
        cJSON_Delete(cur->snssais);
        cur->snssais = new_snssais;
    }
    if (amf_change) {
        free(cur->amf_set_id);
        cur->amf_set_id = new_amf;
    }
    if (set_reallocation) {
        cur->reallocation_indication = reallocation_indication;
    }
    return 0;

oom:
    cJSON_Delete(new_plmn);
    cJSON_Delete(new_tai);
    cJSON_Delete(new_snssais);
    free(new_amf);
    return -1;
}

static int mem_delete(nssf_availability_repo_t *r, const char *id)
{
    mem_repo_t *self = (mem_repo_t *)r;
    long idx = mem_index_of(self, id);
    if (idx < 0) {
        return 1;
    }
    nssf_availability_record_clear(&self->items[idx]);
    /* Compact — move the tail item into the hole (order is not significant). */
    self->items[idx] = self->items[self->count - 1];
    --self->count;
    return 0;
}

static void mem_destroy(nssf_availability_repo_t *r)
{
    mem_repo_t *self = (mem_repo_t *)r;
    for (size_t i = 0; i < self->count; ++i) {
        nssf_availability_record_clear(&self->items[i]);
    }
    free(self->items);
    free(self);
}

static const nssf_availability_vtable_t MEM_VTABLE = {
    .find_for_plmn_tai = mem_find_for_plmn_tai,
    .upsert = mem_upsert,
    .patch = mem_patch,
    .del = mem_delete,
    .destroy = mem_destroy,
};

nssf_availability_repo_t *nssf_availability_repo_new_inmemory(void)
{
    mem_repo_t *self = calloc(1, sizeof(*self));
    if (self == NULL) {
        return NULL;
    }
    self->base.vt = &MEM_VTABLE;
    return &self->base;
}

/* ── public dispatch ──────────────────────────────────────────────────── */

void nssf_availability_repo_free(nssf_availability_repo_t *repo)
{
    if (repo == NULL) {
        return;
    }
    repo->vt->destroy(repo);
}

int nssf_availability_repo_find_for_plmn_tai(nssf_availability_repo_t *repo,
                                             const cJSON *plmn_id,
                                             const cJSON *tai,
                                             nssf_availability_record_list_t *out)
{
    if (repo == NULL || out == NULL) {
        return -1;
    }
    return repo->vt->find_for_plmn_tai(repo, plmn_id, tai, out);
}

int nssf_availability_repo_upsert(nssf_availability_repo_t *repo,
                                  const nssf_availability_record_t *rec)
{
    if (repo == NULL || rec == NULL) {
        return -1;
    }
    return repo->vt->upsert(repo, rec);
}

int nssf_availability_repo_patch(nssf_availability_repo_t *repo,
                                 const char *id,
                                 const cJSON *plmn_id,
                                 const cJSON *tai,
                                 const cJSON *snssais,
                                 const char *amf_set_id,
                                 bool set_reallocation,
                                 bool reallocation_indication)
{
    if (repo == NULL || id == NULL) {
        return -1;
    }
    return repo->vt->patch(repo, id, plmn_id, tai, snssais, amf_set_id,
                           set_reallocation, reallocation_indication);
}

int nssf_availability_repo_delete(nssf_availability_repo_t *repo, const char *id)
{
    if (repo == NULL || id == NULL) {
        return -1;
    }
    return repo->vt->del(repo, id);
}
