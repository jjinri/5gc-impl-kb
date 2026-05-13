# Worker 3 — Core Assumption Verdicts and Option Scoring

- Repo root audited: `/home/jjinri/AI/5gc-impl-kb`
- Team/task: `audit-home-jjinri-ai-8e37f13b` / task `3`
- Worker scope: no-fix methodology/product-strategy audit; no repo implementation edits.
- Output type: evidence summaries with command excerpts; full command output remains in worker transcript.
- Timestamp: 2026-05-11T07:26:14Z

## Evidence base

Commands run from `/home/jjinri/AI/5gc-impl-kb` unless noted:

- `git log --oneline -8` → HEAD `150c64e docs(skills): design/ 디렉터리·신규 gate 이름·build-handoff 자동 호출 반영`; recent sequence includes C3 `644cac6`, C4 `7a79a7e`, C5 `c29ad9b`.
- `git branch -vv` → `main 150c64e [origin/main: ahead 4]`.
- `git status --short` / `git diff --stat` → no tracked or worktree diff printed in audited root during capture.
- Existence checks → present: `.venv/bin/python3`, `design/scripts/build-handoff.py`, `handoff/nssf/_handoff.yaml`, `design/nssf/_manifest.yaml`, `design/nssf/3gpp-ts-29531.md`, `design/scripts/nf-status.py`; missing generated ignored status: `design/nssf/_status.yaml`.
- `python3 design/scripts/nf-status.py nssf --no-write` → PASS 11, FAIL 0, NOT_RUN 1; gates `draft`, `ready_for_review`, `handoff_ready` PASS; `canonical` FAIL only on `implementation_guidance_quality` NOT_RUN.
- Handoff YAML parse summary → `schema_version: handoff-v1`, `coverage.status: complete`, missing categories `[]`, top-level keys include `nf`, `spec`, `api`, `data_model`; counts: `api` 8, `interface` 2, `service_scenarios` 3, `configuration` 4, `error_handling` 2.
- Stale/conflict grep counts over README, CLAUDE, docs, skills, scripts (excluding `.omx` and `.git`) → `kb/`: 31, `implementation_ready`: 12, `production`: 16, `ready_for_review`: 7.
- README evidence → still describes `kb/<nf>/_manifest.yaml`, `kb/<nf>/3gpp-*`, `scripts/<name>.py`, and “software 구현/C struct” framing.
- CLAUDE evidence → still states `kb/` as answer source, `scripts/extract.py`, `kb/<nf>/_archive`, old gates `implementation_ready` and `production`, and “implementation-ready” value language.
- Skill evidence → `.claude/skills/nf-build` references `build-handoff.py`, but template/status examples still include `ready_for_review`; `.claude/skills/nf-status` still says `ready_for_review` in output examples and contains “구현 가능 수준 달성” wording.
- Design page evidence → `design/nssf/3gpp-ts-29531.md` has the expected seven H2 categories and API matrix; frontmatter status remains `draft` despite gates through `handoff_ready` passing.
- Handoff strategy evidence → `handoff/nssf/_handoff.yaml` is self-contained by status check (`$ref` key count 0) and generated from `design/nssf/3gpp-ts-29531.md`.

## Core assumption verdict table

| # | Core assumption | Verdict | Supporting evidence | Counter-evidence / risk | Consequence | What would change the verdict |
|---|---|---|---|---|---|---|
| 1 | Primary consumer is a downstream LLM/dev agent, not a human wiki reader. | PARTIAL | `docs/plan.md` and `docs/handover.md` explicitly define downstream LLM/dev agent and `handoff/<nf>/_handoff.yaml` as contract. `dev/README.md` marks dev as separate placeholder consumer. | README still opens as human-readable “knowledge base/wiki” and emphasizes “software 구현”; CLAUDE still says answer from `kb/` first. | The intent is now clear in planning memory, but onboarding docs still pull agents toward a human wiki / implementation-grade KB frame. | README and CLAUDE become aligned around 5gc-design deliverables and downstream dev-agent consumption; no stale `kb/`/old gate framing remains. |
| 2 | Correct design output is spec-grounded markdown plus self-contained handoff YAML. | PASS | `design/nssf/3gpp-ts-29531.md` exists with seven required categories; `handoff/nssf/_handoff.yaml` exists, parses, has `handoff-v1`, has `coverage.status: complete`, and nf-status passes `handoff_yaml_valid` and `handoff_yaml_self_contained`. | README still documents `kb/` outputs and does not present the handoff contract as the visible pipeline output. `_status.yaml` is ignored/generated and absent until status write. | The product shape is technically proven for NSSF, but documentation lag can cause future runs to regenerate or evaluate the wrong contract. | README/CLAUDE/index/skills describe `design/` + `handoff/` as the contract consistently; status generation policy is explicit. |
| 3 | `design/` owns spec-derived facts; `dev/` owns OS/language/DB/deployment/runtime choices. | PARTIAL | `docs/plan.md` and `docs/handover.md` state the split; `dev/README.md` is only a placeholder; `schema_implementable` is framed as language-neutral sanity probe; C3-C5 assets keep generated handoff under `handoff/`. | README and CLAUDE still use implementation/C struct/production terminology; `implementation_guidance_quality` check name remains dev-colored even if its role is guidance quality; CLAUDE says old KB/scripts paths. | Boundary is recoverable and mostly implemented in scripts, but source-of-truth docs can still instruct agents to cross the design/dev line. | C7-style source-of-truth rewrite removes old implementation/production state language and records dev choices as out-of-scope for design. |
| 4 | Human role stays limited to target NF, source spec copy, and command triggers. | PARTIAL | `docs/handover.md` states the limited human role exactly; existing scripts generate manifest/status/handoff; nf-status produces actionable `to_pass` commands. | `.claude/skills/nf-build` template includes `status: ... # 사람이 갱신`; skill text says service mermaid is manually written; README frames browsing with Obsidian and KB use, implying human reader/editor affordance. | The intended autonomy model is known, but skill text can normalize manual artifact edits that conflict with generated-pipeline discipline. | Skills and CLAUDE remove “human updates generated/body fields” language or confine it to explicit approval/override cases. |
| 5 | Readiness gates measure handoff fitness without pretending implementation is complete. | PARTIAL | `nf-status` uses `draft`, `ready_for_review`, `handoff_ready`, `canonical`; `handoff_ready` PASS proves current handoff fitness; canonical remains blocked only by explicit judge NOT_RUN. | Plan/memory says gate should be `review_ready`, while code/skills currently use `ready_for_review`; README/CLAUDE still contain `implementation_ready` and `production`; skills still use “구현 가능 수준” wording. | Gate mechanics are good enough to protect handoff readiness, but naming/source drift undermines the methodology claim. | Decide and normalize `review_ready` vs `ready_for_review`, remove old gates from docs, and re-run/write `_status.yaml` after canonical judge policy is settled. |

## Option scoring

Scoring rubric: 10 = strongly supported by audited evidence, 0 = contradicted by audited evidence.

| Option | Score | Evidence for | Evidence against | Threshold result |
|---|---:|---|---|---|
| Discard/restart | 1/10 | Stale docs are numerous; source-of-truth drift is real. | No core assumption is FAIL; handoff pipeline exists; NSSF handoff is valid/self-contained; `handoff_ready` passes; recent commits form a coherent migration trail. | Not triggered: fewer than 3 core assumptions fail, and consumer/output/gates are not all fail/unknown. |
| Full overhaul | 8/10 | All five assumptions are PASS/PARTIAL, but at least two contract surfaces conflict: README/CLAUDE still document old `kb/`/implementation/prod framing; skills still expose `ready_for_review` examples and implementation wording; plan/memory says `review_ready` while code says `ready_for_review`. | This should be a bounded source-of-truth/methodology overhaul, not a rewrite from scratch; scripts and generated artifacts are salvageable. | Triggered by plan rule: ≥3 assumptions pass/partial and ≥2 contract surfaces inconsistent. |
| Partial fix | 5/10 | Current technical blockers are small: canonical judge not run; `_status.yaml` absent because generated/ignored; README/CLAUDE C7 cleanup is a known next step. | “Partial fix” risks underreacting to contract drift because authoritative docs, skills, and gate names disagree; future agents may follow stale sources. | Not preferred unless “partial fix” is explicitly defined as source-of-truth normalization across README/CLAUDE/skills/docs plus judge/status refresh. |

## Recommendation from worker-3 lane

Recommend **bounded full overhaul**, not discard/restart. Preserve `design/`, `design/scripts/`, specs, NSSF markdown, and handoff YAML. Treat the overhaul as source-of-truth convergence and audit hardening:

1. Normalize public docs and control docs (`README.md`, `CLAUDE.md`, relevant skills, `docs/plan.md`) around `design/` + `handoff/` and downstream dev-agent consumer.
2. Resolve gate naming drift: choose `review_ready` or `ready_for_review` once, then update code/docs/skills/status examples together.
3. Remove old `kb/`, `scripts/`, `implementation_ready`, and `production` language where it denotes design status or source-of-truth paths.
4. Decide canonical judge policy, run/write status when appropriate, and keep `_status.yaml` generated/ignored if that remains the intended workflow.
5. After cleanup, run a second-NF probe only if confidence remains below the plan threshold or if the source-of-truth cleanup reveals workflow-specific NSSF assumptions.

Confidence: **0.74**. This is high enough to reject discard, but just below the plan’s 0.75 full-overhaul confidence branch because only NSSF was audited end-to-end and a second NF has not been probed.

## Salvage vs discard assets

| Asset | Verdict | Reason |
|---|---|---|
| `specs/` | Salvage | Raw source corpus; no evidence of methodological defect. |
| `design/nssf/3gpp-ts-29531.md` | Salvage with review | Passes structural checks and feeds handoff; frontmatter status/guidance judge still need normalization. |
| `design/scripts/build-handoff.py` | Salvage | Produces valid `handoff-v1` for NSSF; should be tested against another NF later. |
| `design/scripts/nf-status.py` | Salvage with naming cleanup | Gate logic passes handoff readiness; naming drift remains (`ready_for_review` vs plan `review_ready`, `implementation_guidance_quality`). |
| `handoff/nssf/_handoff.yaml` | Salvage | Valid, self-contained, complete coverage according to current checks. |
| README / CLAUDE source-of-truth sections | Overhaul | They are the largest stale-contract source (`kb/`, old gates, implementation/prod framing). |
| `.claude/skills` | Partial overhaul | Paths mostly updated; examples and wording still leak stale gate/manual-edit assumptions. |
| `dev/` | Keep placeholder | Correctly prevents design from absorbing dev responsibilities for now. |

## What would change my mind

- Toward discard/restart: a second NF probe shows the handoff generator or NF lifecycle only works for NSSF through bespoke assumptions; or docs/scripts cannot be normalized without changing the product premise.
- Toward partial fix: leader audit proves README/CLAUDE are the only authoritative stale surfaces and skills/code already converge after a simple naming decision.
- Toward stronger full-overhaul confidence: worker-2 source-of-truth map confirms all conflicts are known C7/C8 cleanup items with no hidden contract owner.

## Subagent evidence

- Subagents spawned: 4 total attempts.
  - Failed launch due role/model mismatch: `019e15e5-cf37-7783-990f-1519c0cfbea5`, `019e15e5-ee54-7631-bfe4-9338f22cf64b`.
  - Retried default `gpt-5.4-mini` read-only probes: `019e15e6-4929-7d41-a4e4-c42eb3d1501a`, `019e15e6-648e-7f20-aaa4-89709592b168`; both remained running past two waits and were shut down without final findings.
- Findings integrated: none from subagents; local command evidence above is the basis for verdicts.
- Serial searches before spawn: 2 repo/state read commands after claim (`task-3.json`/claim/status and initial top-level inventory), satisfying the spawn-before-threshold instruction.
