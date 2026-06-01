/*
 * oauth2_jwks.c — inbound OAuth2 bearer validation for NSSF (ADR-0004 M3).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten runtime module.
 *
 * libjwt v3 owns all JWT/JWS/JWKS crypto (M6). The JWKS fetch verifies Host +
 * Peer of the TLS connection to the issuer (M2 — verification is never
 * disabled). jwks_url and TTL are operator config (M7).
 */

#define _POSIX_C_SOURCE 200809L

#include "oauth2_jwks.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ctype.h>

#include <jwt.h>

/* libjwt jwks_load_fromurl verify level: 2 = verify Host and Peer (M2). */
#define NSSF_JWKS_TLS_VERIFY_HOST_AND_PEER 2

struct nssf_jwks_cache {
    char *jwks_url;            /* owned copy; NULL → offline/seeded only. */
    uint32_t ttl_seconds;
    jwk_set_t *jwk_set;        /* current keyring; NULL until first load. */
    time_t last_refresh;       /* epoch of last successful (re)load. */
    bool refresh_enabled;      /* false for json-seeded offline caches. */
    pthread_mutex_t lock;
};

/* Callback context: carries the keyring in, and the captured scope out. */
typedef struct {
    jwk_set_t *jwk_set;
    char *scope_out;           /* malloc'd copy of "scope" claim, or NULL. */
} validate_ctx_t;

static uint32_t effective_ttl(uint32_t ttl_seconds)
{
    /* Config default per contract. */
    return ttl_seconds != 0 ? ttl_seconds : 3600u;
}

static nssf_jwks_cache_t *cache_alloc(uint32_t ttl_seconds)
{
    nssf_jwks_cache_t *cache = calloc(1, sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    if (pthread_mutex_init(&cache->lock, NULL) != 0) {
        free(cache);
        return NULL;
    }
    cache->ttl_seconds = effective_ttl(ttl_seconds);
    cache->jwk_set = NULL;
    cache->last_refresh = 0;
    cache->refresh_enabled = false;
    cache->jwks_url = NULL;
    return cache;
}

/* Case-insensitive prefix test on the scheme portion of url. */
static bool has_prefix_ci(const char *url, const char *prefix)
{
    size_t i = 0;
    for (; prefix[i] != '\0'; i++) {
        if (url[i] == '\0' ||
            tolower((unsigned char)url[i]) != tolower((unsigned char)prefix[i])) {
            return false;
        }
    }
    return true;
}

/*
 * TEST/DEV-only loopback gate. After the http:// scheme, the authority must be a
 * loopback host (127.0.0.1, [::1], localhost) — terminated by ':' (port), '/'
 * (path), or end-of-string. Keeps even the insecure path unusable against a real
 * remote.
 *
 * WHY reject userinfo: an RFC-3986 authority is [ userinfo "@" ] host [ ":" port ].
 * A '@' makes everything before it userinfo and the REAL host the part after it,
 * so "http://127.0.0.1@evil.com/jwks" resolves to evil.com. A prefix-only loopback
 * check would be spoofed by such userinfo. We therefore isolate the authority
 * (between "://" and the first '/', '?', or '#') and reject any '@' in it before
 * trusting the host bytes.
 */
static bool host_is_loopback(const char *url)
{
    static const char scheme[] = "http://";
    if (!has_prefix_ci(url, scheme)) {
        return false;
    }
    /* Authority = bytes after "://" up to the first '/', '?', '#', or '\0'. */
    const char *authority = url + (sizeof(scheme) - 1);
    for (const char *p = authority; *p != '\0'; p++) {
        if (*p == '/' || *p == '?' || *p == '#') {
            break;
        }
        if (*p == '@') {
            /* userinfo present → real host is after '@'; not a trusted loopback. */
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
 * Shared URL-bound cache body. allow_insecure_loopback selects the scheme
 * policy: production requires https://; the test-only path accepts http:// but
 * only to a loopback host. Anything outside the selected policy → NULL.
 */
static nssf_jwks_cache_t *create_with_scheme_policy(const char *jwks_url,
                                                    uint32_t ttl_seconds,
                                                    bool allow_insecure_loopback)
{
    if (jwks_url == NULL || jwks_url[0] == '\0') {
        return NULL;
    }
    if (allow_insecure_loopback) {
        if (!host_is_loopback(jwks_url)) {
            return NULL;
        }
    } else if (!has_prefix_ci(jwks_url, "https://")) {
        return NULL;
    }

    nssf_jwks_cache_t *cache = cache_alloc(ttl_seconds);
    if (cache == NULL) {
        return NULL;
    }
    cache->jwks_url = strdup(jwks_url);
    if (cache->jwks_url == NULL) {
        pthread_mutex_destroy(&cache->lock);
        free(cache);
        return NULL;
    }
    cache->refresh_enabled = true;
    return cache;
}

nssf_jwks_cache_t *nssf_jwks_cache_create(const char *jwks_url,
                                          uint32_t ttl_seconds)
{
    return create_with_scheme_policy(jwks_url, ttl_seconds, false);
}

nssf_jwks_cache_t *nssf_jwks_cache_create_insecure(const char *jwks_url,
                                                   uint32_t ttl_seconds)
{
    return create_with_scheme_policy(jwks_url, ttl_seconds, true);
}

nssf_jwks_cache_t *nssf_jwks_cache_create_from_json(const char *jwks_json,
                                                    uint32_t ttl_seconds)
{
    if (jwks_json == NULL) {
        return NULL;
    }
    jwk_set_t *set = jwks_create(jwks_json);
    if (set == NULL || jwks_error(set)) {
        if (set != NULL) {
            jwks_free(set);
        }
        return NULL;
    }
    nssf_jwks_cache_t *cache = cache_alloc(ttl_seconds);
    if (cache == NULL) {
        jwks_free(set);
        return NULL;
    }
    cache->jwk_set = set;
    cache->last_refresh = time(NULL);
    /* Seeded keyring has no URL to refresh from. */
    cache->refresh_enabled = false;
    return cache;
}

/* Caller must hold cache->lock. Replaces keyring on a clean fetch only. */
static int refresh_locked(nssf_jwks_cache_t *cache)
{
    if (!cache->refresh_enabled || cache->jwks_url == NULL) {
        /* Offline/seeded cache: nothing to fetch. Treat existing set as fresh. */
        return cache->jwk_set != NULL ? 0 : -1;
    }

    jwk_set_t *fresh = jwks_create_fromurl(cache->jwks_url,
                                           NSSF_JWKS_TLS_VERIFY_HOST_AND_PEER);
    if (fresh == NULL || jwks_error(fresh)) {
        if (fresh != NULL) {
            jwks_free(fresh);
        }
        return -1;
    }

    if (cache->jwk_set != NULL) {
        jwks_free(cache->jwk_set);
    }
    cache->jwk_set = fresh;
    cache->last_refresh = time(NULL);
    return 0;
}

int nssf_jwks_cache_refresh(nssf_jwks_cache_t *cache)
{
    if (cache == NULL) {
        return -1;
    }
    pthread_mutex_lock(&cache->lock);
    int rc = refresh_locked(cache);
    pthread_mutex_unlock(&cache->lock);
    return rc;
}

/* Caller must hold cache->lock. */
static int ensure_fresh_locked(nssf_jwks_cache_t *cache)
{
    if (cache->jwk_set == NULL) {
        return refresh_locked(cache);
    }
    if (!cache->refresh_enabled) {
        return 0; /* seeded keyring never expires. */
    }
    time_t now = time(NULL);
    double age = difftime(now, cache->last_refresh);
    if (age >= (double)cache->ttl_seconds) {
        return refresh_locked(cache);
    }
    return 0;
}

int nssf_jwks_cache_ensure_fresh(nssf_jwks_cache_t *cache)
{
    if (cache == NULL) {
        return -1;
    }
    pthread_mutex_lock(&cache->lock);
    int rc = ensure_fresh_locked(cache);
    pthread_mutex_unlock(&cache->lock);
    return rc;
}

/*
 * libjwt verification callback. Runs after the header/claims are parsed but
 * before signature verification — exactly where we resolve the signing key by
 * kid and capture the scope claim. Returning nonzero aborts verification.
 */
static int verify_cb(jwt_t *jwt, jwt_config_t *config)
{
    validate_ctx_t *vctx = (validate_ctx_t *)config->ctx;
    if (vctx == NULL || vctx->jwk_set == NULL) {
        return -1;
    }

    /* Resolve the signing key strictly by the token's kid. */
    jwt_value_t hv;
    jwt_set_GET_STR(&hv, "kid");
    if (jwt_header_get(jwt, &hv) != JWT_VALUE_ERR_NONE || hv.str_val == NULL) {
        return -1;
    }

    jwk_item_t *key = jwks_find_bykid(vctx->jwk_set, hv.str_val);
    if (key == NULL) {
        return -1;
    }

    /* Bind alg to the JWK's own alg — reject token-controlled alg confusion. */
    jwt_alg_t key_alg = jwks_item_alg(key);
    if (key_alg == JWT_ALG_NONE || key_alg == JWT_ALG_INVAL) {
        return -1;
    }
    config->key = key;
    config->alg = key_alg;

    /* Capture the scope claim (string) for the caller; absence is not fatal. */
    jwt_value_t sv;
    jwt_set_GET_STR(&sv, "scope");
    if (jwt_claim_get(jwt, &sv) == JWT_VALUE_ERR_NONE && sv.str_val != NULL) {
        vctx->scope_out = strdup(sv.str_val);
    }

    return 0;
}

int nssf_oauth2_validate_bearer(nssf_jwks_cache_t *cache,
                                const char *bearer_token,
                                char **out_scope_claim,
                                char *errbuf, size_t errlen)
{
    if (out_scope_claim != NULL) {
        *out_scope_claim = NULL;
    }
    if (cache == NULL || bearer_token == NULL || bearer_token[0] == '\0') {
        if (errbuf != NULL && errlen > 0) {
            snprintf(errbuf, errlen, "invalid arguments");
        }
        return -1;
    }

    pthread_mutex_lock(&cache->lock);
    if (ensure_fresh_locked(cache) != 0 || cache->jwk_set == NULL) {
        pthread_mutex_unlock(&cache->lock);
        if (errbuf != NULL && errlen > 0) {
            snprintf(errbuf, errlen, "jwks unavailable");
        }
        return -1;
    }

    /*
     * Hold the lock across verify so the keyring cannot be freed/replaced by a
     * concurrent refresh while the JWK pointer is in use.
     */
    validate_ctx_t vctx = {.jwk_set = cache->jwk_set, .scope_out = NULL};

    jwt_checker_t *checker = jwt_checker_new();
    if (checker == NULL) {
        pthread_mutex_unlock(&cache->lock);
        if (errbuf != NULL && errlen > 0) {
            snprintf(errbuf, errlen, "checker alloc failed");
        }
        return -1;
    }

    int rc = -1;
    if (jwt_checker_setcb(checker, verify_cb, &vctx) != 0) {
        if (errbuf != NULL && errlen > 0) {
            snprintf(errbuf, errlen, "checker setcb failed");
        }
    } else if (jwt_checker_verify(checker, bearer_token) != 0) {
        if (errbuf != NULL && errlen > 0) {
            const char *msg = jwt_checker_error_msg(checker);
            snprintf(errbuf, errlen, "token verification failed: %s",
                     msg != NULL ? msg : "unknown");
        }
        free(vctx.scope_out);
        vctx.scope_out = NULL;
    } else {
        rc = 0;
    }

    jwt_checker_free(checker);
    pthread_mutex_unlock(&cache->lock);

    if (rc == 0 && out_scope_claim != NULL) {
        *out_scope_claim = vctx.scope_out;
    } else {
        free(vctx.scope_out);
    }
    return rc;
}

void nssf_jwks_cache_free(nssf_jwks_cache_t *cache)
{
    if (cache == NULL) {
        return;
    }
    if (cache->jwk_set != NULL) {
        jwks_free(cache->jwk_set);
    }
    free(cache->jwks_url);
    pthread_mutex_destroy(&cache->lock);
    free(cache);
}
