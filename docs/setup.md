# 다른 PC 에서 본 repo 작업 시작 — setup

> repo 자체 setup 은 [`README.md`](../README.md) 의 Quick start 가 진실 출처. 본 파일은 *추가로 필요한 user-global 자산* (auto-memory 외) 의 재구성 절차만 다룬다.

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
