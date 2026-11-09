#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

TB="$SRC/mayhem/build-tests"
PASSED=0 FAILED=0

# 1) Known-answer probe: the output of the (dynamically linked) probe must match literal expected values.
EXPECTED='wave ok=1 sr=16000 n=4 0.0000 0.5000 -0.5000 -1.0000
badwave ok=0 n=0'
if [ ! -x "$TB/kat_probe" ]; then echo "missing $TB/kat_probe (build.sh must build it)" >&2; emit_ctrf "sherpa-onnx-kat" 0 1; exit 1; fi
ACTUAL="$("$TB/kat_probe" 2>/dev/null)"
EXPECTED_LINES=$(printf '%s\n' "$EXPECTED" | wc -l)
i=0
while IFS= read -r want; do
  i=$((i + 1))
  got="$(printf '%s\n' "$ACTUAL" | sed -n "${i}p")"
  if [ "$got" = "$want" ]; then PASSED=$((PASSED + 1)); else
    FAILED=$((FAILED + 1)); echo "KAT line $i: want [$want] got [$got]" >&2
  fi
done <<< "$EXPECTED"
[ "$(printf '%s\n' "$ACTUAL" | wc -l)" -eq "$EXPECTED_LINES" ] || { FAILED=$((FAILED + 1)); echo "KAT line count mismatch" >&2; }

# 2) Upstream's own gtest suites: the run must report real passed counts.
for t in wave-reader; do
  bin="$TB/$t-test"
  if [ ! -x "$bin" ]; then echo "missing $bin" >&2; FAILED=$((FAILED + 1)); continue; fi
  out="$("$bin" 2>&1)"; rc=$?
  p=$(printf '%s\n' "$out" | sed -n 's/^\[  PASSED  \] \([0-9][0-9]*\) test.*/\1/p' | tail -1)
  f=$(printf '%s\n' "$out" | sed -n 's/^\[  FAILED  \] \([0-9][0-9]*\) test.*/\1/p' | tail -1)
  if [ -z "$p" ] || [ "$p" -eq 0 ]; then echo "$t: no passing tests reported (rc=$rc)" >&2; FAILED=$((FAILED + 1)); continue; fi
  PASSED=$((PASSED + p)); FAILED=$((FAILED + ${f:-0}))
  [ "$rc" -eq 0 ] || { [ -n "${f:-}" ] || FAILED=$((FAILED + 1)); printf '%s\n' "$out" | tail -20 >&2; }
done

emit_ctrf "sherpa-onnx-wave-kat+gtest" "$PASSED" "$FAILED" 0
