# ADR-0004 — Project Security Baseline (TLS · mTLS · OAuth2 capability 의무)

Date: 2026-05-21
Status: Accepted (2026-05-21)

> **Accepted** — NSSF 첫 engineering-design freeze 사이클 (PR #31/#32 merge 후) 의 follow-up 검토에서 `tls_security=external` + `oauth2_token_validation=false` frozen 결정이 *project 의 자율 코드 생성 목표* 와 충돌함을 발견. 33.501 을 NF spec lifecycle dependency 로 끌어들이는 대신, 본 ADR 가 *project 차원* 의 security capability 의무를 명시하고 NF 별 engineering-design 산출은 이 의무를 만족시키는 lib/구조 결정만 한다. Pane 2 다회 second-opinion 수렴 결과.

## Context

본 repo 의 최종 목표는 *AI agent 가 자율적으로 완성된 NF 코드를 생성* 하는 것이다 (`ONBOARDING.md` §0, `CONTEXT.md` **자율 코드 생성**). codegen 산출 NF 는 *production-capable* 이어야 한다.

NSSF 첫 사이클의 engineering-design.md 는 `tls_security = external (mesh sidecar 종단)` + `oauth2_token_validation = NF 미구현 (mesh/gateway 외부화)` 로 freeze 됐다 (`engineering/nssf/engineering-design.md` 2026-05-19 ratify). 근거는 *33.501 manifest exclude → mesh 외부화 정책* 인용. 이는 두 가지 문제가 있다.

1. **codegen 산출이 mesh 의존을 강제**. NF 바이너리에 TLS stack 이 없으면 mesh 없는 환경에서 동작 불가 (예 dev profile, peer-to-peer 테스트, edge 배포). production-capable 목표와 충돌.
2. **33.501 결론이 contract 의 hidden default 로 누수**. spec dependency 는 lifecycle extraction 의 source 인데, 33.501 의 결론 (TLS profile/OAuth2 scope) 은 *NF 별 contract* 가 아니라 *project 전체 보안 baseline* 이다. NF 별 manifest exclude 로 처리하면 codegen agent 가 baseline 의 traceability 를 잃는다.

또한 33.310 (SBA certificate profile) / 33.210 (network domain security) 등의 후속 profile spec 까지 끌어들이면 NF lifecycle 의 dependency 가 비대해진다.

## Decision

본 repo 에 *Project Security Baseline* 을 정의한다. 모든 NF 의 codegen 산출은 본 baseline 을 의무 source 로 따른다. 33.501 / 33.310 / 33.210 등 security/profile spec 은 NF 별 `_manifest.yaml` 의 lifecycle extraction dependency 로 추가하지 않으며, 본 baseline 이 그 결론을 흡수한다.

### Baseline 7 항목

1. **내부 HTTPS/TLS code path 의무.** NF 바이너리는 TLS-capable HTTP/2 stack 을 *내부에* 보유한다. 외부 sidecar 의존 단일값으로 close 금지.
2. **mTLS code path 의무.** server/client certificate, CA bundle, peer verification 을 NF 가 *수행할 수 있는* code path 가 존재한다.
3. **inbound OAuth2 bearer token validation code path 의무.** NF 가 inbound access token 을 *검증할 수 있는* code path 가 존재한다.
4. **outbound OAuth2 client credentials / token attach code path 의무.** NF 가 outbound 호출에 access token 을 *부착할 수 있는* code path 가 존재한다 (NRF token endpoint 호출 + AMF callback 등 peer NF 호출 양쪽).
5. **dev/local disable 가능, production-capable path 존재.** 1~4 의 capability 는 config 로 enable/disable 할 수 있으나, *production-capable code path 자체는 항상 존재* 한다. dev profile 에서 disable 했다는 이유로 production code path 를 누락 금지.
6. **third-party library 사용 의무.** TLS / X.509 / JWT / JWS / OAuth2 primitive 는 NF 가 *직접 구현하지 않는다*. maintained library 선택 (예 OpenSSL/BoringSSL/mbedTLS, libcurl, libjwt, jansson) — 후보 비교는 NF 별 engineering-design.md 의 `sbi_server_stack`/`sbi_client_stack`/관련 slot 에서 수행.
7. **profile spec 깊이 외부 처리.** 33.310 (certificate enrollment, CMPv2 등) / 33.210 (cipher suite profile, IPsec 등) 수준의 세부 — *operator-provided compliant cert/config + library compliance assumption* 으로 처리한다. NF 는 cipher suite / TLS version / profile 별 검증을 *직접 결정* 하지 않고, *operator/library default + config policy 로 설정 가능* 하게 둔다. compliance 는 library + operator-provided config 책임.

### Source-of-truth

- 본 ADR = baseline 의무 *source*.
- NF 별 `engineering/<nf>/engineering-design.md` = baseline 을 만족시키는 lib/구조 *결정*.
- NF 별 `design/<nf>/architecture/` = baseline 이 강제하는 flow (inbound HTTPS/h2 + mTLS peer + bearer validation, outbound token acquisition + token attach) 의 구조화.
- NF 별 `dev/<nf>/` = task/test 분해.
- codegen agent = 본 ADR (의무) + engineering-design (선택된 lib) + architecture (flow) + dev (task) 4 source 합집합을 따른다.

### Source precedence (normative)

충돌 시 *normative precedence / enforcement order* (생성 순서가 아니라 강제 순서):

> **ADR-0004 baseline > engineering-design > architecture > dev**

- 하위 산출이 시간상 *먼저 작성* 됐어도 ADR baseline 항목을 *override 못 한다*.
- 하위 산출은 baseline 항목을 *약화* 할 수 없다 — 예: production-capable code path 누락 금지, third-party library 의무 회피 금지, dev disable 을 production 으로 확장 금지.
- 하위 산출은 baseline 을 *강화·세부화* 할 수 있다 — 예: NF 가 추가 cipher suite 제약, NF-specific OAuth2 scope, 더 엄격한 cert validation 요구.

### NF-specific security role

본 baseline 은 *모든 NF 공통 minimum*. NF-specific security role 은 추가될 수 있다 — 예: NRF = OAuth2 token issuer / authorization server, SCP/SEPP = mediation + peer auth gateway, AMF = NAS security context manager. 추가된 role 은 baseline 항목을 *약화* 할 수 없다. NF-specific role 추가는 NF 별 `engineering/<nf>/engineering-design.md` 또는 별 ADR 에서 닫힌다.

### Spec dependency 정책

- 33.501 / 33.310 / 33.210 등 *project-wide security/profile spec* 은 NF 별 `_manifest.yaml.deps.security` 에서 `excluded` reason 으로 *명시 외부화* 한다. excluded reason 은 "project security baseline ADR-0004 으로 흡수, lifecycle extraction dependency 아님" 형식.
- **manifest `present` vs `excluded` 의미 분리** (현 `nf-manifest.py` 는 두 의미를 *섞어* 표기해 정정 대상):
  - `present` = *spec 파일 보유 여부*. `specs/<spec>/` 안에 docx/yaml 가 존재하면 `true`. 사용자가 cp 했는지의 사실 정보.
  - `excluded` / `manual_overrides.exclude` = *lifecycle extraction scope 여부*. 본 spec 을 contract extraction 의 입력으로 *사용* 하느냐.
  - 두 의미는 직교 — 33.501 docx 가 `specs/33.501/` 에 cp 되면 `present=true`, 동시에 ADR-0004 흡수로 `excluded=true` 가능 (가장 정확한 상태). 현재 manifest 의 `present:false + excluded` 표기는 *implementation bug* 로 follow-up PR 에서 정정.
- NF 별 spec discovery (`/nf-spec-discover`) 는 본 정책을 자동 적용한다 — security/profile spec 발견 시 ADR-0004 reference 로 자동 exclude (present 는 사실대로 표기).
- 추후 33.501 본문 인용이 필요한 결정 (예 특정 OAuth2 scope 정의) 은 본 ADR 의 amendment 로 추가한다. NF spec dependency 로 끌어들이지 않는다.

## Considered options

- **33.501 in-scope dependency 추가 (NF spec lifecycle 확장).** contract 가 33.501 본문 어휘를 hidden default 로 주입해 traceability 흐려짐. 33.310/33.210 까지 chain 으로 끌려와 lifecycle 비대. → reject.
- **engineering-design.md amendment 만, project ADR 없음.** baseline 이 NF 별 산출에 분산되면 다음 NF (AMF/NRF/...) 작성 시 재발견 비용·일관성 위험. → reject.
- **baseline 을 CONTEXT.md 만으로 명시.** CONTEXT.md 는 *용어집* (vocabulary doc) — 결정 source 가 아니다. ADR + CONTEXT 어휘 항목 양쪽 필요. → 채택.
- **TLS/OAuth primitive 직접 구현 허용 (FIPS 인증 등 사유).** scope 폭발 + maintained library 가 이미 standard-compliant + 본 repo 의 codegen 목표상 자체 구현은 surface area 과대. → reject (의무 6).
- **production-capable path 를 *선택적* 으로.** dev profile 에서 production code 없이도 OK 면 codegen 산출이 production-not-capable 로 출시될 위험. → reject (의무 5).

## Consequences

- **NSSF engineering-design.md 재-ratify 필요.** `tls_security=external` 단일값 → internal_capable + config-driven 으로 재결정. `oauth2_token_validation=false` 단일값 → configurable inbound validation 으로 재결정. `sbi_server_stack`/`sbi_client_stack` 의 TLS-capable HTTP/2 후보 비교 (server side: nghttp2+TLS backend; client side: libcurl HTTP/2+TLS vs nghttp2 direct+TLS vs nghttp2+libuv+TLS). `configuration_management` row 에 cert/key/CA path, mTLS required flag, OAuth2 token URL/client secret ref/scope 추가.
- **NSSF architecture 갱신 필요.** `configuration-strategy.md` (TLS/mTLS/OAuth config 의 external-only/disabled 인상 정정), `request-flow.md` (inbound HTTPS/TLS → mTLS peer verify → OAuth2 bearer validation flow), `runtime-model.md` (TLS context, cert/key/CA load, token validator/cache init), `error-propagation.md` (cert/token 실패 케이스), `test-strategy.md` (TLS handshake / mTLS peer reject / OAuth2 invalid token 시나리오). `design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md` Open choices 의 TLS/OAuth 관련 row 정리.
- **NSSF dev/<nf>/ 4 파일 정합.** `implementation-plan.md`, `tasks.yaml`, `test-matrix.md`, `traceability.md` 가 새 engineering truth 와 정합.
- **`/nf-spec-discover` 정책 적용.** 새 NF (AMF/NRF/...) 의 spec discovery 가 security/profile spec 을 ADR-0004 reference 로 자동 exclude. NF 별 manifest exclude reason 통일.
- **engineering-core-slots.yaml v3 후보.** 본 ADR 의 의무 5 (dev disable + production path 의무) 는 *모든 NF 의 eng_frozen 의미* 에 영향. `tls_security` / `oauth2_token_validation` slot 의 typed shape 가 *internal-capable 만 PASS* 로 강화되어야 할 수 있다 — profile v3 사이클의 입력 (본 ADR 의 follow-up).
- **history alignment.** 기존 `ONBOARDING.md` / `docs/adr/ADR-0001~0003` / `docs/retros/2026-05-*` 의 "33.501 externalized" / "구현 깊이 0" 취지 문구는 본 ADR 가 정정함을 cross-reference 로 명시 (원본 보존 + 후속 cleanup note).
- **manifest tool update required (follow-up PR).** `design/scripts/nf-manifest.py` 의 `present` vs `excluded` 의미 혼용 정정 — `present` = spec 파일 보유 여부, `excluded` = lifecycle extraction scope 여부 두 독립 필드로 분리. `/nf-spec-discover` 가 security/profile spec 에 ADR-0004 reference reason 을 자동 적용하도록 갱신. 본 PR 는 *정책 명시* 만, tool 구현 반영은 별 PR.
- **codegen 사이클 시작 차단 해소.** baseline 명시 후 NSSF engineering-design 재-ratify 가 eng_frozen 재통과 → codegen 사이클의 의무 source 가 확정된다.
- **transitional 안전 (본 PR merge ~ PR4 사이).** 본 PR merge 후 NSSF `eng_frozen` 은 일시적으로 ADR-0004 와 불일치 상태 (tls_security=external + oauth2_token_validation=false 가 frozen 으로 남음). *codegen 사이클 시작은 PR4 (engineering-design 재-ratify, eng_frozen 재통과) 완료 전까지 blocked* 한다. 본 transitional mismatch 는 의도된 단계적 정정이며 PR4 merge 시 해소.

## Open choices

없음. 본 ADR 의 baseline 7 항목 + spec dependency 정책 + source precedence + NF-specific role 정책 모두 Decided (2026-05-21, Pane 2 second-opinion 수렴).

## Deferred follow-up

본 ADR 의 결정에서 *파생* 되지만 본 ADR 의 책임은 아닌 후속 작업.

| 항목 | 처리 |
|---|---|
| `engineering-core-slots.yaml` v3 typed shape 강화 (`tls_security` / `oauth2_token_validation` slot 의 internal-capable 만 PASS 로) | 별 ADR + profile v3 사이클 |
| `nf-manifest.py` 의 `present` vs `excluded` 의미 분리 + `/nf-spec-discover` 자동 reason 적용 | follow-up PR (Consequences §manifest tool update) |
| NSSF engineering-design.md 재-ratify + eng_frozen 재통과 | PR4 (본 5-PR 사이클의 4 번째) |
| NSSF architecture / dev 정합 | PR2 / PR3 (본 5-PR 사이클) |
| history cross-reference (ONBOARDING / ADR-0001~0003 / retros) | PR5 (본 5-PR 사이클) |

## References

- `CONTEXT.md` — **자율 코드 생성** (목표) · **개발 설계** (NF 별 결정) · **게이트** (구조·의미)
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md` — lifecycle 단계·canonical skill 명명
- `docs/adr/ADR-0002-engineering-design-freeze.md` — Engineering Design Freeze 단계·`eng_frozen` 게이트
- `docs/adr/ADR-0003-engineering-dependency-closure.md` — profile v2 dependency closure
- `engineering/nssf/engineering-design.md` (2026-05-19 ratify) — 본 ADR 가 재-ratify 입력으로 갱신 대상
- `design/nssf/_manifest.yaml` — 33.501 / 38.413 exclude reason 본 ADR reference 로 갱신
- Pane 2 second-opinion 다회 라운드 (2026-05-21) — TLS/mTLS/OAuth capability 의무 + spec dependency 외부화 수렴
