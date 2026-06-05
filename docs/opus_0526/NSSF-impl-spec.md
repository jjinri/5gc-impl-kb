---
title: NSSF Implementation Specification
tags: [implementation-specs, impl-spec, c-language, modules, handlers, nf:nssf]
last_updated: 2026-05-26
source: doc/analysis/impl-specs/NSSF_impl_spec.md
nf: NSSF
spec: 29.531
ids_summary: VS=51 (NSSF-VS-0001~0051), Modules=13, Handlers=8, Clients=5
---

> **위키 편입 정보**
> - 원본: `doc/analysis/impl-specs/NSSF_impl_spec.md`
> - last_updated: 2026-05-26
> - 안정 ID 요약: VS=51 (NSSF-VS-0001~0051), Modules=13, Handlers=8, Clients=5

# NSSF 구현 명세 (C)

## 0. 메타

| 항목 | 값 |
|---|---|
| 대상 NF | NSSF |
| 메인 규격 | TS 29.531 j60 (Rel-18) |
| `target_language` | **C** |
| `tech_stack.libraries` | nghttp2 (T-HTTP-SRV), libcurl (T-HTTP-CLI), libuv (T-IO), cJSON (T-JSON, T-MODEL), OpenSSL (T-AUTH TLS), libjwt (T-AUTH JWT), zlog (T-LOG), uthash (T-DB inmem), libpq (T-DB persistent), Unity (T-TEST), CMake (T-BUILD) |
| 입력 산출물 | [features.md](../entities-features/29.531-features.md) (107 행, 매핑 4컬럼 충전 완료) / [api-analysis](../implementation-specs/NSSF-api-spec.md) (8 endpoints + 1 callback) / [procedure-analysis](../entities-features/NSSF-procedure-analysis.md) (PROC-0001~0020) / [db-design](../implementation-specs/NSSF-db-design.md) (ENT-0001~0016) |
| 0단계 입구 점검 | ✅ 통과 (4 산출물 존재·비공백, 자체 체크리스트 모두 ✅, features.md placeholder 3 컬럼 충전율 100%, `tech_stack.libraries` 11개 항목 결정, `target_language=C`) |
| 도메인 힌트 | [NSSF_hints.md](../sources/NSSF-hints.md) 적용 (H5 C 매핑 sketch + H8 NSSF 특이사항) |
| binary_name | `nssfd` (`agent_context.json` project.binary_name) |

## 1. 파일·모듈 구조

C 관용 디렉토리 (`agent_context.json` project.* 기반): `dev/src/{module}/*.c`, `dev/libsrc/{module}/*.c` (공유 모듈), `dev/include/nssf/{module}.h`, `dev/conf/`, `dev/test/`. 빌드: CMake (`CMakeLists.txt` 루트 + `dev/src/CMakeLists.txt` 서브).

### 1-A. 논리 모듈 카탈로그 적용·미적용

| 모듈 ID | 적용 | 비고 |
|---|---|---|
| M-CFG | ✅ | `dev/include/nssf/cfg.h`, `dev/libsrc/cfg/cfg.c` |
| M-LOG | ✅ | `dev/include/nssf/log.h`, `dev/libsrc/log/log.c` (zlog wrapper) |
| M-DB | ✅ | `dev/libsrc/db/{pg,cache}.c` + 엔티티별 모듈 (`amf_reg.c`, `subscription.c` 등) |
| M-HTTP-SRV | ✅ | `dev/libsrc/http_srv/server.c` (nghttp2 + libuv) |
| M-HTTP-CLI | ✅ | `dev/libsrc/http_cli/client.c` (libcurl multi + libuv) |
| M-AUTH | ✅ | `dev/libsrc/auth/{tls,oauth2,jwt}.c` (OpenSSL + libjwt) |
| M-MODEL | ✅ | `dev/libsrc/model/{snssai,tai,nssai_avail,subscription,nssf_event,...}.c` (cJSON) |
| M-HANDLER-1 ... M-HANDLER-8 | ✅ 8개 | E1~E8 각각 |
| M-CLIENT-NRF | ✅ | NRF NFM·NFD·OAuth2 통합 클라이언트 |
| M-CLIENT-NOTIFY | ✅ | C1 callback (NSSF→Consumer Notify 송신) |
| M-CLIENT-SEPP | ✅ | PROC-0012 V-NSSF↔H-NSSF (N32 over HTTP/2) |
| M-PROC-0001 ... M-PROC-0020 | ✅ 20개 | 절차별 오케스트레이터 |
| M-LIFE | ✅ | `dev/libsrc/life/life.c` (init·shutdown·timer·signal) |
| M-MAIN | ✅ | `dev/src/main.c` |

### 1-B. 산출 표

| 모듈 ID | C 파일/패키지 | 역할 1줄 | 의존 모듈 |
|---|---|---|---|
| M-CFG | `dev/include/nssf/cfg.h` + `dev/libsrc/cfg/cfg.c` | cfg JSON 파싱·검증·lifecycle 변환 | cJSON, M-LOG |
| M-LOG | `dev/include/nssf/log.h` + `dev/libsrc/log/log.c` | zlog 초기화·카테고리 매핑·구조화 출력 | zlog |
| M-DB | `dev/include/nssf/db.h` + `dev/libsrc/db/pg.c` + `dev/libsrc/db/cache.c` + `dev/libsrc/db/{amf_reg,authorized,subscription,validity,replacement,nrf_reg,config}.c` | PostgreSQL CRUD + uthash 핫 캐시 | libpq, uthash, M-MODEL |
| M-HTTP-SRV | `dev/include/nssf/http_srv.h` + `dev/libsrc/http_srv/{server,route,header}.c` | nghttp2 HTTP/2 서버 + 라우팅 + 헤더 처리 | nghttp2, libuv, OpenSSL, M-LOG |
| M-HTTP-CLI | `dev/include/nssf/http_cli.h` + `dev/libsrc/http_cli/{client,multi,retry}.c` | libcurl multi handle + libuv 통합 + 재시도 | libcurl, libuv, OpenSSL, M-LOG |
| M-AUTH | `dev/include/nssf/auth.h` + `dev/libsrc/auth/{tls,oauth2,jwt,token_cache}.c` | mTLS · OAuth2 token 발급·검증 (libjwt RS256/ES256) | OpenSSL, libjwt, uthash (ENT-0013), M-HTTP-CLI |
| M-MODEL | `dev/include/nssf/model.h` + `dev/libsrc/model/{snssai,tai,plmn_id,nssai_avail_info,authorized,subscription,nssf_event,patch_doc,problem_details,supported_features,...}.c` | cJSON 기반 모델 직렬화/역직렬화 + 메모리 소유권 | cJSON |
| M-HANDLER-1 (E1) | `dev/libsrc/handler/network_slice_info.c` | GET /network-slice-information 핸들러 | M-MODEL, M-DB, M-AUTH, M-CLIENT-NRF |
| M-HANDLER-2 (E2) | `dev/libsrc/handler/nssai_avail_put.c` | PUT /nssai-availability/{nfId} | M-MODEL, M-DB |
| M-HANDLER-3 (E3) | `dev/libsrc/handler/nssai_avail_patch.c` | PATCH (RFC 6902) | M-MODEL (PatchDocument), M-DB |
| M-HANDLER-4 (E4) | `dev/libsrc/handler/nssai_avail_delete.c` | DELETE /nssai-availability/{nfId} | M-DB |
| M-HANDLER-5 (E5) | `dev/libsrc/handler/sub_create.c` | POST /subscriptions | M-MODEL, M-DB |
| M-HANDLER-6 (E6) | `dev/libsrc/handler/sub_modify.c` | PATCH /subscriptions/{id} (SUMOD) | M-MODEL, M-DB |
| M-HANDLER-7 (E7) | `dev/libsrc/handler/sub_delete.c` | DELETE /subscriptions/{id} | M-DB |
| M-HANDLER-8 (E8) | `dev/libsrc/handler/options.c` | OPTIONS /nssai-availability | – |
| M-CLIENT-NRF | `dev/libsrc/client/nrf.c` | NFRegister/Update/Deregister/Discovery + OAuth2 Token | M-HTTP-CLI, M-AUTH, M-MODEL |
| M-CLIENT-NOTIFY | `dev/libsrc/client/notify.c` | C1 callback POST (Notify 송신, 백오프 재시도) | M-HTTP-CLI, M-AUTH, M-MODEL, M-DB (ENT-0015) |
| M-CLIENT-SEPP | `dev/libsrc/client/sepp.c` | N32-f forward (V-NSSF↔H-NSSF) | M-HTTP-CLI, M-AUTH |
| M-PROC-0001~0020 | `dev/libsrc/proc/{nrf_register,nrf_deregister,nrf_heartbeat,nrf_discover,oauth_token,nssel_get_*,nssai_avail_update,nssai_avail_sub,nssai_avail_notify_*,nssai_avail_unsub,nssai_avail_delete,sub_expiry}.c` | 절차별 오케스트레이션 (state 전이·트랜잭션 시퀀싱) | M-HANDLER-*, M-CLIENT-*, M-DB |
| M-LIFE | `dev/include/nssf/life.h` + `dev/libsrc/life/life.c` | init·shutdown·signal·timer wire | M-CFG, M-LOG, M-DB, M-HTTP-SRV, M-HTTP-CLI, M-AUTH, M-CLIENT-NRF |
| M-MAIN | `dev/src/main.c` | argv·init·signal_wait·graceful_shutdown | M-LIFE |

## 2. 공개 API 표면

C 가시성 규칙: 헤더 (`dev/include/nssf/<module>.h`) 에 선언된 심볼만 외부 공개. `static` 함수는 모듈 내부 전용.

| 모듈 ID | 공개 심볼 | 시그니처 (의사) | 가시성 |
|---|---|---|---|
| M-CFG | `nssf_config_t` (struct typedef) | – | header 공개 struct |
| M-CFG | `cfg_load`, `cfg_validate`, `cfg_free`, `cfg_to_life_cfg` | `int cfg_load(const char *path, nssf_config_t **out);` `int cfg_validate(const nssf_config_t *cfg);` `void cfg_free(nssf_config_t *cfg);` `int cfg_to_life_cfg(const nssf_config_t *cfg, life_cfg_t *out);` | extern |
| M-LOG | `log_init`, `log_shutdown`, `LOG_CATEGORY`, `LOG_DEBUG`/`LOG_INFO`/`LOG_WARN`/`LOG_ERROR` macros | `int log_init(const char *conf_path);` `void log_shutdown(void);` 매크로는 zlog wrapper | extern + macros |
| M-DB | `db_init`, `db_shutdown`, ENT-0001~0016 CRUD 함수 (예: `db_amf_reg_upsert`, `db_sub_create`, ...) | 각 함수의 입력·출력은 §6 호출 매핑 표 인용 | extern |
| M-HTTP-SRV | `http_srv_start`, `http_srv_shutdown`, `http_srv_route_register` | `int http_srv_start(const http_srv_cfg_t *cfg, uv_loop_t *loop);` `void http_srv_shutdown(void);` `int http_srv_route_register(const char *method, const char *path_pattern, http_handler_fn);` | extern |
| M-HTTP-CLI | `http_cli_start`, `http_cli_shutdown`, `http_cli_send` | `int http_cli_start(const http_cli_cfg_t *cfg, uv_loop_t *loop);` `int http_cli_send(const http_req_t *req, http_response_cb cb, void *user_data);` | extern |
| M-AUTH | `auth_init`, `auth_shutdown`, `auth_verify_jwt`, `auth_get_token`, `auth_tls_ctx` | `int auth_init(const auth_cfg_t *cfg);` `int auth_verify_jwt(const char *bearer, const char *expected_scope, jwt_claims_t *out_claims);` `int auth_get_token(const char *target_nf_type, const char *scope, char **out_bearer, time_t *out_expires_at);` | extern |
| M-MODEL | 각 모델 별 `<model>_parse`, `<model>_to_json`, `<model>_free` | 예: `int snssai_parse(const cJSON *node, snssai_t *out);` `cJSON *snssai_to_json(const snssai_t *s);` `void snssai_free(snssai_t *s);` | extern (모델당 3 함수) |
| M-HANDLER-{n} | `handler_<endpoint>_dispatch` | `int handler_e1_dispatch(http_req_t *req, http_resp_t *resp);` ... `handler_e8_dispatch` | extern (라우터 등록용) |
| M-CLIENT-NRF | `client_nrf_register`, `client_nrf_deregister`, `client_nrf_heartbeat`, `client_nrf_discover_amf`, `client_nrf_token_request` | 각 비동기 호출, callback 으로 결과 전달 | extern |
| M-CLIENT-NOTIFY | `client_notify_send`, `client_notify_retry_due` | – | extern |
| M-CLIENT-SEPP | `client_sepp_forward` | – | extern |
| M-PROC-{n} | `proc_<name>_trigger` | – | extern (대부분 M-LIFE 또는 핸들러에서 호출) |
| M-LIFE | `life_init`, `life_run`, `life_shutdown`, `life_signal_wait` | `int life_init(const life_cfg_t *cfg);` `int life_run(void);` `void life_shutdown(void);` | extern |
| M-MAIN | `main` | `int main(int argc, char **argv);` | C entry point |

## 3. 데이터 모델

### 3-A. 공통 타입 매핑 인용 (api-analysis §3)

본 산출물은 [NSSF_api_analysis.md §3](../implementation-specs/NSSF-api-spec.md) 의 C 매핑 원칙을 *그대로 인용*하며 추가/변경하지 않음. 짧은 인용 (§3 전체는 api-analysis 본문 참조):

| 3GPP 타입 | C 표현 (api-analysis §3 인용) | 직렬화 라이브러리 |
|---|---|---|
| `Snssai` | `struct snssai { uint8_t sst; char sd[7]; bool has_sd; }` | cJSON |
| `Tai` | `struct tai { struct plmn_id plmn; char tac[7]; char nid[12]; bool has_nid; }` | cJSON |
| `PlmnId` | `struct plmn_id { char mcc[4]; char mnc[4]; }` | cJSON |
| `NfInstanceId` | `char nf_id[37]` (UUID + NUL) | cJSON |
| `Uri` | `char *uri` (heap) | cJSON |
| `DateTime` | `time_t` (epoch) + ISO 8601 변환 함수 | cJSON |
| `SupportedFeatures` | hex 문자열 ↔ `uint64_t bitmap[K]` | cJSON + 변환 헬퍼 |
| `ProblemDetails` | api-analysis §3 참조 (`type/title/status/detail/instance/cause/invalidParams[]`) | cJSON |
| `PatchItem` (RFC 6902) | `struct patch_item { enum patch_op; char *path; cJSON *value; char *from; ... }` | cJSON (value 는 opaque cJSON 노드 보관) |
| `NsagId` | `uint8_t` | cJSON (hex string 변환) |
| `TaiRange`, `NFType`, `AccessType`, `RoamingIndication`, `NssfEventType` | enum + open string fallback (anyOf 패턴) | cJSON |

### 3-B. 모델별 C 표현 (api-analysis §2 인용)

| 모델명 (api §) | C 표현 | 메모리 소유권 | 직렬화 |
|---|---|---|---|
| `AuthorizedNetworkSliceInfo` (api §2.1) | `struct authorized_network_slice_info { allowed_nssai_t *allowed_nssai_list; size_t n_allowed; configured_snssai_t *configured_nssai; size_t n_configured; char target_amf_set[16]; bool has_target_amf_set; nf_instance_id_t *candidate_amf_list; size_t n_candidates; snssai_t *rejected_nssai_in_plmn; size_t n_rej_plmn; ... nsi_information_t *nsi_information; uint64_t supported_features[2]; ... }` | caller free (handler 가 build → serialize → free) | cJSON |
| `AllowedNssai`, `AllowedSnssai`, `ConfiguredSnssai`, `NsiInformation`, `MappingOfSnssai`, `NsagInfo`, `SnssaiInfo` | 동등 struct (필드 표는 api §2.1 그대로) | 동 | cJSON |
| `SliceInfoForRegistration`, `SliceInfoForPDUSession`, `SliceInfoForUEConfigurationUpdate`, `RoamingIndication` | 동 (api §2.1) | parser 가 alloc, handler 가 free | cJSON |
| `NssaiAvailabilityInfo`, `SupportedNssaiAvailabilityData` (api §2.2) | 동 | parser alloc → handler 가 db 에 transfer 후 free | cJSON |
| `AuthorizedNssaiAvailabilityInfo`, `AuthorizedNssaiAvailabilityData`, `RestrictedSnssai` (api §2.2) | 동 | db 에서 build → serialize → free | cJSON |
| `NssfEventSubscriptionCreateData`, `NssfEventSubscriptionCreatedData` (api §2.5) | 동 | parser/builder alloc | cJSON |
| `NssfEventNotification` (api §2.9) | 동, `nssaiValidityTimeInfoList` 는 `struct { snssai_t key; recur_time_t *values; size_t n; } *map_entries; size_t n_entries;` (cJSON 객체→struct list) | 동 | cJSON |
| `SnssaiReplacementSubscribeInfo`, `NsiUnavailabilitySubscribeInfo` (api §2.5) | 동 | 동 | cJSON |
| `PatchDocument` (RFC 6902) | `struct patch_doc { patch_item_t *items; size_t n; }` | parser alloc, handler free | cJSON |
| `ProblemDetails` (TS 29.571 §5.2.4) | api-analysis §3 인용 | builder alloc, server response 송신 후 free | cJSON |

### 3-C. M-CFG schema (운영 부팅 anchor — R11)

#### 3-C-1. seed entry struct 명세 (db-design §8-C 시드 인용)

```c
typedef struct nssai_config_entry {
    plmn_id_t plmn;
    char tac[7];
    char nid[12];
    bool has_nid;
    snssai_t *supported_snssai_list;
    size_t n_supported;
} nssai_config_entry_t;

typedef struct restricted_entry {
    char tac[7];
    char nid[12];
    bool has_nid;
    plmn_id_t home_plmn;
    snssai_t *snssai_list;
    size_t n_snssai;
    bool roaming_restriction;
} restricted_entry_t;

typedef struct amf_set_mapping_entry {
    char amf_set_id[16];
    snssai_t *snssai_list;
    size_t n_snssai;
    nf_instance_id_t *candidate_amf_list;
    size_t n_candidates;
} amf_set_mapping_entry_t;

typedef struct nsi_mapping_entry {
    snssai_t snssai;
    char **nsi_id_list;
    size_t n_nsi;
} nsi_mapping_entry_t;

typedef struct nrf_per_slice_entry {
    snssai_t snssai;
    char *nrf_id;
    char *nrf_nfm_uri;
    char *nrf_access_token_uri;
} nrf_per_slice_entry_t;

typedef struct vplmn_hplmn_entry {
    plmn_id_t vplmn;
    snssai_t serving_snssai;
    snssai_t home_snssai;
} vplmn_hplmn_entry_t;

typedef struct nsag_config_entry {
    char tac[7];
    char nid[12];
    bool has_nid;
    uint8_t nsag_id;
    snssai_t *snssai_list;
    size_t n_snssai;
} nsag_config_entry_t;

typedef struct validity_time_entry {
    snssai_t snssai;
    recur_time_t *recur_times;
    size_t n_recurs;
} validity_time_entry_t;
```

#### 3-C-2. cfg load 의사 코드 (`tech_stack` JSON 파서 = cJSON)

```c
int cfg_load(const char *path, nssf_config_t **out) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return ERR_CFG_FILE_NOT_FOUND;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return ERR_CFG_PARSE;

    nssf_config_t *cfg = calloc(1, sizeof(nssf_config_t));

    // top-level keys MUST match struct fields (DEF-011 sanity check anchor)
    cJSON *node;
    if ((node = cJSON_GetObjectItem(root, "nf_instance_id")))   parse_nf_instance_id(node, cfg->nf_instance_id);
    if ((node = cJSON_GetObjectItem(root, "life")))             parse_life_cfg(node, &cfg->life);
    if ((node = cJSON_GetObjectItem(root, "log")))              parse_log_cfg(node, &cfg->log);
    if ((node = cJSON_GetObjectItem(root, "db")))               parse_db_cfg(node, &cfg->db);
    if ((node = cJSON_GetObjectItem(root, "http_srv")))         parse_http_srv_cfg(node, &cfg->http_srv);
    if ((node = cJSON_GetObjectItem(root, "http_cli")))         parse_http_cli_cfg(node, &cfg->http_cli);
    if ((node = cJSON_GetObjectItem(root, "auth")))             parse_auth_cfg(node, &cfg->auth);
    if ((node = cJSON_GetObjectItem(root, "nrf")))              parse_nrf_cfg(node, &cfg->nrf);
    if ((node = cJSON_GetObjectItem(root, "features")))         parse_features_cfg(node, &cfg->features);
    if ((node = cJSON_GetObjectItem(root, "seed")))             parse_seed_cfg(node, &cfg->seed);

    cJSON_Delete(root);
    *out = cfg;
    return 0;
}
```

#### 3-C-3. cfg validate 검증 룰

| 영역 | 룰 |
|---|---|
| `nf_instance_id` | RFC 4122 UUID 패턴 매칭; 빈 문자열 → 거부 |
| `life.shutdown_grace_sec` | 1..120 |
| `log.conf_path` | 파일 존재 + 읽기 가능 |
| `db.conninfo` | libpq `PQconnectdb` dry-run; 실패 → 거부 |
| `db.uthash_max_subscriptions` | 100..1_000_000 |
| `http_srv.bind_port` | 1024..65535 |
| `http_srv.tls.cert_path`/`key_path`/`ca_path` | 파일 존재·읽기 가능; OpenSSL 로 cert/key 페어 정합성 검증 |
| `http_cli.timeout_ms` | 1000..60000 |
| `auth.oauth2.enabled` | bool |
| `auth.jwt.pubkey_path` | OAuth2 활성 시 필수, 파일 존재 |
| `auth.tls.mtls_required` | bool |
| `nrf.base_url` | `^https://` 검증 |
| `nrf.heartbeat_hint_sec` | 10..3600 (NRF가 최종 결정) |
| `features.nsselection_bitmap` / `features.nssaiavail_bitmap` | hex string, length ≤ 16 chars |
| `features.nsselection_bitmap` ES3XX 비트 | M (필수, 비트 1 켜져 있어야 함) |
| `features.nssaiavail_bitmap` ES3XX 비트 | M (비트 4 켜져 있어야 함) |
| `seed.nssai_config` | minItems 1, 각 entry 의 `plmn` + `tac` 유효성 |
| `seed.amf_set_mapping` | minItems 1, `amf_set_id` 정규식 |

검증 실패 시 `cfg_validate` 가 ERR 코드 반환 → M-MAIN 이 부팅 거부 (`exit(1)`) + 로그 ERROR.

#### 3-C-4. lifecycle 매핑 (`cfg → life_cfg`)

```c
typedef struct life_cfg {
    char *log_conf_path;
    char *db_conninfo;
    http_srv_cfg_t http_srv;
    http_cli_cfg_t http_cli;
    auth_cfg_t auth;
    nrf_cfg_t nrf;
    features_cfg_t features;
    seed_cfg_t seed;
    int shutdown_grace_sec;
    nf_instance_id_t nf_instance_id;
} life_cfg_t;

int cfg_to_life_cfg(const nssf_config_t *cfg, life_cfg_t *out) {
    memcpy(&out->nf_instance_id, cfg->nf_instance_id, sizeof(out->nf_instance_id));
    out->log_conf_path = strdup(cfg->log.conf_path);
    out->db_conninfo = strdup(cfg->db.conninfo);
    out->http_srv = cfg->http_srv;
    out->http_cli = cfg->http_cli;
    out->auth = cfg->auth;
    out->nrf = cfg->nrf;
    out->features = cfg->features;
    out->seed = cfg->seed;
    out->shutdown_grace_sec = cfg->life.shutdown_grace_sec;
    return 0;
}
```

#### 3-C-5. cfg 페이로드 예시 (R11)

`nssf_config_t` C struct top-level fields: **`nf_instance_id`, `life`, `log`, `db`, `http_srv`, `http_cli`, `auth`, `nrf`, `features`, `seed`** (10개).

##### 3-C-5-1. 운영 변형 (`dev/conf/nssfd.json`)

```json
{
  "nf_instance_id": "8e6f8a3c-1d4e-4f12-9a3b-7e5c2f4a1b0d",
  "life": {
    "shutdown_grace_sec": 30
  },
  "log": {
    "conf_path": "/etc/nssfd/log.conf"
  },
  "db": {
    "conninfo": "host=db.internal port=5432 user=nssf dbname=nssf sslmode=require",
    "uthash_max_subscriptions": 100000,
    "uthash_max_amf_records": 10000
  },
  "http_srv": {
    "bind_host": "::",
    "bind_port": 8443,
    "tls": {
      "cert_path": "/etc/nssfd/tls/server.crt",
      "key_path": "/etc/nssfd/tls/server.key",
      "ca_path": "/etc/nssfd/tls/ca.crt",
      "mtls_required": true
    }
  },
  "http_cli": {
    "timeout_ms": 5000,
    "max_concurrent": 256,
    "connect_timeout_ms": 2000,
    "max_retries": 3,
    "retry_backoff_ms": 1000
  },
  "auth": {
    "oauth2": {
      "enabled": true,
      "token_cache_ttl_margin_sec": 30
    },
    "jwt": {
      "pubkey_path": "/etc/nssfd/tls/nrf_pubkey.pem",
      "algorithms": ["RS256", "ES256"]
    },
    "tls": {
      "client_cert_path": "/etc/nssfd/tls/client.crt",
      "client_key_path": "/etc/nssfd/tls/client.key",
      "verify_peer": true
    }
  },
  "nrf": {
    "base_url": "https://nrf.internal:8443",
    "heartbeat_hint_sec": 60,
    "nfm_path": "/nnrf-nfm/v1",
    "disc_path": "/nnrf-disc/v1",
    "oauth_token_path": "/oauth2/token"
  },
  "features": {
    "nsselection_bitmap": "F",
    "nssaiavail_bitmap": "78"
  },
  "seed": {
    "nssai_config": [
      {
        "plmn": {"mcc": "450", "mnc": "08"},
        "tac": "000001",
        "supported_snssai_list": [
          {"sst": 1},
          {"sst": 2, "sd": "000001"}
        ]
      }
    ],
    "restricted_snssai": [],
    "amf_set_mapping": [
      {
        "amf_set_id": "450-08-01-001",
        "snssai_list": [{"sst": 1}],
        "candidate_amf_list": ["1f2e3d4c-5b6a-7e8f-9012-3456789abcde"]
      }
    ],
    "nsi_mapping": [],
    "nrf_per_slice": [],
    "vplmn_hplmn_mapping": [],
    "nsag_config": [],
    "validity_time": []
  }
}
```

##### 3-C-5-2. dev 변형 (`dev/conf/samples/nssfd.dev.json`)

```json
{
  "nf_instance_id": "00000000-0000-4000-8000-000000000001",
  "life": {"shutdown_grace_sec": 5},
  "log": {"conf_path": "dev/conf/samples/log.dev.conf"},
  "db": {
    "conninfo": "host=127.0.0.1 port=5432 user=nssf_dev password=nssf_dev dbname=nssf_dev sslmode=disable",
    "uthash_max_subscriptions": 1000,
    "uthash_max_amf_records": 100
  },
  "http_srv": {
    "bind_host": "127.0.0.1",
    "bind_port": 18443,
    "tls": {
      "cert_path": "dev/conf/samples/tls/server.crt",
      "key_path": "dev/conf/samples/tls/server.key",
      "ca_path": "dev/conf/samples/tls/ca.crt",
      "mtls_required": false
    }
  },
  "http_cli": {
    "timeout_ms": 3000,
    "max_concurrent": 16,
    "connect_timeout_ms": 1000,
    "max_retries": 2,
    "retry_backoff_ms": 500
  },
  "auth": {
    "oauth2": {"enabled": false, "token_cache_ttl_margin_sec": 30},
    "jwt": {"pubkey_path": "dev/conf/samples/tls/nrf_pubkey.pem", "algorithms": ["RS256"]},
    "tls": {
      "client_cert_path": "dev/conf/samples/tls/client.crt",
      "client_key_path": "dev/conf/samples/tls/client.key",
      "verify_peer": false
    }
  },
  "nrf": {
    "base_url": "http://127.0.0.1:18000",
    "heartbeat_hint_sec": 30,
    "nfm_path": "/nnrf-nfm/v1",
    "disc_path": "/nnrf-disc/v1",
    "oauth_token_path": "/oauth2/token"
  },
  "features": {"nsselection_bitmap": "F", "nssaiavail_bitmap": "78"},
  "seed": {
    "nssai_config": [{"plmn": {"mcc": "001", "mnc": "01"}, "tac": "000001", "supported_snssai_list": [{"sst": 1}]}],
    "restricted_snssai": [],
    "amf_set_mapping": [{"amf_set_id": "001-01-01-001", "snssai_list": [{"sst": 1}], "candidate_amf_list": ["11111111-1111-4111-8111-111111111111"]}],
    "nsi_mapping": [],
    "nrf_per_slice": [],
    "vplmn_hplmn_mapping": [],
    "nsag_config": [],
    "validity_time": []
  }
}
```

##### 3-C-5-3. 영역별 필수/선택 매트릭스

| 영역 (top-level key) | 운영 | dev | 비고 |
|---|---|---|---|
| `nf_instance_id` | M | M | UUID |
| `life` | M | M | – |
| `log` | M | M | – |
| `db` | M | M | dev: sslmode=disable 허용 |
| `http_srv` | M | M | – |
| `http_cli` | M | M | – |
| `auth` | M | M | dev: oauth2.enabled=false 허용 |
| `nrf` | M | M | – |
| `features` | M | M | ES3XX 비트 M |
| `seed` | M | M | dev: 최소 시드 1건 |

##### 3-C-5-4. DEF-011 schema-payload 일관성 sanity check 결과

| 점검 | 결과 |
|---|---|
| `nssf_config_t` top-level field 집합 | `{nf_instance_id, life, log, db, http_srv, http_cli, auth, nrf, features, seed}` |
| 3-C-5-1 운영 JSON top-level keys | 동 (10개 1:1 매칭) |
| 3-C-5-2 dev JSON top-level keys | 동 (10개 1:1 매칭) |
| 영역별 child key 1:1 (`auth.{oauth2, jwt, tls}`, `db.{conninfo, uthash_max_*}`, `http_srv.{bind_host, bind_port, tls.{cert_path,key_path,ca_path,mtls_required}}`, `http_cli.{timeout_ms,max_concurrent,connect_timeout_ms,max_retries,retry_backoff_ms}`, `nrf.{base_url,heartbeat_hint_sec,nfm_path,disc_path,oauth_token_path}`, `features.{nsselection_bitmap,nssaiavail_bitmap}`, `seed.{nssai_config,restricted_snssai,amf_set_mapping,nsi_mapping,nrf_per_slice,vplmn_hplmn_mapping,nsag_config,validity_time}`) | ✅ 동일 |
| primitive 타입 일치 (string/int/object/array) | ✅ |

#### 3-C-6. TLS 운영 정책 (R11)

| 항목 | 운영 | dev (sample) |
|---|---|---|
| 디렉토리 | `/etc/nssfd/tls/` | `dev/conf/samples/tls/` |
| 서버 cert | `server.crt` (PEM, RSA 2048+ 또는 ECDSA P-256+) | 동 |
| 서버 key | `server.key` (PEM, 0400 권한) | 동 |
| 클라이언트 cert (NRF/AMF 호출용) | `client.crt` | 동 |
| 클라이언트 key | `client.key` (0400) | 동 |
| CA bundle | `ca.crt` (NRF/AMF/SEPP 인증서 검증용) | 동 |
| JWT 검증 공개키 (NRF token verifier) | `nrf_pubkey.pem` (PEM, RS256/ES256 공개키) | 동 |

##### 3-C-6-1. .gitignore 룰

```
# 운영 cert/key 추적 금지
/etc/nssfd/tls/
dev/conf/tls/

# sample bundle 만 추적 허용
!dev/conf/samples/tls/
```

##### 3-C-6-2. 더미 cert 발급 절차 (`tech_stack.tls=OpenSSL` CLI)

```bash
# CA self-signed (dev 전용)
openssl req -x509 -newkey rsa:2048 -nodes -keyout ca.key -out ca.crt -days 365 \
  -subj "/CN=NSSF-DEV-CA"

# server cert (CN = NSSF)
openssl req -newkey rsa:2048 -nodes -keyout server.key -out server.csr \
  -subj "/CN=nssf.dev.local"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out server.crt -days 365 -extfile <(printf "subjectAltName=DNS:nssf.dev.local,IP:127.0.0.1")

# client cert (mutual TLS)
openssl req -newkey rsa:2048 -nodes -keyout client.key -out client.csr \
  -subj "/CN=nssf-client"
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out client.crt -days 365

# NRF JWT pubkey (dev: 동일 CA 의 ECDSA 키 페어)
openssl ecparam -name prime256v1 -genkey -noout -out nrf_priv.pem
openssl ec -in nrf_priv.pem -pubout -out nrf_pubkey.pem

chmod 400 *.key
```

스크립트화: `dev/scripts/gen-dummy-tls.sh` (plan §5 책임 task).

##### 3-C-6-3. mTLS 활성 조건

- `http_srv.tls.mtls_required = true` + `ca_path` 유효 → 서버는 클라이언트 cert 검증 (verify_peer + verify_fail_if_no_peer_cert).
- `auth.tls.verify_peer = true` → 클라이언트(libcurl)는 서버 cert 검증.
- 운영: 양방향 모두 true 필수 (TS 33.501 §13). dev: false 허용.

##### 3-C-6-4. 만료·갱신 정책

- 운영: cert 유효 ≥ 90일 모니터링; 30일 이내 만료 시 운영 알람 (M-LOG WARN).
- 갱신: 무중단 reload (`SIGHUP` 트리거 → M-LIFE 가 OpenSSL `SSL_CTX` 재로드).

## 4. Producer 핸들러

### 4-A. 핸들러 카탈로그

| 모듈 ID | 매핑 endpoint (api §1-B) | 매핑 절차 | 매핑 기능 ID | 동시성 |
|---|---|---|---|---|
| M-HANDLER-1 | E1 GET /network-slice-information | PROC-0006~0012 | SVC-0001~0011, DAT-0001~0012, MGMT-0001, ERR-0001~0009, SVC-0033~0037, DAT-0029, SEC-0003 | read-lock on ENT-0001~0007 (RWL) |
| M-HANDLER-2 | E2 PUT /nssai-availability/{nfId} | PROC-0013 | SVC-0012, DAT-0013, DAT-0014, ERR-0010, SVC-0038~0041, SVC-0043, SVC-0044, SEC-0004 | serializable on ENT-0008/0009 (per-nfId row lock) |
| M-HANDLER-3 | E3 PATCH /nssai-availability/{nfId} | PROC-0013 | SVC-0013, DAT-0015, DAT-0016, ERR-0010, SVC-0042, DAT-0030 | serializable |
| M-HANDLER-4 | E4 DELETE /nssai-availability/{nfId} | PROC-0019 | SVC-0030, SVC-0031, ERR-0015, DAT-0031 | serializable |
| M-HANDLER-5 | E5 POST /nssai-availability/subscriptions | PROC-0014 | SVC-0014~0017, DAT-0017~0023, ERR-0011, DAT-0032, DAT-0033 | serializable on ENT-0010 |
| M-HANDLER-6 | E6 PATCH /subscriptions/{id} | PROC-0014 (Modify) | SVC-0018~0020, DAT-0024/0025, ERR-0012 | serializable |
| M-HANDLER-7 | E7 DELETE /subscriptions/{id} | PROC-0018 | SVC-0021, SVC-0022, ERR-0013 | serializable |
| M-HANDLER-8 | E8 OPTIONS /nssai-availability | (없음) | SVC-0032, ERR-0016 | lock-free |

### 4-B. 핸들러 의사 코드

#### 4-B-1. M-HANDLER-1 handle_e1_dispatch (GET /network-slice-information)

```c
int handler_e1_dispatch(http_req_t *req, http_resp_t *resp) {
    // [기능 트레이스: SVC-0001~0011, DAT-0001~0012, MGMT-0001, ERR-0001~0009]

    // 1. 인증·인가 (M-AUTH, api §4: API-level scope=nnssf-nsselection)
    jwt_claims_t claims;
    if (auth_verify_jwt(req->headers.authorization, "nnssf-nsselection", &claims) != 0)
        return respond_problem(resp, 401, "TOKEN_INVALID");
    if (!claims_audience_matches_self(&claims))
        return respond_problem(resp, 403, "NOT_AUTHORIZED");

    // 2. 헤더·Content-Type 검증 (api §5)
    if (!header_accept_supports(req->headers.accept, "application/json"))
        return respond_problem(resp, 406, NULL);

    // 3. 입력 모델 역직렬화 (M-MODEL, api §2.1 2-A)
    nssel_query_t q;
    int rc = nssel_query_parse(&req->query, &q);  // nf-type, nf-id, slice-info-for-* 등
    if (rc != 0)
        return respond_problem(resp, 400, "MANDATORY_QUERY_PARAM_MISSING", q.invalid_param);

    // 4. 의미 검증 (slice-info-for-* one-of, RSIPCE/SIOP feature gate)
    if (count_slice_info_variants(&q) > 1)
        return respond_problem(resp, 400, "SEMANTIC_ERROR");
    if (q.has_pdn_connection && !feature_negotiated(req, "RSIPCE"))
        return respond_problem(resp, 400, "FEATURE_NOT_SUPPORTED");
    if (q.has_other_purpose && !feature_negotiated(req, "SIOP"))
        return respond_problem(resp, 400, "FEATURE_NOT_SUPPORTED");

    // 5. db CRUD 호출 (M-DB; db-design §7 작업 인용)
    nssai_config_entry_t *cfg_entry = db_nssai_config_read_by_tai(&q.tai);  // ENT-0001 ReadByTai
    if (!cfg_entry)
        return respond_problem(resp, 403, "SNSSAI_NOT_SUPPORTED");
    amf_set_entry_t *amf_set = db_amf_set_lookup(&q.requested_snssai);     // ENT-0003
    authorized_network_slice_info_t result;
    int build_rc = build_authorized_nssi(&q, cfg_entry, amf_set, &result);
    if (build_rc == BUILD_NEED_NRF_DISCOVERY) {
        // MGMT-0001: NRF query for candidate AMF Set
        client_nrf_discover_amf(&q, on_nrf_discover_complete, /*ctx=*/resp);
        return ASYNC_PENDING;  // [PROC-0004]
    }

    // 6. 상태 전이 기록 (procedure-analysis §7: 상태 전이 없음 — read-only)

    // 7. 응답 직렬화 + 헤더
    cJSON *body = authorized_network_slice_info_to_json(&result);
    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    respond_ok(resp, 200, "application/json", body_str);
    free(body_str);
    authorized_network_slice_info_free(&result);

    // 8. 로그 (M-LOG INFO: nf-id, allowed/configured 요약)
    LOG_INFO(NSSEL, "PROC-0006~0011 ok nf_id=%s allowed_count=%zu", q.nf_id, result.n_allowed);

    return 0;
}
```

#### 4-B-2. M-HANDLER-2 handle_e2_dispatch (PUT /nssai-availability/{nfId})

```c
int handler_e2_dispatch(http_req_t *req, http_resp_t *resp) {
    // [기능 트레이스: SVC-0012, DAT-0013/0014, ERR-0010]

    // 1. auth
    jwt_claims_t claims;
    if (auth_verify_jwt(req->headers.authorization, "nnssf-nssaiavailability", &claims) != 0)
        return respond_problem(resp, 401, "TOKEN_INVALID");
    if (!claims_audience_matches_self(&claims))
        return respond_problem(resp, 403, "NOT_AUTHORIZED");

    // 2. header
    if (!ct_is(req->headers.content_type, "application/json"))
        return respond_problem(resp, 415, NULL);

    // 3. parse body
    nssai_availability_info_t info;
    if (nssai_availability_info_parse_buf(req->body, req->body_len, &info) != 0)
        return respond_problem(resp, 400, "MANDATORY_IE_MISSING");

    // 4. semantic: nfId 형식 검증, supported S-NSSAI 정책 확인
    nf_instance_id_t nf_id;
    if (parse_nf_id(req->path_params.nf_id, &nf_id) != 0)
        return respond_problem(resp, 400, "INVALID_IE", "nfId");
    if (!all_snssai_supported(&info)) {
        nssai_availability_info_free(&info);
        return respond_problem(resp, 403, "SNSSAI_NOT_SUPPORTED");
    }

    // 5. db CRUD (transaction T-AMF-UPSERT — db-design §8-A)
    authorized_nssai_availability_info_t authorized;
    int rc = db_tx_begin();
    rc |= db_amf_reg_upsert(&nf_id, &info);                          // ENT-0008 UpsertByNfId
    rc |= db_authorized_compute_and_store(&nf_id, &info, &authorized); // ENT-0009 Upsert
    if (rc != 0) {
        db_tx_rollback();
        nssai_availability_info_free(&info);
        return respond_problem(resp, 500, NULL);
    }
    db_tx_commit();
    // 트랜잭션 COMMIT 후 enqueue (commit 손실 방지)
    db_notify_enqueue_status_change(&nf_id, &info, &authorized);  // ENT-0015

    // 6. state transition (procedure §7: AMF Reg vN → vN+1)
    LOG_INFO(AVAIL, "PROC-0013 PUT ok nf_id=%s ta_count=%zu", nf_id_str(&nf_id), info.n_data);

    // 7. response
    if (authorized.n_data == 0) {
        nssai_availability_info_free(&info);
        authorized_nssai_availability_info_free(&authorized);
        return respond_no_content(resp, 204);
    }
    cJSON *body = authorized_nssai_availability_info_to_json(&authorized);
    char *s = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    respond_ok(resp, 200, "application/json", s);
    free(s);
    nssai_availability_info_free(&info);
    authorized_nssai_availability_info_free(&authorized);

    // 8. log (handled in step 6)
    return 0;
}
```

#### 4-B-3. M-HANDLER-3 handle_e3_dispatch (PATCH /nssai-availability/{nfId})

```c
int handler_e3_dispatch(http_req_t *req, http_resp_t *resp) {
    // [트레이스: SVC-0013, DAT-0015/0016, ERR-0010, DAT-0030]

    // 1. auth (E2 동일)
    // 2. Content-Type
    if (!ct_is(req->headers.content_type, "application/json-patch+json"))
        return respond_problem(resp, 415, NULL);

    // 3. parse PatchDocument
    patch_doc_t pd;
    if (patch_doc_parse_buf(req->body, req->body_len, &pd) != 0)
        return respond_problem(resp, 400, "INVALID_IE");
    if (pd.n == 0)
        return respond_problem(resp, 400, "MANDATORY_IE_MISSING");

    // 4. JSON Patch 무결성 검증 (each op enum, path JSON Pointer, value 의존성)
    int verr = patch_doc_validate(&pd);
    if (verr != 0) {
        patch_doc_free(&pd);
        return respond_problem(resp, 400, "INVALID_IE", verr_param(&pd, verr));
    }

    // 5. db tx (T-AMF-PATCH; per-nfId row lock SELECT ... FOR UPDATE)
    nf_instance_id_t nf_id;
    parse_nf_id(req->path_params.nf_id, &nf_id);
    db_tx_begin();
    nssai_availability_info_t current;
    int rc = db_amf_reg_read_for_update(&nf_id, &current);  // SELECT FOR UPDATE
    if (rc == DB_NOT_FOUND) {
        db_tx_rollback();
        patch_doc_free(&pd);
        return respond_problem(resp, 404, "RESOURCE_NOT_FOUND");
    }
    if (patch_doc_apply(&current, &pd) != 0) {
        db_tx_rollback();
        patch_doc_free(&pd);
        return respond_problem(resp, 400, "SEMANTIC_ERROR");
    }
    authorized_nssai_availability_info_t authorized;
    db_amf_reg_upsert(&nf_id, &current);
    db_authorized_compute_and_store(&nf_id, &current, &authorized);
    db_tx_commit();
    db_notify_enqueue_status_change(&nf_id, &current, &authorized);

    // 6. state transition recorded
    // 7. response (E2 동일 — 200 또는 204)
    // 8. log
    return respond_authorized(resp, &authorized);
}
```

#### 4-B-4. M-HANDLER-4 handle_e4_dispatch (DELETE /nssai-availability/{nfId})

```c
int handler_e4_dispatch(http_req_t *req, http_resp_t *resp) {
    // [트레이스: SVC-0030/0031, ERR-0015, DAT-0031]
    // 1. auth (NSSAIAvailability scope)
    // 2. (헤더 검증 없음 — body 없음)
    // 3-4. (parse·semantic 없음)

    // 5. db tx (T-AMF-DELETE)
    nf_instance_id_t nf_id;
    if (parse_nf_id(req->path_params.nf_id, &nf_id) != 0)
        return respond_problem(resp, 400, "INVALID_IE", "nfId");
    int rc = db_amf_reg_delete(&nf_id);  // FK CASCADE → ENT-0009
    if (rc == DB_NOT_FOUND)
        return respond_problem(resp, 404, "RESOURCE_NOT_FOUND");  // 또는 멱등 정책 시 204
    // 영향 받는 구독자에게 status change Notify enqueue
    db_notify_enqueue_amf_deleted(&nf_id);

    // 6. state transition: AMF Reg 존재 → 삭제
    // 7. 204 No Content
    LOG_INFO(AVAIL, "PROC-0019 ok nf_id=%s", nf_id_str(&nf_id));
    return respond_no_content(resp, 204);
}
```

#### 4-B-5. M-HANDLER-5 handle_e5_dispatch (POST /subscriptions)

```c
int handler_e5_dispatch(http_req_t *req, http_resp_t *resp) {
    // [트레이스: SVC-0014~0017, DAT-0017~0023, ERR-0011, DAT-0032, DAT-0033]
    // 1. auth
    jwt_claims_t claims;
    if (auth_verify_jwt(req->headers.authorization, "nnssf-nssaiavailability", &claims) != 0)
        return respond_problem(resp, 401, "TOKEN_INVALID");
    // 2. Content-Type
    if (!ct_is(req->headers.content_type, "application/json"))
        return respond_problem(resp, 415, NULL);
    // 3. parse
    nssf_event_sub_create_data_t in;
    if (nssf_event_sub_create_data_parse_buf(req->body, req->body_len, &in) != 0)
        return respond_problem(resp, 400, "MANDATORY_IE_MISSING");
    // 4. semantic: event enum, URI 형식, NSRP/NSIUN feature gate
    if (!is_valid_uri(in.nf_nssai_availability_uri))
        return respond_problem(resp, 400, "INVALID_IE", "nfNssaiAvailabilityUri");
    if (all_events_unsupported(&in)) {
        nssf_event_sub_create_data_free(&in);
        return respond_problem(resp, 501, "UNSUPPORTED_EVENT_TYPE");  // DAT-0033
    }
    if (in.has_nsrp && !feature_negotiated(req, "NSRP"))
        return respond_problem(resp, 403, "NOT_AUTHORIZED");
    // 5. db tx (T-SUB-CREATE)
    nssf_event_sub_created_data_t out;
    db_tx_begin();
    if (db_subscription_create(&in, claims.consumer_nf_id, &out) != 0) {
        db_tx_rollback();
        nssf_event_sub_create_data_free(&in);
        return respond_problem(resp, 500, NULL);
    }
    db_tx_commit();
    // 6. state transition: 구독 record 신규 생성
    // 7. 응답 (Location 헤더 필수)
    char location[512];
    snprintf(location, sizeof(location), "/nnssf-nssaiavailability/v1/nssai-availability/subscriptions/%s", out.subscription_id);
    cJSON *body = nssf_event_sub_created_data_to_json(&out);
    char *s = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    respond_created(resp, 201, "application/json", s, location);
    free(s);
    // 8. log
    LOG_INFO(SUB, "PROC-0014 create ok sub_id=%s consumer=%s", out.subscription_id, in.consumer_nf_id_str);
    nssf_event_sub_create_data_free(&in);
    nssf_event_sub_created_data_free(&out);
    return 0;
}
```

#### 4-B-6. M-HANDLER-6 handle_e6_dispatch (PATCH /subscriptions/{id})

```c
int handler_e6_dispatch(http_req_t *req, http_resp_t *resp) {
    // [트레이스: SVC-0018~0020, DAT-0024/0025, ERR-0012]
    // 1. auth
    // 2. Content-Type: application/json-patch+json
    if (!ct_is(req->headers.content_type, "application/json-patch+json"))
        return respond_problem(resp, 415, NULL);
    // 3. parse PatchDocument
    patch_doc_t pd;
    if (patch_doc_parse_buf(req->body, req->body_len, &pd) != 0)
        return respond_problem(resp, 400, "INVALID_IE");
    // 4. semantic:
    //    (a) SUMOD feature 협상 확인
    //    (b) event IE 변경 시도 거부 (SVC-0020 M-Not)
    if (!feature_negotiated(req, "SUMOD")) {
        patch_doc_free(&pd);
        return respond_problem(resp, 403, "NOT_AUTHORIZED");
    }
    for (size_t i = 0; i < pd.n; i++) {
        if (path_targets_event_ie(pd.items[i].path)) {
            patch_doc_free(&pd);
            return respond_problem(resp, 400, "MODIFICATION_NOT_ALLOWED");
        }
    }
    // 5. db tx (T-SUB-MODIFY)
    const char *sub_id = req->path_params.subscription_id;
    nssf_event_sub_created_data_t out;
    int rc = db_subscription_patch_by_id(sub_id, &pd, &out);  // ENT-0010 PatchById
    patch_doc_free(&pd);
    if (rc == DB_NOT_FOUND) return respond_problem(resp, 404, "SUBSCRIPTION_NOT_FOUND");
    if (rc != 0) return respond_problem(resp, 500, NULL);
    // 6. state transition: filter/expiry 갱신
    // 7. 200 OK + body
    return respond_ok_json(resp, 200, nssf_event_sub_created_data_to_json(&out));
}
```

#### 4-B-7. M-HANDLER-7 handle_e7_dispatch (DELETE /subscriptions/{id})

```c
int handler_e7_dispatch(http_req_t *req, http_resp_t *resp) {
    // [트레이스: SVC-0021/0022, ERR-0013]
    // 1. auth
    // 2-4. 검증 없음
    // 5. db tx
    const char *sub_id = req->path_params.subscription_id;
    int rc = db_subscription_delete_by_id(sub_id);  // ENT-0010 DeleteById
    if (rc == DB_NOT_FOUND) return respond_problem(resp, 404, "SUBSCRIPTION_NOT_FOUND");
    // 6. state transition: 구독 삭제
    // 7. 204
    LOG_INFO(SUB, "PROC-0018 delete ok sub_id=%s", sub_id);
    return respond_no_content(resp, 204);
}
```

#### 4-B-8. M-HANDLER-8 handle_e8_dispatch (OPTIONS /nssai-availability)

```c
int handler_e8_dispatch(http_req_t *req, http_resp_t *resp) {
    // [트레이스: SVC-0032, ERR-0016]
    // 1. auth (운영 정책: OPTIONS 면제 가능, 본 구현은 면제)
    // 2-6. 해당 없음
    // 7. 200 OK + Accept-Encoding 헤더
    http_resp_set_header(resp, "Accept-Encoding", "gzip");
    return respond_no_body(resp, 200);
    // 8. log DEBUG
}
```

## 5. Consumer 클라이언트

### 5-A. 클라이언트 카탈로그

| 클라이언트 모듈 | 호출 대상 | 매핑 절차 | 트리거 | 재시도 정책 |
|---|---|---|---|---|
| M-CLIENT-NRF.register | NRF NFManagement_NFRegister | PROC-0001 | T-STATE (boot) | 5xx 백오프 (기동 시 무한 재시도, 운영 알람) |
| M-CLIENT-NRF.deregister | NRF NFManagement_NFDeregister | PROC-0002 | T-STATE (shutdown) | 1회 시도, 실패 시 강제 종료 |
| M-CLIENT-NRF.heartbeat | NRF NFManagement_NFUpdate (PATCH) | PROC-0003 | T-TIME | 5xx 백오프, 404 → register 재트리거 |
| M-CLIENT-NRF.discover | NRF NFDiscovery (GET) | PROC-0004 | T-MSG (E1 핸들러 내부) | 5xx 백오프 1회, 실패 시 partial response |
| M-CLIENT-NRF.token | NRF AccessToken_Get | PROC-0005 | T-MSG, T-STATE | 5xx 백오프, 4xx 거부 (운영 알람) |
| M-CLIENT-NOTIFY.send | Consumer (callback URI) | PROC-0015/0016/0017 | T-STATE / T-EVT | api §6.8 `Retry-After` 인용 + 지수 백오프 (`http_cli.max_retries`) |
| M-CLIENT-SEPP.forward | H-NSSF via SEPP (N32-f) | PROC-0012 | T-MSG | 5xx 백오프 1회 |

### 5-B. 클라이언트 의사 코드

#### 5-B-1. M-CLIENT-NRF.register (PROC-0001)

```c
void client_nrf_register(const nf_profile_t *profile, on_register_cb cb, void *user) {
    // [절차 트레이스: PROC-0001]
    // 1. 토큰 — NRF NFM 등록은 OAuth2 면제 (TS 33.501) 또는 사전 협상; 본 구현은 mTLS only
    // 2. 요청 빌드
    char url[512];
    snprintf(url, sizeof(url), "%s/nnrf-nfm/v1/nf-instances/%s", g_cfg.nrf.base_url, profile->nf_instance_id);
    cJSON *body = nf_profile_to_json(profile);
    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    http_req_t req = {
        .method = "PUT",
        .url = url,
        .headers = {.content_type = "application/json"},
        .body = body_str,
        .body_len = strlen(body_str)
    };
    // 3. HTTP/2 호출
    http_cli_send(&req, on_register_response, /*ctx=*/cb);
    free(body_str);
}

static void on_register_response(const http_resp_t *resp, void *ctx) {
    on_register_cb cb = ctx;
    // 4. 상태 분기
    switch (resp->status) {
        case 201:
        case 200: {
            nf_profile_t profile;
            nf_profile_parse_buf(resp->body, resp->body_len, &profile);
            db_nrf_reg_mark_registered(&profile);  // ENT-0016
            cb(0, &profile);
            break;
        }
        case 401: case 403:
            LOG_ERROR(NRF, "PROC-0001 auth fail status=%d", resp->status);
            cb(ERR_NRF_AUTH, NULL);
            break;
        case 400:
            LOG_ERROR(NRF, "PROC-0001 invalid profile");
            cb(ERR_NRF_INVALID, NULL);
            break;
        case 429: case 503:
            // 5. 재시도 (Retry-After 인용, 지수 백오프)
            schedule_retry(client_nrf_register_retry, resp->headers.retry_after_sec);
            break;
        case 500: case 502:
            schedule_retry(client_nrf_register_retry, /*default=*/5);
            break;
        default:
            LOG_ERROR(NRF, "PROC-0001 unexpected status=%d", resp->status);
            cb(ERR_NRF_UNEXPECTED, NULL);
    }
}
```

#### 5-B-2. M-CLIENT-NRF.heartbeat (PROC-0003)

```c
void client_nrf_heartbeat(void) {
    // [PROC-0003]
    nf_instance_id_t *nf_id = &g_cfg.nf_instance_id;
    patch_doc_t pd = make_heartbeat_patch();  // [{op:"replace", path:"/nfStatus", value:"REGISTERED"}]
    char *body_str = patch_doc_to_json_str(&pd);
    char url[512];
    snprintf(url, sizeof(url), "%s/nnrf-nfm/v1/nf-instances/%s", g_cfg.nrf.base_url, nf_id_str(nf_id));
    http_req_t req = {.method="PATCH", .url=url, .headers={.content_type="application/json-patch+json"}, .body=body_str, .body_len=strlen(body_str)};
    http_cli_send(&req, on_heartbeat_response, NULL);
    free(body_str);
}

static void on_heartbeat_response(const http_resp_t *resp, void *ctx) {
    switch (resp->status) {
        case 204:
            db_nrf_reg_update_heartbeat();
            break;
        case 404:
            // 재등록
            LOG_WARN(NRF, "PROC-0003 NRF lost reg, re-registering");
            db_nrf_reg_mark_unregistered();
            client_nrf_register(&g_self_profile, on_reregister_complete, NULL);
            break;
        case 401:
            // (NFM 은 일반적으로 OAuth2 비적용; 적용 시 token 재발급)
            break;
        case 429: case 5xx_pattern:
            schedule_retry(client_nrf_heartbeat, /*backoff=*/exp_backoff());
            break;
    }
}
```

#### 5-B-3. M-CLIENT-NRF.deregister (PROC-0002)

```c
void client_nrf_deregister(on_deregister_cb cb) {
    // [PROC-0002]
    char url[512];
    snprintf(url, sizeof(url), "%s/nnrf-nfm/v1/nf-instances/%s", g_cfg.nrf.base_url, nf_id_str(&g_cfg.nf_instance_id));
    http_req_t req = {.method="DELETE", .url=url};
    http_cli_send(&req, on_deregister_response, cb);
}

static void on_deregister_response(const http_resp_t *resp, void *ctx) {
    on_deregister_cb cb = ctx;
    if (resp->status == 204) {
        db_nrf_reg_mark_unregistered();
        cb(0);
    } else {
        LOG_WARN(NRF, "PROC-0002 dereg failed status=%d, force-exit", resp->status);
        cb(ERR_NRF_DEREG);
    }
}
```

#### 5-B-4. M-CLIENT-NRF.discover (PROC-0004)

```c
void client_nrf_discover_amf(const nssel_query_t *q, on_discover_cb cb, void *ctx) {
    // [PROC-0004]
    char *bearer; time_t expires;
    auth_get_token("NRF", "nnrf-disc", &bearer, &expires);  // [PROC-0005 인용]
    char url[1024];
    snprintf(url, sizeof(url), "%s/nnrf-disc/v1/nf-instances?target-nf-type=AMF&requester-nf-type=NSSF&amf-set-id=%s&snssais=%s", g_cfg.nrf.base_url, q->amf_set_hint, q->snssai_query_str);
    http_req_t req = {.method="GET", .url=url, .headers={.authorization=bearer}};
    http_cli_send(&req, on_discover_response, ctx);
    free(bearer);
}

static void on_discover_response(const http_resp_t *resp, void *ctx) {
    switch (resp->status) {
        case 200: {
            search_result_t sr;
            search_result_parse_buf(resp->body, resp->body_len, &sr);
            on_discover_cb cb = ((discover_ctx_t*)ctx)->cb;
            cb(0, &sr);
            break;
        }
        case 401:
            auth_invalidate_token("NRF", "nnrf-disc");
            schedule_retry(client_nrf_discover_amf_retry, 0);
            break;
        case 5xx_pattern:
            // partial response 정책: NSSelection_Get 응답에 candidateAmfList 생략
            ((discover_ctx_t*)ctx)->cb(ERR_NRF_DISC_FAIL, NULL);
            break;
    }
}
```

#### 5-B-5. M-CLIENT-NRF.token (PROC-0005)

```c
int auth_get_token(const char *target_nf_type, const char *scope, char **out_bearer, time_t *out_expires) {
    // [PROC-0005]
    // 1. cache lookup
    token_entry_t *entry = token_cache_get(target_nf_type, scope);  // ENT-0013 GetOrFetch
    if (entry && entry->expires_at > now() + g_cfg.auth.oauth2.token_cache_ttl_margin_sec) {
        *out_bearer = strdup(entry->token);
        *out_expires = entry->expires_at;
        return 0;
    }
    // 2-3. 발급
    char body[512];
    int n = snprintf(body, sizeof(body),
        "grant_type=client_credentials&nfInstanceId=%s&nfType=NSSF&targetNfType=%s&scope=%s",
        nf_id_str(&g_cfg.nf_instance_id), target_nf_type, scope);
    char url[512];
    snprintf(url, sizeof(url), "%s/oauth2/token", g_cfg.nrf.base_url);
    http_req_t req = {.method="POST", .url=url, .headers={.content_type="application/x-www-form-urlencoded"}, .body=body, .body_len=n};
    http_resp_t resp;
    int rc = http_cli_send_sync(&req, &resp);  // 동기 호출 (boot 시점) 또는 비동기 + future
    // 4. 분기
    if (resp.status == 200) {
        access_token_resp_t tok;
        access_token_resp_parse_buf(resp.body, resp.body_len, &tok);
        token_cache_put(target_nf_type, scope, NULL, tok.access_token, now() + tok.expires_in);
        *out_bearer = strdup(tok.access_token);
        *out_expires = now() + tok.expires_in;
        return 0;
    }
    if (resp.status == 400 || resp.status == 401) return ERR_AUTH_INVALID;
    return ERR_AUTH_RETRIABLE;
}
```

#### 5-B-6. M-CLIENT-NOTIFY.send (PROC-0015/0016/0017)

```c
void client_notify_send(notify_task_t *task) {
    // [PROC-0015/0016/0017]
    // 1. token (Consumer 측 NRF; 본 구현은 동일 NRF 가정 + targetNfType from subscription consumer)
    char *bearer; time_t expires;
    if (auth_get_token(task->consumer_nf_type, task->consumer_scope, &bearer, &expires) != 0) {
        notify_task_mark_retry(task, exp_backoff(task->attempts));
        return;
    }
    // 2. 요청 빌드
    http_req_t req = {
        .method = "POST",
        .url = task->callback_uri,
        .headers = {.authorization = bearer, .content_type = "application/json"},
        .body = task->payload_json,
        .body_len = strlen(task->payload_json)
    };
    // 3. 송신
    http_cli_send(&req, on_notify_response, task);
    free(bearer);
}

static void on_notify_response(const http_resp_t *resp, void *ctx) {
    notify_task_t *task = ctx;
    // 4. 분기 (api §6.8 매트릭스)
    switch (resp->status) {
        case 204:
            db_subscription_mark_notified(task->subscription_id);
            db_notify_mark_success(task->task_id);
            break;
        case 307: case 308: {
            const char *loc = resp->headers.location;
            strncpy(task->callback_uri, loc, sizeof(task->callback_uri));
            schedule_retry(client_notify_send, /*backoff=*/0);  // 즉시 redirect
            break;
        }
        case 400: {  // RESOURCE_CONTEXT_NOT_FOUND
            db_subscription_mark_stale(task->subscription_id);
            db_notify_mark_permanent_fail(task->task_id);
            LOG_WARN(NOTIFY, "PROC-0015 stale sub_id=%s", task->subscription_id);
            break;
        }
        case 404: {  // RESOURCE_URI_STRUCTURE_NOT_FOUND
            db_subscription_invalidate(task->subscription_id);
            db_notify_mark_permanent_fail(task->task_id);
            break;
        }
        case 401: case 403:
            auth_invalidate_token(task->consumer_nf_type, task->consumer_scope);
            notify_task_mark_retry(task, exp_backoff(task->attempts));
            break;
        case 429: case 503:
            notify_task_mark_retry(task, resp->headers.retry_after_sec);
            break;
        case 500: case 502:
            notify_task_mark_retry(task, exp_backoff(task->attempts));
            break;
        default:
            notify_task_mark_retry(task, exp_backoff(task->attempts));
    }
    // 5. state transition (subscription record lastNotifiedAt 갱신은 위 db_subscription_mark_notified 에서)
    if (task->attempts >= g_cfg.http_cli.max_retries) {
        db_subscription_mark_stale(task->subscription_id);
        db_notify_mark_permanent_fail(task->task_id);
    }
}
```

#### 5-B-7. M-CLIENT-SEPP.forward (PROC-0012)

```c
void client_sepp_forward(const http_req_t *orig_req, on_sepp_cb cb, void *ctx) {
    // [PROC-0012]
    // 1. token (Consumer 측 PLMN scope)
    char *bearer; time_t exp;
    auth_get_token("H-NSSF", "nnssf-nsselection", &bearer, &exp);
    // 2. N32-f forward (SEPP 가 routing — `3gpp-Sbi-Target-apiRoot` 헤더로 H-NSSF 지정)
    http_req_t fwd = *orig_req;
    fwd.url = build_sepp_url(orig_req->url);  // SEPP 통과
    fwd.headers.target_api_root = g_cfg.nrf.h_plmn_nssf_url;  // 3gpp-Sbi-Target-apiRoot
    fwd.headers.authorization = bearer;
    // 3. send
    http_cli_send(&fwd, on_sepp_response, ctx);
    free(bearer);
}

static void on_sepp_response(const http_resp_t *resp, void *ctx) {
    // 4. 분기 (E1 응답 분기와 동일)
    on_sepp_cb cb = ((sepp_ctx_t*)ctx)->cb;
    if (resp->status == 200) cb(0, resp);
    else if (resp->status == 502 || resp->status == 504) cb(ERR_SEPP_FORWARDING_FAILED, NULL);
    else cb(resp->status, resp);
    // 5. state: 변경 없음 (read-through)
}
```

## 6. db CRUD 호출 매핑

| 호출 위치 | db-design 작업명 | 엔티티 ID | 트랜잭션 그룹 | 동시성 |
|---|---|---|---|---|
| M-LIFE.life_init | `LoadFromConfigFile` | ENT-0001~0007 | – (boot) | exclusive |
| M-LIFE.life_init | `LoadOnBoot` | ENT-0016 | – | exclusive |
| M-HANDLER-1 | `ReadByTai` | ENT-0001 | – | read-lock |
| M-HANDLER-1 | `ReadByKey` (AMF set) | ENT-0003 | – | read-lock |
| M-HANDLER-1 | `ReadByKey` (NSI) | ENT-0004 | – | read-lock |
| M-HANDLER-1 | `ReadByKey` (VPLMN↔HPLMN) | ENT-0006 | – | read-lock |
| M-HANDLER-2/3 | `UpsertByNfId` / `PatchByNfId` | ENT-0008 | T-AMF-UPSERT / T-AMF-PATCH | serializable |
| M-HANDLER-2/3 | `UpsertByKey` | ENT-0009 | 동 트랜잭션 | serializable |
| M-HANDLER-2/3 | `Enqueue` | ENT-0015 | (트랜잭션 COMMIT 이후) | serializable |
| M-HANDLER-4 | `DeleteByNfId` | ENT-0008 (FK CASCADE → ENT-0009) | T-AMF-DELETE | serializable |
| M-HANDLER-4 | `Enqueue` (status change) | ENT-0015 | (commit 이후) | serializable |
| M-HANDLER-5 | `Create` | ENT-0010 | T-SUB-CREATE | serializable |
| M-HANDLER-6 | `PatchById` | ENT-0010 | T-SUB-MODIFY | serializable |
| M-HANDLER-7 | `DeleteById` | ENT-0010 | – | serializable |
| M-CLIENT-NOTIFY.send | `MarkSuccess`/`MarkRetry`/`MarkPermanentFail` | ENT-0015 | – | serializable |
| M-CLIENT-NOTIFY.send | `mark_stale`/`invalidate`/`mark_notified` | ENT-0010 | – | serializable |
| M-CLIENT-NRF.register/heartbeat/deregister | `MarkRegistered`/`UpdateHeartbeat`/`MarkUnregistered` | ENT-0016 | T-NRF-REG | RWL |
| M-CLIENT-NRF.token | `GetOrFetch`/`Invalidate` | ENT-0013 | – | lock-free |
| M-CLIENT-NRF.discover | (db 호출 없음 — read-through) | – | – | – |
| M-PROC-0016 (Slice Replacement) | `StartReplacement`/`StopReplacement`/`TerminateReplacement` | ENT-0012 | T-REPLACE-START | serializable |
| M-PROC-0017 (Validity Time) | `UpsertBySnssai` | ENT-0011 | – | RWL |
| M-PROC-0020 (Expiry sweeper) | `ExpireDueAt` | ENT-0010 | T-SUB-EXPIRE | serializable |

## 7. 에러 처리 분기 (api-analysis §6 인용)

> 본 표는 api-analysis §6.1~6.8 매트릭스를 인용하며 신규 cause 를 생성하지 않음.

| 엔드포인트 | HTTP·cause | 코드 분기 | 로그 레벨 | retriable |
|---|---|---|---|---|
| E1 | 401 TOKEN_* | M-HANDLER-1 step 1 → respond_problem(401) | WARN | Yes |
| E1 | 403 SNSSAI_NOT_SUPPORTED | M-HANDLER-1 step 5 (db lookup 실패) | INFO | No |
| E1 | 400 MANDATORY_QUERY_PARAM_MISSING | M-HANDLER-1 step 3 | WARN | No |
| E1 | 5xx | M-HANDLER-1 step 5 (db 오류) → respond_problem(500/502/503) | ERROR | Yes |
| E2/E3 | 415 | M-HANDLER-2/3 step 2 | WARN | No |
| E2/E3 | 400 MANDATORY_IE_MISSING / INVALID_IE | step 3 | WARN | No |
| E2/E3 | 403 SNSSAI_NOT_SUPPORTED / NOT_AUTHORIZED | step 4 | INFO/WARN | No |
| E3 | 404 RESOURCE_NOT_FOUND | step 5 (PATCH 대상 미존재) | WARN | No |
| E3 | 400 SEMANTIC_ERROR (Patch 무결성) | step 4 | WARN | No |
| E4 | 404 RESOURCE_NOT_FOUND | step 5 | INFO | No |
| E5 | 501 UNSUPPORTED_EVENT_TYPE | M-HANDLER-5 step 4 | INFO | No |
| E5 | 400 INVALID_IE (nfNssaiAvailabilityUri) | step 4 | WARN | No |
| E6 | 400 MODIFICATION_NOT_ALLOWED | M-HANDLER-6 step 4 (event IE) | WARN | No |
| E6 | 404 SUBSCRIPTION_NOT_FOUND | step 5 | INFO | No |
| E7 | 404 SUBSCRIPTION_NOT_FOUND | M-HANDLER-7 step 5 | INFO | No |
| E8 | 4xx/5xx 표준 | M-HANDLER-8 (대부분 5xx 만 발생) | WARN/ERROR | Yes (5xx) |
| C1 (NSSF→Consumer 응답) | 400 RESOURCE_CONTEXT_NOT_FOUND | M-CLIENT-NOTIFY on_notify_response 400 case | WARN | No (구독 stale) |
| C1 | 404 RESOURCE_URI_STRUCTURE_NOT_FOUND | 동 404 case | WARN | No (구독 invalidate) |
| C1 | 5xx/429 | retry case | ERROR/WARN | Yes |

## 8. 메모리·동시성·로깅·기동/종료

### 8-A. 메모리 소유권 (C — 명시적 메모리 모델)

| 데이터 | 할당 시점 | 소유권 이전 | 해제 책임 | 비고 |
|---|---|---|---|---|
| `nssf_config_t *cfg` | `cfg_load` (heap) | M-MAIN → M-LIFE → 정적 전역 | M-LIFE.shutdown 또는 process exit | 단일 인스턴스 |
| cJSON `cJSON_Parse` 결과 | parser | parser 내부 | parser 끝에 `cJSON_Delete` | parser caller 가 즉시 free |
| 모델 struct (`snssai_t`, `nssai_availability_info_t` 등) | `<model>_parse` (heap of inner pointers) | request scope | handler 가 `<model>_free` | request 종료 시 |
| HTTP 응답 body 문자열 | `cJSON_PrintUnformatted` | request scope | handler 가 `free` | – |
| db row buffer | libpq `PQgetvalue` 인접 메모리 | libpq 가 보유 | libpq `PQclear(res)` | snapshot 처리 후 즉시 clear |
| uthash entry | `db_<entity>_*` insert 시 | uthash 가 보유 | `db_<entity>_evict` 또는 shutdown 시 `HASH_ITER + HASH_DEL + free` | – |
| token cache token string | `auth_get_token` | uthash | shutdown 또는 `Invalidate` | strdup |
| notify_task payload | `db_notify_enqueue` | uthash | `MarkSuccess`/`MarkPermanentFail` | – |
| libcurl easy handle | M-HTTP-CLI 풀에서 `curl_easy_init` | M-HTTP-CLI 풀 보유 | shutdown 시 `curl_easy_cleanup` | – |
| OpenSSL `SSL_CTX` | M-AUTH 또는 M-HTTP-SRV/CLI 초기화 | 모듈 전역 | shutdown 시 `SSL_CTX_free` | – |
| zlog category | `zlog_get_category` | zlog 내부 | shutdown 시 `zlog_fini` | – |

### 8-B. 동시성·스레드 모델

| 흐름 | 실행 컨텍스트 | 동시성 보장 | 락 정책 |
|---|---|---|---|
| HTTP 요청 처리 (M-HANDLER-*) | libuv 메인 이벤트 루프 (단일 스레드) | serializable (libuv loop 단일 스레드) | 락 불필요 (libuv 자체 직렬화) |
| DB I/O (libpq) | libuv worker pool (별도 스레드) — `uv_queue_work` | serializable per-row (PG 트랜잭션) | PG row lock + 결과는 메인 루프에서 callback |
| HTTP 클라이언트 (libcurl multi) | libuv 메인 루프 (`curl_multi_socket_action` + libuv timer/poll handle) | serializable (메인 루프) | – |
| uthash 조회/갱신 | libuv 메인 루프 | serializable | 락 불필요 |
| Notify 큐 dequeue/dispatch | libuv timer 콜백 (메인 루프) | serializable | – |
| Subscription expiry sweep | libuv timer (주기, 메인 루프) | serializable | – |
| OAuth2 token 발급 | libuv 메인 루프 비동기 | lock-free (atomic insert in uthash) | double-checked locking 불필요 (단일 스레드) |
| 시그널 처리 | libuv `uv_signal_t` (메인 루프) | – | – |

**스레드 모델 결론**: NSSF 는 *단일 메인 이벤트 루프* 모델 (libuv). DB I/O 만 worker pool 로 위임. 모든 공유 상태(uthash·구독·큐) 는 메인 루프에서만 접근 → **락 불필요**. 다중 코어 활용은 *프로세스 단위 수평 확장*(NSSF set + 로드밸런서) 으로 처리 (NSSF 인스턴스 1개 = 1 메인 스레드 + N worker).

### 8-C. 로깅·관측

zlog 카테고리 + 표준 출력 필드.

| 카테고리 | 레벨 | 출력 필드 | 매핑 OBS 행 |
|---|---|---|---|
| `NSSEL` | INFO/WARN/ERROR | timestamp, nf_id, tai, requested_snssai, allowed_count, status, 3gpp-Sbi-* | – (features.md OBS 0건) |
| `AVAIL` | INFO/WARN/ERROR | nf_id, ta_count, change_summary, transaction_id | – |
| `SUB` | INFO/WARN | subscription_id, consumer_nf, event, tai_filter, expiry | – |
| `NOTIFY` | INFO/WARN/ERROR | subscription_id, callback_uri, attempts, http_status, retry_after | – |
| `NRF` | INFO/WARN/ERROR | request_id, target_uri, http_status, latency_ms | – |
| `AUTH` | DEBUG/WARN/ERROR | scope, aud, token_cache_hit, jwt_claims_summary | – |
| `HTTP` | DEBUG/INFO | method, path, status, latency_ms, 3gpp-Sbi-Sender-Timestamp delta | – |
| `DB` | DEBUG/WARN/ERROR | operation, entity, rows_affected, latency_ms | – |
| `LIFE` | INFO/ERROR | phase (init/run/shutdown), step, error | – |

> features.md `OBS` 카테고리 0건이므로 본 NF 의 OBS 요구사항은 *운영 권고 수준*. structured log 출력은 운영 모니터링(Grafana/ELK 연동)에서 활용.

#### 8-C-1. log conf 본문 명세 (R11)

zlog conf 파일 형식 (`tech_stack.logging = zlog` 의 표준).

##### 운영 변형 (`/etc/nssfd/log.conf`)

```ini
[global]
strict init = true
buffer min = 1024
buffer max = 2MB
rotate lock file = /var/run/nssfd/zlog.lock
default format = "%d.%us [%V] [%c] %m%n"
file perms = 0640

[levels]
TRACE = 10, LOG_DEBUG
NOTICE = 65, LOG_INFO

[formats]
json = "{\"ts\":\"%d.%us\",\"level\":\"%V\",\"cat\":\"%c\",\"file\":\"%F\",\"line\":%L,\"msg\":\"%m\"}%n"
default_text = "%d.%us [%V] [%c:%F:%L] %m%n"

[rules]
NSSEL.INFO         /var/log/nssfd/nssel.log; json
NSSEL.WARN         /var/log/nssfd/nssel.log; json
NSSEL.ERROR        /var/log/nssfd/nssel-error.log; json
AVAIL.INFO         /var/log/nssfd/avail.log; json
AVAIL.WARN         /var/log/nssfd/avail.log; json
AVAIL.ERROR        /var/log/nssfd/avail-error.log; json
SUB.INFO           /var/log/nssfd/sub.log; json
SUB.WARN           /var/log/nssfd/sub.log; json
NOTIFY.INFO        /var/log/nssfd/notify.log; json
NOTIFY.WARN        /var/log/nssfd/notify.log; json
NOTIFY.ERROR       /var/log/nssfd/notify-error.log; json
NRF.INFO           /var/log/nssfd/nrf.log; json
NRF.WARN           /var/log/nssfd/nrf.log; json
NRF.ERROR          /var/log/nssfd/nrf-error.log; json
AUTH.WARN          /var/log/nssfd/auth.log; json
AUTH.ERROR         /var/log/nssfd/auth-error.log; json
HTTP.INFO          /var/log/nssfd/http.log; json
DB.WARN            /var/log/nssfd/db.log; json
DB.ERROR           /var/log/nssfd/db-error.log; json
LIFE.INFO          /var/log/nssfd/life.log; default_text
LIFE.ERROR         /var/log/nssfd/life-error.log; default_text
```

##### dev 변형 (`dev/conf/samples/log.dev.conf`)

```ini
[global]
strict init = false
default format = "%d.%us [%V] [%c:%F:%L] %m%n"

[rules]
*.DEBUG  >stdout
*.INFO   >stdout
*.WARN   >stderr
*.ERROR  >stderr
```

### 8-D. 기동·종료 절차

#### 8-D-1. Entry function completeness 표 (R7)

| STEP | 책임 | expected_call | 책임 task (plan §5) | code 위치 | unit_test_id | integration_test_id |
|---|---|---|---|---|---|---|
| 1 | argv 파싱 + cfg 로드 | M-CFG.cfg_load + cfg_validate | TASK-NNNN (plan §5 에서 부여) | `dev/src/main.c:main` 초입 | TC-NNNN | TC-NNNN |
| 2 | log init | M-LOG.log_init | TASK-NNNN | `dev/src/main.c` → M-LIFE.life_init | TC-NNNN | TC-NNNN |
| 3 | DB 초기화 (PG 연결 + uthash 초기화 + Runtime persistent → in-memory mirror) | M-DB.db_init | TASK-NNNN | M-LIFE.life_init | TC-NNNN | TC-NNNN |
| 4 | auth init (TLS 인증서 로드 + JWT pubkey 로드 + token cache 초기화) | M-AUTH.auth_init | TASK-NNNN | M-LIFE.life_init | TC-NNNN | TC-NNNN |
| 5 | HTTP 서버 시작 (라우터 등록 + listen) | M-HTTP-SRV.http_srv_start + http_srv_route_register × 8 | TASK-NNNN | M-LIFE.life_init | TC-NNNN | TC-NNNN |
| 6 | HTTP 클라이언트 풀 시작 (libcurl multi + libuv wire) | M-HTTP-CLI.http_cli_start | TASK-NNNN | M-LIFE.life_init | TC-NNNN | TC-NNNN |
| 7 | 외부 NF 등록 (NRF NFRegister) | M-CLIENT-NRF.client_nrf_register (PROC-0001) | TASK-NNNN | M-LIFE.life_init step 7 | TC-NNNN | TC-NNNN |
| 8 | 백그라운드 타이머 wire (heartbeat / notify dispatcher / subscription expiry sweeper / token cache PurgeExpired) | M-LIFE.life_timer_wire (libuv `uv_timer_t`) | TASK-NNNN | M-LIFE.life_init step 8 | TC-NNNN | TC-NNNN |
| 9 | signal_wait (uv_signal_t SIGTERM/SIGINT/SIGHUP) | M-LIFE.life_signal_wait + life_run (`uv_run(loop, UV_RUN_DEFAULT)`) | TASK-NNNN | `dev/src/main.c:main` 메인 호출 | TC-NNNN | TC-NNNN |
| 10 | graceful_shutdown (NRF dereg → cancel sub timers → drain in-flight → close clients/server → flush logs → close db → free cfg) | M-LIFE.life_shutdown | TASK-NNNN | signal callback 또는 critical error 시 | TC-NNNN | TC-NNNN |

> task_id 및 TC-NNNN 은 `/generate-plan` 단계에서 부여. 본 표는 anchor — plan §5 와 test-spec 가 1:1 인용.

#### 8-D-2. 기동 순서 의사 코드

```c
int main(int argc, char **argv) {
    // STEP 1: argv + cfg
    const char *cfg_path = (argc > 1) ? argv[1] : "/etc/nssfd/nssfd.json";
    nssf_config_t *cfg;
    if (cfg_load(cfg_path, &cfg) != 0) { fprintf(stderr, "cfg load fail\n"); return 1; }
    if (cfg_validate(cfg) != 0)         { fprintf(stderr, "cfg invalid\n"); cfg_free(cfg); return 1; }

    life_cfg_t life;
    cfg_to_life_cfg(cfg, &life);

    // STEP 2-8: life_init 안에서
    if (life_init(&life) != 0) { cfg_free(cfg); return 1; }

    // STEP 9: run
    int rc = life_run();

    // STEP 10: shutdown
    life_shutdown();
    cfg_free(cfg);
    return rc;
}

int life_init(const life_cfg_t *cfg) {
    // STEP 2
    if (log_init(cfg->log_conf_path) != 0) return ERR_LIFE_LOG;
    LOG_INFO(LIFE, "step 2 log ok");

    // STEP 3
    if (db_init(cfg->db_conninfo) != 0) { log_shutdown(); return ERR_LIFE_DB; }
    LOG_INFO(LIFE, "step 3 db ok");

    // STEP 4
    if (auth_init(&cfg->auth) != 0) { db_shutdown(); log_shutdown(); return ERR_LIFE_AUTH; }
    LOG_INFO(LIFE, "step 4 auth ok");

    // STEP 5
    g_loop = uv_default_loop();
    if (http_srv_start(&cfg->http_srv, g_loop) != 0) goto fail;
    register_all_routes();  // 8 endpoints (E1~E8) 라우팅 등록
    LOG_INFO(LIFE, "step 5 http_srv ok port=%d", cfg->http_srv.bind_port);

    // STEP 6
    if (http_cli_start(&cfg->http_cli, g_loop) != 0) goto fail;
    LOG_INFO(LIFE, "step 6 http_cli ok");

    // STEP 7
    nf_profile_t profile = build_self_profile(cfg);
    if (client_nrf_register_blocking(&profile, NRF_REG_TIMEOUT_SEC) != 0) goto fail;
    LOG_INFO(LIFE, "step 7 nrf registered");

    // STEP 8
    life_timer_wire(g_loop);
    LOG_INFO(LIFE, "step 8 timers wired");

    return 0;
fail:
    LOG_ERROR(LIFE, "init failed");
    life_shutdown();
    return ERR_LIFE_INIT;
}

int life_run(void) {
    // STEP 9
    uv_signal_t sig_term, sig_int, sig_hup;
    uv_signal_init(g_loop, &sig_term); uv_signal_start(&sig_term, on_signal, SIGTERM);
    uv_signal_init(g_loop, &sig_int);  uv_signal_start(&sig_int,  on_signal, SIGINT);
    uv_signal_init(g_loop, &sig_hup);  uv_signal_start(&sig_hup,  on_signal_reload, SIGHUP);
    LOG_INFO(LIFE, "step 9 signal wait");
    return uv_run(g_loop, UV_RUN_DEFAULT);
}

void life_shutdown(void) {
    // STEP 10
    LOG_INFO(LIFE, "step 10 shutdown begin");
    int rc = client_nrf_deregister_blocking(DEREGISTER_TIMEOUT_SEC);  // PROC-0002
    LOG_INFO(LIFE, "  10a nrf_dereg rc=%d", rc);
    life_timer_cancel_all();
    LOG_INFO(LIFE, "  10b timers cancelled");
    http_srv_shutdown();
    http_cli_shutdown();
    LOG_INFO(LIFE, "  10c http closed");
    auth_shutdown();
    db_shutdown();
    LOG_INFO(LIFE, "  10d auth/db closed");
    log_shutdown();
}
```

##### 8-D-2-1. 단계별 실패 처리 정책

| STEP | 실패 시 동작 |
|---|---|
| 1 | stderr 출력, `exit(1)` |
| 2 | stderr, `exit(1)` |
| 3 | log ERROR, log shutdown, `exit(1)` |
| 4 | log ERROR, db shutdown, log shutdown, `exit(1)` |
| 5 | log ERROR, auth/db/log shutdown, `exit(1)` |
| 6 | http_srv shutdown 후 위와 동일 |
| 7 | NRF 등록 실패 → 운영 알람 + `exit(1)` (기동 거부). 재시도 정책은 운영 시작 스크립트(`systemd` Restart) 가 처리 |
| 8 | log ERROR, 위와 동일 |
| 9 | 정상 종료 또는 시그널 |
| 10 | 각 sub-step 실패는 로그 후 다음 단계 진행 (강제 종료 보장) |

### 8-E. 운영 conf bundle anchor (R11)

#### 8-E-1. 산출 의무 표

| 산출 절 | 의무 내용 | 입력 인용 | 페어 task |
|---|---|---|---|
| §3-C-5 cfg 페이로드 예시 | `nssf_config_t` 10 top-level fields 의 운영 + dev JSON 2종 + 영역별 매트릭스 | db-design §8-C 시드 + §3-C M-CFG schema + tech_stack DB/HTTP/AUTH | plan §5 TASK-cfg-example |
| §3-C-6 TLS 운영 정책 | `auth.*_path` 파일명 컨벤션 + `.gitignore` + 더미 발급 절차 + mTLS 활성 조건 + 만료/갱신 | M-AUTH schema + tech_stack OpenSSL | plan §5 TASK-gen-dummy-tls |
| §8-C-1 log conf 본문 | zlog conf 운영 + dev 2 변형 | §8-C 카테고리 표 + tech_stack zlog | plan §5 TASK-log-conf |

#### 8-E-2. 검증 규칙 통과

- §3-C-5/3-C-6/§8-C-1 모두 본 문서에 존재 ✅
- `tech_stack.libraries` 의 파서·라이브러리만 사용 (cJSON JSON / zlog ini / OpenSSL CLI) ✅
- mock-data-generation 이 인용할 MOCK-CFG / MOCK-LOG / MOCK-TLS ID 채번 대상 ✅
- DEF-011 schema-payload 일관성 §3-C-5-4 통과 ✅

## 9. 검증 시나리오

| 시나리오 ID | 시나리오명 | 분류 | 입력 | 기대 결과 | 매핑 기능 ID | 매핑 endpoint·절차 | 매핑 코드 분기 |
|---|---|---|---|---|---|---|---|
| NSSF-VS-0001 | E1 Registration 정상 호출 | 정상 | nf-type=AMF, nf-id=UUID, slice-info-request-for-registration={requestedNssai=[{sst:1}]}, tai={mcc:450,mnc:08,tac:000001} | 200 + AuthorizedNetworkSliceInfo (allowedNssaiList ≥1) | SVC-0002, DAT-0001~0004 | E1 / PROC-0006 | handler_e1 step 5 정상 분기 |
| NSSF-VS-0002 | E1 Requested NSSAI 미지원 → 403 | 이상 | nf-type=AMF, requestedNssai=[{sst:99}] (미지원) | 403 ProblemDetails cause=SNSSAI_NOT_SUPPORTED | ERR-0001 | E1 / PROC-0006 | step 5 db lookup 실패 |
| NSSF-VS-0003 | E1 토큰 만료 → 401 | 이상 | Authorization: Bearer (expired) | 401 cause=TOKEN_EXPIRED | SEC-0003 | E1 | step 1 |
| NSSF-VS-0004 | E1 nf-id 누락 → 400 | 이상 | query 에 nf-id 없음 | 400 invalidParams=[nf-id] | – | E1 | step 3 |
| NSSF-VS-0005 | E1 PDU Session 정상 + HR roaming | 정상 | slice-info-request-for-pdu-session={sNssai, roamingIndication=HOME_ROUTED_ROAMING, homeSnssai}, home-plmn-id=different | 200 + AuthorizedNetworkSliceInfo (nsiInformation + mappingOfNssai) | SVC-0005, DAT-0005/0006 | E1 / PROC-0008 + PROC-0012 | step 5 + SEPP forward |
| NSSF-VS-0006 | E1 PDN Connection (RSIPCE) 정상 | 정상 | slice-info-request-for-pdn-connection=[Snssai], supportedFeatures=RSIPCE | 200 + mappingOfNssai | SVC-0010, DAT-0009/0010 | E1 / PROC-0010 | step 4 (RSIPCE gate) + 5 |
| NSSF-VS-0007 | E1 PDN Connection RSIPCE 미협상 → 400 | 이상 | supportedFeatures=0 | 400 FEATURE_NOT_SUPPORTED | DAT-0009 (M-Not 부정 시험) | E1 / PROC-0010 | step 4 (RSIPCE gate 실패) |
| NSSF-VS-0008 | E1 NWDAF Other Purpose (SIOP) 정상 | 정상 | slice-info-request-for-other-purpose=[Snssai], supportedFeatures=SIOP | 200 + snssaiInfoRspData | SVC-0011, DAT-0011/0012 | E1 / PROC-0011 | step 4 + 5 |
| NSSF-VS-0009 | E1 NRF discover 실패 → partial response | 이상 | nf-type=AMF, registration 시 NRF가 5xx | 200 + targetAmfSet (candidateAmfList 생략) | MGMT-0001 | E1 / PROC-0006 + PROC-0004 | client_nrf_discover_amf 실패 → handler step 5 partial |
| NSSF-VS-0010 | E2 PUT 정상 | 정상 | NssaiAvailabilityInfo (supportedNssaiAvailabilityData 1건) | 200 + AuthorizedNssaiAvailabilityInfo + ENT-0015 Enqueue Notify | SVC-0012, DAT-0013 | E2 / PROC-0013 | handler_e2 정상 |
| NSSF-VS-0011 | E2 PUT S-NSSAI 미지원 → 403 | 이상 | supportedSnssaiList 에 미지원 SST | 403 SNSSAI_NOT_SUPPORTED | ERR-0010 | E2 | step 4 |
| NSSF-VS-0012 | E2 PUT 0개 인증 → 204 | 정상 | 모든 S-NSSAI 가 인증 후 빈 결과 | 204 No Content | DAT-0013 | E2 | step 7 (`authorized.n_data == 0`) |
| NSSF-VS-0013 | E2 PUT Content-Type 불일치 → 415 | 이상 | Content-Type: application/xml | 415 | – | E2 | step 2 |
| NSSF-VS-0014 | E3 PATCH 정상 (add op) | 정상 | PatchDocument=[{op:add, path:"/supportedNssaiAvailabilityData/0/supportedSnssaiList/-", value:{sst:2}}] | 200 + AuthorizedNssaiAvailabilityInfo | SVC-0013, DAT-0015 | E3 / PROC-0013 | handler_e3 정상 |
| NSSF-VS-0015 | E3 PATCH nfId 미존재 → 404 | 이상 | 선행 PUT 없이 PATCH | 404 RESOURCE_NOT_FOUND | DAT-0030, ERR-0010 | E3 | step 5 |
| NSSF-VS-0016 | E3 PATCH JSON Patch 무결성 위반 → 400 | 이상 | path 무효 (`/invalid/path`) | 400 INVALID_IE invalidParams=[path] | SVC-0042 | E3 | step 4 |
| NSSF-VS-0017 | E3 PATCH Content-Type 불일치 → 415 | 이상 | Content-Type: application/json | 415 | – | E3 | step 2 |
| NSSF-VS-0018 | E4 DELETE 정상 | 정상 | 기존 nfId | 204 + ENT-0015 enqueue status change | SVC-0030/0031 | E4 / PROC-0019 | handler_e4 정상 |
| NSSF-VS-0019 | E4 DELETE 미존재 → 404 | 이상 | 미존재 nfId | 404 RESOURCE_NOT_FOUND | ERR-0015, DAT-0031 | E4 | step 5 |
| NSSF-VS-0020 | E5 Subscribe 정상 (4 이벤트) | 정상 | 모든 NssfEventType 구독 | 201 + Location + subscriptionId | SVC-0014, DAT-0017/0018 | E5 / PROC-0014 | handler_e5 정상 |
| NSSF-VS-0021 | E5 Subscribe 모든 이벤트 미지원 → 501 | 이상 | event=UNKNOWN_X (open string), additionalEvents 모두 미지원 | 501 UNSUPPORTED_EVENT_TYPE | DAT-0033 | E5 | step 4 |
| NSSF-VS-0022 | E5 Subscribe nfNssaiAvailabilityUri 누락 → 400 | 이상 | body 에 callbackUri 없음 | 400 MANDATORY_IE_MISSING invalidParams=[nfNssaiAvailabilityUri] | ERR-0011, DAT-0032 | E5 | step 3 |
| NSSF-VS-0023 | E5 동일 expiry 다중 구독 분산 검증 | 정상 (SVC-0016 M-Not 부정형) | expiry=T1 인 구독 N개 동시 생성 | N 모두 201, 단 NSSF 가 expiry 균등 분산 → 동일 timestamp 다수 부여 안 함 | SVC-0016 (M-Not 부정 시험) | E5 / PROC-0014 | step 5 + expiry distribution 로직 |
| NSSF-VS-0024 | E6 PATCH 정상 (SUMOD 협상) | 정상 | SUMOD 비트 협상, taiList 변경 PATCH | 200 + NssfEventSubscriptionCreatedData | SVC-0018/0019, DAT-0024 | E6 / PROC-0014 Modify | handler_e6 정상 |
| NSSF-VS-0025 | E6 PATCH event IE 변경 시도 → 400 | 이상 (SVC-0020 M-Not) | path=`/event` | 400 MODIFICATION_NOT_ALLOWED | SVC-0020 (M-Not 부정), DAT-0025 (M-Not 부정), ERR-0012 | E6 | step 4 |
| NSSF-VS-0026 | E6 PATCH SUMOD 미협상 → 403 | 이상 | supportedFeatures 에 SUMOD 비트 OFF | 403 NOT_AUTHORIZED | SVC-0018 | E6 | step 4 |
| NSSF-VS-0027 | E6 PATCH subscriptionId 미존재 → 404 | 이상 | 미존재 ID | 404 SUBSCRIPTION_NOT_FOUND | ERR-0012 | E6 | step 5 |
| NSSF-VS-0028 | E7 Unsubscribe 정상 | 정상 | 기존 subscriptionId | 204 + 진행 중 알림 작업 cancel | SVC-0021, SVC-0022 | E7 / PROC-0018 | handler_e7 정상 |
| NSSF-VS-0029 | E7 Unsubscribe 미존재 → 404 | 이상 | 미존재 | 404 SUBSCRIPTION_NOT_FOUND | ERR-0013 | E7 | step 5 |
| NSSF-VS-0030 | E8 OPTIONS 정상 | 정상 | OPTIONS /nssai-availability | 200 + Accept-Encoding: gzip | SVC-0032 | E8 | handler_e8 정상 |
| NSSF-VS-0031 | C1 Notify (status change) 정상 | 정상 | PROC-0013 후 자동 트리거 + 매칭 구독 1건 | NSSF→Consumer POST → 204 응답 수신 | SVC-0023/0024, DAT-0026, PRC-0001 | C1 / PROC-0015 | client_notify_send + on_notify_response 204 |
| NSSF-VS-0032 | C1 Notify Consumer 컨텍스트 없음 → 400, 구독 stale | 이상 | Consumer 가 400 RESOURCE_CONTEXT_NOT_FOUND 응답 | 구독 status=STALE, notify_task=permanent_fail, 로그 WARN | DAT-0034, ERR-0014 | C1 / PROC-0015 | on_notify_response 400 case |
| NSSF-VS-0033 | C1 Notify URI 미인식 → 404, 구독 invalidate | 이상 | Consumer 가 404 RESOURCE_URI_STRUCTURE_NOT_FOUND | 구독 status=INVALIDATED | DAT-0035, ERR-0014 | C1 / PROC-0015 | on_notify_response 404 case |
| NSSF-VS-0034 | C1 Notify 5xx → 백오프 재시도 | 이상 | Consumer 가 503 + Retry-After=5 | 5초 후 재시도, 최대 max_retries 후 stale | SVC-0027 | C1 / PROC-0015 | on_notify_response retry case |
| NSSF-VS-0035 | C1 Notify 307 redirect → 새 Location 으로 재시도 | 이상 | Consumer 307 + Location: new_uri | 즉시 new_uri 로 POST | DAT-0036 | C1 / PROC-0015 | 307 case |
| NSSF-VS-0036 | C1 Notify Slice Replacement (NSRP) | 정상 | NWDAF 트리거 → altNssai 포함 Notify | NSSF→Consumer POST → 204 | SVC-0027, PRC-0003 | C1 / PROC-0016 | client_notify_send (replacement payload) |
| NSSF-VS-0037 | C1 Notify Validity Time | 정상 | validity time 변경 트리거 | NSSF→Consumer POST (nssaiValidityTimeInfoList) → 204 | PRC-0002 | C1 / PROC-0017 | client_notify_send (validity payload) |
| NSSF-VS-0038 | C1 Notify EANAN — 빈 배열 | 정상 (EANAN feature) | 모든 슬라이스 미지원 상태 | authorizedNssaiAvailabilityData=[] 통지 | SEC-0002, SVC-0029 (M-Not 부정) | C1 / PROC-0015 | – |
| NSSF-VS-0039 | C1 Notify SEC SHALL NOT 위반 시험 (부정) | 부정 (SEC-0001 M-Not) | Replacement 진행 중인 S-NSSAI 에 대해 AuthorizedNssaiAvailabilityData 송신 시도 | 코드가 송신 거부 (위반 0건) | SEC-0001 (M-Not 부정) | C1 | client_notify_send 사전 필터링 |
| NSSF-VS-0040 | PROC-0001 NRF Register 정상 | 정상 | 기동 시점 | NRF 201 + heartBeatTimer | – | PROC-0001 | M-CLIENT-NRF.register on 201 |
| NSSF-VS-0041 | PROC-0001 NRF 5xx → 백오프 재시도 | 이상 | NRF 503 | 백오프 후 재시도, 무한 retry (운영 알람) | – | PROC-0001 | retry case |
| NSSF-VS-0042 | PROC-0003 Heartbeat 정상 | 정상 | heartBeatTimer 만료 | NRF 204 | – | PROC-0003 | – |
| NSSF-VS-0043 | PROC-0003 NRF 404 → 재등록 | 이상 | NRF 가 등록 잃음 | 자동 PROC-0001 재호출 | – | PROC-0003 | on_heartbeat_response 404 case |
| NSSF-VS-0044 | PROC-0005 Token 정상 발급 | 정상 | cache miss | NRF 200 + token cached | – | PROC-0005 | auth_get_token |
| NSSF-VS-0045 | PROC-0005 Token 401 → invalidate + 재발급 | 이상 | NRF 401 | cache invalidate + 재시도 | – | PROC-0005 | – |
| NSSF-VS-0046 | PROC-0020 Subscription Expiry sweeper | 정상 | expiry < now | 구독 삭제 + 큐 정리 | – | PROC-0020 | timer 콜백 |
| NSSF-VS-0047 | M-Not 부정: SVC-0029 — Update 후 중복 status_change Notify 금지 | 부정 (SVC-0029 M-Not) | PUT 직후 동일 변경 다중 Notify 시도 | NSSF 가 dedupe → 단일 통지만 송신 | SVC-0029 (M-Not 부정) | C1 / PROC-0015 | enqueue dedup 로직 |
| NSSF-VS-0048 | 통합: Initial Registration end-to-end | 정상 시스템 시험 | AMF→NSSF GET (with NRF 발견) → NSSF→NRF disc → 200 응답 | 전체 흐름 ≤ N ms | SVC-0002, MGMT-0001 | PROC-0006 + PROC-0004 | – |
| NSSF-VS-0049 | 통합: NSSAI Update + Notify 사이클 | 정상 통합 | AMF PUT → 매칭 구독자에게 Notify → 204 | 큐 비움 + lastNotifiedAt 갱신 | SVC-0012, SVC-0023 | PROC-0013 + PROC-0015 | – |
| NSSF-VS-0050 | 통합: HR Roaming PDU Session | 정상 통합 | SMF→V-NSSF→SEPP→H-NSSF→ … 응답 chain | V-NSSF 200 OK | SVC-0004 | PROC-0008 + PROC-0012 | – |
| NSSF-VS-0051 | 시스템: restart 후 구독 활성 유지 | 정상 시스템 | NSSF restart | PG → uthash 로드, 구독 활성 유지 | – | PROC-0014 + restart | db_init |

## 10. features.md `매핑 시험항목` 컬럼

본 스킬에서는 채우지 않음. `/generate-test-spec` 워크플로우가 §9 시나리오 ID 표(NSSF-VS-0001~0051)를 입력으로 받아 features.md 의 `매핑 시험항목` 컬럼을 충전한다.

## 11. 자체 완료 체크리스트

| 점검 항목 | 합격 기준 | 결과 |
|---|---|---|
| 0단계 입구 점검 통과 | 4 산출물 + features.md placeholder 모두 통과 | ✅ |
| 4개 산출물 인용 흔적 | features.md / api-analysis / procedure-analysis / db-design 본문 인용 ≥1회 | ✅ (각 ≥10회) |
| 1단계 모듈 카탈로그 점검 | 12 항목 모두 적용/미적용 명기 | ✅ (모두 적용) |
| 2단계 공개 API 표면 표 | 모든 모듈에 작성 | ✅ |
| 3-B 메모리 소유권 컬럼 | C — 명시적 메모리 모델, 모두 채움 | ✅ (§8-A) |
| 4-B 핸들러 골격 8단계 무결성 | 8 핸들러 모두 8단계 또는 명시적 비적용 | ✅ |
| 5-B 클라이언트 의사 코드 분기 무결성 | 5 단계 분기 | ✅ (NRF·NOTIFY·SEPP) |
| 6단계 db CRUD 재정의 부재 | 모든 호출이 db-design §7 작업명과 일치 | ✅ (재정의 0건) |
| 7단계 에러 매트릭스 인용 부재→재작성 0건 | api §6 인용만 | ✅ |
| 8-A/B/C/D 명세 완비 | 4개 모두 | ✅ |
| 8-D-1 Entry function completeness 표 (R7) | M-MAIN/M-LIFE 적용, §8-D-1 표 10 STEP, 의사 코드와 일치, expected_call 빈 셀 0 | ✅ (10/10 STEP, expected_call 모두 채움) |
| 9단계 시나리오 ID 형식 | `NSSF-VS-[0-9]{4}` 정규식 | ✅ (51개, VS-0001~VS-0051) |
| 9단계 부정 시나리오 ≥1 | M-Not/C-Not 행마다 1건 이상 | ✅ (SVC-0016/0020/0025/0029, SEC-0001, DAT-0009 부정 시험 매핑) |
| R11 운영 conf bundle anchor | §3-C-5 + §3-C-6 + §8-C-1 모두 존재, 형식 = tech_stack 파서 결정 (JSON cJSON / ini zlog) | ✅ |
| 라이브러리·언어 비하드코딩 | 모든 라이브러리·경로가 `agent_context.json` 또는 hints 에서 추출 | ✅ |
| NF-무지 검증 | NSSF·서비스명 모두 규격에서 추출 | ✅ |
| 미치환 템플릿 변수 | `{{...}}` 잔존 0건 (산출물 형식 가이드 라인 예외) | ✅ |
