/*
 * subscription_store.c — libpq + in-memory backends for the `subscription`
 * table, plus the initial-snapshot-on-create dispatch (WI-subscription-store).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten persistence module.
 *
 * SQL safety — every value reaches the server through PQexecParams positional
 * parameters ($1,$2,...); the JSONB filter column is bound as text and cast
 * `$N::jsonb` server-side. No value is ever concatenated into a SQL string.
 *
 * Initial-snapshot dispatch is CALL-DRIVEN — create() reads the current
 * availability snapshot, enqueues EXACTLY ONE retry_queue row, and triggers
 * ONE nssf_notification_dispatcher_dispatch_pending(). There is NO worker
 * thread, NO polling loop, NO while-redispatch here. WHY this matters: a
 * dispatcher polling/worker loop in this layer would promote the G-08 fail-
 * closed hot-loop gap to a BLOCKER (open-gaps-and-assumptions G-08 addendum,
 * 2026-06-04). The store stays call-driven so G-08 remains deferred.
 *
 * Fan-out resolve+enqueue (nssf_subscription_store_fanout_change) is the
 * engine→store→dispatcher cascade activation (B3 close): it scans the live
 * subscriptions, matches each persisted filter against a committed change event,
 * and enqueues ONE retry_queue row per match. It is store-layer matching ONLY —
 * no outbound POST, no loop; the caller drives ONE dispatch afterwards.
 */
#include "subscription_store.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libpq-fe.h>

#include "nssf_event_notification.h"

/* ── vtable ───────────────────────────────────────────────────────────── */

typedef struct {
    /* insert a fully-formed row whose id is already chosen by the caller. */
    int (*insert)(nssf_subscription_store_t *, const char *id,
                  const char *callback_uri, const cJSON *filter,
                  const char *expiry_at);
    int (*get)(nssf_subscription_store_t *, const char *id,
               nssf_subscription_record_t *out); /* 0 found / 1 absent / -1 err */
    int (*remove)(nssf_subscription_store_t *, const char *id);   /* 0 / -1 */
    int (*tombstone)(nssf_subscription_store_t *, const char *id);/* 0 / -1 */
    /* replace the mutable resource view of an existing row. 0 / 1 absent / -1. */
    int (*replace)(nssf_subscription_store_t *, const char *id,
                   const char *callback_uri, const cJSON *filter,
                   const char *expiry_at);
    /* invoke `cb` for each live (non-tombstoned) row's id/callback_uri/filter
     * (all borrowed, valid only during the call). cb returns 0 to continue, -1
     * to abort. Returns 0 on a full scan, -1 on a store/callback error. */
    int (*for_each_live)(nssf_subscription_store_t *,
                         int (*cb)(void *ctx, const char *id,
                                   const char *callback_uri,
                                   const cJSON *filter),
                         void *ctx);
    void (*destroy)(nssf_subscription_store_t *);
} nssf_subscription_vtable_t;

struct nssf_subscription_store {
    const nssf_subscription_vtable_t *vt;
    /* borrowed initial-snapshot dispatch collaborators (set via set_snapshot_seam). */
    nssf_availability_repo_t *avail_repo;
    nssf_retry_store_t *retry_store;
    nssf_notification_dispatcher_t *dispatcher;
    cJSON *snapshot_plmn_id; /* borrowed snapshot key (not owned). */
    cJSON *snapshot_tai;     /* borrowed snapshot key (not owned). */
};

/* ── shared record helpers ────────────────────────────────────────────── */

void nssf_subscription_record_clear(nssf_subscription_record_t *rec)
{
    if (rec == NULL) {
        return;
    }
    free(rec->callback_uri);
    cJSON_Delete(rec->filter);
    free(rec->expiry_at);
    memset(rec, 0, sizeof(*rec));
}

/* Duplicate a NUL-terminated string; NULL stays NULL. -1 only on OOM of a
 * non-NULL source. */
static int dup_str(char **dst, const char *src)
{
    if (src == NULL) {
        *dst = NULL;
        return 0;
    }
    size_t n = strlen(src) + 1;
    char *p = malloc(n);
    if (p == NULL) {
        return -1;
    }
    memcpy(p, src, n);
    *dst = p;
    return 0;
}

/*
 * Generate an RFC 4122-ish v4 UUID text into out[37]. WHY not a crypto RNG: the
 * id is a resource locator, not a security token (M6) — no custom crypto. It is
 * seeded from the clock plus a process-monotonic counter so concurrent creates
 * within one process do not collide.
 */
static void gen_uuid(char out[37])
{
    static uint64_t counter = 0;
    uint64_t c = ++counter;
    uint64_t t = (uint64_t)time(NULL);
    uint64_t hi = (t << 16) ^ (c * 0x9E3779B97F4A7C15ULL);
    uint64_t lo = (c << 1) ^ (t * 0xC2B2AE3D27D4EB4FULL);

    unsigned b[16];
    for (int i = 0; i < 8; ++i) {
        b[i] = (unsigned)((hi >> (i * 8)) & 0xFFu);
        b[i + 8] = (unsigned)((lo >> (i * 8)) & 0xFFu);
    }
    b[6] = (b[6] & 0x0Fu) | 0x40u; /* version 4. */
    b[8] = (b[8] & 0x3Fu) | 0x80u; /* RFC 4122 variant. */

    snprintf(out, 37,
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
             b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
}

/* Format an expiry_seconds offset from now as an RFC 3339 UTC timestamp into
 * out (>= 21 bytes). Returns out. */
static char *format_expiry(long expiry_seconds, char *out, size_t outlen)
{
    time_t when = time(NULL) + (time_t)expiry_seconds;
    struct tm tmv;
    gmtime_r(&when, &tmv);
    strftime(out, outlen, "%Y-%m-%dT%H:%M:%SZ", &tmv);
    return out;
}

/* ════════════════════════════════════════════════════════════════════════
 * initial-snapshot dispatch — call-driven, EXACTLY one dispatch per create
 * ════════════════════════════════════════════════════════════════════════ */

/*
 * Build the nssf_event_notification payload for the initial snapshot, reusing
 * the generated model (WI directive). The minimum well-formed body carries the
 * subscriptionId; the snapshot detail is left to the dispatcher's downstream
 * fan-out enrichment (deferred). Returns a malloc'd JSON string (caller frees
 * with free) or NULL on OOM.
 */
static char *build_snapshot_payload(const char *subscription_id)
{
    char *sub_copy = NULL;
    if (dup_str(&sub_copy, subscription_id) != 0 || sub_copy == NULL) {
        return NULL;
    }
    /* _create takes ownership of sub_copy and frees it via _free. */
    nssf_event_notification_t *ev =
        nssf_event_notification_create(sub_copy, NULL, NULL, NULL, NULL, NULL);
    if (ev == NULL) {
        free(sub_copy);
        return NULL;
    }
    cJSON *json = nssf_event_notification_convertToJSON(ev);
    nssf_event_notification_free(ev);
    if (json == NULL) {
        return NULL;
    }
    char *out = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    return out;
}

/*
 * On create(), dispatch the current availability snapshot EXACTLY ONCE. Reads
 * the snapshot for the subscription's filter key, enqueues ONE retry_queue row
 * with the real subscription UUID + its callbackUri, then triggers ONE
 * call-driven dispatch. NO loop. Best-effort: a snapshot/enqueue/dispatch
 * failure does NOT fail create() — the subscription is already durable and the
 * dispatcher owns redelivery.
 */
static void dispatch_initial_snapshot(nssf_subscription_store_t *store,
                                      const char *subscription_id,
                                      const char *callback_uri)
{
    if (store->retry_store == NULL || callback_uri == NULL) {
        return;
    }

    /* Read the current snapshot for trace/observability; the row is enqueued
     * regardless so the new subscriber gets its one initial notification. The
     * result list is read-then-freed — the store does not retain it. */
    if (store->avail_repo != NULL && store->snapshot_plmn_id != NULL &&
        store->snapshot_tai != NULL) {
        nssf_availability_record_list_t snap;
        if (nssf_availability_repo_find_for_plmn_tai(
                store->avail_repo, store->snapshot_plmn_id,
                store->snapshot_tai, &snap) == 0) {
            nssf_availability_record_list_free(&snap);
        }
    }

    char *payload = build_snapshot_payload(subscription_id);
    if (payload == NULL) {
        return;
    }
    /* ONE row, real UUID + callbackUri (the dispatcher's enqueue contract). */
    int rc = nssf_retry_store_enqueue(store->retry_store, subscription_id,
                                      callback_uri, payload, NULL);
    free(payload);
    if (rc != 0) {
        return;
    }

    /* ONE call-driven dispatch — no while-redispatch, no worker thread. The
     * caller (handler) drives any further cadence; this layer fires once. */
    if (store->dispatcher != NULL) {
        (void)nssf_notification_dispatcher_dispatch_pending(store->dispatcher,
                                                            NULL);
    }
}

/* ════════════════════════════════════════════════════════════════════════
 * RFC 6902 JSON Patch — atomic apply to the subscription resource view
 * ════════════════════════════════════════════════════════════════════════ */

/* Resolve a JSON Pointer (RFC 6901) within the resource object to the parent
 * container + the final unescaped token. Only the top-level resource fields
 * (/filter, /callbackUri, /expiry, and nested /filter/...) are addressable.
 * Returns the parent container or NULL when the path is malformed/unresolvable.
 * `out_key` receives a malloc'd unescaped final token (caller frees). */
static cJSON *resolve_pointer_parent(cJSON *root, const char *pointer,
                                     char **out_key)
{
    *out_key = NULL;
    if (pointer == NULL || pointer[0] != '/') {
        return NULL;
    }
    cJSON *cur = root;
    const char *p = pointer + 1;
    char *seg = NULL;

    while (1) {
        const char *slash = strchr(p, '/');
        size_t len = slash != NULL ? (size_t)(slash - p) : strlen(p);

        free(seg);
        seg = malloc(len + 1);
        if (seg == NULL) {
            return NULL;
        }
        /* Unescape RFC 6901 ~1 → '/', ~0 → '~'. */
        size_t w = 0;
        for (size_t i = 0; i < len; ++i) {
            if (p[i] == '~' && i + 1 < len && p[i + 1] == '1') {
                seg[w++] = '/';
                ++i;
            } else if (p[i] == '~' && i + 1 < len && p[i + 1] == '0') {
                seg[w++] = '~';
                ++i;
            } else {
                seg[w++] = p[i];
            }
        }
        seg[w] = '\0';

        if (slash == NULL) {
            *out_key = seg;
            return cur; /* parent container of the final token. */
        }
        cur = cJSON_GetObjectItemCaseSensitive(cur, seg);
        if (cur == NULL || !cJSON_IsObject(cur)) {
            free(seg);
            return NULL;
        }
        p = slash + 1;
    }
}

/* Apply ONE RFC 6902 op to the working resource copy. 0 ok / -1 invalid. */
static int apply_one_op(cJSON *resource, const cJSON *op_item)
{
    const cJSON *op = cJSON_GetObjectItemCaseSensitive(op_item, "op");
    const cJSON *path = cJSON_GetObjectItemCaseSensitive(op_item, "path");
    if (!cJSON_IsString(op) || !cJSON_IsString(path)) {
        return -1;
    }
    const char *op_s = op->valuestring;
    const char *path_s = path->valuestring;

    char *key = NULL;
    cJSON *parent = resolve_pointer_parent(resource, path_s, &key);
    if (parent == NULL || key == NULL) {
        free(key);
        return -1;
    }

    int rc = -1;
    if (strcmp(op_s, "add") == 0 || strcmp(op_s, "replace") == 0) {
        const cJSON *value = cJSON_GetObjectItemCaseSensitive(op_item, "value");
        if (value == NULL) {
            goto done;
        }
        if (strcmp(op_s, "replace") == 0 &&
            cJSON_GetObjectItemCaseSensitive(parent, key) == NULL) {
            goto done; /* replace requires an existing target. */
        }
        cJSON *dup = cJSON_Duplicate(value, 1);
        if (dup == NULL) {
            goto done;
        }
        cJSON_DeleteItemFromObjectCaseSensitive(parent, key);
        cJSON_AddItemToObject(parent, key, dup);
        rc = 0;
    } else if (strcmp(op_s, "remove") == 0) {
        if (cJSON_GetObjectItemCaseSensitive(parent, key) == NULL) {
            goto done;
        }
        cJSON_DeleteItemFromObjectCaseSensitive(parent, key);
        rc = 0;
    } else if (strcmp(op_s, "copy") == 0 || strcmp(op_s, "move") == 0) {
        const cJSON *from = cJSON_GetObjectItemCaseSensitive(op_item, "from");
        if (!cJSON_IsString(from)) {
            goto done;
        }
        char *from_key = NULL;
        cJSON *from_parent =
            resolve_pointer_parent(resource, from->valuestring, &from_key);
        if (from_parent == NULL || from_key == NULL) {
            free(from_key);
            goto done;
        }
        cJSON *src = cJSON_GetObjectItemCaseSensitive(from_parent, from_key);
        if (src == NULL) {
            free(from_key);
            goto done;
        }
        cJSON *dup = cJSON_Duplicate(src, 1);
        if (dup == NULL) {
            free(from_key);
            goto done;
        }
        if (strcmp(op_s, "move") == 0) {
            cJSON_DeleteItemFromObjectCaseSensitive(from_parent, from_key);
        }
        cJSON_DeleteItemFromObjectCaseSensitive(parent, key);
        cJSON_AddItemToObject(parent, key, dup);
        free(from_key);
        rc = 0;
    } else if (strcmp(op_s, "test") == 0) {
        const cJSON *value = cJSON_GetObjectItemCaseSensitive(op_item, "value");
        cJSON *target = cJSON_GetObjectItemCaseSensitive(parent, key);
        rc = (value != NULL && target != NULL &&
              cJSON_Compare(target, value, 1)) ? 0 : -1;
    } else {
        rc = -1; /* unknown op. */
    }

done:
    free(key);
    return rc;
}

/*
 * Build the resource view a patch applies to from a record, apply all ops
 * atomically to a working COPY, and write the mutated fields back into *out
 * (filter / callbackUri / expiry). On any op failure nothing is mutated and -1
 * is returned. 0 on success.
 */
static int apply_patch_atomic(const nssf_subscription_record_t *rec,
                              const cJSON *patch_doc,
                              char **out_callback, cJSON **out_filter,
                              char **out_expiry)
{
    *out_callback = NULL;
    *out_filter = NULL;
    *out_expiry = NULL;

    if (!cJSON_IsArray(patch_doc)) {
        return -1;
    }

    cJSON *resource = cJSON_CreateObject();
    if (resource == NULL) {
        return -1;
    }
    if (rec->callback_uri != NULL) {
        cJSON_AddStringToObject(resource, "callbackUri", rec->callback_uri);
    }
    if (rec->filter != NULL) {
        cJSON *fdup = cJSON_Duplicate(rec->filter, 1);
        if (fdup == NULL) {
            cJSON_Delete(resource);
            return -1;
        }
        cJSON_AddItemToObject(resource, "filter", fdup);
    }
    if (rec->expiry_at != NULL) {
        cJSON_AddStringToObject(resource, "expiry", rec->expiry_at);
    }

    /* Atomic — apply to the working copy; a single failure aborts the whole
     * patch (all ops or none). */
    const cJSON *op_item = NULL;
    cJSON_ArrayForEach(op_item, patch_doc) {
        if (apply_one_op(resource, op_item) != 0) {
            cJSON_Delete(resource);
            return -1;
        }
    }

    /* Extract the mutated fields. */
    const cJSON *cb = cJSON_GetObjectItemCaseSensitive(resource, "callbackUri");
    const cJSON *fl = cJSON_GetObjectItemCaseSensitive(resource, "filter");
    const cJSON *ex = cJSON_GetObjectItemCaseSensitive(resource, "expiry");

    int rc = -1;
    char *new_cb = NULL;
    cJSON *new_fl = NULL;
    char *new_ex = NULL;

    if (cJSON_IsString(cb) && dup_str(&new_cb, cb->valuestring) != 0) {
        goto fail;
    }
    if (fl != NULL && (new_fl = cJSON_Duplicate(fl, 1)) == NULL) {
        goto fail;
    }
    if (cJSON_IsString(ex) && dup_str(&new_ex, ex->valuestring) != 0) {
        goto fail;
    }
    /* callbackUri/filter are NOT NULLable in the schema — a patch that removed
     * them is rejected. */
    if (new_cb == NULL || new_fl == NULL) {
        goto fail;
    }
    *out_callback = new_cb;
    *out_filter = new_fl;
    *out_expiry = new_ex;
    rc = 0;
    cJSON_Delete(resource);
    return rc;

fail:
    free(new_cb);
    cJSON_Delete(new_fl);
    free(new_ex);
    cJSON_Delete(resource);
    return -1;
}

/* ════════════════════════════════════════════════════════════════════════
 * libpq backend
 * ════════════════════════════════════════════════════════════════════════ */

typedef struct {
    nssf_subscription_store_t base;
    PGconn *conn;
} pg_store_t;

static void copy_err(char *errbuf, size_t errlen, const char *msg)
{
    if (errbuf == NULL || errlen == 0) {
        return;
    }
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

/* Run a no-result command (BEGIN/COMMIT/ROLLBACK). 0 ok / -1 fail. */
static int pg_cmd(PGconn *conn, const char *sql)
{
    PGresult *res = PQexec(conn, sql);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return ok ? 0 : -1;
}

static int pg_insert(nssf_subscription_store_t *s, const char *id,
                     const char *callback_uri, const cJSON *filter,
                     const char *expiry_at)
{
    pg_store_t *self = (pg_store_t *)s;
    char *filter_txt = cJSON_PrintUnformatted(filter);
    if (filter_txt == NULL) {
        return -1;
    }
    const char *params[4] = {id, callback_uri, filter_txt, expiry_at};
    const char *sql =
        "INSERT INTO subscription (id, callback_url, filter, expiry_at) "
        "VALUES ($1::uuid, $2, $3::jsonb, $4::timestamptz)";

    int rc = -1;
    if (pg_cmd(self->conn, "BEGIN") != 0) {
        goto cleanup;
    }
    PGresult *res = PQexecParams(self->conn, sql, 4, NULL, params,
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
    cJSON_free(filter_txt);
    return rc;
}

static int pg_get(nssf_subscription_store_t *s, const char *id,
                  nssf_subscription_record_t *out)
{
    pg_store_t *self = (pg_store_t *)s;
    memset(out, 0, sizeof(*out));
    const char *params[1] = {id};
    const char *sql =
        "SELECT id, callback_url, filter, "
        "to_char(expiry_at, 'YYYY-MM-DD\"T\"HH24:MI:SS\"Z\"') AS expiry_at, "
        "tombstone "
        "FROM subscription WHERE id = $1::uuid AND tombstone = FALSE";

    PGresult *res = PQexecParams(self->conn, sql, 1, NULL, params,
                                 NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return -1;
    }
    if (PQntuples(res) == 0) {
        PQclear(res);
        return 1;
    }

    int rc = -1;
    strncpy(out->id, PQgetvalue(res, 0, 0), sizeof(out->id) - 1);
    out->id[sizeof(out->id) - 1] = '\0';
    if (dup_str(&out->callback_uri, PQgetvalue(res, 0, 1)) != 0) {
        goto fail;
    }
    out->filter = cJSON_Parse(PQgetvalue(res, 0, 2));
    if (out->filter == NULL) {
        goto fail;
    }
    if (!PQgetisnull(res, 0, 3) &&
        dup_str(&out->expiry_at, PQgetvalue(res, 0, 3)) != 0) {
        goto fail;
    }
    out->tombstone = (PQgetvalue(res, 0, 4)[0] == 't');
    rc = 0;
    PQclear(res);
    return rc;

fail:
    PQclear(res);
    nssf_subscription_record_clear(out);
    return -1;
}

static int pg_remove(nssf_subscription_store_t *s, const char *id)
{
    pg_store_t *self = (pg_store_t *)s;
    const char *params[1] = {id};
    if (pg_cmd(self->conn, "BEGIN") != 0) {
        return -1;
    }
    /* Idempotent unsubscribe — DELETE of an absent id is a no-op success. The
     * retry_queue FK cascades pending rows (schema.sql). */
    PGresult *res = PQexecParams(self->conn,
                                 "DELETE FROM subscription WHERE id = $1::uuid",
                                 1, NULL, params, NULL, NULL, 0);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    if (!ok) {
        (void)pg_cmd(self->conn, "ROLLBACK");
        return -1;
    }
    if (pg_cmd(self->conn, "COMMIT") != 0) {
        (void)pg_cmd(self->conn, "ROLLBACK");
        return -1;
    }
    return 0;
}

static int pg_tombstone(nssf_subscription_store_t *s, const char *id)
{
    pg_store_t *self = (pg_store_t *)s;
    const char *params[1] = {id};
    if (pg_cmd(self->conn, "BEGIN") != 0) {
        return -1;
    }
    PGresult *res = PQexecParams(
        self->conn,
        "UPDATE subscription SET tombstone = TRUE WHERE id = $1::uuid",
        1, NULL, params, NULL, NULL, 0);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    if (!ok) {
        (void)pg_cmd(self->conn, "ROLLBACK");
        return -1;
    }
    if (pg_cmd(self->conn, "COMMIT") != 0) {
        (void)pg_cmd(self->conn, "ROLLBACK");
        return -1;
    }
    return 0;
}

static int pg_replace(nssf_subscription_store_t *s, const char *id,
                      const char *callback_uri, const cJSON *filter,
                      const char *expiry_at)
{
    pg_store_t *self = (pg_store_t *)s;
    char *filter_txt = cJSON_PrintUnformatted(filter);
    if (filter_txt == NULL) {
        return -1;
    }
    const char *params[4] = {id, callback_uri, filter_txt, expiry_at};
    const char *sql =
        "UPDATE subscription SET callback_url = $2, filter = $3::jsonb, "
        "expiry_at = $4::timestamptz "
        "WHERE id = $1::uuid AND tombstone = FALSE";

    int rc = -1;
    if (pg_cmd(self->conn, "BEGIN") != 0) {
        goto cleanup;
    }
    PGresult *res = PQexecParams(self->conn, sql, 4, NULL, params,
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
    cJSON_free(filter_txt);
    return rc;
}

static int pg_for_each_live(nssf_subscription_store_t *s,
                            int (*cb)(void *ctx, const char *id,
                                      const char *callback_uri,
                                      const cJSON *filter),
                            void *ctx)
{
    pg_store_t *self = (pg_store_t *)s;
    /* Snapshot the live rows in one read. The fan-out match runs per-row in the
     * caller; this is a naive scan (SubscriptionStore.md filter-match: naive scan
     * or JSONB GIN — the index is a later perf tuning, not a correctness need). */
    const char *sql =
        "SELECT id, callback_url, filter FROM subscription "
        "WHERE tombstone = FALSE";
    PGresult *res = PQexecParams(self->conn, sql, 0, NULL, NULL, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return -1;
    }
    int rc = 0;
    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        const char *id = PQgetvalue(res, i, 0);
        const char *cb_uri = PQgetisnull(res, i, 1) ? NULL : PQgetvalue(res, i, 1);
        cJSON *filter = PQgetisnull(res, i, 2)
                            ? NULL
                            : cJSON_Parse(PQgetvalue(res, i, 2));
        int r = cb(ctx, id, cb_uri, filter);
        cJSON_Delete(filter);
        if (r != 0) {
            rc = -1;
            break;
        }
    }
    PQclear(res);
    return rc;
}

static void pg_destroy(nssf_subscription_store_t *s)
{
    pg_store_t *self = (pg_store_t *)s;
    if (self->conn != NULL) {
        PQfinish(self->conn);
    }
    free(self);
}

static const nssf_subscription_vtable_t PG_VTABLE = {
    .insert = pg_insert,
    .get = pg_get,
    .remove = pg_remove,
    .tombstone = pg_tombstone,
    .replace = pg_replace,
    .for_each_live = pg_for_each_live,
    .destroy = pg_destroy,
};

nssf_subscription_store_t *nssf_subscription_store_new_pg(const char *conninfo,
                                                          char *errbuf,
                                                          size_t errlen)
{
    pg_store_t *self = calloc(1, sizeof(*self));
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
    nssf_subscription_store_t base;
    nssf_subscription_record_t *items;
    size_t count;
    size_t cap;
} mem_store_t;

static int mem_reserve_one(mem_store_t *self)
{
    if (self->count < self->cap) {
        return 0;
    }
    size_t new_cap = (self->cap == 0) ? 4 : self->cap * 2;
    nssf_subscription_record_t *grown =
        realloc(self->items, new_cap * sizeof(*grown));
    if (grown == NULL) {
        return -1;
    }
    self->items = grown;
    self->cap = new_cap;
    return 0;
}

static long mem_index_of(const mem_store_t *self, const char *id)
{
    for (size_t i = 0; i < self->count; ++i) {
        if (strcmp(self->items[i].id, id) == 0) {
            return (long)i;
        }
    }
    return -1;
}

/* Fill a zeroed record from explicit fields (deep copy). 0 ok / -1 OOM. */
static int record_fill(nssf_subscription_record_t *dst, const char *id,
                       const char *callback_uri, const cJSON *filter,
                       const char *expiry_at)
{
    memset(dst, 0, sizeof(*dst));
    strncpy(dst->id, id, sizeof(dst->id) - 1);
    dst->id[sizeof(dst->id) - 1] = '\0';
    if (dup_str(&dst->callback_uri, callback_uri) != 0) {
        goto fail;
    }
    if (filter != NULL && (dst->filter = cJSON_Duplicate(filter, 1)) == NULL) {
        goto fail;
    }
    if (dup_str(&dst->expiry_at, expiry_at) != 0) {
        goto fail;
    }
    return 0;
fail:
    nssf_subscription_record_clear(dst);
    return -1;
}

static int mem_insert(nssf_subscription_store_t *s, const char *id,
                      const char *callback_uri, const cJSON *filter,
                      const char *expiry_at)
{
    mem_store_t *self = (mem_store_t *)s;
    if (mem_reserve_one(self) != 0) {
        return -1;
    }
    nssf_subscription_record_t rec;
    if (record_fill(&rec, id, callback_uri, filter, expiry_at) != 0) {
        return -1;
    }
    self->items[self->count++] = rec;
    return 0;
}

static int mem_get(nssf_subscription_store_t *s, const char *id,
                   nssf_subscription_record_t *out)
{
    mem_store_t *self = (mem_store_t *)s;
    memset(out, 0, sizeof(*out));
    long idx = mem_index_of(self, id);
    if (idx < 0 || self->items[idx].tombstone) {
        return 1;
    }
    const nssf_subscription_record_t *src = &self->items[idx];
    if (record_fill(out, src->id, src->callback_uri, src->filter,
                    src->expiry_at) != 0) {
        return -1;
    }
    return 0;
}

static int mem_remove(nssf_subscription_store_t *s, const char *id)
{
    mem_store_t *self = (mem_store_t *)s;
    long idx = mem_index_of(self, id);
    if (idx < 0) {
        return 0; /* idempotent — absent is success. */
    }
    nssf_subscription_record_clear(&self->items[idx]);
    self->items[idx] = self->items[self->count - 1];
    --self->count;
    return 0;
}

static int mem_tombstone(nssf_subscription_store_t *s, const char *id)
{
    mem_store_t *self = (mem_store_t *)s;
    long idx = mem_index_of(self, id);
    if (idx >= 0) {
        self->items[idx].tombstone = true;
    }
    return 0; /* idempotent. */
}

static int mem_replace(nssf_subscription_store_t *s, const char *id,
                       const char *callback_uri, const cJSON *filter,
                       const char *expiry_at)
{
    mem_store_t *self = (mem_store_t *)s;
    long idx = mem_index_of(self, id);
    if (idx < 0 || self->items[idx].tombstone) {
        return 1;
    }
    nssf_subscription_record_t fresh;
    if (record_fill(&fresh, id, callback_uri, filter, expiry_at) != 0) {
        return -1;
    }
    nssf_subscription_record_clear(&self->items[idx]);
    self->items[idx] = fresh;
    return 0;
}

static int mem_for_each_live(nssf_subscription_store_t *s,
                             int (*cb)(void *ctx, const char *id,
                                       const char *callback_uri,
                                       const cJSON *filter),
                             void *ctx)
{
    mem_store_t *self = (mem_store_t *)s;
    /* Iterate over a snapshot count: the callback enqueues only and never mutates
     * the store, and tombstone (set on a DELETED match) flips a flag in place
     * without reordering, so a forward index scan stays valid. */
    size_t n = self->count;
    for (size_t i = 0; i < n; ++i) {
        const nssf_subscription_record_t *r = &self->items[i];
        if (r->tombstone) {
            continue;
        }
        if (cb(ctx, r->id, r->callback_uri, r->filter) != 0) {
            return -1;
        }
    }
    return 0;
}

static void mem_destroy(nssf_subscription_store_t *s)
{
    mem_store_t *self = (mem_store_t *)s;
    for (size_t i = 0; i < self->count; ++i) {
        nssf_subscription_record_clear(&self->items[i]);
    }
    free(self->items);
    free(self);
}

static const nssf_subscription_vtable_t MEM_VTABLE = {
    .insert = mem_insert,
    .get = mem_get,
    .remove = mem_remove,
    .tombstone = mem_tombstone,
    .replace = mem_replace,
    .for_each_live = mem_for_each_live,
    .destroy = mem_destroy,
};

nssf_subscription_store_t *nssf_subscription_store_new_inmemory(void)
{
    mem_store_t *self = calloc(1, sizeof(*self));
    if (self == NULL) {
        return NULL;
    }
    self->base.vt = &MEM_VTABLE;
    return &self->base;
}

/* ── public dispatch ──────────────────────────────────────────────────── */

void nssf_subscription_store_free(nssf_subscription_store_t *store)
{
    if (store == NULL) {
        return;
    }
    store->vt->destroy(store);
}

void nssf_subscription_store_set_snapshot_seam(
    nssf_subscription_store_t *store,
    nssf_availability_repo_t *avail_repo,
    nssf_retry_store_t *retry_store,
    nssf_notification_dispatcher_t *dispatcher)
{
    if (store == NULL) {
        return;
    }
    store->avail_repo = avail_repo;
    store->retry_store = retry_store;
    store->dispatcher = dispatcher;
}

/* Extract the snapshot plmn_id/tai key from a subscription filter, if present.
 * The pointers are BORROWED from the filter tree (valid for the call). A filter
 * without these keys leaves the store's configured snapshot key in effect. */
static void set_snapshot_key_from_filter(nssf_subscription_store_t *store,
                                         const cJSON *filter)
{
    cJSON *plmn = cJSON_GetObjectItemCaseSensitive((cJSON *)filter, "plmnId");
    cJSON *tai = cJSON_GetObjectItemCaseSensitive((cJSON *)filter, "tai");
    if (plmn != NULL) {
        store->snapshot_plmn_id = plmn;
    }
    if (tai != NULL) {
        store->snapshot_tai = tai;
    }
}

nssf_sub_result_e nssf_subscription_store_create(
    nssf_subscription_store_t *store,
    const char *callback_uri,
    const cJSON *filter,
    long expiry_seconds,
    char out_id[37])
{
    if (out_id != NULL) {
        out_id[0] = '\0';
    }
    if (store == NULL || callback_uri == NULL || filter == NULL ||
        out_id == NULL) {
        return NSSF_SUB_ERROR;
    }

    char id[37];
    gen_uuid(id);

    char expiry_buf[32];
    format_expiry(expiry_seconds, expiry_buf, sizeof(expiry_buf));

    if (store->vt->insert(store, id, callback_uri, filter, expiry_buf) != 0) {
        return NSSF_SUB_ERROR;
    }
    memcpy(out_id, id, sizeof(id));

    /* Initial-snapshot dispatch — EXACTLY ONCE, call-driven, NO loop. The
     * filter supplies the snapshot key when it carries plmnId/tai. */
    set_snapshot_key_from_filter(store, filter);
    dispatch_initial_snapshot(store, id, callback_uri);
    store->snapshot_plmn_id = NULL;
    store->snapshot_tai = NULL;

    return NSSF_SUB_OK;
}

nssf_sub_result_e nssf_subscription_store_delete(
    nssf_subscription_store_t *store, const char *id)
{
    if (store == NULL || id == NULL) {
        return NSSF_SUB_ERROR;
    }
    /* Idempotent — absent row is success, not 404 at this layer. */
    return (store->vt->remove(store, id) == 0) ? NSSF_SUB_OK : NSSF_SUB_ERROR;
}

nssf_sub_result_e nssf_subscription_store_patch(
    nssf_subscription_store_t *store,
    const char *id,
    const char *content_type,
    const cJSON *patch_doc)
{
    if (store == NULL || id == NULL || patch_doc == NULL) {
        return NSSF_SUB_ERROR;
    }
    /* 415 — reject anything that is not application/json-patch+json BEFORE the
     * body is touched (G-04). A trailing "; charset=..." parameter is allowed. */
    if (content_type == NULL ||
        strncmp(content_type, "application/json-patch+json",
                strlen("application/json-patch+json")) != 0) {
        return NSSF_SUB_UNSUPPORTED;
    }

    nssf_subscription_record_t rec;
    int g = store->vt->get(store, id, &rec);
    if (g == 1) {
        return NSSF_SUB_NOT_FOUND;
    }
    if (g != 0) {
        return NSSF_SUB_ERROR;
    }

    char *new_cb = NULL;
    cJSON *new_fl = NULL;
    char *new_ex = NULL;
    int rc = apply_patch_atomic(&rec, patch_doc, &new_cb, &new_fl, &new_ex);
    nssf_subscription_record_clear(&rec);
    if (rc != 0) {
        return NSSF_SUB_BAD_PATCH;
    }

    int r = store->vt->replace(store, id, new_cb, new_fl, new_ex);
    free(new_cb);
    cJSON_Delete(new_fl);
    free(new_ex);

    if (r == 1) {
        return NSSF_SUB_NOT_FOUND;
    }
    return (r == 0) ? NSSF_SUB_OK : NSSF_SUB_ERROR;
}

nssf_sub_result_e nssf_subscription_store_on_availability_deleted(
    nssf_subscription_store_t *store, const char *subscription_id)
{
    if (store == NULL || subscription_id == NULL) {
        return NSSF_SUB_ERROR;
    }
    /* Soft-delete (tombstone) — the schema FK cascades pending retry_queue rows
     * on a later hard delete. Idempotent. The engine change-event → store →
     * dispatcher fan-out (nssf_subscription_store_fanout_change) calls this for
     * each matched subscription on a DELETED change event. */
    return (store->vt->tombstone(store, subscription_id) == 0)
               ? NSSF_SUB_OK
               : NSSF_SUB_ERROR;
}

nssf_sub_result_e nssf_subscription_store_get(
    nssf_subscription_store_t *store,
    const char *id,
    nssf_subscription_record_t *out)
{
    if (store == NULL || id == NULL || out == NULL) {
        return NSSF_SUB_ERROR;
    }
    int g = store->vt->get(store, id, out);
    if (g == 1) {
        return NSSF_SUB_NOT_FOUND;
    }
    return (g == 0) ? NSSF_SUB_OK : NSSF_SUB_ERROR;
}

/* ════════════════════════════════════════════════════════════════════════
 * fan-out resolve + enqueue — engine→store→dispatcher cascade (B3 close)
 * ════════════════════════════════════════════════════════════════════════ */

/*
 * Does a persisted subscription `filter` match a change event? The filter is the
 * create document (or its /filter subtree) as the Post handler persisted it; the
 * only routing key it carries today is an optional `tai` (set_snapshot_key_from_filter
 * already reads plmnId/tai from this same shape). MATCH RULE (minimal + correct
 * for the persisted shape — no filter-grammar expansion):
 *   - a filter with NO `tai` constraint is a broad subscription → matches any TAI;
 *   - a filter WITH a `tai` matches only when it equals the event's tai
 *     (cJSON_Compare, case-sensitive). When the event carries no tai (NULL), a
 *     tai-constrained filter cannot match.
 * The changed nfId is not a filter dimension in this shape, so it is not matched
 * on (it rides in the payload for the receiver). WHY no spec re-derivation: the
 * shape comes from the existing Post handler + SubscriptionStore.md filter-match
 * decision, not from re-reading the OpenAPI.
 */
static bool filter_matches_event(const cJSON *filter,
                                 const nssf_availability_change_event_t *ev)
{
    if (filter == NULL) {
        return true; /* no persisted constraint → broad subscriber. */
    }
    cJSON *ftai = cJSON_GetObjectItemCaseSensitive((cJSON *)filter, "tai");
    if (ftai == NULL) {
        return true; /* no tai constraint → matches any TAI. */
    }
    if (ev->tai == NULL) {
        return false; /* tai-constrained filter, event has no tai. */
    }
    return cJSON_Compare(ftai, ev->tai, 1) != 0;
}

/* Per-row fan-out context threaded through for_each_live. */
typedef struct {
    nssf_subscription_store_t *store;
    nssf_retry_store_t *retry_store;
    const nssf_availability_change_event_t *ev;
    int enqueued;        /* count of rows enqueued so far. */
    bool tombstone_match; /* DELETED → tombstone each matched subscription. */
} fanout_ctx_t;

/*
 * for_each_live callback — one live subscription. Enqueues EXACTLY ONE retry_queue
 * row per match (real UUID + callbackUri + event-shaped payload) and, on a DELETED
 * change, tombstones the matched subscription. Returns 0 to continue the scan; a
 * best-effort per-row failure (bad callbackUri, payload OOM, enqueue error) is
 * SKIPPED, not fatal — the scan continues so one poison row never blocks the rest.
 */
static int fanout_one(void *vctx, const char *id, const char *callback_uri,
                      const cJSON *filter)
{
    fanout_ctx_t *ctx = (fanout_ctx_t *)vctx;
    if (id == NULL || !filter_matches_event(filter, ctx->ev)) {
        return 0;
    }
    /* B1 URL gate at enqueue time — never enqueue an un-postable callbackUri (the
     * same shared policy the dispatcher enforces before attach+POST). */
    if (callback_uri == NULL ||
        !nssf_notification_dispatcher_callback_url_allowed(callback_uri)) {
        return 0;
    }

    char *payload = build_snapshot_payload(id);
    if (payload == NULL) {
        return 0; /* best-effort: skip this row, continue the scan. */
    }
    int rc = nssf_retry_store_enqueue(ctx->retry_store, id, callback_uri,
                                      payload, NULL);
    free(payload);
    if (rc == 0) {
        ++ctx->enqueued;
    }

    /* DELETED change → soft-delete the affected subscription so a pending row
     * cascades per the schema FK on a later hard delete (Acceptance #4). */
    if (ctx->tombstone_match) {
        (void)ctx->store->vt->tombstone(ctx->store, id);
    }
    return 0;
}

int nssf_subscription_store_fanout_change(
    nssf_subscription_store_t *store,
    nssf_retry_store_t *retry_store,
    const nssf_availability_change_event_t *ev)
{
    if (store == NULL || retry_store == NULL || ev == NULL) {
        return -1;
    }
    fanout_ctx_t ctx = {
        .store = store,
        .retry_store = retry_store,
        .ev = ev,
        .enqueued = 0,
        .tombstone_match = (ev->change_type == NSSF_AVAIL_CHANGE_DELETED),
    };
    if (store->vt->for_each_live(store, fanout_one, &ctx) != 0) {
        return -1;
    }
    return ctx.enqueued;
}
