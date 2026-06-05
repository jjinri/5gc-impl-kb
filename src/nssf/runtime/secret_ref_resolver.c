/*
 * secret_ref_resolver.c — deploy-time secret_ref resolver seam for NSSF.
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten runtime module.
 *
 * Pure libc (getenv + stdio file read). No hand-rolled crypto (M6), no spec
 * dependency (M7), no third-party dependency. zlog is used only for the scheme
 * and the env-name / file-path — the resolved secret VALUE is never logged.
 *
 * Contract and invariants are documented in secret_ref_resolver.h.
 */

#define _POSIX_C_SOURCE 200809L

#include "secret_ref_resolver.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlog.h>

/* scheme prefixes — the only two accepted reference forms. */
static const char ENV_SCHEME[] = "env:";
static const char FILE_SCHEME[] = "file:";

/*
 * Scrub helper. memset() over a buffer that is freed right after is a textbook
 * dead-store the optimizer may elide; route the write through a volatile
 * function pointer so the zeroing is observable and cannot be removed.
 */
static void *(*const volatile secure_memset)(void *, int, size_t) = memset;

static void secure_scrub(char *buf, size_t len)
{
    if (buf != NULL && len > 0) {
        (void)secure_memset(buf, 0, len);
    }
}

const char *nssf_secret_ref_status_str(nssf_secret_ref_status_t status)
{
    switch (status) {
    case NSSF_SECRET_REF_OK:
        return "ok";
    case NSSF_SECRET_REF_BAD_FORMAT:
        return "bad-format";
    case NSSF_SECRET_REF_NOT_FOUND:
        return "not-found";
    case NSSF_SECRET_REF_IO_ERROR:
        return "io-error";
    case NSSF_SECRET_REF_TOO_LARGE:
        return "too-large";
    default:
        return "unknown";
    }
}

void nssf_secret_ref_scrub_free(char *secret, size_t len)
{
    if (secret == NULL) {
        return;
    }
    secure_scrub(secret, len);
    free(secret);
}

/*
 * Resolve "env:NAME". The env value is taken verbatim (a NUL byte cannot occur
 * in the environment, so strlen is exact). We copy it out immediately into an
 * owned buffer so the resolver holds no reference to libc's environ storage.
 * The NAME may appear in diagnostics; the value never does.
 */
static nssf_secret_ref_status_t resolve_env(const char *name,
                                            char **out_secret,
                                            size_t *out_len)
{
    /* Reject empty operand ("env:" with nothing after). */
    if (name[0] == '\0') {
        dzlog_error("secret_ref env: empty variable name");
        return NSSF_SECRET_REF_BAD_FORMAT;
    }

    const char *value = getenv(name);
    if (value == NULL) {
        /* Name is safe to log; the value (absent here anyway) is not. */
        dzlog_error("secret_ref env:%s — variable not set", name);
        return NSSF_SECRET_REF_NOT_FOUND;
    }

    size_t len = strlen(value);
    if (len > NSSF_SECRET_REF_MAX_BYTES) {
        dzlog_error("secret_ref env:%s — value exceeds %u byte cap",
                    name, (unsigned)NSSF_SECRET_REF_MAX_BYTES);
        return NSSF_SECRET_REF_TOO_LARGE;
    }

    char *buf = malloc(len + 1);
    if (buf == NULL) {
        /* No value bytes were copied yet — nothing to scrub. */
        dzlog_error("secret_ref env:%s — alloc failed", name);
        return NSSF_SECRET_REF_IO_ERROR;
    }
    memcpy(buf, value, len);
    buf[len] = '\0';

    *out_secret = buf;
    *out_len = len;
    return NSSF_SECRET_REF_OK;
}

/*
 * Strip at most one trailing line break in place: a single '\n', plus a single
 * immediately-preceding '\r' (CRLF). Returns the new length. No further
 * trimming — internal newlines and surrounding spaces in the secret are kept.
 */
static size_t strip_one_trailing_newline(char *buf, size_t len)
{
    if (len > 0 && buf[len - 1] == '\n') {
        len--;
        if (len > 0 && buf[len - 1] == '\r') {
            len--;
        }
    }
    return len;
}

/*
 * Resolve "file:/path". Reads up to the cap+1 bytes so an oversize file is
 * detected (and rejected) without reading the whole thing into memory. On any
 * failure path the partially-read buffer is scrubbed before free. The path may
 * appear in diagnostics; the file contents never do.
 */
static nssf_secret_ref_status_t resolve_file(const char *path,
                                             char **out_secret,
                                             size_t *out_len)
{
    /* Reject empty operand ("file:" with nothing after). */
    if (path[0] == '\0') {
        dzlog_error("secret_ref file: empty path");
        return NSSF_SECRET_REF_BAD_FORMAT;
    }

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        /*
         * Distinguish not-found from other I/O errors so the caller can tell a
         * mis-pathed deployment from a permissions/device error. errno text is
         * a generic class string and carries no secret content.
         */
        if (errno == ENOENT) {
            dzlog_error("secret_ref file:%s — not found", path);
            return NSSF_SECRET_REF_NOT_FOUND;
        }
        dzlog_error("secret_ref file:%s — open failed (%s)",
                    path, strerror(errno));
        return NSSF_SECRET_REF_IO_ERROR;
    }

    /*
     * Read one extra byte beyond the cap: if we ever fill it, the file is over
     * the limit. cap+2 holds cap content bytes, the over-limit probe byte, and
     * the NUL terminator.
     */
    size_t cap = NSSF_SECRET_REF_MAX_BYTES;
    char *buf = malloc(cap + 2);
    if (buf == NULL) {
        dzlog_error("secret_ref file:%s — alloc failed", path);
        fclose(fp);
        return NSSF_SECRET_REF_IO_ERROR;
    }

    size_t total = fread(buf, 1, cap + 1, fp);
    if (ferror(fp)) {
        dzlog_error("secret_ref file:%s — read failed", path);
        secure_scrub(buf, total);
        free(buf);
        fclose(fp);
        return NSSF_SECRET_REF_IO_ERROR;
    }
    fclose(fp);

    if (total > cap) {
        dzlog_error("secret_ref file:%s — exceeds %u byte cap",
                    path, (unsigned)cap);
        secure_scrub(buf, total);
        free(buf);
        return NSSF_SECRET_REF_TOO_LARGE;
    }

    size_t len = strip_one_trailing_newline(buf, total);
    buf[len] = '\0';

    /*
     * Trim the allocation down to the exact resolved size. realloc may copy and
     * leave the secret in the old block, so scrub any shrunk tail of the old
     * buffer is not directly reachable — instead allocate exact, copy, scrub the
     * oversize buffer, and free it. This guarantees no plaintext lingers in the
     * larger freed block.
     */
    char *exact = malloc(len + 1);
    if (exact == NULL) {
        dzlog_error("secret_ref file:%s — alloc failed", path);
        secure_scrub(buf, total);
        free(buf);
        return NSSF_SECRET_REF_IO_ERROR;
    }
    memcpy(exact, buf, len);
    exact[len] = '\0';
    secure_scrub(buf, total);
    free(buf);

    *out_secret = exact;
    *out_len = len;
    return NSSF_SECRET_REF_OK;
}

nssf_secret_ref_status_t nssf_secret_ref_resolve(const char *secret_ref,
                                                 char **out_secret,
                                                 size_t *out_len)
{
    /* Defensive: out params required before we can report success. */
    if (out_secret == NULL || out_len == NULL) {
        return NSSF_SECRET_REF_BAD_FORMAT;
    }
    *out_secret = NULL;
    *out_len = 0;

    if (secret_ref == NULL || secret_ref[0] == '\0') {
        dzlog_error("secret_ref — NULL/empty reference");
        return NSSF_SECRET_REF_BAD_FORMAT;
    }

    if (strncmp(secret_ref, ENV_SCHEME, sizeof(ENV_SCHEME) - 1) == 0) {
        return resolve_env(secret_ref + sizeof(ENV_SCHEME) - 1,
                           out_secret, out_len);
    }
    if (strncmp(secret_ref, FILE_SCHEME, sizeof(FILE_SCHEME) - 1) == 0) {
        return resolve_file(secret_ref + sizeof(FILE_SCHEME) - 1,
                            out_secret, out_len);
    }

    /*
     * Unknown scheme. We log a fixed message and deliberately do NOT echo the
     * reference back — a mis-set config could place a literal secret here, so
     * the unparsed handle is treated as potentially sensitive and never logged.
     */
    dzlog_error("secret_ref — unknown scheme (expected env:/file:)");
    return NSSF_SECRET_REF_BAD_FORMAT;
}
