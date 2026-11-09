#!/usr/bin/env bash
#
# mayhem/build.sh — build sherpa-onnx's fuzz harnesses (sanitized) and its unit tests (normal flags).
#
# Mining layer: only the WAVE reader (wave-reader.cc + file-utils.cc) is built, directly from the upstream
# sources; no ONNX Runtime is needed. googletest comes from /opt/toolchains/deps (cached by the
# Dockerfile), so the build works offline.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

DEPS=/opt/toolchains/deps
GTEST=$DEPS/googletest-1.13.0
INC="-I$SRC"
STD="-std=c++17"

LIB_SRCS="wave-reader file-utils"
HARNESSES="fuzz_wave_reader"

# build_lib <outdir> <flags...>: compile the upstream sources into <outdir>/libsherpa_subset.a
build_lib() {
  local out=$1; shift
  rm -rf "$out"; mkdir -p "$out/obj"
  for s in $LIB_SRCS; do echo "$s"; done | xargs -P"$MAYHEM_JOBS" -I{} \
    "$CXX" $STD $INC "$@" -c "sherpa-onnx/csrc/{}.cc" -o "$out/obj/{}.o"
  ar rcs "$out/libsherpa_subset.a" "$out"/obj/*.o
}

# 1) Fuzz build: the fuzzed code itself carries the sanitizers, coverage and DWARF-3 symbols.
FUZZ=$SRC/mayhem/build-fuzz
FUZZ_FLAGS="$SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS -O1"
build_lib "$FUZZ" $FUZZ_FLAGS

"$CXX" $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$FUZZ/lsan_off.o"
"$CC" $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$FUZZ/standalone_main.o"

for h in $HARNESSES; do
  "$CXX" $STD $INC $FUZZ_FLAGS -c "$SRC/mayhem/$h.cc" -o "$FUZZ/$h.o"
  "$CXX" $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$FUZZ/$h.o" "$FUZZ/libsherpa_subset.a" \
    "$FUZZ/lsan_off.o" -o "/mayhem/$h"
  "$CXX" $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS "$FUZZ/$h.o" "$FUZZ/standalone_main.o" \
    "$FUZZ/libsherpa_subset.a" "$FUZZ/lsan_off.o" -o "/mayhem/$h-standalone"
done

# 2) Test build with the project's normal flags (no sanitizers); test.sh only runs these.
TB=$SRC/mayhem/build-tests
build_lib "$TB" -O2 $COVERAGE_FLAGS

link_test() { # <out> <sources...>
  local out=$1; shift
  "$CXX" $STD $INC -O2 $COVERAGE_FLAGS -I"$GTEST/src/googletest/include" "$@" "$TB/libsherpa_subset.a" \
    -L"$GTEST/lib" -lgtest_main -lgtest -lpthread $COVERAGE_FLAGS -o "$out"
}
link_test "$TB/wave-reader-test" "$SRC/sherpa-onnx/csrc/wave-reader-test.cc"
link_test "$TB/kat_probe" "$SRC/mayhem/kat_probe.cc"
