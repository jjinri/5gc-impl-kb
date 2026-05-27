# 다른 PC 에서 본 repo 작업 시작 — setup

> repo 자체 setup 은 [`README.md`](../README.md) 의 Quick start 가 진실 출처. 본 파일은 *추가로 필요한 user-global 자산* (auto-memory 외) 의 재구성 절차만 다룬다.


## 0. Fresh clone 후 NSSF readiness quick guide

repo 자체 setup 은 `README.md` 의 Quick start 가 진실 출처다. 새 디렉터리에서 NSSF readiness 를 재생성·확인하려면 아래 순서로 시작한다.

```bash
git clone https://github.com/jjinri/5gc-impl-kb.git
cd 5gc-impl-kb

python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

NSSF source spec 이 존재하는지 확인한다. 현재 NSSF baseline 은 아래 파일을 사용한다.

```bash
ls specs/29.531/
# 29531-j60.docx
# TS29531_Nnssf_NSSelection.yaml
# TS29531_Nnssf_NSSAIAvailability.yaml
```

Agent session 에서 repo root 기준으로 public wrapper 를 호출한다.

```text
/nf-readiness nssf
```

기대 stop condition 은 aggregate gate PASS 다.

```text
readiness_pack_ready: PASS
blocked_by: []
```

readiness 상태만 재확인하려면 다음 script 를 사용한다.

```bash
.venv/bin/python3 design/scripts/nf-readiness-status.py nssf --no-write
```

Fresh clone 또는 cleanup 이후 빠른 sanity 검증은 아래 묶음을 사용한다.

```bash
.venv/bin/python3 -m pytest tests/ -q
.venv/bin/python3 design/scripts/validate-artifact-catalog.py
.venv/bin/python3 design/scripts/validate-execution-control-drift.py nssf
.venv/bin/python3 design/scripts/verify-generated-models.py nssf

cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure

bash infra/nssf/codegen/regenerate.sh --check
```

중요 구분: `design/<nf>/_*.yaml`, `design/<nf>/contract/**`, `handoff/<nf>/contract.yaml`, `_*_status.yaml` 은 local cache/status 이므로 fresh clone 에 없거나 cleanup 후 사라질 수 있다. `/nf-readiness nssf` 가 이 cache/status 를 다시 만들고 `readiness_pack_ready` 를 계산한다.

## 1. mattpocock/skills 13개 user-global 설치

사용자가 이전 PC 에서 install 한 외부 skill 묶음. 본 repo 의 `/nf-*` skill 과 무관하지만 `/grill-me`·`/diagnose`·`/tdd` 등 일반 코딩 skill 을 같이 쓰려면 재설치.

```bash
# 1) 영구 clone (한 번만)
mkdir -p ~/.claude/skills-src
git clone --depth 1 https://github.com/mattpocock/skills.git ~/.claude/skills-src/mattpocock-skills

# 2) 권장 13개 symlink
REPO=~/.claude/skills-src/mattpocock-skills
DEST=~/.claude/skills
mkdir -p "$DEST"
for path in \
  engineering/diagnose \
  engineering/grill-with-docs \
  engineering/triage \
  engineering/improve-codebase-architecture \
  engineering/setup-matt-pocock-skills \
  engineering/tdd \
  engineering/to-issues \
  engineering/to-prd \
  engineering/zoom-out \
  engineering/prototype \
  productivity/caveman \
  productivity/grill-me \
  productivity/write-a-skill
do
  name=${path##*/}
  /usr/bin/ln -sfn "$REPO/skills/$path" "$DEST/$name"
done
```

업데이트 — `cd ~/.claude/skills-src/mattpocock-skills && git pull` (symlink 은 자동 반영).
삭제 — `rm ~/.claude/skills/<name>` (원본 보존, 자유).

## 2. 본 repo 의 `_extracted` cache 재생성 (선택)

`specs/<spec>/_extracted/` 는 git tracked 라 clone 으로 따라옴. 그러나 *재추출* 이 필요하면.

```bash
.venv/bin/python3 design/scripts/spec-split.py 29.531 --force
```

## 3. auto-memory 재생성 (선택)

`~/.claude/projects/-home-<...>-5gc-impl-kb/memory/` 는 PC 별 local 이라 clone 후 비어있다. 본 repo 의 [`docs/handover.md`](./handover.md) 에 그 내용이 보존되어 있어, 첫 세션에서 agent 가 그 파일을 읽고 필요 시 memory 로 복원한다. 사람이 직접 cp 해도 되지만 자동 처리에 맡겨도 무방.
