# NSSF Database Bootstrap (`schema.sql`)

운영자 가이드. 본 README 는 NSSF 가 요구하는 PostgreSQL schema 의 *bootstrap* 절차를 설명한다.

> 본 문서는 인계 산출 ([`infra/nssf/migrations/manifest.yaml`](./migrations/manifest.yaml)) 의 *operator_apply* 절차를 실용 명령으로 풀어 쓴 것이다. policy 결정은 manifest, 실행 절차는 본 README.

## 1. 전제 조건

- PostgreSQL **14+** 가 endpoint 에서 reachable.
- `db.dsn` 이 [`dev/nssf/operator-inputs.yaml`](../../dev/nssf/operator-inputs.yaml) 정책에 따라 주입된 상태 (host / port / database / user / password).
- libpq client (`psql`, `libpq-dev`) 가 적용 호스트에 설치.
- TLS — [`docs/adr/ADR-0004-project-security-baseline.md`](../../docs/adr/ADR-0004-project-security-baseline.md) baseline 의 `sslmode=verify-full` 필수. operator 가 PostgreSQL endpoint cert 의 CA 신뢰 chain 을 사전 검증.

## 2. 1회 적용

```bash
psql -h $POSTGRES_HOST \
     -U $POSTGRES_USER \
     -d $POSTGRES_DB \
     -f infra/nssf/schema.sql
```

본 명령은 *idempotent* — 동일 DSN 에 반복 실행해도 안전 (모든 `CREATE TABLE IF NOT EXISTS` + `INSERT ... ON CONFLICT DO NOTHING`).

## 3. 적용 검증

```sql
-- (a) schema_version row 가 1 로 도달.
SELECT version, description FROM nssf_schema_version ORDER BY version DESC LIMIT 1;
-- 기대 — (1, 'M001-bootstrap — availability/subscription/retry_queue tables')

-- (b) 3 application table 모두 생성.
SELECT to_regclass('public.availability'),
       to_regclass('public.subscription'),
       to_regclass('public.retry_queue');
-- 기대 — 셋 다 non-NULL.

-- (c) 핵심 index 존재.
SELECT indexname FROM pg_indexes
 WHERE tablename IN ('availability', 'subscription', 'retry_queue')
 ORDER BY indexname;
-- 기대 — availability_plmn_tai_idx / subscription_expiry_idx / retry_queue_next_attempt_idx 포함.
```

## 4. 후속 migration

`schema_version` 의 increment 는 별도 forward-only migration script 로 진행한다. M002+ 의 신규 script 는 본 디렉토리 (`infra/nssf/migrations/`) 에 추가 + [`infra/nssf/migrations/manifest.yaml`](./migrations/manifest.yaml) 의 `migrations:` 배열에 등록 후 동일 `psql -f` 절차로 적용한다.

## 5. Rollback 안전선

`M001-bootstrap` 의 rollback (`DROP TABLE`) 은 운영 환경에서 **data loss** 다. 운영 절차는 다음 우선순위.

1. *백업/복원* — 정기 backup 으로부터 시점 복원이 1순위.
2. *forward migration* — 잘못된 스키마는 새로운 migration 으로 정정한다.
3. *명시적 동의* — 운영자가 명시적 동의로만 destructive DDL 실행.

DDL 직접 수정 금지. 모든 schema 변경은 manifest 등록 후 script 로 진행한다.

## 6. CI / 자동화 검증

- `tests/nssf/integration/test_schema_bootstrap.c` — libpq fixture. `POSTGRES_HOST` 등 env 가 주입된 환경에서 `psql -f infra/nssf/schema.sql` 적용 + 본 README §3 의 검증 SQL 실행.
- 본 PR 시점의 readiness-check CI 는 PostgreSQL service container 미포함 — 본 integration test 는 env 미주입 시 *SKIP PASS* 로 회피 (P3 backlog — CI 에 PostgreSQL service 추가).

## 7. Cross-reference

- [`infra/nssf/migrations/manifest.yaml`](./migrations/manifest.yaml) — schema policy 의 단일 진실 출처.
- [`design/nssf/architecture/state-persistence.md`](../../design/nssf/architecture/state-persistence.md) — architecture 단계의 상태 분류 + backend 선정 결정. 본 PR 시점 manifest 와 *table/column 명명 drift* 존재 (architecture draft 가 `nssf_subscriptions` / `nssf_availability` / `nssf_notification_retry_queue`, manifest 가 `subscription` / `availability` / `retry_queue`). drift 정렬은 backlog — Phase 2 진입 전 architecture 문서 update PR 분리.
- [`dev/nssf/failure-recovery.md`](../../dev/nssf/failure-recovery.md) — retry queue 의 at-least-once 정책 + FOR UPDATE SKIP LOCKED dequeue 흐름.
- [`engineering/nssf/dependency-decisions.yaml`](../../engineering/nssf/dependency-decisions.yaml) — `persistence` slot 의 PostgreSQL/libpq 단일 결정.
