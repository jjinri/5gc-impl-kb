/*
 * notification_dispatcher.c — NSSF NotificationDispatcher (WI-notification-dispatcher).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten outbound module.
 *
 * Phase 4 RESILIENCE (open-gaps-and-assumptions G-08): fail-closed BOUNDED retry.
 * A claimed row is attempted EXACTLY ONCE per call (no in-line immediate retry); a
 * retriable failure RE-ARMS the row with exponential backoff + bounded jitter for
 * a LATER call (caller-driven cadence), and a terminal failure or an exhausted
 * attempt budget moves the row to a terminal `dead_letter` status that is RETAINED
 * for audit and never re-claimed. Stays CALL-DRIVEN — AT MOST one row per call,
 * NO worker / poll / sleep / thread / timer. The retry_queue persists
 * subscription_id/payload/attempt_count/next_attempt_at/status
 * (infra/nssf/schema.sql), so callback_uri + correlation_id ride inside the row's
 * payload envelope.
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

/*
 * Phase 4 bounded-retry budget. A row is delivered AT MOST NSSF_DISPATCH_MAX_ATTEMPTS
 * times across calls (the first attempt counts); once attempt_count reaches the
 * ceiling a retriable failure goes to dead_letter instead of re-arming. WHY 5:
 * with the backoff schedule below (200ms base, 30s cap) five attempts span ~tens
 * of seconds, enough to ride out a transient AMF/NRF blip without an unbounded
 * queue residency.
 */
#define NSSF_DISPATCH_MAX_ATTEMPTS 5

/*
 * Exponential backoff bounds (milliseconds). delay = base * 2^attempt_count,
 * capped at max, plus a bounded "equal jitter" term in [0, capped/2] (see
 * nssf_retry_backoff_delay_ms). base/max are operator-tunable in spirit; with no
 * dispatcher config struct on this seam they are fixed named defaults here.
 */
#define NSSF_DISPATCH_BACKOFF_BASE_MS 200L
#define NSSF_DISPATCH_BACKOFF_MAX_MS  30000L

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
    /* re-arm with backoff — next_attempt_at = now + backoff(attempt_count),
     * attempt_count++ (backoff computed inside the store from the row). */
    int (*requeue)(nssf_retry_store_t *, const char *id);
    /* terminal failure — status='dead_letter', row RETAINED (never re-claimed). */
    int (*dead_letter)(nssf_retry_store_t *, const char *id);
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

/* ──────────────────────────────────────────────────────────────────────────
 * backoff schedule — pure, deterministic-given-rand (tester bounds it)
 * ────────────────────────────────────────────────────────────────────────── */

/*
 * delay_ms for a row that has already made `attempt_count` attempts. Exponential
 * (base * 2^attempt_count) capped at max, then an "equal jitter" component in
 * [0, capped/2] scaled by rand_unit ∈ [0,1) is ADDED. So the result is bounded:
 *   capped     = min(base * 2^attempt_count, max)              (overflow-safe)
 *   delay      = capped + floor(rand_unit * (capped / 2))      ∈ [capped, capped + capped/2)
 * Tester invariants: monotonic non-decreasing `capped` floor up to the cap, never
 * below base, never above max + max/2, and exactly `capped` when rand_unit==0.
 * WHY equal-jitter (not full): keeps a guaranteed minimum spacing (the `capped`
 * floor) so a thundering-herd of re-armed rows still spreads, without ever firing
 * sooner than the deterministic floor.
 */
long nssf_retry_backoff_delay_ms(int attempt_count, long base_ms, long max_ms,
                                 double rand_unit)
{
    if (base_ms < 0) {
        base_ms = 0;
    }
    if (max_ms < base_ms) {
        max_ms = base_ms;
    }
    /* capped = min(base << attempt_count, max), shift computed overflow-safe. */
    long capped = max_ms;
    if (attempt_count < 0) {
        attempt_count = 0;
    }
    /* Stop doubling as soon as we reach/exceed max — avoids long overflow on a
     * pathological attempt_count and keeps the cap exact. */
    long step = base_ms;
    for (int i = 0; i < attempt_count; ++i) {
        if (step >= max_ms || step > (max_ms / 2)) {
            step = max_ms;
            break;
        }
        step *= 2;
    }
    if (step < max_ms) {
        capped = step;
    }
    if (rand_unit < 0.0) {
        rand_unit = 0.0;
    } else if (rand_unit >= 1.0) {
        rand_unit = 0.999999;
    }
    long jitter = (long)(rand_unit * (double)(capped / 2));
    return capped + jitter;
}

/* Draw a jitter unit in [0,1). Trace-correlator grade, NOT crypto (M6) — rand()
 * is sufficient; the schedule bound does not depend on randomness quality. */
static double jitter_unit(void)
{
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

/* ──────────────────────────────────────────────────────────────────────────
 * outbound URL policy gate (B1) — mirrors oauth2_outbound.c's scheme policy
 *
 * Runs BEFORE build_headers / oauth2 bearer attach and BEFORE the POST so a
 * token is never attached to a plaintext / attacker-shifted / malformed URL.
 * libcurl's CURLOPT_PROTOCOLS cap is belt-and-suspenders that runs too late to
 * protect the token attach step.
 * ────────────────────────────────────────────────────────────────────────── */

static bool has_prefix_ci(const char *s, const char *prefix)
{
    for (size_t i = 0; prefix[i] != '\0'; i++) {
        if (s[i] == '\0' ||
            tolower((unsigned char)s[i]) != tolower((unsigned char)prefix[i])) {
            return false;
        }
    }
    return true;
}

/*
 * Reject userinfo('@') and fragment('#') anywhere in the URL. A '@' shifts the
 * real host (credential-in-URL / host confusion) and a '#' is never sent to the
 * server — both are config/data errors on a callback URI.
 */
static bool url_has_userinfo_or_fragment(const char *url)
{
    for (const char *p = url; *p != '\0'; p++) {
        if (*p == '@' || *p == '#') {
            return true;
        }
    }
    return false;
}

/* The authority must be non-empty (something between "://" and the path). */
static bool url_authority_nonempty(const char *url)
{
    const char *sep = strstr(url, "://");
    if (sep == NULL) {
        return false;
    }
    const char *authority = sep + 3;
    return *authority != '\0' && *authority != '/' && *authority != '?';
}

/* Loopback gate for the insecure (test-only) dispatcher — mirrors oauth2_outbound. */
static bool host_is_loopback(const char *url)
{
    static const char scheme[] = "http://";
    if (!has_prefix_ci(url, scheme)) {
        return false;
    }
    const char *authority = url + (sizeof(scheme) - 1);
    for (const char *p = authority; *p != '\0'; p++) {
        if (*p == '/' || *p == '?' || *p == '#') {
            break;
        }
        if (*p == '@') {
            return false;
        }
    }
    static const char *const prefixes[] = {
        "http://127.0.0.1", "http://[::1]", "http://localhost",
    };
    for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); i++) {
        if (has_prefix_ci(url, prefixes[i])) {
            char next = url[strlen(prefixes[i])];
            if (next == '\0' || next == ':' || next == '/') {
                return true;
            }
        }
    }
    return false;
}

/*
 * The shared callback-URL policy gate. `allow_insecure_loopback` is set ONLY by
 * the test-only insecure dispatcher. Returns true when the POST may proceed.
 * Production: https-only. Both modes reject userinfo / fragment / empty
 * authority / NULL.
 */
static bool callback_url_allowed(const char *url, bool allow_insecure_loopback)
{
    if (url == NULL || url[0] == '\0') {
        return false;
    }
    if (url_has_userinfo_or_fragment(url)) {
        return false;
    }
    if (!url_authority_nonempty(url)) {
        return false;
    }
    if (allow_insecure_loopback) {
        /* test-only: https OR an http loopback host. */
        return has_prefix_ci(url, "https://") || host_is_loopback(url);
    }
    return has_prefix_ci(url, "https://");
}

/*
 * Public reuse entry — the inbound Post handler validates a request callbackUri
 * with the production posture (no loopback relaxation). Thin pass-through to the
 * one gate above; no behavior change to the outbound dispatch path.
 */
bool nssf_notification_dispatcher_callback_url_allowed(const char *url)
{
    return callback_url_allowed(url, /*allow_insecure_loopback=*/false);
}

/* ──────────────────────────────────────────────────────────────────────────
 * correlation-id sanitization (B2) — header injection guard
 * ────────────────────────────────────────────────────────────────────────── */

/* Max correlation id we will propagate verbatim into a header value. */
#define NSSF_CORRELATION_ID_MAX 128

/*
 * A correlation id is header-safe iff it is non-empty, within the length cap,
 * and free of CR/LF and control chars (which would split / inject headers).
 * Printable ASCII only — the 3gpp-Sbi-Correlation-Info / traceparent values are
 * trace correlators, not arbitrary text.
 */
static bool correlation_id_is_safe(const char *id)
{
    if (id == NULL) {
        return false;
    }
    size_t n = 0;
    for (const char *p = id; *p != '\0'; p++, n++) {
        unsigned char ch = (unsigned char)*p;
        if (ch < 0x20 || ch == 0x7f) {
            return false; /* control char (incl. CR/LF/TAB) — reject. */
        }
        if (n + 1 > NSSF_CORRELATION_ID_MAX) {
            return false; /* over-length. */
        }
    }
    return n > 0;
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
 * string. Returns 0 on success (fields owned by the caller), -1 on a corrupt /
 * unparseable envelope OR a MISSING/empty body OR a body that is not strict-
 * valid JSON (F7): the dispatcher MUST never emit a null-body POST nor POST an
 * invalid-JSON body as application/json, so a row that cannot yield a non-empty
 * strict-valid-JSON body is reported as an error and the caller quarantines
 * (terminally drops) it. On -1 all out fields are freed and zeroed.
 */
static int envelope_split(const char *envelope_json, char **out_callback,
                          char **out_correlation, char **out_body)
{
    *out_callback = NULL;
    *out_correlation = NULL;
    *out_body = NULL;
    if (envelope_json == NULL) {
        return -1;
    }
    cJSON *env = cJSON_Parse(envelope_json);
    if (env == NULL) {
        return -1; /* corrupt/unparseable envelope. */
    }
    *out_callback = envelope_get_string(env, "callbackUri");
    *out_correlation = envelope_get_string(env, "correlationId");

    const cJSON *body = cJSON_GetObjectItemCaseSensitive(env, "body");
    if (cJSON_IsString(body) && body->valuestring != NULL) {
        *out_body = strdup(body->valuestring);
    } else if (body != NULL && !cJSON_IsNull(body)) {
        *out_body = cJSON_PrintUnformatted(body);
    }
    cJSON_Delete(env);

    /* A non-empty body is mandatory — never POST a null body (F7). */
    if (*out_body == NULL || (*out_body)[0] == '\0') {
        free(*out_callback);
        free(*out_correlation);
        free(*out_body);
        *out_callback = NULL;
        *out_correlation = NULL;
        *out_body = NULL;
        return -1;
    }

    /*
     * F7 follow-up — strict-valid-JSON body gate. The string branch above can
     * carry a non-empty but INVALID-JSON body (envelope_build stores an
     * unparseable payload_json verbatim as a string). The dispatch path POSTs
     * *out_body with Content-Type: application/json, so an invalid-JSON string
     * would be sent as application/json. Treat that as an un-deliverable poison
     * row exactly like the null/empty case — one chokepoint for all "cannot
     * yield a deliverable body" rejects. WHY here: this is the single pre-POST
     * point where F7 quarantine already lives. (The JSON sub-tree branch
     * re-serializes a parsed tree, so it is always valid JSON; only the string
     * branch can carry invalid JSON.)
     */
    cJSON *body_check = cJSON_Parse(*out_body);
    if (body_check == NULL) {
        /* WHY generic message — never echo body bytes / callback_uri / token /
         * correlation id (no body/secret logging). */
        fprintf(stderr, "nssf_dispatcher: F7 quarantine: body is not valid JSON.\n");
        free(*out_callback);
        free(*out_correlation);
        free(*out_body);
        *out_callback = NULL;
        *out_correlation = NULL;
        *out_body = NULL;
        return -1;
    }
    cJSON_Delete(body_check);
    return 0;
}

/* ── libpq backend ─────────────────────────────────────────────────────── */

/*
 * The PG store is SINGLE-WORKER / NOT thread-safe (F4). Ownership unit = one
 * store + one PGconn per worker. We take NO mutex here (unlike the in-memory
 * seam) — instead a debug owner-thread guard catches a contract violation (two
 * threads driving the SAME store) early in dev. WHY a guard not a lock: a lock
 * would mask the real bug (sharing one conn/transaction across workers) and
 * still be wrong, since FOR UPDATE SKIP LOCKED on one connection cannot protect
 * an interleaved second caller's in-flight transaction.
 */
typedef struct {
    nssf_retry_store_t base;
    PGconn *conn;
    pthread_t owner;     /* first thread to touch the store. */
    bool owner_set;
} pg_store_t;

/*
 * Assert single-worker ownership in debug builds. Records the first caller's
 * thread and aborts if a different thread ever drives the same store. No-op
 * under NDEBUG (zero release overhead) — the contract is documented in the
 * header for release callers.
 */
static void pg_assert_owner(pg_store_t *self)
{
#ifndef NDEBUG
    pthread_t me = pthread_self();
    if (!self->owner_set) {
        self->owner = me;
        self->owner_set = true;
    } else if (!pthread_equal(self->owner, me)) {
        fprintf(stderr,
                "nssf_retry_store(pg): single-worker contract violated — "
                "a second thread drove the same store (one store/conn per "
                "worker; see notification_dispatcher.h F4).\n");
        abort();
    }
#else
    (void)self;
#endif
}

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
    pg_assert_owner(self);
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
    pg_assert_owner(self);
    memset(out, 0, sizeof(*out));

    if (pg_cmd(self->conn, "BEGIN") != 0) {
        return -1;
    }
    /* Claim = next DUE pending row only. dead_letter rows are structurally
     * unclaimable (status filter). */
    const char *sql =
        "SELECT id, subscription_id, payload, attempt_count "
        "FROM retry_queue "
        "WHERE status = 'pending' AND next_attempt_at <= NOW() "
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

    /* Transient (OOM) read failure — leave the row, ROLLBACK and retry later. */
    if (env == NULL || out->id == NULL || out->subscription_id == NULL) {
        free(env);
        nssf_retry_item_clear(out);
        (void)pg_cmd(self->conn, "ROLLBACK");
        return -1;
    }

    /*
     * F7 quarantine — a corrupt/unparseable envelope or a missing/empty body can
     * never produce a deliverable POST. DELETE the poison row inside the open
     * locked transaction (and COMMIT) rather than ROLLBACK, so it does not loop
     * forever as a poison row. Report 0 (no usable row this claim).
     */
    if (envelope_split(env, &out->callback_uri, &out->correlation_id,
                       &out->payload_json) != 0) {
        fprintf(stderr,
                "nssf_dispatcher: quarantining poison retry_queue row id=%s "
                "(corrupt envelope or missing body).\n", out->id);
        const char *params[1] = {out->id};
        const char *del = "DELETE FROM retry_queue WHERE id = $1::bigint";
        PGresult *dr = PQexecParams(self->conn, del, 1, NULL, params, NULL, NULL, 0);
        int del_ok = (PQresultStatus(dr) == PGRES_COMMAND_OK);
        PQclear(dr);
        (void)pg_cmd(self->conn, del_ok ? "COMMIT" : "ROLLBACK");
        free(env);
        nssf_retry_item_clear(out);
        return del_ok ? 0 : -1;
    }
    free(env);
    /* Transaction stays OPEN — the row is held under FOR UPDATE until the
     * dispatcher calls complete()/requeue() (which COMMIT). */
    return 1;
}

static int pg_complete(nssf_retry_store_t *s, const char *id)
{
    pg_store_t *self = (pg_store_t *)s;
    pg_assert_owner(self);
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
    pg_assert_owner(self);
    /*
     * Phase 4 — re-arm with backoff computed SQL-side from the row's CURRENT
     * attempt_count, then bump attempt_count, keep status='pending'. The schedule
     * mirrors nssf_retry_backoff_delay_ms exactly:
     *   capped = LEAST(base_ms * 2^attempt_count, max_ms)   (equal-jitter floor)
     *   delay  = capped + random()*(capped/2)               (equal jitter, [0,half))
     * base/max are positional params (never concatenated). random() is Postgres's
     * jitter source — trace-grade, not crypto (M6). next_attempt_at moves into the
     * future so the row is not immediately re-claimable.
     */
    char basebuf[32];
    char maxbuf[32];
    snprintf(basebuf, sizeof(basebuf), "%ld", NSSF_DISPATCH_BACKOFF_BASE_MS);
    snprintf(maxbuf, sizeof(maxbuf), "%ld", NSSF_DISPATCH_BACKOFF_MAX_MS);
    const char *params[3] = {id, basebuf, maxbuf};
    const char *sql =
        "WITH b AS ("
        "  SELECT LEAST($2::double precision * power(2, attempt_count),"
        "               $3::double precision) AS capped"
        "  FROM retry_queue WHERE id = $1::bigint"
        ") "
        "UPDATE retry_queue SET "
        "  attempt_count = attempt_count + 1, "
        "  next_attempt_at = NOW() + "
        "    (((SELECT capped FROM b) + random() * ((SELECT capped FROM b) / 2.0))"
        "     * INTERVAL '1 millisecond') "
        "WHERE id = $1::bigint AND status = 'pending'";
    PGresult *res = PQexecParams(self->conn, sql, 3, NULL, params, NULL, NULL, 0);
    int ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    (void)pg_cmd(self->conn, ok ? "COMMIT" : "ROLLBACK");
    return ok ? 0 : -1;
}

/* Terminal failure — set status='dead_letter' and RETAIN the row (audit). */
static int pg_dead_letter(nssf_retry_store_t *s, const char *id)
{
    pg_store_t *self = (pg_store_t *)s;
    pg_assert_owner(self);
    const char *params[1] = {id};
    const char *sql =
        "UPDATE retry_queue SET status = 'dead_letter' WHERE id = $1::bigint";
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
    .dead_letter = pg_dead_letter,
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

/* In-memory mirror of retry_queue.status (enum-by-convention in the schema). */
typedef enum {
    MEM_STATUS_PENDING = 0,
    MEM_STATUS_DEAD_LETTER = 1,
} mem_status_e;

typedef struct {
    long long id;
    char *subscription_id;
    char *envelope; /* same envelope JSON the PG backend stores in `payload`. */
    int attempt_count;
    bool claimed;      /* mirrors FOR UPDATE — a claimed row is hidden from dequeue. */
    mem_status_e status; /* mirrors the retry_queue.status column. */
    long long due_at_ms; /* next_attempt_at as monotonic wall ms; claim needs due. */
} mem_row_t;

/* Monotonic-ish wall clock in milliseconds for the in-memory due-time mirror. */
static long long mem_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

typedef struct {
    nssf_retry_store_t base;
    mem_row_t *rows;
    size_t count;
    size_t cap;
    long long next_id;
    pthread_mutex_t lock;
} mem_store_t;

static void mem_remove_at(mem_store_t *self, long idx); /* fwd — F7 quarantine. */

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
    row->status = MEM_STATUS_PENDING;
    row->due_at_ms = mem_now_ms(); /* due immediately, like next_attempt_at=NOW(). */
    pthread_mutex_unlock(&self->lock);
    return 0;
}

static int mem_dequeue(nssf_retry_store_t *s, nssf_retry_item_t *out)
{
    mem_store_t *self = (mem_store_t *)s;
    memset(out, 0, sizeof(*out));

    pthread_mutex_lock(&self->lock);
    /*
     * Claim = lowest-id row that is pending, unclaimed, AND due — mirrors the PG
     * claim `status='pending' AND next_attempt_at <= NOW()` ordered by
     * next_attempt_at ASC, id ASC. dead_letter rows are skipped (unclaimable), and
     * a re-armed row whose backoff has not elapsed (due_at_ms > now) is not yet
     * due. We order by due_at_ms then id to match the PG ORDER BY.
     */
    long long now_ms = mem_now_ms();
    long pick = -1;
    for (size_t i = 0; i < self->count; ++i) {
        mem_row_t *r = &self->rows[i];
        if (r->claimed || r->status != MEM_STATUS_PENDING || r->due_at_ms > now_ms) {
            continue;
        }
        if (pick < 0 ||
            r->due_at_ms < self->rows[pick].due_at_ms ||
            (r->due_at_ms == self->rows[pick].due_at_ms &&
             r->id < self->rows[pick].id)) {
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

    /* Transient (OOM) — leave the row claimable, report error and retry later. */
    if (out->id == NULL || out->subscription_id == NULL) {
        nssf_retry_item_clear(out);
        pthread_mutex_unlock(&self->lock);
        return -1;
    }

    /*
     * F7 quarantine — a corrupt envelope / missing body can never be delivered,
     * so DROP the poison row here rather than leaving it to loop forever. Report
     * 0 (no usable row this claim).
     */
    if (envelope_split(row->envelope, &out->callback_uri, &out->correlation_id,
                       &out->payload_json) != 0) {
        fprintf(stderr,
                "nssf_dispatcher: quarantining poison retry_queue row id=%s "
                "(corrupt envelope or missing body).\n", out->id);
        mem_remove_at(self, pick);
        nssf_retry_item_clear(out);
        pthread_mutex_unlock(&self->lock);
        return 0;
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
        /* Phase 4 — backoff computed from the row's CURRENT attempt_count (the
         * same input as the PG path), then bump attempt_count. Pushes due_at_ms
         * into the future so the row is NOT immediately re-claimable. status stays
         * pending. */
        long delay = nssf_retry_backoff_delay_ms(self->rows[idx].attempt_count,
                                                 NSSF_DISPATCH_BACKOFF_BASE_MS,
                                                 NSSF_DISPATCH_BACKOFF_MAX_MS,
                                                 jitter_unit());
        self->rows[idx].attempt_count += 1;
        self->rows[idx].claimed = false;
        self->rows[idx].due_at_ms = mem_now_ms() + delay;
    }
    pthread_mutex_unlock(&self->lock);
    return 0;
}

/* Terminal failure — mark dead_letter and RETAIN (row stays for audit, never
 * claimed: claim filters status==pending). Also un-claim so it is not stuck. */
static int mem_dead_letter(nssf_retry_store_t *s, const char *id)
{
    mem_store_t *self = (mem_store_t *)s;
    long long key = atoll(id);
    pthread_mutex_lock(&self->lock);
    long idx = mem_index_of(self, key);
    if (idx >= 0) {
        self->rows[idx].status = MEM_STATUS_DEAD_LETTER;
        self->rows[idx].claimed = false;
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
    .dead_letter = mem_dead_letter,
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
    /*
     * Defense-in-depth (ADR-0004 M4/M2) — enqueue-time STRUCTURAL callback_uri
     * hard-reject, layered IN FRONT of the dispatch-time callback_url_allowed gate
     * (which STAYS as the second layer). A structurally un-dispatchable callback_uri
     * (NULL/empty, userinfo('@'), fragment('#'), empty-authority, or a scheme no
     * dispatcher ctor could ever emit) must never enter the retry queue, so it is
     * rejected here BEFORE any row is written.
     *
     * WHY permissive mode (allow_insecure_loopback=true) and NOT https-only: the
     * store is ctor-agnostic — it cannot know whether its rows feed a production
     * https-only dispatcher (_new) or a test loopback dispatcher (_new_insecure).
     * Enforcing https-only here would wrongly reject the legitimate test
     * loopback-http path. So we reject ONLY what NO ctor could dispatch, while still
     * admitting both the production https path AND the test http-loopback path. The
     * https-only decision stays a dispatch-time / ctor concern (the second layer).
     * This only makes enqueue STRICTER (bad URL → no row, -1); it weakens no
     * existing reject. Pure synchronous validation — no worker / poll / loop (G-08).
     */
    if (!callback_url_allowed(callback_uri, /*allow_insecure_loopback=*/true)) {
        fprintf(stderr,
                "nssf_dispatcher: callback_uri rejected at enqueue (structural) — "
                "no retry_queue row written.\n");
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

int nssf_retry_store_dead_letter(nssf_retry_store_t *store, const char *id)
{
    if (store == NULL || id == NULL) {
        return -1;
    }
    return store->vt->dead_letter(store, id);
}

/* ──────────────────────────────────────────────────────────────────────────
 * dispatcher
 * ────────────────────────────────────────────────────────────────────────── */

struct nssf_notification_dispatcher {
    nssf_retry_store_t *store;      /* borrowed. */
    nssf_oauth2_outbound_t *oauth;  /* borrowed; NULL → never attach a bearer. */
    nssf_notification_transport_fn transport;
    void *transport_ctx;
    bool allow_insecure_loopback;   /* test-only ctor — relaxes https-only gate. */
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
    /*
     * Phase 1~3 producer — NOT-READY path (B3). The committed change event carries
     * nfId / tai / after but NOT a subscription UUID nor a callbackUri: resolving
     * the changed nfId/tai to the AFFECTED subscriptions (and their callbackUri)
     * is the Phase-3 SubscriptionStore fan-out, which is not wired yet.
     *
     * WHY we do NOT enqueue here: writing the nfId into retry_queue.subscription_id
     * (a real UUID/FK column) is wrong — it fabricates a subscription that does not
     * exist and produces a poison row with a NULL callback that can never deliver.
     * A retry_queue row is enqueued ONLY when a real subscription UUID AND a
     * callbackUri are both present; that path is exercised today via the
     * nssf_retry_store_enqueue API directly (the in-memory test backend is the
     * slice deliverable for the enqueue-on-commit seam). Until the Phase-3 fan-out
     * lands, the availability-producer-to-store wiring stays behind this explicit
     * not-ready log path rather than silently dropping or poisoning the queue.
     */
    char *payload = event_to_payload(ev);
    fprintf(stderr,
            "nssf_dispatcher: change_publish not-ready — Phase-3 SubscriptionStore "
            "fan-out unavailable; no retry_queue row enqueued for nfId=%s%s.\n",
            ev->nf_id != NULL ? ev->nf_id : "(null)",
            payload != NULL ? "" : " (payload build failed)");
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
    /* Header line = prefix(27) + correlation id (caller-capped at
     * NSSF_CORRELATION_ID_MAX) + NUL — sized to never truncate a valid id. */
    char buf[64 + NSSF_CORRELATION_ID_MAX];
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

    /*
     * B1 belt-and-suspenders — cap the allowed protocol to HTTPS so even a
     * mis-validated URL cannot drive a plaintext/file/gopher request. The shared
     * callback_url_allowed() gate is the primary defence (runs before the token
     * attach); this is the libcurl-level backstop.
     *
     * libcurl is 7.81.0 here — CURLOPT_PROTOCOLS_STR is 7.85+ only and would
     * break the build, so the 7.81 path uses the (deprecated-but-present)
     * CURLOPT_PROTOCOLS bitmask. The _STR branch is compiled ONLY on 7.85+ for
     * forward-compat.
     */
#if LIBCURL_VERSION_NUM >= 0x075500 /* 7.85.0 */
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");
#else
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, (long)CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, (long)CURLPROTO_HTTPS);
#endif

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

static nssf_notification_dispatcher_t *dispatcher_new_with_policy(
    nssf_retry_store_t *store, nssf_oauth2_outbound_t *oauth,
    bool allow_insecure_loopback)
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
    disp->allow_insecure_loopback = allow_insecure_loopback;
    return disp;
}

nssf_notification_dispatcher_t *nssf_notification_dispatcher_new(
    nssf_retry_store_t *store, nssf_oauth2_outbound_t *oauth)
{
    /* Production — https-only callback gate enforced (no loopback relaxation). */
    return dispatcher_new_with_policy(store, oauth, /*allow_insecure_loopback=*/false);
}

nssf_notification_dispatcher_t *nssf_notification_dispatcher_new_insecure(
    nssf_retry_store_t *store, nssf_oauth2_outbound_t *oauth)
{
    /* TEST/DEV ONLY — relaxes the https-only gate to http loopback hosts. */
    return dispatcher_new_with_policy(store, oauth, /*allow_insecure_loopback=*/true);
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

/* 2xx is success. */
static bool is_success_status(long status)
{
    return status >= 200 && status < 300;
}

/*
 * Retriable on the HTTP status (transport-level failures are classified
 * separately, by the nonzero transport return). 5xx is a server-side transient;
 * 429 Too Many Requests is an explicit back-pressure signal — both re-arm with
 * backoff. WHY 429 here and not under is_terminal: a 429 is the server asking us
 * to slow down, which is exactly what backoff does.
 */
static bool is_retriable_status(long status)
{
    return (status >= 500 && status < 600) || status == 429;
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

    /*
     * B1 — outbound URL policy gate, run BEFORE build_headers / oauth2 bearer
     * attach and BEFORE the POST. A row whose callbackUri is NULL/empty, not
     * https:// (production), malformed, or carries userinfo('@') / fragment('#')
     * is dropped TERMINALLY: no token is attached and no POST is emitted. WHY
     * here and not just at libcurl: this closes the window where a bearer could
     * be attached to a plaintext / attacker-shifted URL before the transport's
     * own protocol cap would fire. classify terminal/forbidden (DROPPED).
     */
    if (!callback_url_allowed(item.callback_uri, disp->allow_insecure_loopback)) {
        fprintf(stderr,
                "nssf_dispatcher: rejecting row id=%s — callback URL fails policy "
                "(https-only / no userinfo / no fragment / well-formed). No token "
                "attached, no POST emitted.\n", item.id);
        (void)nssf_retry_store_complete(disp->store, item.id);
        nssf_retry_item_clear(&item);
        return NSSF_DISPATCH_DROPPED;
    }

    /*
     * B2 — header-injection guard. The effective correlation id (explicit inbound
     * > stored > freshly generated) is propagated into 3gpp-Sbi-Correlation-Info
     * and traceparent, so any CR/LF/control/over-length id is rejected and a safe
     * id is regenerated. Applies to BOTH the inbound-supplied and stored-row
     * sources (a freshly generated hex id is safe by construction). gen_correlation
     * is the fallback for absent OR unsafe ids.
     */
    char gen[33];
    const char *corr = inbound_correlation_id != NULL ? inbound_correlation_id
                       : item.correlation_id != NULL  ? item.correlation_id
                                                      : NULL;
    if (corr == NULL || !correlation_id_is_safe(corr)) {
        gen_correlation_hex(gen);
        corr = gen;
    }

    /*
     * Phase 4 fail-closed bounded retry — attempt the claimed row EXACTLY ONCE
     * this call (NO in-line immediate retry, G-08). The decision after the attempt
     * routes the row to one of three terminal/non-terminal store ops:
     *   - complete()     — terminal SUCCESS (2xx): delete the row.
     *   - requeue(delay) — retriable failure WITH budget remaining: re-arm with
     *                      backoff for a LATER call (status stays pending).
     *   - dead_letter()  — terminal failure (4xx≠429, OAuth2 ABORT) OR retriable
     *                      failure that EXHAUSTED the budget: retain for audit.
     */
    struct curl_slist *headers = NULL;
    int hb = build_headers(disp, corr, &headers);
    if (hb == 1) {
        /*
         * fail-closed (ADR-0004 M4 / Phase 4 ratified design) — no unauthenticated
         * POST is ever emitted. Per the ratified design an OAuth2 ABORT is TERMINAL:
         * dead_letter the row (do NOT re-arm into a hot retry against the NRF token
         * endpoint) and report FORBIDDEN. NO POST was emitted.
         */
        (void)nssf_retry_store_dead_letter(disp->store, item.id);
        nssf_retry_item_clear(&item);
        return NSSF_DISPATCH_FORBIDDEN;
    }
    if (hb < 0) {
        /* Header build OOM — transient, NOT a delivery decision. Re-arm with
         * backoff (no POST emitted) so the row is retried on a later call. The
         * store computes the backoff from the row's attempt_count. */
        (void)nssf_retry_store_requeue(disp->store, item.id);
        nssf_retry_item_clear(&item);
        return NSSF_DISPATCH_ERROR;
    }

    long status = 0;
    int trc = disp->transport(disp, item.callback_uri, item.payload_json,
                              headers, &status, disp->transport_ctx);
    curl_slist_free_all(headers);

    /* SUCCESS (2xx) — terminal, delete the row. */
    if (trc == 0 && is_success_status(status)) {
        (void)nssf_retry_store_complete(disp->store, item.id);
        nssf_retry_item_clear(&item);
        return NSSF_DISPATCH_SENT;
    }

    /*
     * RETRIABLE — transport nonzero (timeout/connection/TLS handshake) OR an HTTP
     * 5xx OR 429. item.attempt_count is the number of attempts ALREADY made before
     * this one; this call made one more, so attempts-so-far = attempt_count + 1. If
     * that reaches the ceiling, the budget is spent → dead_letter; otherwise re-arm
     * with backoff for a LATER call (status stays pending).
     */
    bool retriable = (trc != 0) || is_retriable_status(status);
    if (retriable) {
        if (item.attempt_count + 1 >= NSSF_DISPATCH_MAX_ATTEMPTS) {
            (void)nssf_retry_store_dead_letter(disp->store, item.id);
            nssf_retry_item_clear(&item);
            return NSSF_DISPATCH_DROPPED;
        }
        /* Re-arm with backoff — the store pushes next_attempt_at by
         * backoff(attempt_count) so this row is claimed again only on a LATER
         * call once the backoff elapses (no in-line retry, G-08). */
        (void)nssf_retry_store_requeue(disp->store, item.id);
        nssf_retry_item_clear(&item);
        return NSSF_DISPATCH_DROPPED;
    }

    /*
     * TERMINAL failure — a 4xx other than 429, or any other non-success /
     * non-retriable status (e.g. an unexpected 3xx/1xx the transport surfaced).
     * Dead_letter and retain for audit; never re-claimed. WHY treat the residual
     * bucket as terminal: a status we cannot classify as success or retriable will
     * not be fixed by waiting, and re-arming it would loop forever. (429 is NOT
     * here — it is folded into the retriable bucket above as a back-pressure
     * signal.)
     */
    (void)nssf_retry_store_dead_letter(disp->store, item.id);
    nssf_retry_item_clear(&item);
    return NSSF_DISPATCH_DROPPED;
}
