/* tests/security_patterns/fixtures/positive_violations.c
 *
 * 의도적 위반 sample. 각 block-severity pattern 이 최소 1회 매칭해야 한다.
 * 본 파일은 컴파일/링크 대상이 아니다 — pytest fixture 전용.
 */

/* M1 — TLS disable. */
#define NSSF_TLS_DISABLED 1

/* M1 — min proto SSL3. */
static void bad_tls_min(SSL_CTX *ctx) {
    SSL_CTX_set_min_proto_version(ctx, SSL3_VERSION);
}

/* M2 — peer verify off. */
static void bad_peer_verify(SSL_CTX *ctx) {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
}

/* M2 — curl peer verify disabled. */
static void bad_curl_peer(CURL *c) {
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
}

/* M3 — token validation skipped. */
static const int skip_token_validation = 1;
static void bad_jwt_decode(void) {
    jwt_decode(t, token_str, NULL, 0);
}

/* M5 — production TLS path stripped in dev. */
#ifdef DEV_BUILD
    SSL_CTX_free(ctx);
#endif

/* M6 — custom crypto primitive. */
static int nssf_aes_encrypt(const uint8_t *key, const uint8_t *plain, uint8_t *cipher) {
    return 0;
}

/* M7 — spec 33.501 inline reference. */
// per 33.501 §13.3.1 — handle re-authentication
void inline_spec_referenced(void) {}
