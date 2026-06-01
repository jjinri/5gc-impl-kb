/*
 * tls_context.h — OpenSSL mTLS server context for NSSF.
 *
 * ADR-0004 Layer A baseline M1/M2/M5/M6/M7. PEER verification is mandatory
 * (SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT); all paths/versions/ciphers
 * come from operator config — no hardcoded profile depth here.
 */

#ifndef NSSF_RUNTIME_TLS_CONTEXT_H
#define NSSF_RUNTIME_TLS_CONTEXT_H

#include <stdbool.h>
#include <stddef.h>

#include <openssl/ssl.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Operator-supplied TLS profile. The caller (config loader, not yet present)
 * owns all string lifetimes — this struct never copies them.
 */
typedef struct nssf_tls_config {
    bool enabled;                  /* M5 toggle. Prod path is built regardless. */
    const char *server_cert_path;  /* PEM server cert chain. required. */
    const char *server_key_path;   /* PEM private key. required. */
    const char *client_ca_path;    /* PEM CA bundle for peer (mTLS). required. */
    const char *min_version;       /* "TLSv1.2" | "TLSv1.3"; NULL → TLSv1.3. */
    const char *cipher_suites;     /* nullable → OpenSSL secure defaults. */
} nssf_tls_config_t;

typedef struct nssf_tls_context nssf_tls_context_t;

/*
 * Build an mTLS-ready SSL_CTX from cfg. Fail-fast: returns NULL and writes a
 * message into errbuf on any failure. The production code path (M5) is always
 * constructed; cfg->enabled is recorded for the caller to consult, not used to
 * skip context creation.
 */
nssf_tls_context_t *nssf_tls_context_create(const nssf_tls_config_t *cfg,
                                            char *errbuf, size_t errlen);

/* Underlying SSL_CTX, for accept/handshake wiring and for tests. */
SSL_CTX *nssf_tls_context_ssl_ctx(const nssf_tls_context_t *ctx);

/* Whether the operator enabled TLS (M5). The context exists either way. */
bool nssf_tls_context_is_enabled(const nssf_tls_context_t *ctx);

void nssf_tls_context_free(nssf_tls_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_RUNTIME_TLS_CONTEXT_H */
