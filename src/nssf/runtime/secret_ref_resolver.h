/*
 * secret_ref_resolver.h — deploy-time secret_ref resolver seam for NSSF.
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten runtime module.
 *
 * oauth2_outbound.h reserves a `secret_ref` field that carries a REFERENCE
 * handle (e.g. "env:NRF_CLIENT_SECRET" or "file:/run/secrets/nrf"), never the
 * plaintext secret value. This module is the resolver that turns such a
 * reference into the actual secret bytes at deploy time.
 *
 * engineering-design.md configuration_management.secret_strategy — "secret 미저장,
 * ref 만 보유, 실제 secret/key 는 배포(env/vault/file) 외부 주입." 본 resolver 는 그
 * 전략의 env:/file: arm 이다. override order defaults<file<env 의 file/env 입력원.
 *
 * Supported secret_ref formats (no other scheme is accepted):
 *
 *   "env:NAME"   — resolve from environment variable NAME (taken verbatim).
 *   "file:/path" — read the secret from a file at /path.
 *
 * Security invariants (ADR-0004 secret hygiene / M7):
 *
 *   - 값 미보유 (no-retain). The resolver caches/stores nothing. Each call
 *     resolves on demand and hands an OWNED buffer to the caller. The caller
 *     MUST release it with nssf_secret_ref_scrub_free() — which scrubs (memset
 *     0) the bytes before free.
 *   - 값 로그 금지 (no-log-value). The resolved secret VALUE is NEVER logged,
 *     printed, placed in an error message, or returned in any diagnostic. The
 *     scheme ("env"/"file") and the variable NAME / file PATH MAY appear in
 *     operator diagnostics; the resolved bytes never do.
 *   - fail-fast. Any error (NULL/empty/bad format, missing env, missing or
 *     unreadable file, oversize) returns a non-OK status with *out_secret=NULL
 *     and *out_len=0. There is no partial result and no silent empty/default
 *     substitution — the caller fails closed on a non-OK status.
 *
 * The resolver is purely synchronous — no thread, timer, poll, or retry loop.
 */

#ifndef NSSF_RUNTIME_SECRET_REF_RESOLVER_H
#define NSSF_RUNTIME_SECRET_REF_RESOLVER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Maximum resolved secret size, in bytes (excludes the resolver-added NUL
 * terminator). A file larger than this resolves to NSSF_SECRET_REF_TOO_LARGE
 * rather than being read into memory. Env values are bounded by the same cap.
 * 64 KiB is generously above any real client secret / token while bounding the
 * read of an unexpectedly huge file.
 */
#define NSSF_SECRET_REF_MAX_BYTES (64u * 1024u)

typedef enum nssf_secret_ref_status {
    NSSF_SECRET_REF_OK = 0,      /* resolved; *out_secret owned by caller. */
    NSSF_SECRET_REF_BAD_FORMAT,  /* NULL/empty ref, unknown scheme, or empty operand. */
    NSSF_SECRET_REF_NOT_FOUND,   /* env var unset, or file does not exist. */
    NSSF_SECRET_REF_IO_ERROR,    /* file open/stat/read failure (other than not-found). */
    NSSF_SECRET_REF_TOO_LARGE,   /* resolved value exceeds NSSF_SECRET_REF_MAX_BYTES. */
} nssf_secret_ref_status_t;

/*
 * Resolve a "env:NAME" or "file:/path" reference into the actual secret bytes.
 *
 * On NSSF_SECRET_REF_OK:
 *   - *out_secret points to a heap buffer OWNED by the caller, containing the
 *     resolved bytes followed by a NUL terminator (so it is safe to treat as a
 *     C string when the secret is text). The NUL is NOT counted in *out_len.
 *   - *out_len is the secret byte length (may be 0 only for an env var that is
 *     explicitly set to the empty string; a file that resolves to 0 bytes after
 *     newline handling is also length 0). The terminator is always present.
 *   - The caller MUST release the buffer with nssf_secret_ref_scrub_free(
 *     *out_secret, *out_len) so the plaintext is scrubbed before free.
 *
 * On any non-OK status: *out_secret is set to NULL and *out_len to 0. No
 * partial buffer is ever returned, and any temporary buffer is scrubbed+freed
 * internally before returning. fail-fast — the caller fails closed.
 *
 * file: trailing-newline handling — at most ONE trailing '\n' is stripped, plus
 * a single immediately-preceding '\r' (so "echo secret > file" and CRLF-written
 * files both yield the bare secret). No further trimming is done — a secret may
 * legitimately contain internal newlines, leading/trailing spaces, etc., so only
 * the single terminating line break is removed. env: values are taken verbatim
 * with no trimming.
 *
 * The resolved VALUE is never logged or placed in any diagnostic by this call.
 *
 * out_secret and out_len must be non-NULL; passing NULL for either returns
 * NSSF_SECRET_REF_BAD_FORMAT.
 */
nssf_secret_ref_status_t nssf_secret_ref_resolve(const char *secret_ref,
                                                 char **out_secret,
                                                 size_t *out_len);

/*
 * Scrub then free a buffer returned by nssf_secret_ref_resolve: overwrite `len`
 * bytes with zero, then free. NULL-safe (a NULL secret is a no-op). The caller
 * MUST use this rather than a bare free() so the plaintext does not linger in
 * freed heap memory.
 */
void nssf_secret_ref_scrub_free(char *secret, size_t len);

/*
 * Human-readable label for a status code, for operator diagnostics. Returns a
 * static string describing the failure class only — it NEVER contains a secret
 * value (the function has no access to one).
 */
const char *nssf_secret_ref_status_str(nssf_secret_ref_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_RUNTIME_SECRET_REF_RESOLVER_H */
