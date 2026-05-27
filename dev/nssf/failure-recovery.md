---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_agent_execution_plan: dev/nssf/agent-execution-plan.yaml
generated_date: 2026-05-27
---

# NSSF Failure Recovery — autonomous agent / runtime 장애 시 행동 규칙

본 문서는 *agent 실행 중 장애* 와 *runtime 운영 중 장애* 두 영역의 *최소 복구 contract* 를 명시한다. 상세 retry / circuit-breaker / observability 정책은 Phase 5 (`WI-observability-finalize`) 에서 freeze. 본 문서는 *Phase 1~4 동안 agent 가 멈췄을 때 어떻게 재개* + *runtime 에러가 발생하면 어떤 응답을 emit* 의 단일 진실 출처다.

---

## 1. Agent execution recovery

### 1.1 Checkpoint 시점

`dev/nssf/agent-execution-plan.yaml` §4 `checkpointing.triggers` 의 시점 = `dev/nssf/_implementation_run_state.yaml` 갱신 의무.

- phase 시작 / 완료
- long-running step (cmake build, ctest, openapi-generator 실행) 직전
- blocker 발생 (compile error, test fail, drift check fail, secret 누락)
- context / usage risk (long-running run 의 안전 멈춤)

### 1.2 Resume 절차

1. 다음 agent 가 `dev/nssf/_implementation_run_state.yaml` load.
2. `readiness_generated_at` 가 현재 `dev/nssf/_readiness_status.yaml.generated_at` 와 일치 확인. mismatch → resume 거부.
3. `completed_phases` 의 *다음 phase* 부터 진입 (이미 완료된 phase 는 skip).
4. `spec_reread_audit` 에 `purpose: blocked` entry 가 있으면 즉시 stop + `/nf-readiness <nf>` reopen 안내.
5. 각 phase 의 `required_checks` (verification-matrix.yaml) 전부 재실행 — checkpoint 가 stale 일 가능성.

### 1.3 Blocker resolution

| Blocker 유형 | 즉시 행동 | 후속 |
|---|---|---|
| compile error (`-Werror` violation) | code lane 이 fix. reviewer lane 이 ADR-0004 정합성 확인. | PR 별 commit. |
| test fail (unit/integration/contract) | tester lane 이 root cause 분석 후 code lane 에 redirect. | gap 이 발견되면 `dev/nssf/open-gaps-and-assumptions.md` 갱신. |
| ASan / UBSan trap | code lane 즉시 fix. memory ownership 정책 재확인 (`engineering/nssf/engineering-design.md` language slot). | valgrind 추가 검증 (Phase 5 권고). |
| openapi-generator drift | WI-codegen-bootstrap 재실행. `infra/nssf/codegen/drift-allowlist.yaml` 검토. | drift 가 의도면 allowlist 갱신, 아니면 contract 재생성. |
| operator secret 누락 (DSN / cert / JWKS) | `dev/nssf/operator-inputs.yaml` 의 placeholder 항목 확인. runtime config 에 주입 안내. | 누락 secret 은 *코드 변경 아님* — operator 책임. |
| context exhaustion | 안전 stop + run state checkpoint. 다음 agent 가 resume. | run state 의 `next_step` 명시. |
| spec_reread_audit blocked entry | wrapper 즉시 stop. `/nf-readiness <nf>` 재평가. | readiness pack 보강 후 재진입. |

### 1.4 Handoff artifact 검사

phase 종료 시 handoff 의무 — `dev/nssf/agent-execution-plan.yaml` §4 `handoff_required_artifacts` 참조.

- tracked 산출 (codegen-work-items / team-execution-plan / agent-execution-plan / verification-matrix / verification-plan / api-implementation-matrix / data-model-implementation-map / spec-to-design-coverage / open-gaps-and-assumptions / design/architecture / engineering-design) 모두 commit 됐는지 확인.
- regenerable gitignored 산출 (_readiness_status / _impl_status / _implementation_run_state / _contract_status / _arch_status / _engineering_status) 은 commit 대상 아님. fresh-clone 의 다음 agent 가 `/nf-readiness` + 본 phase 의 status script 로 재생성.

---

## 2. Runtime failure semantics

본 NF runtime 의 *외부 면 응답* 정책. Phase 4 의 `verification-matrix.yaml.phase4_security_baseline_evidence` 와 `phase4_contract_matrix` 가 evidence 수집.

### 2.1 HTTP error contract

| 카테고리 | status | 응답 본문 | 의무 header |
|---|---|---|---|
| TLS handshake 실패 | (connection-level) | — | — |
| OAuth2 token 부재 / 유효성 실패 | 401 | application/problem+json (RFC 7807) | WWW-Authenticate: Bearer realm=, error= |
| OAuth2 scope 불일치 | 403 | application/problem+json | — |
| URI not found | 404 | application/problem+json | — |
| URI too long | 414 | application/problem+json | — |
| unsupported media type | 415 | application/problem+json | Accept-Patch (NSSAIAvailability Patch 에 한정) |
| over rate limit | 429 | application/problem+json | Retry-After |
| backend down (libpq fail, retry queue full) | 503 | application/problem+json | Retry-After |
| unhandled exception | 500 | application/problem+json | — |
| not implemented (Phase 1 stub) | 501 | application/problem+json | — |
| (everything else 5xx) | 500/503 | application/problem+json | — |

상세 cause type 은 `dev/nssf/error-cause-catalog.yaml` 의 18 cause 행 참조.

### 2.2 Database failure

- libpq connection drop → 단발 reconnect 1회, 실패 시 503.
- `retry_queue` full (예 advisory limit) → 429.
- transaction conflict → 단발 retry 1회, 실패 시 503.
- migration mismatch (schema version mismatch) → startup 실패, 즉시 exit code 78 (configuration error).

### 2.3 Outbound dispatch failure (libcurl)

- AMF callback / NRF discovery timeout → retry_queue 에 re-enqueue, NotificationDispatcher 가 backoff 처리.
- DNS 실패 → 503 emit + log warn. NotificationDispatcher 가 retry_queue 에 re-enqueue.
- TLS verify 실패 → log error + drop. 재시도 안 함 (cert 문제는 operator 책임).

### 2.4 Operator misconfiguration

`dev/nssf/operator-inputs.yaml` 의 항목 누락 시 startup 실패 정책.

| 누락 | startup 동작 |
|---|---|
| `tls.server_cert_path` / `tls.server_key_path` | exit 78 |
| `db.dsn` | exit 78 |
| `oauth2.jwks_url` | exit 78 |
| `oauth2.scopes` | exit 78 (필수 2 scope 명시 안 됨) |
| `log.level` | default INFO 로 진행 (non-fatal) |
| `metric.endpoint` | metric off 로 진행 (non-fatal) |

exit code 78 = `EX_CONFIG` (sysexits.h).

### 2.5 Observability emit on failure

- 모든 5xx 응답 = log error + metric `nssf_response_5xx_total{op, cause}` 증가.
- 모든 4xx 응답 = log warn + metric `nssf_response_4xx_total{op, cause}` 증가.
- ASan / UBSan trap (Debug 빌드) = process exit + core dump (CI 에서만).
- Phase 5 `phase5_log_redaction` 정책 — token / cert / user id 등 민감 필드는 redact 후 log.

---

## 3. Cross-references

- `dev/nssf/agent-execution-plan.yaml` — checkpoint / handoff schema.
- `dev/nssf/verification-matrix.yaml` — phase exit gate check id.
- `dev/nssf/error-cause-catalog.yaml` — 18 cause 행 상세.
- `dev/nssf/operator-inputs.yaml` — operator-provided secret placeholder.
- `docs/adr/ADR-0004-project-security-baseline.md` — 7 baseline_mandate.
- `design/nssf/architecture/error-propagation.md` — error propagation 설계.
