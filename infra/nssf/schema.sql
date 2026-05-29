-- ──────────────────────────────────────────────────────────────────────
-- NSSF Database Bootstrap (M001-bootstrap, schema_version=1)
--
-- 출처 — infra/nssf/migrations/manifest.yaml §2~§4.
-- 적용 — psql -h $POSTGRES_HOST -U $POSTGRES_USER -d $POSTGRES_DB -f infra/nssf/schema.sql
-- 운영자 가이드 — infra/nssf/schema_README.md.
--
-- 본 파일은 idempotent — CREATE TABLE IF NOT EXISTS + INSERT ON CONFLICT.
-- 동일 DSN 에 반복 적용해도 데이터 손상·중복 row 발생 없음. forward only —
-- rollback 은 운영 환경에서 *data loss* 이므로 별도 disaster recovery 절차.
-- ──────────────────────────────────────────────────────────────────────

BEGIN;

-- 1. nssf_schema_version — schema lifecycle 추적.
CREATE TABLE IF NOT EXISTS nssf_schema_version (
    version     INT          PRIMARY KEY,
    applied_at  TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    description TEXT
);

-- 2. availability — NSSAIAvailability source-of-truth.
--    NSSelectionGet 가 read, NSSAIAvailability Put/Patch/Delete 가 write.
CREATE TABLE IF NOT EXISTS availability (
    id                       UUID         PRIMARY KEY,
    plmn_id                  JSONB        NOT NULL,
    tai                      JSONB        NOT NULL,
    snssais                  JSONB        NOT NULL,
    amf_set_id               TEXT,
    reallocation_indication  BOOLEAN      NOT NULL DEFAULT FALSE,
    created_at               TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    updated_at               TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS availability_plmn_tai_idx
    ON availability USING GIN (plmn_id, tai);

-- 3. subscription — NSSAIAvailability subscription source-of-truth.
--    G-10 — expiry_at NULL 시 default 24h 는 application layer 가 enforce.
--    tombstone — soft-delete signal, cascade 정리는 retry_queue FK 가 책임.
CREATE TABLE IF NOT EXISTS subscription (
    id            UUID         PRIMARY KEY,
    callback_url  TEXT         NOT NULL,
    filter        JSONB        NOT NULL,
    expiry_at     TIMESTAMPTZ,
    tombstone     BOOLEAN      NOT NULL DEFAULT FALSE,
    created_at    TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS subscription_expiry_idx
    ON subscription (expiry_at) WHERE tombstone = FALSE;

-- 4. retry_queue — NotificationDispatcher outbound retry.
--    FOR UPDATE SKIP LOCKED 로 dequeue.
--    subscription cascade delete — subscription 삭제 시 pending retry 동시 정리.
CREATE TABLE IF NOT EXISTS retry_queue (
    id                BIGSERIAL    PRIMARY KEY,
    subscription_id   UUID         NOT NULL
                                   REFERENCES subscription(id) ON DELETE CASCADE,
    payload           JSONB        NOT NULL,
    attempt_count     INT          NOT NULL DEFAULT 0,
    next_attempt_at   TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    created_at        TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS retry_queue_next_attempt_idx
    ON retry_queue (next_attempt_at);

-- 5. M001-bootstrap version row — bootstrap 적용 표식 (idempotent).
INSERT INTO nssf_schema_version (version, description)
VALUES (1, 'M001-bootstrap — availability/subscription/retry_queue tables')
ON CONFLICT (version) DO NOTHING;

COMMIT;
