/*
 * tls_context.c — OpenSSL mTLS server context for NSSF.
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — this is a handwritten runtime module.
 *
 * Crypto is OpenSSL only (M6 — no hand-rolled primitives). Peer verification
 * is never disabled (M2). TLS min version and ciphers are operator config with
 * OpenSSL secure defaults (M7).
 */

#define _POSIX_C_SOURCE 200809L

#include "tls_context.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

struct nssf_tls_context {
    SSL_CTX *ssl_ctx;
    bool enabled;
};

/* Pull the most recent OpenSSL error into errbuf; fall back to a fixed string. */
static void capture_ssl_error(char *errbuf, size_t errlen, const char *prefix)
{
    if (errbuf == NULL || errlen == 0) {
        return;
    }

    unsigned long code = ERR_get_error();
    char ossl[256] = {0};
    if (code != 0) {
        ERR_error_string_n(code, ossl, sizeof(ossl));
        snprintf(errbuf, errlen, "%s: %s", prefix, ossl);
    } else {
        snprintf(errbuf, errlen, "%s", prefix);
    }
}

/* Map operator min_version string to an OpenSSL proto constant. */
static int resolve_min_version(const char *min_version, int *out_proto,
                               char *errbuf, size_t errlen)
{
    /* NULL → secure default (M7). */
    if (min_version == NULL || strcmp(min_version, "TLSv1.3") == 0) {
        *out_proto = TLS1_3_VERSION;
        return 0;
    }
    if (strcmp(min_version, "TLSv1.2") == 0) {
        *out_proto = TLS1_2_VERSION;
        return 0;
    }

    /* Anything else (incl. attempts at <1.2) is rejected — never downgrade. */
    if (errbuf != NULL && errlen > 0) {
        snprintf(errbuf, errlen,
                 "unsupported tls min_version '%s' (allowed: TLSv1.2, TLSv1.3)",
                 min_version);
    }
    return -1;
}

nssf_tls_context_t *nssf_tls_context_create(const nssf_tls_config_t *cfg,
                                            char *errbuf, size_t errlen)
{
    if (cfg == NULL) {
        if (errbuf != NULL && errlen > 0) {
            snprintf(errbuf, errlen, "tls config is NULL");
        }
        return NULL;
    }
    if (cfg->server_cert_path == NULL || cfg->server_key_path == NULL ||
        cfg->client_ca_path == NULL) {
        if (errbuf != NULL && errlen > 0) {
            snprintf(errbuf, errlen,
                     "tls config requires server_cert_path, server_key_path, "
                     "client_ca_path");
        }
        return NULL;
    }

    int min_proto = 0;
    if (resolve_min_version(cfg->min_version, &min_proto, errbuf, errlen) != 0) {
        return NULL;
    }

    SSL_CTX *ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (ssl_ctx == NULL) {
        capture_ssl_error(errbuf, errlen, "SSL_CTX_new failed");
        return NULL;
    }

    if (SSL_CTX_set_min_proto_version(ssl_ctx, min_proto) != 1) {
        capture_ssl_error(errbuf, errlen, "SSL_CTX_set_min_proto_version failed");
        SSL_CTX_free(ssl_ctx);
        return NULL;
    }

    if (SSL_CTX_use_certificate_chain_file(ssl_ctx, cfg->server_cert_path) != 1) {
        capture_ssl_error(errbuf, errlen, "load server cert chain failed");
        SSL_CTX_free(ssl_ctx);
        return NULL;
    }

    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, cfg->server_key_path,
                                    SSL_FILETYPE_PEM) != 1) {
        capture_ssl_error(errbuf, errlen, "load server private key failed");
        SSL_CTX_free(ssl_ctx);
        return NULL;
    }

    if (SSL_CTX_check_private_key(ssl_ctx) != 1) {
        capture_ssl_error(errbuf, errlen, "server key does not match cert");
        SSL_CTX_free(ssl_ctx);
        return NULL;
    }

    /* mTLS trust anchor for peer cert chain (M2). */
    if (SSL_CTX_load_verify_locations(ssl_ctx, cfg->client_ca_path, NULL) != 1) {
        capture_ssl_error(errbuf, errlen, "load client CA bundle failed");
        SSL_CTX_free(ssl_ctx);
        return NULL;
    }

    /* M2 hard requirement: require + verify a peer cert. Never SSL_VERIFY_NONE. */
    SSL_CTX_set_verify(ssl_ctx,
                       SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    /*
     * Operator ciphers, each independently optional (M7). TLSv1.3 and TLSv1.2
     * use incompatible OpenSSL syntaxes, so each string feeds only its own slot —
     * never cross-fed. Omitting either leaves the OpenSSL secure default for that
     * protocol.
     */
    if (cfg->tls13_ciphersuites != NULL && cfg->tls13_ciphersuites[0] != '\0' &&
        SSL_CTX_set_ciphersuites(ssl_ctx, cfg->tls13_ciphersuites) != 1) {
        capture_ssl_error(errbuf, errlen, "set TLSv1.3 ciphersuites failed");
        SSL_CTX_free(ssl_ctx);
        return NULL;
    }
    if (cfg->tls12_cipher_list != NULL && cfg->tls12_cipher_list[0] != '\0' &&
        SSL_CTX_set_cipher_list(ssl_ctx, cfg->tls12_cipher_list) != 1) {
        capture_ssl_error(errbuf, errlen, "set TLSv1.2 cipher list failed");
        SSL_CTX_free(ssl_ctx);
        return NULL;
    }

    nssf_tls_context_t *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        if (errbuf != NULL && errlen > 0) {
            snprintf(errbuf, errlen, "out of memory");
        }
        SSL_CTX_free(ssl_ctx);
        return NULL;
    }

    ctx->ssl_ctx = ssl_ctx;
    ctx->enabled = cfg->enabled;
    return ctx;
}

SSL_CTX *nssf_tls_context_ssl_ctx(const nssf_tls_context_t *ctx)
{
    return ctx != NULL ? ctx->ssl_ctx : NULL;
}

bool nssf_tls_context_is_enabled(const nssf_tls_context_t *ctx)
{
    return ctx != NULL ? ctx->enabled : false;
}

void nssf_tls_context_free(nssf_tls_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    if (ctx->ssl_ctx != NULL) {
        SSL_CTX_free(ctx->ssl_ctx);
    }
    free(ctx);
}
