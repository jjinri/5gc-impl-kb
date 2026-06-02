/*
 * notification_dispatcher.c — NSSF NotificationDispatcher (WI-notification-dispatcher).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten outbound module.
 *
 * Phase 1~3 MINIMUM (open-gaps-and-assumptions G-08): synchronous dispatch plus
 * ONE retry. No exponential backoff / max_attempts ceiling / jitter / dead-letter
 * table — those are Phase 4 hardening. The retry_queue persists only
 * subscription_id/payload/attempt_count/next_attempt_at (infra/nssf/schema.sql),
 * so callback_uri + correlation_id ride inside the row's payload envelope.
 *
 * The outbound POST uses libcurl with TLS peer + hostname verification always on
 * (ADR-0004 M2), min TLS 1.2 (M1), and redirect-follow always off (M2) —
 * OpenSSL (via libcurl) owns TLS and no primitive is hand-rolled (M6). The
 * OAuth2 bearer is attached through oauth2_outbound (M4); a fail-closed ABORT
 * FORBIDS the POST (no unauthenticated outbound).
 *
 * callback URIs and the libpq conninfo come from operator config (M7); nothing
 * secret-adjacent is logged.
 */

#define _POSIX_C_SOURCE 200809L

#include "notification_dispatcher.h"

#include <ctype.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>
#include <libpq-fe.h>

#include "cJSON.h"

/* Phase 1~3 retry budget — ONE retry after the first attempt (G-08). */
#define NSSF_DISPATCH_MAX_ATTEMPTS 2

/* Outbound POST timeouts (seconds). Conservative fixed defaults this slice. */
#define NSSF_DISPATCH_CONNECT_TIMEOUT_S 2L
#define NSSF_DISPATCH_REQUEST_TIMEOUT_S 5L

/* ──────────────────────────────────────────────────────────────────────────
 * retry-queue store seam — vtable + two backends (mirrors availability_repo)
 * ────────────────────────────────────────────────────────────────────────── */

typedef struct {
    int (*enqueue)(nssf_retry_store_t *, const char *sub_id,
                   const char *callback_uri, const char *payload_json,
                   const char *correlation_id);
    int (*dequeue)(nssf_retry_store_t *, nssf_retry_item_t *out);
    int (*complete)(nssf_retry_store_t *, const char *id);
    int (*requeue)(nssf_retry_store_t *, const char *id);
    void (*destroy)(nssf_retry_store_t *);
} nssf_retry_vtable_t;

struct nssf_retry_store {
    const nssf_retry_vtable_t *vt;
};

void nssf_retry_item_clear(nssf_retry_item_t *item)
{
    if (item == NULL) {
        return;
    }
    free(item->id);
    free(item->subscription_id);
    free(item->callback_uri);
    free(item->payload_json);
    free(item->correlation_id);
    memset(item, 0, sizeof(*item));
}

static char *dup_or_null(const char *s)
{
    return s != NULL ? strdup(s) : NULL;
}

/*
 * Build the persisted payload envelope. The Phase 1~3 schema has no callback_uri
 * / correlation_id columns, so we wrap the notification body plus its routing /
 * trace metadata into the single JSONB `payload` column. Returns a malloc'd JSON
 * string (caller frees) or NULL on OOM.
 */
static char *envelope_build(const char *callback_uri,
                            const char *payload_json,
                            const char *correlation_id)
{
    cJSON *env = cJSON_CreateObject();
    if (env == NULL) {
        return NULL;
    }
    if (callback_uri != NULL) {
        cJSON_AddStringToObject(env, "callbackUri", callback_uri);
    }
    if (correlation_id != NULL) {
        cJSON_AddStringToObject(env, "correlationId", correlation_id);
    }
    /* body is stored as a parsed sub-tree when it is itself JSON, else a string. */
    cJSON *body = payload_json != NULL ? cJSON_Parse(payload_json) : NULL;
    if (body != NULL) {
        cJSON_AddItemToObject(env, "body", body);
    } else if (payload_json != NULL) {
        cJSON_AddStringToObject(env, "body", payload_json);
    }
    char *out = cJSON_PrintUnformatted(env);
    cJSON_Delete(env);
    return out;
}

/* Extract one envelope string field into a malloc'd copy (or NULL). */
static char *envelope_get_string(const cJSON *env, const char *key)
{
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(env, key);
    if (!cJSON_IsString(v) || v->valuestring == NULL) {
        return NULL;
    }
    return strdup(v->valuestring);
}

/*
 * Split a stored envelope back into callback_uri / correlation_id / body. The
 * body is re-serialized when it is a JSON sub-tree, or copied when a plain
 * string. Returns 0 on success (fields owned by the caller), -1 on OOM.
 */
static int envelope_split(const char *envelope_json, char **out_callback,
                          char **out_correlation, char **out_body)
{
    *out_callback = NULL;
    *out_correlation = NULL;
    *out_body = NULL;
    cJSON *env = cJSON_Parse(envelope_json);
    if (env == NULL) {
        return -1;
    }
    *out_callback = envelope_get_string(env, "callbackUri");
    *out_correlation = envelope_get_string(env, "correlationId");

    const cJSON *body = cJSON_GetObjectItemCaseSensitive(env, "body");
    if (cJSON_IsString(body) && body->valuestring != NULL) {
        *out_body = strdup(body->valuestring);
    } else if (body != NULL) {
        *out_body = cJSON_PrintUnformatted(body);
    }
    cJSON_Delete(env);
    return 0;
}

/* ── libpq backend ─────────────────────────────────────────────────────── */

typedef struct {
    nssf_retry_store_t base;
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

static int pg_cmd(PGconn *conn, const char *sql)
{
    PGresult *res = PQexec(conn, sql);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return ok ? 0 : -1;
}

static int pg_enqueue(nssf_retry_store_t *s, const char *sub_id,
                      const char *callback_uri, const char *payload_json,
                      const char *correlation_id)
{
    pg_store_t *self = (pg_store_t *)s;
    char *envelope = envelope_build(callback_uri, payload_json, correlation_id);
    if (envelope == NULL) {
        return -1;
    }
    /* Positional params only — no value is concatenated into the SQL string. */
    const char *params[2] = {sub_id, envelope};
    const char *sql =
        "INSERT INTO retry_queue (subscription_id, payload, next_attempt_at) "
        "VALUES ($1::uuid, $2::jsonb, NOW())";
    PGresult *res = PQexecParams(self->conn, sql, 2, NULL, params, NULL, NULL, 0);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    free(envelope);
    return ok ? 0 : -1;
}

/*
 * Dequeue the next due row. FOR UPDATE SKIP LOCKED keeps concurrent dispatchers
 * from claiming the same row; the row stays locked inside the open transaction
 * until complete()/requeue() commits. Returns 1 (claimed), 0 (none due), -1.
 */
static int pg_dequeue(nssf_retry_store_t *s, nssf_retry_item_t *out)
{
    pg_store_t *self = (pg_store_t *)s;
    memset(out, 0, sizeof(*out));

    if (pg_cmd(self->conn, "BEGIN") != 0) {
        return -1;
    }
    const char *sql =
        "SELECT id, subscription_id, payload, attempt_count "
        "FROM retry_queue "
        "WHERE next_attempt_at <= NOW() "
        "ORDER BY next_attempt_at ASC, id ASC "
        "LIMIT 1 FOR UPDATE SKIP LOCKED";
    PGresult *res = PQexecParams(self->conn, sql, 0, NULL, NULL, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        (void)pg_cmd(self->conn, "ROLLBACK");
        return -1;
    }
    if (PQntuples(res) == 0) {
        PQclear(res);
        (void)pg_cmd(self->conn, "COMMIT");
        return 0;
    }

    char *env = strdup(PQgetvalue(res, 0, 2));
    out->id = strdup(PQgetvalue(res, 0, 0));
    out->subscription_id = strdup(PQgetvalue(res, 0, 1));
    out->attempt_count = atoi(PQgetvalue(res, 0, 3));
    PQclear(res);

    if (env == NULL || out->id == NULL || out->subscription_id == NULL ||
        envelope_split(env, &out->callback_uri, &out->correlation_id,
                       &out->payload_json) != 0) {
        free(env);
        nssf_retry_item_clear(out);
        (void)pg_cmd(self->conn, "ROLLBACK");
        return -1;
    }
    free(env);
    /* Transaction stays OPEN — the row is held under FOR UPDATE until the
     * dispatcher calls complete()/requeue() (which COMMIT). */
    return 1;
}

static int pg_complete(nssf_retry_store_t *s, const char *id)
{
    pg_store_t *self = (pg_store_t *)s;
    const char *params[1] = {id};
    const char *sql = "DELETE FROM retry_queue WHERE id = $1::bigint";
    PGresult *res = PQexecParams(self->conn, sql, 1, NULL, params, NULL, NULL, 0);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    /* Close the dequeue transaction regardless — releases the row lock. */
    (void)pg_cmd(self->conn, ok ? "COMMIT" : "ROLLBACK");
    return ok ? 0 : -1;
}

static int pg_requeue(nssf_retry_store_t *s, const char *id)
{
    pg_store_t *self = (pg_store_t *)s;
    const char *params[1] = {id};
    /* Phase 1~3: no backoff — re-arm for an immediate next attempt (G-08). */
    const char *sql =
        "UPDATE retry_queue "
        "SET attempt_count = attempt_count + 1, next_attempt_at = NOW() "
        "WHERE id = $1::bigint";
    PGresult *res = PQexecParams(self->conn, sql, 1, NULL, params, NULL, NULL, 0);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    (void)pg_cmd(self->conn, ok ? "COMMIT" : "ROLLBACK");
    return ok ? 0 : -1;
}

static void pg_destroy(nssf_retry_store_t *s)
{
    pg_store_t *self = (pg_store_t *)s;
    if (self->conn != NULL) {
        PQfinish(self->conn);
    }
    free(self);
}

static const nssf_retry_vtable_t PG_VTABLE = {
    .enqueue = pg_enqueue,
    .dequeue = pg_dequeue,
    .complete = pg_complete,
    .requeue = pg_requeue,
    .destroy = pg_destroy,
};

nssf_retry_store_t *nssf_retry_store_new_pg(const char *conninfo,
                                            char *errbuf, size_t errlen)
{
    if (conninfo == NULL || conninfo[0] == '\0') {
        copy_err(errbuf, errlen, "conninfo is required");
        return NULL;
    }
    pg_store_t *self = calloc(1, sizeof(*self));
    if (self == NULL) {
        copy_err(errbuf, errlen, "out of memory");
        return NULL;
    }
    self->conn = PQconnectdb(conninfo);
    if (self->conn == NULL || PQstatus(self->conn) != CONNECTION_OK) {
        copy_err(errbuf, errlen,
                 self->conn != NULL ? PQerrorMessage(self->conn)
                                    : "PQconnectdb returned NULL");
        if (self->conn != NULL) {
            PQfinish(self->conn);
        }
        free(self);
        return NULL;
    }
    self->base.vt = &PG_VTABLE;
    return &self->base;
}

/* ── in-memory backend (test seam) ─────────────────────────────────────── */

typedef struct {
    long long id;
    char *subscription_id;
    char *envelope; /* same envelope JSON the PG backend stores in `payload`. */
    int attempt_count;
    bool claimed; /* mirrors FOR UPDATE — a claimed row is hidden from dequeue. */
} mem_row_t;

typedef struct {
    nssf_retry_store_t base;
    mem_row_t *rows;
    size_t count;
    size_t cap;
    long long next_id;
    pthread_mutex_t lock;
} mem_store_t;

static long mem_index_of(mem_store_t *self, long long id)
{
    for (size_t i = 0; i < self->count; ++i) {
        if (self->rows[i].id == id) {
            return (long)i;
        }
    }
    return -1;
}

static int mem_enqueue(nssf_retry_store_t *s, const char *sub_id,
                       const char *callback_uri, const char *payload_json,
                       const char *correlation_id)
{
    mem_store_t *self = (mem_store_t *)s;
    char *envelope = envelope_build(callback_uri, payload_json, correlation_id);
    char *sub = dup_or_null(sub_id);
    if (envelope == NULL || sub == NULL) {
        free(envelope);
        free(sub);
        return -1;
    }

    pthread_mutex_lock(&self->lock);
    if (self->count == self->cap) {
        size_t new_cap = (self->cap == 0) ? 4 : self->cap * 2;
        mem_row_t *grown = realloc(self->rows, new_cap * sizeof(*grown));
        if (grown == NULL) {
            pthread_mutex_unlock(&self->lock);
            free(envelope);
            free(sub);
            return -1;
        }
        self->rows = grown;
        self->cap = new_cap;
    }
    mem_row_t *row = &self->rows[self->count++];
    row->id = self->next_id++;
    row->subscription_id = sub;
    row->envelope = envelope;
    row->attempt_count = 0;
    row->claimed = false;
    pthread_mutex_unlock(&self->lock);
    return 0;
}

static int mem_dequeue(nssf_retry_store_t *s, nssf_retry_item_t *out)
{
    mem_store_t *self = (mem_store_t *)s;
    memset(out, 0, sizeof(*out));

    pthread_mutex_lock(&self->lock);
    /* Lowest id among unclaimed rows = FIFO insertion order (SKIP LOCKED). */
    long pick = -1;
    for (size_t i = 0; i < self->count; ++i) {
        if (self->rows[i].claimed) {
            continue;
        }
        if (pick < 0 || self->rows[i].id < self->rows[pick].id) {
            pick = (long)i;
        }
    }
    if (pick < 0) {
        pthread_mutex_unlock(&self->lock);
        return 0;
    }
    mem_row_t *row = &self->rows[pick];

    char idbuf[32];
    snprintf(idbuf, sizeof(idbuf), "%lld", row->id);
    out->id = strdup(idbuf);
    out->subscription_id = dup_or_null(row->subscription_id);
    out->attempt_count = row->attempt_count;
    int split = (out->id != NULL && out->subscription_id != NULL)
                    ? envelope_split(row->envelope, &out->callback_uri,
                                     &out->correlation_id, &out->payload_json)
                    : -1;
    if (out->id == NULL || out->subscription_id == NULL || split != 0) {
        nssf_retry_item_clear(out);
        pthread_mutex_unlock(&self->lock);
        return -1;
    }
    row->claimed = true; /* held until complete()/requeue(). */
    pthread_mutex_unlock(&self->lock);
    return 1;
}

static void mem_remove_at(mem_store_t *self, long idx)
{
    free(self->rows[idx].subscription_id);
    free(self->rows[idx].envelope);
    for (size_t i = (size_t)idx; i + 1 < self->count; ++i) {
        self->rows[i] = self->rows[i + 1];
    }
    self->count--;
}

static int mem_complete(nssf_retry_store_t *s, const char *id)
{
    mem_store_t *self = (mem_store_t *)s;
    long long key = atoll(id);
    pthread_mutex_lock(&self->lock);
    long idx = mem_index_of(self, key);
    if (idx >= 0) {
        mem_remove_at(self, idx);
    }
    pthread_mutex_unlock(&self->lock);
    return 0;
}

static int mem_requeue(nssf_retry_store_t *s, const char *id)
{
    mem_store_t *self = (mem_store_t *)s;
    long long key = atoll(id);
    pthread_mutex_lock(&self->lock);
    long idx = mem_index_of(self, key);
    if (idx >= 0) {
        self->rows[idx].attempt_count += 1;
        self->rows[idx].claimed = false; /* re-arm for an immediate attempt. */
    }
    pthread_mutex_unlock(&self->lock);
    return 0;
}

static void mem_destroy(nssf_retry_store_t *s)
{
    mem_store_t *self = (mem_store_t *)s;
    for (size_t i = 0; i < self->count; ++i) {
        free(self->rows[i].subscription_id);
        free(self->rows[i].envelope);
    }
    free(self->rows);
    pthread_mutex_destroy(&self->lock);
    free(self);
}

static const nssf_retry_vtable_t MEM_VTABLE = {
    .enqueue = mem_enqueue,
    .dequeue = mem_dequeue,
    .complete = mem_complete,
    .requeue = mem_requeue,
    .destroy = mem_destroy,
};

nssf_retry_store_t *nssf_retry_store_new_inmemory(void)
{
    mem_store_t *self = calloc(1, sizeof(*self));
    if (self == NULL) {
        return NULL;
    }
    if (pthread_mutex_init(&self->lock, NULL) != 0) {
        free(self);
        return NULL;
    }
    self->next_id = 1;
    self->base.vt = &MEM_VTABLE;
    return &self->base;
}

/* ── store dispatch wrappers ───────────────────────────────────────────── */

void nssf_retry_store_free(nssf_retry_store_t *store)
{
    if (store == NULL) {
        return;
    }
    store->vt->destroy(store);
}

int nssf_retry_store_enqueue(nssf_retry_store_t *store, const char *subscription_id,
                             const char *callback_uri, const char *payload_json,
                             const char *correlation_id)
{
    if (store == NULL || subscription_id == NULL || payload_json == NULL) {
        return -1;
    }
    return store->vt->enqueue(store, subscription_id, callback_uri, payload_json,
                              correlation_id);
}

int nssf_retry_store_dequeue(nssf_retry_store_t *store, nssf_retry_item_t *out)
{
    if (store == NULL || out == NULL) {
        return -1;
    }
    return store->vt->dequeue(store, out);
}

int nssf_retry_store_complete(nssf_retry_store_t *store, const char *id)
{
    if (store == NULL || id == NULL) {
        return -1;
    }
    return store->vt->complete(store, id);
}

int nssf_retry_store_requeue(nssf_retry_store_t *store, const char *id)
{
    if (store == NULL || id == NULL) {
        return -1;
    }
    return store->vt->requeue(store, id);
}

/* ──────────────────────────────────────────────────────────────────────────
 * dispatcher
 * ────────────────────────────────────────────────────────────────────────── */

struct nssf_notification_dispatcher {
    nssf_retry_store_t *store;      /* borrowed. */
    nssf_oauth2_outbound_t *oauth;  /* borrowed; NULL → never attach a bearer. */
    nssf_notification_transport_fn transport;
    void *transport_ctx;
};

/* ── outbound payload from a change event ──────────────────────────────── */

/*
 * Serialize the change event into a notification body. Reuses the engine's
 * borrowed cJSON snapshots (nf_id / tai / after) rather than reinventing a
 * payload struct — the No-Spec-Rediscovery invariant. `after` is the new
 * availability snapshot (NULL for a delete).
 */
static char *event_to_payload(const nssf_availability_change_event_t *ev)
{
    cJSON *body = cJSON_CreateObject();
    if (body == NULL) {
        return NULL;
    }
    if (ev->nf_id != NULL) {
        cJSON_AddStringToObject(body, "nfId", ev->nf_id);
    }
    if (ev->tai != NULL) {
        cJSON_AddItemToObject(body, "tai", cJSON_Duplicate(ev->tai, 1));
    }
    const char *kind = ev->change_type == NSSF_AVAIL_CHANGE_REPLACED ? "REPLACED"
                       : ev->change_type == NSSF_AVAIL_CHANGE_PATCHED ? "PATCHED"
                                                                      : "DELETED";
    cJSON_AddStringToObject(body, "changeType", kind);
    if (ev->after != NULL) {
        cJSON_AddItemToObject(body, "authorizedNssaiAvailabilityData",
                              cJSON_Duplicate(ev->after, 1));
    }
    char *out = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    return out;
}

void nssf_notification_dispatcher_change_publish(
    void *ctx, const nssf_availability_change_event_t *ev)
{
    nssf_notification_dispatcher_t *disp = (nssf_notification_dispatcher_t *)ctx;
    if (disp == NULL || ev == NULL) {
        return;
    }
    char *payload = event_to_payload(ev);
    if (payload == NULL) {
        return;
    }
    /*
     * Phase 1~3 producer: enqueue keyed by the changed nfId. SubscriptionStore
     * fan-out (resolve nfId/tai → affected subscriptions + their callbackUri) is
     * a Phase 3 collaborator; until it lands the row carries the nfId as its
     * subscription key and a NULL callback (the dispatch step then has no target
     * and treats it as a terminal drop). The required behavior this slice proves
     * is the enqueue-on-commit seam, not the fan-out.
     */
    const char *key = ev->nf_id != NULL ? ev->nf_id : "";
    (void)nssf_retry_store_enqueue(disp->store, key, NULL, payload, NULL);
    free(payload);
}

/* ── correlation-id propagation ────────────────────────────────────────── */

/*
 * Generate a 32-hex-char id from the clock + a counter when the inbound trigger
 * carried none (NotificationDispatcher.md §Decisions — NSSF self-generates a
 * correlation id when absent). Not a cryptographic identifier — it is a trace
 * correlator, so no crypto primitive is involved (M6). Writes a NUL-terminated
 * 32-hex string into out[33].
 */
static void gen_correlation_hex(char out[33])
{
    static pthread_mutex_t seed_lock = PTHREAD_MUTEX_INITIALIZER;
    static uint64_t counter = 0;
    pthread_mutex_lock(&seed_lock);
    uint64_t c = ++counter;
    pthread_mutex_unlock(&seed_lock);

    uint64_t t = (uint64_t)time(NULL);
    snprintf(out, 33, "%016llx%016llx", (unsigned long long)t,
             (unsigned long long)c);
}

/* Lowercase-hex copy of the first 16 bytes (32 hex chars) of an id, padded. */
static void to_trace_id(const char *src, char out[33])
{
    size_t w = 0;
    for (size_t i = 0; src != NULL && src[i] != '\0' && w < 32; ++i) {
        unsigned char ch = (unsigned char)src[i];
        if (isxdigit(ch)) {
            out[w++] = (char)tolower(ch);
        }
    }
    while (w < 32) {
        out[w++] = '0';
    }
    out[32] = '\0';
}

/*
 * Append the correlation headers to `headers`. The effective correlation id is
 * propagated as 3gpp-Sbi-Correlation-Info and folded into a W3C traceparent
 * (version-00, sampled). Returns the (possibly grown) list, or NULL on OOM.
 */
static struct curl_slist *append_correlation(struct curl_slist *headers,
                                             const char *correlation_id)
{
    char buf[128];
    int n = snprintf(buf, sizeof(buf), "3gpp-Sbi-Correlation-Info: %s",
                     correlation_id);
    if (n < 0 || (size_t)n >= sizeof(buf)) {
        return NULL;
    }
    struct curl_slist *next = curl_slist_append(headers, buf);
    if (next == NULL) {
        return NULL;
    }
    headers = next;

    char trace_id[33];
    to_trace_id(correlation_id, trace_id);
    char tp[96];
    /* W3C traceparent: 00-<32hex trace-id>-<16hex span-id>-01. */
    snprintf(tp, sizeof(tp), "traceparent: 00-%s-%.16s-01", trace_id, trace_id);
    next = curl_slist_append(headers, tp);
    if (next == NULL) {
        return NULL;
    }
    return next;
}

/* ── default libcurl transport (production outbound POST) ──────────────── */

typedef struct {
    char *buf;
    size_t len;
} body_sink_t;

static size_t sink_write(char *ptr, size_t size, size_t nmemb, void *userp)
{
    body_sink_t *sink = (body_sink_t *)userp;
    size_t add = size * nmemb;
    char *grown = realloc(sink->buf, sink->len + add + 1);
    if (grown == NULL) {
        return 0;
    }
    sink->buf = grown;
    memcpy(sink->buf + sink->len, ptr, add);
    sink->len += add;
    sink->buf[sink->len] = '\0';
    return add;
}

/*
 * Production outbound transport. TLS peer + hostname verification are MANDATORY
 * and never disabled (ADR-0004 M2); min TLS 1.2 (M1); redirects are NEVER
 * followed (CURLOPT_FOLLOWLOCATION=0). All crypto is delegated to libcurl/
 * OpenSSL (M6).
 */
static int libcurl_transport(const nssf_notification_dispatcher_t *disp,
                             const char *callback_uri, const char *body,
                             struct curl_slist *headers, long *out_http_status,
                             void *transport_ctx)
{
    (void)disp;
    (void)transport_ctx;

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return -1;
    }
    body_sink_t sink = {.buf = NULL, .len = 0};

    curl_easy_setopt(curl, CURLOPT_URL, callback_uri);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sink_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sink);

    /* TLS verification MANDATORY (ADR-0004 M2) — never 0. */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
    /* Redirects NEVER followed — a 3xx Location fails closed. */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, NSSF_DISPATCH_CONNECT_TIMEOUT_S);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, NSSF_DISPATCH_REQUEST_TIMEOUT_S);

    CURLcode rc = curl_easy_perform(curl);
    long status = 0;
    if (rc == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    }
    curl_easy_cleanup(curl);
    free(sink.buf); /* the notification response body is not consumed this slice. */

    if (rc != CURLE_OK) {
        return -1; /* connection / timeout / TLS handshake → retriable. */
    }
    *out_http_status = status;
    return 0;
}

/* ── construction / seam install ───────────────────────────────────────── */

nssf_notification_dispatcher_t *nssf_notification_dispatcher_new(
    nssf_retry_store_t *store, nssf_oauth2_outbound_t *oauth)
{
    if (store == NULL) {
        return NULL;
    }
    nssf_notification_dispatcher_t *disp = calloc(1, sizeof(*disp));
    if (disp == NULL) {
        return NULL;
    }
    disp->store = store;
    disp->oauth = oauth;
    disp->transport = libcurl_transport;
    disp->transport_ctx = NULL;
    return disp;
}

void nssf_notification_dispatcher_free(nssf_notification_dispatcher_t *disp)
{
    free(disp); /* store + oauth are borrowed — not freed here. */
}

void nssf_notification_dispatcher_set_transport(
    nssf_notification_dispatcher_t *disp,
    nssf_notification_transport_fn transport_fn, void *transport_ctx)
{
    if (disp == NULL) {
        return;
    }
    disp->transport = transport_fn != NULL ? transport_fn : libcurl_transport;
    disp->transport_ctx = transport_ctx;
}

/* ── dispatch one row ──────────────────────────────────────────────────── */

/* HTTP 4xx is a terminal client error — no retry (NotificationDispatcher.md). */
static bool is_terminal_status(long status)
{
    return status >= 400 && status < 500;
}

/* 2xx is success. */
static bool is_success_status(long status)
{
    return status >= 200 && status < 300;
}

/*
 * Build the outbound header list for one attempt: Content-Type, correlation
 * headers, and (when M4 is enabled) the OAuth2 bearer. Returns 0 on success with
 * *out_headers set (caller frees with curl_slist_free_all); returns 1 to signal
 * fail-closed ABORT (caller must NOT emit the POST); -1 on OOM/build failure.
 */
static int build_headers(nssf_notification_dispatcher_t *disp,
                         const char *correlation_id,
                         struct curl_slist **out_headers)
{
    *out_headers = NULL;
    struct curl_slist *headers =
        curl_slist_append(NULL, "Content-Type: application/json");
    if (headers == NULL) {
        return -1;
    }
    struct curl_slist *with_corr = append_correlation(headers, correlation_id);
    if (with_corr == NULL) {
        curl_slist_free_all(headers);
        return -1;
    }
    headers = with_corr;

    if (disp->oauth != NULL) {
        nssf_oauth2_outbound_status_t st =
            nssf_oauth2_outbound_attach_bearer(disp->oauth, NULL, &headers);
        if (st == NSSF_OAUTH2_ABORT) {
            /* fail-closed (ADR-0004 M4 / ratified decision #7) — no unauth POST. */
            curl_slist_free_all(headers);
            return 1;
        }
        /* NSSF_OAUTH2_NO_ATTACH (disabled/dev) → proceed without a bearer. */
    }
    *out_headers = headers;
    return 0;
}

nssf_dispatch_result_e nssf_notification_dispatcher_dispatch_pending(
    nssf_notification_dispatcher_t *disp, const char *inbound_correlation_id)
{
    if (disp == NULL) {
        return NSSF_DISPATCH_ERROR;
    }

    nssf_retry_item_t item;
    int dq = nssf_retry_store_dequeue(disp->store, &item);
    if (dq == 0) {
        return NSSF_DISPATCH_NONE;
    }
    if (dq < 0) {
        return NSSF_DISPATCH_ERROR;
    }

    /* A row with no resolved callback target cannot be delivered (the Phase 3
     * SubscriptionStore fan-out is not wired yet) — drop it terminally. */
    if (item.callback_uri == NULL || item.callback_uri[0] == '\0') {
        (void)nssf_retry_store_complete(disp->store, item.id);
        nssf_retry_item_clear(&item);
        return NSSF_DISPATCH_DROPPED;
    }

    /* Effective correlation id: explicit inbound > stored > freshly generated. */
    char gen[33];
    const char *corr = inbound_correlation_id != NULL ? inbound_correlation_id
                       : item.correlation_id != NULL  ? item.correlation_id
                                                      : NULL;
    if (corr == NULL) {
        gen_correlation_hex(gen);
        corr = gen;
    }

    /*
     * In-line attempt budget for THIS dispatch call. Phase 1~3 (G-08) is the
     * first attempt + at most ONE retry, capped further by any attempts the row
     * already carries from a prior call. The retry happens in-line only on a
     * RETRIABLE failure (5xx / timeout / connection / TLS handshake); a 4xx is
     * terminal immediately and a 2xx succeeds. After the budget is spent the row
     * is removed — there is no dead-letter table this phase.
     */
    int budget = NSSF_DISPATCH_MAX_ATTEMPTS - item.attempt_count;
    if (budget < 1) {
        budget = 1; /* always make at least one attempt for a claimed row. */
    }

    nssf_dispatch_result_e result = NSSF_DISPATCH_ERROR;
    for (int i = 0; i < budget; ++i) {
        struct curl_slist *headers = NULL;
        int hb = build_headers(disp, corr, &headers);
        if (hb == 1) {
            /*
             * fail-closed (ADR-0004 M4 / ratified decision #7) — no unauthenticated
             * POST. Re-arm the row so it is retried once M4 token acquire recovers
             * rather than silently dropped.
             */
            (void)nssf_retry_store_requeue(disp->store, item.id);
            nssf_retry_item_clear(&item);
            return NSSF_DISPATCH_FORBIDDEN;
        }
        if (hb < 0) {
            (void)nssf_retry_store_requeue(disp->store, item.id);
            nssf_retry_item_clear(&item);
            return NSSF_DISPATCH_ERROR;
        }

        long status = 0;
        int trc = disp->transport(disp, item.callback_uri, item.payload_json,
                                  headers, &status, disp->transport_ctx);
        curl_slist_free_all(headers);

        if (trc == 0 && is_success_status(status)) {
            result = NSSF_DISPATCH_SENT;
            break;
        }
        if (trc == 0 && is_terminal_status(status)) {
            result = NSSF_DISPATCH_DROPPED; /* 4xx → no retry. */
            break;
        }
        /* trc != 0 (connection/timeout/TLS) or 5xx → retriable; loop once more. */
        result = NSSF_DISPATCH_DROPPED;
    }

    /* Terminal either way — remove the row (no dead-letter table, G-08). */
    (void)nssf_retry_store_complete(disp->store, item.id);
    nssf_retry_item_clear(&item);
    return result;
}
