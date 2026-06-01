/*
 * oauth2_jwks.h — inbound OAuth2 bearer validation for NSSF (ADR-0004 M3).
 *
 * libjwt v3 JWKS fetch (jwks_load_fromurl over HTTPS with peer verification) +
 * TTL cache + signature/exp verification of inbound access tokens. No crypto is
 * hand-rolled (M6). The JWKS URL and TTL come from operator config (M7).
 *
 * Scope: inbound bearer validation (M3). Outbound token acquisition/attach (M4)
 * is a separate slice — not implemented here.
 */

#ifndef NSSF_RUNTIME_OAUTH2_JWKS_H
#define NSSF_RUNTIME_OAUTH2_JWKS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Operator-supplied OAuth2 profile. String/array lifetimes are owned by the
 * caller; this module copies only the JWKS URL into the cache.
 */
typedef struct nssf_oauth2_config {
    const char *jwks_url;             /* required. HTTPS endpoint. */
    uint32_t jwks_cache_ttl_seconds;  /* default 3600. */
    const char *const *scopes;        /* required NSSF scopes. */
    size_t scopes_len;                /* expected 2. */
} nssf_oauth2_config_t;

typedef struct nssf_jwks_cache nssf_jwks_cache_t;

/*
 * Create a thread-safe JWKS cache bound to jwks_url with the given TTL. The
 * keyring is initially empty — first validate (or explicit refresh) populates
 * it. ttl_seconds == 0 → default 3600. Returns NULL on OOM / NULL url.
 *
 * Production JWKS URLs MUST be https:// — http:// is rejected to prevent MITM
 * keyset poisoning (ADR-0004 M3/M7). Any non-https scheme (or no scheme) → NULL.
 */
nssf_jwks_cache_t *nssf_jwks_cache_create(const char *jwks_url,
                                          uint32_t ttl_seconds);

/*
 * TEST/DEV ONLY. Bypasses the https:// scheme enforcement of
 * nssf_jwks_cache_create. MUST NOT be used in production — exists solely so
 * integration tests can point at a 127.0.0.1 loopback mock JWKS server over
 * http. Not a production code path.
 *
 * Defense-in-depth: the URL host must be a loopback host (127.0.0.1, [::1], or
 * localhost); any other host → NULL. ttl_seconds == 0 → default 3600.
 */
nssf_jwks_cache_t *nssf_jwks_cache_create_insecure(const char *jwks_url,
                                                   uint32_t ttl_seconds);

/*
 * Offline helper: build a cache whose keyring is seeded from a JWKS JSON string
 * (no network). jwks_url may be NULL; in that case network refresh is disabled
 * and the seeded keyring is used as-is. Returns NULL on parse failure / OOM.
 */
nssf_jwks_cache_t *nssf_jwks_cache_create_from_json(const char *jwks_json,
                                                    uint32_t ttl_seconds);

/* Force a fetch from jwks_url and replace the keyring. 0 ok / nonzero error. */
int nssf_jwks_cache_refresh(nssf_jwks_cache_t *cache);

/* Refresh iff (now - last_refresh) >= ttl, or keyring empty. 0 ok / nonzero. */
int nssf_jwks_cache_ensure_fresh(nssf_jwks_cache_t *cache);

/*
 * Validate an inbound bearer access token: ensure_fresh, parse the kid from the
 * token header, find the matching JWK, verify signature + exp via libjwt v3.
 * On success returns 0 and, when out_scope_claim != NULL, sets *out_scope_claim
 * to a malloc'd copy of the token's "scope" claim (caller frees; NULL if the
 * token carries no scope). Returns nonzero on any validation failure; the
 * signature is never bypassed.
 */
int nssf_oauth2_validate_bearer(nssf_jwks_cache_t *cache,
                                const char *bearer_token,
                                char **out_scope_claim,
                                char *errbuf, size_t errlen);

void nssf_jwks_cache_free(nssf_jwks_cache_t *cache);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_RUNTIME_OAUTH2_JWKS_H */
