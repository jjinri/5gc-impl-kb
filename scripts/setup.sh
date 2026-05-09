#!/usr/bin/env bash
# llm-wiki 자동 셋업 — venv·pip·extract.py 검증 + 선택 항목 대화형 가이드
set -euo pipefail

ASSUME_YES=0
NO_PROMPTS=0
for arg in "$@"; do
  case "$arg" in
    -y|--yes) ASSUME_YES=1 ;;
    --no-prompts) NO_PROMPTS=1 ;;
    -h|--help)
      cat <<'USAGE'
Usage: bash scripts/setup.sh [--yes] [--no-prompts]

옵션
  -y, --yes      선택 질문에 모두 yes
      --no-prompts   선택 질문 단계 전체 skip (필수 단계만 수행)

사전 조건 (사용자가 직접 실행):
  sudo apt install python3-venv python3-pip
USAGE
      exit 0
      ;;
    *)
      printf '[setup.sh] 알 수 없는 인자: %s\n' "$arg" >&2
      exit 2
      ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

log()  { printf '[setup.sh] %s\n' "$*"; }
ok()   { printf '  [ok] %s\n' "$*"; }
fail() { printf '[setup.sh] ERROR: %s\n' "$*" >&2; exit 1; }

ask() {
  # ask "<prompt>" → 0 if yes, 1 if no
  local prompt="$1"
  if [[ "$NO_PROMPTS" == "1" ]]; then
    printf '  %s [y/N]: N (--no-prompts)\n' "$prompt"
    return 1
  fi
  if [[ "$ASSUME_YES" == "1" ]]; then
    printf '  %s [y/N]: y (--yes)\n' "$prompt"
    return 0
  fi
  local reply=""
  read -r -p "  $prompt [y/N]: " reply || true
  [[ "$reply" =~ ^[yY]$ ]]
}

# ---------- Phase 0: prerequisites ----------
log "Phase 0 — 사전 점검"

if [[ "$(uname -s)" != "Linux" ]]; then
  fail "이 스크립트는 WSL2/Ubuntu 전용입니다 (uname=$(uname -s))."
fi

if [[ ! -r /etc/os-release ]]; then
  fail "/etc/os-release 가 없습니다 — 지원 대상 OS 가 아닙니다."
fi
# shellcheck disable=SC1091
. /etc/os-release
case "${ID:-}:${ID_LIKE:-}" in
  *debian*|*ubuntu*) ok "OS = ${PRETTY_NAME:-Linux}" ;;
  *) fail "Debian/Ubuntu 계열이 아닙니다 (ID=${ID:-?}, ID_LIKE=${ID_LIKE:-?})." ;;
esac

command -v python3 >/dev/null 2>&1 \
  || fail "python3 가 없습니다. 사전 단계: sudo apt install python3"
ok "python3 = $(python3 --version 2>&1)"

# 시스템 pip 은 굳이 검사하지 않는다. python3 -m venv 가 성공하면
# venv 내부에 ensurepip 으로 pip 이 부트스트랩되므로 그것만 사용한다.

# ---------- Phase 1: venv + deps ----------
log "Phase 1 — venv + 의존성"

if [[ -d .venv ]]; then
  if [[ ! -x .venv/bin/pip ]]; then
    fail ".venv 가 있지만 .venv/bin/pip 가 없습니다 (손상된 venv).
  rm -rf .venv 후 다시 실행하세요."
  fi
  ok ".venv 재사용"
else
  if ! VENV_ERR="$(python3 -m venv .venv 2>&1)"; then
    printf '%s\n' "$VENV_ERR" | sed 's/^/    /' >&2
    rm -rf .venv 2>/dev/null || true
    fail "python3 -m venv 실패 (위 메시지 참고).
  보통 다음 사전 설치가 필요합니다:
    sudo apt install python3-venv
  Ubuntu 가 버전별 패키지를 요구할 수 있습니다 (예: python3.12-venv).
  python 의 위 메시지가 정확한 패키지명을 알려줍니다."
  fi
  ok ".venv 신규 생성"
fi

.venv/bin/pip install --quiet --upgrade pip
ok "pip 업그레이드 (.venv 내부)"

.venv/bin/pip install --quiet pypdf python-docx
ok "pypdf, python-docx 설치/확인"

# ---------- Phase 2: extract.py smoke test ----------
log "Phase 2 — extract.py smoke test (.docx 경로)"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

TOKEN="LLM_WIKI_SETUP_PROBE"
PROBE="$TMP/probe.docx"

PROBE_PATH="$PROBE" TOKEN="$TOKEN" .venv/bin/python3 - <<'PY'
import os
from docx import Document
doc = Document()
doc.add_paragraph(os.environ["TOKEN"])
doc.save(os.environ["PROBE_PATH"])
PY

if ! OUT="$(.venv/bin/python3 scripts/extract.py "$PROBE" 2>&1)"; then
  printf '%s\n' "$OUT" >&2
  fail "extract.py 실행 실패."
fi
if ! grep -q "$TOKEN" <<<"$OUT"; then
  printf '%s\n' "$OUT" >&2
  fail "extract.py smoke test 실패 — 토큰 '$TOKEN' 미검출."
fi
ok "extract.py 동작 확인 (probe 토큰 검출)"

# ---------- Phase 3: optional, interactive ----------
log "Phase 3 — 선택 항목"

# 3a — .doc 지원
if command -v soffice >/dev/null 2>&1 \
   || command -v libreoffice >/dev/null 2>&1 \
   || command -v antiword >/dev/null 2>&1; then
  ok ".doc 변환기 이미 설치됨 (soffice/libreoffice/antiword 중 하나)"
else
  echo
  echo "  .doc (legacy MS Word) 처리 도구가 없습니다. 필요하면 별도 셸에서 다음 중 하나를 실행하세요."
  echo "    sudo apt install libreoffice-core   # 권장 (포맷 정확도 높음)"
  echo "    sudo apt install antiword            # 가벼운 대안"
fi

# 3a' — mermaid-cli (Mermaid 다이어그램을 SVG 로 렌더, 선택)
if command -v mmdc >/dev/null 2>&1 || [[ -x .npm-tools/node_modules/.bin/mmdc ]]; then
  ok "mermaid-cli (mmdc) 이미 설치됨"
else
  echo
  if ! command -v npm >/dev/null 2>&1; then
    echo "  Mermaid 다이어그램 → SVG 렌더링이 필요하면 먼저 Node.js+npm 설치가 필요합니다."
    echo "    sudo apt install nodejs npm"
    echo "  설치 후 본 setup.sh 를 다시 실행하면 mmdc 설치 옵션이 나타납니다."
  else
    if ask "Mermaid 다이어그램을 SVG 로 렌더하는 mmdc 를 프로젝트 로컬(.npm-tools) 에 설치할까요?"; then
      echo "  npm install --prefix .npm-tools @mermaid-js/mermaid-cli  ..."
      if npm install --prefix .npm-tools @mermaid-js/mermaid-cli --no-fund --no-audit; then
        ok "mmdc 설치 완료 (.npm-tools/node_modules/.bin/mmdc)"
        echo "  사용법: .venv/bin/python3 scripts/render-mermaid.py [--check] [--clean]"
        echo "  주의 — puppeteer 가 chromium 을 처음 실행할 때 다음 시스템 라이브러리가 필요할 수 있습니다."
        echo "    sudo apt install ca-certificates fonts-liberation libasound2 libatk-bridge2.0-0 \\"
        echo "                     libatk1.0-0 libcups2 libdbus-1-3 libdrm2 libgbm1 libgtk-3-0 \\"
        echo "                     libnspr4 libnss3 libx11-xcb1 libxcomposite1 libxdamage1 \\"
        echo "                     libxfixes3 libxkbcommon0 libxrandr2 xdg-utils"
      else
        echo "  [warn] mmdc 설치 실패. 수동 설치를 시도하세요."
        echo "    npm install --prefix .npm-tools @mermaid-js/mermaid-cli"
      fi
    else
      ok "mmdc 설치 skip"
    fi
  fi
fi

# 3b — checklist.md 동기화 (현 위치 docs/checklist.md, 옛 위치 root checklist.md 도 호환)
CHECKLIST_PATH=""
[[ -f docs/checklist.md ]] && CHECKLIST_PATH="docs/checklist.md"
[[ -z "$CHECKLIST_PATH" && -f checklist.md ]] && CHECKLIST_PATH="checklist.md"
if [[ -n "$CHECKLIST_PATH" ]] && git rev-parse HEAD >/dev/null 2>&1; then
  echo
  if ask "$CHECKLIST_PATH 의 부트스트랩 항목을 자동으로 [x] 로 동기화할까요?"; then
    # 주의: sed 치환부의 `&` 는 매치 전체로 확장되므로 `\&` 로 이스케이프해야 한다.
    sed -i \
      -e 's|^- \[ \] `git init` + 첫 커밋$|- [x] `git init` + 첫 커밋|' \
      -e 's|^- \[ \] `python3 -m venv .venv && .venv/bin/pip install pypdf python-docx`$|- [x] `python3 -m venv .venv \&\& .venv/bin/pip install pypdf python-docx`|' \
      -e 's|^- \[ \] (신규 머신) `bash scripts/setup.sh` 실행 — venv·pip·extract.py 검증 자동화$|- [x] (신규 머신) `bash scripts/setup.sh` 실행 — venv·pip·extract.py 검증 자동화|' \
      "$CHECKLIST_PATH"
    ok "$CHECKLIST_PATH 갱신 (git/venv/pip/setup.sh 항목)"
  else
    ok "$CHECKLIST_PATH 갱신 skip"
  fi
fi

# 3c — Obsidian 안내
echo
echo "  (선택) Obsidian 으로 이 폴더를 Vault 로 열면 [[wikilinks]]·graph·전문검색 사용 가능."
echo "    https://obsidian.md/"

# ---------- 마무리 ----------
echo
log "셋업 완료. 다음 단계 (예: NSSF TS 29.531 첫 문서 등록)"
cat <<'NEXT'
  1. mkdir -p papers/29.531
  2. cp <원본>.docx papers/29.531/                    # 절대 symlink 금지
  3. .venv/bin/python3 scripts/extract.py papers/29.531/<원본>.docx
  4. sources/3gpp-ts-29531-v{ver}.md 와 wiki/nssf/3gpp-ts-29531.md 작성, index.md 갱신
NEXT
