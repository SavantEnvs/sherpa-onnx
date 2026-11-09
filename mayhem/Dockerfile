# syntax=docker/dockerfile:1.7
#
# Commit image for sherpa-onnx: builds ON TOP of the org C/C++ base image.
# Build (from the repo root):
#   DOCKER_BUILDKIT=1 docker build -f mayhem/Dockerfile --build-arg COMMIT=$(git rev-parse HEAD) -t mytag .
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

# Third-party sources upstream's CMake would FetchContent (pinned to the same URLs/hashes as
# cmake/*.cmake), cached once under /opt/toolchains/deps (SPEC §6.2 item 19). utf8proc and googletest are
# built here as static, uninstrumented libraries; Eigen and hclust-cpp are header-only.
USER root
ARG UTF8PROC_VER=2.12.0
ARG UTF8PROC_SHA256=f564011d38b2888d583d510b08e69ffa15aa117155db1b9b49ef1dfe1fa25111
ARG GTEST_VER=1.13.0
ARG GTEST_SHA256=ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363
ARG EIGEN_VER=5.0.1
ARG EIGEN_SHA256=e9c326dc8c05cd1e044c71f30f1b2e34a6161a3b6ecf445d56b53ff1669e3dec
ARG HCLUST_VER=2026-02-25
ARG HCLUST_SHA256=8f14e024c709d73afb40ae69cb22de4b73dba67cbce40f2e518813da8139ab56
RUN set -eux; \
    fetch() { \
      d=/opt/toolchains/deps/$1-$2; mkdir -p "$d/src"; \
      curl -fsSL --retry 3 -o /tmp/dep.tgz "$3"; \
      echo "$4  /tmp/dep.tgz" | sha256sum -c -; \
      tar -xzf /tmp/dep.tgz --strip-components=1 -C "$d/src"; rm -f /tmp/dep.tgz; \
    }; \
    fetch utf8proc "$UTF8PROC_VER" "https://github.com/JuliaStrings/utf8proc/archive/refs/tags/v${UTF8PROC_VER}.tar.gz" "$UTF8PROC_SHA256"; \
    fetch googletest "$GTEST_VER" "https://github.com/google/googletest/archive/refs/tags/v${GTEST_VER}.tar.gz" "$GTEST_SHA256"; \
    fetch eigen "$EIGEN_VER" "https://gitlab.com/libeigen/eigen/-/archive/${EIGEN_VER}/eigen-${EIGEN_VER}.tar.gz" "$EIGEN_SHA256"; \
    fetch hclust-cpp "$HCLUST_VER" "https://github.com/csukuangfj/hclust-cpp/archive/refs/tags/${HCLUST_VER}.tar.gz" "$HCLUST_SHA256"; \
    d=/opt/toolchains/deps/utf8proc-${UTF8PROC_VER}; mkdir -p "$d/include" "$d/lib"; \
    clang -O2 -g -gdwarf-3 -c "$d/src/utf8proc.c" -I"$d/src" -DUTF8PROC_STATIC -o /tmp/utf8proc.o; \
    ar rcs "$d/lib/libutf8proc.a" /tmp/utf8proc.o; cp "$d/src/utf8proc.h" "$d/include/"; \
    d=/opt/toolchains/deps/googletest-${GTEST_VER}; mkdir -p "$d/lib"; \
    for f in gtest/src/gtest-all.cc gtest/src/gtest_main.cc; do \
      clang++ -std=c++17 -O2 -g -gdwarf-3 -c "$d/src/googletest/src/$(basename $f)" \
        -I"$d/src/googletest/include" -I"$d/src/googletest" -o "/tmp/$(basename $f).o"; \
    done; \
    ar rcs "$d/lib/libgtest.a" /tmp/gtest-all.cc.o; ar rcs "$d/lib/libgtest_main.a" /tmp/gtest_main.cc.o; \
    rm -f /tmp/*.o; \
    chmod -R a+rX,a-w /opt/toolchains/deps
USER mayhem

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh — repo ships no functional tests"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
